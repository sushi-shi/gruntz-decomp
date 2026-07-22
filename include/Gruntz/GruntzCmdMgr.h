#ifndef GRUNTZ_GRUNTZCMDMGR_H
#define GRUNTZ_GRUNTZCMDMGR_H

#include <Mfc.h> // the REAL MFC CPtrList (CPtrList IS CPtrList; see the note below)
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/GruntzCommand.h> // the queued command (the ex-CGruntzCommand view)
class CState;

SIZE_UNKNOWN();

class CGruntzMgr; // consumers that deref m_38 include <Gruntz/GruntzMgr.h>

struct GzCmdNode {
    GzCmdNode* m_0; // +0x00  next node
    char m_pad4[4];
    CGruntzCommand* m_8; // +0x08  payload command
};
SIZE_UNKNOWN();

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
    i32 Serialize(CFileMemBase* stream, i32 mode, i32 a3, i32 a4);
    // 0x024a90 - predicate: is the registry's multiplayer slot active?
    i32 IsActive(i32 enable);
    // 0x023d90 - snap the cursor rect to the 0x20 tile grid and dispatch the
    // command-target tile-marker blit (thunk 0x2095). Body in GruntzCmdMgr.cpp
    // (ex `CObj23d90::Blit`, a placeholder view of THIS class: its receiver is
    // [CGruntzMgr+0x6c] == m_cmdSubMgr, and its m_38 chain IS this class's m_38
    // manager back-ptr - m_38->m_world->m_level->m_planeCtx/m_mainPlane).
    void BlitTileMarker(i32 a1, i32 a2, i32 x, i32 y, i32 a5); // 0x023d90
    // 0x085bd0 - the /GX destructor.
    ~CGruntzCmdMgr();
    // Per-frame poke CMulti::PumpA drives with (m_curSlotId % 128); role unrecovered
    // (reloc-masked external; ex CMultiLogicList::Step20b3 - that view is dissolved).
    void Step20b3(i32 v);

    CPtrList m_base;  // +0x00  primary target queue
    CPtrList m_1c;    // +0x1c  nested subset queue
    CGruntzMgr* m_38; // +0x38  the game-manager singleton (RezSync::Init self-registers)
};
SIZE_UNKNOWN();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern CPtrList g_singleCmdList;
extern CPtrList g_multiCmdList;

#endif // GRUNTZ_GRUNTZCMDMGR_H
