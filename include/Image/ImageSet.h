// ImageSet.h - CImageSet, the engine's sparse image-frame collection.
//
// A CImageSet owns an array of frame pointers addressed by a signed frame index
// that runs over the inclusive range [m_minIndex, m_maxIndex]; the set carries an
// inline name buffer and a CObArray-style backing store at +0x10. Only the three
// leaf accessors in ImageSet.cpp are matched here - the FUN_* labels carry no real
// name, so the class name (CImageSet) and field names are descriptive placeholders
// recovered from usage; the OFFSETS + code bytes are the load-bearing facts (object
// size 0x6c, vtable @0x5efbe8, frame array @0x14 / count @0x18 / inline name @0x24 /
// index range @0x64,0x68). The frame element is plausibly the RTTI CImage
// (`.?AVCImage@@`, vtable @0x5eaa2c) but its layout here (+0x30 = a format helper)
// does NOT line up with the real CImage (+0x30 = owned object; vptr @+0), so the
// frame/format types stay placeholders - see CImageFrame below.
#ifndef SRC_IMAGE_IMAGESET_H
#define SRC_IMAGE_IMAGESET_H

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
    char m_pad14[0x18 - 0x14];
    i32 m_18;             // +0x18  field written directly by SetAllField18 (0x1524d0)
    i32 m_resolvedFormat; // +0x1c  resolved format word (written directly by SetAllFormats)
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
    virtual void* GetRuntimeClass();                      // [0]  +0x00
    virtual void* ImgScalarDtor(i32 flags);               // [1]  +0x04  scalar-deleting dtor (ILT)
    virtual void* Serialize();                            // [2]  +0x08 (ILT)
    virtual void* AssertValid();                          // [3]  +0x0c (ILT)
    virtual void* Dump();                                 // [4]  +0x10 (ILT)
    virtual void* HasFrames();                            // [5]  +0x14 (ILT)
    virtual void* IsValidImage();                         // [6]  +0x18 (ILT)
    virtual void* FreeAll();                              // [7]  +0x1c  CImage::FreeAll
    virtual void* GetImageCategory();                     // [8]  +0x20 (ILT)
    virtual i32 Create24(i32 a, i32 b, i32 c);            // [9]  +0x24  CImage::Create24
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

class CImageSet {
public:
    // Walk every populated frame in [m_minIndex, m_maxIndex] and (re)set its pixel
    // format via the frame's +0x30 helper; returns the number of frames touched.
    i32 SetAllTypes(i32 type);

    // Same walk, but writes `format` straight into each frame's resolved format word
    // (+0x1c). A null format is a no-op (returns 0). Returns the count touched.
    i32 SetAllFormats(i32 format);

    // Same walk, writing `value` into each populated frame's format +0x18.
    i32 SetAllField18(i32 value); // 0x1524d0

    // Sum the decoded byte size of every populated frame (width*height scaled by the
    // held surface's bit depth, or the owned object's exact count when present). When
    // `raw` is 0 each frame also carries a fixed 0x34-byte overhead. 0x1523f0.
    i32 GetMemoryUsage(i32 raw); // 0x1523f0

    // Linear-search the frame array for `frame`; on a hit, copy the set's name into
    // `outName` (when non-null) and store the matched array index through `outIndex`
    // (when non-null). Returns 1 on a hit, 0 otherwise.
    i32 FindFrame(CImageFrame* frame, char* outName, i32* outIndex);

    // Three frame-creation overloads (0x151fb0/0x152060/0x152110). Each refuses if a
    // frame already occupies `index`, else allocates a CImageFrame, runs the frame's
    // loader virtual (slot 0x30/0x28/0x24), inserts it (SetAtGrow), and widens the
    // populated index range. Returns the new frame, or 0 on refusal/load failure.
    CImageFrame* CreateFrame30(i32 a0, i32 index, i32 a2);
    CImageFrame* CreateFrame28(i32 a0, i32 a1, i32 index, i32 a3);
    CImageFrame* CreateFrame24(i32 a0, i32 a1, i32 index, i32 a3);

    // The bounds-checked accessor SetAllTypes/SetAllFormats inline: a frame index
    // outside [m_minIndex, m_maxIndex] yields a null frame.
    CImageFrame* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }
        return m_frames[index];
    }

    char m_pad00[0x0c];
    void* m_owner;             // +0x0c  shared into each created frame's +0xc
    char m_array[0x14 - 0x10]; // +0x10  embedded frame array (SetAtGrow this)
    CImageFrame** m_frames;    // +0x14  frame pointer array (indexed by signed frame index)
    i32 m_count;               // +0x18  array element count (for the linear FindFrame scan)
    char m_pad1c[0x24 - 0x1c];
    char m_name[0x40 - 0x24]; // +0x24  inline name buffer (copied out by FindFrame)
    char m_pad40[0x64 - 0x40];
    i32 m_minIndex; // +0x64  lowest populated frame index
    i32 m_maxIndex; // +0x68  highest populated frame index
};

#endif // SRC_IMAGE_IMAGESET_H
