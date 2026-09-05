#ifndef GRUNTZ_RANDOMRANGE_H
#define GRUNTZ_RANDOMRANGE_H

#include <Gruntz/GruntzMgr.h>

inline i32 RandRange(CGruntzMgr* mgr, i32 lo, i32 hi) {
    i32 range = hi - lo + 1;
    if (range == 0) {
        return (mgr->Rand() & 1) ? lo : hi;
    }
    return mgr->Rand() % range + lo;
}

#endif // GRUNTZ_RANDOMRANGE_H
