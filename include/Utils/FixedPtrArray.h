#ifndef UTILS_FIXEDPTRARRAY_H
#define UTILS_FIXEDPTRARRAY_H

#include <rva.h>

#include <Ints.h>

#include <stddef.h>

template<class T, i32 Capacity> class CFixedPtrArray {
public:
    CFixedPtrArray() : m_reserved00(0), m_count(0) {}

    void Clear();
    i32 FillFrom(T** src, i32 n, i32 unused);
    i32 Add(T* item);

    i32 m_reserved00;
    i32 m_count;
    T* m_items[Capacity];
};

template<class T, i32 Capacity>
i32 CFixedPtrArray<T, Capacity>::FillFrom(T** src, i32 n, i32 unused) {
    if (!src) {
        return 0;
    }
    if (n >= Capacity) {
        return 0;
    }
    m_reserved00 = 0;
    m_count = 0;
    for (i32 j = 0; j < Capacity; j++) {
        m_items[j] = NULL;
    }
    for (i32 i = 0; i < n; i++) {
        if (src[i]) {
            if (!Add(src[i])) {
                return 0;
            }
        }
    }
    return 1;
}

template<class T, i32 Capacity> void CFixedPtrArray<T, Capacity>::Clear() {
    for (i32 j = 0; j < Capacity; j++) {
        m_items[j] = NULL;
    }
    m_count = 0;
}

template<class T, i32 Capacity> i32 CFixedPtrArray<T, Capacity>::Add(T* item) {
    if (m_count >= Capacity) {
        return 0;
    }
    m_items[m_count] = item;
    m_count++;
    return 1;
}

#endif // UTILS_FIXEDPTRARRAY_H
