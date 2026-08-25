#ifndef GRUNTZ_ADDRWORD_H
#define GRUNTZ_ADDRWORD_H

#include <Ints.h>

// ABI-forced pointer/integer slot seam.
template<class T> union AddrWord {
    T* m_addr;
    i32 m_word;
    u32 m_uword;
};

#endif // GRUNTZ_ADDRWORD_H
