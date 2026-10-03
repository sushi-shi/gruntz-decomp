#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntVoice.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/StreamVoice.h>
#include <Globals.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntVoiceActReg.h>
#include <Gruntz/GruntVoiceInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Ufo.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/VoiceTrigger.h>
#include <Image/CImage.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/LogicRecordEvent.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

template<>
CActReg CActRegPool<CGruntVoice>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CVoiceTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);


CVoiceTrigger::CVoiceTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

void ButeParseErrorSink(const char* msg) {
    if (g_gameReg) {
        g_gameReg->EnterModalUI(msg);
    }
}

i32 DispatchGruntVoiceLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntVoice)
}

i32 DispatchVoiceTriggerLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CVoiceTrigger)
}

CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXCLAMATION");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_VOICE)
    m_stream = NULL;
    m_playbackTiming.Clear();
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_SKIP_ACTIVE_KEEP_ACTIVE);
    Hide();
    m_priority = 0;
    SET_ANIMATION_ACT("A");
    m_sourceObjectId = 0;
    m_positionMode = VOICE_INDICATOR_AT_LOGIC_OBJECT;
}

CVoiceTrigger::CVoiceTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
    SET_ANIMATION_ACT("A");
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << TILE_SHIFT_PX) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << TILE_SHIFT_PX) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << TILE_SHIFT_PX) - 7;
    m_object->m_area.bottom =
        m_object->m_screenY + (m_object->m_extent.bottom << TILE_SHIFT_PX) + 7;
}

void CGruntVoice::FireActivation(i32 actionId) {
    DispatchRegisteredAct(this, actionId);
}

void RegisterGruntVoiceActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CGruntVoice>::s_table[id] = static_cast<CActHandler>(&CGruntVoice::HideIndicator);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CGruntVoice>::s_table[id2] =
        static_cast<CActHandler>(&CGruntVoice::UpdateIndicator);
}

void CVoiceTrigger::FireActivation(i32 actionId) {
    DispatchRegisteredAct(this, actionId);
}

void CVoiceTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CVoiceTrigger>::s_table[id] =
        static_cast<i32 (CUserLogic::*)()>(&CVoiceTrigger::Tick);
}

i32 CVoiceTrigger::Tick() {
    i32 playerIndex, unitIndex;
    CGrunt* hit = g_gameReg->m_triggerMgr->FindGruntAt(
        m_object->m_screenX,
        m_object->m_screenY,
        &m_object->m_extent,
        &playerIndex,
        &unitIndex,
        &m_object->m_area
    );
    if (hit && playerIndex == g_curPlayer) {
        CGameObject* hs = hit->m_object;
        i32 hy = hs->m_screenY;
        i32 hx = hs->m_screenX;
        if (::PtInRect(&g_gameReg->m_viewBounds, hx, hy)) {
            if (g_gameReg->VoiceMgr()
                    ->PlayVoice(hit, m_object->m_smarts, m_object->m_health, 0, -1, -1)) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            }
        }
    }
    return 0;
}

i32 CGruntVoice::BeginPlayback(
    i32 sourceObjectId,
    StreamVoice* stream,
    i32 priority,
    i32 positionMode
) {
    if (stream == NULL) {
        return 0;
    }
    m_sourceObjectId = sourceObjectId;
    m_positionMode = positionMode;
    m_stream = stream;
    m_playbackTiming.Start(stream->GetDurationMs());
    m_priority = priority;
    m_previousAnimationActId = m_logicRecord->m_eventCode;
    m_logicRecord->SetEventCode(ActFindId("B"));
    return 1;
}

void CGruntVoice::ResetPlayback() {
    m_stream = NULL;
    SET_ANIMATION_ACT("A");
    m_priority = 0;
    m_sourceObjectId = 0;
}

i32 CGruntVoice::HideIndicator() {
    m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 0;
}

i32 CGruntVoice::UpdateIndicator() {
    if (m_stream == NULL || m_playbackTiming.Expired()) {
        m_stream = NULL;
        m_sourceObjectId = 0;
        m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
        SET_ANIMATION_ACT("A");
        m_priority = 0;
        return 0;
    }
    if (m_positionMode == VOICE_INDICATOR_AT_LOGIC_OBJECT) {
        if (PositionIndicatorAtLogicObject()) {
            return 0;
        }
    } else if (PositionIndicatorAtSourceObject()) {
        return 0;
    }
    m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 0;
}
