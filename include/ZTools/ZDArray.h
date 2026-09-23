#ifndef GRUNTZ_ZTOOLS_ZDARRAY_H
#define GRUNTZ_ZTOOLS_ZDARRAY_H

#include <ZTools/PlacementNew.h>
#include <ZTools/ZVec.h>

template<class T> class zDArray : public _zdvec {
public:
    zDArray(i32 lo, i32 hi);
    virtual ~zDArray() OVERRIDE;

    T& operator[](i32 id);
};

template<class T>
inline zDArray<T>::zDArray(i32 lo, i32 hi) : _zdvec(sizeof(T), lo, hi, ZVecNoScratch()) {
    T* p = static_cast<T*>(m_alloc);
    if (!p) {
        return;
    }
    for (i32 i = m_grown; i--; ++p) {
        T* t = new (p, 0, 0) T;
    }
}

template<class T> inline zDArray<T>::~zDArray() {
    T* p = static_cast<T*>(static_cast<void*>(m_base));
    if (!p) {
        return;
    }
    for (i32 i = m_hi - m_lo + 1; i--; ++p) {
        p->~T();
    }
}

template<class T> T& zDArray<T>::operator[](i32 i) {
    T *t, *rv = static_cast<T*>(IndexToPtr(i)), *p = static_cast<T*>(m_alloc);
    for (i32 j = m_grown; j--; ++p) {
        t = new (p, 0, 0) T;
    }
    return *rv;
}

#endif // GRUNTZ_ZTOOLS_ZDARRAY_H
