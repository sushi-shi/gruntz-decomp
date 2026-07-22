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
    // (0xf = CLASSID_SUBMGRPAGES - CLoadable-rebased, see <Gruntz/Loadable.h>.)
    // 0x10 was mislabeled STATE_WORKERMAPSMALL: 0x157600's ONLY reference in the
    // whole binary is ??_7CDDrawChildGroup@@6B@+0x20 (slot 8) - it is the child
    // group's id. CDDrawWorkerMapSmall's real slot 8 is 0x156cf0 (`mov eax,0x14`).
    // (0x10 = CLASSID_CHILDGROUP - CLoadable-rebased, see <Gruntz/Loadable.h>.)
    // (0x11 = CLASSID_WORKERLIST and 0x12 = STATE_WORKERREGISTRY's class are
    // CLoadable-rebased - see <Gruntz/Loadable.h>.)
    // (0x12 = CLASSID_WORKERREGISTRY - CLoadable-rebased, see <Gruntz/Loadable.h>.)
    // Only 0x9 remains: AnimWorkerObj (vtbl 0x1efb80 slot 8) - the next flip
    // candidate; StateId.h dies when it lands.
    // (0x13 = CLASSID_WORKERCACHE - CLoadable-rebased, see <Gruntz/Loadable.h>.)
    // (0x14 = CLASSID_WORKERMAPSMALL - CLoadable-rebased, see <Gruntz/Loadable.h>.)
};

#endif // GRUNTZ_STATEID_H
