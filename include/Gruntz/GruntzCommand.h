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

typedef unsigned int gz_size_t;
void *operator new(gz_size_t);

// The recycle-list type each leaf allocator pulls from (a WAP32 CObList-family
// list). Only RemoveTail() is reached (a __thiscall returning the recycled
// node). The leaf allocator: if the list is non-empty, tail-call RemoveTail();
// else operator new(0x14) the object + stamp its vftable.
struct CGruntzCmdList {
    void *RemoveTail();
};

// ---------------------------------------------------------------------------
// CGruntzCommand - the abstract base command.
//
// The base sub-object is vptr-only. ~CGruntzCommand is the slot-0 scalar-
// deleting dtor (??_G shape, reached through the vtable's incremental-link
// thunk): restore the vftable and, if the low bit of the hidden flags arg is
// set, operator delete(this). Empty body (no destructible members, no base to
// chain) - modeled as `virtual ~CGruntzCommand() {}` so MSVC synthesizes the
// deleting thunk; its mangled symbol is pinned by @rva-symbol in the .cpp.
// ---------------------------------------------------------------------------
class CGruntzCommand {
public:
    virtual ~CGruntzCommand() {}     // slot 0 (scalar-deleting dtor)
    virtual void Vfunc1();           // slot 1
    virtual void Vfunc2();           // slot 2
    virtual void Vfunc3();           // slot 3
    virtual int  Vslot04();          // slot 4 (+0x10)  base default = return 1;
    virtual int  Vslot05();          // slot 5 (+0x14)
    virtual void Vslot06();          // slot 6
    virtual void Vslot07();          // slot 7
};

// ---------------------------------------------------------------------------
// CGruntzSingleCommand - single-target command (0x14 bytes; vtable 0x5e9634).
// Allocate() is the new-or-recycle allocator: pull a recycled node from the
// per-class free list if non-empty, else operator new(0x14) + inline ctor
// (the ctor just stamps the leaf vftable - empty body, so it folds inline).
// ---------------------------------------------------------------------------
class CGruntzSingleCommand : public CGruntzCommand {
public:
    int m_4, m_8, m_c, m_10;         // four scalar members (size -> 0x14)
    CGruntzSingleCommand() {}        // inline empty ctor (vftable store only)
    static CGruntzSingleCommand *Allocate();
};

// ---------------------------------------------------------------------------
// CGruntzMultiCommand - multi-target command (0x14 bytes; vtable 0x5e96b4).
// Same new-or-recycle allocator shape as the single-target command.
// ---------------------------------------------------------------------------
class CGruntzMultiCommand : public CGruntzCommand {
public:
    int m_4, m_8, m_c, m_10;
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand *Allocate();
};

// The per-class recycle lists + their non-empty gates (file-scope globals the
// allocators test/pull from). Reloc-masked; only the addresses are load-bearing.
extern int g_singleCmdCount;          // 0x62b5dc - non-empty gate
extern CGruntzCmdList g_singleCmdList; // 0x62b5d0 - the recycle list (ecx for RemoveTail)
extern int g_multiCmdCount;           // 0x62b64c
extern CGruntzCmdList g_multiCmdList;  // 0x62b640

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H
