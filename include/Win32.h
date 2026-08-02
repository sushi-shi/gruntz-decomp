#ifndef GRUNTZ_WIN32_H
#define GRUNTZ_WIN32_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

typedef int INT_PTR;

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_WIN32_H
