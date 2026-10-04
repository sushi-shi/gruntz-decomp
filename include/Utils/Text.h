#ifndef GRUNTZ_UTILS_TEXT_H
#define GRUNTZ_UTILS_TEXT_H

#include <string>
#include <algorithm>
#include <climits>
#include <cstdarg>

char asciiUpper(char value);
char asciiLower(char value);
int compareAsciiCaseInsensitive(const std::string& left, const std::string& right);
int stringIndex(std::string::size_type position);
std::string sliceText(const std::string& text, int start, int count = INT_MAX);
std::string rightText(const std::string& text, int count);
std::string joinResourceKey(const std::string& prefix, const std::string& separator, const std::string& name);
bool copyTextToBuffer(const std::string& text, char* buffer, std::string::size_type capacity);
std::string formatTextV(const char* format, va_list arguments);
std::string formatText(const char* format, ...);

#endif
