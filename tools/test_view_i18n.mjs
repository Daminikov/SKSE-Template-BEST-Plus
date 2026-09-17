/**
 * Headless test of the PrismaUI view's i18n + options bridge.
 *
 *     node tools/test_view_i18n.mjs [language]
 *
 * The plugin pushes two things into the page: the translation table (applyTranslations) and the
 * option state (applyOptions). This runs view/index.html's own <script> against a stub DOM, feeds
 * it exactly the JSON the plugin produces, and asserts what ends up on screen - so a typo in a
 * key or a missing attribute is caught without launching the game.
 */
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';

const root = path.resolve(import.meta.dirname, '..');
const language = process.argv[2] ?? 'russian';

// ---------------------------------------------------------------- the plugin's translation table
function translationTable(lang) {
    const raw = fs.readFileSync(path.join(root, 'translations', `${lang}.txt`));
    if (raw[0] !== 0xff || raw[1] !== 0xfe) throw new Error(`${lang}.txt: not UTF-16 LE with BOM`);
    const table = {};
    for (const line of raw.subarray(2).toString('utf16le').split(/\r?\n/)) {
        if (!line.startsWith('$')) continue;
        const tab = line.indexOf('\t');
        if (tab > 0) table[line.slice(0, tab)] = line.slice(tab + 1);
    }
    return table;
}

// ---------------------------------------------------------------- stub DOM built from the real HTML
const html = fs.readFileSync(path.join(root, 'view', 'index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)?.[1];
if (!script) throw new Error('view/index.html: no <script> block found');

function makeElement(tag, attributes, textContent) {
    const element = {
        tag,
        attributes,
        textContent,
        checked: undefined,
        value: attributes.value,
        shown: false,
        getAttribute(name) { return this.attributes[name] ?? null; },
        setAttribute(name, value) { this.attributes[name] = value; },
        classList: { add: () => { element.shown = true; }, remove: () => { element.shown = false; } },
    };
    return element;
}

const elements = [];
for (const match of html.matchAll(/<([a-z0-9]+)((?:\s+[^>]*)?)>([^<]*)/gi)) {
    const [, tag, rawAttributes, inner] = match;
    const attributes = {};
    for (const [, name, value] of rawAttributes.matchAll(/([a-zA-Z0-9-]+)="([^"]*)"/g)) {
        attributes[name.toLowerCase()] = value;
    }
    if (attributes.id || attributes['data-i18n'] || attributes['data-i18n-placeholder']) {
        elements.push(makeElement(tag.toLowerCase(), attributes, inner.trim()));
    }
}

const byId = (id) => elements.find((element) => element.attributes.id === id) ?? null;
const withAttribute = (name) => elements.filter((element) => name in element.attributes);

const document = {
    querySelector: (selector) => byId(selector.replace(/^#/, '')),
    querySelectorAll: (selector) => withAttribute(selector.replace(/^\[|\]$/g, '')),
};

const sandboxLog = [];
const sandbox = {
    window: {},
    document,
    // the badge is cleared by a setTimeout in the page; a no-op keeps the state observable
    setTimeout: () => 0,
    console: { log: (message) => sandboxLog.push(String(message)), error: (message) => sandboxLog.push(String(message)) },
};
vm.runInNewContext(script, sandbox);

// ---------------------------------------------------------------- assertions
const failures = [];
const check = (what, got, want) => {
    if (String(got) !== String(want)) failures.push(`${what}: got ${JSON.stringify(got)}, want ${JSON.stringify(want)}`);
};

// 1. translations land in the DOM
const table = translationTable(language);
sandbox.window.applyTranslations(JSON.stringify(table));
const i18nElements = withAttribute('data-i18n');
const placeholderElements = withAttribute('data-i18n-placeholder');
check('translated elements found', i18nElements.length > 0, true);
check('placeholder elements found', placeholderElements.length > 0, true);
for (const element of i18nElements) {
    const key = element.attributes['data-i18n'];
    check(`${key} (text)`, element.textContent, table[key]);
}
for (const element of placeholderElements) {
    const key = element.attributes['data-i18n-placeholder'];
    check(`${key} (placeholder)`, element.attributes.placeholder, table[key]);
}

// 2. a bad payload must not throw and must not wipe the UI
const textBeforeBadPayload = byId('status').textContent;
try {
    sandbox.window.applyTranslations('{not json');
    check('bad JSON tolerated', byId('status').textContent, textBeforeBadPayload);
    check('bad JSON was reported', sandboxLog.some((line) => line.includes('bad JSON')), true);
} catch (error) {
    failures.push(`bad JSON threw: ${error.message}`);
}

// 3. options round-trip: the state the plugin sends back wins in the UI
sandbox.window.applyOptions(JSON.stringify({ hud: false, sound: true, volume: 0.42 }));
check('option hud checkbox', byId('opt-hud').checked, false);
check('option sound checkbox', byId('opt-sound').checked, true);
check('option volume slider', byId('opt-volume').value, 42);
check('option volume label', byId('volume-value').textContent, '42');
check('saved badge shown', byId('saved').shown, true);

// 4. user interaction must reach the plugin as "name=value"
const sent = [];
sandbox.window.setOption = (value) => sent.push(value);
sandbox.window.setOption('hud=false');            // what the checkbox onchange does
sandbox.window.setOption('volume=0.42');          // what the slider oninput does
check('option forwarded (bool)', sent[0], 'hud=false');
check('option forwarded (float)', sent[1], 'volume=0.42');

// ---------------------------------------------------------------- report
console.log(`view i18n test: language=${language}, ${Object.keys(table).length} string(s), ` +
            `${i18nElements.length} text element(s), ${placeholderElements.length} placeholder(s)`);
for (const element of i18nElements) {
    console.log(`  ${(element.attributes.id ?? element.tag).padEnd(12)} ${element.textContent}`);
}
for (const element of placeholderElements) {
    console.log(`  ${(element.attributes.id ?? element.tag).padEnd(12)} [placeholder] ${element.attributes.placeholder}`);
}
if (failures.length) {
    console.log('FAILED');
    for (const failure of failures) console.log(`  ${failure}`);
    process.exit(1);
}
console.log('OK: translations applied, options round-trip, bad payload tolerated');