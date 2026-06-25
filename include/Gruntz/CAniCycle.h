// CAniCycle.h - an animation-cycle eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf in the same animation family as CSimpleAnimation /
// CSingleFrameMessage: a per-class activation registry (g_aniCycleActReg
// @0x646088) bound by RegisterActs to a per-frame AdvanceAnim handler under the
// shared activation-name key "A".
//
// CAniCycle : CUserLogic. Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CAniCycle : CUserLogic)

class CAniCycle : public CUserLogic {
public:
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab0e0
    // The per-frame handler (@0x0ab2e0); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
};

#endif // GRUNTZ_CANICYCLE_H
