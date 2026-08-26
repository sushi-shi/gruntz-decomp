#include <rva.h>

#include <Gruntz/TileLogicPump.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileTrigger.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/WarpStonePad.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>
#include <Wwd/LogicRecordEvent.h>

#include <string.h>

RVA_DYNINIT(0x0010d950, 0xa, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d970, 0x15, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d9a0, 0xe, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d9c0, 0x1f, CActRegPool<CWarpStonePad>::s_table)
template<> DATA(0x0024f5f8)
CActReg CActRegPool<CWarpStonePad>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010df30, 0xa, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010df50, 0x15, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010df80, 0xe, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010dfa0, 0x1f, CActRegPool<CTileTriggerSwitch>::s_table)
template<> DATA(0x0024f6f0)
CActReg CActRegPool<CTileTriggerSwitch>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010e530, 0xa, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e550, 0x15, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e580, 0xe, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e5a0, 0x1f, CActRegPool<CTileTrigger>::s_table)
template<> DATA(0x0024f768)
CActReg CActRegPool<CTileTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010eb10, 0xa, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010eb30, 0x15, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010eb60, 0xe, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010eb80, 0x1f, CActRegPool<CBrickz>::s_table)
template<> DATA(0x0024f718)
CActReg CActRegPool<CBrickz>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010f270, 0xa, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f290, 0x15, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f2c0, 0xe, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f2e0, 0x1f, CActRegPool<CCheckpointTrigger>::s_table)
template<> DATA(0x0024f740)
CActReg CActRegPool<CCheckpointTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_DYNINIT(0x0010fda0, 0xa, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fdc0, 0x15, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fdf0, 0xe, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fe10, 0x1f, CActRegPool<CTileTriggerTransition>::s_table)
template<> DATA(0x0024f678)
CActReg CActRegPool<CTileTriggerTransition>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

#define TILE_LOGIC_RECORD_DISPATCH(LEAF)                                                           \
    CLogicRecord* record = obj->m_logicRecord;                                                     \
    switch (record->LogicEvent()) {                                                                \
        case ACT_UNINITIALISED: {                                                                  \
            record->SetLogicEvent(ACT_LIVE);                                                       \
            LEAF* t = new LEAF(obj);                                                               \
            t->Activate();                                                                         \
            record->m_userLogic = t;                                                               \
            break;                                                                                 \
        }                                                                                          \
        case ACT_OBJECT_REMOVED:                                                                   \
            record->m_userLogic->OnObjectRemoved();                                                \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            record->m_userLogic->OnLeaveActiveRegion();                                            \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            record->m_userLogic->PrepareSave();                                                    \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            record->m_userLogic->AfterSave();                                                      \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            record->m_userLogic->AfterLoad();                                                      \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            record->m_userLogic->AfterLoadReferences();                                            \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
            break;                                                                                 \
        default:                                                                                   \
            DispatchLogicEvent(record->m_userLogic);                                               \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA_COMPGEN(0x00010fa0, 0x1e, ??_GCWarpStonePad@@UAEPAXI@Z)
RVA_COMPGEN(0x00010fd0, 0x44, ??1CWarpStonePad@@UAE@XZ)

RVA_COMPGEN(0x000110d0, 0x1e, ??_GCTileTriggerSwitch@@UAEPAXI@Z)
RVA_COMPGEN(0x00011100, 0x44, ??1CTileTriggerSwitch@@UAE@XZ)

RVA_COMPGEN(0x00011270, 0x1e, ??_GCTileTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000112a0, 0x44, ??1CTileTrigger@@UAE@XZ)

RVA_COMPGEN(0x000113a0, 0x1e, ??_GCBrickz@@UAEPAXI@Z)
RVA_COMPGEN(0x000113d0, 0x44, ??1CBrickz@@UAE@XZ)

RVA_COMPGEN(0x00011460, 0x1e, ??_GCCheckpointTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011490, 0x44, ??1CCheckpointTrigger@@UAE@XZ)

RVA_COMPGEN(0x00011520, 0x1e, ??_GCTileSecretTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011550, 0x44, ??1CTileSecretTrigger@@UAE@XZ)

RVA_COMPGEN(0x000115e0, 0x1e, ??_GCGiantRock@@UAEPAXI@Z)
RVA_COMPGEN(0x00011610, 0x44, ??1CGiantRock@@UAE@XZ)

RVA_COMPGEN(0x000116a0, 0x1e, ??_GCCoveredPowerup@@UAEPAXI@Z)
RVA_COMPGEN(0x000116d0, 0x44, ??1CCoveredPowerup@@UAE@XZ)

RVA_COMPGEN(0x000117d0, 0x1e, ??_GCTileTriggerTransition@@UAEPAXI@Z)
RVA_COMPGEN(0x00011800, 0x44, ??1CTileTriggerTransition@@UAE@XZ)

RVA(0x0010cc40, 0xf1)
i32 DispatchTileTriggerLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileTrigger)}

RVA(0x0010cd80, 0xf1)
i32 DispatchTileTriggerSwitchLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileTriggerSwitch)}

RVA(0x0010cec0, 0xf1)
i32 DispatchTileSecretTriggerLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileSecretTrigger)}

RVA(0x0010d000, 0xf1)
i32 DispatchGiantRockLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CGiantRock)}

RVA(0x0010d140, 0xf1)
i32 DispatchCoveredPowerupLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CCoveredPowerup)}

RVA(0x0010d280, 0xf1)
i32 DispatchTileTriggerTransitionLogic(CGameObject* obj){
    TILE_LOGIC_RECORD_DISPATCH(CTileTriggerTransition)
}

RVA(0x0010d3c0, 0xf4)
i32 DispatchCheckpointTriggerLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CCheckpointTrigger* t = new CCheckpointTrigger(obj);
            t->Activate();
            record->m_userLogic = t;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0010d500, 0xf1)
i32 DispatchBrickzLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CBrickz)}

RVA(0x0010d640, 0xf1)
i32 DispatchWarpStonePadLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CWarpStonePad)}

RVA(0x0010d780, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        Hide();
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    SET_ANIMATION_ACT("A");
}

RVA(0x0010d9f0, 0x102)
void CWarpStonePad::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010db50, 0x18d)
void CWarpStonePad::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CWarpStonePad>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CWarpStonePad::AdvanceAnim);
}

RVA(0x0010dd50, 0x3)
i32 CWarpStonePad::AdvanceAnim() {
    return 0;
}

RVA(0x0010dd70, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");

    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();
}

RVA(0x0010dfd0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e130, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerSwitch::AdvanceAnim);
}

RVA(0x0010e330, 0x3)
i32 CTileTriggerSwitch::AdvanceAnim() {
    return 0;
}

// @early-stop
RVA(0x0010e350, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();

    i32 tileX = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_speedX = tileX;
    m_object->m_speedY = tileY;
    m_object->m_id = (tileX << 8) + tileY;
}

RVA(0x0010e5d0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e730, 0x18d)
void CTileTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTrigger::AdvanceAnim);
}

// @early-stop
RVA(0x0010e930, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();

    i32 tileX = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_speedX = tileX;
    m_object->m_speedY = tileY;
    m_object->m_id = (tileX << 8) + tileY;
}

RVA(0x0010ebb0, 0x102)
void CBrickz::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010ed10, 0x18d)
void CBrickz::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CBrickz>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBrickz::Trigger);
}

RVA(0x0010ef10, 0x3)
i32 CBrickz::Trigger() {
    return 0;
}

RVA(0x0010ef30, 0x3)
i32 CTileTrigger::AdvanceAnim() {
    return 0;
}

// @early-stop
RVA(0x0010ef50, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));

    CWwdSpriteObject* o = m_object;
    i32 zk = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(o, zk)
    memset(m_state, 0, sizeof(m_state));
    if (m_object->m_extent.left == COORD_UNSET) {
        m_object->m_extent.left = 0;
    }
    if (m_object->m_area.left == COORD_UNSET) {
        m_object->m_area.left = 0;
    }
    if (m_object->m_switchRect.left == COORD_UNSET) {
        m_object->m_switchRect.left = 0;
    }
    if (m_object->m_clip.left == COORD_UNSET) {
        m_object->m_clip.left = 0;
    }
    m_state[0] = m_object->m_extent.left;
    m_state[1] = m_object->m_extent.top;
    m_state[2] = m_object->m_extent.right;
    m_state[3] = m_object->m_extent.bottom;
    m_state[4] = m_object->m_area.left;
    m_state[5] = m_object->m_area.top;
    m_state[6] = m_object->m_area.right;
    m_state[7] = m_object->m_area.bottom;
    m_state[8] = m_object->m_switchRect.left;
    m_state[9] = m_object->m_switchRect.top;
    m_state[10] = m_object->m_switchRect.right;
    m_state[11] = m_object->m_switchRect.bottom;
    m_state[12] = m_object->m_clip.left;
    m_state[13] = m_object->m_clip.top;
    m_state[14] = m_object->m_clip.right;

    b32 found = false;
    m_firstEmpty = 0;
    while (found == false && m_firstEmpty < 15) {

        if (m_state[m_firstEmpty] != 0) {
            m_firstEmpty++;
        } else {
            found = true;
        }
    }
}

RVA(0x0010f310, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010f470, 0x2ac)
void CCheckpointTrigger::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act);

    ACT_NAME_ID(id2, "B")
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id2)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::AdvanceCheckpointAnimation);
}

RVA(0x0010f7d0, 0x235)
i32 CCheckpointTrigger::Act() {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);

    for (i32 i = 0; i < m_firstEmpty; i++) {
        i32 key = m_state[i];
        if (key == 0) {
            return 0;
        }
        CTileTriggerSwitchLogic* child =
            play->m_tileTriggers->FindSwitchLogic(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (child == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), 0x44c);
            return 0;
        }
        if (child->m_linkGate == false) {
            return 0;
        }
    }

    SET_ANIMATION_ACT("B");
    SwitchAnimationByName("GAME_CHECKPOINTFLAGSET", 0);

    if (play->m_levelTimer != NULL) {
        i32 minutes = m_object->m_score;
        i32 seconds = m_object->m_points;
        if (g_gameReg->m_isEasyMode != false && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            seconds += seconds;
            minutes += minutes;
            if (seconds > 0x3b) {
                minutes++;
                seconds -= 0x3c;
            }
        }
        play->m_levelTimer->AddTime(minutes, seconds);
    }

    CObject* cue = m_wwdObject->OwnerMgr()->m_soundRegistry->Lookup("GAME_FLAGRISE");
    if (cue != NULL) {
        static_cast<SoundCue*>(cue)->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
    }
    g_gameReg->OnCheckpointReached();

    i32 hi = m_firstEmpty - 1;

    CGruntzMgr* reg = g_gameReg;
    i32 span = hi + 1;
    i32 pick;
    if (span == 0) {
        if ((GetRandomNumber() & 1)) {
            pick = 0;
        } else {
            pick = hi;
        }
    } else {
        pick = reg->Rand() % span;
    }

    CTileTriggerSwitchLogic* pad =
        play->m_tileTriggers->FindSwitchLogic(m_state[pick], TRIGID_CHECKPOINT_SWITCH_8);
    if (pad == NULL) {
        g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), 0x44c);
        return 0;
    }

    i32 gy = pad->m_tileY;
    i32 gx = pad->m_tileX;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 owner;
    if (static_cast<u32>(gx) < grid->m_width && static_cast<u32>(gy) < grid->m_height) {
        owner = grid->m_rows[gy][gx].m_occupantId;
    } else {
        owner = -1;
    }
    if (owner == -1) {
        return 0;
    }

    i32 ownerCol = (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
    owner &= GRUNT_IDENTITY_COMPONENT_MASK;
    CGrunt* g = g_gameReg->m_triggerMgr->m_units[ownerCol * TM_UNITS_PER_PLAYER + owner];
    if (g == NULL) {
        return 0;
    }

    i32 sy = g->m_object->m_screenY;
    i32 sx = g->m_object->m_screenX;
    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
    if (sx >= view->right) {
        return 0;
    }
    if (sx < view->left) {
        return 0;
    }
    if (sy >= view->bottom) {
        return 0;
    }
    if (sy < view->top) {
        return 0;
    }
    g_gameReg->m_voiceManager->PlayVoice(g, 0x334, -1, 0, -1, -1);
    return 0;
}

RVA(0x0010faa0, 0x17)
i32 CCheckpointTrigger::AdvanceCheckpointAnimation() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x0010fad0, 0x8f)
i32 CCheckpointTrigger::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    switch (mode) {
        case SERIAL_LOAD:
            sa->Read(m_state, 0x3c);
            sa->Read(&m_firstEmpty, sizeof(m_firstEmpty));
            break;
        case SERIAL_SAVE:
            sa->Write(m_state, 0x3c);
            sa->Write(&m_firstEmpty, sizeof(m_firstEmpty));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM(arc, sa, mode, typeId, object)
}

RVA(0x0010fb90, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fbc0, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fbf0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

RVA(0x0010fc20, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION));

    CGameObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, 0)
}

RVA(0x0010fe40, 0x102)
void CTileTriggerTransition::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010ffa0, 0x18d)
void CTileTriggerTransition::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerTransition::TransitionAct);
}

RVA(0x001101a0, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    if (SwitchAnimationByName(geom, 0) == 0) {
        return 0;
    }
    APPLY_CURRENT_ANIMATION_FRAME_SPRITE(sprite, desc, elem)
    SET_ANIMATION_ACT("A");
    return 1;
}

RVA(0x00110240, 0x39)
i32 CTileTriggerTransition::TransitionAct() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}
