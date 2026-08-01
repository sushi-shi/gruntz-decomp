#ifndef GRUNTZ_ADDRWORD_H
#define GRUNTZ_ADDRWORD_H

#include <Ints.h>

union AddrWord {
    void* m_addr;

    struct tagRECT* m_rect;
    i32 m_word;
    u32 m_uword;
};

#endif // GRUNTZ_ADDRWORD_H
