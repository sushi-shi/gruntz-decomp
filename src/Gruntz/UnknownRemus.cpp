// UnknownRemus.cpp - leaf methods of the tomalla-named class UnknownRemus
// (vtable slot 0x38 == CGameLevel::LoadWwd; UnknownRemus = CGameLevel-family).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine). The class carries a 4-int coordinate/extent
// record at +0x10 and a shared "default parameters" block at +0xb0..+0xdc:
//     +0xb0 = 500  +0xb4 = 250  +0xb8 = 1000 +0xbc = 1000
//     +0xc0 = 250  +0xc4 = 125  +0xc8 = 1600 +0xcc = 1200
//     +0xd0 = 2560 +0xd4 = 1920 +0xd8 = 768  +0xdc = 576
//
// The three 184-byte siblings (Unknown24/28/2C) dispatch through vtable +0x38/
// +0x3c/+0x40. The cleanup siblings (Unknown1C/44) release child pointers and
// call CDWordArray::SetSize. Unknown40 opens a CFileIO, reads a file into an
// operator-new buffer, dispatches through Vfunc38, then free and return.
#include "../Io/FileStream.h"
#include <string.h>

// ---------------------------------------------------------------------------
// The 4-int coordinate/extent record copied into UnknownRemus+0x10.
// ---------------------------------------------------------------------------
struct DWORD_PTR {
    int m_0;
    int m_4;
    int m_8;
    int m_c;
};

// ---------------------------------------------------------------------------
// External CDWordArray::SetSize (reloc-masked NAFXCW engine call).
// ---------------------------------------------------------------------------
struct CDWordArray {
    void SetSize(int nNewSize, int nGrowBy);
};

// ---------------------------------------------------------------------------
// UnknownChild - placeholder for whatever class lives in the pointer arrays
// at +0x38 and +0x4c. Only vtable slot 4 (+0x04, virtual Release(1)) is used.
// ---------------------------------------------------------------------------
class UnknownChild {
public:
    virtual void Dummy();
    virtual void Release(int arg);
};

// ---------------------------------------------------------------------------
// UnknownRemus - the level object. Member layout per CGameLevel.
//   +0x04  m_04           arg2 from ctor
//   +0x08  m_08           arg3 from ctor
//   +0x0c  m_0c           arg1 from ctor (parent/root handle)
//   +0x10  m_10           DWORD_PTR (4-int coordinate/extent record)
//   +0x20  planeCtx[0x14] first CByteArray sub-object (only constructed)
//   +0x34  arrayA         CDWordArray (m_data@+0x38, m_nSize@+0x3c)
//   +0x48  arrayB         CDWordArray (m_data@+0x4c, m_nSize@+0x50)
//   +0x5c  m_5c           main-plane ptr (set to 0 by cleanup siblings)
//   +0x60  m_60           main-plane index (set to -1 by cleanup siblings)
//   +0x64  m_64           set to 0x40 in ctor
//   +0x68  m_68           set to 0x40 in ctor
//   +0x6c  m_levelName[64]
//   +0xac  m_ac           checksum (set to 0 in ctor)
//   +0xb0..+0xdc          param block (12 ints)
//   +0xe0  m_header[381]  WwdHeader work buffer (1524 B)
//
// The vtable must place the dispatched siblings at exactly these byte offsets:
//   +0x1c (slot 7)  fail/reset hook        +0x38 (slot 14) variant for Unknown24
//   +0x3c (slot 15) variant for Unknown28  +0x40 (slot 16) variant for Unknown2C
// so the declaration order below pads slots 0..16 to land them precisely. The
// matched methods themselves occupy OTHER vtable slots; their bodies are what
// we match, not their slot numbers.
// ---------------------------------------------------------------------------
class UnknownRemus {
public:
    // --- vtable padding slots (land dispatched siblings at correct offsets) -----
    virtual void Slot00();              // +0x00
    virtual void Slot04();              // +0x04
    virtual void Slot08();              // +0x08
    virtual void Slot0C();              // +0x0c
    virtual void Slot10();              // +0x10
    virtual void Slot14();              // +0x14
    virtual void Slot18();              // +0x18
    virtual void Vfunc1C();             // +0x1c  fail/reset hook
    virtual void Slot20();              // +0x20
    virtual void Slot24();              // +0x24
    virtual void Slot28();              // +0x28
    virtual void Slot2C();              // +0x2c
    virtual void Slot30();              // +0x30
    virtual void Slot34();              // +0x34
    virtual int  Vfunc38(int arg1);     // +0x38
    virtual int  Vfunc3C(int arg1);     // +0x3c
    virtual int  Vfunc40(int arg1);     // +0x40

    // --- matched functional leaves (bodies are what we match) -------------------
    UnknownRemus(int a1, int a2, int a3);
    int  VirtualMethodUnknown1C();
    int  VirtualMethodUnknown20();
    int  VirtualMethodUnknown40(const char *path);
    void VirtualMethodUnknown44();
    int  VirtualMethodUnknown24(int arg1, DWORD_PTR *coords);
    int  VirtualMethodUnknown28(int arg1, DWORD_PTR *coords);
    int  VirtualMethodUnknown2C(int arg1, DWORD_PTR *coords);
    int  VirtualMethodUnknown34(int arg0, int arg1);
    int  VirtualMethodUnknown30(DWORD_PTR *coords);
    int  VirtualMethodUnknown14();

    // --- members ------------------------------------------------------------
    int         m_04;                   // +0x04  arg2 from ctor
    int         m_08;                   // +0x08  arg3 from ctor
    int         m_0c;                   // +0x0c  arg1 from ctor (parent handle)
    DWORD_PTR m_10;                   // +0x10  coordinate/extent record (4 ints)

    // +0x20: CDWordArray / CByteArray (unused by these methods, just padding)
    char        m_pad20[0x14];          // +0x20..0x33

    // +0x34: CDWordArray / CLevelPtrArray (m_pData@+0x38, m_nSize@+0x3c)
    char        m_pad34[0x04];          // +0x34..0x37  (vtable)
    void **     m_38;                   // +0x38  child pointer array
    int         m_3c;                   // +0x3c  count
    char        m_pad40[0x48 - 0x40];   // +0x40..0x47

    // +0x48: CDWordArray / CLevelPtrArray (m_pData@+0x4c, m_nSize@+0x50)
    char        m_pad48[0x04];          // +0x48..0x4b  (vtable)
    void **     m_4c;                   // +0x4c  child pointer array
    int         m_50;                   // +0x50  count
    char        m_pad54[0x5c - 0x54];   // +0x54..0x5b

    int         m_5c;                   // +0x5c  main-plane ptr (set to 0)
    int         m_60;                   // +0x60  main-plane index (set to -1)
    int         m_64;                   // +0x64  (set to 0x40 in ctor)
    int         m_68;                   // +0x68  (set to 0x40 in ctor)
    char        m_pad6c[0xac - 0x6c];   // +0x6c..0xab  m_levelName[64]
    int         m_ac;                   // +0xac  checksum

    int         m_b0;                   // +0xb0  = 500
    int         m_b4;                   // +0xb4  = 250
    int         m_b8;                   // +0xb8  = 1000
    int         m_bc;                   // +0xbc  = 1000
    int         m_c0;                   // +0xc0  = 250
    int         m_c4;                   // +0xc4  = 125
    int         m_c8;                   // +0xc8  = 1600
    int         m_cc;                   // +0xcc  = 1200
    int         m_d0;                   // +0xd0  = 2560
    int         m_d4;                   // +0xd4  = 1920
    int         m_d8;                   // +0xd8  = 768
    int         m_dc;                   // +0xdc  = 576

    int         m_header[381];          // +0xe0  WwdHeader work buffer (1524 B)
};

// ---------------------------------------------------------------------------
// Stamps the +0xb0..+0xdc param block. Defined inline so it folds into each
// method as MSVC would schedule it (some store-ordering residue is expected).
// ---------------------------------------------------------------------------
static inline void StampParamBlock(UnknownRemus *o)
{
    o->m_b0 = 500;
    o->m_b4 = 250;
    o->m_b8 = 1000;
    o->m_bc = 1000;
    o->m_c0 = 250;
    o->m_c4 = 125;
    o->m_c8 = 1600;
    o->m_cc = 1200;
    o->m_d0 = 2560;
    o->m_d4 = 1920;
    o->m_d8 = 768;
    o->m_dc = 576;
}

// ---------------------------------------------------------------------------
// UnknownRemus::UnknownRemus  @0x15ccd0  (__thiscall ret 0xc)
// ---------------------------------------------------------------------------
// @address: 0x15ccd0
// @size:    0x118
UnknownRemus::UnknownRemus(int a1, int a2, int a3)
{
    m_04 = a2;
    m_08 = a3;
    m_0c = a1;

    // Construct three inline CByteArray sub-objects (external engine calls).
    extern void CByteArrayCtor(void *pThis);
    CByteArrayCtor((char *)this + 0x20);
    CByteArrayCtor((char *)this + 0x34);
    CByteArrayCtor((char *)this + 0x48);

    m_64 = 0x40;
    m_68 = 0x40;
    m_b4 = 250;
    m_c0 = 250;
    m_b8 = 1000;
    m_bc = 1000;

    m_10.m_0 = (int)0x80000000;
    m_5c = 0;
    m_60 = -1;
    m_ac = 0;
    m_b0 = 500;
    m_c4 = 125;
    m_c8 = 1600;
    m_cc = 1200;
    m_d0 = 2560;
    m_d4 = 1920;
    m_d8 = 768;
    m_dc = 576;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown14  @0x161190  (__thiscall, ret 0)
// ---------------------------------------------------------------------------
// @address: 0x161190
// @size:    0x1f
int UnknownRemus::VirtualMethodUnknown14()
{
    if (m_10.m_0 == (int)0x80000000)
        goto fail;
    if (m_0c == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown30  @0x15d0d0  (__thiscall, ret 4)
// ---------------------------------------------------------------------------
// @address: 0x15d0d0
// @size:    0x99
int UnknownRemus::VirtualMethodUnknown30(DWORD_PTR *coords)
{
    m_10 = *coords;
    StampParamBlock(this);
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown34  @0x15d030  (__thiscall, ret 8)
// RESIDUE (~84%): one independent store scheduling difference.
// ---------------------------------------------------------------------------
// @address: 0x15d030
// @size:    0x92
int UnknownRemus::VirtualMethodUnknown34(int arg0, int arg1)
{
    m_10.m_0 = 0;
    m_10.m_4 = 0;
    m_10.m_8 = arg0 - 1;
    m_10.m_c = arg1 - 1;
    StampParamBlock(this);
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown24  @0x15cf70  (__thiscall, ret 8)
// ---------------------------------------------------------------------------
// @address: 0x15cf70
// @size:    0xb8
int UnknownRemus::VirtualMethodUnknown24(int arg1, DWORD_PTR *coords)
{
    m_10 = *coords;
    StampParamBlock(this);
    if (Vfunc38(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown28  @0x15ceb0  (__thiscall, ret 8)
// ---------------------------------------------------------------------------
// @address: 0x15ceb0
// @size:    0xb8
int UnknownRemus::VirtualMethodUnknown28(int arg1, DWORD_PTR *coords)
{
    m_10 = *coords;
    StampParamBlock(this);
    if (Vfunc3C(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown2C  @0x15cdf0  (__thiscall, ret 8)
// ---------------------------------------------------------------------------
// @address: 0x15cdf0
// @size:    0xb8
int UnknownRemus::VirtualMethodUnknown2C(int arg1, DWORD_PTR *coords)
{
    m_10 = *coords;
    StampParamBlock(this);
    if (Vfunc40(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown20  @0x1611b0  (__thiscall)
// Returns constant 0x19 (25) — a type-tag or enum identifier.
// ---------------------------------------------------------------------------
// @address: 0x1611b0
// @size:    0x6
int UnknownRemus::VirtualMethodUnknown20()
{
    return 0x19;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown44  @0x15d680  (__thiscall)
// Releases all child pointers, resets both CDWordArrays, clears members.
// ---------------------------------------------------------------------------
// @address: 0x15d680
// @size:    0x71
void UnknownRemus::VirtualMethodUnknown44()
{
    int i;
    for (i = 0; i < m_3c; i++) {
        UnknownChild *child = (UnknownChild *)m_38[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x34))->SetSize(0, -1);
    for (i = 0; i < m_50; i++) {
        UnknownChild *child = (UnknownChild *)m_4c[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x48))->SetSize(0, -1);
    m_5c = 0;
    m_60 = -1;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown1C  @0x15d1f0  (__thiscall)
// Like Unknown44 plus resets the sentinel and zeroes the WwdHeader buffer.
// ---------------------------------------------------------------------------
// @address: 0x15d1f0
// @size:    0x87
int UnknownRemus::VirtualMethodUnknown1C()
{
    int i;
    for (i = 0; i < m_3c; i++) {
        UnknownChild *child = (UnknownChild *)m_38[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x34))->SetSize(0, -1);
    for (i = 0; i < m_50; i++) {
        UnknownChild *child = (UnknownChild *)m_4c[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x48))->SetSize(0, -1);
    m_10.m_0 = (int)0x80000000;
    m_5c = 0;
    m_60 = -1;
    memset(m_header, 0, sizeof(m_header));
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown40  @0x15d500 (__thiscall ret 4)
// Opens a file path, reads its content into an operator-new buffer, dispatches
// through Vfunc38(buffer), frees the buffer, returns Vfunc38 result != 0.
// ---------------------------------------------------------------------------
// @address: 0x15d500
// @size:    0x127
int UnknownRemus::VirtualMethodUnknown40(const char *path)
{
    CFileIO file;
    if (!file.Open(path, 0, 0))
        return 0;

    void *buf = operator new(file.GetLength());
    if (!buf)
        return 0;

    file.Read(buf, file.GetLength());
    int result = Vfunc38((int)buf);
    operator delete(buf);
    return result ? 1 : 0;
}
