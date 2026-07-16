#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

// WwdGameObjectFamily.h - the wide game-object family: ONE real hierarchy, one
// class per retail ??_7 (the 2026-07-14 megafold; the former parallel WwdB*
// chain + WwdEdgeA/WwdEdgeB/WwdName/WwdSub/WwdSubA scaffolding are DISSOLVED):
//
//   CLoadable : CWapObj : CObject                 (Loadable.h)
//   CResolveNode : CLoadable        ??_7 @0x1efbc0  (ResolveNode.h; ex WwdBResolve dup)
//   CWwdGameObjectE : CResolveNode  ??_7 @0x1f0020  (the shared base; ex WwdBMid dup)
//   CWwdGameObjectA : E             ??_7 @0x1f00a8  (0x1dc; ex WwdBLevel2 dup)
//   CWwdGameObjectB : A             ??_7 @0x1f00e8  (0x1fc; slot 15 inherited from A
//                                                    proves B : A in the retail table)
//   CWwdGameObjectF : E             ??_7 @0x1f0060  (0x18c)
//   CWwdGameObjectC : E             ??_7 @0x1effd0  (0x190)
//
// Retail slot maps (raw table dumps, 2026-07-13): every slot 5..18 body below is
// the table's ground truth; slots 0-4 come from CObject, slot 6 (IsReady 0x1c08)
// from CWapObj, slot 8 (GetClassId 0x154a00 default) from CLoadable, slot 9
// (SetPosition 0x164790) from CResolveNode - all INHERITED where not overridden,
// never redeclared.
//
// The teardown chain is REAL inheritance: each kind's dtor is `{ Unload(); }`
// (devirtualized in the dtor to its own override) + the member/base folds -
// ~CString (0x1b9cde) on m_dc, ~CAniAdvanceCursor inline on A's m_1a0 (the
// 0x5f0128 stamp in retail ~A/~B), ~CObList on B's m_1dc, inline ~CResolveNode
// (the 0x5efbc0 stamp retail keeps alive in ~B before `call 0x429b`, DSE'd in
// the fully-inline ~E/~A/~F/~C tails), inline ~CLoadable, and the single CObject
// grand-base restamp. The per-kind Unload bodies (0x15b5d0/0x15b980/0x15bf00/
// 0x15bc50/0x15c200) are INLINE here so the same-TU dtors fold their content
// exactly as retail does (their out-of-line copies emit where the vtables do).
//
// Method-set TUs (wave4-L dossier #15): the 0x150xxx live methods
// (src/Wwd/WwdGameObject.cpp), the 0x15b2c0-0x15ccc8 lifecycle obj
// (src/Wwd/WwdFactoryObject.cpp), the 0x1660f0+ render block
// (src/Wwd/WwdGameObjectRender.cpp). Field names are placeholders; only offsets
// + code bytes are load-bearing.

#include <Ints.h>
#include <Mfc.h> // CString (+0xdc) / CObList (+0x1dc)
#include <rva.h>
#include <Gruntz/ResolveNode.h>      // CResolveNode - the REAL base (+0x00..+0x67)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - A's +0x1a0 member
#include <DDrawMgr/AnimWorkerObj.h>  // AnimWorkerObj - the owned +0x7c..+0x90 workers

class
    CDDrawSurfacePair; // slots 11-14 params (render ctx + blit pairs; <DDrawMgr/DDrawSurfacePair.h>)
                       // (slot 11's ctx WAS the WwdRenderCtx view - offset-exact m_width/
                       //  m_height/m_surface, now the real class)
class CWwdGameObject;  // the flat dispatch model (CWwdGameObjectB factory pair return type)

// Manual scalar-delete of an owned worker pointer (the retail idiom).
#define WORKER_FREE(p)                                                                             \
    do {                                                                                           \
        if (p) {                                                                                   \
            delete (p);                                                                            \
            (p) = 0;                                                                               \
        }                                                                                          \
    } while (0)

// ---------------------------------------------------------------------------
// CWwdGameObjectE - the shared wide-object base (vtable 0x5f0020, 16 slots).
// Owns +0x68..+0x18b: the four polymorphic workers, the +0x9c region node + the
// +0xb8 shadow dirty-rect block (as raw ranges here; their factory-ctor record
// views live below), the CString name (+0xdc), and the serialized state block.
// Abstract (Render/BltDirty* are pure -> retail's __purecall @0x11fec0 slots).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CWwdGameObjectE); // base subobject; the concrete kinds carry the sizes
class CWwdGameObjectE : public CResolveNode {
public:
    virtual ~CWwdGameObjectE() OVERRIDE; // 0x15b4f0 (out-of-line, WwdFactoryObject.cpp;
                                         // body = { Unload(); } + member/base folds)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  @0x15b370 (m_7c && m_0c && m_04 != -1)
    // slot 7 - release the four workers + disarm the live/shadow dirty-rect
    // sentinels. INLINE so the family dtors + sibling Unloads fold the content
    // (retail ~A/~F/~C inline it; deep contexts spill to `call 0x15b5d0`).
    // The out-of-line copy (the vtable slot) lands at 0x15b5d0.
    RVA(0x0015b5d0, 0x7c)
    virtual i32 Unload() OVERRIDE {
        WORKER_FREE(m_7c);
        WORKER_FREE(m_80);
        WORKER_FREE(m_88);
        WORKER_FREE(m_90);
        m_c0 = static_cast<i32>(0x80000000);
        m_d8 = -1;
        m_5c = static_cast<i32>(0x80000000);
        m_20 = static_cast<i32>(0x80000000);
        m_38 = -1;
        // retail leaves eax = the INT_MIN it just materialized for the stores
        return static_cast<i32>(0x80000000);
    }
    // slots 8 (GetClassId @0x154a00) and 9 (SetPosition @0x164790) INHERITED.
    // slot 10 - the factories' 4-arg build dispatch (the flat model's Setup
    // @0x150d60 is the body; renamed onto the family in the flat-merge stage).
    virtual i32 Setup28(i32 a1, i32 a2, i32 a3, i32 a4); // slot 10 @0x150d60
    // slots 11-14 - per-object render + dirty-rect blit hooks: PURE in this base
    // (retail table holds __purecall @0x11fec0); every concrete kind overrides.
    virtual void Render(CDDrawSurfacePair* ctx) = 0;                                     // slot 11
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) = 0;               // slot 12
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) = 0;      // slot 13
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) = 0; // slot 14
    // slot 15 - the 4-arg play/serialize dispatch (the flat model's Play
    // @0x151150 is the body).
    virtual i32 Play3C(i32 ar, i32 mode, i32 a3, void* self); // slot 15 @0x151150

    // 0x15b650: per-tick notify - under flag bit 0x8 decrement the +0x128 budget
    // (latch the worker's error state on underflow); else fire the +0x80
    // notifier's m_notify with the owner. (Ex CWwdFactoryObject::Notify_15b650.)
    void Notify_15b650(void* p); // 0x15b650

    i32 m_clipTop;       // +0x68  clip rect tail (m_64 = clipLeft on CResolveNode)
    i32 m_clipRight;     // +0x6c
    i32 m_clipBottom;    // +0x70
    i32 m_sortKey;       // +0x74  the manager z-order sort key (Setup28 stores a3;
                         //         CDDrawChildGroup::InsertSorted orders the list by it)
    i32 m_posCache;      // +0x78  CObList POSITION cache (InsertSorted stores the node;
                         //         TickKillCues/RemoveAndDelete unlink through it)
    AnimWorkerObj* m_7c; // +0x7c  the owned worker/logic record
    AnimWorkerObj* m_80; // +0x80  lazily-built Hit handler worker
    char _p84[0x88 - 0x84];
    AnimWorkerObj* m_88; // +0x88  lazily-built Attack handler worker
    char _p8c[0x90 - 0x8c];
    AnimWorkerObj* m_90;    // +0x90  lazily-built Bump/collide handler worker
    char _p94[0xb8 - 0x94]; // +0x94..+0xb7 (the +0x9c region node - see CWwdSlot9c)
    i32 m_b8;               // +0xb8  shadow dirty-rect x (prev-frame copy of +0x18)
    i32 m_bc;               // +0xbc  shadow dirty-rect y
    i32 m_c0;               // +0xc0  shadow dirty-rect corner (INT_MIN sentinel)
    char _pc4[0xd0 - 0xc4];
    i32 m_d0;                // +0xd0  shadow dirty-rect size x
    i32 m_d4;                // +0xd4  shadow dirty-rect size y
    i32 m_d8;                // +0xd8  shadow dirty-rect armed flag (-1 == disarmed)
    CString m_dc;            // +0xdc  the object's name (dtor 0x1b9cde folds in ~E)
    char _pe0[0x18c - 0xe0]; // +0xe0..+0x18b serialized state block (named on the
                             //  flat model until the flat merge)
};

// ---------------------------------------------------------------------------
// CWwdGameObjectA - the 0x1dc "created sprite" kind (vtable 0x5f00a8; the
// CDDrawChildGroup/CreateSprite instance class). Adds the +0x18c geometry cache +
// the embedded +0x1a0 CAniAdvanceCursor.
// ---------------------------------------------------------------------------
class CWwdGameObjectA : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectA() OVERRIDE; // slot 1  0x15b790 (out-of-line, I obj)
    // slot 7 override: null the geometry cache then the base release pass.
    // INLINE so ~A folds it (retail ~A = { Unload(); } + folds); the out-of-line
    // copy (the vtable slot) lands at 0x15b980.
    RVA(0x0015b980, 0x96)
    virtual i32 Unload() OVERRIDE {
        m_18c = -1;
        m_190 = -1;
        m_198 = 0;
        m_194 = 0;
        // the E pass (0x15b5d0 content); its INT_MIN residue is the return
        return CWwdGameObjectE::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;                            // slot 8  @0x15b760 (0x1c)
    virtual i32 Setup28(i32 a1, i32 a2, i32 a3, i32 a4) OVERRIDE; // slot 10 @0x15b940 (Init)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;         // slot 11 @0x15ba20 (ret 4)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x150660
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1506b0
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x1508a0
    virtual i32 Play3C(i32 ar, i32 mode, i32 a3, void* self)
        OVERRIDE; // slot 15 @0x150a70 (Dispatch)

    i32 m_18c; // +0x18c  (WwdFile stamp -1; the C kind reads its low byte as dot color)
    i32 m_190; // +0x190  cached frame number
    i32 m_194; // +0x194  cached sprite (typed CSprite* on the flat model)
    i32 m_198; // +0x198  cached frame/layer (typed CGameObjLayer* on the flat model)
    i32 m_19c; // +0x19c  resolved anim/cue slot (CreateObject_166640 zero-inits it)
    CAniAdvanceCursor m_1a0; // +0x1a0..+0x1db  the anim/command cursor (its
                             //  ~CAniAdvanceCursor folds inline in ~A/~B - the
                             //  retail 0x5f0128 member restamp)
};

// ---------------------------------------------------------------------------
// CWwdGameObjectB - the 0x1fc broadcast-group kind (vtable 0x5f00e8) ON TOP OF A
// (retail slot 15 = 0x150a70 = A's Play3C, inherited). Adds the child CObList.
// ---------------------------------------------------------------------------
class CWwdGameObjectB : public CWwdGameObjectA {
public:
    virtual ~CWwdGameObjectB() OVERRIDE; // 0x15bd10 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  0x15bcd0 (`return m_7c != 0`)
    // slot 7 override: destroy the child list, then the A/E release pass.
    // INLINE so ~B folds it; the out-of-line copy lands at 0x15bf00.
    RVA(0x0015bf00, 0xa1)
    virtual i32 Unload() OVERRIDE {
        Clear_166810(); // 0x166810 destroy the m_1dc children + RemoveAll
        m_1f8 = 0;
        m_18c = -1;
        m_190 = -1;
        m_198 = 0;
        m_194 = 0;
        // the E pass (deep contexts spill to `call 0x15b5d0`); INT_MIN residue return
        return CWwdGameObjectE::Unload();
    }
    virtual i32 GetClassId() OVERRIDE; // slot 8  0x15bce0 (0x1b)
    // slot 10/11-14 overrides (bodies in WwdGameObjectRender.cpp).
    virtual i32 Setup28(i32 a1, i32 a2, i32 a3, i32 a4) OVERRIDE; // slot 10 0x1665e0
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;         // slot 11 0x1668b0 (broadcast)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 0x1668e0
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 0x166910
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 0x166950
    // slot 15 INHERITED from A (retail table: 0x150a70).

    void Clear_166810();                            // 0x166810 (destroy m_1dc list + RemoveAll)
    i32 AddChild_1667e0(CWwdGameObjectE* child);    // 0x1667e0
    i32 RemoveChild_166850(CWwdGameObjectE* child); // 0x166850
    i32 WalkChildWorkers_166880();                  // 0x166880 (per-child worker cb + count)
    // The child-object factory pair (bodies in WwdGameObjectRender.cpp; the ex-CWwdObjMgrL
    // view is dissolved): build a child CWwdGameObjectA and publish it into m_1dc.
    CWwdGameObject* CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6); // 0x166640
    CWwdGameObject*
    CreateNamed_166780(int a1, int a2, int a3, int a4, const char* name, int a6); // 0x166780

    CObList m_1dc; // +0x1dc  real MFC CObList (0x1c bytes; head @ +0x1e0 = m_pNodeHead;
                   // AddTail/RemoveAt = 0x1b5af6/0x1b5c2c; member dtor = ~CObList 0x1b5a2b)
    i32 m_1f8;     // +0x1f8
};

// ---------------------------------------------------------------------------
// CWwdGameObjectF - the 0x18c deferred kind (vtable 0x5f0060, 17 slots): E plus
// the 2-arg build at new slot 16. Adds no data.
// ---------------------------------------------------------------------------
class CWwdGameObjectF : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectF() OVERRIDE; // slot 1  0x15bad0 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  @0x15ba40 (own copy of E's)
    // slot 7 override: the E release pass (a full inline copy in retail).
    // INLINE so ~F folds it; the out-of-line copy lands at 0x15bc50.
    RVA(0x0015bc50, 0x7c)
    virtual i32 Unload() OVERRIDE {
        return CWwdGameObjectE::Unload(); // a full inline copy of the E pass in retail
    }
    virtual i32 GetClassId() OVERRIDE;                    // slot 8  @0x15ba60 (0x16)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE; // slot 11 @0x15ba70 (ret 4 - empty)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x15ba80
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x15ba90
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x15baa0
    // slot 16 (new) - the F kind's 2-arg build (the 0x159440 factory's `call
    // [eax+0x40]` pushes two args; body 0x15bc30 == the flat SetupDeferred(a3, a4)).
    virtual i32 SetupDeferredV(i32 a3, i32 a4); // slot 16 @0x15bc30 (new)
};

// ---------------------------------------------------------------------------
// CWwdGameObjectC - the 0x190 dot-marker kind (vtable 0x5effd0, 19 slots): E plus
// the 5-arg build + the dot-color byte accessors at new slots 16-18.
// ---------------------------------------------------------------------------
class CWwdGameObjectC : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectC() OVERRIDE; // slot 1  0x15c070 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  @0x15c000 (own copy of E's)
    // slot 7 override: clear the dot-color byte then the E release pass.
    // INLINE so ~C folds it; the out-of-line copy lands at 0x15c200.
    RVA(0x0015c200, 0x82)
    virtual i32 Unload() OVERRIDE {
        m_dotColor = 0;
        return CWwdGameObjectE::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;                    // slot 8  @0x15c020 (6)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE; // slot 11 @0x1660f0 (RenderDot)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x1661d0
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1662a0
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x1664a0
    // Slots 16-18 unique to the C variant (0x5effd0 is a 19-slot table).
    // slot 16 - the C kind's 5-arg build (the 0x159250 factory's `call [eax+0x40]`
    // pushes five args; body 0x15c1d0 == the flat SetupFlagged(a1..a4, flag)).
    virtual i32 SetupFlagged16(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag); // slot 16 @0x15c1d0
    virtual u8 GetDotColor();        // slot 17 @0x15c030 (`mov al,[this+0x18c]`)
    virtual void SetDotColor(u8 c8); // slot 18 @0x15c040 (byte store to +0x18c)

    u8 m_dotColor; // +0x18c (byte dot color / setup flag)
    char _p18d[0x190 - 0x18d];
};

// ---------------------------------------------------------------------------
// The embedded sub-object records the CDDrawChildGroup factories placement-construct
// (ctor bodies at 0x15b270/0x15b2a0/0x15b2b0 in WwdObjMgr.cpp). CWwdShadowRec is the
// E-level SHADOW dirty-rect block at +0xb8: its m_8 (abs +0xc0) seeds the INT_MIN
// sentinel the family dtors re-clear as m_c0, and its m_20 (abs +0xd8) the -1
// disarm flag (m_d8). The +0x9c pair (CWwdSlot9c for the C/F kinds, CWwdSlot9cA
// for the A kind) is the sibling record at +0x9c.. (== the WwdGridNode region).
// Kept as placement-ctor record views of E's member ranges: folding them into E
// as real members requires inline member-ctor modeling of the 0x15b390 ctor
// (deferred with that ctor's CWwdGameObjBaseCtor view).
// ---------------------------------------------------------------------------
class CWwdSlot9c {
public:
    char m_pad00[0x08]; // +0x00..0x07
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c
    char m_pad10[0x18 - 0x10];
    i32 m_18;     // +0x18  -> obj+0xb4
    CWwdSlot9c(); // 0x15b2a0
};
SIZE_UNKNOWN(CWwdSlot9c);
class CWwdShadowRec { // the E-level shadow dirty-rect block (+0xb8)
public:
    CWwdShadowRec(); // 0x15b270
    char m_pad0[0x8];
    i32 m_8; // abs +0xc0 == CWwdGameObjectE::m_c0 (INT_MIN sentinel)
    char m_pad0c[0x20 - 0xc];
    i32 m_20; // abs +0xd8 == CWwdGameObjectE::m_d8 (-1 == disarmed)
};
SIZE_UNKNOWN(CWwdShadowRec);
class CWwdSlot9cA { // the A kind's +0x9c sibling record
public:
    CWwdSlot9cA(); // 0x15b2b0
    char m_pad0[0x8];
    i32 m_8;
    i32 m_c;
    char m_pad10[0x18 - 0x10];
    i32 m_18;
};
SIZE_UNKNOWN(CWwdSlot9cA);

// Exact retail object sizes from the CWwdObjMgrFactories RezAlloc(0xNN) calls:
// A=0x166640 (0x1dc), B=0x1598d0 (0x1fc), C=0x159250 (0x190), F=0x159440 (0x18c).
// E is the shared base subobject, not directly allocated -> size unresolved.
SIZE(CWwdGameObjectA, 0x1dc);
SIZE(CWwdGameObjectB, 0x1fc);
SIZE(CWwdGameObjectC, 0x190);
SIZE(CWwdGameObjectF, 0x18c);
// Per-variant game-object vtables (slot RVAs = each table's ground truth).
VTBL(CWwdGameObjectE, 0x001f0020); // ??_7 (base, 16 slots)
VTBL(CWwdGameObjectA, 0x001f00a8); // ??_7 (16 slots)
VTBL(CWwdGameObjectF, 0x001f0060); // ??_7 (17 slots)
VTBL(CWwdGameObjectC, 0x001effd0); // ??_7 (19 slots)
VTBL(CWwdGameObjectB, 0x001f00e8); // ??_7 (16 slots; B : A)

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
