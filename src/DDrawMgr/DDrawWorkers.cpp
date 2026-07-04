// DDrawWorkers.cpp - the CDDrawWorkerA/CDDrawWorkerB frame-set virtuals (vtable
// slots 11/12/13). Each stashes the requested frame + marks the worker armed
// (m_74 = 2), then forwards to the inherited reset/arm helper (CDDrawWorkerBase::
// Helper_164790, 0x164790) or the own named-object frame fetch
// (CDDrawWorkerB::Helper_166040, 0x166040). The worker hierarchy lives in the
// shared <DDrawMgr/DDrawWorkerNode.h>; calling the (inherited/own) helper directly
// replaces the former (HelperHost*)this cross-cast.
//
// Plain /O2 /MT leaves (no SEH frame). Field names are placeholders; offsets +
// code bytes are load-bearing.
#include <DDrawMgr/DDrawWorkerNode.h>

#include <Ints.h>
#include <rva.h>

// The frame-source passed as Vfunc30's a3: an int array @+0x14 indexed by a4,
// bounded by [m_lowerBound, m_upperBound].
struct CDDrawFrameSource {
    char _pad00[0x14];
    i32* m_frameTable; // +0x14  frame table
    char _pad18[0x64 - 0x18];
    i32 m_lowerBound; // +0x64  lower bound
    i32 m_upperBound; // +0x68  upper bound
};
SIZE_UNKNOWN(CDDrawFrameSource);

// ---------------------------------------------------------------------------
RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = (char)a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper_166040(a3, a4);
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
// 0x1572b0: store frame `src->m_frameTable[a4]` (0 if a4 out of [m_lowerBound,
// m_upperBound]) into m_78, set m_74=2, then forward (a1,a2) to Helper_164790.
RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::Vfunc30(i32 a1, i32 a2, CDDrawFrameSource* src, i32 a4) {
    i32 frame;
    if (a4 >= src->m_lowerBound && a4 <= src->m_upperBound) {
        frame = src->m_frameTable[a4];
    } else {
        frame = 0;
    }
    m_78 = frame;
    m_74 = 2;
    return Helper_164790(a1, a2);
}
