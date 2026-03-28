#pragma once

#include "UiAutomationTextProvider.h"

#include <objidl.h>
#include <string>
#include <windows.h>

enum class CorrectionMode {
    None,
    Selection,
    PreviousWord,
};

enum class CorrectionStatus {
    Replaced,
    NoText,
    NoChange,
    NoForegroundWindow,
    ClipboardUnavailable,
};

struct CorrectionOutcome {
    CorrectionStatus status = CorrectionStatus::NoText;
    CorrectionMode mode = CorrectionMode::None;
    std::wstring originalText;
    std::wstring correctedText;
};

class TextInteractor {
public:
    explicit TextInteractor(HWND ownerWindow);
    CorrectionOutcome CorrectForegroundText();

private:
    class ClipboardSnapshot {
    public:
        ClipboardSnapshot() = default;
        ~ClipboardSnapshot();

        bool Capture();
        void Restore() const;

    private:
        IDataObject* dataObject_ = nullptr;
    };

    bool TryGetSelectedText(std::wstring& text) const;
    bool TryReadSelectionUiAutomation(std::wstring& text) const;
    bool TryCopyCurrentSelection(std::wstring& text) const;
    bool SelectPreviousWord() const;
    bool ReplaceSelection(const std::wstring& text) const;
    void CollapseSelectionToCaret() const;

    static bool SendKeyChord(WORD virtualKey, bool ctrl, bool alt, bool shift);
    static bool SendUnicodeText(const std::wstring& text);
    static bool ReadClipboardText(std::wstring& text);
    static bool SetClipboardText(const std::wstring& text);
    static bool OpenClipboardWithRetry(HWND owner);
    static std::wstring BuildClipboardProbeText();
    static bool WaitForClipboardTextChange(const std::wstring& probe, DWORD timeoutMs, std::wstring& text);

    HWND ownerWindow_ = nullptr;
    UiAutomationTextProvider uiAutomation_{};
};
