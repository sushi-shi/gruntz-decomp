#ifndef UTILS_PTRLISTPOOL_H
#define UTILS_PTRLISTPOOL_H

template<class T> struct CPtrListPool {
    static CPtrList s_freeList;
};

#endif
