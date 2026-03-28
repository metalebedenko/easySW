# EasySW

EasySW is a tiny native Windows tray utility that fixes the keyboard layout of the current selection or the word at the caret.

The app is intentionally small:

- native Win32 UI
- no Electron, WebView, .NET desktop runtime, or auto-updater
- one tray icon, one hotkey setting, one optional startup setting
- portable `exe` or simple per-user installer

## Features

- Runs in the background and lives in the system tray.
- Fixes the currently selected text with one global hotkey.
- If nothing is selected, tries to select the word at the caret and fix it.
- Supports English and Russian layouts via physical key remapping.
- Stores only:
  - global hotkey
  - launch at Windows startup

## How It Works

EasySW uses a layered approach for reliability:

1. Try to read the current selection through UI Automation.
2. If UI Automation is not available, fall back to `Ctrl+C` with clipboard preservation.
3. If nothing is selected, try to select the word at the caret through UI Automation.
4. If that also fails, fall back to keyboard selection (`Ctrl+Shift+Left`).
5. Replace the selected text through clipboard paste, with Unicode input as a last resort.

This keeps the app lightweight while still working in many common Windows editors and text fields.

## Project Structure

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

## Build

### Option 1: Visual Studio / CMake

Requirements:

- Visual Studio 2022 or Build Tools with Desktop C++ workload
- CMake 3.21+

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output:

```text
build\Release\EasySW.exe
```

### Option 2: LLVM-MinGW helper script

If you use the same lightweight toolchain as the local build here:

```powershell
.\scripts\build-release.ps1
```

Output:

```text
build\EasySW.exe
```

## Installer

An Inno Setup script is included:

```text
packaging\EasySW.iss
```

It installs per-user into `%LocalAppData%` and does not require administrator rights.

## GitHub Actions

The repository includes a Windows build workflow:

```text
.github\workflows\build.yml
```

Every push and pull request builds a Release executable and uploads it as an artifact.

## Known Limitations

- Some applications expose no usable UI Automation text model.
- Some elevated or protected windows may reject automation and simulated input from a normal user process.
- Some custom controls ignore standard copy/paste shortcuts.
- Terminal-like apps and games may require app-specific handling.

For those cases, the app intentionally fails quietly instead of guessing and corrupting text.
