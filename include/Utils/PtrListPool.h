#ifndef UTILS_PTRLISTPOOL_H
#define UTILS_PTRLISTPOOL_H

#include <Mfc.h>

// @identity-TODO
// Only static-storage construction/destruction sites survive; an original mangled
// symbol or debug record would be needed to recover these source names.
template<class T> struct CPtrListPool {
    static CPtrList s_freeList;
};

#endif // UTILS_PTRLISTPOOL_H
