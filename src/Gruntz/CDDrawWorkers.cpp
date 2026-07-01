#include <rva.h>
// UnknownWorkerFuncs.cpp - flat struct implementations of unnamed leaf
// functions in the Draco/Lucius region.
//
// Each struct avoids base-class virtual-inheritance so MSVC5 can resolve
// member-field references. Field offsets match the target EXACTLY.
// __thiscall via member function definitions.
// Plain /O2 /MT (/GX for SEH frames where local CString dtors are needed).
//
// Only the three reconstructed Vfunc leaves (CDDrawWorkerA::Vfunc2C, CDDrawWorkerB::Vfunc2C,
// CDDrawWorkerB::Vfunc34) are matched here; Init/Vfunc30 remain on the backlog.
// ---------------------------------------------------------------------------

struct HelperHost {
    void Helper_166040(i32 a, i32 b);
    i32 Helper_164790(i32 a, i32 b);
};

// The frame-source passed as Vfunc30's a3: an int array @0x14 indexed by a4,
// bounded by [m_64, m_68].
struct CDDrawFrameSource {
    char _pad00[0x14];
    i32* m_14; // +0x14  frame table
    char _pad18[0x64 - 0x18];
    i32 m_64; // +0x64  lower bound
    i32 m_68; // +0x68  upper bound
};

// =========================================================================
// CDDrawWorkerA (BYTE-flag, 0x7c bytes)
// =========================================================================
struct CDDrawWorkerA {
    void Init();
    i32 Vfunc2C(i32 a1, i32 a2, i32 a3);

    void* m_vptr; // +0x00
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    char _pad10[0x20 - 0x10];
    i32 m_20; // +0x20
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    char _pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c
    char _pad60[0x64 - 0x60];
    i32 m_64; // +0x64
    char _pad68[0x74 - 0x68];
    i32 m_st;  // +0x74
    char m_78; // +0x78 (BYTE)
    char _pad79[0x7c - 0x79];
};

// ---------------------------------------------------------------------------
RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    HelperHost* h = (HelperHost*)this;
    m_78 = (char)a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// =========================================================================
// CDDrawWorkerB (int-flag, 0x7c bytes)
// =========================================================================
struct CDDrawWorkerB {
    void Init();
    i32 Vfunc2C(i32 a1, i32 a2, i32 a3);
    i32 Vfunc30(i32 a1, i32 a2, i32 a3, i32 a4);
    i32 Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4);

    void* m_vptr; // +0x00
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    char _pad10[0x20 - 0x10];
    i32 m_20; // +0x20
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    char _pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c
    char _pad60[0x64 - 0x60];
    i32 m_64; // +0x64
    char _pad68[0x74 - 0x68];
    i32 m_st; // +0x74
    i32 m_78; // +0x78 (int)
};

// ---------------------------------------------------------------------------
RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    HelperHost* h = (HelperHost*)this;
    m_78 = a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4) {
    HelperHost* h = (HelperHost*)this;
    h->Helper_166040(a3, a4);
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
// 0x1572b0: store frame `a3->m_14[a4]` (0 if a4 out of [m_64,m_68]) into m_78,
// set m_st=2, then forward (a1,a2) to Helper_164790.
RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::Vfunc30(i32 a1, i32 a2, i32 a3, i32 a4) {
    CDDrawFrameSource* src = (CDDrawFrameSource*)a3;
    i32 frame;
    if (a4 >= src->m_64 && a4 <= src->m_68) {
        frame = src->m_14[a4];
    } else {
        frame = 0;
    }
    m_78 = frame;
    m_st = 2;
    return ((HelperHost*)this)->Helper_164790(a1, a2);
}

// class-metadata sweep: size annotations (SIZE_UNKNOWN = retail size TBD).
SIZE_UNKNOWN(CDDrawFrameSource);
SIZE_UNKNOWN(HelperHost);
