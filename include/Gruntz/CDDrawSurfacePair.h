#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>
#include <Ints.h>

// ---------------------------------------------------------------------------
// CDDrawSurfacePair - a surface-backed drawing region in the DDrawMgr "Remus"
// image family. It derives from the engine SeverusWorker base (the same
// polymorphic base CGameLevel derives from: its grand-base dtor vtable is
// g_severusWorkerDtorVtbl == g_remusBaseDtorVtbl @0x5e8cb4). Its own vtable is
// @0x5eff30. It owns ONE held DDraw surface (a CPoolItemA, the CDDSurface
// wrapper) borrowed from the parent CDirectDrawMgr's surface pool, plus a cached
// pixel geometry (width @+0x10 / height @+0x14 / bpp @+0x18) and an x/y offset
// window @+0x1c.
//
// It is the polymorphic surface element held at the worker-manager's
// +0x10/+0x14/+0x18 slots (see src/Gruntz/CDDrawSubMgr.cpp's CDDrawSurfacePair
// placeholder). The own vtable @0x5eff30:
//   slot 1  (@0x04)  scalar-deleting dtor (0x1590d0)
//   slot 5  (@0x14)  IsValid              (0x159090) - the "surface ready?" pred
//   slot 7  (@0x1c)  TeardownSurface      (0x163e20) - remove from pool, zero m_2c/m_10
//   slot 9  (@0x24)  SetGeometry          (0x158fd0) - {w,h,bpp} cache
//   slot 11 (@0x2c)  InitFromSurface      (0x163db0)
//   slot 12 (@0x30)  Create               (0x163c90)
//   slot 13 (@0x34)  LoadImage            (0x163e50) - BMP/PCX/DIR/DIP magic dispatch
//   slot 14 (@0x38)  ResolveImage         (0x163ee0)
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

#include <Gruntz/CDirectDrawMgr.h> // CDDSurface (the held surface @+0x2c)

// The two vtables in the dtor chain: this class's own (0x5eff30) and the
// grand-base dtor vtable (0x5e8cb4). Reloc-masked DATA externs (the manual
// stamps reference the RETAIL tables, whose contents are unmatched engine code).
DATA(0x001eff30)
extern void* g_surfacePairVtbl;
DATA(0x001e8cb4)
extern void* g_surfacePairBaseDtorVtbl; // == g_severusWorkerDtorVtbl / g_remusBaseDtorVtbl

// ---------------------------------------------------------------------------
// CSurfacePairBase - the polymorphic SeverusWorker base. POLYMORPHIC so the vptr
// sits at +0x00 and ~CSurfacePairBase is a real non-trivial base subobject dtor
// (its inline body folds into the leaf ~CDDrawSurfacePair, supplying the /GX EH
// frame; see docs/patterns/inline-base-dtor-folds-into-leaves.md and
// eh-dtor-needs-base-subobject.md). Its vtable contents are external engine code,
// so the vptr is MANUAL-stamped (not compiler-emitted) - the declared virtuals
// only fix the vptr slot at +0, they are never called from this TU.
// ---------------------------------------------------------------------------
class CSurfacePairBase {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual i32 IsValid(); // slot 5 (@0x14) - the "surface ready?" predicate

    // The base-subobject destructor: resets the three base fields and restores the
    // grand-base vftable. INLINE so it folds into ~CDDrawSurfacePair, exactly as
    // the retail compiler emitted the base-dtor tail.
    ~CSurfacePairBase() {
        m_08 = 0;
        m_0c = 0;
        m_04 = -1;
        *(void**)this = &g_surfacePairBaseDtorVtbl;
    }

    // vptr @+0x00 (implicit, polymorphic)
    i32 m_04;   // +0x04  status word (-1 inactive, 0x63 active)
    i32 m_08;   // +0x08
    void* m_0c; // +0x0c  parent manager (its surface pool at +0x1c)
};

// CDDrawPtrCollections::RemoveItemA(CPoolItemA*) @0x142160 - reloc-masked
// __thiscall engine callee modeled as a method on a tiny view so the call falls
// out as mov ecx,pool; call with no caller-side stack cleanup. AcquireA/MakeAndAddB/
// CreateB are the three pool surface-acquire entries Create() fans into.
class CDDrawSurfacePool {
public:
    void RemoveItemA(CDDSurface* item);                        // 0x142160
    CDDSurface* AcquireA(i32 a, i32 b);                        // 0x143630
    CDDSurface* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
    CDDSurface* CreateB(i32 a, i32 b, i32 c, i32 d, i32 e);     // 0x1423c0
    i32 CreateModeSurface(i32 fmt, i32 fs, i32 w, i32 h, i32 bpp, i32 mode); // 0x141dc0
    CDDSurface* AttachMode(i32 mode);                                        // 0x142b70
    char _pad0[0x944];
    i32 m_944; // +0x944  last DirectDraw error stash
};

// CDDrawSurfaceMgr - the parent manager view m_0c points at: a pixel-format chain
// at +0x4 -> +0x10 -> +0x2c, the surface pool at +0x1c, and a last-error word at
// +0x38. Only the offsets Create() reads are pinned.
struct CDDrawSurfChainB {
    char _pad0[0x2c];
    i32 m_2c; // +0x2c  pixel-format token
};
struct CDDrawSurfChainA {
    char _pad0[0x10];
    CDDrawSurfChainB* m_10; // +0x10
};
struct CDDrawSurfaceMgr {
    char _pad0[0x04];
    CDDrawSurfChainA* m_4; // +0x04  pixel-format chain
    char _pad8[0x1c - 0x08];
    CDDrawSurfacePool* m_1c; // +0x1c  surface pool
    char _pad20[0x30 - 0x20];
    i32 m_30; // +0x30  device/context handle
    i32 m_34; // +0x34  caps flags (bit4 = fullscreen, bit1 = double-buffer)
    i32 m_38; // +0x38  last-error word
};

class CDDrawSurfacePair : public CSurfacePairBase {
public:
    void BltSelf(CDDrawSurfacePair* src);     // 0x03a1d0
    ~CDDrawSurfacePair();                      // 0x1590f0
    i32 Create(i32 w, i32 h, i32 bpp, i32 a3); // 0x163c90  (vtable slot 12)
    i32 RestoreIfLost();                       // 0x163f00
    void TeardownSurface();                    // 0x163e20  (vtable slot 7)
    void DrawBox(i32* rect, i32 color);        // 0x163f40
    void DrawCross(i32 x, i32 y);              // 0x164180
    // 0x1644a0 - the DirectDraw mode-surface creator: cache {w,h,bpp}, ask the pool
    // to create the device surface (mode 0x11 if w>320 else 0x51; fullscreen bit
    // from mgr->m_34), then attach + validate it; on any failure stash a 0x80e9..ed
    // / 0xbb9 / 0xbba error in mgr->m_38.
    i32 directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA(i32 w, i32 h, i32 bpp);
    i32 Probe_164660();                        // 0x164660  (surface-lost probe)

    // --- layout (continues the base; base ends at +0x10) ----------------------
    i32 m_10;                   // +0x10  width
    i32 m_14;                   // +0x14  height
    i32 m_18;                   // +0x18  bits-per-pixel (8/16/24/32)
    char m_rect1c[0x2c - 0x1c]; // +0x1c  x/y offset window {x,y,w,h} (src RECT)
    CDDSurface* m_2c;           // +0x2c  the held surface (CPoolItemA)
    i32 m_30;                   // +0x30  "owns surface" flag (free on teardown)
};

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
