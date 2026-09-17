# SKSE-Template BEST+

Opinionated SKSE plugin template for all of my mods. Builds a single SE/AE DLL on
**CommonLibSSE-NG v8.x**, ships a settings page in **ApocryphaRealm Menu Framework (AMF)**, a
**PrismaUI** web view, a per-mod **logger** and an INI config — and copies itself into the mod
manager on every successful build.

## What is inside

| Piece | Where | Notes |
|---|---|---|
| Build system | `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` | CMake + Ninja + vcpkg (`x64-windows-static`), C++23, IPO |
| SE/AE (VR off) | `CMakeLists.txt` | `ENABLE_SKYRIM_SE/AE`, NG options set with `FORCE` |
| PCH (**required**) | `include/PCH.h` | NG v8's generated TU has no std includes — see *Pitfalls* |
| Logger | `include/Logger.h`, `src/Logger.cpp` | `SKSE\<PRODUCT_NAME>.log` via spdlog, level switchable at runtime |
| Config | `include/Configuration.h`, `src/Configuration.cpp` | `Data\SKSE\Plugins\<PRODUCT_NAME>.ini`, created on first run |
| Translations | `include/Localization.h`, `src/Localization.cpp`, `translations/*.txt`, `tools/make_translations.py` | Skyrim's own `Interface\Translations\<PRODUCT_NAME>_<language>.txt` format, live language switch |
| Menu page | `include/Menu.h`, `src/Menu.cpp` | AMF-native probes + the public SMF consumer header for widgets |
| Web view | `include/PrismaUI.h`, `src/PrismaUI.cpp`, `view/index.html` | soft dependency: no PrismaUI → plugin still works; test window with options (`window.setOption` → INI → `window.applyOptions`) |
| Hotkey | `include/InputSink.h`, `src/InputSink.cpp` | SKSE input sink, toggles the view (default scan code `0x3D` = F3) |
| Vendored headers | `include/vendor/` | `PrismaUI_API.h`, `MenuFramework/SKSEMenuFramework.h`, `AMF/API.h` |

## Requirements on this machine

- Visual Studio 2022 Community with the C++ x64 workload (MSVC 14.44 + Windows SDK 10.0.26100).
- `VCPKG_ROOT` → `C:\Code\libs\vcpkg` (already set as a user variable).
- `COMMONLIB_SSE_FOLDER` → your CommonLibSSE-NG checkout, v8.x (`C:\Code\libs\CommonLibSSE-NG-ng`).
- `SKYRIM_MODS_FOLDER` → the mod manager *mods* folder for auto-deploy (`C:\MO2 Daminikov\mods`).

## New mod from this template

1. Copy the folder (or clone it as the new mod's repo).
2. In `CMakeLists.txt` change `PRODUCT_NAME` (no spaces — it becomes the DLL name and the mod
   folder), `BEAUTIFUL_NAME` (display name used by the menu section and the log line),
   `MOD_AUTHOR`, `MOD_VERSION`, and rename the `project()`-visible name if you like.
3. `git init` and commit; `src/`, `include/`, `view/` are yours to fill in.

## Build

```bash
# in a VS developer prompt, or with vcvars64.bat sourced:
cmake --preset release          # configure (vcpkg installs spdlog/directxtk/rapidcsv/glm/json once)
cmake --build build/release     # -> build/release/<PRODUCT_NAME>.dll
```

`PLUGIN_AUTO_DEPLOY=OFF` (configure option) or an unset `SKYRIM_MODS_FOLDER` turns the copy step off —
useful for compile-only smoke tests that must not touch any game/manager folder.

Deployed layout: `<SKYRIM_MODS_FOLDER>\<PRODUCT_NAME>\SKSE\Plugins\<PRODUCT_NAME>.dll` and
`...\<PRODUCT_NAME>\PrismaUI\views\<PRODUCT_NAME>\index.html`.

## Verify in game

1. Enable the mod in MO2 and launch Skyrim through SKSE.
2. `Documents\My Games\Skyrim Special Edition\SKSE\<PRODUCT_NAME>.log` — the plugin logs the runtime,
   the SKSE version, the resolved menu framework and the PrismaUI view handle.
3. AMF: the page shows up under the mod's name in the framework menu (default `F1`).
4. PrismaUI: the hotkey (default F3) focuses the web view; the button in the view writes to the log.

## Translations (every mod gets them from day one)

`translations/<language>.txt` in the repo becomes `<mod>\Interface\Translations\<PRODUCT_NAME>_<language>.txt`
in the deployed mod — that is Skyrim's own interface-translation location, the one the menu framework
uses for its strings, and the one it scans to build its font atlas: characters from every
`<anything>_<language>.txt` land in the atlas for that language, so Cyrillic / kana / hangul / hanzi
need no font work of your own.

Format is exactly Skyrim's: **UTF-16 LE with BOM, CRLF, one line per string, `$Key<TAB>text`**.
`tools/make_translations.py` generates the files (edit the dict there, run the script) so nobody has to
fight an editor's encoding. Adding a language = one more key in that script.

Three helpers, all run from the template root:

| Tool | What it does |
|---|---|
| `python tools/make_translations.py` | (re)writes the languages defined in the script |
| `python tools/new_language.py german` | creates a new-language stub: every english key, english text, ready to translate |
| `python tools/check_translations.py` | validates format (BOM/UTF-16/CRLF/TAB), missing & extra keys, empty values, untranslated count, smuggled `%` conversions; exit code 1 on problems |
| `python tools/check_view_keys.py` | cross-checks keys between code, the HTML view and `english.txt`, and enforces the key convention |
| `node tools/test_view_i18n.mjs russian` | runs the view's own script against a stub DOM with the real translation file: asserts the texts actually land on screen and the options round-trip works |

### Key convention: `$<Mod>_<Surface>_<Element>`

| Prefix | Surface | Drawn by |
|---|---|---|
| `$MyPlugin_Menu_*` | the settings page | the menu framework (ImGui) |
| `$MyPlugin_View_*` | the web view | HTML/JS (PrismaUI) |
| `$MyPlugin_Notice_*` | in-game notifications | `RE::SendHUDMessage` |

One string belongs to exactly one surface; if both need the same wording, they get two keys with the
same text — cheaper for a translator than guessing where a shared string will fit. Surfaces are
separated by blank lines in the file (`make_translations.py` does it for you).

`translations/README.md` is the page a translator gets: what to change, what never to touch, how to
test it in game.

In code:

```cpp
Loc::Init();                                   // once, on kDataLoaded (after Config::Load)
ImGuiMCP::Text("%s", Loc::Get("$MyPlugin_Settings_DebugLogging"));
Loc::Refresh();                                // cheap; returns true when the player switched language
Prisma::Interop("applyTranslations", Loc::Json());   // push the table into the PrismaUI view
```

- The active language comes from the framework (`AMF_GetLanguage()`); without a framework it is `english`.
- `english` is always loaded as a fallback, so a half-finished translation still shows text, and
  `Loc::Get` logs every key it could not resolve (grep the log for `localization: missing key`).
- In the web view every element carries `data-i18n="$Key"` (or `data-i18n-placeholder`), and
  `window.applyTranslations(json)` fills them in — including on a live language switch.
- Never pass a translated string as a *format* string to the framework's variadic text calls: keep the
  format literal and pass `Loc::Get(...)` as an argument.

## Pitfalls (learned the hard way)

- **NG v8 needs the PCH.** `CommonLibSSE.cmake` generates `__<Target>Plugin.cpp` that includes only
  `REL/Relocation.h` and `SKSE/SKSE.h`, while `REL/Version.h` uses `std::uint16_t`, `std::array`,
  `std::less`, `std::strong_ordering` and `"sv"` literals. Drop
  `target_precompile_headers(... include/PCH.h)` and the build dies inside `REL/Version.h`.
- **The version resource must go through CMake's RC language.** `project(... LANGUAGES CXX RC)` plus the
  configured `.rc` as a source is what embeds `FileVersion`/`ProductName`. Compiling the `.rc` yourself
  with a custom command and passing the resulting `.res` to the target silently makes it an order-only
  dependency — the DLL links fine, and `VersionInfo` is empty. This template already does it right.
- **AMF is not SMF.** AMF is an original framework (Dear ImGui inside) that answers the stock
  `SKSEMenuFramework` module/file probes. Vendor the public SMF consumer header — it works against
  AMF unchanged — and reach for `AMF_*` exports (`include/vendor/AMF/API.h`) for the extras.
- **PrismaUI view paths are relative to `Data`**: `CreateView("<PRODUCT_NAME>/index.html")` resolves to
  `Data\PrismaUI\views\<PRODUCT_NAME>\index.html`, which is exactly what the build copies.
- **Lib licence**: CommonLibSSE-NG v8 is GPL-3.0 **with the Modding Exception** (linking your own mod
  code is allowed and stays yours; NG itself stays GPL). The older CharmedBaryon checkout is MIT.