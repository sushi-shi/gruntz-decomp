// RockBreakMgr.h - CRockBreakMgr, the rock-break particle spawner reached through the
// registry's +0x68 command-grid slot.
//
// Promoted out of the .cpp files (2026-07-13). It was defined TWICE as a .cpp-local view
// (src/Gruntz/TriggerMgr.cpp, which owns the body, and src/Gruntz/RockBreakEffectUpdate.cpp,
// a decl-only copy) - a type in a .cpp is a fake per-TU view whichever TU wins, and the
// duplicate is exactly what let a THIRD consumer (CGruntBehaviorLeaf) invent
// `CDecayAnim::DrawAnimAt` for the same function instead of naming it.
#ifndef GRUNTZ_ROCKBREAKMGR_H
#define GRUNTZ_ROCKBREAKMGR_H

#include <Ints.h>
#include <rva.h>

// The write-grid host at +0x22c IS the registry world holder (g_gameReg->m_world):
// ::CSpriteFactoryHolder (<Gruntz/GameRegistry.h>). Its m_8 is the CSpriteFactory the
// particle spawner calls CreateSprite on, and its m_28 the CSndHost cue registry - both
// already modeled there. The old `RockMapHost` .cpp-local view is dissolved.
#include <Gruntz/GameRegistry.h>

class CRockBreakMgr {
public:
    void Prepare(i32 cx, i32 cy, i32 r, i32, i32);              // thunk 0x400c __thiscall
    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4); // 0x0007b440 (body: TriggerMgr.cpp)

    char m_pad00[0x22c];
    CSpriteFactoryHolder* m_22c; // +0x22c  (== g_gameReg->m_world)
};
SIZE_UNKNOWN(CRockBreakMgr);

#endif // GRUNTZ_ROCKBREAKMGR_H
