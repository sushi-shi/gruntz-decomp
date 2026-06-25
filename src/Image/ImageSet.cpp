// ImageSet.cpp - CImageSet, the engine's sparse CImage-frame collection.
//
// Three matched leaf accessors over the frame array (@0x14, count @0x18, name @0x24,
// index range @0x64,0x68):
//   SetAllTypes   (0x152480) - re-set every populated frame's pixel format type.
//   SetAllFormats (0x152520) - write a resolved format word into every frame.
//   FindFrame     (0x1525c0) - locate a frame and copy out its set name + index.
//
// SetAllTypes/SetAllFormats inline the bounds-checked GetAt(index) accessor: an index
// outside [m_minIndex, m_maxIndex] yields a null frame (the inner re-check of m_minIndex
// is the inlined accessor's own guard, not a redundant source statement). FindFrame's
// name copy lowers to inline repne-scasb/rep-movs (strcpy at /O2 /Oi). The CImage frame
// format helper (CImageFormat::SetType @0x14dd90) is external/no-body so its call
// reloc-masks. No destructible stack locals -> plain /O2 (base flags, no /GX).
#include <Image/ImageSet.h>
#include <rva.h>

#include <string.h>

// SetAllTypes - 0x152480 (__thiscall, ret 4). Returns the number of frames touched.
RVA(0x00152480, 0x4e)
i32 CImageSet::SetAllTypes(i32 type) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImageFrame* frame = GetAt(i);
        if (frame && frame->m_format) {
            frame->m_format->SetType(type, 0);
            count++;
        }
    }
    return count;
}

// SetAllFormats - 0x152520 (__thiscall, ret 4). Returns the number of frames touched.
RVA(0x00152520, 0x4b)
i32 CImageSet::SetAllFormats(i32 format) {
    if (!format) {
        return 0;
    }
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImageFrame* frame = GetAt(i);
        if (frame && frame->m_format) {
            frame->m_format->m_1c = format;
            count++;
        }
    }
    return count;
}

// FindFrame - 0x1525c0 (__thiscall, ret 0xc). Returns 1 on a hit, 0 otherwise.
RVA(0x001525c0, 0x76)
i32 CImageSet::FindFrame(CImageFrame* frame, char* outName, i32* outIndex) {
    if (frame) {
        for (i32 i = 0; i < m_count; i++) {
            CImageFrame* cur = m_frames[i];
            if (cur && cur == frame) {
                if (outName) {
                    strcpy(outName, m_name);
                }
                if (outIndex) {
                    *outIndex = i;
                }
                return 1;
            }
        }
    }
    return 0;
}
