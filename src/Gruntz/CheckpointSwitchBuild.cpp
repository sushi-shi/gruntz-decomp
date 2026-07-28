#include <string.h>               // memcpy -> the /Oi `rep movsd` that copies rect into m_block
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
// 62.4 -> 72.6 via the shared-exit spelling; residual is the prologue arg-load
// scheduling (retail interleaves the ebx/ebp arg loads into the push run).
// Every failure exit is a `goto fail` onto ONE shared bottom epilogue - retail's
// shape (`xor eax,eax; jmp <epi>`); the per-return spelling emitted an inline
// `pop/pop/pop/pop; ret 0x24` at each of them.
// docs/patterns/positive-gate-enables-shrink-wrap.md (shared-exit half).
// (The pre-existing @early-stop blamed the same wall but was measuring WRONG code: the old
// .cpp-local view shadowed the base's fields, so every access here was 0xc8 too high.)
RVA(0x00112a50, 0xdd)
i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    const RECT* rect,
    i32 a7,
    i32 a8,
    i32 a9
) {
    i32 px;
    i32 py;
    CWwdGameObjectA* spr;

    if (m_initGate != 0) {
        goto fail;
    }
    if (a2 == 4 && rect[0].left == 0) {
        goto fail;
    }
    memcpy(m_block, rect, sizeof(m_block)); // rep movsd, ecx=0x18 -> this+0x2c
    if (!Setup(owner, a2, a3, a4, a5, a7, a8, a9)) {
        goto fail;
    }
    px = (a3 << 5) + 0x10;
    py = (a4 << 5) + 0x10;
    if (a9 == 0) {
        return 1;
    }
    spr = g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        goto fail;
    }
    spr->m_7c->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", a9);
    if (spr->m_layer == 0) {
        goto fail;
    }
    return 1;
fail:
    return 0;
}
