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

// A 16-bit-pixel view of a raw pixel cursor. Retail expands these at every use;
// they were transcribed per site in LightFxRender, DDrawShadeBlit, ShadeTableCache.
static inline u16* Pix16(void* p) {
    return static_cast<u16*>(p);
}

static inline const u16* Pix16(const void* p) {
    return static_cast<const u16*>(p);
}

#endif // GRUNTZ_PIX16_H
