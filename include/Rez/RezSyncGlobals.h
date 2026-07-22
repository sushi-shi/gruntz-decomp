// RezSyncGlobals.h - RezSync.cpp's private decls for two singletons it wires up
// during game init (CGruntzMgr::Run Phase 12). Included ONLY by RezSync.cpp.
//
// @identity-TODO: both are MIS-MODELED here and carry a second (real) view elsewhere:
//   g_inputMgr    real ?g_inputMgr@@3PAVDirectInputMgr2@@A  (DirectInputMgr2*, 0x245570)
//   g_spawnConfig real _g_spawnConfig  (0x245578; the 0x28 object RezSync allocs and
//                 Init's with g_inputMgr -> it is StateMgrBZ, NOT CGruntSpawnConfig;
//                 StateMgrBZ::Init(DirectInputMgr2*,i32) @0x382c0).
// Re-model Phase 12 onto StateMgrBZ/DirectInputMgr2 (a matcher change to the 66%%
// CGruntzMgr::Run) to delete these views - tracked by the single-view ratchet.
#ifndef GRUNTZ_REZ_REZSYNCGLOBALS_H
#define GRUNTZ_REZ_REZSYNCGLOBALS_H

class CGruntSpawnConfig;
extern "C" void* g_inputMgr;
extern "C" CGruntSpawnConfig* g_spawnConfig;

#endif // GRUNTZ_REZ_REZSYNCGLOBALS_H
