#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/StaticHazard.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
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
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CStaticHazard>::s_table(ACT_ID_FIRST, ACT_ID_LAST);


i32 DispatchStaticHazardLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CStaticHazard)
}

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
    SET_RECT_XY_EXTENTS(
        m_object->m_area,
        m_object->m_screenX - 7,
        m_object->m_area.left + 14,
        m_object->m_screenY - 7,
        m_object->m_area.top + 14
    );
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    m_object->m_animationCursor.m_consumeDraw = 0;
    m_object->m_smarts = IDX(g_areaHazardDeath);
    m_activeWindow = 0;
    m_idleWindow = m_object->m_damage;
    m_pulseEpoch = g_frameTime;
    CAniElement* entry = MapFind<CAniElement>(
        g_gameReg->World()->m_animRegistry->m_animations,
        "LEVEL_STATICHAZARDGO"
    );
    if (entry != NULL) {
        i32 durationMs = entry->m_durationMs;
        m_activeWindow = g_buteMgr.GetInt("Hazardz", "AniPad", 0x64) + durationMs;
    } else {
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x461);
    }
    if (m_object->m_damage == 0) {
        m_idleWindow = m_activeWindow;
    }
}

void CStaticHazard::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CStaticHazard::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CStaticHazard>::s_table[id]) =
        static_cast<CActHandler>(&CStaticHazard::UpdateIdleState);

    ACT_NAME_ID(id2, "B")
    (CActRegPool<CStaticHazard>::s_table[id2]) =
        static_cast<CActHandler>(&CStaticHazard::UpdateActiveState);
}

i32 CStaticHazard::UpdateIdleState() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != false && reg->GetGameMode() == GAMEMODE_QUESTZ) {
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
            if (static_cast<u32>(col) < static_cast<u32>(grid->GetWidth())
                && static_cast<u32>(row) < static_cast<u32>(grid->GetHeight())) {
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
        CGrunt* victim = g_gameReg->m_triggerMgr->HitTestCell(
            m_object->m_screenX,
            m_object->m_screenY,
            &playerIndex,
            &unitIndex,
            0
        );
        if (victim != NULL) {
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
        if (static_cast<u32>(col) < static_cast<u32>(grid->GetWidth())
            && static_cast<u32>(row) < static_cast<u32>(grid->GetHeight())) {
            grid->m_rowInts[row][col * 7] |= 0x8000000;
        }
    } else {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 row = m_tileRow;
        i32 col = m_tileCol;
        if (static_cast<u32>(col) < static_cast<u32>(grid->GetWidth())
            && static_cast<u32>(row) < static_cast<u32>(grid->GetHeight())) {
            grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
        }
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
    }
    {
        CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
        if (sub->IsComplete()) {
            SwitchAnimationByName("LEVEL_STATICHAZARDIDLE", 0);
            {APPLY_CURRENT_ANIMATION_FRAME_SPRITE("LEVEL_STATICHAZARD", d, e)} CMapMgr* grid =
                g_gameReg->m_tileGrid;
            i32 row = m_tileRow;
            i32 col = m_tileCol;
            if (static_cast<u32>(col) < static_cast<u32>(grid->GetWidth())
                && static_cast<u32>(row) < static_cast<u32>(grid->GetHeight())) {
                grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

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
