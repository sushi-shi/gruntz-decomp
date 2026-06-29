// CFrontCandyAni.h - a front-candy eyecandy animation game-object
// (C:\Proj\Gruntz). The sibling of CBehindCandyAni (the eyecandy that draws in
// FRONT of the gruntz rather than behind), sharing the same CUserLogic leaf
// archetype: a per-coordinate activation registry (g_frontCandyActReg @0x6460b0)
// that FireActivation dispatches through, bound by RegisterActs to the per-frame
// AdvanceAnim handler under the shared activation-name key "A".
//
// CFrontCandyAni : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#ifndef GRUNTZ_CFRONTCANDYANI_H
#define GRUNTZ_CFRONTCANDYANI_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CFrontCandyAni : CUserLogic)

class CFrontCandyAni : public CUserLogic {
public:
    // Construct the class's activation-coordinate registry (g_frontCandyActReg
    // @0x6460b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0ad130
    // Look the activation coordinate up in the class registry; if its entry has a
    // bound handler, look it up again and dispatch it __thiscall on this. The SAME
    // archetype as CParticlez::FireActivation (0x046d30).
    void FireActivation(i32 coord); // 0x0ad1b0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ad310
    i32 AdvanceAnim();          // 0x0ad510 (re-target bound anim to the draw-delta; ret 0)
};

#endif // GRUNTZ_CFRONTCANDYANI_H
