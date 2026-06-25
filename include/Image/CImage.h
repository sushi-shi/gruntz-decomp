#ifndef SRC_IMAGE_CIMAGE_H
#define SRC_IMAGE_CIMAGE_H

// CImage.h - the RTTI-confirmed polymorphic CImage (`.?AVCImage@@`, the ONLY
// CImage type-descriptor in the binary; primary vftable @0x5eaa2c). It is a
// surface-backed image element in the DDrawMgr "Remus" image family and a SIBLING
// of CDDrawSurfacePair (include/Gruntz/CDDrawSurfacePair.h): both derive from the
// same polymorphic SeverusWorker base (grand-base dtor vtable g_remusBaseDtorVtbl
// @0x5e8cb4) and share the +0x04/+0x08/+0x0c base header and the +0x0c parent /
// +0x2c held-surface (CPoolItemA) / +0x30 owned-object layout.
//
// NOTE: the existing non-polymorphic loader class in src/Image/Image.cpp is ALSO
// named CImage but is a DIFFERENT physical type (BITMAPINFOHEADER @+0, no vptr) -
// a misattribution by an earlier matcher (it has no RTTI/vtable). This class is
// the real RTTI CImage; the symbols here (??1CImage@@, ??_7CImage@@, the named
// members) are disjoint from the loader's (LoadBmp/DecodeBmpHeader/...), so the
// two TUs do not collide. See the matcher report.
//
// The own vftable @0x5eaa2c (slot fn RVAs):
//   slot  7 (@0x1c)  FreeAll       (0x153260)   - free held surface + owned obj
//   slot  9 (@0x24)  Create24      (0x1530e0)
//   slot 10 (@0x28)  LoadDispatch  (0x152fb0)
//   slot 11 (@0x2c)  Resolve       (0x152f20)
//   slot 12 (@0x30)  Create        (0x152e90)
//   slot 13 (@0x34)  ???           (0x153380)
//   slot 14 (@0x38)  RenderImage   (0x153470)   - dispatched by RenderFrame
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

#include <rva.h>
#include <Ints.h>

// The two vtables in the dtor chain: this class's own (0x5eaa2c) and the
// grand-base CObject dtor vtable (0x5e8cb4 = g_remusBaseDtorVtbl). Reloc-masked
// DATA externs (the manual stamps reference the RETAIL tables, whose contents are
// unmatched engine code).
class CImageParent; // +0x0c parent (CDDrawPtrCollections); defined below

DATA(0x001eaa2c)
extern void* g_imageVtbl; // 0x5eaa2c - CImage primary vftable
DATA(0x001e8cb4)
extern void* g_imageBaseDtorVtbl; // 0x5e8cb4 - the CObject grand-base dtor vtable

// ---------------------------------------------------------------------------
// CImageBase - the polymorphic SeverusWorker/"Remus" base (same one as
// CSurfacePairBase). POLYMORPHIC so the vptr sits at +0x00 and ~CImageBase is a
// real non-trivial base subobject dtor: its inline body folds into the leaf
// ~CImage and supplies the /GX EH frame (docs/patterns/
// inline-base-dtor-folds-into-leaves.md, eh-dtor-needs-base-subobject.md). Its
// vtable contents are external engine code, so the vptr is MANUAL-stamped (the
// declared virtuals only fix the +0 vptr slot; they are never called here).
// ---------------------------------------------------------------------------
class CImageBase {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();

    // The base-subobject destructor: resets the three base fields and restores the
    // grand-base vftable. INLINE so it folds into ~CImage exactly as retail's
    // base-dtor tail.
    ~CImageBase() {
        m_04 = -1;
        m_08 = 0;
        m_0c = 0;
        *(void**)this = &g_imageBaseDtorVtbl;
    }

    // vptr @+0x00 (implicit, polymorphic)
    i32 m_04;           // +0x04  status word (-1 inactive)
    i32 m_08;           // +0x08
    CImageParent* m_0c; // +0x0c  parent CDDrawPtrCollections (its surface pool at +0x1c)
};

// The held +0x2c surface (a CDDrawPtrCollections CPoolItemA): LoadDispatch reads
// its geometry (+0x18 / +0x1c) and the +0xbc keyed flag. Only those offsets are
// load-bearing here; the rest of the 0xc0-byte item lives in CDDrawPtrCollections.
class CImageSurfaceItem {
public:
    char _00[0x18];
    i32 m_18; // +0x18  height
    i32 m_1c; // +0x1c  width
    char _20[0xbc - 0x20];
    i32 m_bc; // +0xbc  has-color-key flag
};

// The parent CDDrawPtrCollections surface pool, reached through CImage::m_0c->m_1c.
// RemoveItemA (0x142160) frees a held surface; CreateA (0x142260) allocates one.
// Reloc-masked __thiscall engine callees modeled on a tiny view so each lowers to
// `mov ecx,pool; call` with callee-side stack cleanup.
class CImageSurfacePool {
public:
    void RemoveItemA(void* item);                                          // 0x142160
    CImageSurfaceItem* CreateA(i32 desc, i32 mode, void* a, i32 b, i32 c); // 0x142260
};

// The owned +0x30 object: destroyed by a direct (non-virtual) call to the engine
// teardown at 0x148d10 then RezFree'd. Modeled as a tiny stand-in whose only
// load-bearing fact is the teardown method's RVA.
class CImageOwned {
public:
    void Teardown(); // 0x148d10
};

// The +0x0c parent (CDDrawPtrCollections): its surface pool sits at +0x1c. Both
// FreeAll and LoadDispatch reach the pool through it.
class CImageParent {
public:
    char _00[0x1c];
    CImageSurfacePool* m_1c; // +0x1c  the surface pool
};

// The frame descriptor passed to LoadDispatch / through the Resolve dispatch: a
// flag word at +0x04 (bit 0x20 = "slot-13 path", bit 0x40 = "post-load notify")
// and the +0x10/+0x14 geometry copied into the image's m_20/m_24. Reloc-masked.
class CImageFrameDesc {
public:
    char _00[0x04];
    i32 m_04; // +0x04  flag word (0x20 / 0x40)
    char _08[0x10 - 0x08];
    i32 m_10; // +0x10
    i32 m_14; // +0x14
};

// The Remus parse-source the Resolve virtual drives: GetTag (0x139800) reads its
// 3-char format tag, Resolve (0x139960) primes it, Release (0x1399d0) tears it
// down; its +0x0c field feeds the LoadDispatch call. Reloc-masked __thiscall.
class CImageSource {
public:
    i32 GetTag();   // 0x139800
    i32 Resolve();  // 0x139960
    void Release(); // 0x1399d0

    char _00[0x0c];
    i32 m_0c; // +0x0c
};

// The two severus load counters (?g_severusCounterA/B@@3HA), gating the CreateA
// cap (0x800) and the flags arg. Reloc-masked C++ globals.
DATA(0x002bf37c)
extern i32 g_severusCounterA;
DATA(0x002bf380)
extern i32 g_severusCounterB;

// ClassUnknown_36 (0x14dd90): a __stdcall(i32, i32) post-load notify the slot-13
// path fires with (2, 0). Reloc-masked external; no body.
void __stdcall ImageNotify(i32 a, i32 b); // 0x14dd90

class CImage : public CImageBase {
public:
    i32 Resolve(CImageSource* src, i32 arg);                           // 0x152f20  (vtable slot 11)
    i32 LoadDispatch(CImageFrameDesc* desc, u32 mode, void* a, i32 b); // 0x152fb0  (vtable slot 10)
    i32
    BuildSlot13(CImageFrameDesc* desc, void* a); // 0x153180  (non-virtual /GX builder, external)
    void FreeAll();                              // 0x153260  (vtable slot 7)
    ~CImage();                                   // 0x0d5e80
    void RenderFrame(void* a, void* b, void* c, void* d);                    // 0x153790
    void RenderFrameClipped(void* a, void* b, void* c, void* d, void* rect); // 0x153810

    // --- layout (continues the base; base ends at +0x10) ----------------------
    i32 m_10;                // +0x10  width  (from item->m_1c)
    i32 m_14;                // +0x14  height (from item->m_18)
    i32 m_18;                // +0x18  width>>1
    i32 m_1c;                // +0x1c  height>>1
    i32 m_20;                // +0x20  desc->m_10 (or 0)
    i32 m_24;                // +0x24  desc->m_14 (or 0)
    i32 m_28;                // +0x28  load-result code (0x10 / 0x11)
    CImageSurfaceItem* m_2c; // +0x2c  the held surface (CPoolItemA), pool-removed
    CImageOwned* m_30;       // +0x30  owned object (teardown + RezFree)
};

#endif // SRC_IMAGE_CIMAGE_H
