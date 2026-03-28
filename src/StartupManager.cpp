#include "StartupManager.h"

#include <windows.h>

namespace {
constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValueName[] = L"EasySW";
} // namespace

bool StartupManager::IsEnabled() const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t buffer[2048] = {};
    DWORD type = 0;
    DWORD size = sizeof(buffer);
    const LSTATUS status = RegQueryValueExW(
        key,
        kValueName,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(buffer),
        &size);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return false;
    }

    return RegistryValue() == buffer;
}

bool StartupManager::SetEnabled(bool enabled) const {
    HKEY key = nullptr;
    if (RegCreateKeyExW(
            HKEY_CURRENT_USER,
            kRunKeyPath,
            0,
            nullptr,
            0,
            KEY_SET_VALUE,
            nullptr,
            &key,
            nullptr) != ERROR_SUCCESS) {
        return false;
    }

    LSTATUS status = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring value = RegistryValue();
        const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
        status = RegSetValueExW(
            key,
            kValueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            bytes);
    } else {
        status = RegDeleteValueW(key, kValueName);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

std::wstring StartupManager::ExecutablePath() const {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    return buffer;
}

std::wstring StartupManager::RegistryValue() const {
    const std::wstring path = ExecutablePath();
    if (path.empty()) {
        return L"";
    }

    std::wstring value = L"\"";
    value += path;
    value += L"\"";
    return value;
}
