// StateId.h - the DDraw-manager "state id" tag space returned by every
// CDDrawSubMgr / CDDrawWorker leaf's GetStateId() virtual (vtable slot 8, the
// 6-byte `mov eax,<id>; ret` accessor). Each concrete manager/worker leaf returns
// a UNIQUE constant identifying its state kind; the engine's DDraw dispatch keys
// off it (e.g. GruntzCmdMgr compares against 0x11 for the "worker-list" state).
//
// Named here so every GetStateId body returns the named enumerator, not a bare
// literal (matching-NEUTRAL: an MSVC5 enum is int-width, so a named enumerator
// lowers to the exact same immediate and each `mov eax,<id>; ret` stays
// byte-identical). Mirrors LogicTypeId in LogicTypeId.h.
//
// Field/state names are placeholders keyed off the owning class; only the id
// itself is the recovered fact.
#ifndef GRUNTZ_STATEID_H
#define GRUNTZ_STATEID_H

enum StateId {
    STATE_SUBMGR = 0x1,          // CDDrawSubMgr::GetStateId          @0x157790
    STATE_SUBMGRPAGES = 0xf,     // CDDrawSubMgrPages::GetStateId     @0x1574a0
    STATE_WORKERMAPSMALL = 0x10, // CDDrawWorkerMapSmall::GetStateId  @0x157600
    STATE_WORKERLIST = 0x11,     // CDDrawWorkerList::GetStateId      @0x156f20
    STATE_WORKERREGISTRY = 0x12, // CDDrawWorkerRegistry::GetStateId  @0x156de0
    STATE_WORKERCACHE = 0x13,    // CDDrawWorkerCache::GetStateId     @0x1576f0
};

#endif // GRUNTZ_STATEID_H
