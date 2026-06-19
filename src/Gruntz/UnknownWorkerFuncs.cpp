#include "../rva.h"
// UnknownWorkerFuncs.cpp - flat struct implementations of unnamed leaf
// functions in the Draco/Lucius region (0x157000-0x15a000).
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
    void Helper_166040(int a, int b);
    int  Helper_164790(int a, int b);
};

// =========================================================================
// CDDrawWorkerA (BYTE-flag, 0x7c bytes): Vfunc @0x157110
// =========================================================================
struct CDDrawWorkerA {
    void Init();
    int  Vfunc2C(int a1, int a2, int a3);

    void *m_vptr;          // +0x00
    int   m_04;            // +0x04
    int   m_08;            // +0x08
    int   m_0c;            // +0x0c
    char  _pad10[0x20 - 0x10];
    int   m_20;            // +0x20
    char  _pad24[0x38 - 0x24];
    int   m_38;            // +0x38
    int   m_3c;            // +0x3c
    int   m_40;            // +0x40
    char  _pad44[0x5c - 0x44];
    int   m_5c;            // +0x5c
    char  _pad60[0x64 - 0x60];
    int   m_64;            // +0x64
    char  _pad68[0x74 - 0x68];
    int   m_st;            // +0x74
    char  m_78;            // +0x78 (BYTE)
    char  _pad79[0x7c - 0x79];
};

// ---------------------------------------------------------------------------
// CDDrawWorkerA::Vfunc2C  @0x157110
// ---------------------------------------------------------------------------
RVA(0x157110, 0x20)
int CDDrawWorkerA::Vfunc2C(int a1, int a2, int a3)
{
    HelperHost *h = (HelperHost *)this;
    m_78 = (char)a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// =========================================================================
// CDDrawWorkerB (int-flag, 0x7c bytes): Vfuncs @0x1572f0/0x157280
// =========================================================================
struct CDDrawWorkerB {
    void Init();
    int  Vfunc2C(int a1, int a2, int a3);
    int  Vfunc30(int a1, int a2, int a3, int a4);
    int  Vfunc34(int a1, int a2, int a3, int a4);

    void *m_vptr;          // +0x00
    int   m_04;            // +0x04
    int   m_08;            // +0x08
    int   m_0c;            // +0x0c
    char  _pad10[0x20 - 0x10];
    int   m_20;            // +0x20
    char  _pad24[0x38 - 0x24];
    int   m_38;            // +0x38
    int   m_3c;            // +0x3c
    int   m_40;            // +0x40
    char  _pad44[0x5c - 0x44];
    int   m_5c;            // +0x5c
    char  _pad60[0x64 - 0x60];
    int   m_64;            // +0x64
    char  _pad68[0x74 - 0x68];
    int   m_st;            // +0x74
    int   m_78;            // +0x78 (int)
};

// ---------------------------------------------------------------------------
// CDDrawWorkerB::Vfunc2C  @0x1572f0
// ---------------------------------------------------------------------------
RVA(0x1572f0, 0x20)
int CDDrawWorkerB::Vfunc2C(int a1, int a2, int a3)
{
    HelperHost *h = (HelperHost *)this;
    m_78 = a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
// CDDrawWorkerB::Vfunc34  @0x157280
// ---------------------------------------------------------------------------
RVA(0x157280, 0x30)
int CDDrawWorkerB::Vfunc34(int a1, int a2, int a3, int a4)
{
    HelperHost *h = (HelperHost *)this;
    h->Helper_166040(a3, a4);
    m_st = 2;
    return h->Helper_164790(a1, a2);
}
