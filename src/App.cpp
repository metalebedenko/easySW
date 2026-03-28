#include "App.h"
#include "resource.h"

#include <commctrl.h>
#include <ole2.h>
#include <shellapi.h>

namespace {
constexpr wchar_t kMainWindowClass[] = L"EasySW.MainWindow";
constexpr wchar_t kSettingsWindowClass[] = L"EasySW.SettingsWindow";
constexpr wchar_t kAppTitle[] = L"EasySW";
constexpr wchar_t kSettingsTitle[] = L"Горячая клавиша";
constexpr wchar_t kSettingsSubtitle[] = L"Выберите сочетание, которое исправляет последнее слово или выделенный текст.";
constexpr wchar_t kSettingsHint[] = L"Утилита живет в трее, запускается мгновенно и хранит только хоткей и автозапуск.";

constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr int kHotkeyId = 100;

constexpr UINT_PTR kMenuSettings = 2001;
constexpr UINT_PTR kMenuExit = 2002;

constexpr int kControlHotkey = 3001;
constexpr int kControlApply = 3002;
constexpr int kControlCancel = 3003;
constexpr int kControlStartup = 3004;
constexpr wchar_t kSingleInstanceMutexName[] = L"Local\\EasySW.SingleInstance";
constexpr UINT kShowSettingsMessage = WM_APP + 2;

COLORREF kSettingsBackground = RGB(244, 247, 250);
COLORREF kTitleColor = RGB(20, 29, 37);
COLORREF kTextColor = RGB(69, 85, 101);

void SetControlFont(HWND control, HFONT font) {
    if (control != nullptr && font != nullptr) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}
} // namespace

int App::Run(HINSTANCE instanceHandle, int showCommand) {
    UNREFERENCED_PARAMETER(showCommand);

    if (!Initialize(instanceHandle)) {
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    Shutdown();
    return static_cast<int>(message.wParam);
}

bool App::Initialize(HINSTANCE instanceHandle) {
    instanceHandle_ = instanceHandle;
    hotkeyConfig_ = settingsStore_.Load();
    taskbarCreatedMessage_ = RegisterWindowMessageW(L"TaskbarCreated");
    settingsBackgroundBrush_ = CreateSolidBrush(kSettingsBackground);
    appIcon_ = static_cast<HICON>(LoadImageW(
        instanceHandle_,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        0,
        0,
        LR_DEFAULTSIZE));

    LOGFONTW titleFont{};
    titleFont.lfHeight = -24;
    titleFont.lfWeight = FW_SEMIBOLD;
    lstrcpyW(titleFont.lfFaceName, L"Segoe UI");
    titleFont_ = CreateFontIndirectW(&titleFont);

    LOGFONTW bodyFont{};
    bodyFont.lfHeight = -15;
    bodyFont.lfWeight = FW_NORMAL;
    lstrcpyW(bodyFont.lfFaceName, L"Segoe UI");
    bodyFont_ = CreateFontIndirectW(&bodyFont);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    if (FAILED(OleInitialize(nullptr))) {
        return false;
    }
    if (!EnsureSingleInstance()) {
        OleUninitialize();
        return false;
    }
    if (!CreateMainWindow()) {
        ReleaseSingleInstance();
        OleUninitialize();
        return false;
    }

    textInteractor_ = new TextInteractor(mainWindow_);

    AddTrayIcon();
    if (!RegisterHotkey()) {
        MessageBoxW(mainWindow_,
            L"Не удалось зарегистрировать глобальную горячую клавишу. Откройте настройки и выберите другое сочетание.",
            kAppTitle, MB_OK | MB_ICONWARNING);
    }

    return true;
}

void App::Shutdown() {
    UnregisterHotkey();
    RemoveTrayIcon();

    if (settingsWindow_ != nullptr) {
        DestroyWindow(settingsWindow_);
        settingsWindow_ = nullptr;
    }
    if (mainWindow_ != nullptr) {
        DestroyWindow(mainWindow_);
        mainWindow_ = nullptr;
    }

    delete textInteractor_;
    textInteractor_ = nullptr;

    if (titleFont_ != nullptr) {
        DeleteObject(titleFont_);
        titleFont_ = nullptr;
    }
    if (bodyFont_ != nullptr) {
        DeleteObject(bodyFont_);
        bodyFont_ = nullptr;
    }
    if (settingsBackgroundBrush_ != nullptr) {
        DeleteObject(settingsBackgroundBrush_);
        settingsBackgroundBrush_ = nullptr;
    }
    if (appIcon_ != nullptr) {
        DestroyIcon(appIcon_);
        appIcon_ = nullptr;
    }
    ReleaseSingleInstance();

    OleUninitialize();
}

bool App::EnsureSingleInstance() {
    singleInstanceMutex_ = CreateMutexW(nullptr, FALSE, kSingleInstanceMutexName);
    if (singleInstanceMutex_ == nullptr) {
        return true;
    }

    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        return true;
    }

    HWND existing = FindWindowW(kMainWindowClass, nullptr);
    if (existing != nullptr) {
        PostMessageW(existing, kShowSettingsMessage, 0, 0);
    }

    ReleaseSingleInstance();
    return false;
}

void App::ReleaseSingleInstance() {
    if (singleInstanceMutex_ != nullptr) {
        CloseHandle(singleInstanceMutex_);
        singleInstanceMutex_ = nullptr;
    }
}

bool App::CreateMainWindow() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &App::MainWindowProc;
    windowClass.hInstance = instanceHandle_;
    windowClass.lpszClassName = kMainWindowClass;
    windowClass.hIcon = appIcon_ != nullptr ? appIcon_ : LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = windowClass.hIcon;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (RegisterClassExW(&windowClass) == 0) {
        return false;
    }

    mainWindow_ = CreateWindowExW(
        0,
        kMainWindowClass,
        kAppTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 200,
        nullptr, nullptr, instanceHandle_, this);

    return mainWindow_ != nullptr;
}

bool App::CreateSettingsWindow() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &App::SettingsWindowProc;
    windowClass.hInstance = instanceHandle_;
    windowClass.lpszClassName = kSettingsWindowClass;
    windowClass.hIcon = appIcon_ != nullptr ? appIcon_ : LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = windowClass.hIcon;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = settingsBackgroundBrush_ != nullptr
        ? settingsBackgroundBrush_
        : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassExW(&windowClass);

    settingsWindow_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
        kSettingsWindowClass,
        L"EasySW: горячая клавиша",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 275,
        nullptr, nullptr, instanceHandle_, this);

    if (settingsWindow_ == nullptr) {
        return false;
    }

    titleLabel_ = CreateWindowExW(0, L"STATIC", kSettingsTitle,
        WS_CHILD | WS_VISIBLE,
        20, 18, 220, 32,
        settingsWindow_, nullptr, instanceHandle_, nullptr);

    subtitleLabel_ = CreateWindowExW(0, L"STATIC", kSettingsSubtitle,
        WS_CHILD | WS_VISIBLE,
        20, 50, 380, 38,
        settingsWindow_, nullptr, instanceHandle_, nullptr);

    hotkeyLabel_ = CreateWindowExW(0, L"STATIC", L"Сочетание",
        WS_CHILD | WS_VISIBLE,
        20, 102, 120, 20,
        settingsWindow_, nullptr, instanceHandle_, nullptr);

    hotkeyControl_ = CreateWindowExW(WS_EX_CLIENTEDGE, HOTKEY_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        20, 126, 250, 30,
        settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlHotkey)),
        instanceHandle_, nullptr);

    SendMessageW(hotkeyControl_, HKM_SETHOTKEY, ToHotkeyControlValue(hotkeyConfig_), 0);

    startupCheckbox_ = CreateWindowExW(0, L"BUTTON", L"Запускать вместе с Windows",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        20, 168, 250, 24,
        settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlStartup)),
        instanceHandle_, nullptr);

    helperLabel_ = CreateWindowExW(0, L"STATIC", kSettingsHint,
        WS_CHILD | WS_VISIBLE,
        20, 200, 390, 32,
        settingsWindow_, nullptr, instanceHandle_, nullptr);

    saveButton_ = CreateWindowExW(0, L"BUTTON", L"Сохранить",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        230, 162, 82, 30,
        settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlApply)),
        instanceHandle_, nullptr);

    closeButton_ = CreateWindowExW(0, L"BUTTON", L"Закрыть",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        322, 162, 82, 30,
        settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlCancel)),
        instanceHandle_, nullptr);

    SetControlFont(titleLabel_, titleFont_);
    SetControlFont(subtitleLabel_, bodyFont_);
    SetControlFont(hotkeyLabel_, bodyFont_);
    SetControlFont(hotkeyControl_, bodyFont_);
    SetControlFont(startupCheckbox_, bodyFont_);
    SetControlFont(helperLabel_, bodyFont_);
    SetControlFont(saveButton_, bodyFont_);
    SetControlFont(closeButton_, bodyFont_);

    return true;
}

void App::ShowSettingsWindow() {
    if (settingsWindow_ == nullptr && !CreateSettingsWindow()) {
        return;
    }

    SendMessageW(hotkeyControl_, HKM_SETHOTKEY, ToHotkeyControlValue(hotkeyConfig_), 0);
    SendMessageW(
        startupCheckbox_,
        BM_SETCHECK,
        startupManager_.IsEnabled() ? BST_CHECKED : BST_UNCHECKED,
        0);
    CenterSettingsWindow();
    ShowWindow(settingsWindow_, SW_SHOWNORMAL);
    SetForegroundWindow(settingsWindow_);
}

void App::HideSettingsWindow() {
    if (settingsWindow_ != nullptr) {
        ShowWindow(settingsWindow_, SW_HIDE);
    }
}

void App::CenterSettingsWindow() const {
    if (settingsWindow_ == nullptr) {
        return;
    }

    RECT windowRect{};
    RECT workRect{};
    if (!GetWindowRect(settingsWindow_, &windowRect) ||
        !SystemParametersInfoW(SPI_GETWORKAREA, 0, &workRect, 0)) {
        return;
    }

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = workRect.left + ((workRect.right - workRect.left) - width) / 2;
    const int y = workRect.top + ((workRect.bottom - workRect.top) - height) / 2;
    SetWindowPos(settingsWindow_, nullptr, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

bool App::RegisterHotkey() {
    UnregisterHotkey();
    if (!IsValidHotkey(hotkeyConfig_)) {
        return false;
    }
    return RegisterHotKey(mainWindow_, kHotkeyId, hotkeyConfig_.modifiers, hotkeyConfig_.vk) != FALSE;
}

void App::UnregisterHotkey() {
    if (mainWindow_ != nullptr) {
        UnregisterHotKey(mainWindow_, kHotkeyId);
    }
}

void App::WaitForHotkeyRelease() const {
    const DWORD started = GetTickCount();
    while (GetTickCount() - started < 700) {
        bool released = true;

        if ((hotkeyConfig_.modifiers & MOD_CONTROL) && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
            released = false;
        }
        if ((hotkeyConfig_.modifiers & MOD_SHIFT) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            released = false;
        }
        if ((hotkeyConfig_.modifiers & MOD_ALT) && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            released = false;
        }
        if (GetAsyncKeyState(static_cast<int>(hotkeyConfig_.vk)) & 0x8000) {
            released = false;
        }

        if (released) {
            Sleep(25);
            return;
        }
        Sleep(10);
    }
}

void App::AddTrayIcon() {
    trayIcon_.cbSize = sizeof(trayIcon_);
    trayIcon_.hWnd = mainWindow_;
    trayIcon_.uID = kTrayIconId;
    trayIcon_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    trayIcon_.uCallbackMessage = kTrayMessage;
    trayIcon_.hIcon = appIcon_ != nullptr ? appIcon_ : LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(trayIcon_.szTip, L"EasySW", ARRAYSIZE(trayIcon_.szTip));
    Shell_NotifyIconW(NIM_ADD, &trayIcon_);
}

void App::RemoveTrayIcon() {
    if (trayIcon_.hWnd != nullptr) {
        Shell_NotifyIconW(NIM_DELETE, &trayIcon_);
    }
}

void App::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kMenuSettings, L"Настройки...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuExit, L"Выход");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(mainWindow_);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, cursor.x, cursor.y, 0, mainWindow_, nullptr);
    DestroyMenu(menu);
}

void App::HandleHotkey() {
    if (textInteractor_ == nullptr) {
        return;
    }

    const HWND foreground = GetForegroundWindow();
    if (foreground == nullptr || foreground == settingsWindow_ || foreground == mainWindow_) {
        return;
    }

    WaitForHotkeyRelease();
    textInteractor_->CorrectForegroundText();
}

void App::SaveHotkeyFromSettings() {
    if (hotkeyControl_ == nullptr) {
        return;
    }

    const WORD rawHotkey = static_cast<WORD>(SendMessageW(hotkeyControl_, HKM_GETHOTKEY, 0, 0));
    HotkeyConfig candidate = FromHotkeyControlValue(rawHotkey);

    if (!IsValidHotkey(candidate)) {
        MessageBoxW(settingsWindow_,
            L"Выберите сочетание хотя бы с одним модификатором.",
            kAppTitle, MB_OK | MB_ICONWARNING);
        return;
    }

    const HotkeyConfig previous = hotkeyConfig_;
    hotkeyConfig_ = candidate;

    if (!RegisterHotkey()) {
        hotkeyConfig_ = previous;
        RegisterHotkey();
        MessageBoxW(settingsWindow_,
            L"Это сочетание уже занято другим приложением.",
            kAppTitle, MB_OK | MB_ICONWARNING);
        return;
    }

    const bool startupEnabled = SendMessageW(startupCheckbox_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (!startupManager_.SetEnabled(startupEnabled)) {
        hotkeyConfig_ = previous;
        RegisterHotkey();
        MessageBoxW(settingsWindow_,
            L"Не удалось обновить параметр автозапуска.",
            kAppTitle, MB_OK | MB_ICONWARNING);
        return;
    }

    settingsStore_.Save(hotkeyConfig_);
    HideSettingsWindow();
}

LRESULT App::HandleMainWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kMenuSettings:
            ShowSettingsWindow();
            return 0;
        case kMenuExit:
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
        }
        break;
    case kShowSettingsMessage:
        ShowSettingsWindow();
        return 0;
    case WM_HOTKEY:
        if (wParam == kHotkeyId) {
            HandleHotkey();
            return 0;
        }
        break;
    default:
        if (message == taskbarCreatedMessage_) {
            AddTrayIcon();
            return 0;
        }
        break;
    case kTrayMessage:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayMenu();
            return 0;
        }
        if (lParam == WM_LBUTTONDBLCLK) {
            ShowSettingsWindow();
            return 0;
        }
        break;
    case WM_DESTROY:
        mainWindow_ = nullptr;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::HandleSettingsWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, control == titleLabel_ ? kTitleColor : kTextColor);
        return reinterpret_cast<LRESULT>(settingsBackgroundBrush_);
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kControlApply:
            SaveHotkeyFromSettings();
            return 0;
        case kControlCancel:
            HideSettingsWindow();
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        HideSettingsWindow();
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK App::MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;

    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        if (app != nullptr) {
            app->mainWindow_ = hwnd;
        }
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (app != nullptr) {
        return app->HandleMainWindowMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK App::SettingsWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;

    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (app != nullptr) {
        return app->HandleSettingsWindowMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
