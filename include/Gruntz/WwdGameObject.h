#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)
#include <Gruntz/UserLogic.h>        // CGameObject - the BASE (all data + the 17-slot vtable)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - the real +0x1a0 anim/command cursor
#include <Gruntz/WwdGridIter.h>      // WwdGridNode - the embedded +0x9c region node

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
// CWwdGameObject - the canonical runtime plane object (raw-offset access; only
// offsets are load-bearing). It DERIVES from the real MFC CObject (the WAP engine
// statically links MFC and uses CObject as its game-object grand-base): PROVEN by
// ??_7CObject@0x1e8cb4 whose slots 0/2/3/4 (0x1bef01 GetRuntimeClass / 0x0028ec
// Serialize / 0x00106e AssertValid / 0x00404034 Dump) are EXACTLY this object's
// manual-table slots 0/2/3/4, with slot 1 the dtor override (0x15b4c0). So slots
// 0-4 are inherited from CObject and only slots 5-16 are the derived's own (below).
// The table (?g_wwdGameObjectVtbl@@3PAXA, VA 0x5f0020, read from .rdata) holds
// NON-virtual method addresses the engine hand-placed (retail calls Play/Setup/
// Helper164790 DIRECTLY via rel32), so those stay plain methods; the 12 declared-only
// virtuals model only the table SHAPE so WriteSnapshot dispatches slot 8/16 through
// the real vptr with no cast. Slot RVAs are the binary's ground truth (never
// fabricated); unrecovered slots carry @rva. (Slot 1, the scalar-deleting dtor
// override @0x15b4c0, is realized by the /GX CWwdGameObjectE sibling in this TU.)
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CWwdGameObject); // the DISPATCH model; the concrete kinds carry the sizes
// The data (+0x04..+0x1db) and the 17-slot vtable live in the BASE CGameObject
// (<Gruntz/UserLogic.h>) - ONE wide object, formerly modeled twice (this class and
// the CGameObject "view" were the same 0x5f0020-vtable object). Slot semantics:
// 0-4 CObject's five, 5/6 the CWapObj IsLoaded (@0x15b370 worker-gate: `return
// m_7c && m_0c && m_04 != -1`) / IsReady (0x001c08 default) pair, 7 ReleaseSubs
// @0x15b5d0, 8 GetTypeId @0x154a00 (per-kind tag: E=0, C=6, F=0x16, B=0x1b),
// 9 SetPosition @0x164790 (body = Helper164790 below, the direct-call spelling),
// 10 Setup, 11 Render, 12-14 BltDirty*, 15 Slot3C == Play, 16 GetSnapshotSubId.
class CWwdGameObject : public CGameObject {
public:
    // slot 10 - the factories' 4-arg Build dispatch; the body definition
    // (0x150d60) is below. Direct callers (Init/SetupDeferred/SetupFlagged/
    // ResetAndSetup) spell it CWwdGameObject::Setup (qualified = devirtualized)
    // so they keep retail's direct rel32.
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4) OVERRIDE; // slot 10 @0x150d60

    // On-screen visibility cull (0x1509c0): bounds-check the object's sprite extent
    // (m_198) against the camera rect or the plane grid limits (via m_mgr).
    i32 Test();
    // Dispatch entry (0x150a70) and the methods it routes to.
    i32 Dispatch(i32 a1, i32 type, i32 a3, i32 a4);             // 0x150a70
    i32 ReadState(i32 src);                                     // 0x150b00
    i32 Play(i32 a1, i32 type, i32 a3, i32 a4);                 // 0x151150 (vtbl +0x3c)
    i32 Serialize(i32 ar);                                      // 0x151320
    i32 WriteSnapshot(i32 dst, i32 unused);                     // 0x151c00 (ret 8; 2nd arg unused)
    i32 Init(i32 a1, i32 a2, i32 a3, i32 a4);                   // 0x15b940
    i32 ResetAndSetup(i32 a1, i32 a2, i32 a3, i32 a4);          // 0x1665e0
    i32 SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag); // 0x15c1d0 (out-of-line)
    i32 SetupDeferred(i32 a3, i32 a4);                          // 0x15bc30 (out-of-line)
    void RenderDot(CDDrawSurfacePair* a);                       // 0x1660f0

    // Sibling helpers (modeled as same-class methods so ecx=this matches).
    i32 Helper164790(i32 a2, i32 a1); // 0x164790  __thiscall
    i32 Sub150c30(i32 a1);            // 0x150c30
    i32 Sub151780(i32 a1);            // 0x151780
    i32 Sub151b90(i32 a1);            // 0x151b90  cache linked object (m_98) from key m_184

    // The three "resolve object reference" setters Sub151780 dispatches the
    // deserialized name lookups into are the base CGameObject::EnsureWorker80/88/90
    // (0x150eb0/0x150f90/0x151070) - inherited, called directly.

    CObList m_subList; // +0x1dc  MFC CObList of owned sub-objects
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_WWDGAMEOBJECT_H
