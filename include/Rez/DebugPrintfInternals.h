#ifndef GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
#define GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H

#include <Win32.h>

#include <Ints.h>

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
    dprintfFromTo Ary[16];
    BOOLEAN In(u32 Num);
    void Add(u32 From, u32 To);
    void Scan(char* Str);
};

#endif // GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
