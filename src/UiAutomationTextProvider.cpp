#include "UiAutomationTextProvider.h"

#include <cwctype>
#include <ole2.h>
#include <uiautomation.h>

namespace {

template <typename T>
void SafeRelease(T*& value) {
    if (value != nullptr) {
        value->Release();
        value = nullptr;
    }
}

std::wstring TrimWhitespace(const std::wstring& value) {
    size_t start = 0;
    while (start < value.size() && iswspace(value[start])) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && iswspace(value[end - 1])) {
        --end;
    }

    return value.substr(start, end - start);
}

bool TryGetTextPattern(IUIAutomationElement* element, IUIAutomationTextPattern*& pattern) {
    pattern = nullptr;
    if (element == nullptr) {
        return false;
    }

    IUnknown* unknown = nullptr;
    const HRESULT hr = element->GetCurrentPattern(UIA_TextPatternId, &unknown);
    if (FAILED(hr) || unknown == nullptr) {
        return false;
    }

    const HRESULT queryHr = unknown->QueryInterface(IID_PPV_ARGS(&pattern));
    unknown->Release();
    return SUCCEEDED(queryHr) && pattern != nullptr;
}

bool TryGetFocusedTextPattern(
    IUIAutomation* automation,
    IUIAutomationElement*& focusedElement,
    IUIAutomationTextPattern*& textPattern) {
    focusedElement = nullptr;
    textPattern = nullptr;

    if (automation == nullptr) {
        return false;
    }

    if (FAILED(automation->GetFocusedElement(&focusedElement)) || focusedElement == nullptr) {
        return false;
    }

    if (!TryGetTextPattern(focusedElement, textPattern)) {
        focusedElement->Release();
        focusedElement = nullptr;
        return false;
    }

    return true;
}

} // namespace

UiAutomationTextProvider::UiAutomationTextProvider() {
    CoCreateInstance(
        CLSID_CUIAutomation,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&automation_));
}

UiAutomationTextProvider::~UiAutomationTextProvider() {
    SafeRelease(automation_);
}

bool UiAutomationTextProvider::IsAvailable() const {
    return automation_ != nullptr;
}

bool UiAutomationTextProvider::TryGetSelectedText(std::wstring& text) const {
    text.clear();
    IUIAutomationElement* focusedElement = nullptr;
    IUIAutomationTextPattern* textPattern = nullptr;
    if (!TryGetFocusedTextPattern(automation_, focusedElement, textPattern)) {
        return false;
    }

    IUIAutomationTextRangeArray* ranges = nullptr;
    const HRESULT selectionHr = textPattern->GetSelection(&ranges);
    textPattern->Release();
    focusedElement->Release();

    if (FAILED(selectionHr) || ranges == nullptr) {
        return false;
    }

    int length = 0;
    if (FAILED(ranges->get_Length(&length)) || length <= 0) {
        ranges->Release();
        return false;
    }

    std::wstring combined;
    for (int index = 0; index < length; ++index) {
        IUIAutomationTextRange* range = nullptr;
        if (FAILED(ranges->GetElement(index, &range)) || range == nullptr) {
            continue;
        }

        BSTR chunk = nullptr;
        if (SUCCEEDED(range->GetText(-1, &chunk)) && chunk != nullptr) {
            combined.append(chunk, SysStringLen(chunk));
            SysFreeString(chunk);
        }

        range->Release();
    }

    ranges->Release();
    combined = TrimWhitespace(combined);
    if (combined.empty()) {
        return false;
    }

    text = combined;
    return true;
}

bool UiAutomationTextProvider::TrySelectWordAtCaret(std::wstring& text) const {
    text.clear();

    IUIAutomationElement* focusedElement = nullptr;
    IUIAutomationTextPattern* textPattern = nullptr;
    if (!TryGetFocusedTextPattern(automation_, focusedElement, textPattern)) {
        return false;
    }

    IUIAutomationTextRangeArray* ranges = nullptr;
    const HRESULT selectionHr = textPattern->GetSelection(&ranges);
    textPattern->Release();
    focusedElement->Release();

    if (FAILED(selectionHr) || ranges == nullptr) {
        return false;
    }

    int length = 0;
    if (FAILED(ranges->get_Length(&length)) || length <= 0) {
        ranges->Release();
        return false;
    }

    IUIAutomationTextRange* range = nullptr;
    if (FAILED(ranges->GetElement(0, &range)) || range == nullptr) {
        ranges->Release();
        return false;
    }
    ranges->Release();

    IUIAutomationTextRange* wordRange = nullptr;
    if (FAILED(range->Clone(&wordRange)) || wordRange == nullptr) {
        range->Release();
        return false;
    }
    range->Release();

    if (FAILED(wordRange->ExpandToEnclosingUnit(TextUnit_Word))) {
        wordRange->Release();
        return false;
    }

    BSTR rawText = nullptr;
    std::wstring selectedText;
    if (SUCCEEDED(wordRange->GetText(-1, &rawText)) && rawText != nullptr) {
        selectedText.assign(rawText, SysStringLen(rawText));
        SysFreeString(rawText);
    }

    selectedText = TrimWhitespace(selectedText);
    if (selectedText.empty()) {
        wordRange->Release();
        return false;
    }

    if (FAILED(wordRange->Select())) {
        wordRange->Release();
        return false;
    }

    wordRange->Release();
    text = selectedText;
    return true;
}
