#ifndef GRUNTZ_PIX16_H
#define GRUNTZ_PIX16_H

#include <Ints.h>

// TWO seams, both re-reading ONE allocation at a different element width.
//
// Pix16Ptr / Pix16CPtr: a locked DirectDraw surface. DirectDraw hands back a
// byte pointer and a byte pitch, and the same rows are stepped as u16 at 16bpp
// and as i32 for the two-pixel-at-a-time paths - so the width is a property of
// the loop, not of the surface. Every member is in use; the const twin exists
// because a source cursor is const and a union has no const-conversion.
//
// RecordBytes: a `new u8[len]` file blob whose head is a fixed record
// (PcxHeader, PidHeader, BITMAPINFOHEADER, WwdHeader, DeviceState). The byte
// allocation is byte-forced - retail's operator new[] takes the byte count - so
// the record view cannot be moved into the allocation's own type.
//
// Neither is a licence for a byte cursor in general: an u8*/char* that walks a
// TYPED array is an unmodelled type, and the fix is that type, not a member
// here.

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
