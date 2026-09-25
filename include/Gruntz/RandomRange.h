#ifndef GRUNTZ_RANDOMRANGE_H
#define GRUNTZ_RANDOMRANGE_H

#include <Gruntz/GruntzMgr.h>

inline i32 RandRange(CGruntzMgr* mgr, i32 lo, i32 hi) {
    if ((hi - lo + 1) == 0) {
        if (mgr->Rand() & 1) {
            return lo;
        } else {
            return hi;
        }
    }
    return (mgr->Rand() % (hi - lo + 1)) + lo;
}

#endif // GRUNTZ_RANDOMRANGE_H
