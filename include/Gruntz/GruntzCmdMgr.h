// GruntzCmdMgr.h - CGruntzCmdMgr, the per-game command/target queue the game
// manager owns at CGruntzMgr+0x6c (BroadcastCmd at 0x093460 dispatches into it).
//
// SHAPE (recovered from the 11-method cluster + the engine CPtrList method set):
//   +0x00  base CPtrList   - the primary queue of game-object targets. Its
//                           m_nCount lives at +0x0c (the count the drain/iterate
//                           loops test). RemoveTail()/AddTail()/FindIndex()/
//                           RemoveAt()/RemoveAll() are the engine CPtrList methods
//                           (FLIRT-named; reloc-masked thiscall callees).
//   +0x1c  nested CPtrList - a second 0x1c-byte CPtrList (the +0x1c "selected
//                           subset" queue). Enqueue (0x023d10) adds to BOTH this
//                           and the base; its head is torn down in the dtor.
//   +0x38  m_38           - the CGruntzMgr singleton back-ptr (its m_curState's
//                           vtable slot +0x10 [CState::Update] reports the current
//                           game-state id: 0x11 / 0x3 / 0x4). The setter (0x0239d0)
//                           installs it (RezSync::Init passes `this`); ClearAndReset
//                           (0x0239f0) nulls it then drains.
//
// Non-polymorphic (no vptr of its own; the base CPtrList is not virtual). Field
// names are placeholders; only the OFFSETS + code bytes are load-bearing. The
// destructor (0x085bd0) carries a /GX EH frame (the inline CPtrList teardown is
// the destructible sub-object) - the home TU is built with flags="eh".
#ifndef GRUNTZ_GRUNTZCMDMGR_H
#define GRUNTZ_GRUNTZCMDMGR_H

#include <Mfc.h> // the REAL MFC CPtrList (GzObList IS CPtrList; see the note below)
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/GruntzCommand.h> // the queued command (the ex-GzTargetObj view)
// CState (the ex-GzStateProvider view) is pointer-only here - a fwd decl keeps this
// widely-included header off <Gruntz/State.h>'s decl chain (pulling it in cost
// CGruntzCmdMgr::RemoveMatchingTarget 100 -> 78 to the decl-count regalloc butterfly).
// The one TU that DISPATCHES on it (GruntzCmdMgr.cpp) includes State.h itself.
class CState;

// One CPtrList sub-object (0x1c bytes). Only the engine methods the cluster
// reaches are declared; all are reloc-masked thiscall callees in the engine
// CPtrList region (FLIRT-named). The body is never reproduced here.
//   m_4  head node (CPtrList m_pNodeHead)
//   m_8  tail node (CPtrList m_pNodeTail)
//   m_c  m_nCount (the queue length the loops test)
// GzObList IS the MFC CPtrList (<Mfc.h>) - every one of its methods was already annotated with
// the NAFXCW library rva: RemoveAll @0x1b48a6, ~CPtrList @0x1b48c6, AddTail @0x1b4991, RemoveTail
// @0x1b4a27, RemoveAt @0x1b4ac7, FindIndex @0x1b4afe. Declaring them on a class of OUR name
// mangled them as ?RemoveAll@GzObList@@QAEXXZ / ??1GzObList@@QAE@XZ / ... - symbols NAFXCW does
// NOT define (it defines ?RemoveAll@CPtrList@@QAEXXZ / ??1CObList@@UAE@XZ), i.e. 9 guaranteed
// `unresolved external symbol`s (assert_relocs --fake-targets). Aliasing the REAL class binds
// them all and is layout-identical (0x1c). The raw m_4/m_8/m_c fields the leaves walked are
// MFC-protected, so they now go through CPtrList's PUBLIC accessors GetHeadPosition()/GetCount(),
// which are _AFXCOLL_INLINE and lower to the identical single member load - not a call.
// (The same CTmObList->CPtrList / CFileIO->CFile fold.)
typedef CPtrList GzObList;

// A queued target object the loops walk (the CPtrList node payload). The drain/
// iterate loops read a type/key byte at +0x06, an index byte at +0x04, and a
// flags word at +0x0c, and dispatch the object's own vtable slots +0x24/+0x28.
// Modeled polymorphically so `obj->Select()` emits the thiscall virtual dispatch
// at the right slots (the 9 leading virtuals are placeholders fixing the offsets).
SIZE_UNKNOWN(GzTargetObj);
// (The GzTargetObj 11-slot view is GONE: it IS the canonical CGruntzCommand
// (<Gruntz/GruntzCommand.h>, RTTI vtbl@0x1e9674, 11 slots) - the queued command the
// cmd-mgr's list holds. Its Select/Deselect ARE that class's slots 9/10, which the RTTI
// map shows as __purecall in the base (leaf-provided), and its m_4/m_6/m_c are the
// canonical m_4 / m_6 / m_submitted at the same offsets.)
typedef CGruntzCommand GzTargetObj;

// (The GzStateProvider 5-slot view is GONE: its "GetStateId at slot 4 (+0x10)" IS
// CState::Update - the same slot 4 the state hierarchy uses to report its id, and
// ScanTargets compares it against 0x11, the PLAY id CGruntzMgr::PerFrameTick gates on.
// The GzMgr view is GONE too (2026-07-16): the +0x38 "state-providing manager" IS the
// CGruntzMgr singleton - RezSync::Init (a CGruntzMgr method by its own dossier) passes
// `this` to SetMgr, and GzMgr's m_2c state slot IS CGruntzMgr::m_curState (+0x2c).)
class CGruntzMgr; // consumers that deref m_38 include <Gruntz/GruntzMgr.h>

// The serialization stream the dispatcher reads/writes the command queue through is
// the shared WAP32 CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), now the one
// modeled class in <Gruntz/SerialArchive.h> - the former local `GzStream` view is
// folded away.

// (The GzSerCmd 7-slot serialize lens is GONE too (Fable A2, 2026-07-14): it was
// the SAME CGruntzCommand - Serialize IS slot 1 and GetTag slot 6 (+0x18) of the
// canonical 11-slot table. The fork existed only because the canonical declared
// GetTag as char while the queue serializer's retail bytes need the int-family
// return (`and eax,0xff`, no extension insn); GetTag is now i32 on the canonical.)
typedef CGruntzCommand GzSerCmd;

// One node of the base CPtrList the write pass walks: next@+0x00, payload@+0x08.
SIZE_UNKNOWN(GzCmdNode);
struct GzCmdNode {
    GzCmdNode* m_0; // +0x00  next node
    char m_pad4[4];
    CGruntzCommand* m_8; // +0x08  payload command
};

SIZE_UNKNOWN(CGruntzCmdMgr);
class CGruntzCmdMgr {
public:
    void
    Spawn(i32 a1, char area, i32 a3, i32 a4, i32 px, i32 py, i32 a7, i32 a8); // reloc-masked (fold)
    // 0x023b40 - find the first base-queue target matching (indexByte, typeByte)
    // and remove+deselect it.
    void RemoveMatchingTarget(char indexByte, char typeByte);
    // 0x0239d0 - install the manager pointer; returns 1. Out-of-line.
    i32 SetMgr(CGruntzMgr* mgr); // 0x0239d0
    // 0x0239f0 - null the manager then drain everything (tail-calls Clear). Out-of-line.
    void ClearAndReset(); // 0x0239f0
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
    // 0x023d90 - snap the cursor rect to the 0x20 tile grid and dispatch the
    // command-target tile-marker blit (thunk 0x2095). Body in GruntzCmdMgr.cpp
    // (ex `CObj23d90::Blit`, a placeholder view of THIS class: its receiver is
    // [CGruntzMgr+0x6c] == m_cmdSubMgr, and its m_38 chain IS this class's m_38
    // manager back-ptr - m_38->m_world->m_24->m_planeCtx/m_mainPlane).
    void BlitTileMarker(i32 a1, i32 a2, i32 x, i32 y, i32 a5); // 0x023d90
    // 0x085bd0 - the /GX destructor.
    ~CGruntzCmdMgr();

    GzObList m_base; // +0x00  primary target queue
    GzObList m_1c;   // +0x1c  nested subset queue
    CGruntzMgr* m_38; // +0x38  the game-manager singleton (RezSync::Init self-registers)
};

// --- vtable catalog ---

#endif // GRUNTZ_GRUNTZCMDMGR_H
