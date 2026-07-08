// DDrawWorkerNode.h - the shared CDDrawWorkerList "worker" class hierarchy. A
// worker is a 0x7c-byte polymorphic frame-animation node the CDDrawWorkerList
// factory (CreateWorker24/28/2C/30) allocates, seeds, and dispatches. Two
// concrete subtypes appear: CDDrawWorkerA (vtable 0x1efea0, 12 slots, BYTE frame at
// +0x78) and CDDrawWorkerB (vtable 0x1efed0, 14 slots, int frame at +0x78). Both
// share CDDrawWorkerBase (slots 0..11 + the non-virtual reset/arm helper 0x164790
// and the +0x00..+0x77 field layout); CDDrawWorkerB adds two frame-source virtuals
// (Vfunc30 @0x1572b0, Vfunc34 @0x157280) plus the named-object frame fetch
// (0x166040).
//
// This header is the single owner of the hierarchy; previously the three classes
// were re-declared flat in CDDrawWorkers.cpp (with a (HelperHost*)this cross-cast
// onto a made-up "HelperHost" class that IS this base) and again in
// CDDrawWorkerList.cpp. Modeling the real base means CDDrawWorkerA/B::Vfunc* call
// the inherited Helper_164790 / own Helper_166040 directly - no cross-cast. (The
// BoundaryUpper2.cpp Reset() pair keeps its own volatile-tuned flat copy; that TU
// needs the redundant-store `volatile` shape, not this layout.)
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The +0x0c owner sub-manager (CDDrawWorkerCtx) is dereferenced only
// in HelperHost.cpp (Helper_164790/Helper_166040), so it is a forward-declared
// pointer here; likewise CDDrawFrameSource (Vfunc30's a3) is completed in
// CDDrawWorkers.cpp.
#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>

// +0x0c owner sub-manager (a CDDrawSubMgr-family node); its +0x24 int primes m_3c
// and its +0x10 named-object map backs Helper_166040. Completed in HelperHost.cpp.
struct CDDrawWorkerCtx;

// Vfunc30's frame-source arg (int array @+0x14 bounded by +0x64/+0x68). Completed
// in CDDrawWorkers.cpp where Vfunc30 reads its fields.
struct CDDrawFrameSource;

// The shared base (slots 0..11). Slots 0/2/3/4 are the WAP CObject-interface
// thunks (0x1bef01 / 0x0028ec / 0x00106e / 0x004034); slot 1 is the scalar-
// deleting destructor; slots 5..10 are engine virtuals; slot 11 is Vfunc2C, which
// both subtypes override. The FUN_<VA> slot names are the usual reloc-masked
// declared-only convention. Non-virtual Helper_164790 (0x164790) resets/arms the
// worker and is inherited by both subtypes.
class CDDrawWorkerBase {
public:
    virtual void GetRuntimeClass();              // [0]  0x1bef01
    virtual ~CDDrawWorkerBase();                 // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Serialize();                    // [2]  0x0028ec
    virtual void AssertValid();                  // [3]  0x00106e
    virtual void Dump();                         // [4]  0x004034
    virtual void Slot05_157200();                // [5]  0x157200 (B)
    virtual void IsValidImage();                 // [6]  0x001c08
    virtual void Slot07_157310();                // [7]  0x157310 (B)
    virtual void Slot08_157210();                // [8]  0x157210 (B)
    virtual void Slot09_157080();                // [9]  0x157080
    virtual void Slot10_1660b0();                // [10] 0x1660b0 (B)
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3); // [11] 0x1572f0 (B) / 0x157110 (A)

    // Non-virtual: reset/arm the worker from (a, b); seeds m_3c off the owner ctx.
    i32 Helper_164790(i32 a, i32 b); // 0x164790

    i32 m_04;               // +0x04
    i32 m_08;               // +0x08
    CDDrawWorkerCtx* m_ctx; // +0x0c  owner sub-manager
    i32 m_10;               // +0x10
    i32 m_14;               // +0x14
    char _pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    char _pad54[0x58 - 0x54];
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
    char _pad68[0x74 - 0x68];
    i32 m_74; // +0x74  state
};
SIZE(CDDrawWorkerBase, 0x78);
RELOC_VTBL(
    CDDrawWorkerBase,
    0x001efed0
); // reduced/derived view aliases CDDrawWorkerB (slot-RVA verified)

// BYTE-frame worker (12-slot vtable 0x1efea0). Overrides only Vfunc2C in source;
// its other retail overrides (slots 1/5/7/8/10) stay inherited (reloc-masked).
struct CDDrawWorkerA : public CObject {
    virtual ~CDDrawWorkerA() OVERRIDE; // slot 1 (was scalar-dtor -> compiler ??_G)
    virtual void Slot05_157200();      // [5]  0x157200 (B)
    virtual void IsValidImage();       // [6]  0x001c08
    virtual void Slot07_157310();      // [7]  0x157310 (B)
    virtual void Slot08_157210();      // [8]  0x157210 (B)
    virtual void Slot09_157080();      // [9]  0x157080
    virtual void Slot10_1660b0();      // [10] 0x1660b0 (B)
    // Non-virtual: reset/arm the worker from (a, b); seeds m_3c off the owner ctx.
    i32 Helper_164790(i32 a, i32 b); // 0x164790
    i32 m_04;                        // +0x04
    i32 m_08;                        // +0x08
    CDDrawWorkerCtx* m_ctx;          // +0x0c  owner sub-manager
    i32 m_10;                        // +0x10
    i32 m_14;                        // +0x14
    char _pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    char _pad54[0x58 - 0x54];
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
    char _pad68[0x74 - 0x68];
    i32 m_74; // +0x74  state
    CDDrawWorkerA() {}
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3); // 0x157110

    char m_78; // +0x78 (BYTE frame)
    char _pad79[0x7c - 0x79];
};
SIZE(CDDrawWorkerA, 0x7c);
VTBL(CDDrawWorkerA, 0x001efea0); // vtable_names -> code (RTTI game class)

// int-frame worker (14-slot vtable 0x1efed0): adds Vfunc30 (slot 12) / Vfunc34
// (slot 13) plus the non-virtual named-object frame fetch Helper_166040 (0x166040).
struct CDDrawWorkerB : public CObject {
    virtual ~CDDrawWorkerB() OVERRIDE; // slot 1 (was scalar-dtor -> compiler ??_G)
    virtual void Slot05_157200();      // [5]  0x157200 (B)
    virtual void IsValidImage();       // [6]  0x001c08
    virtual void Slot07_157310();      // [7]  0x157310 (B)
    virtual void Slot08_157210();      // [8]  0x157210 (B)
    virtual void Slot09_157080();      // [9]  0x157080
    virtual void Slot10_1660b0();      // [10] 0x1660b0 (B)
    // Non-virtual: reset/arm the worker from (a, b); seeds m_3c off the owner ctx.
    i32 Helper_164790(i32 a, i32 b); // 0x164790
    i32 m_04;                        // +0x04
    i32 m_08;                        // +0x08
    CDDrawWorkerCtx* m_ctx;          // +0x0c  owner sub-manager
    i32 m_10;                        // +0x10
    i32 m_14;                        // +0x14
    char _pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    char _pad54[0x58 - 0x54];
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
    char _pad68[0x74 - 0x68];
    i32 m_74; // +0x74  state
    CDDrawWorkerB() {}
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3);                         // [11] 0x1572f0
    virtual i32 Vfunc30(i32 a1, i32 a2, CDDrawFrameSource* src, i32 a4); // [12] 0x1572b0
    virtual i32 Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4);                 // [13] 0x157280

    // Non-virtual: look up a named object in the owner map, fetch element[idx] when
    // in range, cache at m_78, return whether it is non-null.
    i32 Helper_166040(i32 key, i32 idx); // 0x166040

    i32 m_78; // +0x78 (int frame)
};
SIZE(CDDrawWorkerB, 0x7c);

// ??_7CDDrawWorkerB (was g_ddrawWorkerBVtbl). Emitted by `new CDDrawWorkerB` in
// CDDrawWorkerList.cpp; the slot-11/12/13 relocs now name the real overrides.
VTBL(CDDrawWorkerB, 0x001efed0);

// --- vtable catalog ---

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
