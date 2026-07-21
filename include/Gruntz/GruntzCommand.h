#ifndef SRC_GRUNTZ_GRUNTZCOMMAND_H
#define SRC_GRUNTZ_GRUNTZCOMMAND_H

#include <Mfc.h> // the REAL MFC CPtrList (CGruntzCmdList IS CPtrList; see the note below)
#include <rva.h>
#include <Ints.h>

typedef u32 gz_size_t;
void* operator new(gz_size_t);

typedef CPtrList CGruntzCmdList;

class CPlay;

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
    // m_targetIndex/m_targetType are UNSIGNED: CGruntzCmdMgr::RemoveMatchingTarget (0x23b00) compares them
    // against (u8)-cast params and retail emits the ZERO-extending `movzx` - a signed
    // `char` here makes cl emit `movsx` and drops that fn 100 -> 78.47 (the ex-GzTargetObj
    // view, which matched 100%, declared them u8; the binary is the arbiter).
    u8 m_targetIndex; // +0x04  per-team index byte
    char m_5;         // +0x05
    u8 m_targetType;  // +0x06  type/key byte
    char m_7;         // +0x07 (unused by this cluster)
    i16 m_8;          // +0x08
    i16 m_a;          // +0x0a
    i32 m_submitted;  // +0x0c  submit-context latch (serialized by Save/Load): net
                      //         parse path sets =1, the cmd-mgr sets =2 (playing) /
                      //         =4 (ready) by game state before enqueue
    char m_10;        // +0x10
    char m_11;        // +0x11
    i16 m_12;         // +0x12 (pad -> 0x14)

    virtual ~CGruntzCommand() {} // slot 0 (the non-deleting dtor; trivial -> vptr stamp only)
    // The `??_G` scalar-deleting destructor (vtable slot 0 @0x24330): run the trivial
    // ~CGruntzCommand (inlined vptr stamp), conditionally RezFree, return this. Modeled
    // as a hand-written non-virtual method pinned by RVA (the CFileImageSurface pattern).
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
    // slot 6 - the command's type/tag word (the tag byte, int-typed: the queue
    // serializer's retail bytes are `call [vptr+0x18]; and eax,0xff` with NO
    // sign-extension insn, so the compiler saw an int-family return; Pack stores
    // its low byte first). Was declared char - which forked the GzSerCmd view.
    virtual i32 GetTag();
    // slot 7 - parse a flat command buffer in place; the leaves (Single/Multi)
    // override it with real parsers, the base anchor is a no-op. Consumed-byte
    // count returned. (Recovered from NetCmdSlot.cpp's ProcessCmd dispatch.)
    virtual i32 Parse(void* data, i32 len);
    // slot 8 - pack this command into a flat byte buffer; __purecall in the base, so
    // each leaf provides the body. It was declared `Vfunc8()` (0-arg) purely as a
    // placeholder guess: the leaves' bodies (0x24050/0x240d0) end in `ret 8`, so the
    // slot is a 2-ARG __thiscall, and they read the buffer from the first stack arg and
    // return the byte count. Named from that proven role - a CHOICE, not a reading.
    virtual i32 Pack(char* buf, i32 unused); // slot 8 (+0x20) __purecall in the base
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
                        u8* buf); // 0x023ed0
    i32 ApplyOne(CPlay* p);       // 0x024140
    i32 ApplyMask(CPlay* p);      // 0x024190

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

extern const u16 g_cmdBitTable[16]; // 0x1e9608

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
    virtual i32 GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;
    // 0x024050 (slot 8) - pack this command into a flat byte buffer: tag (GetTag), then
    // the five scalar params, then m_10, and conditionally m_11 (when m_5 >= 8). Returns
    // the number of bytes written. WAS declared twice: as the `Vfunc8` placeholder here
    // and as a non-virtual `Pack` below - one body, two names.
    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual void Select(CState* state) OVERRIDE; // slot 9  (base is __purecall)
    virtual void Deselect() OVERRIDE;            // slot 10 (base is __purecall)
    CGruntzSingleCommand() {}                    // inline empty ctor (vftable store only)
    static CGruntzSingleCommand* Allocate();
    static void FreeAll(); // 0x024450 - drain g_singleCmdList, delete each node
};

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
    virtual i32 GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;
    // 0x0240d0 (slot 8) - pack this command into a flat byte buffer: tag (GetTag), the
    // five scalar params, then the +0x10 16-bit flag mask as a WORD. Returns the number
    // of bytes written. Same one-body-two-names shadow as the Single twin.
    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual void Select(CState* state) OVERRIDE; // slot 9  (base is __purecall)
    virtual void Deselect() OVERRIDE;            // slot 10 (base is __purecall)
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void FreeAll(); // 0x024490 - drain g_multiCmdList, delete each node
};

extern i32 g_singleCmdCount;           // 0x62b5dc - non-empty gate
extern CGruntzCmdList g_singleCmdList; // 0x62b5d0 - the recycle list (ecx for RemoveTail)
extern i32 g_multiCmdCount;            // 0x62b64c
extern CGruntzCmdList g_multiCmdList;  // 0x62b640

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H

VTBL(CGruntzCommand, 0x001e9674);
