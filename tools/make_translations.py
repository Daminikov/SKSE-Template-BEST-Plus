#!/usr/bin/env python3
"""Writes the translation files in Skyrim's own interface format:
   UTF-16 LE with BOM, CRLF, lines of  $Key<TAB>text  .
Run from the template root:  python tools/make_translations.py
"""
import pathlib

STRINGS = {
    "english": {
        "$MyPlugin_Settings_Framework": "Menu framework",
        "$MyPlugin_Settings_EnablePrismaUI": "Enable the PrismaUI web view",
        "$MyPlugin_Settings_RegisterPage": "Register this settings page",
        "$MyPlugin_Settings_DebugLogging": "Verbose logging",
        "$MyPlugin_Settings_ExampleSlider": "Example slider",
        "$MyPlugin_Settings_Language": "Active language",
        "$MyPlugin_Settings_HotkeyHint": "Web view toggle hotkey",
        "$MyPlugin_View_Title": "My Plugin - web view",
        "$MyPlugin_View_Send": "Send to the plugin",
        "$MyPlugin_View_Placeholder": "type something for the plugin log...",
        "$MyPlugin_View_Waiting": "waiting for the plugin...",
        "$MyPlugin_View_Connected": "plugin connected",
    },
    "russian": {
        "$MyPlugin_Settings_Framework": "Фреймворк меню",
        "$MyPlugin_Settings_EnablePrismaUI": "Включить веб-окно PrismaUI",
        "$MyPlugin_Settings_RegisterPage": "Регистрировать эту страницу настроек",
        "$MyPlugin_Settings_DebugLogging": "Подробный лог",
        "$MyPlugin_Settings_ExampleSlider": "Пример слайдера",
        "$MyPlugin_Settings_Language": "Активный язык",
        "$MyPlugin_Settings_HotkeyHint": "Клавиша показа веб-окна",
        "$MyPlugin_View_Title": "My Plugin - веб-окно",
        "$MyPlugin_View_Send": "Отправить в плагин",
        "$MyPlugin_View_Placeholder": "введите что-нибудь для лога плагина...",
        "$MyPlugin_View_Waiting": "ожидание плагина...",
        "$MyPlugin_View_Connected": "плагин подключён",
    },
}

root = pathlib.Path(__file__).resolve().parent.parent / "translations"
root.mkdir(exist_ok=True)

for language, entries in STRINGS.items():
    out = root / f"{language}.txt"
    body = "".join(f"{key}\t{value}\r\n" for key, value in entries.items())
    out.write_bytes(b"\xff\xfe" + body.encode("utf-16-le"))
    print(f"wrote {out} ({len(entries)} strings, utf-16le + BOM, CRLF)")