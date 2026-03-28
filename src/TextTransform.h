#pragma once

#include <string>

namespace TextTransform {

enum class Direction {
    EnglishToRussian,
    RussianToEnglish,
    Unknown,
};

Direction GuessDirection(const std::wstring& text);
std::wstring FixLayout(const std::wstring& text);

} // namespace TextTransform
