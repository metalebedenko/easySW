#pragma once

#include <string>

struct IUIAutomation;

class UiAutomationTextProvider {
public:
    UiAutomationTextProvider();
    ~UiAutomationTextProvider();

    UiAutomationTextProvider(const UiAutomationTextProvider&) = delete;
    UiAutomationTextProvider& operator=(const UiAutomationTextProvider&) = delete;

    bool IsAvailable() const;
    bool TryGetSelectedText(std::wstring& text) const;
    bool TrySelectWordAtCaret(std::wstring& text) const;

private:
    IUIAutomation* automation_ = nullptr;
};
