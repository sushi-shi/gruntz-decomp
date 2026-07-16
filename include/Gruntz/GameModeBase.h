// GameModeBase.h - CGameModeBase, the WAP32 base the game-state classes chain
// their cleanup to (CState's dtor casts `(CGameModeBase *)this` and calls it).
// BaseCleanup has no body here: it is a reloc-masked external call.
#ifndef GRUNTZ_GRUNTZ_GAMEMODEBASE_H
#define GRUNTZ_GRUNTZ_GAMEMODEBASE_H

#include <rva.h>

class CDDSurface; // the owned blit surfaces BaseCleanup returns to the pool

// The mode's +0x0c context holder is the ONE canonical CDDrawSurfaceMgr (the ex
// `Holder_f9840` placeholder view is DISSOLVED 2026-07-16: its "+0x1c surface
// pool" was m_ptrColl and its "+0x28 map sub-manager" m_soundRegistry - the same
// slots every CState::m_c consumer reads; CGameModeBase::m_c IS CState::m_c, the
// same +0x0c field of the same state object).
#include <DDrawMgr/DDrawSurfaceMgr.h>

SIZE_UNKNOWN(CGameModeBase);
struct CGameModeBase {
    char m_pad0[0xc];
    CDDrawSurfaceMgr* m_c; // +0x0c
    char m_pad10[0x14 - 0x10];
    CDDSurface* m_14; // +0x14  owned blit surface
    CDDSurface* m_18; // +0x18  owned blit surface
    char m_pad1c[0x3c - 0x1c];
    i32 m_3c; // +0x3c  cleared on cleanup
    char m_pad40[0x160 - 0x40];
    CDDSurface* m_160;   // +0x160 owned blit surface
    CDDSurface* m_164;   // +0x164 owned blit surface
    void BaseCleanup();  // 0x0fa150 (re-homed to src/Gruntz/GameModeBase.cpp)
    void Reset();        // 0x0f9840
    void ResetPreview(); // 0x0de140
};

#endif // GRUNTZ_GRUNTZ_GAMEMODEBASE_H
