#include "SettingsStore.h"

#include <array>
#include <shlobj.h>

namespace {
constexpr wchar_t kAppFolder[] = L"EasySW";
constexpr wchar_t kIniFile[] = L"settings.ini";
constexpr wchar_t kSection[] = L"Hotkey";
constexpr wchar_t kModifiersKey[] = L"Modifiers";
constexpr wchar_t kVirtualKeyKey[] = L"VirtualKey";
} // namespace

std::wstring SettingsStore::EnsureConfigDirectory() const {
    PWSTR roamingPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &roamingPath))) {
        return L".";
    }

    std::wstring folder = roamingPath;
    CoTaskMemFree(roamingPath);
    folder += L"\\";
    folder += kAppFolder;
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder;
}

std::wstring SettingsStore::ConfigPath() const {
    std::wstring path = EnsureConfigDirectory();
    path += L"\\";
    path += kIniFile;
    return path;
}

HotkeyConfig SettingsStore::Load() const {
    HotkeyConfig config;
    const std::wstring path = ConfigPath();

    config.modifiers = static_cast<UINT>(GetPrivateProfileIntW(
        kSection, kModifiersKey, static_cast<int>(config.modifiers), path.c_str()));
    config.vk = static_cast<UINT>(GetPrivateProfileIntW(
        kSection, kVirtualKeyKey, static_cast<int>(config.vk), path.c_str()));

    if (!IsValidHotkey(config)) {
        config = HotkeyConfig{};
    }
    return config;
}

bool SettingsStore::Save(const HotkeyConfig& config) const {
    if (!IsValidHotkey(config)) {
        return false;
    }

    const std::wstring path = ConfigPath();
    wchar_t modifiersBuffer[16] = {};
    wchar_t vkBuffer[16] = {};
    wsprintfW(modifiersBuffer, L"%u", config.modifiers);
    wsprintfW(vkBuffer, L"%u", config.vk);

    const BOOL ok1 = WritePrivateProfileStringW(kSection, kModifiersKey, modifiersBuffer, path.c_str());
    const BOOL ok2 = WritePrivateProfileStringW(kSection, kVirtualKeyKey, vkBuffer, path.c_str());
    return ok1 && ok2;
}

bool IsValidHotkey(const HotkeyConfig& config) {
    constexpr UINT supported = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (config.vk == 0) {
        return false;
    }
    if ((config.modifiers & supported) == 0) {
        return false;
    }
    return (config.modifiers & ~supported) == 0;
}

WORD ToHotkeyControlValue(const HotkeyConfig& config) {
    BYTE modifiers = 0;
    if (config.modifiers & MOD_SHIFT) {
        modifiers |= HOTKEYF_SHIFT;
    }
    if (config.modifiers & MOD_CONTROL) {
        modifiers |= HOTKEYF_CONTROL;
    }
    if (config.modifiers & MOD_ALT) {
        modifiers |= HOTKEYF_ALT;
    }
    return MAKEWORD(static_cast<BYTE>(config.vk), modifiers);
}

HotkeyConfig FromHotkeyControlValue(WORD value) {
    HotkeyConfig config{};
    config.vk = LOBYTE(value);
    config.modifiers = 0;

    const BYTE modifiers = HIBYTE(value);
    if (modifiers & HOTKEYF_SHIFT) {
        config.modifiers |= MOD_SHIFT;
    }
    if (modifiers & HOTKEYF_CONTROL) {
        config.modifiers |= MOD_CONTROL;
    }
    if (modifiers & HOTKEYF_ALT) {
        config.modifiers |= MOD_ALT;
    }
    return config;
}

std::wstring FormatHotkey(const HotkeyConfig& config) {
    std::wstring value;
    if (config.modifiers & MOD_CONTROL) {
        value += L"Ctrl+";
    }
    if (config.modifiers & MOD_ALT) {
        value += L"Alt+";
    }
    if (config.modifiers & MOD_SHIFT) {
        value += L"Shift+";
    }

    std::array<wchar_t, 64> buffer{};
    const UINT scanCode = MapVirtualKeyW(config.vk, MAPVK_VK_TO_VSC) << 16;
    if (GetKeyNameTextW(static_cast<LONG>(scanCode), buffer.data(), static_cast<int>(buffer.size())) > 0) {
        value += buffer.data();
    } else {
        wchar_t fallback[16] = {};
        wsprintfW(fallback, L"VK_%u", config.vk);
        value += fallback;
    }

    return value;
}
