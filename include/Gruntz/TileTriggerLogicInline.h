#ifndef GRUNTZ_GRUNTZ_TILETRIGGERLOGICINLINE_H
#define GRUNTZ_GRUNTZ_TILETRIGGERLOGICINLINE_H

#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Rez/FrameClock.h>

#include <string.h>

__inline i32 CTileTriggerLogic::Build(
    CTileTriggerContainer* owner,
    TrigLogicId typeTag,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rects,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    if (m_initGate != false) {
        return 0;
    }
    memcpy(m_linkKeys, rects, sizeof(m_linkKeys));
    return Setup(
        owner,
        typeTag,
        tileX,
        tileY,
        cellKey,
        tileToken,
        dutyOnSpan,
        leadInSpan,
        dutyOffSpan
    );
}

__inline i32 CTileTriggerLogic::Setup(
    CTileTriggerContainer* owner,
    TrigLogicId typeTag,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    if (m_initGate != false) {
        return 0;
    }
    m_tile.m_y = tileY;
    m_tile.m_x = tileX;
    m_owner = owner;
    m_typeTag = typeTag;
    m_cellKey = cellKey;
    m_initGate = true;
    m_tileToken = tileToken;
    m_startClock = g_frameTime;
    m_leadInSpan = leadInSpan;
    m_dutyOn = false;
    m_dutyOnSpan = dutyOnSpan;
    m_dutyOffSpan = dutyOffSpan;
    if (typeTag != TRIGID_COVERED_POWERUP_26 && dutyOffSpan == 0) {
        m_dutyOffSpan = dutyOnSpan;
        m_startClock = g_frameTime;
    }
    return 1;
}

#endif // GRUNTZ_GRUNTZ_TILETRIGGERLOGICINLINE_H
