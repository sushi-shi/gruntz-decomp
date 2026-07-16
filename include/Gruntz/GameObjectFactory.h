// GameObjectFactory.h - the game-object type-registry seeding entry (owner:
// src/Gruntz/GameObjectFactory.cpp). CGruntzMgr::Run calls it at boot (via ILT
// thunk 0x3526 -> 0xa3b0) with the world/surface manager: GameObjFactoryCtx is
// a facet view of that CSpriteFactoryHolder/CDDrawSurfaceMgr whose +0x14
// (m_workerCache) it reads as the type registry - the GameObjTypeRegistry ==
// CDDrawWorkerCache identity is a pending fold (see GameObjectFactory.cpp).
#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

struct GameObjFactoryCtx;

void RegisterGameObjectTypes(GameObjFactoryCtx* ctx); // 0x00a3b0

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
