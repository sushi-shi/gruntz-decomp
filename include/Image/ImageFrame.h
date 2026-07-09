// ImageFrame.h - the frame element + its format helper owned by a CImageSet
// (split out of Image/ImageSet.h so the frame/format types can be shared with the
// consumers that reach a CImageSet's frames through the OTHER CImageSet view -
// notably GameLevel.h's collision/tile CImageSet, which the two definitions are a
// known conflation of: same 0x152480 SetAllTypes, divergent field slices).
#ifndef SRC_IMAGE_IMAGEFRAME_H
#define SRC_IMAGE_IMAGEFRAME_H
#include <rva.h>
#include <Ints.h>

// The frame's +0x30 sub-object: a small format/state helper. SetAllTypes calls its
// SetType(type, 0) (external no-body @0x14dd90, reloc-masked - the SAME rel32 target
// SBI_WarlordHead models as WhShowItem, so the callee's true name is unresolved);
// SetAllFormats writes the resolved format word at +0x1c directly. PLACEHOLDER type:
// only SetType + m_resolvedFormat are evidenced for the resolved format word.
class CImageFormat {
public:
    // (SetType @0x14dd90 IS ShadeSelector::Select; the field-view stays for m_resolvedFormat
    //  etc., the call casts to ShadeSelector in ImageSet.cpp.)

    char m_pad00[0x10];
    i32 m_decodedByteCount; // +0x10  decoded byte count (read by GetMemoryUsage)
    i32 m_14;               // +0x14  shade/type state (the minor-cheat toggles read it)
    i32 m_18;               // +0x18  field written directly by SetAllField18 (0x1524d0)
    i32 m_resolvedFormat;   // +0x1c  resolved format word (written directly by SetAllFormats)
};

// The frame's held surface (m_surface): a pool surface whose +0xa8 holds the bit depth
// (0x10 = 16bpp, 0x18 = 24bpp), used by GetMemoryUsage to scale the pixel count.
struct CImageFrameSurface {
    char m_pad00[0xa8];
    i32 m_bitDepth; // +0xa8  bit depth
};

// The frame as seen by CImageSet: only the +0x30 format sub-object is used. The
// frame may be the RTTI CImage but the +0x30 here is a format helper, not CImage's
// +0x30 owned object - so this is a PLACEHOLDER element type (see header note).
class CImageFrame {
public:
    inline CImageFrame(void* owner, i32 index);

    // The frame element IS an RTTI CImage: its vtable is the SHARED ??_7CImage@@6B@
    // at RVA 0x1eaa2c (VA 0x5eaa2c, 18 slots / 0x48; cataloged in
    // config/vtable_names.csv). Real-polymorphic: cl auto-stamps the CImageFrame vptr
    // at ctor entry; the declared-only slots reloc-mask. Slots are named by their
    // retail vtable-slot RVA read from the 0x1eaa2c .rdata (FUN_<rva>); the low ones
    // (0x1000-0x7c20) are ILT jmp-thunks into the CObject/MFC base. NO VTBL() here -
    // the vtable is the shared ??_7CImage (already at 0x1eaa2c); a per-class VTBL
    // would collide/misname the datum. The manual image-frame vptr stamp is removed
    // per the all-vtables mandate (cl auto-stamps the implicit vptr at ctor entry).
    virtual void* GetRuntimeClass();           // [0]  +0x00
    virtual ~CImageFrame();                    // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void* Serialize();                 // [2]  +0x08 (ILT)
    virtual void* AssertValid();               // [3]  +0x0c (ILT)
    virtual void* Dump();                      // [4]  +0x10 (ILT)
    virtual void* HasFrames();                 // [5]  +0x14 (ILT)
    virtual void* IsValidImage();              // [6]  +0x18 (ILT)
    virtual void* FreeAll();                   // [7]  +0x1c  CImage::FreeAll
    virtual void* GetImageCategory();          // [8]  +0x20 (ILT)
    virtual i32 Create24(i32 a, i32 b, i32 c); // [9]  +0x24  CImage::Create24
    virtual i32 LoadDispatch(i32 a, i32 b, i32 c, i32 d); // [10] +0x28  CImage::LoadDispatch
    virtual void* Resolve();                              // [11] +0x2c  CImage::Resolve
    virtual i32 Create(i32 a, i32 b);                     // [12] +0x30  CImage::Create

    i32 m_index;   // +0x04  frame index
    i32 m_8;       // +0x08
    void* m_owner; // +0x0c  owner (the CImageSet's +0xc)
    i32 m_width;   // +0x10  width  (GetMemoryUsage: pixel-count factor)
    i32 m_height;  // +0x14  height (GetMemoryUsage: pixel-count factor)
    char m_pad18[0x2c - 0x18];
    CImageFrameSurface* m_surface; // +0x2c  held surface (its +0xa8 = bit depth)
    CImageFormat* m_format;        // +0x30  format/state helper (factory inits to null)
};

#endif // SRC_IMAGE_IMAGEFRAME_H
