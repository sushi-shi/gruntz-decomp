#ifndef UTILS_PTRLISTPOOL_H
#define UTILS_PTRLISTPOOL_H

#include <list>

template<class T> struct ObjectPoolStorage {
    static std::list<T*> s_freeList;
};

#endif
