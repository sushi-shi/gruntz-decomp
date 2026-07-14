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
    // (The former STATE_SUBMGR=1 was a fiction: 0x157790's only reference is
    // ??_7CDDrawWorkerCache+0x18 - it is the cache's slot-6 IsReady `return 1`
    // copy, not a state id. Enumerate only PROVEN ids.)
    STATE_ANIMWORKER = 0x9,  // AnimWorkerObj::GetStateId         @0x151d70 (vtbl 0x1efb80[8])
    STATE_SUBMGRPAGES = 0xf, // CDDrawSubMgrPages::GetStateId     @0x1574a0
    // 0x10 was mislabeled STATE_WORKERMAPSMALL: 0x157600's ONLY reference in the
    // whole binary is ??_7CDDrawChildGroup@@6B@+0x20 (slot 8) - it is the child
    // group's id. CDDrawWorkerMapSmall's real slot 8 is 0x156cf0 (`mov eax,0x14`).
    STATE_CHILDGROUP = 0x10,     // CDDrawChildGroup::GetStateId      @0x157600
    STATE_WORKERLIST = 0x11,     // CDDrawWorkerList::GetStateId      @0x156f20
    STATE_WORKERREGISTRY = 0x12, // CDDrawWorkerRegistry::GetStateId  @0x156de0
    STATE_WORKERCACHE = 0x13,    // CDDrawWorkerCache::GetStateId     @0x1576f0
    STATE_WORKERMAPSMALL = 0x14, // CDDrawWorkerMapSmall::GetStateId  @0x156cf0 (vtbl slot 8)
};

#endif // GRUNTZ_STATEID_H
