#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Enums.h>
#include <Ints.h>

#include <stdio.h>

extern "C" void DiscardDebugOutput(char* line);

extern "C" void RezAssertFail(char* fmt, ...);
extern "C" void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...);
extern "C" void RezDebugPrintfCh(i32 channel, char* fmt, ...);
extern "C" void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...);

GZ_ENUM_BEGIN(DebugPrintMode)
    DEBUG_PRINT_DISABLED = 0,
    DEBUG_PRINT_DISCARD = 1,
    DEBUG_PRINT_MONO = 2,
    DEBUG_PRINT_COM1 = 3,
    DEBUG_PRINT_COM2 = 4,
    DEBUG_PRINT_FILE = 5,
    DEBUG_PRINT_FILE_APPEND = 6,
    DEBUG_PRINT_STDOUT = 7,
    DEBUG_PRINT_LPT = 8,
    DEBUG_PRINT_LPT2 = 9,
    DEBUG_PRINT_PRN = 10
GZ_ENUM_END(DebugPrintMode)

GZ_ENUM_CONST_BEGIN(DebugMonoGeometry)
    DEBUG_MONO_ROW_COUNT = 25
GZ_ENUM_CONST_END(DebugMonoGeometry)

extern DebugPrintMode g_debugPrintMode;

void DebugSetCursorXY(i32 x, i32 y);

void DebugSetCursor(i32, i32, i32);
int vsprintf(char* buf, const char* fmt, char* va);
void DiscardDebugOutput(char* line);

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
