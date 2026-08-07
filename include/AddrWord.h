#ifndef GRUNTZ_ADDRWORD_H
#define GRUNTZ_ADDRWORD_H

#include <Ints.h>

template<class T> union AddrWord {
    T* m_addr;
    char* m_bytes;

    struct tagRECT* m_rect;
    i32 m_word;
    u32 m_uword;
};

// A Win32/COM out-parameter declared `void**` fed by the address of a typed member
// (cf. MapOutRef for the MFC container form).
template<class T> union PtrOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

template<class T> inline void** PtrOut(T** p) {
    PtrOutRef<T> r;
    r.m_asTyped = p;
    return r.m_asVoid;
}

#endif // GRUNTZ_ADDRWORD_H
