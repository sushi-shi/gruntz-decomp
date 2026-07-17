// ImageSet.h - CImageSet, the engine's sparse image-frame collection.
//
// A CImageSet owns an array of frame pointers addressed by a signed frame index
// that runs over the inclusive range [m_minIndex, m_maxIndex]; the set carries an
// inline name buffer and a CObArray-style backing store at +0x10. Only the three
// leaf accessors in ImageSet.cpp are matched here - the FUN_* labels carry no real
// name, so the class name (CImageSet) and field names are descriptive placeholders
// recovered from usage; the OFFSETS + code bytes are the load-bearing facts (object
// size 0x6c, vtable @0x5efbe8, frame array @0x14 / count @0x18 / inline name @0x24 /
// index range @0x64,0x68). NOTE: that "vtable @0x5efbe8" is CDDrawWorker's own
// ??_7 (RVA 0x1efbe8) - this class IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
// see the fold-state note in <Gruntz/Sprite.h>.
//
// The frame element IS the RTTI CImage (`.?AVCImage@@`, vtable @0x5eaa2c). The old
// "+0x30 does NOT line up" wall (which kept a CImageFrame/CImageFormat/
// CImageFrameSurface placeholder trio in a now-deleted Image/ImageFrame.h) is
// FALSIFIED: the frame's "+0x30 format helper" IS CImage's +0x30 owned object, the
// CDDrawShadeBlit shaded sprite -
//   format +0x10 "decoded byte count"  == CDDrawShadeBlit::m_rleLen  (RLE byte count;
//        GetMemoryUsage's own comment already called it "the owned object's count")
//   format +0x14 "shade/type state"    == m_drawType (ctor default 1; the minor-cheat
//        toggles set it to 2/3 via SetAllTypes -> the row-convert selector)
//   format +0x18 "SetAllField18 field" == m_light   (ctor default 0x80; CheatEclipse
//        writes rand() % 256 into it - a random LIGHT LEVEL, 0..255)
//   format +0x1c "resolved format"     == m_palDescr (a ShadeDescr*; every SetAllFormats
//        caller casts a shade-table POINTER into the i32 param)
//   and the helper's SetType @0x14dd90 IS the same __thiscall body the blitter calls
//        (it writes [ecx+0x14]=mode and [ecx+0x1c]=descr - m_drawType/m_palDescr).
// Likewise the frame's held +0x2c surface is the real CDDSurface (its +0xa8 bit depth
// was the whole of the CImageFrameSurface placeholder).
#ifndef SRC_IMAGE_IMAGESET_H
#define SRC_IMAGE_IMAGESET_H
#include <rva.h>

#include <Ints.h>

#include <Image/CImage.h> // the frame element IS the RTTI CImage (was the CImageFrame view)

// Object size 0x6c (recorded in the header note above; the body computes exactly that:
// m_maxIndex at +0x68). The SIZE line used to sit on the GameLevel.h class of the same
// name - that class is CTileImageSet now, so it lives here, on the class it describes.
SIZE(CImageSet, 0x6c);
class CImageSet {
public:
    // Walk every populated frame in [m_minIndex, m_maxIndex] and (re)set the draw type
    // of its +0x30 owned shaded sprite; returns the number of frames touched.
    i32 SetAllTypes(i32 type);

    // Same walk, but writes `format` straight into each owned sprite's palette/shade
    // descriptor (m_palDescr, +0x1c). A null format is a no-op (returns 0).
    // @fake-param: `format` is really a ShadeDescr* - every caller casts a pointer into
    // it ((i32)table / (i32)spr). Retyping it re-mangles the 0x152520 symbol and ripples
    // through the second (GameLevel.h) CImageSet view, so the cast at the store site
    // stays and is honest. Returns the count touched.
    i32 SetAllFormats(i32 format);

    // Same walk, writing `value` into each populated frame's owned-sprite light level
    // (m_light, +0x18).
    i32 SetAllField18(i32 value); // 0x1524d0

    // Read the lowest-indexed frame's owned-sprite draw type (+0x14); returns 1 when
    // that frame or its owned sprite is absent. 0x152570.
    i32 GetFirstFrameState(); // 0x152570

    // Sum the decoded byte size of every populated frame (width*height scaled by the
    // held surface's bit depth, or the owned sprite's exact RLE byte count when
    // present). When `raw` is 0 each frame also carries a fixed 0x34-byte overhead
    // (== sizeof(CImage), the frame element itself). 0x1523f0.
    i32 GetMemoryUsage(i32 raw); // 0x1523f0

    // Linear-search the frame array for `frame`; on a hit, copy the set's name into
    // `outName` (when non-null) and store the matched array index through `outIndex`
    // (when non-null). Returns 1 on a hit, 0 otherwise.
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex);

    // Three frame-creation overloads (0x151fb0/0x152060/0x152110). Each refuses if a
    // frame already occupies `index`, else allocates a CImage, runs the frame's
    // loader virtual (slot 0x30/0x28/0x24), inserts it (SetAtGrow), and widens the
    // populated index range. Returns the new frame, or 0 on refusal/load failure.
    CImage* CreateFrame30(i32 a0, i32 index, i32 a2);
    CImage* CreateFrame28(i32 a0, i32 a1, i32 index, i32 a3);
    CImage* CreateFrame24(i32 a0, i32 a1, i32 index, i32 a3);

    // The bounds-checked accessor SetAllTypes/SetAllFormats inline: a frame index
    // outside [m_minIndex, m_maxIndex] yields a null frame.
    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }
        return m_frames[index];
    }

    char m_pad00[0x0c];
    CImageParent* m_owner;     // +0x0c  shared into each created frame's m_parent (+0xc)
    char m_array[0x14 - 0x10]; // +0x10  embedded frame array (SetAtGrow this)
    CImage** m_frames;         // +0x14  frame pointer array (indexed by signed frame index)
    i32 m_count;               // +0x18  array element count (for the linear FindFrame scan)
    char m_pad1c[0x24 - 0x1c];
    char m_name[0x40 - 0x24]; // +0x24  inline name buffer (copied out by FindFrame)
    char m_pad40[0x64 - 0x40];
    i32 m_minIndex; // +0x64  lowest populated frame index
    i32 m_maxIndex; // +0x68  highest populated frame index
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // SRC_IMAGE_IMAGESET_H
