// Effect6b.h - CEffect6b, the CGrunt +0x150 sub-object (Apply @0x6b2e0 advances the
// grunt's anim sink). Extracted from OrphanMethods.cpp so CGrunt (Grunt.cpp) can reach
// it cast-free (was the CGruntExitHolder view).
#ifndef GRUNTZ_EFFECT6B_H
#define GRUNTZ_EFFECT6B_H

#include <Ints.h>
#include <rva.h>

class CAnimOwner6b;

SIZE_UNKNOWN(CEffect6b);
struct CEffect6b {
    char _00[4];
    CAnimOwner6b* m_4; // +0x04
    char _08[0xc - 8];
    i32 m_c;                  // +0x0c
    void Apply(i32 a, i32 b); // 0x6b2e0
};

#endif // GRUNTZ_EFFECT6B_H
