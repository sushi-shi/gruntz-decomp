#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif

#include <Utils/Text.h>
#include <cstdio>
#include <stdexcept>
#include <vector>

char asciiUpper(char value) {
    return value >= 'a' && value <= 'z' ? static_cast<char>(value - 'a' + 'A') : value;
}

char asciiLower(char value) {
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

int compareAsciiCaseInsensitive(const std::string& left, const std::string& right) {
    const std::string::size_type count = (left.size() < right.size() ? left.size() : right.size());
    for (std::string::size_type i = 0; i < count; ++i) {
        const unsigned char a = static_cast<unsigned char>(asciiLower(left[i]));
        const unsigned char b = static_cast<unsigned char>(asciiLower(right[i]));
        if (a != b) return a < b ? -1 : 1;
    }
    return left.size() == right.size() ? 0 : left.size() < right.size() ? -1 : 1;
}

int stringIndex(std::string::size_type position) {
    if (position == std::string::npos) return -1;
    if (position > INT_MAX) throw std::length_error("Text index exceeds game limits");
    return static_cast<int>(position);
}

std::string sliceText(const std::string& text, int start, int count) {
    const std::string::size_type offset = (start < 0 ? 0 : static_cast<std::string::size_type>(start) > text.size() ? text.size() : start);
    return text.substr(offset, static_cast<std::string::size_type>(count < 0 ? 0 : count));
}

std::string rightText(const std::string& text, int count) {
    const std::string::size_type length = (count < 0 ? 0 : static_cast<std::string::size_type>(count) > text.size() ? text.size() : count);
    return text.substr(text.size() - length);
}

std::string formatTextV(const char* format, va_list arguments) {
    std::vector<char> buffer(256);
    for (;;) {
        va_list copy;
#if defined(_MSC_VER)
        copy = arguments;
#else
        va_copy(copy, arguments);
#endif
#if defined(_MSC_VER) && _MSC_VER < 1900
        const int written = _vsnprintf(&buffer[0], buffer.size(), format, copy);
#else
        const int written = vsnprintf(&buffer[0], buffer.size(), format, copy);
#endif
        va_end(copy);
        if (written >= 0 && static_cast<std::vector<char>::size_type>(written) < buffer.size()) {
            return std::string(&buffer[0], written);
        }
        if (buffer.size() > static_cast<std::vector<char>::size_type>(INT_MAX / 2)) {
            throw std::length_error("Formatted text exceeds game limits");
        }
        buffer.resize(written >= 0 ? static_cast<std::vector<char>::size_type>(written) + 1 : buffer.size() * 2);
    }
}

std::string formatText(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    try {
        std::string result = formatTextV(format, arguments);
        va_end(arguments);
        return result;
    } catch (...) {
        va_end(arguments);
        throw;
    }
}
