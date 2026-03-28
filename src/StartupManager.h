#pragma once

#include <string>

class StartupManager {
public:
    bool IsEnabled() const;
    bool SetEnabled(bool enabled) const;

private:
    std::wstring ExecutablePath() const;
    std::wstring RegistryValue() const;
};
