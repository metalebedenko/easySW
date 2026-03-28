#include "TextInteractor.h"

#include "TextTransform.h"

#include <cstring>
#include <ole2.h>
#include <vector>

namespace {
constexpr DWORD kClipboardTimeoutMs = 500;

void AppendKeyEvent(std::vector<INPUT>& inputs, WORD vk, DWORD flags) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = flags;
    inputs.push_back(input);
}
} // namespace

TextInteractor::TextInteractor(HWND ownerWindow)
    : ownerWindow_(ownerWindow) {
}

TextInteractor::ClipboardSnapshot::~ClipboardSnapshot() {
    if (dataObject_ != nullptr) {
        dataObject_->Release();
        dataObject_ = nullptr;
    }
}

bool TextInteractor::ClipboardSnapshot::Capture() {
    if (dataObject_ != nullptr) {
        dataObject_->Release();
        dataObject_ = nullptr;
    }

    return SUCCEEDED(OleGetClipboard(&dataObject_)) && dataObject_ != nullptr;
}

void TextInteractor::ClipboardSnapshot::Restore() const {
    if (dataObject_ != nullptr) {
        OleSetClipboard(dataObject_);
    }
}

CorrectionOutcome TextInteractor::CorrectForegroundText() {
    CorrectionOutcome outcome{};

    if (GetForegroundWindow() == nullptr) {
        outcome.status = CorrectionStatus::NoForegroundWindow;
        return outcome;
    }

    ClipboardSnapshot snapshot;
    const bool hasSnapshot = snapshot.Capture();

    std::wstring sourceText;
    if (TryGetSelectedText(sourceText) && !sourceText.empty()) {
        outcome.mode = CorrectionMode::Selection;
    } else {
        if (uiAutomation_.TrySelectWordAtCaret(sourceText) && !sourceText.empty()) {
            outcome.mode = CorrectionMode::PreviousWord;
        } else if (!SelectPreviousWord()) {
            if (hasSnapshot) {
                snapshot.Restore();
            }
            outcome.status = hasSnapshot ? CorrectionStatus::NoText : CorrectionStatus::ClipboardUnavailable;
            return outcome;
        } else if (!TryGetSelectedText(sourceText) || sourceText.empty()) {
            CollapseSelectionToCaret();
            if (hasSnapshot) {
                snapshot.Restore();
            }
            outcome.status = hasSnapshot ? CorrectionStatus::NoText : CorrectionStatus::ClipboardUnavailable;
            return outcome;
        } else {
            outcome.mode = CorrectionMode::PreviousWord;
        }
    }

    outcome.originalText = sourceText;
    outcome.correctedText = TextTransform::FixLayout(sourceText);

    if (outcome.correctedText.empty() || outcome.correctedText == outcome.originalText) {
        if (outcome.mode == CorrectionMode::PreviousWord) {
            ReplaceSelection(outcome.originalText);
        }
        if (hasSnapshot) {
            snapshot.Restore();
        }
        outcome.status = CorrectionStatus::NoChange;
        return outcome;
    }

    if (!ReplaceSelection(outcome.correctedText)) {
        if (outcome.mode == CorrectionMode::PreviousWord) {
            CollapseSelectionToCaret();
        }
        if (hasSnapshot) {
            snapshot.Restore();
        }
        outcome.status = CorrectionStatus::ClipboardUnavailable;
        return outcome;
    }

    Sleep(120);
    if (hasSnapshot) {
        snapshot.Restore();
    }

    outcome.status = CorrectionStatus::Replaced;
    return outcome;
}

bool TextInteractor::TryReadSelectionUiAutomation(std::wstring& text) const {
    return uiAutomation_.TryGetSelectedText(text);
}

bool TextInteractor::TryGetSelectedText(std::wstring& text) const {
    text.clear();
    if (TryReadSelectionUiAutomation(text) && !text.empty()) {
        return true;
    }
    return TryCopyCurrentSelection(text) && !text.empty();
}

bool TextInteractor::TryCopyCurrentSelection(std::wstring& text) const {
    const std::wstring probe = BuildClipboardProbeText();
    if (!SetClipboardText(probe)) {
        return false;
    }
    Sleep(15);

    if (!SendKeyChord('C', true, false, false)) {
        return false;
    }

    return WaitForClipboardTextChange(probe, kClipboardTimeoutMs, text);
}

bool TextInteractor::SelectPreviousWord() const {
    if (!SendKeyChord(VK_LEFT, true, false, true)) {
        return false;
    }

    Sleep(60);
    return true;
}

bool TextInteractor::ReplaceSelection(const std::wstring& text) const {
    if (SetClipboardText(text)) {
        Sleep(20);
        return SendKeyChord('V', true, false, false);
    }

    return SendUnicodeText(text);
}

void TextInteractor::CollapseSelectionToCaret() const {
    std::vector<INPUT> inputs;
    AppendKeyEvent(inputs, VK_RIGHT, 0);
    AppendKeyEvent(inputs, VK_RIGHT, KEYEVENTF_KEYUP);
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

bool TextInteractor::SendKeyChord(WORD virtualKey, bool ctrl, bool alt, bool shift) {
    std::vector<INPUT> inputs;

    if (ctrl) {
        AppendKeyEvent(inputs, VK_CONTROL, 0);
    }
    if (alt) {
        AppendKeyEvent(inputs, VK_MENU, 0);
    }
    if (shift) {
        AppendKeyEvent(inputs, VK_SHIFT, 0);
    }

    AppendKeyEvent(inputs, virtualKey, 0);
    AppendKeyEvent(inputs, virtualKey, KEYEVENTF_KEYUP);

    if (shift) {
        AppendKeyEvent(inputs, VK_SHIFT, KEYEVENTF_KEYUP);
    }
    if (alt) {
        AppendKeyEvent(inputs, VK_MENU, KEYEVENTF_KEYUP);
    }
    if (ctrl) {
        AppendKeyEvent(inputs, VK_CONTROL, KEYEVENTF_KEYUP);
    }

    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) ==
           static_cast<UINT>(inputs.size());
}

bool TextInteractor::SendUnicodeText(const std::wstring& text) {
    if (text.empty()) {
        return false;
    }

    std::vector<INPUT> inputs;
    inputs.reserve(text.size() * 2);

    for (wchar_t ch : text) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = ch;
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(down);

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }

    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) ==
           static_cast<UINT>(inputs.size());
}

std::wstring TextInteractor::BuildClipboardProbeText() {
    wchar_t buffer[96] = {};
    wsprintfW(
        buffer,
        L"__EASYSW_PROBE_%lu_%lu__",
        GetCurrentProcessId(),
        GetTickCount());
    return buffer;
}

bool TextInteractor::WaitForClipboardTextChange(const std::wstring& probe, DWORD timeoutMs, std::wstring& text) {
    const DWORD started = GetTickCount();
    while (GetTickCount() - started < timeoutMs) {
        std::wstring current;
        if (ReadClipboardText(current) && current != probe) {
            text = current;
            return true;
        }
        Sleep(15);
    }
    return false;
}

bool TextInteractor::ReadClipboardText(std::wstring& text) {
    text.clear();

    if (!OpenClipboardWithRetry(nullptr)) {
        return false;
    }
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        CloseClipboard();
        return false;
    }

    HGLOBAL memory = GetClipboardData(CF_UNICODETEXT);
    if (memory == nullptr) {
        CloseClipboard();
        return false;
    }

    const wchar_t* raw = static_cast<const wchar_t*>(GlobalLock(memory));
    if (raw == nullptr) {
        CloseClipboard();
        return false;
    }

    text = raw;
    GlobalUnlock(memory);
    CloseClipboard();
    return true;
}

bool TextInteractor::SetClipboardText(const std::wstring& text) {
    if (!OpenClipboardWithRetry(nullptr)) {
        return false;
    }
    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) {
        CloseClipboard();
        return false;
    }

    void* raw = GlobalLock(memory);
    if (raw == nullptr) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    memcpy(raw, text.c_str(), bytes);
    GlobalUnlock(memory);

    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

bool TextInteractor::OpenClipboardWithRetry(HWND owner) {
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (OpenClipboard(owner)) {
            return true;
        }
        Sleep(15);
    }
    return false;
}
