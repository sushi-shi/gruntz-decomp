#ifndef GRUNTZ_WAP32_ZDARRAYINDEX_H
#define GRUNTZ_WAP32_ZDARRAYINDEX_H

#include <Wap32/ZVec.h>

#include <new>

template<class T> T& zDArray<T>::operator[](i32 i) {
    T* t;
    T* rv = AsElem(IndexToPtr(i));
    T* p = AsElem(m_alloc);
    for (i32 j = m_grown; j--; ++p) {
        t = new (p) T;
    }
    return *rv;
}

#endif // GRUNTZ_WAP32_ZDARRAYINDEX_H
