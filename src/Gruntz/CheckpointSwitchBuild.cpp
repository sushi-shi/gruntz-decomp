#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/UserLogic.h>

#include <string.h>

// @early-stop
RVA(0x00112a50, 0xdd)

i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    i32 linkGate,
    i32 damageParam,
    i32 checkpointType
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
    memcpy(m_block, rect, sizeof(m_block));
    if (!Setup(owner, typeId, tileX, tileY, cellKey, linkGate, damageParam, checkpointType)) {
        goto fail;
    }
    px = (tileX << 5) + 0x10;
    py = (tileY << 5) + 0x10;
    if (checkpointType == 0) {
        return 1;
    }
    spr = g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        goto fail;
    }
    spr->m_animWorker->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", checkpointType);
    if (spr->m_layer == 0) {
        goto fail;
    }
    return 1;
fail:
    return 0;
}
