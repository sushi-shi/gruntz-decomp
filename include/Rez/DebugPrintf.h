#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Ints.h>
#include <stdio.h>

extern "C" int vsprintf(char* buf, const char* fmt, char* va);
extern "C" void DiscardDebugOutput(char* line);

extern "C" void RezAssertFail(char* fmt, ...);
extern "C" void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...);
extern "C" void RezDebugPrintfCh(i32 channel, char* fmt, ...);
extern "C" void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...);

extern i32 g_debugPrintMode;

void DebugSetCursorXY(i32 x, i32 y);

void DebugSetCursor(i32, i32, i32);
int vsprintf(char* buf, const char* fmt, char* va);
void DiscardDebugOutput(char* line);

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
