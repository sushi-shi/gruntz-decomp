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
    i32 m_04;   // +0x04  status word (-1 inactive)
    i32 m_08;   // +0x08
    void* m_0c; // +0x0c  parent CDDrawPtrCollections (its surface pool at +0x1c)
};

// CDDrawPtrCollections::RemoveItemA(CPoolItemA*) @0x142160 - reloc-masked
// __thiscall engine callee modeled as a method on a tiny view so the call falls
// out as `mov ecx,pool; call` with no caller-side stack cleanup.
class CImageSurfacePool {
public:
    void RemoveItemA(void* item); // 0x142160
};

// The owned +0x30 object: destroyed by a direct (non-virtual) call to the engine
// teardown at 0x148d10 then RezFree'd. Modeled as a tiny stand-in whose only
// load-bearing fact is the teardown method's RVA.
class CImageOwned {
public:
    void Teardown(); // 0x148d10
};

class CImage : public CImageBase {
public:
    void FreeAll();                                       // 0x153260  (vtable slot 7)
    ~CImage();                                            // 0x0d5e80
    void RenderFrame(void* a, void* b, void* c, void* d); // 0x153790

    // --- layout (continues the base; base ends at +0x10) ----------------------
    i32 m_10;                  // +0x10  width/handle (cleared by FreeAll)
    i32 m_14;                  // +0x14  height/handle (cleared by FreeAll)
    char m_pad18[0x2c - 0x18]; // +0x18..+0x2b
    void* m_2c;                // +0x2c  the held surface (CPoolItemA), pool-removed
    CImageOwned* m_30;         // +0x30  owned object (teardown + RezFree)
};

#endif // SRC_IMAGE_CIMAGE_H
