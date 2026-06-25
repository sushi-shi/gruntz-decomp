// ImageSet.h - CImageSet, the engine's sparse CImage-frame collection.
//
// A CImageSet owns an array of CImage* frames (the RTTI `.?AVCImage@@` 0x34-byte
// sprite primitive, vtable @0x5eaa2c) addressed by a signed frame index that runs
// over the inclusive range [m_minIndex, m_maxIndex]; the set carries an inline name
// buffer and a CObArray-style backing store at +0x10. Only the three leaf accessors
// in ImageSet.cpp are matched here - the FUN_* labels carry no real name, so the
// class name (CImageSet) and field names are descriptive placeholders; the OFFSETS +
// code bytes are the load-bearing facts (object size 0x6c, vtable @0x5efbe8, the
// frame array @0x14 / count @0x18 / inline name @0x24 / index range @0x64,0x68).
#ifndef SRC_IMAGE_IMAGESET_H
#define SRC_IMAGE_IMAGESET_H

#include <Ints.h>

// The CImage frame's +0x30 sub-object: a tiny format/state helper. SetType installs
// a pixel-format code (m_14) and resolves its DDraw format word (m_1c). It is the
// engine's CImage::SetType (FUN_0054dd90), external/no-body here so its call
// reloc-masks; only the m_1c field is touched directly by CImageSet.
class CImageFormat {
public:
    void SetType(i32 type, i32 noResolve); // 0x14dd90 (__thiscall, ret 8)

    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  resolved DDraw format word (written directly by SetAllFormats)
};

// The CImage frame as seen by CImageSet: only the +0x30 format sub-object is used.
class CImageFrame {
public:
    char m_pad00[0x30];
    CImageFormat* m_format; // +0x30  format/state helper (may be null)
};

class CImageSet {
public:
    // Walk every populated frame in [m_minIndex, m_maxIndex] and (re)set its pixel
    // format via the frame's +0x30 helper; returns the number of frames touched.
    i32 SetAllTypes(i32 type);

    // Same walk, but writes `format` straight into each frame's resolved format word
    // (+0x1c). A null format is a no-op (returns 0). Returns the count touched.
    i32 SetAllFormats(i32 format);

    // Linear-search the frame array for `frame`; on a hit, copy the set's name into
    // `outName` (when non-null) and store the matched array index through `outIndex`
    // (when non-null). Returns 1 on a hit, 0 otherwise.
    i32 FindFrame(CImageFrame* frame, char* outName, i32* outIndex);

    // The bounds-checked accessor SetAllTypes/SetAllFormats inline: a frame index
    // outside [m_minIndex, m_maxIndex] yields a null frame.
    CImageFrame* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }
        return m_frames[index];
    }

    char m_pad00[0x14];
    CImageFrame** m_frames; // +0x14  frame pointer array (indexed by signed frame index)
    i32 m_count;            // +0x18  array element count (for the linear FindFrame scan)
    char m_pad1c[0x24 - 0x1c];
    char m_name[0x40 - 0x24]; // +0x24  inline name buffer (copied out by FindFrame)
    char m_pad40[0x64 - 0x40];
    i32 m_minIndex; // +0x64  lowest populated frame index
    i32 m_maxIndex; // +0x68  highest populated frame index
};

#endif // SRC_IMAGE_IMAGESET_H
