// GameModeBase.h - CGameModeBase, the WAP32 base the game-state classes chain
// their cleanup to (CState's dtor casts `(CGameModeBase *)this` and calls it).
// BaseCleanup has no body here: it is a reloc-masked external call.
#ifndef GRUNTZ_GRUNTZ_GAMEMODEBASE_H
#define GRUNTZ_GRUNTZ_GAMEMODEBASE_H

#include <rva.h>

class CDDrawSubMgrLeafScan; // m_28 sub-manager (full type in DDrawSubMgrLeafScan.h)
class CDDrawPtrCollections; // m_1c surface pool (full type in DDrawPtrCollections.h)
class CDDSurface;           // the owned blit surfaces BaseCleanup returns to the pool

// The mode's context holder (CGameModeBase::m_c). Identity unrecovered (the
// _f9840 suffix is a placeholder); the +0x1c surface pool (BaseCleanup returns the
// owned blits there) and the +0x28 leafscan sub-manager (Reset/ResetPreview) modeled.
SIZE_UNKNOWN(Holder_f9840);
struct Holder_f9840 {
    char m_pad0[0x1c];
    CDDrawPtrCollections* m_1c; // +0x1c  surface pool (RemoveItemA @0x142160)
    char m_pad20[0x28 - 0x20];
    CDDrawSubMgrLeafScan* m_28; // +0x28  map sub-manager (ClearMap / RemoveKeysEqual_157c70)
};

SIZE_UNKNOWN(CGameModeBase);
struct CGameModeBase {
    char m_pad0[0xc];
    Holder_f9840* m_c; // +0x0c
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
