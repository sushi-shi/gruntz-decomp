// GruntzCommand.cpp - the command-pattern class family (CGruntzCommand base +
// CGruntzSingleCommand / CGruntzMultiCommand leaves). See GruntzCommand.h for
// the hierarchy + layout. Names are placeholders; offsets + code bytes are
// load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CGruntzCommand::~CGruntzCommand() (??_G) 0x024330 - slot-0 scalar-deleting
//       dtor: restore vftable, (flag&1) -> operator delete(this).  BYTE-EXACT.
//   CGruntzSingleCommand::Allocate()  0x024220 - new-or-recycle allocator:
//       recycle-list tail-call else operator new(0x14) + inline-ctor vftable
//       store.  (Retail labels this RVA ??0CGruntzSingleCommand; the body is the
//       allocator, not a plain ctor - see the allocator note below.)
//   CGruntzMultiCommand::Allocate()   0x024360 - same shape, Multi list/vftable.
//
// NOT matched (see report): the two 7-byte void-thiscall vtable-set helpers at
// 0x0242f0 / 0x024430 (?CGruntzCommand_0242f0 / _024430). Their retail bytes are
// a bare `mov [this],&vftable; ret` (void, no eax-return, no null guard) - an
// out-of-line materialization of the inlined base-vftable store that MSVC 5.0
// will NOT emit as a standalone COMDAT from the canonical class under the locked
// flags (a real ??0 emits `mov eax,ecx`; placement-new emits a null guard).
// They stay in the src/Stub/ backlog as the documented COMDAT-duplication case.
#include <Gruntz/GruntzCommand.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CGruntzCommand::~CGruntzCommand() - the slot-0 scalar-deleting dtor (??_G).
// Restore the vftable, then (if the low bit of the hidden flags arg is set)
// operator delete(this). Empty body (no members, no base) - MSVC synthesizes
// the deleting thunk from `virtual ~CGruntzCommand() {}` in the header. The
// thunk has no source body so it cannot carry an RVA() attribute; pin the
// deleting-dtor symbol by mangled name here.
// @rva-symbol: ??_GCGruntzCommand@@UAEPAXI@Z 0x00024330 0x20

// Out-of-line vtable anchors (the slots NOT reconstructed here) so the
// CGruntzCommand vftable is emitted in this TU (the ctor/dtor reference it).
// Bodies are placeholders.
void CGruntzCommand::Vfunc1() {}
void CGruntzCommand::Vfunc2() {}
void CGruntzCommand::Vfunc3() {}
i32 CGruntzCommand::Vslot05() {
    return 1;
}
void CGruntzCommand::Vslot06() {}
void CGruntzCommand::Vslot07() {}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParams() - 0x023e20 (vtable slot 4). The base "set the
// five scalar params" implementation: store kind/sub bytes + two words, return
// 1. Inherited unchanged by both leaves (slot 4 = this in all three vtables).
// ---------------------------------------------------------------------------
RVA(0x00023e20, 0x2f)
i32 CGruntzCommand::SetParams(char a0, char a1, char a2, i16 a3, i16 a4) {
    m_4 = a0;
    m_5 = a1;
    m_6 = a2;
    m_8 = a3;
    m_a = a4;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParamsEx() - 0x023e60. Delegate the five scalar params to
// the base SetParams (a direct, devirtualized call), then store the +0x10/+0x11
// byte pair; return 1 (or 0 if SetParams failed).
// ---------------------------------------------------------------------------
RVA(0x00023e60, 0x42)
i32 CGruntzCommand::SetParamsEx(char a0, char a1, char a2, i16 a3, i16 a4, char a5, char a6) {
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    m_10 = a5;
    m_11 = a6;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetMaskFromList() - 0x023ed0. Validate (buf!=0, count<=0x10),
// delegate the five scalar params to SetParams, then OR a 16-bit flag mask at
// +0x10 from `count` indices in `buf` (m_10word |= 1<<buf[i]). Returns 1/0.
// ---------------------------------------------------------------------------
RVA(0x00023ed0, 0x83)
i32 CGruntzCommand::SetMaskFromList(char a0, char a1, char a2, i16 a3, i16 a4, i32 count, u8* buf) {
    if (!buf) {
        return 0;
    }
    if ((u8)count > 0x10) {
        return 0;
    }
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    *(u16*)&m_10 = 0;
    for (i32 i = 0; i < (count & 0xff); i++) {
        *(u16*)&m_10 |= g_cmdBitTable[buf[i]];
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyOne() - 0x024140. If p!=0, unpack the params and call the
// CPlay executor once with index slot = m_10. Returns its result (0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024140, 0x35)
i32 CGruntzCommand::ApplyOne(CGruntzCmdTarget* p) {
    if (!p) {
        return 0;
    }
    return p->Exec(m_4, m_10, m_5, m_8, m_a, m_11, m_6);
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyMask() - 0x024190. For each of the 16 bit positions set
// in the +0x10 flag mask, call the executor with that index; AND the results
// (return 1 only if every call succeeded; 0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024190, 0x6c)
i32 CGruntzCommand::ApplyMask(CGruntzCmdTarget* p) {
    if (!p) {
        return 0;
    }
    i32 ok = 1;
    for (i32 i = 0; i < 16; i++) {
        if (g_cmdBitTable[i] & *(u16*)&m_10) {
            if (!p->Exec(m_4, (char)i, m_5, m_8, m_a, 0, m_6)) {
                ok = 0;
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Allocate() - 0x024220.
//
// new-or-recycle allocator: if the per-class recycle list is non-empty
// (g_singleCmdCount != 0), tail-call g_singleCmdList.RemoveTail() to reuse a
// node; otherwise operator new(0x14) a fresh object and (if non-null) run the
// inline ctor, which only stamps the leaf vftable. Returns the object (or 0).
// ---------------------------------------------------------------------------
RVA(0x00024220, 0x2b)
CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    if (g_singleCmdCount) {
        return (CGruntzSingleCommand*)g_singleCmdList.RemoveTail();
    }
    return new CGruntzSingleCommand;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Allocate() - 0x024360. Same shape, Multi list/vftable.
// ---------------------------------------------------------------------------
RVA(0x00024360, 0x2b)
CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    if (g_multiCmdCount) {
        return (CGruntzMultiCommand*)g_multiCmdList.RemoveTail();
    }
    return new CGruntzMultiCommand;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::FreeAll() - 0x024490. Drain the per-class recycle list:
// while it is non-empty, RemoveTail() a node and `delete` it (the virtual
// scalar-deleting dtor with flag 1). A static method (no this).
// ---------------------------------------------------------------------------
RVA(0x00024490, 0x29)
void CGruntzMultiCommand::FreeAll() {
    while (g_multiCmdCount) {
        CGruntzCommand* node = (CGruntzCommand*)g_multiCmdList.RemoveTail();
        if (node) {
            delete node;
        }
    }
}

// size 0x14 from operator-new vtable attribution (gruntz.analysis.news)
SIZE(CGruntzMultiCommand, 0x14);
// size 0x14 from operator-new vtable attribution (gruntz.analysis.news)
SIZE(CGruntzSingleCommand, 0x14);
SIZE(CGruntzCommand, 0x14);

// The 1<<i bit table (0x5e9608) the mask builder/scanner indexes. DATA-pinned so
// the *(short*)... mask loop's address operands reloc-mask against it.
DATA(0x001e9608)
const u16 g_cmdBitTable[16] = {
    1,
    2,
    4,
    8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000
};
