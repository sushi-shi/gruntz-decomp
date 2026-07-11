// AniAdvanceCursor.h - CAniAdvanceCursor, a CLoadable anim-advance cursor (the
// m_38+0x1a0 sub-object). Promoted so the reduced per-TU advance/geo views fold
// onto it. wave4-L: the satellite advance types (CAniRenderCtx/CAniDesc/
// CAniBlitTrigger) live in <DDrawMgr/AniAdvance.h>; the pointer members here are
// typed via forward declarations (the Advance body is the I obj, TriggerBlit the
// G obj - dossier #15).
#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <rva.h>

class CAniRenderCtx; // the sprite-render context (satellite def in AniAdvance.h)
class CAniDesc;      // the animation descriptor (playlist entry)
class CAniElement;   // the descriptor playlist (<Gruntz/AniElement.h>)

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
    i32 Advance_15c360(u32 elapsed);       // 0x15c360 (advance / set-geo-source)

    CAniRenderCtx* m_10; // +0x10  sprite-render context
    CAniElement* m_14;   // +0x14  descriptor playlist (CObArray of CAniDesc)
    CAniDesc* m_18;      // +0x18  current descriptor
    i32 m_1c;            // +0x1c  playlist index
    u32 m_20;            // +0x20  per-frame timer remaining (ticks)
    i32 m_24;            // +0x24  "decrement-each-tick" flag
    i32 m_28;            // +0x28  paused/done flag
    i32 m_2c;            // +0x2c  owns-buffer flag (consume the draw value on read)
    i32 m_pendingDraw;   // +0x30  pending draw value
    i32 m_curDraw;       // +0x34  current draw value
    i32 m_speed;         // +0x38  float speed multiplier (raw bits; vs 1.0f)
};
VTBL(CAniAdvanceCursor, 0x001f0128); // ??_7CAniAdvanceCursor@@6B@ (9-slot CLoadable-derived)

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
