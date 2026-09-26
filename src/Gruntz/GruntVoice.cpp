#include <rva.h>

#include <Gruntz/GruntVoice.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/StreamVoice.h>
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

RVA_DYNINIT(0x00119350, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00119370, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x001193a0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x001193c0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x001193f0, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00119410, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00119440, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00119460, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x00119490, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x001194b0, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x001194e0, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00119500, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00119530, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00119550, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00119580, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x001195a0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x001195d0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x001195f0, 0x1a, s_gruntDirCenter)

RVA_DYNINIT(0x00119da0, 0xa, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119dc0, 0x15, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119df0, 0xe, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119e10, 0x1f, CActRegPool<CGruntVoice>::s_table)
template<> DATA(0x002514d8)
CActReg CActRegPool<CGruntVoice>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0011a300, 0xa, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a320, 0x15, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a350, 0xe, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a370, 0x1f, CActRegPool<CVoiceTrigger>::s_table)
template<> DATA(0x00251500)
CActReg CActRegPool<CVoiceTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

struct CString;

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

RVA_COMPGEN(0x00013570, 0x1e, ??_GCVoiceTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

RVA(0x00119320, 0x15)
void ButeParseErrorSink(const char* msg) {
    if (g_gameReg) {
        g_gameReg->EnterModalUI(msg);
    }
}

RVA(0x00119620, 0xf1)
i32 DispatchGruntVoiceLogic(CGameObject* obj) {
    TILE_LOGIC_RECORD_DISPATCH(CGruntVoice)
}

RVA(0x00119760, 0xf1)
i32 DispatchVoiceTriggerLogic(CGameObject* obj) {
    TILE_LOGIC_RECORD_DISPATCH(CVoiceTrigger)
}

RVA(0x001198a0, 0x195)
CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXCLAMATION");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_VOICE);
    m_stream = NULL;
    m_playbackTiming.m_startLo = 0;
    m_playbackTiming.m_intervalLo = 0;
    m_playbackTiming.m_startHi = 0;
    m_playbackTiming.m_intervalHi = 0;
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_SKIP_ACTIVE_KEEP_ACTIVE);
    Hide();
    m_priority = 0;
    SET_ANIMATION_ACT("A");
    m_sourceObjectId = 0;
    m_positionMode = VOICE_INDICATOR_AT_LOGIC_OBJECT;
}

RVA_COMPGEN(0x00119ab0, 0x1e, ??_GCGruntVoice@@UAEPAXI@Z)
RVA_COMPGEN(0x00119ae0, 0x44, ??1CGruntVoice@@UAE@XZ)

// @early-stop
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
    SET_ANIMATION_ACT("A");
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_object->m_area.left =
        m_object->m_screenPosition.m_x - (m_object->m_extent.left << TILE_SHIFT_PX) - 7;
    m_object->m_area.right =
        m_object->m_screenPosition.m_x + (m_object->m_extent.right << TILE_SHIFT_PX) + 7;
    m_object->m_area.top =
        m_object->m_screenPosition.m_y - (m_object->m_extent.top << TILE_SHIFT_PX) - 7;
    m_object->m_area.bottom =
        m_object->m_screenPosition.m_y + (m_object->m_extent.bottom << TILE_SHIFT_PX) + 7;
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 actionId) {
    DispatchRegisteredAct(this, actionId);
}

RVA(0x00119fa0, 0x2ac)
void RegisterGruntVoiceActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CGruntVoice>::s_table[id] = static_cast<CActHandler>(&CGruntVoice::HideIndicator);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CGruntVoice>::s_table[id2] =
        static_cast<CActHandler>(&CGruntVoice::UpdateIndicator);
}

RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 actionId) {
    DispatchRegisteredAct(this, actionId);
}

RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CVoiceTrigger>::s_table[id] =
        static_cast<i32 (CUserLogic::*)()>(&CVoiceTrigger::Tick);
}

RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 playerIndex, unitIndex;
    CGrunt* hit = g_gameReg->m_triggerMgr->FindGruntAt(
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y,
        &m_object->m_extent,
        &playerIndex,
        &unitIndex,
        &m_object->m_area
    );
    if (hit && playerIndex == g_curPlayer) {
        CGameObject* hs = hit->m_object;
        i32 hy = hs->m_screenPosition.m_y;
        i32 hx = hs->m_screenPosition.m_x;
        if (::PtInRect(&g_gameReg->m_viewBounds, hx, hy)) {
            if (g_gameReg->m_voiceManager
                    ->PlayVoice(hit, m_object->m_smarts, m_object->m_health, 0, -1, -1)) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            }
        }
    }
    return 0;
}

RVA(0x0011a7e0, 0x6e)

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

RVA(0x0011a870, 0x38)
void CGruntVoice::ResetPlayback() {
    m_stream = NULL;
    SET_ANIMATION_ACT("A");
    m_priority = 0;
    m_sourceObjectId = 0;
}

RVA(0x0011a8c0, 0xf)
i32 CGruntVoice::HideIndicator() {
    m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 0;
}

RVA(0x0011a8e0, 0x198)
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
