#ifndef INCLUDE_REZ_FRAMECOUNTDOWN_H
#define INCLUDE_REZ_FRAMECOUNTDOWN_H

#include <Ints.h>

inline u32 CountdownRemaining(u32 remaining, u32 elapsed) {
    return elapsed >= remaining ? 0 : remaining - elapsed;
}

template<class T> inline void CountDown(T& remaining, const u32& elapsed) {
    if (elapsed >= static_cast<u32>(remaining)) {
        remaining = 0;
    } else {
        remaining -= elapsed;
    }
}

#endif // INCLUDE_REZ_FRAMECOUNTDOWN_H
