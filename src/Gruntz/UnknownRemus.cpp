#include "../rva.h"
#include <string.h>
// UnknownRemus.cpp - five leaf methods of the tomalla-named class UnknownRemus
// (src/Stub/ metadata: vtable slot 0x38 == CGameLevel::LoadWwd, so UnknownRemus is
// the obfuscated handle for a CGameLevel-family object; modeled here as
// UnknownRemus so clang's MS mangling reproduces the target symbols
// ?VirtualMethodUnknown24@UnknownRemus@@... that synth_pdb names the delinked
// targets). All five are plain /O2 /MT leaves: NO SEH frame, NO string/global
// relocations (the dumps report "Relocations: none") - they only touch member
// offsets, an argument struct, and sibling virtuals via the object's own vtable.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine). The class carries a 4-int
// coordinate/extent record at +0x10 and a shared "default parameters" block at
// +0xb0..+0xdc that several of these methods stamp with the same constants
// (also written by the ctor @0x15ccd0 and the param-setters below):
//     +0xb0 = 500  +0xb4 = 250  +0xb8 = 1000 +0xbc = 1000
//     +0xc0 = 250  +0xc4 = 125  +0xc8 = 1600 +0xcc = 1200
//     +0xd0 = 2560 +0xd4 = 1920 +0xd8 = 768  +0xdc = 576
//
// The three 184-byte siblings (Unknown24/28/2C) are identical except for which
// sibling virtual they dispatch to: vtable +0x38 / +0x3c / +0x40 respectively.
// Each loads the +0x10 record from a caller struct, stamps the param block, then
// calls that sibling virtual with arg1; on a 0 result it invokes the +0x1c
// virtual (a "fail/reset" hook) and returns 0, else returns 1.
// ---------------------------------------------------------------------------

// The 4-int coordinate/extent record copied into UnknownRemus+0x10.
struct RemusCoords {
    int m_0;
    int m_4;
    int m_8;
    int m_c;
};

// External CDWordArray::SetSize (reloc-masked NAFXCW engine call).
struct CDWordArray {
    void SetSize(int nNewSize, int nGrowBy);
};

// UnknownChild - placeholder for whatever class lives in the pointer arrays
// at +0x38 and +0x4c. Only vtable slot 4 (+0x04, virtual Release(1)) is used.
class UnknownChild {
public:
    virtual void Dummy();
    virtual void Release(int arg);
};

// ---------------------------------------------------------------------------
// UnknownRemus - only the load-bearing vtable slots + member offsets are modeled.
// The vtable must place the dispatched siblings at exactly these byte offsets:
//   +0x1c (slot 7)  fail/reset hook        +0x38 (slot 14) variant for Unknown24
//   +0x3c (slot 15) variant for Unknown28  +0x40 (slot 16) variant for Unknown2C
// so the declaration order below pads slots 0..16 to land them precisely. The
// five matched methods themselves occupy other (lower) slots; their bodies are
// what we match, not their slot numbers, so they are placed last.
// ---------------------------------------------------------------------------
class UnknownRemus {
public:
    // --- vtable padding to land the dispatched siblings at the right offsets ---
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

    // --- the five matched leaves --------------------------------------------
    int  VirtualMethodUnknown24(int arg1, RemusCoords *coords);
    int  VirtualMethodUnknown28(int arg1, RemusCoords *coords);
    int  VirtualMethodUnknown2C(int arg1, RemusCoords *coords);
    int  VirtualMethodUnknown34(int arg0, int arg1);
    int  VirtualMethodUnknown30(RemusCoords *coords);
    int  VirtualMethodUnknown14();

    // --- matched leaves moved out of the backlog -------
    int  VirtualMethodUnknown1C();
    void VirtualMethodUnknown44();
    int  VirtualMethodUnknown20();

    // --- members ------------------------------------------------------------
    int         m_04;                   // +0x04  initialized to -1 when inactive
    char        m_pad08[0x0c - 0x08];   // +0x08..0x0b
    int         m_0c;                   // +0x0c  parent/root handle
    RemusCoords m_10;                   // +0x10  coordinate/extent record (4 ints)
    char        m_pad20[0x14];          // +0x20..0x33
    char        m_pad34[0x04];          // +0x34..0x37  (CDWordArray vtable)
    void **     m_38;                   // +0x38  child pointer array
    int         m_3c;                   // +0x3c  count
    char        m_pad40[0x48 - 0x40];   // +0x40..0x47
    char        m_pad48[0x04];          // +0x48..0x4b  (CDWordArray vtable)
    void **     m_4c;                   // +0x4c  child pointer array
    int         m_50;                   // +0x50  count
    char        m_pad54[0x5c - 0x54];   // +0x54..0x5b
    int         m_5c;                   // +0x5c  main-plane ptr (set to 0)
    int         m_60;                   // +0x60  main-plane index (set to -1)
    int         m_64;                   // +0x64
    int         m_68;                   // +0x68
    char        m_pad6c[0xac - 0x6c];   // +0x6c..0xab
    int         m_ac;                   // +0xac
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

    // Engine-label backlog stubs.
    void Stub_15d1f0();
    void Stub_15d500();
    void Stub_15d630();
    void Stub_15d680();
    void Stub_1611b0();
    void Stub_1611c0();
    void Stub_1611e0();
};

// Stamps the shared +0xb0..+0xdc "default parameters" block. Defined inline so it
// folds into each method exactly as the retail compiler emitted the block inline.
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
// UnknownRemus::VirtualMethodUnknown14  @0x161190  (__thiscall, ret 0)
// Remus adds a +0x10 sentinel check before the common parent/status predicate.
RVA(0x161190, 0x1f)
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
// UnknownRemus::VirtualMethodUnknown34  @0x15d030  (__thiscall, ret 8)
// Zeroes the first two ints of the +0x10 record, stores (arg0-1)/(arg1-1) into
// the last two, stamps the param block, returns 1.
//
// RESIDUE (~84%, NOT a logic/offset/type/CFG error): byte-for-byte identical to
// the target EXCEPT the position of one instruction - the immediate store
// `mov dword ptr [ecx+0xb0], 0x1f4`. The retail compiler schedules it mid-block
// (after +0xbc, before +0xb4); here MSVC hoists the same store to the earliest
// free slot (right after `mov eax,[esp+4]`, before `dec eax`). Same bytes, same
// register allocation everywhere else. This is the documented store-scheduling
// entropy (matching-patterns.md "optimizer reorders field stores"): an
// independent immediate-to-memory store has no register dependency to pin it, so
// MSVC floats it freely. Every source ordering tried either kept this single
// slip or regressed the eax(0x3e8)/edx(0xfa) allocation (b8,bc,b0,b4 order ->
// ~75%); calling the param block before the +0x10 writes moves the whole block
// ahead (wrong). Logic + offsets + CFG are exact, so this is left as the plateau.
RVA(0x15d030, 0x8f)
int UnknownRemus::VirtualMethodUnknown34(int arg0, int arg1)
{
    m_10.m_0 = 0;
    m_10.m_4 = 0;
    m_10.m_8 = arg0 - 1;
    m_10.m_c = arg1 - 1;
    StampParamBlock(this);
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------

// @confidence: high
// @source: tomalla
// @stub
RVA(0x15d500, 0x127)
void UnknownRemus::Stub_15d500() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x15d630, 0x41)
void UnknownRemus::Stub_15d630() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1611c0, 0x1e)
void UnknownRemus::Stub_1611c0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x1611e0, 0x82)
void UnknownRemus::Stub_1611e0() {}

// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown1C  @0x15d1f0  (__thiscall)
// Like Unknown44 plus resets the sentinel and zeroes the WwdHeader buffer.
// ---------------------------------------------------------------------------
RVA(0x15d1f0, 0x87)
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
// UnknownRemus::VirtualMethodUnknown44  @0x15d680  (__thiscall)
// Releases all child pointers, resets both CDWordArrays, clears members.
// ---------------------------------------------------------------------------
RVA(0x15d680, 0x71)
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
// UnknownRemus::VirtualMethodUnknown20  @0x1611b0  (__thiscall)
// Returns constant 0x19 (25) — a type-tag or enum identifier.
// ---------------------------------------------------------------------------
RVA(0x1611b0, 0x6)
int UnknownRemus::VirtualMethodUnknown20()
{
    return 0x19;
}

// --- restored: matching's RemusCoords sibling definitions (do not drop) ---
// ---------------------------------------------------------------------------
// UnknownRemus::VirtualMethodUnknown2C  @0x15cdf0  (__thiscall, ret 8)
// As Unknown24 but dispatches the +0x40 sibling virtual.
RVA(0x15cdf0, 0xb8)
int UnknownRemus::VirtualMethodUnknown2C(int arg1, RemusCoords *coords)
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
// UnknownRemus::VirtualMethodUnknown28  @0x15ceb0  (__thiscall, ret 8)
// As Unknown24 but dispatches the +0x3c sibling virtual.
RVA(0x15ceb0, 0xb8)
int UnknownRemus::VirtualMethodUnknown28(int arg1, RemusCoords *coords)
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
// UnknownRemus::VirtualMethodUnknown24  @0x15cf70  (__thiscall, ret 8)
// Loads the +0x10 record from *coords, stamps the param block, then dispatches
// the +0x38 sibling virtual with arg1. On a 0 result it runs the +0x1c hook and
// returns 0; otherwise returns 1.
RVA(0x15cf70, 0xb8)
int UnknownRemus::VirtualMethodUnknown24(int arg1, RemusCoords *coords)
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
// UnknownRemus::VirtualMethodUnknown30  @0x15d0d0  (__thiscall, ret 4)
// Loads the +0x10 record from *coords, stamps the param block, returns 1.
RVA(0x15d0d0, 0x99)
int UnknownRemus::VirtualMethodUnknown30(RemusCoords *coords)
{
    m_10 = *coords;
    StampParamBlock(this);
    return 1;
}
