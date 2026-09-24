#ifndef LITH_TYPEDLIST_H
#define LITH_TYPEDLIST_H

#include <Lith/BaseList.h>

template<class T> class CLTList : public CLTBaseList {
public:
    T* GetFirst() {
        return static_cast<T*>(CLTBaseList::GetFirst());
    }
    T* GetLast() {
        return static_cast<T*>(CLTBaseList::GetLast());
    }
};

#endif // LITH_TYPEDLIST_H
