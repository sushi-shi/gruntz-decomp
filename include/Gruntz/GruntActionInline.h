#ifndef GRUNTZ_GRUNTACTIONINLINE_H
#define GRUNTZ_GRUNTACTIONINLINE_H

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/VoiceManager.h>
#include <Wap32/TileGeometry.h>

#define GRUNT_IS_USING_TOY(result)                                                                 \
    (((result) = IsAnimationAct("G")) || ((result) = IsAnimationAct("L"))                          \
     || ((result) = IsAnimationAct("P")))

inline void CGrunt::RestorePreviousAppearance() {
    m_entranceActive = false;
    bool previousWasWalk = (::GetAnimationActName(m_previousAnimationActId) == "D");
    if (previousWasWalk) {
        if (m_poweredUp != false && m_neighborValid == false) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        m_tileMoveCommitted = false;
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        char* name = EntranceCell()->WalkName().GetBuffer(0);
        SetImageSetByName(name);
    } else {
        ResetEntranceAnimation(1, 0, 0);
    }
}

inline void CGrunt::SettleTubeMove() {
    CWwdSpriteObject* object = m_object;
    i32 savedX = m_lastTilePx.m_x;
    i32 savedY = m_lastTilePx.m_y;
    DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)
    i32 redo = 1;
    if (PIXEL_PAIR_NOT_AT_POSITION(pixelX, pixelY, savedX, savedY)) {
        if (IsDropReady(1)) {
            m_coordToggle = (m_coordToggle == false);
            redo = 0;
        }
    }
    SnapToLastTile(1);
    if (redo) {
        SET_ANIMATION_ACT("D");
        SetupTubeAnim(m_coordToggle);
    }
}

inline bool CGrunt::SettleActiveTubeMove() {
    if (!IsAnimationAct("N")) {
        return false;
    }
    SettleTubeMove();
    return true;
}

inline void CGrunt::RestoreToolAfterToyUse(i32 defer) {
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, defer);
    {
        i32 sortKey = m_object->m_screenY + 0x186a0;
        CWwdSpriteObject* object = m_object;
        SET_SORT_KEY_IF_CHANGED(object, sortKey)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopVehicleLoopSound();
}

#endif // GRUNTZ_GRUNTACTIONINLINE_H
