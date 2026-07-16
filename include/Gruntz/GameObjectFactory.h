// GameObjectFactory.h - the game-object type-registry seeding entry (owner:
// src/Gruntz/GameObjectFactory.cpp). CGruntzMgr::Run calls it at boot (via ILT
// thunk 0x3526 -> 0xa3b0) with the world/surface manager. The ctx IS that
// CDDrawSurfaceMgr (== CSpriteFactoryHolder, the settled dual view): its +0x14
// m_workerCache (CDDrawWorkerCache) is the type registry - the ex
// "GameObjFactoryCtx"/"GameObjTypeRegistry" views are DISSOLVED (2026-07-16;
// see GameObjectFactory.cpp for the CreateWorker slot-9 identity proof).
#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

class CDDrawSurfaceMgr; // the world/surface manager (<DDrawMgr/DDrawSurfaceMgr.h>)

void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx); // 0x00a3b0

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
