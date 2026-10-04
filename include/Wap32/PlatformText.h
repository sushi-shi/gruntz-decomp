#ifndef GRUNTZ_WAP32_PLATFORMTEXT_H
#define GRUNTZ_WAP32_PLATFORMTEXT_H

#include <string>

bool loadResourceText(unsigned int id, std::string& text);
std::string readWindowText(HWND window);
std::string readListBoxText(HWND window, int index, bool combo);

#endif
