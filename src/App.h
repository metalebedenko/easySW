#pragma once

#include "SettingsStore.h"
#include "StartupManager.h"
#include "TextInteractor.h"

#include <shellapi.h>
#include <windows.h>

class App {
public:
    int Run(HINSTANCE instanceHandle, int showCommand);

private:
    static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SettingsWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool Initialize(HINSTANCE instanceHandle);
    void Shutdown();
    bool EnsureSingleInstance();
    void ReleaseSingleInstance();

    bool CreateMainWindow();
    bool CreateSettingsWindow();
    void ShowSettingsWindow();
    void HideSettingsWindow();
    void CenterSettingsWindow() const;

    bool RegisterHotkey();
    void UnregisterHotkey();
    void WaitForHotkeyRelease() const;

    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();

    void HandleHotkey();
    void SaveHotkeyFromSettings();

    LRESULT HandleMainWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleSettingsWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE instanceHandle_ = nullptr;
    HWND mainWindow_ = nullptr;
    HWND settingsWindow_ = nullptr;
    HWND titleLabel_ = nullptr;
    HWND subtitleLabel_ = nullptr;
    HWND helperLabel_ = nullptr;
    HWND hotkeyLabel_ = nullptr;
    HWND hotkeyControl_ = nullptr;
    HWND startupCheckbox_ = nullptr;
    HWND saveButton_ = nullptr;
    HWND closeButton_ = nullptr;
    HFONT titleFont_ = nullptr;
    HFONT bodyFont_ = nullptr;
    HBRUSH settingsBackgroundBrush_ = nullptr;
    HICON appIcon_ = nullptr;
    HANDLE singleInstanceMutex_ = nullptr;
    UINT taskbarCreatedMessage_ = 0;
    NOTIFYICONDATAW trayIcon_{};
    SettingsStore settingsStore_{};
    StartupManager startupManager_{};
    HotkeyConfig hotkeyConfig_{};    
    TextInteractor* textInteractor_ = nullptr;
};
