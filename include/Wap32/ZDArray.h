#ifndef GRUNTZ_WAP32_ZDARRAY_H
#define GRUNTZ_WAP32_ZDARRAY_H

#include <Wap32/ZVec.h>

#include <new>

template<class T>
inline zDArray<T>::zDArray(i32 lo, i32 hi) : _zdvec(sizeof(T), lo, hi, ZVecNoScratch()) {
    T* p = AsElem(m_alloc);
    if (!p) {
        return;
    }
    for (i32 i = m_grown; i--; ++p) {
        T* t = new (p) T;
    }
}

template<class T> inline zDArray<T>::~zDArray() {
    T* p = AsElem(m_base);
    if (!p) {
        return;
    }
    for (i32 i = m_hi - m_lo + 1; i--; ++p) {
        p->~T();
    }
}

#endif // GRUNTZ_WAP32_ZDARRAY_H
