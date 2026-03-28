# EasySW

EasySW — маленькая нативная Windows-утилита в трее, которая исправляет раскладку у выделенного текста или у слова под кареткой.

Проект специально сделан легким:

- чистый Win32 без Electron, WebView и тяжелых рантаймов
- один tray icon, одна горячая клавиша и опция автозапуска
- portable `exe` или простой per-user installer
- предсказуемое поведение без автоисправления “за спиной” пользователя

## Возможности

- Работает в фоне и живет в системном трее.
- Исправляет текущий выделенный текст по глобальной горячей клавише.
- Если выделения нет, пытается выделить и исправить слово под кареткой.
- Поддерживает английскую и русскую раскладки через посимвольную перекладку клавиш.
- Хранит только две настройки:
  - глобальная горячая клавиша
  - запуск вместе с Windows

## Как это работает

EasySW использует несколько уровней fallback, чтобы оставаться маленьким, но при этом работать в обычных Windows-приложениях:

1. Сначала пытается прочитать текущее выделение через UI Automation.
2. Если UI Automation недоступен, пробует стандартный `Ctrl+C` с сохранением clipboard.
3. Если выделения нет, пытается выбрать слово под кареткой через UI Automation.
4. Если и это не получилось, использует клавиатурный fallback для выделения предыдущего слова.
5. Возвращает исправленный текст обратно в активное окно через paste, а в крайнем случае через Unicode input.

## Структура проекта

```text
.
|-- .github/workflows/build.yml
|-- packaging/EasySW.iss
|-- resources/
|   |-- app.ico
|   |-- app.manifest
|   `-- app.rc
|-- scripts/build-release.ps1
|-- src/
|   |-- App.cpp
|   |-- SettingsStore.cpp
|   |-- StartupManager.cpp
|   |-- TextInteractor.cpp
|   |-- TextTransform.cpp
|   `-- UiAutomationTextProvider.cpp
`-- CMakeLists.txt
```

## Сборка

### Вариант 1: Visual Studio / CMake

Требования:

- Visual Studio 2022 или Build Tools с Desktop C++ workload
- CMake 3.21+

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Готовый файл:

```text
build\Release\EasySW.exe
```

### Вариант 2: LLVM-MinGW helper script

Если использовать такой же легкий toolchain, как в локальной сборке:

```powershell
.\scripts\build-release.ps1
```

Готовый файл:

```text
build\EasySW.exe
```

## Установка

В проекте есть Inno Setup script:

```text
packaging\EasySW.iss
```

Он ставит приложение per-user в `%LocalAppData%` и не требует прав администратора.

## GitHub Actions

В репозитории уже есть Windows build workflow:

```text
.github\workflows\build.yml
```

При каждом push и pull request он собирает Release-версию и прикладывает `EasySW.exe` как artifact.

## Ограничения

- Некоторые приложения не отдают текст через UI Automation.
- Некоторые elevated или защищенные окна не принимают automation и simulated input от обычного процесса.
- Некоторые кастомные контролы игнорируют стандартные copy/paste shortcuts.
- Терминалы, игры и нестандартные редакторы могут потребовать отдельной адаптации.

В таких случаях утилита специально предпочитает тихо ничего не сделать, чем пытаться “угадать” и испортить текст.
