#include "TextTransform.h"

#include <unordered_map>

namespace {

using CharMap = std::unordered_map<wchar_t, wchar_t>;

CharMap BuildEnglishToRussianMap() {
    CharMap map;

    const std::wstring en = L"`qwertyuiop[]asdfghjkl;'zxcvbnm,./"
                            L"~QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?";
    const std::wstring ru = L"ёйцукенгшщзхъфывапролджэячсмитьбю."
                            L"ЁЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ,";

    for (size_t i = 0; i < en.size() && i < ru.size(); ++i) {
        map.emplace(en[i], ru[i]);
    }
    return map;
}

const CharMap& EnglishToRussian() {
    static const CharMap map = BuildEnglishToRussianMap();
    return map;
}

const CharMap& RussianToEnglish() {
    static const CharMap map = [] {
        CharMap reversed;
        for (const auto& pair : EnglishToRussian()) {
            reversed.emplace(pair.second, pair.first);
        }
        return reversed;
    }();
    return map;
}

bool IsLatinLetter(wchar_t ch) {
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

bool IsCyrillicLetter(wchar_t ch) {
    return (ch >= L'а' && ch <= L'я') ||
           (ch >= L'А' && ch <= L'Я') ||
           ch == L'ё' || ch == L'Ё';
}

std::wstring ApplyMap(const std::wstring& text, const CharMap& map) {
    std::wstring result;
    result.reserve(text.size());

    for (wchar_t ch : text) {
        const auto it = map.find(ch);
        result.push_back(it != map.end() ? it->second : ch);
    }

    return result;
}

} // namespace

namespace TextTransform {

Direction GuessDirection(const std::wstring& text) {
    int latin = 0;
    int cyrillic = 0;

    for (wchar_t ch : text) {
        if (IsLatinLetter(ch)) {
            ++latin;
        } else if (IsCyrillicLetter(ch)) {
            ++cyrillic;
        }
    }

    if (latin > cyrillic) {
        return Direction::EnglishToRussian;
    }
    if (cyrillic > latin) {
        return Direction::RussianToEnglish;
    }

    for (wchar_t ch : text) {
        if (EnglishToRussian().count(ch) != 0) {
            return Direction::EnglishToRussian;
        }
        if (RussianToEnglish().count(ch) != 0) {
            return Direction::RussianToEnglish;
        }
    }

    return Direction::Unknown;
}

std::wstring FixLayout(const std::wstring& text) {
    switch (GuessDirection(text)) {
    case Direction::EnglishToRussian:
        return ApplyMap(text, EnglishToRussian());
    case Direction::RussianToEnglish:
        return ApplyMap(text, RussianToEnglish());
    case Direction::Unknown:
    default:
        return text;
    }
}

} // namespace TextTransform
