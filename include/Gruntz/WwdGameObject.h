#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)
#include <Wap32/WapObj.h>            // CWapObj - the IsLoaded/IsReady (slots 5/6) intermediate base
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
    i32 m_0c;          // +0x0c  0, or this->Vfunc40() when GetTypeId()==0x1c
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
class CWwdGameObject : public CWapObj {
public:
    // Slots 0-4 come from CObject and 5/6 from CWapObj: slot 6 holds the 0x001c08
    // IsReady default thunk (the CWapObj fingerprint) and slot 5 the family's
    // IsLoaded override @0x15b370 (`return m_7c && m_0c && m_04 != -1` - the
    // worker-gate). Neither is redeclared. Slots 7-16 are the class's own
    // (slot RVAs = the 0x5f0020 table's ground truth):
    virtual void ReleaseSubs(); // slot 7  @0x15b5d0  ReleaseSubs_15b5d0
    // slot 8 - the per-kind type tag (`mov eax,<id>; ret`: E=0, C=6, F=0x16,
    // B=0x1b; the FindBy* probes compare ==5). Read by WriteSnapshot.
    virtual i32 GetTypeId(); // slot 8  @0x154a00
    // slot 9 - set position + reset the draw state (x->m_posX, y->m_posY, zero the
    // clip/plot fields, reseed m_48=0x32/m_50=1, cache mgr->m_24). The defined body
    // is Helper164790 below (same RVA; the direct-call spelling).
    virtual i32 SetPosition(i32 x, i32 y); // slot 9  @0x164790
    // slot 10 - the factories' 4-arg Build dispatch == this class's own Setup
    // (0x150d60; the body definition). Direct callers (Init/SetupDeferred/
    // SetupFlagged/ResetAndSetup) spell it CWwdGameObject::Setup (qualified =
    // devirtualized) so they keep retail's direct rel32.
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4); // slot 10 @0x150d60
    // slot 11 - the per-object render hook (1 arg = the render context; F's
    // override is `ret 4`, C's is RenderDot below; __purecall in this base table).
    virtual void Render(CDDrawSurfacePair* ctx); // slot 11 @0x11fec0  __purecall
    // slots 12-14 - the dirty-rect blit ops on the two render surface-pairs
    // (__purecall in this base table; the A/C/F kinds override - see the family).
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b);               // slot 12
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c);      // slot 13
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c); // slot 14
    virtual i32 Slot3C(i32 ar, i32 mode, i32 a3, void* self); // slot 15 @0x151150 == Play
                                                              // (the manager's walk dispatch)
    virtual i32
    Vfunc40(); // slot 16 @0x1bef01  const-getter (== inherited slot 0) read by WriteSnapshot

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
    // deserialized name lookups into are CGameObject::EnsureWorker80/88/90 (0x150eb0/
    // 0x150f90/0x151070) - the same wide object under the CGameObject view; called
    // through a CGameObject* cast, no fake local placeholders.

    // +0x00 is the CObject base vptr (the 17-slot table); m_04 at +0x04.
    i32 m_04;                // +0x04
    i32 m_flags;             // +0x08  bit flags (|=0x800000 / 0x1000000)
    CDDrawSurfaceMgr* m_mgr; // +0x0c  owning manager (the canonical CDDrawSurfaceMgr)
    i32 m_10;                // +0x10
    i32 m_14;                // +0x14
    i32 m_lastX;             // +0x18  last-drawn column (cached by RenderDot)
    i32 m_lastY;             // +0x1c  last-drawn row
    i32 m_20;                // +0x20
    i32 m_24;                // +0x24
    i32 m_28;                // +0x28
    i32 m_2c;                // +0x2c
    i32 m_30;                // +0x30  set 1 on a successful plot
    i32 m_34;                // +0x34  set 1 on a successful plot
    i32 m_clipResult;        // +0x38  clip result (0 plotted / -1 rejected)
    char m_pad3c[0x40 - 0x3c];
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    char m_pad4c[0x50 - 0x4c];
    i32 m_50;         // +0x50
    i32 m_54;         // +0x54
    i32 m_58;         // +0x58
    i32 m_posX;       // +0x5c  position column
    i32 m_posY;       // +0x60  position row
    i32 m_clipLeft;   // +0x64  clip rect (0x80000000 = unbounded)
    i32 m_clipTop;    // +0x68
    i32 m_clipRight;  // +0x6c
    i32 m_clipBottom; // +0x70
    i32 m_sortKey;    // +0x74  the manager's z-order sort key (Setup stores its a3;
                      //         CDDrawChildGroup::InsertSorted orders the list by it)
    i32 m_posCache;   // +0x78  CObList POSITION cache (CDDrawChildGroup::InsertSorted stores the
                      //         node; TickKillCues/RemoveAndDelete unlink through it)
    // +0x7c  the owned 0x17c worker/logic record - ONE class, ONE api
    // (the ex-CLogicRecord kill-cue view is folded onto AnimWorkerObj).
    AnimWorkerObj* m_worker;
    // The three lazily-built Hit/Attack/Bump handler workers (== CGameObject::m_80/
    // m_88/m_90 in UserLogic.h + WwdGameObjectFamily.h, both AnimWorkerObj*): each is
    // set up with the AnimWorkerObj field block + StampWorkerVtbl, then ->Init/->Clear'd.
    AnimWorkerObj* m_80;            // +0x80  Hit handler worker (serialized by name)
    i32 m_84;                       // +0x84
    AnimWorkerObj* m_88;            // +0x88  Attack handler worker
    i32 m_8c;                       // +0x8c
    AnimWorkerObj* m_90;            // +0x90  Bump handler worker
    i32 m_94;                       // +0x94
    CWwdGameObject* m_linkedObject; // +0x98  linked object (Play case 3 reads its +0x188 object id)
    // +0x9c  the embedded spatial-grid region node (<Gruntz/WwdGridIter.h>): its
    // m_x/m_y (+0xac/+0xb0) are the position copies Setup refreshes and its
    // m_object (+0xb4) the self back-pointer - the old m_ac/m_b0/m_self trio.
    // The factories' +0x9c record ctors (0x15b2a0/0x15b2b0) initialize exactly it.
    WwdGridNode m_region; // +0x9c..+0xb7
    char m_b8[0x24];      // +0xb8  serialized state block
    char* m_name;         // +0xdc  CString name (handle = buffer pointer)
    i32 m_e0;             // +0xe0
    i32 m_e4;             // +0xe4
    i32 m_e8;             // +0xe8
    i32 m_ec;             // +0xec
    i32 m_f0;             // +0xf0
    i32 m_f4;             // +0xf4
    i32 m_f8;             // +0xf8
    i32 m_fc;             // +0xfc
    i32 m_100;            // +0x100
    i32 m_104;            // +0x104
    i32 m_108;            // +0x108
    i32 m_10c;            // +0x10c
    i32 m_110;            // +0x110
    i32 m_114;            // +0x114
    i32 m_118;            // +0x118
    i32 m_11c;            // +0x11c
    i32 m_120;            // +0x120
    i32 m_124;            // +0x124
    i32 m_128;            // +0x128
    i32 m_12c;            // +0x12c
    i32 m_130;            // +0x130
    i32 m_134;            // +0x134  block head (0x80000000 sentinel)
    i32 m_138;            // +0x138
    i32 m_13c;            // +0x13c
    i32 m_140;            // +0x140
    i32 m_144;            // +0x144  block head (0x80000000 sentinel)
    i32 m_148;            // +0x148
    i32 m_14c;            // +0x14c
    i32 m_150;            // +0x150
    i32 m_154;            // +0x154  block head (0x80000000 sentinel)
    i32 m_158;            // +0x158
    i32 m_15c;            // +0x15c
    i32 m_160;            // +0x160
    i32 m_164;            // +0x164
    i32 m_168;            // +0x168
    i32 m_16c;            // +0x16c
    i32 m_170;            // +0x170
    i32 m_174;            // +0x174
    i32 m_178;            // +0x178
    i32 m_17c;            // +0x17c
    i32 m_180;            // +0x180
    i32 m_184;            // +0x184
    i32 m_188;            // +0x188  object id / the manager's CMapPtrToPtr key
                          //         (g_wwdObjIdCounter stamp; m_2c/m_48 maps)
    i32 m_dotColor;       // +0x18c  low byte = dot color / setup flag
    i32 m_190;            // +0x190  the cached frame NUMBER (index into m_194's frame array)
    // +0x194/+0x198 are the cached sprite and the cached FRAME it resolves to. Proven by
    // Sub150c30 (0x150c30), which is CGameObject::ApplyLookupSprite spelled through offset
    // macros: it bounds-checks the frame number against the looked-up object's +0x64/+0x68
    // (== CSprite::m_firstFrame / m_lastFrame) and then indexes its +0x14 (==
    // CSprite::m_frames.m_pData) to get +0x198 - exactly what ApplyLookupSprite assigns to
    // its CImage* m_layer at the SAME +0x198. ReadState confirms the sprite: it
    // strcpy's the name at m_194+0x24, and CSprite::m_name is at +0x24.
    CSprite* m_sprite;      // +0x194  cached sprite (was void*)
    CImage* m_layer;        // +0x198  cached frame (a CImage; ex CGameObjLayer view, was i32)
    // +0x19c is the resolved sound-cue value: ReadState hands it straight to
    // CDDrawSubMgrLeafScan::FindKeyOfValue_158570(LeafCue*), which is its type.
    LeafCue* m_19c;             // +0x19c  resolved leaf-scan cue (was void*)
    CAniAdvanceCursor m_cmdMap; // +0x1a0  anim/command cursor sub-object (real class)
    CObList m_subList;          // +0x1dc  MFC CObList of owned sub-objects
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_WWDGAMEOBJECT_H
