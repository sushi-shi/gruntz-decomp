// RezSyncViews.h - RezSync.cpp's PRIVATE split-views of the reset-manager
// singletons (0x245570 / 0x245578). RezSync sees them at the types it allocates/
// uses (void* / CGruntSpawnConfig*); the canonical DirectInputMgr2* / StateMgrBZ*
// decls live in <DinMgr2/InputMgrPtr.h> / <Gruntz/Play.h>. This header is included
// ONLY by RezSync.cpp so the two typings never collide in one TU. @identity-TODO
// unify onto the canonical classes.
#ifndef GRUNTZ_REZ_REZSYNCVIEWS_H
#define GRUNTZ_REZ_REZSYNCVIEWS_H

class CGruntSpawnConfig;
extern "C" void* g_inputMgr;
extern "C" CGruntSpawnConfig* g_spawnConfig;

#endif // GRUNTZ_REZ_REZSYNCVIEWS_H
