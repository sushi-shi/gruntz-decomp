#ifndef GRUNTZ_PIX16_H
#define GRUNTZ_PIX16_H

#include <Ints.h>

// Byte-forced views of raw surface and record allocations.

union Pix16Ptr {
    u8* m_bytes;
    char* m_chars;
    u16* m_words;
    i16* m_swords;
    i32* m_dwords;
};

template<class T> union RecordBytes {
    T* m_rec;
    u8* m_bytes;
    char* m_chars;
    i32* m_dwords;
};

union Pix16CPtr {
    const u8* m_bytes;
    const char* m_chars;
    const u16* m_words;
    const i16* m_swords;
    const i32* m_dwords;
};

static inline u16* Pix16(u8* p) {
    Pix16Ptr view;
    view.m_bytes = p;
    return view.m_words;
}

static inline u16* Pix16(char* p) {
    Pix16Ptr view;
    view.m_chars = p;
    return view.m_words;
}

#endif // GRUNTZ_PIX16_H
