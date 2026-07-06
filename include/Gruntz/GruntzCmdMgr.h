// GruntzCmdMgr.h - CGruntzCmdMgr, the per-game command/target queue the game
// manager owns at CGruntzMgr+0x6c (BroadcastCmd at 0x093460 dispatches into it).
//
// SHAPE (recovered from the 11-method cluster + the engine CObList method set):
//   +0x00  base CObList   - the primary queue of game-object targets. Its
//                           m_nCount lives at +0x0c (the count the drain/iterate
//                           loops test). RemoveTail()/AddTail()/FindIndex()/
//                           RemoveAt()/RemoveAll() are the engine CObList methods
//                           (FLIRT-named; reloc-masked thiscall callees).
//   +0x1c  nested CObList - a second 0x1c-byte CObList (the +0x1c "selected
//                           subset" queue). Enqueue (0x023d10) adds to BOTH this
//                           and the base; its head is torn down in the dtor.
//   +0x38  m_38           - a manager pointer (a CGameMgr-family object whose
//                           +0x2c sub-object's vtable slot +0x10 reports the
//                           current game-state id: 0x11 / 0x3 / 0x4). The setter
//                           (0x0239d0) installs it; ClearAndReset (0x0239f0)
//                           nulls it then drains.
//
// Non-polymorphic (no vptr of its own; the base CObList is not virtual). Field
// names are placeholders; only the OFFSETS + code bytes are load-bearing. The
// destructor (0x085bd0) carries a /GX EH frame (the inline CObList teardown is
// the destructible sub-object) - the home TU is built with flags="eh".
#ifndef GRUNTZ_GRUNTZCMDMGR_H
#define GRUNTZ_GRUNTZCMDMGR_H

#include <rva.h>
#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

// One CObList sub-object (0x1c bytes). Only the engine methods the cluster
// reaches are declared; all are reloc-masked thiscall callees in the engine
// CObList region (FLIRT-named). The body is never reproduced here.
//   m_4  head node (CObList m_pNodeHead)
//   m_8  tail node (CObList m_pNodeTail)
//   m_c  m_nCount (the queue length the loops test)
SIZE_UNKNOWN(GzObList);
struct GzObList {
    char m_pad0[4]; // +0x00
    void* m_4;      // +0x04  head node
    void* m_8;      // +0x08  tail node
    i32 m_c;        // +0x0c  node count
    char m_pad10[0x1c - 0x10];

    ~GzObList();                // 0x1b48c6  ~CObList (auto member teardown)
    void RemoveAll();           // 0x1b48a6
    void AddTail(void* node);   // 0x1b4991
    void* RemoveTail();         // 0x1b4a27  (CGruntzCmdList::RemoveTail)
    void* FindIndex(i32 index); // 0x1b4afe  -> POSITION (a node*)
    void RemoveAt(void* pos);   // 0x1b4ac7
};

class GzStateProvider; // defined below; Select() takes the +0x38 state sub-object

// A queued target object the loops walk (the CObList node payload). The drain/
// iterate loops read a type/key byte at +0x06, an index byte at +0x04, and a
// flags word at +0x0c, and dispatch the object's own vtable slots +0x24/+0x28.
// Modeled polymorphically so `obj->Select()` emits the thiscall virtual dispatch
// at the right slots (the 9 leading virtuals are placeholders fixing the offsets).
SIZE_UNKNOWN(GzTargetObj);
class GzTargetObj {
public:
    virtual void Slot0();                      // +0x00
    virtual void Slot1();                      // +0x04
    virtual void Slot2();                      // +0x08
    virtual void Slot3();                      // +0x0c
    virtual void Slot4();                      // +0x10
    virtual void Slot5();                      // +0x14
    virtual void Slot6();                      // +0x18
    virtual void Slot7();                      // +0x1c
    virtual void Slot8();                      // +0x20
    virtual void Select(GzStateProvider* mgr); // +0x24 (slot 9)
    virtual void Deselect();                   // +0x28 (slot 10)

    u8 m_4; // +0x04  per-team index byte
    char m_pad5[0x6 - 0x5];
    u8 m_6; // +0x06  type/key byte (matched against the param)
    char m_pad7[0xc - 0x7];
    i32 m_c; // +0x0c  flags (bit 0x1 / 0x2)
};

// The +0x38 manager pointer's state sub-object: ->GetStateId() (vtable slot +0x10)
// reports the current game-state id (compared against 0x11 / 0x3 / 0x4). Modeled
// polymorphically; the 4 leading virtuals fix GetStateId at slot 4 (+0x10).
SIZE_UNKNOWN(GzStateProvider);
class GzStateProvider {
public:
    virtual void Slot0();     // +0x00
    virtual void Slot1();     // +0x04
    virtual void Slot2();     // +0x08
    virtual void Slot3();     // +0x0c
    virtual i32 GetStateId(); // +0x10 (slot 4)
};
SIZE_UNKNOWN(GzMgr);
struct GzMgr {
    char m_pad0[0x2c];
    GzStateProvider* m_2c; // +0x2c  state provider
};

// The serialization stream the dispatcher reads/writes the command queue through is
// the shared WAP32 CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), now the one
// modeled class in <Gruntz/SerialArchive.h> - the former local `GzStream` view is
// folded away.

// A queued command as the dispatcher serializes it: vtable slot +0x04 (index 1) =
// Serialize(stream, mode, a, b) -> i32; slot +0x18 (index 6) = GetTag() -> i32.
// Modeled independently of CGruntzCommand so this TU's serialize dispatch does not
// disturb the matched CGruntzCommand vtable.
SIZE_UNKNOWN(GzSerCmd);
class GzSerCmd {
public:
    virtual void Slot0();
    virtual i32 Serialize(CSerialArchive* s, i32 mode, i32 a, i32 b); // +0x04 (index 1)
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual i32 GetTag(); // +0x18 (index 6)
};

// One node of the base CObList the write pass walks: next@+0x00, payload@+0x08.
SIZE_UNKNOWN(GzCmdNode);
struct GzCmdNode {
    GzCmdNode* m_0; // +0x00  next node
    char m_pad4[4];
    GzSerCmd* m_8; // +0x08  payload command
};

SIZE_UNKNOWN(CGruntzCmdMgr);
class CGruntzCmdMgr {
public:
    void
    Spawn(i32 a1, char area, i32 a3, i32 a4, i32 px, i32 py, i32 a7, i32 a8); // reloc-masked (fold)
    // 0x023b40 - find the first base-queue target matching (indexByte, typeByte)
    // and remove+deselect it.
    void RemoveMatchingTarget(char indexByte, char typeByte);
    // 0x0239d0 - install the manager pointer; returns 1.
    i32 SetMgr(GzMgr* mgr);
    // 0x0239f0 - null the manager then drain everything (tail-calls Clear).
    void ClearAndReset();
    // 0x023a10 - the state-filtered target scan/select pass.
    i32 ScanTargets(i32 param);
    // 0x023bc0 - drain the base queue, deselecting each node.
    void DrainBase();
    // 0x023c00 - full reset: drain base + RemoveAll the +0x1c list + free both
    //            command recycle lists.
    void Clear();
    // 0x023c30 - build a single-target command + enqueue it. p1 is the enqueue
    // flag; p2..p8 are the command's scalar params (forwarded permuted).
    void Report1(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g); // 0x90db8 reloc-masked (fold)
    void EnqueueSingle(i32 p1, char p2, char p3, char p4, i16 p5, i16 p6, char p7, char p8);
    // 0x023ca0 - build a multi-target (mask) command + enqueue it.
    void EnqueueMulti(i32 p1, char p2, i32 p3, u8* p4, char p5, i16 p6, i16 p7, char p8);
    // 0x023d10 - enqueue a built command onto the two queues, tagging its
    //            phase from the manager state. flag != 0 -> also AddTail onto
    //            the nested +0x1c list (with the phase tag).
    void EnqueueCommand(i32 flag, void* cmd);
    // 0x423b40 (via ILT 0x2a63) - re-dispatch a queued command by sequence during
    // the net resend pass (CNetMgr::ResetPlayerCommands: m_4->m_6c->Dispatch).
    // External/no-body here (reloc-masked thiscall).
    void Dispatch(i32 cmdHead, i32 seq);
    // 0x024890 - the command-queue (de)serializer. mode 4 = write the queue to
    // the stream; mode 7 = read it back, rebuilding the queue.
    i32 Serialize(CSerialArchive* stream, i32 mode, i32 a3, i32 a4);
    // 0x024a90 - predicate: is the registry's multiplayer slot active?
    i32 IsActive(i32 enable);
    // 0x085bd0 - the /GX destructor.
    ~CGruntzCmdMgr();

    GzObList m_base; // +0x00  primary target queue
    GzObList m_1c;   // +0x1c  nested subset queue
    GzMgr* m_38;     // +0x38  state-providing manager
};

// --- vtable catalog ---
VTBL(GzSerCmd, 0x001ea42c);
VTBL(GzStateProvider, 0x001ea42c);
VTBL(GzTargetObj, 0x001ea42c);

#endif // GRUNTZ_GRUNTZCMDMGR_H
