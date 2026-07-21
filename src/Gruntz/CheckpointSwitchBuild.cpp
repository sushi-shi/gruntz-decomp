#include <string.h>                   // memcpy -> the /Oi `rep movsd` that copies rect into m_block
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite) + AnimWorkerObj
#include <rva.h>
#include <Gruntz/TileTriggerSwitchLogic.h>

// The class itself now lives in <Gruntz/TileTriggerSwitchLogic.h> (real derived class, no
// data members, sizeof 0x8c). BuildSmall is its slot-1 override; the base's slot-0 "build"
// virtual (Setup) is the 8-arg builder it chains to.
// @early-stop
// tail-merge + prologue-scheduling wall (~62%). The LAYOUT is now byte-correct and verified
// against retail: `mov ecx,[this+0x20]` (the m_20 gate), `lea edi,[this+0x2c]` (m_block) and
// `mov ecx,0x18; rep movsd` all appear on BOTH sides of the objdiff. What is left is not
// source-steerable: retail tail-merges both early `return 0` exits into ONE shared epilogue
// (`jmp`), while our /O2 emits an inline `pop/pop/pop/pop; ret 0x24` at each, and it loads
// the ebx/ebp args inside the prologue push run. See
// docs/patterns/identical-return-epilogue-tailmerge.md.
// (The pre-existing @early-stop blamed the same wall but was measuring WRONG code: the old
// .cpp-local view shadowed the base's fields, so every access here was 0xc8 too high.)
RVA(0x00112a50, 0xdd)
i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    const i32* rect,
    i32 a7,
    i32 a8,
    i32 a9
) {
    if (m_initGate != 0) {
        return 0;
    }
    if (a2 == 4 && rect[0] == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block)); // rep movsd, ecx=0x18 -> this+0x2c
    if (!Setup(owner, a2, a3, a4, a5, a7, a8, a9)) {
        return 0;
    }
    i32 px = (a3 << 5) + 0x10;
    i32 py = (a4 << 5) + 0x10;
    if (a9 == 0) {
        return 1;
    }
    CWwdGameObjectA* spr =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        return 0;
    }
    spr->m_7c->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", a9);
    if (spr->m_layer == 0) {
        return 0;
    }
    return 1;
}
