# HANDOFF — Layout Shadow мод (Geode / GD 2.2081)

> Этот документ — передача контекста другой сессии ИИ. Прочитай **readme.md** в этой же папке (это исходный ТЗ), затем этот файл, затем проверь реальные файлы кода.

## 📌 Суть задачи (одним абзацем)

Мод для Geometry Dash 2.2081 на **Geode SDK 5.7.1**. На мониторе игрока уровень показывается в **layout-режиме XDBot** (голубой фон, белые силуэты solid/hazard, без декораций). В **OBS** при записи тот же уровень показывается **полностью** (все декорации) — через **второй (shadow) PlayLayer**, рендерящийся в offscreen render texture и отдающий кадры в OBS через **Spout2**. Архитектура «Вариант B»: два PlayLayer в одном процессе. См. `readme.md` раздел «АРХИТЕКТУРА».

Исходники XDBot, чей layout-код вендорим: `../XDBotFork-main/src/hacks/layout_mode.cpp` и `.hpp`.

---

## 🗂️ Структура проекта (`D:\claude\another\layout\`)

```
readme.md            ← ИСХОДНОЕ ТЗ (читать первым)
HANDOFF.md           ← этот файл
mod.json             ← Geode 5.7.1, gd.win=2.2081, id=laymod.shadow
CMakeLists.txt       ← ⚠️ НЕ ПОДКЛЮЧАЕТ новые файлы (см. БЛОК 1)
.github/workflows/build.yml
src/
  main.cpp           ← хуки PlayLayer + GJBaseGameLayer
  ShadowManager.hpp  ← владелец shadow-слоя
  ShadowManager.cpp  ← create/destroy/lockstep/render
  Spout.hpp          ← обёртка Spout2 (runtime LoadLibrary)
  Spout.cpp          ← Windows-реализация + stub для не-Windows
  LayoutGate.hpp     ← per-layer gate (thread_local bool)
  LayoutGate.cpp
  layout_mode.hpp    ← vendored из XDBot (ID-таблицы + структура)
  layout_mode.cpp    ← vendored getModifiedString + хуки, gated через LayoutGate
```

---

## ✅ Что уже сделано (статус по шагам из readme «ПОРЯДОК РЕАЛИЗАЦИИ»)

| Шаг | Статус | Где |
|-----|--------|-----|
| **1.** Стабильный второй PlayLayer offscreen (create/destroy) | ✅ ГОТОВО, билдится | `ShadowManager.cpp::create/destroy`, `main.cpp` хуки init/setupHasCompleted/onExit |
| **2.** Зеркало ввода + единый тайминг + debug-проверка | ✅ НАПИСАНО (не тестировалось вживую) | `ShadowManager.cpp::onPrimaryPostUpdate/applyMirrorInput/checkSync` |
| **3.** Рендер SHADOW в render texture | ✅ НАПИСАНО (не тестировалось) | `ShadowManager.cpp::renderShadowToTexture` |
| **4.** Layout XDBot только на PRIMARY (per-layer gate) | ✅ НАПИСАНО (не тестировалось) | `LayoutGate.hpp/cpp`, `layout_mode.cpp` |
| **5.** Индикаторы (FPS/CPS) в shadow-проходе | ❌ НЕ СДЕЛАНО | — |
| **6.** Spout2 | ✅ НАПИСАНО, ⚠️ НАПИСАНО С ПРЕДПОЛОЖЕНИЕМ об именах экспортов Spout (см. БЛОК 4) | `Spout.cpp` |
| **7.** Полировка ЖЦ (меню/редактор/пауза/рестарт, аудио только из PRIMARY) | ⚠️ ЧАСТИЧНО: reset/onExit есть; пауза/аудио НЕ обработаны | `main.cpp` |

**Главное: код написан и должен билдиться, но НЕ тестировался вживую в игре.** Ожидаются краши/баги при первом прогоне — см. блоки ниже.

---

## 🔥 БЛОК 1 (КРИТИЧНО, СДЕЛАТЬ ПЕРВЫМ): CMake не подключает новые файлы

`CMakeLists.txt` сейчас содержит:
```cmake
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ShadowManager.cpp
)
```
Заменить на:
```cmake
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ShadowManager.cpp
    src/Spout.cpp
    src/LayoutGate.cpp
    src/layout_mode.cpp
)
```
Без этого — linker errors (undefined symbols для `LayoutGate::shouldApply`, `LayoutMode::getModifiedString` и т.д.).

**Также** под Windows нужно прилинковать `d3d11`/`dxgi` для `Spout.cpp`. В конец CMakeLists.txt добавить:
```cmake
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxgi)
endif()
```
Уже есть блок `if (MSVC)` с `/bigobj` — он **оставить**, `/bigobj` нужен для огромных ID-таблиц в `layout_mode.cpp`.

### mod.json: убрать android/mac

Текущий `mod.json` содержит только `"win": "2.2081"` — это **правильно**, мод Windows-only. Не добавлять `android`/`mac` (мы ушли от мультиплатформы чтобы избежать «unable to load platform binary»).

### build.yml: ок как есть

`.github/workflows/build.yml` уже настроен win-only (нет matrix), использует `geode-sdk/build-geode-mod@main` + `upload-artifact@v4`. Менять **не нужно**.

---

## 🧪 БЛОК 2: Что протестировать (по шагам readme)

После фикса CMake — собрать и проверить последовательно. **Не прыгать вперёд, пока предыдущий шаг не подтверждён в логе Geode.**

### Шаг 1 (уже работал раньше): второй PlayLayer
- Зайти в уровень → лог `[Shadow] created shadow PlayLayer <addr> for level '<name>'`
- PRIMARY (экран) ведёт себя штатно — синглтон `GameManager::m_playLayer` восстановлен на PRIMARY после `PlayLayer::create` shadow
- Выйти в меню → `[Shadow] destroying shadow PlayLayer`, без краша

### Шаг 2: синхронизация
- Оба игрока (PRIMARY p1 и SHADOW p1) должны идти **кадр-в-кадр**
- В логе не должно быть `[Shadow/SYNC] drift` (если есть — тайминги разъехались, см. БЛОК 3)
- При смерти/рестарте — `[Shadow] reset` синхронно (хук `resetLevel`)

### Шаг 3: рендер в текстуру
- Сейчас при включённом Spout текстура рендерится, но **не выводится на экран для сверки** (readme шаг 3 просил временный онскрин-вывод). Добавить спрайт `m_rt->getSprite()` как child PRIMARY для визуальной проверки, либо проверить через Spout в OBS.

### Шаг 4: layout только на PRIMARY
- PRIMARY = голубой фон, белые силуэты, без декораций
- SHADOW (текстура) = полный уровень со всеми декорациями
- Ключевой механизм: `LayoutGate::scope` в `PlayLayer::init` (`main.cpp`) выставляет thread_local флаг `s_active=true` только для PRIMARY, `false` для SHADOW. Layout-хуки (`LevelTools`, `PlayLayer::addObject`) читают `LayoutGate::shouldApply()` вместо глобального bool.

### Шаг 5 (НЕ СДЕЛАН): индикаторы
- См. отдельный раздел ниже.

### Шаг 6: Spout
- Установить Spout2 (идёт с OBS). В OBS: источник **Spout2 Capture** (НЕ Game Capture), имя = значение setting `spout_sender_name` (по умолчанию `GDShadow`).
- См. БЛОК 4 про сомнительные имена экспортов.

---

## ⚠️ БЛОК 3: Известные риски и спорные места (ПРОВЕРИТЬ)

### 3.1. `m_shadow->update(dt)` — спорный вызов синхронизации
В `ShadowManager::onPrimaryPostUpdate` мы зовём `m_shadow->update(dt)` чтобы шагнуть shadow тем же dt, что PRIMARY. Но:
- `PlayLayer` наследует `GJBaseGameLayer` → `CCLayer` → `CCNode`. Метод `update(float)` есть, но он может не делать полную игровую симуляцию (фиксированные шаги). В GD фиксированный степпинг делает `GJBaseGameLayer::update` или `PlayLayer::postUpdate`.
- **Возможно правильнее** шагать shadow через `postUpdate(dt)` или специальный метод, а не `update`. Нужно свериться с биндингами `geode-sdk/bindings` для `PlayLayer`/`GJBaseGameLayer` (методы `update`, `postUpdate`, `processCommands`, stepped update loop).
- В `main.cpp` есть хук `$modify(GJBaseGameLayer)::update` который блокирует shadow от собственного schedule — это belt-and-suspenders, но может быть избыточным/неправильным. Пересмотреть.

### 3.2. Зеркало ввода через `m_holdingButtons`
`applyMirrorInput` читает `m_player1->m_holdingButtons[1/2/3]` (jump/left/right) и вызывает `pushButton`/`releaseButton` на shadow. Индексы `[1/2/3]` — **предположение**, основанное на коде XDBot (`practice_fixes/play_layer.cpp` использует `static_cast<PlayerButton>(1)` для jump). Проверить фактический размер массива `m_holdingButtons` и соответствие индексов enum `PlayerButton` в биндингах.

### 3.2b. `PlayerButtonEnum.hpp` — может не существовать
В `ShadowManager.cpp` есть `#include <Geode/binding/PlayerButtonEnum.hpp>`. Этот хедер может называться иначе или не существовать. Если билд падает на этом include — убрать его; `PlayerButton` enum определён внутри `PlayerObject.hpp` или `PlayerButton.hpp`. Проверить в https://docs.geode-sdk.org/classes/PlayerButton/.

### 3.2c. Хук `GJBaseGameLayer::update` в main.cpp — сомнителен
Блок `$modify(GJBaseGameLayer) { void update(float dt) { ... } }` в `main.cpp` предназначен для блокировки shadow от собственного schedule. Но:
- `GJBaseGameLayer` — базовый класс для ВСЕХ игровых слоёв, включая EditLayer, MenuLayer и т.д. Хукать его `update` **опасно** — может сломать меню/редактор.
- Shadow не добавлен в сцену, поэтому движок его `update` **не вызывает** в любом случае. Вероятно этот хук **нужно удалить**.
- Если оставить — как минимум: проверять `this == ShadowManager::get().shadow()` и только тогда return (не调用 GJBaseGameLayer::update для shadow, а для всех остальных — нормальный вызов).

### 3.3. Позиция для сверки `m_obPosition`
`checkSync` сравнивает `m_player1->m_obPosition`. Это поле `CCNode`. Подтвердить, что это правильное поле позиции игрока (в GD часто `getPosition()` или кастомное поле). XDBot использует `m_obPosition` косвенно — проверить.

### 3.4. Render texture: readback между begin/end
`renderShadowToTexture` зовёт `glReadPixels` **после** `m_shadow->visit()` но **до** `m_rt->end()`, пока FBO ещё привязан. Это правильно, но:
- `glReadPixels` синхронный → может быть узким местом. Для RTX 3090 ок, но可以考虑 PBO (asynchronous readback) как оптимизация (readme «целевое железо RTX 3090, можно нагружать»).
- Включить `<Geode/cocos/CCGL.h>` для GL-символов — уже есть в ShadowManager.cpp.
- Размер readback-буфера `m_readback` устанавливается в `configureOutput` — ок.

### 3.5. Аудио
**НЕ обработано.** readme: «звук только из PRIMARY». Сейчас SHADOW потенциально может воспроизводить звуки (клики объектов, музыка?). Нужны хуки на FMOD / звуковые вызовы с gate «если этот PlayLayer == shadow, пропустить звук». Свериться, как GD воспроизводит gameplay SFX (вероятно через `PlayLayer`/`GameObject`).

### 3.6. Пауза
**НЕ обработана.** readme: «оба слоя на паузу». Нужно хукнуть `PlayLayer` pause/resume и применять к shadow. См. биндинги `PauseLayer`.

---

## 🎨 БЛОК 4: Индикаторы + меню (шаг 5, НЕ СДЕЛАН)

readme требование: «Индикаторы (FPS и прочие оверлеи) должны быть видны в OBS-картинке» + из чата: «FPS, CPS и т.д., и менюшки».

### Что нужно
1. Создать overlay-узел (CCLayer / CCNode) с CCLabelBMFont-метками: FPS, CPS (clicks per second), прогресс уровня, и т.п.
2. Рисовать этот overlay **поверх** shadow-прохода **внутри** `renderShadowToTexture`, после `m_shadow->visit()` но до `m_rt->end()`:
   ```cpp
   m_rt->beginWithClear(0,0,0,1);
   m_shadow->visit();
   m_overlay->updateLabels();   // обновить значения
   m_overlay->visit();          // нарисовать поверх
   if (m_spout.isOpen()) glReadPixels(...);
   m_rt->end();
   ```
3. FPS считать самому (дельта времени кадра) или читать из GD.
4. CPS = счётчик кликов PRIMARY / секунду.
5. **Меню** (по словам юзера «менюшки меню»): вероятно мод-меню с тогглами настроек (layout on/off, spout on/off, размер выхода). Реализовать через `geode::Popup<>` или FLAlertLayer, навешиваемое кнопкой. Или просто сослаться на стандартные настройки Geode (они уже есть в mod.json: `enabled`, `layout_on_primary`, `shadow_width/height`, `spout_sender_name`).

### Бонус
Юзер сказал «делай быстрее», поэтому меню можно свести к существующим settings (Mod::get()->getSettingValue), а overlay сделать минимальным (FPS + CPS).

---

## 🔌 БЛОК 5: Spout2 — проверить имена экспортов

`Spout.cpp` использует runtime-resolved символы:
- `spoutCreateSender(name, width, height, format)`
- `spoutUpdateSender(name, width, height, pixels)`
- `spoutReleaseSender(name)`

**Эти имена — ПРЕДПОЛОЖЕНИЕ.** Реальный Spout SDK (https://github.com/leadedge/Spout2) имеет другой API — обычно через C++ класс `SpoutSender` с методами `CreateSender`, `SendTexture`/`SendImage`, `ReleaseSender`, и C-API обёртка может отличаться. 

**Что сделать:**
1. Скачать Spout2 SDK, заглянуть в `SpoutLibrary.h` / `Spout.h` — найти актуальные имена функций.
2. Популярный паттерн — статическая библиотека + D3D11 texture. Spout работает с **D3D11 текстурами**, а у нас OpenGL framebuffer. Нужен bridge:
   - Вариант A: `SendImage(sender, rgbaPixels, w, h, GL_RGBA)` — Spout умеет принимать CPU-side RGBA буферы (так называемый image sender). Это самый простой путь, и он уже заложен в `Spout.cpp::sendFrame`.
   - Вариант B: interop GL→D3D11 (сложнее, быстрее).
3. При отсутствии Spout.dll — graceful fallback (уже есть: лог warning + текстура всё равно рендерится, просто не уходит в OBS).
4. **Для CI (Linux runner)** Spout недоступен → Spout.cpp под не-Windows компилируется в stub. Это уже реализовано через `#if LAYOUT_SHADOW_SPOUT`. Билд должен проходить на Linux runner для win-таргета.

---

## 📋 Чеклист для следующей сессии (по приоритету)

1. **[ ] Зафиксить CMakeLists.txt** — добавить `Spout.cpp`, `LayoutGate.cpp`, `layout_mode.cpp`, прилинковать d3d11/dxgi. (БЛОК 1)
2. **[ ] Удалить/заменить `#include <Geode/binding/PlayerButtonEnum.hpp>`** в `ShadowManager.cpp` если не существует (БЛОК 3.2b).
3. **[ ] Рассмотреть удаление хука `GJBaseGameLayer::update`** из `main.cpp` (БЛОК 3.2c) — он опасен и возможно не нужен (shadow не в сцене).
4. **[ ] Добиться билда** (push → CI). Проверить все `<Geode/binding/...>` includes.
5. **[ ] Тест шага 1** вживую (второй PlayLayer без крашей).
6. **[ ] Тест шага 2** (синхронизация, `checkSync` не должен логировать drift). Если дрифт — разбираться с тем, как правильно шагать shadow (БЛОК 3.1). **Это критичное место.**
7. **[ ] Тест шага 3** (рендер в текстуру).
8. **[ ] Тест шага 4** (layout только на PRIMARY — голубой экран + белые силуэты у игрока; полный уровень в текстуре).
9. **[ ] Шаг 5: индикаторы overlay** (FPS/CPS) — нарисовать в shadow-проходе (БЛОК 4).
10. **[ ] Шаг 6: Spout** — сверить имена экспортов с реальным SDK, проверить в OBS (БЛОК 5).
11. **[ ] Шаг 7: полировка** — пауза, аудио только из PRIMARY, рестарт (БЛОК 3.5, 3.6).
12. **[ ] Сверка с биндингами** всех сомнительных методов/полей (БЛОК 3) через https://docs.geode-sdk.org/ и https://github.com/geode-sdk/bindings.

---

## 🔑 Ключевые факты (запомнить)

- **GD 2.2081, Geode 5.7.1**, Windows-only целевая платформа (но CI на Linux runner собирает win-бинарь — это нормально для `build-geode-mod`).
- **`PlayLayer::create(GJGameLevel*, bool useReplay, bool dontCreateObjects)`** — подтверждённая сигнатура (docs.geode-sdk.org/classes/PlayLayer).
- **`GameManager::get()->m_playLayer`** — публичное поле, ГЛОБАЛЬНЫЙ синглтон активного PlayLayer. Его **обязательно сохранять и восстанавливать** вокруг создания shadow (`ShadowManager::create` уже это делает).
- **`m_fields` / `struct Fields {}`** — Geode idiom для per-instance полей в `$modify`. XDBot использует (см. `../XDBotFork-main/src/main.cpp:43`).
- **Layout XDBot** модифицирует `level->m_levelString` только на момент `init`, потом восстанавливает оригинал. Gating оригинала — один глобальный bool `Global::get().layoutMode`. Мы заменили на per-layer `LayoutGate` (thread_local).
- **Синхронизация** — единый источник времени: shadow НЕ имеет своего schedule, шагается тем же dt что PRIMARY, кадр-в-кадр (разница = 0). Debug-проверка через `checkSync`.

---

## 📚 Источники / ссылки
- Исходное ТЗ: `readme.md` (в этой папке)
- Vendoring layout-кода: `../XDBotFork-main/src/hacks/layout_mode.cpp` + `.hpp`
- Биндинги GD: https://docs.geode-sdk.org/classes/PlayLayer/ , https://docs.geode-sdk.org/classes/GameManager/
- geode-sdk/bindings: https://github.com/geode-sdk/bindings
- CCRenderTexture (cocos2d-x V2.2): `begin/beginWithClear → visit → end`
- Spout2: https://github.com/leadedge/Spout2

---

*Документ создан в условиях нехватки лимитов. Код шагов 2–6 написан, но НЕ тестировался вживую — ожидать багов. Начать с БЛОК 1 (CMake), затем прогнать по чеклисту.*
