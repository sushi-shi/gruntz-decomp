// UnknownWorkerFuncs.cpp - flat struct implementations of unnamed leaf
// functions in the Draco/Lucius region (0x157000-0x15a000).
//
// Each struct avoids base-class virtual-inheritance so MSVC5 can resolve
// member-field references. Field offsets match the target EXACTLY.
// __thiscall via member function definitions.
// Plain /O2 /MT (/GX for SEH frames where local CString dtors are needed).
// ---------------------------------------------------------------------------

struct HelperHost {
    void Helper_166040(int a, int b);
    int  Helper_164790(int a, int b);
};

extern void *g_cobjectVtbl;  // VA 0x5e8cb4

// =========================================================================
// WorkerA (BYTE-flag, 0x7c bytes): Init @0x1570d0, Vfunc @0x157110
// =========================================================================
struct WorkerA {
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

// @address: 0x1570d0
// @size:    0x39
void WorkerA::Init()
{
    m_78 = 0;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_5c = (int)0x80000000;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    *(void **)this = &g_cobjectVtbl;
}

// @address: 0x157110
// @size:    0x20
int WorkerA::Vfunc2C(int a1, int a2, int a3)
{
    HelperHost *h = (HelperHost *)this;
    m_78 = (char)a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// =========================================================================
// WorkerB (int-flag, 0x7c bytes): Init @0x157240, Vfuncs @0x1572f0/0x1572b0/0x157280
// =========================================================================
struct WorkerB {
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

// @address: 0x157240
// @size:    0x3c
void WorkerB::Init()
{
    m_78 = 0;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_5c = (int)0x80000000;
    m_20 = (int)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    *(void **)this = &g_cobjectVtbl;
}

// @address: 0x1572f0
// @size:    0x20
int WorkerB::Vfunc2C(int a1, int a2, int a3)
{
    HelperHost *h = (HelperHost *)this;
    m_78 = a3;
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// @address: 0x1572b0
// @size:    0x38
int WorkerB::Vfunc30(int a1, int a2, int a3, int a4)
{
    HelperHost *h = (HelperHost *)this;
    int lo = *(int *)((char *)a2 + 0x64);
    int hi = *(int *)((char *)a2 + 0x68);
    if (a3 >= lo && a3 <= hi) {
        m_78 = *(int *)(*(int *)((char *)a2 + 0x14) + a3 * 4);
    } else {
        m_78 = 0;
    }
    m_st = 2;
    return h->Helper_164790(a1, a2);
}

// @address: 0x157280
// @size:    0x30
int WorkerB::Vfunc34(int a1, int a2, int a3, int a4)
{
    HelperHost *h = (HelperHost *)this;
    h->Helper_166040(a3, a4);
    m_st = 2;
    return h->Helper_164790(a1, a2);
}
