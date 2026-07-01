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

// Global operator new (NAFXCW new-handler loop).
extern void* operator new(u32 size);

// The retail CImageFrame vtable (.?AVCImage@@ @0x5eaa2c) the factory manually
// stamps into a freshly allocated frame (the frame's own virtuals are unmatched
// engine code, so the stamp is a transitional reloc-masked DATA ref, not an
// emitted ??_7).
DATA(0x001eaa2c)
extern void* g_imageFrameVtbl[];

// The frame seen through its (unmatched) vtable: the loader virtual each factory
// overload runs, plus the scalar-deleting dtor at slot +0x04. A polymorphic view
// so the __thiscall vtable dispatch falls out; no vtable is emitted (no slot is
// defined here).
struct CImageFrameLoader {
    virtual void* v00();
    virtual void* Delete(i32 flags); // +0x04  scalar-deleting dtor
    virtual void* v08();
    virtual void* v0c();
    virtual void* v10();
    virtual void* v14();
    virtual void* v18();
    virtual void* v1c();
    virtual void* v20();
    virtual i32 Load24(i32 a, i32 b, i32 c);        // +0x24
    virtual i32 Load28(i32 a, i32 b, i32 c, i32 d); // +0x28
    virtual void* v2c();
    virtual i32 Load30(i32 a, i32 b); // +0x30
};

// MFC CObArray::SetAtGrow on the embedded frame array (CImageSet::m_array @+0x10).
struct CImageFrameArray {
    void SetAtGrow(i32 index, CImageFrame* f); // 0x1b5822  __thiscall, ret 8
};

inline CImageFrame::CImageFrame(void* owner, i32 index) {
    m_index = index;
    m_8 = 0;
    m_owner = owner;
    m_vptr = &g_imageFrameVtbl;
    m_width = 0;
    m_height = 0;
    m_surface = 0;
    m_format = 0;
}

// CreateFrame30 (__thiscall, ret 0xc). Refuse if a frame already
// occupies `index`; else allocate a CImageFrame (manual vtable stamp), run its
// loader virtual at slot +0x30, insert it (SetAtGrow at `index`) and widen the
// populated index range. The frame's loader/dtor are unmatched -> reloc-masked.
RVA(0x00151fb0, 0xdc)
CImageFrame* CImageSet::CreateFrame30(i32 a0, i32 index, i32 a2) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImageFrame* nf = new CImageFrame(m_owner, index);

    if (((CImageFrameLoader*)nf)->Load30(a0, a2) == 0) {
        if (nf != 0) {
            ((CImageFrameLoader*)nf)->Delete(1);
        }
        return 0;
    }

    ((CImageFrameArray*)&m_array)->SetAtGrow(index, nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// CreateFrame28 (__thiscall, ret 0x10). As CreateFrame30, but the loader
// virtual is at slot +0x28 and takes (a0, a1, a3, 1).
RVA(0x00152060, 0xab)
CImageFrame* CImageSet::CreateFrame28(i32 a0, i32 a1, i32 index, i32 a3) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImageFrame* nf = new CImageFrame(m_owner, index);

    if (((CImageFrameLoader*)nf)->Load28(a0, a1, a3, 1) == 0) {
        if (nf != 0) {
            ((CImageFrameLoader*)nf)->Delete(1);
        }
        return 0;
    }

    ((CImageFrameArray*)&m_array)->SetAtGrow(index, nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// CreateFrame24 (__thiscall, ret 0x10). As CreateFrame30, but the loader
// virtual is at slot +0x24 and takes (a0, a1, a3).
RVA(0x00152110, 0xa9)
CImageFrame* CImageSet::CreateFrame24(i32 a0, i32 a1, i32 index, i32 a3) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImageFrame* nf = new CImageFrame(m_owner, index);

    if (((CImageFrameLoader*)nf)->Load24(a0, a1, a3) == 0) {
        if (nf != 0) {
            ((CImageFrameLoader*)nf)->Delete(1);
        }
        return 0;
    }

    ((CImageFrameArray*)&m_array)->SetAtGrow(index, nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// GetMemoryUsage (__thiscall, ret 4). Walk every populated frame in
// [m_minIndex, m_maxIndex] (the same inlined bounds-checked GetAt) and accumulate its
// decoded byte size: width*height, doubled for a 16bpp held surface or tripled for
// 24bpp, overridden by the owned object's exact count when one is present, plus a fixed
// 0x34-byte per-frame overhead when `raw` is 0. No destructible locals -> plain /O2.
// @early-stop
// 99.96% - every instruction byte-identical except the commutative `width*height` imul:
// retail keeps m_height in esi and reads m_width as the imul memory operand; cl canonicalizes
// to the reverse (keeps m_width, reads m_height) for EVERY spelling (a*b, b*a, temp + *=). A
// 2-byte (displacement) instruction-selection canonicalization, not source-steerable.
RVA(0x001523f0, 0x82)
i32 CImageSet::GetMemoryUsage(i32 raw) {
    i32 sum = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImageFrame* frame = GetAt(i);
        if (frame) {
            i32 size = frame->m_height * frame->m_width;
            if (frame->m_surface && frame->m_surface->m_bitDepth == 0x10) {
                size += size;
            }
            if (frame->m_surface && frame->m_surface->m_bitDepth == 0x18) {
                size = size * 3;
            }
            if (frame->m_format) {
                size = frame->m_format->m_decodedByteCount;
            }
            if (raw == 0) {
                size += 0x34;
            }
            sum += size;
        }
    }
    return sum;
}

// SetAllTypes (__thiscall, ret 4). Returns the number of frames touched.
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

// SetAllField18 (__thiscall, ret 4). Walk every populated frame in
// [m_minIndex, m_maxIndex] and write `value` into its format helper's +0x18 field;
// returns the count touched. Unlike SetAllFormats there is no up-front null guard -
// the empty range simply yields count 0 (the `jg` skips straight to the return).
RVA(0x001524d0, 0x41)
i32 CImageSet::SetAllField18(i32 value) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImageFrame* frame = GetAt(i);
        if (frame && frame->m_format) {
            frame->m_format->m_18 = value;
            count++;
        }
    }
    return count;
}

// SetAllFormats (__thiscall, ret 4). Returns the number of frames touched.
RVA(0x00152520, 0x4b)
i32 CImageSet::SetAllFormats(i32 format) {
    if (!format) {
        return 0;
    }
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImageFrame* frame = GetAt(i);
        if (frame && frame->m_format) {
            frame->m_format->m_resolvedFormat = format;
            count++;
        }
    }
    return count;
}

// FindFrame (__thiscall, ret 0xc). Returns 1 on a hit, 0 otherwise.
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

// Class-metadata annotations (EOF-hosted; ImageSet.h is pulled into Gruntz/
// LightFxMgr.cpp). CImageFrameLoader is a slot-dispatch view (no emitted vtable).
SIZE_UNKNOWN(CImageFormat);
SIZE_UNKNOWN(CImageFrameSurface);
SIZE_UNKNOWN(CImageFrame);
SIZE_UNKNOWN(CImageFrameLoader);
SIZE_UNKNOWN(CImageFrameArray);
