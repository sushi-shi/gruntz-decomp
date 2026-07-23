#ifndef UTILS_PTRLISTPOOL_H
#define UTILS_PTRLISTPOOL_H

#include <Mfc.h>

// @identity-TODO
// Retail's guarded static-destructor helpers prove that these recycle lists are
// explicit specializations of a template static data member. The original
// template/member names, and whether the command and network pools shared one
// original template, are not recoverable from the stripped image.
template<class T> struct CPtrListPool {
    static CPtrList s_freeList;
};

#endif // UTILS_PTRLISTPOOL_H
