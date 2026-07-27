#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <rva.h>

class CWwdGameObjectA;  // the owning wide game object (<Wwd/WwdGameObjectFamily.h>)
class CAniDesc;      // the animation descriptor (playlist entry; == CAniRecord)
class CAniElement;   // the descriptor playlist (<Gruntz/AniElement.h>; the ex
class CFileMemBase;

class CAniAdvanceCursor : public CLoadable {
public:
    CAniAdvanceCursor() {} // default (view-embed)
    CAniAdvanceCursor(i32 owner, i32 field04, i32 field08);
    virtual ~CAniAdvanceCursor() OVERRIDE; // slot 1 (scalar-deleting dtor 0x15b6b0)
    virtual i32 IsLoaded() OVERRIDE;       // slot 5  0x15b6a0
    // slot 6 IsReady (0x001c08) + slot 8 GetClassId (0x154a00) are INHERITED from
    // CLoadable (same body RVAs; audit: redeclare-nothing).
    // slot 7 - clear the bound source refs (m_10/m_14/m_element). The ex
    // "CDDrawBlitParam::Reset_15c2c0" - the body IS the Unload override.
    virtual void Unload() OVERRIDE; // slot 7  0x15c2c0

    // --- the runtime/serialize method set (ex-CDDrawBlitParam; bodies in
    // WwdFactoryObject.cpp / DDrawSubMgr.cpp) ---
    // Bind the owner. There is exactly ONE caller in retail - CWwdGameObjectA::Setup
    // @0x15b940, which does `push esi / lea ecx,[esi+0x1a0] / call` i.e. passes its own
    // `this`. (The old "role-dependent, also a worker source on the blit path" note was
    // unsupported: sema xref --raw finds no second caller, and it was the sole reason
    // the parameter stayed void* and m_10 wore the CAniRenderCtx view.)
    void Construct(CWwdGameObjectA* src);                 // 0x15c290
    void Setup(CAniElement* src);                         // 0x15c2d0  bind a resolved geo source
    void Recompute(i32 a1);                               // 0x15c320  re-derive from the bound m_14
    i32 Serialize(CFileMemBase* ar);                      // 0x15c970
    i32 Deserialize(CFileMemBase* ar);                    // 0x15ca70
    i32 Find(CFileMemBase* ar, i32 type, i32 a3, i32 a4); // 0x15c900
    i32 Advance(u32 elapsed);                             // 0x15c360 (advance / set-geo-source)

    // (+0x0c is the inherited CLoadable owner slot m_0c: the game object /
    // blit worker that owns this cursor - the ex "m_worker".)
    // +0x10  the owning wide game object (a back-pointer to the object this cursor is
    // embedded in at its +0x1a0). The ex-CAniRenderCtx view was that class read through
    // pads: its m_flags/+0x08, m_posModeX-Y/+0x10-14, m_anchor/+0x38, m_byteFlags/+0x40,
    // m_screenX-Y/+0x5c-60 are CLoadable::m_flags + CResolveNode::m_10/m_14/m_dirtyArmed/
    // m_stateFlags/m_screenX/m_screenY, and its m_frameCursor/m_frameSeq/m_curFrame
    // (+0x190/+0x194/+0x198) are CWwdGameObjectA::m_190/m_sprite/m_layer.
    CWwdGameObjectA* m_10;
    CAniElement* m_14;   // +0x14  descriptor playlist (the resolved geo source;
                         //        ex "m_srcRef" - the serialize map value)
    CAniDesc* m_element; // +0x18  current descriptor/element (transient)
    i32 m_index;         // +0x1c  playlist index
    u32 m_20;            // +0x20  per-frame timer remaining (ticks)
    i32 m_24;            // +0x24  "decrement-each-tick" flag
    i32 m_28;            // +0x28  paused/done flag
    i32 m_2c;            // +0x2c  owns-buffer flag (consume the draw value on read)
    i32 m_pendingDraw;   // +0x30  pending draw value
    i32 m_curDraw;       // +0x34  current draw value
    float m_scale;       // +0x38  float speed/scale multiplier (Advance compares
                         //        it as raw bits vs 0x3f800000 == 1.0f)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
