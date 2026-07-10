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
#include <rva.h>

#include <Ints.h>

#include <Image/ImageFrame.h> // CImageFormat / CImageFrameSurface / CImageFrame (frame element)

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

    // Read the lowest-indexed frame's format shade/type state (+0x14); returns 1 when
    // that frame or its format helper is absent. 0x152570.
    i32 GetFirstFrameState(); // 0x152570

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

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // SRC_IMAGE_IMAGESET_H
