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

RVA_DYNINIT(0x0010d820, 0xa, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d840, 0x15, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d870, 0xe, CActRegPool<CWarpStonePad>::s_table)
RVA_DYNINIT(0x0010d890, 0x1f, CActRegPool<CWarpStonePad>::s_table)
template<> DATA(0x0024e6a0)
CActReg CActRegPool<CWarpStonePad>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010de00, 0xa, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010de20, 0x15, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010de50, 0xe, CActRegPool<CTileTriggerSwitch>::s_table)
RVA_DYNINIT(0x0010de70, 0x1f, CActRegPool<CTileTriggerSwitch>::s_table)
template<> DATA(0x0024e798)
CActReg CActRegPool<CTileTriggerSwitch>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010e400, 0xa, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e420, 0x15, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e450, 0xe, CActRegPool<CTileTrigger>::s_table)
RVA_DYNINIT(0x0010e470, 0x1f, CActRegPool<CTileTrigger>::s_table)
template<> DATA(0x0024e810)
CActReg CActRegPool<CTileTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010e9e0, 0xa, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010ea00, 0x15, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010ea30, 0xe, CActRegPool<CBrickz>::s_table)
RVA_DYNINIT(0x0010ea50, 0x1f, CActRegPool<CBrickz>::s_table)
template<> DATA(0x0024e7c0)
CActReg CActRegPool<CBrickz>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0010f140, 0xa, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f160, 0x15, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f190, 0xe, CActRegPool<CCheckpointTrigger>::s_table)
RVA_DYNINIT(0x0010f1b0, 0x1f, CActRegPool<CCheckpointTrigger>::s_table)
template<> DATA(0x0024e7e8)
CActReg CActRegPool<CCheckpointTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_DYNINIT(0x0010fc70, 0xa, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fc90, 0x15, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fcc0, 0xe, CActRegPool<CTileTriggerTransition>::s_table)
RVA_DYNINIT(0x0010fce0, 0x1f, CActRegPool<CTileTriggerTransition>::s_table)
template<> DATA(0x0024e720)
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

RVA_COMPGEN(0x00010f90, 0x1e, ??_GCWarpStonePad@@UAEPAXI@Z)
RVA_COMPGEN(0x00010fc0, 0x44, ??1CWarpStonePad@@UAE@XZ)

RVA_COMPGEN(0x000110c0, 0x1e, ??_GCTileTriggerSwitch@@UAEPAXI@Z)
RVA_COMPGEN(0x000110f0, 0x44, ??1CTileTriggerSwitch@@UAE@XZ)

RVA_COMPGEN(0x00011260, 0x1e, ??_GCTileTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011290, 0x44, ??1CTileTrigger@@UAE@XZ)

RVA_COMPGEN(0x00011390, 0x1e, ??_GCBrickz@@UAEPAXI@Z)
RVA_COMPGEN(0x000113c0, 0x44, ??1CBrickz@@UAE@XZ)

RVA_COMPGEN(0x00011450, 0x1e, ??_GCCheckpointTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011480, 0x44, ??1CCheckpointTrigger@@UAE@XZ)

RVA_COMPGEN(0x00011510, 0x1e, ??_GCTileSecretTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011540, 0x44, ??1CTileSecretTrigger@@UAE@XZ)

RVA_COMPGEN(0x000115d0, 0x1e, ??_GCGiantRock@@UAEPAXI@Z)
RVA_COMPGEN(0x00011600, 0x44, ??1CGiantRock@@UAE@XZ)

RVA_COMPGEN(0x00011690, 0x1e, ??_GCCoveredPowerup@@UAEPAXI@Z)
RVA_COMPGEN(0x000116c0, 0x44, ??1CCoveredPowerup@@UAE@XZ)

RVA_COMPGEN(0x000117c0, 0x1e, ??_GCTileTriggerTransition@@UAEPAXI@Z)
RVA_COMPGEN(0x000117f0, 0x44, ??1CTileTriggerTransition@@UAE@XZ)

RVA(0x0010cb10, 0xf1)
i32 DispatchTileTriggerLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 DispatchTileTriggerSwitchLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 DispatchTileSecretTriggerLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 DispatchGiantRockLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 DispatchCoveredPowerupLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CCoveredPowerup)}

RVA(0x0010d150, 0xf1)
i32 DispatchTileTriggerTransitionLogic(CGameObject* obj){
    TILE_LOGIC_RECORD_DISPATCH(CTileTriggerTransition)
}

RVA(0x0010d290, 0xf4)
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

RVA(0x0010d3d0, 0xf1)
i32 DispatchBrickzLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CBrickz)}

RVA(0x0010d510, 0xf1)
i32 DispatchWarpStonePadLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CWarpStonePad)}

RVA(0x0010d650, 0x16c)
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

RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CWarpStonePad>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CWarpStonePad::AdvanceAnim);
}

RVA(0x0010dc20, 0x3)
i32 CWarpStonePad::AdvanceAnim() {
    return 0;
}

RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");

    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();
}

RVA(0x0010dea0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerSwitch::AdvanceAnim);
}

RVA(0x0010e200, 0x3)
i32 CTileTriggerSwitch::AdvanceAnim() {
    return 0;
}

// @early-stop
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();

    Coord tile;
    GetScreenTile(&tile);
    m_object->m_speed = tile;
    m_object->m_id = (tile.m_x << 8) + tile.m_y;
}

RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTrigger::AdvanceAnim);
}

// @early-stop
RVA(0x0010e800, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    Hide();

    Coord tile;
    GetScreenTile(&tile);
    m_object->m_speed = tile;
    m_object->m_id = (tile.m_x << 8) + tile.m_y;
}

RVA(0x0010ea80, 0x102)
void CBrickz::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010ebe0, 0x18d)
void CBrickz::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CBrickz>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBrickz::Trigger);
}

RVA(0x0010ede0, 0x3)
i32 CBrickz::Trigger() {
    return 0;
}

RVA(0x0010ee00, 0x3)
i32 CTileTrigger::AdvanceAnim() {
    return 0;
}

// @early-stop
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));

    CWwdSpriteObject* o = m_object;
    i32 zk = o->m_frameImage->m_anchor.y + o->m_screenPosition.m_y + 0x186a0;
    o->SetSortKey(zk);
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

RVA(0x0010f1e0, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010f340, 0x2ac)
void CCheckpointTrigger::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act);

    ACT_NAME_ID(id2, "B")
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id2)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::AdvanceCheckpointAnimation);
}

RVA(0x0010f6a0, 0x235)
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

    Coord tile = pad->m_tile;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 owner;
    if (static_cast<u32>(tile.m_x) < grid->m_width && static_cast<u32>(tile.m_y) < grid->m_height) {
        owner = grid->m_rows[tile.m_y][tile.m_x].m_occupantId;
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

    Coord position = g->m_object->ScreenPos();
    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
    if (!::PtInRect(view, position.m_x, position.m_y)) {
        return 0;
    }
    g_gameReg->m_voiceManager->PlayVoice(g, 0x334, -1, 0, -1, -1);
    return 0;
}

RVA(0x0010f970, 0x17)
i32 CCheckpointTrigger::AdvanceCheckpointAnimation() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x0010f9a0, 0x8f)
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

RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION));

    CGameObject* o = m_object;
    o->SetSortKey(0);
}

RVA(0x0010fd10, 0x102)
void CTileTriggerTransition::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010fe70, 0x18d)
void CTileTriggerTransition::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerTransition::TransitionAct);
}

RVA(0x00110070, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    if (SwitchAnimationByName(geom, 0) == 0) {
        return 0;
    }
    APPLY_CURRENT_ANIMATION_FRAME_SPRITE(sprite, desc, elem)
    SET_ANIMATION_ACT("A");
    return 1;
}

RVA(0x00110110, 0x39)
i32 CTileTriggerTransition::TransitionAct() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}
