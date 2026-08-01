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
RVA(0x00112a50, 0xdd)
// Slot names come from the base declaration in <Gruntz/TileTriggerSwitchLogic.h>
// (typeId/tileX/tileY/cellKey/linkGate, proven there by CTileTriggerSwitchLogic::Setup's
// own stores); `iconFrame` is this override's own: it gates the sprite build and is then
// ApplyLookupSprite's FRAME index into the SMALLICONZ set (CGameObject::ApplyLookupSprite
// @0x1504d0 bounds-checks it against the sprite's m_minIndex/m_maxIndex).
i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    i32 linkGate,
    i32 a8,
    i32 iconFrame
) {
    i32 px;
    i32 py;
    CWwdGameObjectA* spr;

    if (m_initGate != 0) {
        goto fail;
    }
    if (typeId == 4 && rect[0].left == 0) {
        goto fail;
    }
    memcpy(m_block, rect, sizeof(m_block)); // rep movsd, ecx=0x18 -> this+0x2c
    if (!Setup(owner, typeId, tileX, tileY, cellKey, linkGate, a8, iconFrame)) {
        goto fail;
    }
    px = (tileX << 5) + 0x10;
    py = (tileY << 5) + 0x10;
    if (iconFrame == 0) {
        return 1;
    }
    spr = g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        goto fail;
    }
    spr->m_7c->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", iconFrame);
    if (spr->m_layer == 0) {
        goto fail;
    }
    return 1;
fail:
    return 0;
}
