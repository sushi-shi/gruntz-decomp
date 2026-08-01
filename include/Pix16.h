#ifndef GRUNTZ_PIX16_H
#define GRUNTZ_PIX16_H

#include <Ints.h>

union Pix16Ptr {
    u8* m_bytes;
    char* m_chars;
    u16* m_words;
    i16* m_swords;
    i32* m_dwords;
};

union RecordBytes {
    void* m_rec;
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

#endif // GRUNTZ_PIX16_H
