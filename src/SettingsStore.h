#pragma once

#include <string>
#include <windows.h>

struct HotkeyConfig {
    UINT modifiers = MOD_CONTROL | MOD_ALT;
    UINT vk = VK_SPACE;
};

class SettingsStore {
public:
    HotkeyConfig Load() const;
    bool Save(const HotkeyConfig& config) const;
    std::wstring ConfigPath() const;

private:
    std::wstring EnsureConfigDirectory() const;
};

bool IsValidHotkey(const HotkeyConfig& config);
WORD ToHotkeyControlValue(const HotkeyConfig& config);
HotkeyConfig FromHotkeyControlValue(WORD value);
std::wstring FormatHotkey(const HotkeyConfig& config);
