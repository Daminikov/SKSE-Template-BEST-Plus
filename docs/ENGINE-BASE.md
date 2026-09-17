# Движковая база шаблона: что взято из UselessFenixUtils и как этим пользоваться

Источник: `https://github.com/fenix31415/UselessFenixUtils` (MIT), клон `C:\Code\libs\UselessFenixUtils`.
Полный разбор «что там есть → что берём → что нет» — в `ANALYSIS-FENIX.md`. Этот файл — практическое
руководство: что за инструмент, зачем он, как выглядит в коде, где применяется в модах и где грабли.

---

## 1. `include/engine/Call.h` — вызов внутренностей движка по ID

**Проблема, которую это решает.** CommonLibSSE-NG оборачивает многое, но не всё. Функций вроде
`PushActorAway`, `PlaceAtMe` (VM-версия), `CombatUtilities::GetAimAnglesFromVector`,
`Actor::GetActorValueModifier` в заголовках нет. Но у каждой функции движка есть **ID в Address
Library**, и по нему её можно вызвать: `REL::Relocation<Fn>{ REL::ID(id) }` даёт вызываемый указатель.
Ровно это и делает `_generic_foo_` из UselessFenixUtils — их `Impl`-неймспейс это по сути каталог
ID: `PushActorAway = 38858`, `PlaceAtMe(VM) = 55672`, `GetActorValueModifier = 37524`,
`BGSImpactManager::GetSingleton = 515123`.

```cpp
// Объявляем сигнатуру так, как её видит движок (arg-типы должны совпадать!)
float GetHeadingAngle(RE::TESObjectREFR* a_refr, const RE::NiPoint3& a_pos, bool a_absolute)
{
    return Engine::Call<36444, decltype(GetHeadingAngle)>::eval(a_refr, a_pos, a_absolute);
}
```

**Глобалы** (то, что движок держит статикой) — через `Engine::Static`:

```cpp
// игровое время, к которому привязаны таймеры ИИ и регенерации
static REL::Relocation<float*> kGameTime{ REL::ID(517597) };
const float now = *kGameTime;
```

**Патч байтов** — когда нужно выключить проверку или заменить константу:

```cpp
// 0x90 — NOP; Offset отсчитывается от начала функции по её ID
Engine::WriteBytes<40314, 0x1A>(std::array<std::uint8_t, 5>{ 0x90, 0x90, 0x90, 0x90, 0x90 });
```

**Хук ветки с самодельной заглушкой (xbyak)** — когда цель известна только по ID, а обёртки нет:

```cpp
// add "xbyak" в vcpkg.json, затем:
#include <xbyak/xbyak.h>
Xbyak::CodeGenerator code;
code.mov(rax, reinterpret_cast<uintptr_t>(&MyHook));
code.jmp(rax);
Engine::Hook<5, 36581, 0x1C>(&code);   // 5 = размер перезаписываемого jump
```

**Где это нужно на практике:** перки на отбрасывание (`PushActorAway`), кастомные заклинания и
резисты, изменение формул урона/регенерации, перехват функций, у которых нет vtable-обёртки,
правка игровых констант без ESP.

**ГЛАВНАЯ ГРАБЛЯ — ID нельзя «проверить».** `REL::ID(id).offset()` при отсутствии ID в versionlib
вызывает `stl::report_and_fail()` — то есть **валит игру** с окном «Failed to find the id within the
address library: NNN». Возврата нуля там нет. Практика:

1. Берите ID только из источника, который работает на **вашем рантайме** (исходник другого мода,
   дамп ID, просмотрщик versionlib). Код, написанный под 1.6.x, для 1.7.104 — лотерея.
2. Каждый ID-вызов держите за флагом в конфиге, чтобы отключать без пересборки.
3. Если игра упала с этим окном — число в нём и есть виновник: заменить или убрать вызов.
4. Обёртка из CommonLibSSE-NG всегда лучше сырого ID: обёртки проверены экосистемой.

---

## 2. `include/engine/Format.h` — печать RE-типов и хэши строк

**Печать.** `NiPoint3` NG форматирует сам, а кватернионы и Havok-типы (`hkVector4`, `hkQuaternion`,
`hkQsTransform`) он не умеет — их добавили мы. Раньше это выглядело так:

```cpp
// было: руками раскладывать в поля
logger::info("pos x={} y={} z={}", p.x, p.y, p.z);

// теперь:
logger::debug("actor at {} facing {:.1f}", refr->GetPosition(), refr->GetAngleZ());
```

**Хэши и литералы `_h` / `_hl`.** djb2-хэш от строки на этапе компиляции. Смысл — сравнивать имена
без `strcmp` в горячих местах, где движок каждый кадр присылает строку. Типовой случай: анимационные
события (`RE::BSAnimationGraphEvent` имеет `tag` и `payload`), EditorID, имена переменных графа
поведения.

```cpp
// в обработчике анимационных событий (event->tag — имя события, event->payload — аргумент)
switch (Engine::HashLowercase(event->tag.c_str(), strlen(event->tag.c_str()))) {
case "staggerStart"_hl:   /* сработало оглушение */            break;
case "attackStop"_hl:     /* закончилась анимация атаки */    break;
case "IdleStop"_hl:       /* закончился idle */               break;
}
```

Обычный `_h` — регистрозависимый (удобно для ключей словарей), `_hl` — регистронезависимый (удобно
для сигнатур движка).

**Грабли:** NG уже сам специализирует `fmt::formatter` для `NiPoint3`, `NiColor(A)`, `BSFixedString`,
`FormType`, `ActorValue`, `COL_LAYER`, `MATERIAL_ID`, `EffectArchetype`. Своя специализация для них
даёт `C2766: explicit specialization; already defined`. Ещё: у `RE::hkQuaternion` данные лежат в
`vec` (`hkVector4`), а не в `quad`.

---

## 3. `include/engine/Math.h` — математика без обращений к движку

Всё здесь — чистая арифметика, падать нечему: `Clamp`, `Clamp01`, `Lerp`, `AnglesToDirection`,
`Rotate`, `HeadingAngle`.

```cpp
// угол между «лицом» NPC и игроком в градусах — классика для «удара в спину» / бэкстаба
const float angle = Engine::HeadingAngle(npc, player->GetPosition(), /*absolute=*/true);
if (angle > 120.0f) {
    // игрок за спиной: множитель урона, спецэффект, перк
}

// направить «взгляд» NPC на точку: угол в радианах из вектора
const RE::NiPoint3 toPlayer = player->GetPosition() - npc->GetPosition();
npc->SetAngleZ(std::atan2(toPlayer.x, toPlayer.y));

// вектор из углов (пригодится для спавна чего-либо перед актором)
npc->SetPosition(npc->GetPosition() + Engine::AnglesToDirection(npc->GetAngle()) * 100.0f);
```

**Уточнение по реализации:** в оригинале `GetHeadingAngle` была вызовом движка по ID, а `Rotate`
нормировала ось через `UnitCross` сама на себя (то есть получала ноль). В шаблоне `HeadingAngle`
считается через `NiFastATan2` без обращения к движку, а `Rotate` нормирует ось честно.

---

## 4. `include/engine/Forms.h` — формы по «плагин + локальный ID»

**Зачем.** Жёстко прописанный FormID вида `0x01123456` ломается, как только меняется порядок
загрузки: старший байт — это индекс плагина в загрузке. Правильно адресовать форму так:
`"Плагин.esp|0x001234"` (локальный ID из вашего ESP), тогда индекс подставляется по факту.

```cpp
// перк/заклинание/сообщение из своего ESP — устойчиво к порядку загрузки
if (auto* perk = Engine::Forms::Lookup<RE::BGSPerk>("MyMod.esp|0x813")) {
    player->AddPerk(perk, 1);
}

// сырой FormID, когда форма ванильная и индекс уже известен не будет меняться
auto* spell = Engine::Forms::Lookup<RE::SpellItem>("Skyrim.esm|0x00012FCD");

// только индекс плагина (для ручной сборки FormID)
if (auto index = Engine::Forms::ModIndex("MyMod.esp")) {
    const RE::FormID id = (*index << 24) | 0x000813;
}
```

Работает и для ESL: `TESFile::GetPartialIndex()` возвращает `0xFE000 | smallIndex`, поэтому
light-плагины адресуются корректно (это то, что в оригинале делалось вручную).

**Грабли:** при отсутствии формы функция пишет `logger::warn("form lookup: ...")` и возвращает
`nullptr` — всегда проверяйте результат; если мод грузится после вашего ESP, форма может быть ещё
не загружена на `kDataLoaded` (используйте `kPostLoadGame`/`kNewGame` для игровых объектов).

---

## 5. `include/Settings.h` + `Configuration.cpp` — INI и хоткеи

Что изменилось по сравнению с первым вариантом шаблона: вместо WinAPI `GetPrivateProfileString`
используется **SimpleIni** (header-only, MIT): появились секции, комментарии и UTF-8, а INI
перестал зависеть от кода страницы Windows.

```cpp
Settings::Ini ini;
ini.Load("Data\\SKSE\\Plugins\\MyMod.ini");

bool enabled = true;
if (ini.GetBool("General", "bEnableFeature", enabled)) {
    // ключ был — значение прочитано
} else {
    // ключа нет — оставляем своё значение по умолчанию
}
```

Ключевое отличие от «просто чтения с дефолтом»: геттеры возвращают `bool` — **есть значение или
нет**. Это позволяет отличать «пользователь выключил» от «ключа нет, ставим дефолт».

**Хоткей строкой.** В INI живёт `sToggleKey=61` (F3) или `sToggleKey=42+61` (Shift+F3) — скан-коды
DirectInput через `+`, порядок не важен:

```cpp
const auto hotkey = Settings::ParseHotkey(Config::Get().toggleKey);
// hotkey.key / shift / ctrl / alt — скан-коды; в логе видно разобранный результат
if (scanCode == hotkey.key && down) { /* и модификаторы проверяются по состоянию из event sink */ }
```

**Почему модификаторы считаются из событий, а не опросом клавиатуры.** `GetKeyboard()->IsPressed()`
ломает линковку (см. раздел 7) — поэтому `InputSink` ведёт состояние Shift/Ctrl/Alt из потока
событий и проверяет его в момент нажатия основной клавиши.

**Куда попадает INI при игре через MO2:** запись в `Data` MO2 перенаправляет в
`<инстанс>\overwrite\SKSE\Plugins\<Мод>.ini`. То есть файл не в папке мода — если нужно, чтобы
настройки жили рядом с модом, делайте отдельный «файл-конфиг мода» в своей папке.

**Миграция:** старые сборки писали `iToggleKeyScanCode=61` числом. `Configuration::Load` читает новый
`sToggleKey`, а при его отсутствии берёт старое числовое поле — существующие конфиги не ломаются.

---

## 6. Как это собирается в реальный мод (пример на перке «удар в спину»)

```cpp
// 1) форма из своего ESP — устойчиво к порядку загрузки (engine/Forms.h)
static auto* perk = Engine::Forms::Lookup<RE::BGSPerk>("MyMod.esp|0x813");

// 2) проверка геометрии — чистая математика (engine/Math.h)
const float behind = Engine::HeadingAngle(victim, attacker->GetPosition(), true);

// 3) реакция движка там, где обёртки нет (engine/Call.h)
if (perk && victim->HasPerk(perk) && behind > 120.0f) {
    Engine::Call<38858, decltype(PushActorAway)>::eval(victim->GetAIProcess(), victim, &pos, 1.5f);
}

// 4) сообщение игроку — строка из translations/ (Localization.h)
RE::SendHUDMessage::ShowHUDMessage(Loc::Get("$MyMod_Notice_Backstab"));

// 5) всё это — только если включено в INI (Settings.h)
```

---

## 7. Грабля, на которую я наступил при переносе

`RE::BSInputDeviceManager::GetSingleton()->GetKeyboard()->IsPressed(code)` — компилируется, но при
линковке падает 14 ошибками `LNK2001`: NG кладёт в `CommonLibSSE.lib` объектники
`BSKeyboardDevice.cpp.obj` и `BSWin32KeyboardDevice.cpp.obj`, которые ссылаются на виртуальные методы
`RE::BSInputDevice`, определённые **только в самой игре**. Правило: не опрашивать классы устройств,
не инстанцировать RE-классы, чьи виртуалы живут в бинарнике игры, — состояние ввода брать из event sink.

---

## 8. Что НЕ взято и когда вернуться

| Не взято | Почему | Когда вернуться |
|---|---|---|
| `ImguiUtils` (ImGui-оверлей: свои хуки `Present`, `DispatchInputEvent`, `WndProc`) | меню уже рисует AMF в своём контексте ImGui; два контекста и два хука Present дерутся за ввод/курсор/D3D | фактически никогда — для UI есть страница AMF и PrismaUI |
| `DebugRenderUtils` (`draw_line/sphere/capsule`) | 48 КБ + хук по ID; полезно только при отладке геймплея в мире | когда понадобится визуально проверять лучи, хитбоксы, зоны |
| `Behavior` (графы hkb, `lookup_node`) | прикладная анимационная кухня | если мод начнёт править поведение/события графов |
| `Json`/`magic_enum`, `Random`, `Timer`, `IO`, `Plinterp` | заменяются пятью строками по месту или уже есть (nlohmann-json) | по необходимости, точечно |