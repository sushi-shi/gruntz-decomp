#include <rva.h>

#include <Gruntz/Wormhole.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

RVA_DYNINIT(0x0003ffb0, 0xa, CActRegPool<CWormhole>::s_table)
RVA_DYNINIT(0x0003ffd0, 0x15, CActRegPool<CWormhole>::s_table)
RVA_DYNINIT(0x00040000, 0xe, CActRegPool<CWormhole>::s_table)
RVA_DYNINIT(0x00040020, 0x1f, CActRegPool<CWormhole>::s_table)
template<> DATA(0x00244660)
CActReg CActRegPool<CWormhole>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000406b0, 0xa, CActRegPool<CGruntPuddle>::s_table)
RVA_DYNINIT(0x000406d0, 0x15, CActRegPool<CGruntPuddle>::s_table)
RVA_DYNINIT(0x00040700, 0xe, CActRegPool<CGruntPuddle>::s_table)
RVA_DYNINIT(0x00040720, 0x1f, CActRegPool<CGruntPuddle>::s_table)
template<> DATA(0x002445e8)
CActReg CActRegPool<CGruntPuddle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x00041480, 0xa, CActRegPool<CTeleporter>::s_table)
RVA_DYNINIT(0x000414a0, 0x15, CActRegPool<CTeleporter>::s_table)
RVA_DYNINIT(0x000414d0, 0xe, CActRegPool<CTeleporter>::s_table)
RVA_DYNINIT(0x000414f0, 0x1f, CActRegPool<CTeleporter>::s_table)
template<> DATA(0x002446b0)
CActReg CActRegPool<CTeleporter>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

DATA(0x0020c1c0)
char g_puddleSpriteKey[] = "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2";

RVA_COMPGEN(0x00010950, 0x1e, ??_GCWormhole@@UAEPAXI@Z)
RVA_COMPGEN(0x00010980, 0x44, ??1CWormhole@@UAE@XZ)

RVA_COMPGEN(0x00010ce0, 0x1e, ??_GCGruntPuddle@@UAEPAXI@Z)
RVA_COMPGEN(0x00010d10, 0x44, ??1CGruntPuddle@@UAE@XZ)

RVA_COMPGEN(0x00010da0, 0x1e, ??_GCTeleporter@@UAEPAXI@Z)
RVA_COMPGEN(0x00010dd0, 0x44, ??1CTeleporter@@UAE@XZ)

RVA(0x0003fc70, 0x1db)
CWormhole::CWormhole(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(0x2000002);
    SetImageSetByName("GAME_WORMHOLE");
    SwitchAnimationByName("GAME_WORMHOLE", 0);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TELEPORT)
    SET_ANIMATION_ACT("A");
    i32 kind = m_object->m_smarts;
    CShadeTable* color;
    if (kind == -1) {
        CLightFxMgr* lightFxMgr = g_gameReg->m_lightFxMgr;
        color = lightFxMgr->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
    } else {
        color = g_gameReg->m_lightFxMgr->m_tables[kind];
    }
    CWwdSpriteObject* s = m_object;
    SET_DRAW_FILL(s, SHADE_DST_BY_SRC_16, color);
}

RVA(0x0003fed0, 0xa9)
i32 CWormhole::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode == SERIAL_POSTLOAD) {

        i32 kind = m_object->m_smarts;
        CShadeTable* color;
        if (kind == -1) {

            CLightFxMgr* lightFxMgr = g_gameReg->m_lightFxMgr;
            color = lightFxMgr->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
        } else {
            color = g_gameReg->m_lightFxMgr->m_tables[kind];
        }

        CWwdSpriteObject* s = m_object;
        SET_DRAW_FILL(s, SHADE_DST_BY_SRC_16, color);
    }
    return 1;
}

RVA(0x00040050, 0x102)
void CWormhole::FireActivation(i32 idx) {
    if (*CActRegPool<CWormhole>::s_table.ResolveEntry(idx) != NULL) {
        CActHandler fn = *CActRegPool<CWormhole>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

RVA(0x000401b0, 0x18d)
void RegisterWormholeLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = CActRegPool<CWormhole>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CWormhole::SpawnPartners);
}

RVA(0x000403b0, 0xa5)
i32 CWormhole::SpawnPartners() {

    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    CWwdSpriteObject* g = m_wwdObject;
    if (!IsAniCursorComplete(&g->m_animationCursor)) {
        return 0;
    }
    g->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);

    i32 tx = m_object->m_speedX;
    i32 ty = m_object->m_speedY;
    if (tx == 0 || ty == 0) {
        return 0;
    }

    CObList* list = &g_gameReg->m_world->m_childGroup->m_list;
    if (list == NULL) {
        return 0;
    }
    POSITION pos = list->GetHeadPosition();
    if (pos == NULL) {
        return 0;
    }
    do {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj != NULL) {
            CLogicRecord* record = obj->m_logicRecord;
            if (record->m_dispatch == &DispatchTeleporterLogic && obj->m_screenX == tx
                && obj->m_screenY == ty && record->m_userLogic != NULL) {
                static_cast<CTeleporter*>(record->m_userLogic)->ReapplyConfig();
            }
        }
    } while (pos != NULL);
    return 0;
}

RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(2);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_PUDDLE)
    SetImageSetByName("GRUNTZ_GRUNTPUDDLE");
    SwitchAnimationByName("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    SET_ANIMATION_ACT("A");
    Hide();
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_pending = 1;
    m_placed = 0;
}

RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CGruntPuddle>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CGruntPuddle>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}

RVA(0x000408b0, 0x2ac)
void RegisterLogic() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntPuddle::Idle);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntPuddle::Remove);
}

RVA(0x00040c10, 0x3)
i32 CGruntPuddle::Idle() {
    return 0;
}

RVA(0x00040c30, 0xb3)
i32 CGruntPuddle::Place(i32 playerIndex, i32 moveIcon, i32 animatePlacement, i32 gaugePoints) {
    CWwdSpriteObject* o = m_object;
    m_tileX = o->m_screenX >> TILE_SHIFT_PX;
    m_tileY = o->m_screenY >> TILE_SHIFT_PX;
    m_gaugePoints = gaugePoints;
    m_playerIndex = playerIndex;
    m_moveIcon = moveIcon;
    CShadeTable* shade = g_gameReg->m_spriteFactory->GetSel(moveIcon, 0);
    CWwdSpriteObject* sprite = m_object;
    SET_DRAW_FILL(sprite, SHADE_PAL_16, shade);
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    SET_ANIMATION_ACT("B");
    if (animatePlacement == 0) {
        m_placed = 1;
        m_pending = 0;
        SwitchAnimationByName(g_puddleSpriteKey, 0);
    }
    return 1;
}

RVA(0x00040d20, 0xe3)
i32 CGruntPuddle::Remove() {
    if (m_placed != 0) {
        CGruntzMgr* reg = g_gameReg;
        i32 ty = m_tileY;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tx = m_tileX;
        i32 flags = grid->CellFlagsAt(tx, ty);
        if ((flags & BRICKZ_BLOCKED_MASK) != 0 || (flags & 0x2) != 0) {
            SetObjectFlags(0x10000);
            CPtrList& list = g_gameReg->m_triggerMgr->m_baseList;
            POSITION pos = list.GetHeadPosition();
            while (pos != NULL) {
                POSITION current = pos;
                if (list.GetNext(pos) == this) {
                    list.RemoveAt(current);
                    return 0;
                }
            }
        }
    }
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* o = m_wwdObject;
    if (IsAniCursorComplete(&o->m_animationCursor)) {
        if (m_placed == 0) {
            SwitchAnimationByName(g_puddleSpriteKey, 0);
            m_placed = 1;
            m_pending = 0;
        } else {
            o->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
    }
    return 0;
}

RVA(0x00040e50, 0x170)
i32 CGruntPuddle::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_tileX, sizeof(m_tileX));
            ar->Write(&m_tileY, sizeof(m_tileY));
            ar->Write(&m_pending, sizeof(m_pending));
            ar->Write(&m_placed, sizeof(m_placed));
            ar->Write(&m_gaugePoints, sizeof(m_gaugePoints));
            ar->Write(&m_playerIndex, sizeof(m_playerIndex));
            ar->Write(&m_moveIcon, sizeof(m_moveIcon));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_tileX, sizeof(m_tileX));
            ar->Read(&m_tileY, sizeof(m_tileY));
            ar->Read(&m_pending, sizeof(m_pending));
            ar->Read(&m_placed, sizeof(m_placed));
            ar->Read(&m_gaugePoints, sizeof(m_gaugePoints));
            ar->Read(&m_playerIndex, sizeof(m_playerIndex));
            ar->Read(&m_moveIcon, sizeof(m_moveIcon));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(m_moveIcon, 0);
            if (sel == NULL) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CGameObject* obj = m_object;
            SET_DRAW_FILL_ARG_FIRST(obj, SHADE_PAL_16, sel);
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_armClock = 0;
    m_interval = 0;
    SetObjectFlags(0x2000002);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TELEPORT)
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    LoadColors();
    ReapplyConfig();
}

RVA(0x000411f0, 0xa0)
void CTeleporter::LoadColors() {
    TeleporterKind kind = static_cast<TeleporterKind>(m_object->m_smarts);

    if (kind == TELEPORTER_SECRET) {

        if (m_object->m_health == 0) {
            m_object->m_health = g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1);
        }
    } else if (kind == TELEPORTER_SINGLE_USE) {

        if (m_object->m_health == 0) {
            m_object->m_health = g_buteMgr.GetIntDef("Wormhole", "SingleUseColor", 2);
        }
    } else {

        if (m_object->m_health == 0) {
            m_object->m_health = g_buteMgr.GetIntDef("Wormhole", "NormalColor", 4);
        }
    }

    CWwdSpriteObject* s = m_object;
    CShadeTable* colorEntry = g_gameReg->m_lightFxMgr->m_tables[s->m_health];
    SET_DRAW_FILL(s, SHADE_DST_BY_SRC_16, colorEntry);
}

RVA(0x000412c0, 0x63)
i32 CTeleporter::ReapplyConfig() {
    SetImageSetByName("GAME_WORMHOLE");
    SwitchAnimationByName("GAME_TELEPORTEROPEN", 0);
    SET_ANIMATION_ACT("A");
    m_armed = 1;
    m_tickHandled = 0;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x00041350, 0xee)
i32 CTeleporter::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    i64* clocks = &m_armClock;
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(clocks, sizeof(*clocks));
            ar->Read(clocks + 1, sizeof(*clocks));
        }
    } else {
        ar->Write(clocks, sizeof(*clocks));
        ar->Write(clocks + 1, sizeof(*clocks));
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_armed, sizeof(m_armed));
            ar->Write(&m_tickHandled, sizeof(m_tickHandled));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_armed, sizeof(m_armed));
            ar->Read(&m_tickHandled, sizeof(m_tickHandled));
            break;
        case SERIAL_POSTLOAD:
            LoadColors();
            break;
    }
    return 1;
}

RVA(0x00041520, 0x102)
void CTeleporter::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTeleporter>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CTeleporter>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x00041680, 0x2ac)
void CTeleporter_RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CTeleporter::Begin);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CTeleporter::Update);
}

RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, g_engineFrameDelta)
    if (cur->m_finished == 0) {
        return 0;
    }
    if (cur->m_frameTicksLeft != 0) {
        return 0;
    }

    m_interval = static_cast<u32>(m_object->m_logicRecord->m_speed);
    m_armClock = static_cast<u32>(g_frameTime);
    SwitchAnimationByName("GAME_TELEPORTER", 0);
    SET_ANIMATION_ACT("B");
    return 0;
}

RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* a = m_wwdObject;
    if (IsAniCursorComplete(&a->m_animationCursor)) {
        if (static_cast<TeleporterKind>(m_object->m_smarts) == TELEPORTER_SINGLE_USE) {
            a->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        } else {
            a->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 0;
    }

    CGruntzMgr* mgr;
    if (m_tickHandled == 0) {
        CWwdSpriteObject* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (CGameLevel::PointInRect(&mgr->m_viewBounds, x, y)) {
            (static_cast<CTriggerMgr*>(mgr->m_triggerMgr))->m_teleportWanted = 1;
        }
    }
    mgr = g_gameReg;
    if (m_armed == 0) {
        return 0;
    }

    CWwdSpriteObject* o = m_object;
    if (o->m_logicRecord->m_speed != 0) {
        i64 delta = static_cast<i64>(g_frameTime) - m_armClock;
        if (delta >= m_interval) {
            SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
            m_object->m_logicRecord->m_speed = 0;
            m_tickHandled = 1;
            return 0;
        }
    }

    i32 playerIndex;
    i32 unitIndex;
    CGrunt* found =
        mgr->m_triggerMgr->HitTestCell(o->m_screenX, o->m_screenY, &playerIndex, &unitIndex, 1);
    if (found == NULL) {
        return 0;
    }

    if (static_cast<TeleporterKind>(m_object->m_smarts) == TELEPORTER_SECRET) {
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, 1, 1);
        g_gameReg->m_gameStats->m_secretsFound++;
        SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
        CWwdSpriteObject* s = m_object;
        CWwdSpriteObject* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            s->m_powerup * 32 + 16,
            s->m_damage * 32 + 16,
            0,
            "Teleporter",
            0x40003
        );
        if (spawned != NULL) {
            spawned->m_smarts = IDX(TELEPORTER_SINGLE_USE);
            spawned->m_health = m_object->m_health;
            spawned->m_speedX = m_object->m_score;
            spawned->m_speedY = m_object->m_points;
            spawned->m_logicRecord->m_speed = 0;
        }
    } else {
        CWwdSpriteObject* s = m_object;
        CWwdSpriteObject* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            s->m_speedX * 32 + 16,
            s->m_speedY * 32 + 16,
            0,
            "Wormhole",
            0x40003
        );
        spawned->m_speedX = m_object->m_screenX;
        spawned->m_speedY = m_object->m_screenY;
        spawned->m_smarts = m_object->m_health;
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, 0, 0);
        SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
    }

    m_armed = 0;
    m_tickHandled = 1;
    mgr = g_gameReg;
    CGrunt* current;
    if ((static_cast<CTriggerMgr*>(mgr->m_triggerMgr))->m_recList.GetCount() != 1) {
        current = NULL;
    } else {
        Coord* rec = (static_cast<CTriggerMgr*>(mgr->m_triggerMgr))->HeadRec();
        current = (static_cast<CTriggerMgr*>(mgr->m_triggerMgr))
                      ->m_units[rec->m_x * TM_UNITS_PER_PLAYER + rec->m_y];
    }
    if (found == current && playerIndex == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}
