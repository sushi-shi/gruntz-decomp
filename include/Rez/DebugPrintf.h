#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Enums.h>
#include <Ints.h>

void dprintf(char* fmt, ...);
void dprintf(i32 x, i32 y, char* fmt, ...);
void dprintf(u32 level, char* fmt, ...);
void dprintf(u32 level, i32 x, i32 y, char* fmt, ...);

GZ_ENUM_BEGIN(dprintfOutputType)
    DPRINTF_UNKNOWN = 0,
    DPRINTF_NOTHING = 1,
    DPRINTF_MONOCHROME = 2,
    DPRINTF_COM1 = 3,
    DPRINTF_COM2 = 4,
    DPRINTF_FILE = 5,
    DPRINTF_FILEAPPEND = 6,
    DPRINTF_STDOUT = 7,
    DPRINTF_LPT1 = 8,
    DPRINTF_LPT2 = 9,
    DPRINTF_PRN = 10
GZ_ENUM_END(dprintfOutputType)

GZ_ENUM_CONST_BEGIN(DebugMonoGeometry)
    DEBUG_PRINT_BUFFER_SIZE = 256,
    DEBUG_MONO_COLUMN_COUNT = 80,
    DEBUG_MONO_ROW_COUNT = 25,
    DEBUG_MONO_ATTRIBUTE = 0x700
GZ_ENUM_CONST_END(DebugMonoGeometry)

void dgotoxy(i32 x, i32 y);

void dgotoxy(u32 level, i32 x, i32 y);
void dclrscr();
void dclrscr(u32 level);

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
