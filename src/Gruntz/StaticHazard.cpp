#include <rva.h>

#include <Gruntz/StaticHazard.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Enums.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordState.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* found = NULL;
    MapLookup(map, name, found);
    return found;
}

RVA_DYNINIT(0x000fbb50, 0xa, CActRegPool<CStaticHazard>::s_table)
RVA_DYNINIT(0x000fbb70, 0x15, CActRegPool<CStaticHazard>::s_table)
RVA_DYNINIT(0x000fbba0, 0xe, CActRegPool<CStaticHazard>::s_table)
RVA_DYNINIT(0x000fbbc0, 0x1f, CActRegPool<CStaticHazard>::s_table)
template<> DATA(0x0024e3d0)
CActReg CActRegPool<CStaticHazard>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00012b00, 0x1e, ??_GCStaticHazard@@UAEPAXI@Z)
RVA_COMPGEN(0x00012b30, 0x44, ??1CStaticHazard@@UAE@XZ)

struct CString;

static inline CActHandler* HaznLookup(i32 coord) {
    return (CActRegPool<CStaticHazard>::s_table.ResolveEntry(coord));
}

inline void DispatchUnhandledLogicEvent(CUserLogic* sub) {
    DispatchLogicEvent(sub);
}

RVA(0x000fb660, 0xf1)
i32 DispatchStaticHazardLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (static_cast<u32>(record->EventCode())) {
        case LOGICREC_INIT:
            record->SetEventCode(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CStaticHazard(owner);
                obj->Activate();
                record->m_userLogic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            record->m_userLogic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            record->m_userLogic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            record->m_userLogic->AfterSave();
            break;
        case LOGICREC_OP_52:
            record->m_userLogic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            record->m_userLogic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x000fb7a0, 0x2f0)
CStaticHazard::CStaticHazard(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    SwitchAnimationByName("LEVEL_STATICHAZARDIDLE", 0);
    {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)}

    SNAP_OBJECT_TO_TILE_CENTER(m_object) CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, 0)
    m_tileCol = m_object->m_screenX >> TILE_SHIFT_PX;
    m_tileRow = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_health = 0;
    switch (g_gameReg->m_curState->m_levelType) {
        case AREA_TROUBLE_IN_THE_TROPICZ:
        case AREA_HIGH_ON_SWEETZ:
        case AREA_MINIATURE_MASTERZ:
        case AREA_GRUNTZ_IN_SPACE:
            m_object->m_health = m_object->m_screenY + 0x186b0;
            break;
        default:
            break;
    }
    m_object->m_area.left = m_object->m_screenX - 7;
    m_object->m_area.right = m_object->m_area.left + 14;
    m_object->m_area.top = m_object->m_screenY - 7;
    m_object->m_area.bottom = m_object->m_area.top + 14;
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    m_object->m_animationCursor.m_consumeDraw = 0;
    m_object->m_smarts = IDX(g_areaHazardDeath);
    m_activeWindow = 0;
    m_idleWindow = m_object->m_damage;
    m_pulseEpoch = g_frameTime;
    CAniElement* entry =
        LookupAnimation(g_gameReg->m_world->m_animRegistry->m_animations, "LEVEL_STATICHAZARDGO");
    if (entry != NULL) {
        i32 durationMs = entry->m_durationMs;
        m_activeWindow = g_buteMgr.GetIntDef("Hazardz", "AniPad", 0x64) + durationMs;
    } else {
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x461);
    }
    if (m_object->m_damage == 0) {
        m_idleWindow = m_activeWindow;
    }
}

RVA(0x000fbbf0, 0x102)
void CStaticHazard::FireActivation(i32 coord) {
    CActHandler* e = HaznLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = HaznLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000fbd50, 0x2ac)
void CStaticHazard::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    (*CActRegPool<CStaticHazard>::s_table.ResolveEntryCallReport(id)) =
        static_cast<CActHandler>(&CStaticHazard::UpdateIdleState);

    ACT_NAME_ID(id2, "B")
    (*CActRegPool<CStaticHazard>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<CActHandler>(&CStaticHazard::UpdateActiveState);
}

RVA(0x000fc0b0, 0xb2)
i32 CStaticHazard::UpdateIdleState() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != false && reg->m_gameMode == GAMEMODE_QUESTZ) {
        return 0;
    }
    u32 phase = g_frameTime - m_pulseEpoch;
    u32 base = static_cast<u32>(m_object->m_points);
    if (phase <= base) {
        return 0;
    }
    phase -= base;
    u32 span = m_idleWindow + m_activeWindow;
    if (phase % span > static_cast<u32>(m_activeWindow)) {
        return 0;
    }
    m_fired = true;
    SwitchAnimationByName("LEVEL_STATICHAZARDGO", 0);
    {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)} SET_ANIMATION_ACT("B");
    return 0;
}

// @early-stop
RVA(0x000fc1a0, 0x33b)
i32 CStaticHazard::UpdateActiveState() {
    u32 phase = (g_frameTime - m_pulseEpoch) - static_cast<u32>(m_object->m_points);
    u32 rem = phase % static_cast<u32>((m_idleWindow + m_activeWindow));
    if (rem > static_cast<u32>(m_activeWindow)) {

        if (m_fired != false) {

            if (m_object->m_damage == 0) {

                SwitchAnimationByName("LEVEL_STATICHAZARDGO", 0);
                {
                    APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)
                } CWwdSpriteObject* o = m_object;
                SET_SORT_KEY_IF_CHANGED(o, 0)
                m_fired = false;
                return 0;
            }

            SET_ANIMATION_ACT("A");
            SwitchAnimationByName("LEVEL_STATICHAZARDIDLE", 0);
            {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)} CWwdSpriteObject* o =
                m_object;
            SET_SORT_KEY_IF_CHANGED(o, 0)

            CMapMgr* grid = g_gameReg->m_tileGrid;
            i32 row = m_tileRow;
            i32 col = m_tileCol;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
            }
            return 0;
        }
    } else if (m_fired == false && m_object->m_damage == 0) {

        SwitchAnimationByName("LEVEL_STATICHAZARDGO", 0);
        {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)} CWwdSpriteObject* o =
            m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
        m_fired = true;
        return 0;
    }

    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) == WWDDRAW_EFFECT_FRAME) {
        i32 playerIndex, unitIndex;
        if (g_gameReg->m_triggerMgr
                ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &playerIndex, &unitIndex, 0)
            != NULL) {
            g_gameReg->m_triggerMgr->StartUnitDeath(
                playerIndex,
                unitIndex,
                static_cast<GruntDeathType>(m_object->m_smarts),
                -1
            );
        }
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, o->m_health)
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 row = m_tileRow;
        i32 col = m_tileCol;
        if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[row][col * 7] |= 0x8000000;
        }
    } else {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 row = m_tileRow;
        i32 col = m_tileCol;
        if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
        }
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
    }
    {
        CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
        if (IsAniCursorComplete(sub)) {
            SwitchAnimationByName("LEVEL_STATICHAZARDIDLE", 0);
            {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)} CMapMgr* grid =
                g_gameReg->m_tileGrid;
            i32 row = m_tileRow;
            i32 col = m_tileCol;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

RVA(0x000fc5b0, 0xf5)
i32 CStaticHazard::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* arc = ar;
    switch (mode) {
        case SERIAL_SAVE:
            arc->Write(&m_pulseEpoch, sizeof(m_pulseEpoch));
            arc->Write(&m_activeWindow, sizeof(m_activeWindow));
            arc->Write(&m_idleWindow, sizeof(m_idleWindow));
            arc->Write(&m_fired, sizeof(m_fired));
            arc->Write(&m_tileCol, sizeof(m_tileCol));
            arc->Write(&m_tileRow, sizeof(m_tileRow));
            break;
        case SERIAL_LOAD:
            arc->Read(&m_pulseEpoch, sizeof(m_pulseEpoch));
            arc->Read(&m_activeWindow, sizeof(m_activeWindow));
            arc->Read(&m_idleWindow, sizeof(m_idleWindow));
            arc->Read(&m_fired, sizeof(m_fired));
            arc->Read(&m_tileCol, sizeof(m_tileCol));
            arc->Read(&m_tileRow, sizeof(m_tileRow));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM(ar, arc, mode, typeId, object)
}
