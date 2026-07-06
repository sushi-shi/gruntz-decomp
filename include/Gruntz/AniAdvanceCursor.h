// AniAdvanceCursor.h - CAniAdvanceCursor, a CLoadable anim-advance cursor (the
// m_38+0x1a0 sub-object). Promoted so the reduced per-TU advance/geo views fold onto it.
#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CAniAdvanceCursor);
class CAniAdvanceCursor : public CLoadable {
public:
    CAniAdvanceCursor() {} // default (view-embed)
    CAniAdvanceCursor(i32 owner, i32 field04, i32 field08);
    ~CAniAdvanceCursor();        // slot 1 (scalar-deleting dtor 0x15b6b0)
    i32 IsLoaded();              // slot 5  0x15b6a0
    i32 Unload();                // slot 7  0x15c2c0
    i32 Advance_15c360(u32 ctx); // 0x15c360 (advance / set-geo-source)

    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
};

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
