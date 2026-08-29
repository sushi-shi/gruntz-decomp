#ifndef GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
#define GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H

#include <Win32.h>

#include <Enums.h>
#include <Ints.h>

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

GZ_ENUM_CONST_BEGIN(dprintfConstants)
    BUFSIZE = 256,
    CPL = 80,
    LPP = 25,
    ATTR = 0x700,
    MAX_EXCLUDE_REGIONS = 16
GZ_ENUM_CONST_END(dprintfConstants)

class dprintfinittype {
public:
    dprintfinittype();
    ~dprintfinittype();
};

class dprintfFromTo {
public:
    u32 From;
    u32 To;
};

class dprintfExcludeRegions {
public:
    u32 NumRegions;
    dprintfFromTo Ary[MAX_EXCLUDE_REGIONS];
    BOOLEAN In(u32 Num);
    void Add(u32 From, u32 To);
    void Scan(char* Str);
};

#endif // GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
