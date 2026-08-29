#ifndef GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
#define GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H

#include <Win32.h>

#include <Ints.h>

struct dprintfFromTo {
    u32 From;
    u32 To;
};

class dprintfExcludeRegions {
public:
    BOOLEAN In(u32 Num);
    void Add(u32 From, u32 To);
    void Scan(char* Str);

    u32 NumRegions;
    dprintfFromTo Ary[16];
};

class dprintfinittype {
public:
    dprintfinittype();
    ~dprintfinittype();
};

#endif // GRUNTZ_REZ_DEBUGPRINTFINTERNALS_H
