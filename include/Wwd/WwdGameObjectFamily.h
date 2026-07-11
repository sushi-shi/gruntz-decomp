#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

// WwdGameObjectFamily.h - the /GX destructor family of CWwdGameObject and its
// factory variants (the CWwdGameObjectE "Mid" base + the A/C/F thin variants and
// the 4-level CResolveNode-derived B chain), hoisted from WwdGameObject.cpp so the
// wave4-L original-TU partition can define the family's methods in their retail
// objs: the 0x150xxx live methods (src/Wwd/WwdGameObject.cpp), the 0x15b2c0-0x15ccc8
// lifecycle obj (src/Wwd/WwdFactoryObject.cpp), and the 0x1660f0+ render block
// (src/Wwd/WwdGameObjectRender.cpp). See docs/exe-map/interval-dossiers.md #15.
//
// Modeled as a REAL local polymorphic hierarchy
// (docs/patterns/eh-dtor-multilevel-polymorphic-chain.md): a base CWwdGameObject
// "Mid" level (vtable 0x5f0020) owns the four polymorphic worker pointers, a CString
// name (+0xdc), and two RAII sentinel-handle members (EdgeA/EdgeB) whose call-free
// dtors clear the base fields; its grand-base CObject (vtable 0x5e8cb4) just
// re-stamps. The thin factory variants A/C/F derive from Mid (each with its own
// most-derived vtable) and re-run the worker pass before folding Mid. cl emits the
// per-level vptr re-stamps + /GX trylevel chain; the stamps reloc-mask against the
// retail engine vtables. Field names are placeholders; only offsets + code bytes
// are load-bearing.
//
// The teardown grand-base is CObject (the SAME class as the flat model's MFC
// CObject - one ??_7CObject@0x1e8cb4, VA 0x5e8cb4). The EH dtors intentionally use
// the CObject reconstruction rather than the real <Mfc.h> CObject because its
// INLINE empty dtor folds into each derived dtor's vptr re-stamp (call-free); the
// real MFC ~CObject is out-of-line in NAFXCW, so cl would emit a CALL and break the
// fold. (The non-dtor flat CWwdGameObject has no dtor to fold, so it uses real MFC
// CObject.)

#include <Ints.h>
#include <Mfc.h> // CString / CObList / POSITION / CObject
#include <rva.h>
#include <Wap32/Object.h>             // the shared WAP CObject grand-base
#include <DDrawMgr/DDrawBlitParam.h>  // CDDrawBlitParam (the +0x1a0 command sub-object)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawGroupChild/Node (B's broadcast list)

class CDDrawSurfacePair; // slot 12-14 params (full def in <DDrawMgr/DDrawSurfacePair.h>)

// An owned polymorphic worker. Its scalar-deleting destructor is vtable slot 1
// (`mov eax,[ecx]; push 1; call [eax+4]`); declared-only (foreign vtable).
class WwdWorker {
public:
    virtual void Slot00();
    virtual void DeleteThis(i32 flag); // +0x04
};

// The CString name at +0xdc (NAFXCW dtor 0x1b9cde, reloc-masked).
struct WwdName {
    // DtorImpl @0x1b9cde IS a CString teardown (~CString family); cast at the call.
    ~WwdName() {
        ((CString*)this)->~CString();
    }
    char* m_data;
};

// The embedded +0x1a0 command sub-object (B variant). Real polymorphic base
// (CObject, vtable 0x5e8cb4): cl auto-emits the grand-base vptr re-stamp at
// teardown. Mirrors WwdSubA.
struct WwdSub : public CObject {
    virtual ~WwdSub() OVERRIDE {
        ((CDDrawBlitParam*)this)->Reset_15c2c0();
    }
    i32 m_04; // 0x1a4
    i32 m_08; // 0x1a8
    i32 m_0c; // 0x1ac
};

// Manual scalar-delete of an owned worker pointer (the retail idiom).
#define WORKER_FREE(p)                                                                             \
    do {                                                                                           \
        if (p) {                                                                                   \
            (p)->DeleteThis(1);                                                                    \
            (p) = 0;                                                                               \
        }                                                                                          \
    } while (0)

// Two RAII sentinel-handle members of the Mid level: each is a small object whose
// (inline, call-free) destructor resets its fields to the "invalid" sentinel. They
// are destroyed in reverse declaration order after the CString member, giving
// retail's groupY tail (EdgeA: 5c,20,38 ; then EdgeB: 04,08,0c) and bumping the /GX
// trylevel (each is a fully-constructed top-level destructible subobject).
struct WwdEdgeB { // 0x04..0x0c
    ~WwdEdgeB();
    i32 a; // 0x04
    i32 b; // 0x08
    i32 c; // 0x0c
};
inline WwdEdgeB::~WwdEdgeB() {
    a = -1;
    b = 0;
    c = 0;
}
// WwdEdgeA doubles as the tail of the live dirty-rect record (0x18-block): its `a`
// (0x20) / `b` (0x38 == the record's flag) fields are the sentinel words the /GX dtor
// resets, and it also carries the record's size corner m_w/m_h (0x30/0x34) that the
// Slot34/38 blit-dispatch reads. Kept as one struct so the dtor RAII tail is unchanged;
// the extra size fields are matching-neutral (the dtor never touches them).
struct WwdEdgeA { // 0x20..0x5c
    ~WwdEdgeA();
    i32 a;     // 0x20  live dirty-rect left (RECT.left)
    i32 top24; // 0x24  live dirty-rect top  (RECT.top)
    char _p28[0x30 - 0x28];
    i32 m_w; // 0x30  live dirty-rect size x (the "second corner" the blit uses)
    i32 m_h; // 0x34  live dirty-rect size y
    i32 b;   // 0x38  == the record's flag word (-1 == disarmed)
    char _p2[0x5c - 0x3c];
    i32 c; // 0x5c
};
inline WwdEdgeA::~WwdEdgeA() {
    c = (i32)0x80000000;
    a = (i32)0x80000000;
    b = -1;
}

// A's embedded +0x1a0 command sub-object, modeled polymorphically: its own vtable
// 0x5f0128, a member-teardown helper (0x15c2c0), an EdgeB sentinel, then the
// wap-object base re-stamp folded in.
struct WwdSubA : public CObject {
    virtual ~WwdSubA() OVERRIDE;
    WwdEdgeB m_04; // +0x04 (0x1a4/0x1a8/0x1ac)
};
inline WwdSubA::~WwdSubA() {
    ((CDDrawBlitParam*)this)->Reset_15c2c0();
}

// ---------------------------------------------------------------------------
// 0x15b4f0 - the base ~CWwdGameObject ("Mid"): vtable 0x5f0020. Frees the four
// workers, clears m_c0/m_d8 + the EdgeA shadow (groupX), then the CString member
// dtor, then folds EdgeA, EdgeB and the wap-object grand base.
// ---------------------------------------------------------------------------
class CWwdGameObjectE : public CObject {
public:
    virtual ~CWwdGameObjectE() OVERRIDE; // 0x15b4f0 (slot 1 scalar-deleting dtor)
    // Derived game-object slots 5-15 (the shared CWwdGameObject interface; slot RVAs
    // are the 0x5f0020 table's ground truth). Declared-only so A/F/C inherit the full
    // 16-slot shape and cl emits the sized ??_7; reloc-masked, matching-neutral.
    virtual void Slot14_15b370(); // slot 5  @0x15b370
    virtual void IsValidImage();  // slot 6  @0x001c08 (shared base thunk)
    virtual void ReleaseSubs();   // slot 7  @0x15b5d0
    virtual i32 Vfunc20();        // slot 8  @0x154a00
    virtual i32 Slot24_164790();  // slot 9  @0x164790
    virtual i32 Setup28();        // slot 10 @0x150d60
    virtual void Slot2C();        // slot 11 @0x11fec0 (__purecall)
    // slots 12-14: dirty-rect blit ops on the two render surface-pairs (front/back).
    virtual void
    Slot30(CDDrawSurfacePair* a, CDDrawSurfacePair* b); // slot 12 @0x11fec0 (__purecall)
    virtual void
    Slot34(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c); // slot 13 @0x11fec0 (__purecall)
    virtual void
    Slot38(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c); // slot 14 @0x11fec0 (__purecall)
    virtual i32 Play3C();                                      // slot 15 @0x151150

    WwdEdgeB m_04; // 0x04
    char _p10[0x18 - 0x10];
    i32 m_18;      // 0x18  live position x (start of the 9-dword state block copied to m_b8)
    i32 m_1c;      // 0x1c  live position y
    WwdEdgeA m_20; // 0x20
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xb8 - 0x94];
    i32 m_b8; // 0xb8  shadow position x (the previous-frame copy of the 0x18 block)
    i32 m_bc; // 0xbc  shadow position y
    i32 m_c0; // 0xc0  shadow record's f2 word (dtor sentinel)
    char _pc4[0xd0 - 0xc4];
    i32 m_d0;     // 0xd0  shadow dirty-rect size x
    i32 m_d4;     // 0xd4  shadow dirty-rect size y
    i32 m_d8;     // 0xd8  shadow record's flag word (-1 == disarmed)
    WwdName m_dc; // 0xdc  CString name
};

// ---------------------------------------------------------------------------
// 0x15b790 - the complete destructor: a thin derived class (vtable 0x5f00a8) on top
// of Mid, adding the m_18c block + the embedded WwdSubA command object at +0x1a0.
// ---------------------------------------------------------------------------
class CWwdGameObjectA : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectA() OVERRIDE; // slot 1  0x15b790
    // Overrides of the CWwdGameObjectE slots this variant re-points (0x5f00a8 table).
    virtual void ReleaseSubs() OVERRIDE; // slot 7  @0x15b980
    virtual i32 Vfunc20() OVERRIDE;      // slot 8  @0x15b760
    virtual i32 Setup28() OVERRIDE;      // slot 10 @0x15b940 (Init)
    virtual void Slot2C() OVERRIDE;      // slot 11 @0x15ba20
    virtual void Slot30(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x150660
    virtual void Slot34(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1506b0
    virtual void Slot38(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE;                  // slot 14 @0x1508a0
    virtual i32 Play3C() OVERRIDE; // slot 15 @0x150a70 (Dispatch)

    char _pe0[0x18c - 0xe0];
    i32 m_18c; // 0x18c
    i32 m_190; // 0x190
    i32 m_194; // 0x194
    i32 m_198; // 0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSubA m_1a0; // 0x1a0
};

// ---------------------------------------------------------------------------
// 0x15bad0 - the 0x159440-final variant: thin derived class (vtable 0x5f0060) on top
// of Mid. Re-runs the worker pass + groupX, then folds Mid + wap-object base.
// ---------------------------------------------------------------------------
class CWwdGameObjectF : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectF() OVERRIDE; // slot 1  0x15bad0
    // Overrides of the CWwdGameObjectE slots this variant re-points (0x5f0060 table).
    virtual void Slot14_15b370() OVERRIDE;                                    // slot 5  @0x15ba40
    virtual void ReleaseSubs() OVERRIDE;                                      // slot 7  @0x15bc50
    virtual i32 Vfunc20() OVERRIDE;                                           // slot 8  @0x15ba60
    virtual void Slot2C() OVERRIDE;                                           // slot 11 @0x15ba70
    virtual void Slot30(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x15ba80
    virtual void Slot34(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x15ba90
    virtual void Slot38(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE;                  // slot 14 @0x15baa0
    virtual void SetupDeferredV(); // slot 16 @0x15bc30 (new)
};

// ---------------------------------------------------------------------------
// 0x15bd10 - the CResolveNode-derived variant (extra +0x1dc CObList, leading init call
// 0x166810, trailing base CResolveNode dtor 0x429b). REAL-POLYMORPHIC 4-level chain
// (the vtable @0x5f00e8 was g_wwd1598d0FinalVtbl / vtbl-50): the destructor's
// four manual vtable restamps become the cl-emitted per-level vptr stamps of
//   CWwdGameObjectB (0x5f00e8) : WwdBLevel2 (0x5f00a8) : WwdBMid (0x5f0020)
//                             : WwdBResolve (0x5efbc0, virtual dtor -> DtorBase 0x429b)
// so cl auto-generates the multi-phase restamp + /GX trylevel chain; each stamp
// reloc-masks against the retail engine vtable. Each level owns a contiguous field
// range + one destructible member (CString@+0xdc / WwdSub@+0x1a0 / CObList@+0x1dc);
// the derived-level dtor bodies re-clear inherited base fields exactly like retail's
// per-phase re-clears (eh-dtor-multilevel-polymorphic-chain.md).
// ---------------------------------------------------------------------------

// Grand-base (vtable 0x5efbc0): a CResolveNode-style base with a virtual dtor (making
// the whole chain polymorphic). Restamps its vftable then tail-calls the base
// CResolveNode teardown (0x429b). Owns the +0x04..+0x5c fields; folded LAST.
struct WwdBResolve : public CObject { // CObject slots 0/2/3/4 inherited; dtor=slot1
    virtual ~WwdBResolve() OVERRIDE;  // slot 1
    void DtorBase();                  // 0x429b
    i32 m_04;                         // +0x04
    i32 m_08;                         // +0x08
    i32 m_0c;                         // +0x0c
    char _p10[0x20 - 0x10];
    i32 m_20; // +0x20
    char _p24[0x38 - 0x24];
    i32 m_38; // +0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c;                   // +0x5c
    char _p60[0x7c - 0x60];     // pad so WwdBMid's m_7c lands at +0x7c
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
};
inline WwdBResolve::~WwdBResolve() {
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    DtorBase();
}

// Mid level (vtable 0x5f0020): frees the four workers, clears m_c0/m_d8 + the
// inherited edge fields, then its CString member folds, then ~WwdBResolve.
struct WwdBMid : public WwdBResolve {
    virtual ~WwdBMid() OVERRIDE;
    WwdWorker* m_7c; // +0x7c
    WwdWorker* m_80; // +0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // +0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // +0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0; // +0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;                // +0xd8
    WwdName m_dc;            // +0xdc  CString
    char _pe0[0x18c - 0xe0]; // pad so WwdBLevel2's m_18c lands at +0x18c
};
inline WwdBMid::~WwdBMid() {
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    // m_dc (CString) auto-destroyed, then ~WwdBResolve folds.
}

// Level-2 (vtable 0x5f00a8): clears the m_18c block + runs SubB (@0x15b5d0), then its
// embedded WwdSub command object folds, then ~WwdBMid.
struct WwdBLevel2 : public WwdBMid {
    virtual ~WwdBLevel2() OVERRIDE;
    void SubB(); // 0x15b5d0
    i32 m_18c;   // +0x18c
    i32 m_190;   // +0x190
    i32 m_194;   // +0x194
    i32 m_198;   // +0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSub m_1a0;              // +0x1a0
    char _p1b0[0x1dc - 0x1b0]; // pad so CWwdGameObjectB's m_1dc lands at +0x1dc
};
inline WwdBLevel2::~WwdBLevel2() {
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    SubB(); // 0x15b5d0 (ReleaseSubs); same-class call, no cast-to-view of this
    // m_1a0 (WwdSub) auto-destroyed, then ~WwdBMid folds.
}

// Most-derived (vtable 0x5f00e8): leading InitDtor, the worker + field pass, then
// its CObList member folds (DtorList), then ~WwdBLevel2.
class CWwdGameObjectB : public WwdBLevel2 {
public:
    virtual ~CWwdGameObjectB() OVERRIDE; // 0x15bd10
    // Own vtable @0x5f00e8: overrides WwdBResolve's slots 5/7/8 + adds 10..15 (binary RVAs).
    virtual void VtSlotFill0() OVERRIDE;                // slot 5  0x15bcd0
    virtual void VtSlotFill2() OVERRIDE;                // slot 7  0x15bf00
    virtual void VtSlotFill3() OVERRIDE;                // slot 8  0x15bce0
    virtual void Slot10_1665e0();                       // slot 10 0x1665e0
    virtual void Slot11_1668b0(i32 a1);                 // slot 11 0x1668b0 (broadcast Slot2C)
    virtual void Slot12_1668e0(i32 a1, i32 a2);         // slot 12 0x1668e0 (broadcast Slot30)
    virtual void Slot13_166910(i32 a1, i32 a2, i32 a3); // slot 13 0x166910 (broadcast Vfunc34)
    virtual void Slot14_166950(i32 a1, i32 a2, i32 a3); // slot 14 0x166950 (broadcast Vfunc38)
    virtual void Slot15_150a70();                       // slot 15 0x150a70
    void Clear_166810();                                // 0x166810 (destroy m_1dc list + RemoveAll)
    i32 AddChild_1667e0(CDDrawGroupChild* child);       // 0x1667e0
    i32 RemoveChild_166850(CDDrawGroupChild* child);    // 0x166850
    i32 WalkChildWorkers_166880();                      // 0x166880 (per-child worker cb + count)
    CObList m_1dc; // +0x1dc  real MFC CObList (0x1c bytes; head @ +0x1e0 = m_pNodeHead;
                   // AddTail/RemoveAt = 0x1b5af6/0x1b5c2c; member dtor = ~CObList 0x1b5a2b)
    i32 m_1f8;     // +0x1f8
};

// ---------------------------------------------------------------------------
// 0x15c070 - the 0x159250-final variant: thin derived class (vtable 0x5effd0) on top
// of Mid; clears the byte flag m_18c, re-runs the worker pass + groupX, then folds
// Mid + wap-object base.
// ---------------------------------------------------------------------------
class CWwdGameObjectC : public CWwdGameObjectE {
public:
    virtual ~CWwdGameObjectC() OVERRIDE; // slot 1  0x15c070
    // Overrides of the CWwdGameObjectE slots this variant re-points (0x5effd0 table).
    virtual void Slot14_15b370() OVERRIDE; // slot 5  @0x15c000
    virtual void ReleaseSubs() OVERRIDE;   // slot 7  @0x15c200
    virtual i32 Vfunc20() OVERRIDE;        // slot 8  @0x15c020
    virtual void Slot2C() OVERRIDE;        // slot 11 @0x1660f0 (RenderDot)
    virtual void Slot30(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x1661d0
    virtual void Slot34(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1662a0
    virtual void Slot38(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x1664a0
    // Slots 16-18 unique to the C variant (0x5effd0 is a 19-slot table).
    virtual i32 SetupFlagged16(); // slot 16 @0x15c1d0
    virtual void Slot44();        // slot 17 @0x15c030
    virtual void Slot48();        // slot 18 @0x15c040

    char _pe0[0x18c - 0xe0];
    u8 m_18c; // 0x18c (byte flag)
};

// Exact retail object sizes from the CWwdObjMgrFactories RezAlloc(0xNN) calls:
// A=0x166640 (0x1dc), B=0x1598d0 (0x1fc), C=0x159250 (0x190), F=0x159440 (0x18c).
// E (Mid) is the shared base subobject, not directly allocated -> size unresolved.
SIZE(CWwdGameObjectA, 0x1dc);
SIZE(CWwdGameObjectB, 0x1fc);
SIZE(CWwdGameObjectC, 0x190);
SIZE_UNKNOWN(CWwdGameObjectE);
SIZE(CWwdGameObjectF, 0x18c);
// Per-variant game-object vtables (slot RVAs = each table's ground truth).
VTBL(CWwdGameObjectE, 0x001f0020); // ??_7 (Mid base, 16 slots)
VTBL(CWwdGameObjectA, 0x001f00a8); // ??_7 (Level2, 16 slots)
VTBL(CWwdGameObjectF, 0x001f0060); // ??_7 (17 slots)
VTBL(CWwdGameObjectC, 0x001effd0); // ??_7 (19 slots)
VTBL(CWwdGameObjectB, 0x001f00e8); // ??_7 (16 slots, 4-level MI chain)
SIZE_UNKNOWN(WwdEdgeA);
SIZE_UNKNOWN(WwdEdgeB);
SIZE_UNKNOWN(WwdName);
SIZE_UNKNOWN(WwdSub);
SIZE_UNKNOWN(WwdSubA);
SIZE_UNKNOWN(WwdWorker);
SIZE_UNKNOWN(WwdBResolve);
SIZE_UNKNOWN(WwdBMid);
SIZE_UNKNOWN(WwdBLevel2);
VTBL(WwdBResolve, 0x001efbc0);

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
