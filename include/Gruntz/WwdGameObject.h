#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)
#include <Gruntz/UserLogic.h>        // CGameObject - the BASE (all data + the 17-slot vtable)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - the real +0x1a0 anim/command cursor
#include <Gruntz/WwdGridIter.h>      // WwdRegion - the embedded +0x9c region node

// CWwdGameObject - a runtime "plane object" deserialized from WWD level data.
// WwdFile::ReadPlaneObjects (0x162af0) constructs one per record via the ctor
// at 0x15b390 (NOT reconstructed here - it lives in the eh-frame ctor TU). The
// object owns a sprite-animation worker at +0x7c (0x17c-byte, the same family
// as CDDrawWorkerCache's WwdAnimWorker, foreign vtable g_*Vtbl), a small
// command-dispatch sub-object at +0x1a0, and a back-pointer to its owning
// manager at +0x0c.
//
// Class identity is a role inference (no RTTI on the vtable @0x5f0020); only the
// this-OFFSETS and emitted code bytes are load-bearing (campaign doctrine), so
// field names are placeholders m_<hexoffset>.

// The owning manager reached through CWwdGameObject+0x0c IS the canonical
// CDDrawSurfaceMgr (<DDrawMgr/DDrawSurfaceMgr.h>): PROVEN by the ctor chain (the
// factories' m_0c is a CDDrawSurfaceMgr*) and by the members the methods reach -
// +0x04 m_drawTarget, +0x08 m_childGroup, +0x10 m_imageRegistry (sprite registry), +0x14
// m_workerCache, +0x24 m_level (CGameLevel), +0x28 m_soundRegistry - which are
// exactly CDDrawSurfaceMgr's fields. The ex WwdMgr / WwdCamHolder views are dissolved.
class CDDrawSurfaceMgr;

// The 0xa0-byte snapshot record CWwdGameObject::WriteSnapshot (0x151c00) assembles on
// the stack and emits through the archive. CWwdGameObject's snapshot serialization
// format (a real record type, not a per-TU view).
struct WwdSnapshot {
    i32 m_00;          // +0x00  m_04
    i32 m_04;          // +0x04  m_188 (object id)
    i32 m_08;          // +0x08  this->GetTypeId()
    i32 m_0c;          // +0x0c  0, or this->GetSnapshotSubId() when GetTypeId()==0x1c
    i32 m_10;          // +0x10  0, or worker->m_logic->GetTypeTag()
    char m_name[0x80]; // +0x14  name string from the mgr
    i32 m_94;          // +0x94  m_posX
    i32 m_98;          // +0x98  m_posY
    i32 m_9c;          // +0x9c  m_sortKey
};
SIZE(WwdSnapshot, 0xa0); // WriteSnapshot emits ar->Write(&rec, 0xa0)

// The cached sprite / frame / sound-cue value the object resolves by name (+0x194,
// +0x198, +0x19c). Real classes: <Gruntz/Sprite.h>, <Gruntz/UserLogic.h> and
// <DDrawMgr/DDrawSubMgrLeafScan.h>. Pointer members only -> forward decls suffice.
class CDDrawWorker; // CSprite IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>); the
typedef CDDrawWorker CSprite; // typedef repeats Sprite.h's - identical, so legal,
                              // and keeps this header pointer-only/include-light.
class CImage;  // the cached frame element (<Image/CImage.h>; ex CGameObjLayer view)
class CDDrawSurfacePair; // slots 12-14 params (<DDrawMgr/DDrawSurfacePair.h>)
struct LeafCue;          // the leaf-scan cache value (<Gruntz/LeafCue.h>; ex LeafScanValue)

// The +0x7c worker is the ONE canonical AnimWorkerObj (<DDrawMgr/AnimWorkerObj.h>,
// vtable 0x1efb80) - the 2026-07-13 worker unification dissolved the former
// WwdAnimWorker view (its "Advance at vtbl+0x10" was a WRONG dispatch shape:
// retail fires the +0x10 FN POINTER m_notify, `mov edx,[obj+0x7c]; push obj;
// call [edx+0x10]; add esp,4` in Play 0x151150 / WriteSnapshot 0x151c00) AND the
// CLogicRecord kill-cue view (Consume / Dispatch / the +0x24 refcount) onto it.
#include <DDrawMgr/AnimWorkerObj.h>

// The +0x1a0 command-dispatch sub-object is the real CAniAdvanceCursor
// (<Gruntz/AniAdvanceCursor.h>, 0x3c bytes; the ex-CDDrawBlitParam view is folded
// onto it): Construct 0x15c290 (1 arg = owner), Find 0x15c900 (4 args). The
// former per-TU `CmdMap` char-blob view is dissolved too - m_cmdMap.Find()/
// Construct() lower to `lea ecx,[this+0x1a0]; call` with no cast.

// (The +0x1dc member is the real MFC CObList, folded below - the former
// WwdSubList/WwdSubNode/WwdSubDel views are dissolved. ResetAndSetup walks it with
// the real CObList::GetHeadPosition/GetNext + `delete` on each MFC CObject payload.)

// The render context (RenderDot's / Render's arg) is the real CDDrawSurfacePair
// (fwd-declared above; its m_width/m_height/m_surface are the clip extent + dest surface
// the former WwdRenderCtx view described offset-for-offset).

// ---------------------------------------------------------------------------
// CWwdGameObject IS the family B kind (<Wwd/WwdGameObjectFamily.h>, 0x1fc,
// vtable 0x5f00e8, the +0x1dc child CObList): the flat "dispatch model" class
// that lived here is DISSOLVED - its method set is homed on the real family
// levels (E: Setup/Play/Serialize/WriteSnapshot/Sub*/EnsureWorker*/AddLogic*;
// A: Apply*/Test/ReadState + the slot-10/15 overrides; B: Setup @0x1665e0 +
// the child-list ops; C: Render/SetupFlagged; F: SetupDeferred).
// ---------------------------------------------------------------------------

// The retail out-of-bounds slot-16 dispatch (a shipped bug, kept for the bytes):
// WriteSnapshot fires `call [vptr+0x40]` when GetClassId()==0x1c (an A-kind
// object) - but A's table has 16 slots (0x00..0x3c), so the read lands on the
// ADJACENT ??_7CWwdGameObjectB[0] (tables pack 0x1f00a8..0x1f00e8..) =
// CObject::GetRuntimeClass @0x1bef01, whose return rides into the snapshot as
// the "sub id". No type in the real hierarchy can spell an out-of-table
// dispatch; this declared-only 17-slot facet (never constructed, no vtable
// emitted, ONE use site) reproduces the call shape. docs: the flat-merge fold.
struct WwdRetailSlot16Facet {
    virtual void S00();
    virtual void* Delete(i32 flag); // [1] the scalar-deleting dtor slot - ReadPlaneObjects
                                    //     calls it BARE (push 1; call [edx+4], no null guard;
                                    //     plain `delete` under MSVC5 emits the guard)
    virtual void S02();
    virtual void S03();
    virtual void S04();
    virtual void S05();
    virtual void S06();
    virtual void S07();
    virtual void S08();
    virtual void S09();
    virtual void S10();
    virtual void S11();
    virtual void S12();
    virtual void S13();
    virtual void S14();
    virtual void S15();
    virtual i32 GetSnapshotSubId(); // [16] +0x40 - lands on B[0] at runtime
};
SIZE_UNKNOWN(WwdRetailSlot16Facet); // dispatch facet (never constructed)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_WWDGAMEOBJECT_H
