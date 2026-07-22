#ifndef GRUNTZ_STATEID_H
#define GRUNTZ_STATEID_H

// NB: this "StateId" space IS the CLoadable class-id space (slot 8 is GetClassId
// family-wide; CDDrawWorkerRegistry's 0x12 and CDDrawWorkerList's 0x11 already
// live in <Gruntz/Loadable.h>'s LoadableClassId). The ids below belong to the
// holdout classes not yet re-based onto CLoadable - migrate each id there as its
// class flips.
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
    // (0x11 = CLASSID_WORKERLIST and 0x12 = STATE_WORKERREGISTRY's class are
    // CLoadable-rebased - see <Gruntz/Loadable.h>.)
    STATE_WORKERREGISTRY = 0x12, // CDDrawWorkerRegistry::GetClassId  @0x156de0
    STATE_WORKERCACHE = 0x13,    // CDDrawWorkerCache::GetStateId     @0x1576f0
    STATE_WORKERMAPSMALL = 0x14, // CDDrawWorkerMapSmall::GetStateId  @0x156cf0 (vtbl slot 8)
};

#endif // GRUNTZ_STATEID_H
