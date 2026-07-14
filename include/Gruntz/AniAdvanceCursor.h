// AniAdvanceCursor.h - CAniAdvanceCursor, THE 0x3c-byte CLoadable anim-advance /
// blit-command cursor embedded at the wide game object's +0x1a0 (and owned by the
// blit-worker path). ONE class: the 2026-07-14 fold dissolved the former
// CDDrawBlitParam view (DDrawMgr/DDrawBlitParam.h - the dispatch/serialize API on
// the SAME 0x3c object: field-for-field overlay proven at +0x14 srcRef==playlist,
// +0x18 element==descriptor, +0x1c index, +0x30/+0x34 draw values, +0x38
// scale==speed float) and the former CDDrawBlitParamSrc source view (== the real
// CAniElement: its +0x0c "elements"/+0x10 "count" are CAniElement::m_records'
// m_pData/m_nSize, +0x20 the same float scale). ~CWwdGameObjectA/B destroy the
// +0x1a0 member by stamping ??_7CAniAdvanceCursor (0x5f0128) - the vtable proof.
//
// wave4-L: the satellite advance types (CAniRenderCtx/CAniDesc/CAniBlitTrigger)
// live in <DDrawMgr/AniAdvance.h>; the Advance body is the I obj, TriggerBlit the
// G obj (dossier #15). Field names are placeholders where the role is unproven.
#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <rva.h>

class CAniRenderCtx; // the sprite-render context (satellite def in AniAdvance.h)
class CAniDesc;      // the animation descriptor (playlist entry; == CAniRecord)
class CAniElement;   // the descriptor playlist (<Gruntz/AniElement.h>; the ex
                     // "CDDrawBlitParamSrc" resolved-source view)
// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

SIZE_UNKNOWN(CAniAdvanceCursor);
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
    virtual i32 Unload() OVERRIDE; // slot 7  0x15c2c0

    // --- the runtime/serialize method set (ex-CDDrawBlitParam; bodies in
    // WwdFactoryObject.cpp / DDrawSubMgr.cpp) ---
    // Construct's argument is role-dependent: the owning wide object on the
    // game-object path (Init 0x15b940 passes `this`), a worker source on the
    // blit path - reinterpreted internally, so it stays void* (one mangled
    // symbol across both).
    void Construct(void* src);                  // 0x15c290
    void Setup_15c2d0(CAniElement* src);        // 0x15c2d0  bind a resolved geo source
    void Recompute_15c320(i32 a1);              // 0x15c320  re-derive from the bound m_14
    i32 SelectCue_157a80(void* force);          // 0x157a80  (cue-role: writes m_2c/m_pendingDraw)
    i32 Serialize_15c970(CSerialArchive* ar);   // 0x15c970
    i32 Deserialize_15ca70(CSerialArchive* ar); // 0x15ca70
    i32 Find(CSerialArchive* ar, i32 type, i32 a3, i32 a4); // 0x15c900
    i32 Advance(u32 elapsed);                        // 0x15c360 (advance / set-geo-source)

    // (+0x0c is the inherited CLoadable owner slot m_0c: the game object /
    // blit worker that owns this cursor - the ex "m_worker".)
    CAniRenderCtx* m_10; // +0x10  sprite-render context / bound source object
    CAniElement* m_14;   // +0x14  descriptor playlist (the resolved geo source;
                         //        ex "m_srcRef" - the serialize map value)
    CAniDesc* m_element; // +0x18  current descriptor/element (transient)
    i32 m_index;         // +0x1c  playlist index
    u32 m_20;            // +0x20  per-frame timer remaining (ticks)
    i32 m_24;            // +0x24  "decrement-each-tick" flag
    i32 m_28;            // +0x28  paused/done flag
    i32 m_2c;            // +0x2c  owns-buffer flag (consume the draw value on
                         //        read); the cue role caches the cue ptr here
    i32 m_pendingDraw;   // +0x30  pending draw value (cue role: present flag)
    i32 m_curDraw;       // +0x34  current draw value
    float m_scale;       // +0x38  float speed/scale multiplier (Advance compares
                         //        it as raw bits vs 0x3f800000 == 1.0f)
};
VTBL(CAniAdvanceCursor, 0x001f0128); // ??_7CAniAdvanceCursor@@6B@ (9-slot CLoadable-derived)

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
