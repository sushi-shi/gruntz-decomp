#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIMEINLINE_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIMEINLINE_H

#include <Gruntz/TypeCollRuntime.h>

inline CTypeCollRuntime::CTypeCollRuntime()

    : _zdvec(sizeof(CString), 0x7d0, 0x7da, ZVecNoScratch()) {
    CString* slot = Slots();
    if (slot != NULL) {
        i32 cnt = m_grown;
        while (cnt-- != 0) {
            if (slot != NULL) {
                slot->CString::CString();
            }
            ++slot;
        }
    }
}

inline CTypeCollRuntime::~CTypeCollRuntime() {
    CString* item = Elem(m_lo);
    if (item != NULL) {
        i32 count = m_hi - m_lo + 1;
        while (count-- != 0) {
            item->CString::~CString();
            ++item;
        }
    }
}

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIMEINLINE_H
