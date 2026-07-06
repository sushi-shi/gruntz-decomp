// GameModeBase.h - CGameModeBase, the WAP32 base the game-state classes chain
// their cleanup to (CState's dtor casts `(CGameModeBase *)this` and calls it).
// BaseCleanup has no body here: it is a reloc-masked external call.
#ifndef GRUNTZ_GRUNTZ_GAMEMODEBASE_H
#define GRUNTZ_GRUNTZ_GAMEMODEBASE_H

#include <rva.h>

class Holder_f9840;
SIZE_UNKNOWN(CGameModeBase);
struct CGameModeBase {
    char m_pad0[0xc];
    Holder_f9840* m_c;   // +0x0c
    void BaseCleanup();  // thiscall, no-body -> reloc-masked external call
    void Reset();        // 0x0f9840
    void ResetPreview(); // 0x0de140
};

#endif // GRUNTZ_GRUNTZ_GAMEMODEBASE_H
