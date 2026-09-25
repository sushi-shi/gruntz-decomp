#ifndef GRUNTZ_ZTOOLS_PLACEMENTNEW_H
#define GRUNTZ_ZTOOLS_PLACEMENTNEW_H

#include <stddef.h>

inline void* operator new(size_t size, void* ptr, int dummy1, int dummy2) {
    return ptr;
}

#endif // GRUNTZ_ZTOOLS_PLACEMENTNEW_H
