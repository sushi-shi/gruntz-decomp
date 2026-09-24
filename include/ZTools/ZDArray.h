#ifndef GRUNTZ_ZTOOLS_ZDARRAY_H
#define GRUNTZ_ZTOOLS_ZDARRAY_H

#include <ZTools/PlacementNew.h>
#include <ZTools/ZVec.h>

template<class T> class zDArray : public _zdvec {
public:
    zDArray(i32 lo = 0, i32 hi = 0);
    virtual ~zDArray() OVERRIDE;

    T& operator[](i32 id);
    void extend(i32 l, i32 h);
};

template<class T> zDArray<T>::zDArray(i32 lo, i32 hi) : _zdvec(sizeof(T), lo, hi, ZVecNoScratch()) {
    T* p = static_cast<T*>(init);
    if (!p) {
        return;
    }
    for (i32 i = initcount; i--; ++p) {
        T* t = new (p, 0, 0) T;
    }
}

template<class T> zDArray<T>::~zDArray() {
    T* p = static_cast<T*>(static_cast<void*>(vec));
    if (!p) {
        return;
    }
    for (i32 i = hi - lo + 1; i--; ++p) {
        p->~T();
    }
}

template<class T> T& zDArray<T>::operator[](i32 i) {
    T *t, *rv = static_cast<T*>(get(i)), *p = static_cast<T*>(init);
    for (i32 j = initcount; j--; ++p) {
        t = new (p, 0, 0) T;
    }
    return *rv;
}

template<class T> void zDArray<T>::extend(i32 l, i32 h) {
    T *t, *p;
    if (l > h) {
        handle("Inconsistent bounds", EINVAL);
        return;
    }
    if (l < lo) {
        realloc(l);
        p = static_cast<T*>(init);
        for (i32 j = initcount; j--; ++p) {
            t = new (p, 0, 0) T;
        }
    }
    if (h > hi) {
        realloc(h);
        p = static_cast<T*>(init);
        for (i32 j = initcount; j--; ++p) {
            t = new (p, 0, 0) T;
        }
    }
}

#endif // GRUNTZ_ZTOOLS_ZDARRAY_H
