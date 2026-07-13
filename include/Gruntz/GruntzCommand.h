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

#include <Mfc.h> // the REAL MFC CPtrList (CGruntzCmdList IS CPtrList; see the note below)
#include <rva.h>
#include <Ints.h>

typedef u32 gz_size_t;
void* operator new(gz_size_t);

// The recycle-list type each leaf allocator pulls from (a WAP32 CPtrList-family
// list). Only RemoveTail() is reached (a __thiscall returning the recycled
// node). The leaf allocator: if the list is non-empty, tail-call RemoveTail();
// else operator new(0x14) the object + stamp its vftable.
// CGruntzCmdList IS the MFC CPtrList too: its RemoveTail is ?RemoveTail@CPtrList@@QAEPAVCObject@@XZ
// @0x1b4a27 (NAFXCW). Declaring it on our own name emitted ?RemoveTail@CGruntzCmdList@@QAEPAXXZ,
// which nothing defines - 4 unresolved externals. Alias the real class (see GruntzCmdMgr.h).
typedef CPtrList CGruntzCmdList;

// The "apply" target each Apply*() passes the unpacked command params to (the
// big CPlay command-executor at 0x0d1b60, ret 0x1c => 7 __thiscall args). It is
// external to this TU (reloc-masked); modeled here as a method on a tiny opaque
// helper so `mov ecx,p; push args...; call` falls out with no stack cleanup.
SIZE_UNKNOWN(CGruntzCmdTarget);
struct CGruntzCmdTarget {
    i32 Exec(char kind, char index, char a2, i16 a3, i16 a4, char a5, char a6);
};

// The network (de)serialization stream the base command Save/Load drive: the shared
// WAP32 archive interface (Read @+0x2c / Write @+0x30), forward-declared here.
// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

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
SIZE(CGruntzCommand, 0x14);
class CState; // the live game state (Select's arg; slot-4 Update() reports its id)

class CGruntzCommand {
public:
    // m_4/m_6 are UNSIGNED: CGruntzCmdMgr::RemoveMatchingTarget (0x23b00) compares them
    // against (u8)-cast params and retail emits the ZERO-extending `movzx` - a signed
    // `char` here makes cl emit `movsx` and drops that fn 100 -> 78.47 (the ex-GzTargetObj
    // view, which matched 100%, declared them u8; the binary is the arbiter).
    u8 m_4;          // +0x04  per-team index byte
    char m_5;        // +0x05
    u8 m_6;          // +0x06  type/key byte
    char m_7;        // +0x07 (unused by this cluster)
    i16 m_8;         // +0x08
    i16 m_a;         // +0x0a
    i32 m_submitted; // +0x0c  submit-context latch (serialized by Save/Load): net
                     //         parse path sets =1, the cmd-mgr sets =2 (playing) /
                     //         =4 (ready) by game state before enqueue
    char m_10;       // +0x10
    char m_11;       // +0x11
    i16 m_12;        // +0x12 (pad -> 0x14)

    virtual ~CGruntzCommand() {} // slot 0 (the non-deleting dtor; trivial -> vptr stamp only)
    // The `??_G` scalar-deleting destructor (vtable slot 0 @0x24330): run the trivial
    // ~CGruntzCommand (inlined vptr stamp), conditionally RezFree, return this. Modeled
    // as a hand-written non-virtual method pinned by RVA (the CFileImageSurface pattern).
    void* ScalarDtor(u32 flags); // 0x24330
    // slot 1 - the (de)serialize dispatcher: on mode 4 call Save (slot 2), on
    // mode 7 call Load (slot 3), both through the vtable. The leaves override it
    // (Single 0x0244d0 / Multi 0x0246c0); the base anchor returns 1.
    virtual i32 Serialize(CSerialArchive* s, i32 mode, i32 a3, i32 a4);
    // slot 2/3 - the network (de)serializers (via stream Write +0x30 / Read +0x2c):
    // pure in the binary (__purecall), each leaf provides its body. Single writes the
    // +0x10 field as the m_10/m_11 byte pair (0x024520/0x0245f0); Multi as one 16-bit
    // unit (0x024710/0x0247d0). The base anchors are placeholders for vtable emission.
    virtual i32 Save(CSerialArchive* s); // slot 2
    virtual i32 Load(CSerialArchive* s); // slot 3
    // slot 4 (+0x10) - the base "set params" implementation (0x023e20): store
    // the five scalar params; returns 1. Inherited unchanged by both leaves.
    virtual i32 SetParams(char a0, char a1, char a2, i16 a3, i16 a4);
    virtual i32 Vslot05(); // slot 5 (+0x14) - base returns 1 (role unrecovered: P2), 0x24310
    virtual char GetTag(); // slot 6 - the command's type/tag byte (Pack writes it first)
    // slot 7 - parse a flat command buffer in place; the leaves (Single/Multi)
    // override it with real parsers, the base anchor is a no-op. Consumed-byte
    // count returned. (Recovered from NetCmdSlot.cpp's ProcessCmd dispatch.)
    virtual i32 Parse(void* data, i32 len);
    virtual i32 Vfunc8(); // slot 8
    // slots 9/10 - __purecall in the base (RTTI: 11 slots, [9] and [10] both
    // __purecall @0x11fec0), so each leaf provides the body. CGruntzCmdMgr::ScanTargets
    // (0x23a10) dispatches them back-to-back on every queued command: Select is handed
    // the live game STATE (CState*, whose slot-4 Update() the same fn reads for the
    // 0x11 PLAY id), then Deselect retires it. (The ex-GzTargetObj view - 9 filler slots
    // + these two - was this class; its m_4/m_6/m_c are m_4 / m_6 / m_submitted.)
    virtual void Select(CState* state); // slot 9  (+0x24)  __purecall in the base
    virtual void Deselect();            // slot 10 (+0x28)  __purecall in the base

    // Non-virtual members of the base (called directly, not via the vtable):
    i32 SetParamsEx(char a0, char a1, char a2, i16 a3, i16 a4, char a5, char a6); // 0x023e60
    i32 SetMaskFromList(char a0, char a1, char a2, i16 a3, i16 a4, i32 count,
                        u8* buf);       // 0x023ed0
    i32 ApplyOne(CGruntzCmdTarget* p);  // 0x024140
    i32 ApplyMask(CGruntzCmdTarget* p); // 0x024190

    // Two out-of-line base-vftable stamps (0x0242f0 / 0x024430): each is a bare
    // `mov [this],&??_7CGruntzCommand; ret` (void __thiscall, no eax-return, no
    // null guard). Byte-for-byte the non-deleting base dtor body (??1CGruntzCommand
    // = `mov [ecx],&vtbl; ret`, verified), materialized twice: retail's
    // CGruntzSingleCommand/CGruntzMultiCommand scalar-deleting dtors (??_G @0x242c0/
    // 0x24400) each CALL a distinct out-of-line copy of the base-vtable restore. A
    // plain empty derived dtor emits 11 B (own-vtable stamp + jmp base); the retail
    // copies stamp ONLY the base vtable, so they are modeled as two void methods
    // whose body is an explicit inline base-dtor call (emits the exact 7-B stamp).
    // Defined out-of-line in the .cpp (an unreferenced inline member would not emit).
    void CGruntzCommand_0242f0();
    void CGruntzCommand_024430();
};

// The 16-entry 1<<i bit table (0x5e9608; VA) the mask loop indexes/scans.
extern const u16 g_cmdBitTable[16]; // 0x1e9608

// ---------------------------------------------------------------------------
// CGruntzSingleCommand - single-target command (0x14 bytes; vtable 0x5e9634).
// Allocate() is the new-or-recycle allocator: pull a recycled node from the
// per-class free list if non-empty, else operator new(0x14) + inline ctor
// (the ctor just stamps the leaf vftable - empty body, so it folds inline).
// ---------------------------------------------------------------------------
SIZE(CGruntzSingleCommand, 0x14);
VTBL(CGruntzSingleCommand, 0x001e9634); // vtable_names -> code (RTTI game class)
class CGruntzSingleCommand : public CGruntzCommand {
public:
    virtual ~CGruntzSingleCommand() OVERRIDE;
    // slots 1/2/3 (0x0244d0 / 0x024520 / 0x0245f0) - the serialize dispatcher and
    // the single-target network Save/Load (the base declares them pure; the leaves
    // provide the bodies).
    virtual i32 Serialize(CSerialArchive* s, i32 mode, i32 a3, i32 a4) OVERRIDE;
    virtual i32 Save(CSerialArchive* s) OVERRIDE;
    virtual i32 Load(CSerialArchive* s) OVERRIDE;
    virtual i32 Vslot05() OVERRIDE; // 0x24260
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;
    virtual i32 Vfunc8() OVERRIDE;
    virtual void Select(CState* state) OVERRIDE; // slot 9  (base is __purecall)
    virtual void Deselect() OVERRIDE;            // slot 10 (base is __purecall)
    CGruntzSingleCommand() {}                    // inline empty ctor (vftable store only)
    static CGruntzSingleCommand* Allocate();
    static void FreeAll(); // 0x024450 - drain g_singleCmdList, delete each node
    // 0x024050 - pack this command into a flat byte buffer: tag (GetTag), then the
    // five scalar params, then m_10, and conditionally m_11 (when m_5 >= 8). Returns
    // the number of bytes written.
    i32 Pack(char* buf, i32 unused);
};

// ---------------------------------------------------------------------------
// CGruntzMultiCommand - multi-target command (0x14 bytes; vtable 0x5e96b4).
// Same new-or-recycle allocator shape as the single-target command.
// ---------------------------------------------------------------------------
SIZE(CGruntzMultiCommand, 0x14);
VTBL(CGruntzMultiCommand, 0x001e96b4); // vtable_names -> code (RTTI game class)
class CGruntzMultiCommand : public CGruntzCommand {
public:
    virtual ~CGruntzMultiCommand() OVERRIDE;
    // slots 1/2/3 (0x0246c0 / 0x024710 / 0x0247d0) - the multi-target overrides of
    // the serialize dispatcher and the network Save/Load (the +0x10 flag word as a
    // single 16-bit unit vs the base's m_10/m_11 byte pair).
    virtual i32 Serialize(CSerialArchive* s, i32 mode, i32 a3, i32 a4) OVERRIDE;
    virtual i32 Save(CSerialArchive* s) OVERRIDE;
    virtual i32 Load(CSerialArchive* s) OVERRIDE;
    virtual i32 Vslot05() OVERRIDE; // 0x243a0
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;
    virtual i32 Vfunc8() OVERRIDE;
    virtual void Select(CState* state) OVERRIDE; // slot 9  (base is __purecall)
    virtual void Deselect() OVERRIDE;            // slot 10 (base is __purecall)
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void FreeAll(); // 0x024490 - drain g_multiCmdList, delete each node
    // 0x0240d0 - pack this command into a flat byte buffer: tag (GetTag), the five
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

VTBL(CGruntzCommand, 0x001e9674);
