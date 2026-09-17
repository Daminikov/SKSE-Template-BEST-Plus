# Разбор UselessFenixUtils: что взято в шаблон и что нет

Источник: `https://github.com/fenix31415/UselessFenixUtils` (клон `C:\Code\libs\UselessFenixUtils`),
лицензия **MIT** (Copyright (c) 2019 Ryan-rsm-McKenzie — библиотека выросла из ExamplePlugin-CommonLibSSE),
последний коммит `9ae7244` от 2025-05-08. Зависимости оригинала: `boost-atomic`, `boost-stl-interfaces`,
`glm`, `jsoncpp`, `magic-enum`, `spdlog`, `xbyak`.

## Что там есть

| Часть | Размер | Суть | Вердикт |
|---|---|---|---|
| `_generic_foo_<ID, Fn>::eval(...)` | 15 строк | вызов функции движка по ID Address Library | **берём** |
| `add_trampoline` (xbyak) + `writebytes` | ~25 строк | хук ветки/вызова, патч байтов по ID+offset | **берём** (`Hook` под `__has_include`) |
| `hash` / `hash_lowercase` + литералы `_h` / `_hl` | ~40 строк | djb2-хэши EditorID и имён событий на этапе компиляции | **берём** |
| `fmt::formatter` для `NiPoint3`, `NiQuaternion`, `hkVector4`, `hkQuaternion`, `hkQsTransform` | ~120 строк | печать векторов/кватернионов в лог | **берём** |
| `Geom`: `angles2dir`, `rotate`, `GetHeadingAngle`, `HK2NI`, `raycast`, наведение снарядов, LOS | ~300 строк | геометрия и баллистика | **берём чистую математику**, остальное — нет |
| `SettingsBase` (SimpleIni) + `Hotkeys` | ~140 строк | INI с секциями + хоткей строками «клавиша+модификаторы» | **берём (переписано)** |
| `Random`, `Timer`, `IO`, константный `Map` | ~60 строк | мелкие утилиты | не берём — пишутся по месту в 5 строк |
| `Json` (jsoncpp) + `magic_enum`, `get_formid`, `Plinterp` | ~120 строк | чтение JSON-конфигов, строки вида `Plugin.esp\|0x123` | **берём только форму `Plugin.esp\|0x123`** |
| `notification` / `messagebox` / `vformat` | ~30 строк | всплывашки и форматирование | берём идею, реализация у нас своя (через `Loc` + `SendHUDMessage`) |
| `Behavior` (`MyGraphTraverser`, `LoadBehaviorHelper`, `lookup_node`) | ~40 строк | обход графов поведения hkb | не берём — прикладная анимационная кухня |
| `ImguiUtils`: `ImGuiHelper` — свой оверлей (хуки `Present`, `DispatchInputEvent`, `WndProc`, D3D11-бэкенд ImGui), `ParseKeyFromKeyboard` | ~700 строк | полноценная ImGui-панель внутри игры | **не берём — конфликт** (см. ниже) |
| `DebugRenderUtils`: `draw_line/sphere/capsule` (хук `REL::ID(35565)+0x748`) | ~2 800 строк | отладочная отрисовка в мире | не берём сейчас, держим как опцию на будущее |

## Что взято и где лежит

| В шаблоне | Из оригинала | Что изменено |
|---|---|---|
| `include/engine/Call.h` | `_generic_foo_`, `add_trampoline`, `writebytes` | переименовано в `Engine::Call/Static/Hook/WriteBytes`, добавлена `Engine::HasOffset()` для проверки, что ID есть в versionlib текущего рантайма |
| `include/engine/Format.h` | `fmt::formatter<...>`, `hash`/`_h`/`_hl` | реализации перенесены в заголовок (у оригинала — в .cpp), убраны зависимости от их cpp |
| `include/engine/Math.h` | `clamp/clamp01/lerp`, `angles2dir`, `rotate`, `GetHeadingAngle` | оставлена только чистая математика: `GetHeadingAngle` переписан без вызовов движка (через `NiFastATan2`), `Rotate` поправлен на нормализацию оси (в оригинале `UnitCross` самого на себя) |
| `include/engine/Forms.h` | `Json::get_formid`, `get_mod_index` | переписано на `TESDataHandler::LookupModByName` + `TESFile::GetPartialIndex()` (корректно для ESL/`0xFE`-индексов), parse через `std::optional` |
| `include/Settings.h` | `SettingsBase`, `Hotkeys`, `ReadBool/Int/Float/String` | `SimpleIni` + типизированные геттеры «есть/нет значения», `ParseHotkey("42+61")` и `Hotkey::IsPressed()` |
| `src/Configuration.cpp` | идея `SettingsBase` | INI теперь пишется SimpleIni (секции, комментарии, UTF-8), ключ `sToggleKey` строкой, старый `iToggleKeyScanCode` читается для совместимости |
| `src/InputSink.cpp` | `Hotkeys::isPressed` + `isPressed_adds` | хоткей с модификаторами: `Shift+F3` вместо одного скан-кода |

## Что сознательно не взято

- **`ImguiUtils` — главный отказ.** Это отдельный оверлей: свой `ImGui::CreateContext()`, свои хуки
  `IDXGISwapChain::Present`, `DispatchInputEvent` и `WndProc`. У нас меню рисует ApocryphaRealm Menu
  Framework в своём контексте ImGui. Два ImGui-контекста и два хука Present в одном процессе — драка
  за ввод, курсор и D3D-стейт, поэтому панель берём только у фреймворка, а свои окна делаем через
  PrismaUI или через страницу AMF.
- **`DebugRenderUtils`** — 48 КБ кода и хук по `REL::ID(35565)`: полезно при разработке геймплея
  (рисовать LOS/капсулы), но тащить это в базовый шаблон незачем. Когда понадобится — приносим
  файлом и проверяем ID.
- **Boost, jsoncpp, magic_enum, xbyak в зависимостях.** Из взятых кусков xbyak нужен только для
  `Engine::Hook` (включается сам, если заголовок доступен), остальные три не нужны: JSON у нас уже
  есть (`nlohmann-json`), enum-строки при надобности пишутся руками, boost — вообще лишние 100+ МБ.
  Единственная новая зависимость — `simpleini` (~200 КБ, MIT, header-only).
- **Прикладные куски** (`Behavior`, наведение снарядов, LOS-конусы, `Plinterp`, `Random`) — они
  осмысленны внутри конкретного мода (лучник/ИИ), а не в универсальном каркасе.

## Важное предупреждение про `REL::ID`

Все вызовы движка идут через Address Library, то есть ID резолвится по `versionlib-1-7-104-0.bin`
работающей игры. ID, которых в текущем versionlib нет, превращаются в мусорный адрес — падение без
внятного стектрейса. Поэтому в `Call.h` есть `Engine::HasOffset(id)`:

```cpp
if (!Engine::HasOffset(36444)) { logger::error("ID 36444 unavailable on this runtime"); return; }
```

Код оригинала писался под рантаймы 1.6.x; перед первым использованием любого ID из него — проверять
наличие в versionlib и тестить на живой игре.

## Лицензия

UselessFenixUtils — MIT. Взятые фрагменты переработаны и живут в `include/engine/` и
`include/Settings.h`; в шапке каждого файла стоит ссылка на источник и авторство. При распространении
мода MIT-текст оригинала (и его предка — ExamplePlugin-CommonLibSSE) сохранять не требуется отдельным
файлом, достаточно указания авторства в заголовке, но если правится и публикуется сам
UselessFenixUtils — MIT обязателен.