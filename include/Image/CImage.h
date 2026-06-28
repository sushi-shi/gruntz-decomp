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

class CString;  // real MFC CString (4-byte ptr); completed via <Mfc.h> in the .cpp
                // (forward-decl here so includers needn't choose <Mfc.h> vs <Win32.h>)
class CBlitInfo; // the sprite blit/draw request (esi); defined in CImageSpriteBlit.cpp

// The two vtables in the dtor chain: this class's own (0x5eaa2c) and the
// grand-base CObject dtor vtable (0x5e8cb4 = g_remusBaseDtorVtbl). Reloc-masked
// DATA externs (the manual stamps reference the RETAIL tables, whose contents are
// unmatched engine code).
class CImageParent; // +0x0c parent (CDDrawPtrCollections); defined below

// ---------------------------------------------------------------------------
// CImageBase - the polymorphic SeverusWorker/"Remus" base (same one as
// CSurfacePairBase). POLYMORPHIC with a REAL virtual destructor: its inline body
// resets the three base fields, and MSVC appends the implicit base-vptr re-stamp
// (the grand-base dtor vtable @0x5e8cb4) - so the dtor's TWO vptr stamps are both
// compiler-emitted and land in the retail "stamp-first" order (resolving the
// eh-dtor-vptr-stamp-vs-trylevel-order wall). ~CImageBase folds into the leaf
// ~CImage and supplies the /GX EH frame (docs/patterns/
// eh-dtor-multilevel-polymorphic-chain.md, inline-base-dtor-folds-into-leaves.md).
// Its emitted vtable (??_7CImageBase) reloc-masks against the target's 0x5e8cb4
// grand-base stamp (which the shared g_severusWorkerDtorVtbl names; the stamp
// bytes are identical, the masked operand differs only in symbol name).
// ---------------------------------------------------------------------------
class CImageBase {
public:
    // The base-subobject destructor is an EMPTY non-trivial inline dtor: MSVC emits
    // ONLY the implicit grand-base vptr re-stamp for it, which folds in as the LAST
    // store of ~CImage (matching retail's stamp-after-resets order). The empty body
    // is still non-trivial, so it supplies the leaf's /GX EH frame. The base-field
    // resets live in ~CImage's body (so they precede this fold's stamp).
    virtual ~CImageBase() {}
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();

    // vptr @+0x00 (implicit, polymorphic)
    i32 m_04;           // +0x04  status word (-1 inactive)
    i32 m_08;           // +0x08
    CImageParent* m_0c; // +0x0c  parent CDDrawPtrCollections (its surface pool at +0x1c)
};

// The source sub-object held at CImageSurfaceItem::m_08 (a polymorphic Remus
// parse-node). Reload (slot 13) probes it via two vtable slots: +0x60 reports
// whether the surface is still clean (skip rebuild), +0x6c returns whether it has
// a live source descriptor (re-run Resolve) vs none (parse the new source). Its
// vtable contents are external engine code; only the two slots are evidenced.
//
// The two slots are called `mov ecx,[obj]; push obj; call [ecx+N]` - the object is
// pushed as an explicit stack arg (the vtable fn ptr is __stdcall(this), not a PMF
// with this-in-ecx). Model the vtable entries as __stdcall fn-ptrs over the object.
struct CImageSurfaceSrc;
struct CImageSurfaceSrcVtbl {
    char _00[0x60 - 0x00];
    i32(__stdcall* IsClean)(CImageSurfaceSrc*); // [0x60]
    char _64[0x6c - 0x64];
    i32(__stdcall* HasSource)(CImageSurfaceSrc*); // [0x6c]
};
struct CImageSurfaceSrc {
    CImageSurfaceSrcVtbl* vptr; // +0x00
};

// The held +0x2c surface (a CDDrawPtrCollections CPoolItemA): LoadDispatch reads
// its geometry (+0x18 / +0x1c) and the +0xbc keyed flag; Reload (slot 13) reaches
// the parse-source at +0x08, Prepare (0x13e760) and Blt (0x13ee60) drive a clone.
// Only those offsets are load-bearing here; the rest of the 0xc0-byte item lives in
// CDDrawPtrCollections.
class CImageSurfaceItem {
public:
    char _00[0x08];
    CImageSurfaceSrc* m_08; // +0x08  parse-source sub-object (Reload path)
    char _0c[0x18 - 0x0c];
    i32 m_18; // +0x18  height
    i32 m_1c; // +0x1c  width
    char _20[0xbc - 0x20];
    i32 m_bc; // +0xbc  has-color-key flag

    i32 Prepare(i32 z);                // 0x13e760  (__thiscall, ret 4)
    i32 Blt(CImageSurfaceItem* other); // 0x13ee60  (__thiscall, ret 4)
    i32 Reload(void* pool, i32 src, i32 index, void* data, i32 flag); // 0x13e550 (__thiscall, ret 0x14)
};

// The parent CDDrawPtrCollections surface pool, reached through CImage::m_0c->m_1c.
// RemoveItemA (0x142160) frees a held surface; CreateA (0x142260) allocates one;
// CreateB (0x1423c0) is the Create24 variant. Reloc-masked __thiscall engine callees
// modeled on a tiny view so each lowers to `mov ecx,pool; call` with callee-side
// stack cleanup.
class CImageSurfacePool {
public:
    void RemoveItemA(void* item);                                          // 0x142160
    CImageSurfaceItem* CreateA(i32 desc, i32 mode, void* a, i32 b, i32 c); // 0x142260
    CImageSurfaceItem* CreateB(i32 desc, i32 mode, void* a, i32 b, i32 c); // 0x1423c0
    CImageSurfaceItem* CreateC(i32 desc, i32 cap, i32 flags);              // 0x142560
};

// The owned +0x30 object (a 0x3c-byte buffer holder built by BuildSlot13): a
// decoded-pixel buffer (+0x0c) and a 256-entry hardware palette (+0x20), plus the
// dimensions/format metadata copied out of the CImageFrameDesc. The ctor primes
// the defaults; Build (0x1490d0) decodes a frame from the desc into the two owned
// buffers; Teardown (0x148d10) RezFree's both. CImageBuildDesc (below) holds the
// desc fields Build reads. Reloc-masked __thiscall throughout.
class CImageBuildDesc;

// The 8-dword (0x20) by-value frame descriptor Rebuild builds on the stack and
// hands to DecodeFrame: [0]=0, [1]=flags, [2]=m_04 (width), [3]=m_08 (height),
// [4]/[5]=the two int args, [6]=the low byte of m_24 (key, 0 when m_24==-1),
// [7]=0. Only the layout is load-bearing (passed whole via rep movs).
struct CImageFrameRebuildDesc {
    i32 f0;
    i32 f1;
    i32 f2;
    i32 f3;
    i32 f4;
    i32 f5;
    i32 f6;
    i32 f7;
};

class CImageOwned {
public:
    CImageOwned();                                      // 0x148ce0
    i32 BuildRle(void* pixels, i32 width, i32 height, i32 stride, i32 keyVal,
                 void* palette);                        // 0x148d40  (/GX, CByteArray RLE encode)
    i32 LoadFromFile(CString name, i32 fmt);            // 0x148fc0  (/GX, open + slurp + Build)
    i32 Build(CImageBuildDesc* src, i32 size, i32 fmt); // 0x1490d0
    void* Remap(void* pixels);                          // 0x1495d0  (palette-remap, external)
    void Teardown();                                    // 0x148d10
    // 0x149250 (external/reloc-masked) and 0x1493b0 (Rebuild): when the owned
    // object is in format-state 1, build the descriptor and decode a frame.
    i32 DecodeFrame(CString name, CImageFrameRebuildDesc desc); // 0x149250
    i32 Rebuild(CString name, i32 a1, i32 a2);                  // 0x1493b0

    i32 m_00;   // +0x00
    i32 m_04;   // +0x04  src->m_08
    i32 m_08;   // +0x08  src->m_0c
    void* m_0c; // +0x0c  decoded pixel buffer (new / RezFree)
    i32 m_10;   // +0x10  pixel byte count
    i32 m_14;   // +0x14  (1)
    i32 m_18;   // +0x18  (0x80)
    i32 m_1c;   // +0x1c  (0)
    void* m_20; // +0x20  256-entry palette buffer (new 0x400 / RezFree)
    i32 m_24;   // +0x24  (-1 / src->m_18)
    u8 m_28;    // +0x28  format flag a
    u8 m_29;    // +0x29  format flag b
    char _2a[0x3c - 0x2a]; // +0x2a  pad to the 0x3c allocation size (operator new(0x3c))
};

// The CImageFrameDesc as seen by CImageOwned::Build: a flag word at +0x04 (bits
// 0x40/0x80/0x100/0x200 steer the decode), two ints copied to the owned object's
// +0x04/+0x08, a +0x18 byte, and the raw frame data starting at +0x20.
class CImageBuildDesc {
public:
    char _00[0x04];
    i32 m_04; // +0x04  decode flags
    i32 m_08; // +0x08  -> owned m_04
    i32 m_0c; // +0x0c  -> owned m_08
    char _10[0x18 - 0x10];
    u8 m_18; // +0x18  -> owned m_24 when 0x100 set
    char _19[0x20 - 0x19];
    u8 m_20[1]; // +0x20  raw frame data (palette + pixels)
};

// The display-mode descriptor reached through CImageParent::m_04 in BuildSlot13:
// m_04->m_10 is the active CDDrawSurface, whose +0x18 holds the format/pitch the
// owned object's Build needs. Reloc-masked; only the +0x10 / +0x18 offsets matter.
class CDDrawSurfaceDesc {
public:
    char _00[0x10];
    i32* m_10; // +0x10  active surface (its +0x18 is the format)
};

// A plain {left, top, right, bottom} rect (the sprite blitters copy the parent
// clip rect into a stack BlitRect before CopyRect'ing it into the working rect).
struct BlitRect {
    i32 m_00; // left
    i32 m_04; // top
    i32 m_08; // right
    i32 m_0c; // bottom
};

// The clip-region owner reached through CImageParent::m_24 by the sprite blitters:
// its +0x10 is the active clip RECT (copied to a local, then CopyRect'd before
// clamping).
class CBlitClipOwner {
public:
    char _00[0x10];
    BlitRect m_10; // +0x10  clip RECT {left, top, right, bottom}
};

// The +0x0c parent (CDDrawPtrCollections): its surface pool sits at +0x1c, and a
// display-mode descriptor at +0x04 (chased by BuildSlot13 for the build format).
// FreeAll and LoadDispatch reach the pool through it; the sprite blitters reach the
// clip-region owner at +0x24.
class CImageParent {
public:
    char _00[0x04];
    CDDrawSurfaceDesc* m_04; // +0x04  display-mode descriptor
    char _08[0x1c - 0x08];
    CImageSurfacePool* m_1c; // +0x1c  the surface pool
    char _20[0x24 - 0x20];
    CBlitClipOwner* m_24; // +0x24  clip-region owner (its +0x10 is the clip RECT)
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
extern i32 g_severusCounterB;

// ClassUnknown_36 (0x14dd90): a __stdcall(i32, i32) post-load notify the slot-13
// path fires with (2, 0). Reloc-masked external; no body.
void __stdcall ImageNotify(i32 a, i32 b); // 0x14dd90

class CImage : public CImageBase {
public:
    i32 Create(CImageFrameDesc* desc, i32 keyed);                      // 0x152e90  (vtable slot 12)
    i32 Resolve(CImageSource* src, i32 arg);                           // 0x152f20  (vtable slot 11)
    i32 LoadDispatch(CImageFrameDesc* desc, u32 mode, void* a, i32 b); // 0x152fb0  (vtable slot 10)
    i32 Create24(CImageFrameDesc* desc, i32 mode, i32 keyed); // 0x1530e0  (vtable slot 9)
    i32
    BuildSlot13(CImageFrameDesc* desc, void* a); // 0x153180  (non-virtual /GX builder, external)
    void FreeAll();                              // 0x153260  (vtable slot 7)
    i32 CopyFrom(CImage* other);                 // 0x1532b0  (clone the surface from another image)
    i32 Reload(CImageSource* src, i32 arg);      // 0x153380  (vtable slot 13)
    virtual ~CImage();                           // 0x0d5e80
    void RenderFrame(void* a, void* b, void* c, void* d);                    // 0x153790
    void RenderFrameClipped(void* a, void* b, void* c, void* d, void* rect); // 0x153810

    // The 5 sprite blit/clip routines (0x1538c0..0x1544d0): compute the on-screen
    // sprite rect (with optional X/Y flip in the anchor math + an optional m_3c
    // coordinate transform), clip it against either the parent clip RECT or the
    // worker clip box, then blit via CDDSurface::BltEx (surface variants) or
    // CDDrawShadeBlit::Blit (shaded variants). See src/Image/CImageSpriteBlit.cpp.
    void BlitNorm(CBlitInfo* info, CImage* dst);       // 0x1538c0  no flip, surface
    void BlitFlipV(CBlitInfo* info, CImage* dst);      // 0x153b20  Y flip, surface
    void BlitFlipH(CBlitInfo* info, CImage* dst);      // 0x153d90  X flip, surface
    void BlitShadeNorm(CBlitInfo* info, CImage* dst);  // 0x154270  no flip, shaded
    void BlitShadeFlipV(CBlitInfo* info, CImage* dst); // 0x1544d0  Y flip, shaded

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
