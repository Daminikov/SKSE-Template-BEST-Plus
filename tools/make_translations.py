#!/usr/bin/env python3
"""Writes the translation files in Skyrim's own interface format:
   UTF-16 LE with BOM, CRLF, lines of  $Key<TAB>text  .

Key convention:  $<Mod>_<Surface>_<Element>
   Menu   - the ApocryphaRealm Menu Framework page (ImGui)
   View   - the PrismaUI web view (HTML/JS)
   Notice - in-game notifications (RE::DebugNotification)
Groups are separated by blank lines; parsers ignore them, translators read them.

Run from the template root:  python tools/make_translations.py
"""
import pathlib

# (surface, key, text) - the order here is the order in the generated file
ENTRIES = [
    ("Menu", "$MyPlugin_Menu_Framework", "Menu framework"),
    ("Menu", "$MyPlugin_Menu_EnablePrismaUI", "Enable the PrismaUI web view"),
    ("Menu", "$MyPlugin_Menu_RegisterPage", "Register this settings page"),
    ("Menu", "$MyPlugin_Menu_DebugLogging", "Verbose logging"),
    ("Menu", "$MyPlugin_Menu_ExampleSlider", "Example slider"),
    ("Menu", "$MyPlugin_Menu_Language", "Active language"),
    ("Menu", "$MyPlugin_Menu_HotkeyHint", "Web view toggle hotkey"),

    ("View", "$MyPlugin_View_Title", "My Plugin - web view"),
    ("View", "$MyPlugin_View_Options", "Options"),
    ("View", "$MyPlugin_View_Option_Hud", "Show the test HUD widget"),
    ("View", "$MyPlugin_View_Option_Sound", "Enable sound"),
    ("View", "$MyPlugin_View_Option_Volume", "Volume"),
    ("View", "$MyPlugin_View_Send", "Send to the plugin"),
    ("View", "$MyPlugin_View_Placeholder", "type something for the plugin log..."),
    ("View", "$MyPlugin_View_Waiting", "waiting for the plugin..."),
    ("View", "$MyPlugin_View_Connected", "plugin connected"),
    ("View", "$MyPlugin_View_Saved", "saved"),

    ("Notice", "$MyPlugin_Notice_OptionsSaved", "My Plugin: options saved"),
]

TRANSLATIONS = {
    "russian": {
        "$MyPlugin_Menu_Framework": "Фреймворк меню",
        "$MyPlugin_Menu_EnablePrismaUI": "Включить веб-окно PrismaUI",
        "$MyPlugin_Menu_RegisterPage": "Регистрировать эту страницу настроек",
        "$MyPlugin_Menu_DebugLogging": "Подробный лог",
        "$MyPlugin_Menu_ExampleSlider": "Пример слайдера",
        "$MyPlugin_Menu_Language": "Активный язык",
        "$MyPlugin_Menu_HotkeyHint": "Клавиша показа веб-окна",

        "$MyPlugin_View_Title": "My Plugin - веб-окно",
        "$MyPlugin_View_Options": "Опции",
        "$MyPlugin_View_Option_Hud": "Показывать тестовый HUD-виджет",
        "$MyPlugin_View_Option_Sound": "Включить звук",
        "$MyPlugin_View_Option_Volume": "Громкость",
        "$MyPlugin_View_Send": "Отправить в плагин",
        "$MyPlugin_View_Placeholder": "введите что-нибудь для лога плагина...",
        "$MyPlugin_View_Waiting": "ожидание плагина...",
        "$MyPlugin_View_Connected": "плагин подключён",
        "$MyPlugin_View_Saved": "сохранено",

        "$MyPlugin_Notice_OptionsSaved": "My Plugin: настройки сохранены",
    },
}


def build(values: dict[str, str]) -> str:
    """english order defines the file; anything untranslated keeps the english text."""
    english = {key: text for _, key, text in ENTRIES}
    parts: list[str] = []
    surface = None
    for group, key, text in ENTRIES:
        if surface is not None and group != surface:
            parts.append("\r\n")  # blank line between surfaces
        surface = group
        parts.append(f"{key}\t{values.get(key, english[key])}\r\n")
    return "".join(parts)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent / "translations"
    root.mkdir(exist_ok=True)

    english = {key: text for _, key, text in ENTRIES}
    for language, values in [("english", english), *TRANSLATIONS.items()]:
        unknown = [key for key in values if key not in english]
        if unknown:
            raise SystemExit(f"{language}: keys missing from the english table: {unknown}")
        path = root / f"{language}.txt"
        path.write_bytes(b"\xff\xfe" + build(values).encode("utf-16-le"))
        translated = sum(1 for key, text in values.items() if text != english.get(key))
        print(f"wrote {path.name}: {len(ENTRIES)} key(s), {translated} translated")

    print("now run: python tools/check_translations.py && python tools/check_view_keys.py")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())