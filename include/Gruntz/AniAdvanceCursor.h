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
    virtual ~CAniAdvanceCursor() OVERRIDE; // slot 1 (scalar-deleting dtor 0x15b6b0)
    virtual i32 IsLoaded() OVERRIDE;       // slot 5  0x15b6a0
    virtual i32 IsReady() OVERRIDE;        // slot 6  0x001c08 (CWapObj default)
    virtual i32 Unload() OVERRIDE;         // slot 7  0x15c2c0
    virtual i32 GetClassId() OVERRIDE;     // slot 8  0x154a00 (CLoadable default)
    i32 Advance_15c360(u32 ctx);           // 0x15c360 (advance / set-geo-source)

    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
};
VTBL(CAniAdvanceCursor, 0x001f0128); // ??_7CAniAdvanceCursor@@6B@ (9-slot CLoadable-derived)

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
