// GruntzCommand.h - the command-pattern class family the Gruntz game-object
// system uses to queue/dispatch per-grunt orders.
//
// THE HIERARCHY (recovered from RTTI + the three vtables, ImageBase 0x400000):
//   CGruntzCommand        the abstract base command (vtable 0x5e9674).
//   CGruntzSingleCommand  a single-target command   (vtable 0x5e9634).
//   CGruntzMultiCommand   a multi-target command    (vtable 0x5e96b4).
// Single/Multi each derive from CGruntzCommand: their vtables share slot 4
// (+0x10 = 0x403373, the base default) and each carries its own slot-0
// scalar-deleting dtor + its own override set. The base sub-object is vptr-only;
// the leaves are 0x14 (20) bytes = vptr + four scalar members (the 0x14 the
// per-leaf allocator passes to operator new).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS, the vtable
// stores, and the code bytes are load-bearing (campaign doctrine). The base
// layout is confirmed from the ctor/dtor; the leaf size (0x14) from the
// operator new(0x14) in each leaf's allocator.
#ifndef SRC_GRUNTZ_GRUNTZCOMMAND_H
#define SRC_GRUNTZ_GRUNTZCOMMAND_H

#include <Ints.h>

typedef u32 gz_size_t;
void* operator new(gz_size_t);

// The recycle-list type each leaf allocator pulls from (a WAP32 CObList-family
// list). Only RemoveTail() is reached (a __thiscall returning the recycled
// node). The leaf allocator: if the list is non-empty, tail-call RemoveTail();
// else operator new(0x14) the object + stamp its vftable.
struct CGruntzCmdList {
    void* RemoveTail();
};

// The "apply" target each Apply*() passes the unpacked command params to (the
// big CPlay command-executor at 0x0d1b60, ret 0x1c => 7 __thiscall args). It is
// external to this TU (reloc-masked); modeled here as a method on a tiny opaque
// helper so `mov ecx,p; push args...; call` falls out with no stack cleanup.
struct CGruntzCmdTarget {
    i32 Exec(char kind, char index, char a2, i16 a3, i16 a4, char a5, char a6);
};

// The network (de)serialization stream the base command Save/Load drive (defined in
// the .cpp). Forward-declared as a class so the member mangling agrees across clang/MSVC.
class CmdStream;

// ---------------------------------------------------------------------------
// CGruntzCommand - the abstract base command.
//
// The whole command object is 0x14 bytes (vptr + a packed parameter block); the
// data lives in the BASE (the leaves only override virtuals). The slot-0 scalar-
// deleting dtor (??_G shape, reached through the vtable's incremental-link
// thunk): restore the vftable and, if the low bit of the hidden flags arg is
// set, operator delete(this). Modeled as `virtual ~CGruntzCommand() {}` so MSVC
// synthesizes the deleting thunk; its mangled symbol is pinned by @rva-symbol in
// the .cpp.
//
// Layout (offsets pinned by the setters/getters; names are placeholders):
//   +0x04 m_4   char   (kind selector; switch index in the executor)
//   +0x05 m_5   char
//   +0x06 m_6   char
//   +0x08 m_8   short
//   +0x0a m_a   short
//   +0x10 m_10  char  } the +0x10 word is used both as a byte pair (the two
//   +0x11 m_11  char  } setters/getters) AND as a 16-bit flag mask (the bit
//                       loop), so it is read/written via *(short*)&m_10.
// ---------------------------------------------------------------------------
class CGruntzCommand {
public:
    char m_4;  // +0x04
    char m_5;  // +0x05
    char m_6;  // +0x06
    char m_7;  // +0x07 (unused by this cluster)
    i16 m_8;   // +0x08
    i16 m_a;   // +0x0a
    i32 m_c;   // +0x0c (unused by this cluster)
    char m_10; // +0x10
    char m_11; // +0x11
    i16 m_12;  // +0x12 (pad -> 0x14)

    virtual ~CGruntzCommand() {} // slot 0 (scalar-deleting dtor)
    virtual void Vfunc1();       // slot 1
    virtual void Vfunc2();       // slot 2
    virtual void Vfunc3();       // slot 3
    // slot 4 (+0x10) - the base "set params" implementation (0x023e20): store
    // the five scalar params; returns 1. Inherited unchanged by both leaves.
    virtual i32 SetParams(char a0, char a1, char a2, i16 a3, i16 a4);
    virtual i32 Vslot05();  // slot 5 (+0x14)
    virtual char Vslot06(); // slot 6  (the type/tag byte the packers write first)
    virtual void Vslot07(); // slot 7

    // Non-virtual members of the base (called directly, not via the vtable):
    i32 SetParamsEx(char a0, char a1, char a2, i16 a3, i16 a4, char a5, char a6); // 0x023e60
    i32 SetMaskFromList(char a0, char a1, char a2, i16 a3, i16 a4, i32 count,
                        u8* buf);       // 0x023ed0
    i32 ApplyOne(CGruntzCmdTarget* p);  // 0x024140
    i32 ApplyMask(CGruntzCmdTarget* p); // 0x024190
    // The network (de)serializers (0x024520 / 0x0245f0): no-op unless the
    // registry's active-game gate is set, then stream the 8 scalar fields.
    i32 Save(CmdStream* s); // 0x024520  via stream Write (+0x30)
    i32 Load(CmdStream* s); // 0x0245f0  via stream Read  (+0x2c)
};

// The 16-entry 1<<i bit table (0x5e9608; VA) the mask loop indexes/scans.
extern const u16 g_cmdBitTable[16]; // 0x1e9608

// ---------------------------------------------------------------------------
// CGruntzSingleCommand - single-target command (0x14 bytes; vtable 0x5e9634).
// Allocate() is the new-or-recycle allocator: pull a recycled node from the
// per-class free list if non-empty, else operator new(0x14) + inline ctor
// (the ctor just stamps the leaf vftable - empty body, so it folds inline).
// ---------------------------------------------------------------------------
class CGruntzSingleCommand : public CGruntzCommand {
public:
    CGruntzSingleCommand() {} // inline empty ctor (vftable store only)
    static CGruntzSingleCommand* Allocate();
    static void FreeAll(); // 0x024450 - drain g_singleCmdList, delete each node
    // 0x024050 - pack this command into a flat byte buffer: tag (Vslot06), then the
    // five scalar params, then m_10, and conditionally m_11 (when m_5 >= 8). Returns
    // the number of bytes written.
    i32 Pack(char* buf, i32 unused);
};

// ---------------------------------------------------------------------------
// CGruntzMultiCommand - multi-target command (0x14 bytes; vtable 0x5e96b4).
// Same new-or-recycle allocator shape as the single-target command.
// ---------------------------------------------------------------------------
class CGruntzMultiCommand : public CGruntzCommand {
public:
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void FreeAll(); // 0x024490 - drain g_multiCmdList, delete each node
    // 0x0240d0 - pack this command into a flat byte buffer: tag (Vslot06), the five
    // scalar params, then the +0x10 16-bit flag mask as a WORD. Returns the number
    // of bytes written.
    i32 Pack(char* buf, i32 unused);
};

// The per-class recycle lists + their non-empty gates (file-scope globals the
// allocators test/pull from). Reloc-masked; only the addresses are load-bearing.
extern i32 g_singleCmdCount;           // 0x62b5dc - non-empty gate
extern CGruntzCmdList g_singleCmdList; // 0x62b5d0 - the recycle list (ecx for RemoveTail)
extern i32 g_multiCmdCount;            // 0x62b64c
extern CGruntzCmdList g_multiCmdList;  // 0x62b640

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H
