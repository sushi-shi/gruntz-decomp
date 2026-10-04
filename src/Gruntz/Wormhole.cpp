#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/Wormhole.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Globals.h>
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
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/Play.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

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
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    SetImageSetByName("GAME_WORMHOLE");
    SwitchAnimationByName("GAME_WORMHOLE", 0);
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_TELEPORT);
    SET_ANIMATION_ACT("A");
    i32 kind = m_object->GetSmarts();
    CShadeTable* color;
    if (kind == -1) {
        CLightFxMgr* lightFxMgr = g_gameReg->GetLightFxMgr();
        color = lightFxMgr->GetShadeTable(g_buteMgr.GetInt("Wormhole", "EntranceColor", 3));
    } else {
        color = g_gameReg->GetLightFxMgr()->GetShadeTable(kind);
    }
    CWwdSpriteObject* s = m_object;
    s->SetDrawFill(SHADE_DST_BY_SRC_16, color);
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

        i32 kind = m_object->GetSmarts();
        CShadeTable* color;
        if (kind == -1) {

            CLightFxMgr* lightFxMgr = g_gameReg->GetLightFxMgr();
            color = lightFxMgr->GetShadeTable(g_buteMgr.GetInt("Wormhole", "EntranceColor", 3));
        } else {
            color = g_gameReg->GetLightFxMgr()->GetShadeTable(kind);
        }

        CWwdSpriteObject* s = m_object;
        s->SetDrawFill(SHADE_DST_BY_SRC_16, color);
    }
    return 1;
}

RVA(0x00040050, 0x102)
void CWormhole::FireActivation(i32 idx) {
    DispatchRegisteredAct(this, idx);
}

RVA(0x000401b0, 0x18d)
void RegisterWormholeLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = &CActRegPool<CWormhole>::s_table[idx];
    *dslot = static_cast<CActHandler>(&CWormhole::SpawnPartners);
}

RVA(0x000403b0, 0xa5)
i32 CWormhole::SpawnPartners() {

    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    CWwdSpriteObject* g = m_wwdObject;
    if (!g->m_animationCursor.IsComplete()) {
        return 0;
    }
    g->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));

    i32 tx = m_object->m_speedX;
    i32 ty = m_object->m_speedY;
    if (tx == 0 || ty == 0) {
        return 0;
    }

    CObList* list = g_gameReg->World()->ChildGroup()->GetList();
    if (list == NULL) {
        return 0;
    }
    POSITION pos = list->GetHeadPosition();
    if (pos == NULL) {
        return 0;
    }
    do {
        CGameObject* obj = g_gameReg->World()->ChildGroup()->NextChild(pos);
        if (obj != NULL) {
            CLogicRecord* record = obj->GetLogicRecord();
            if (record->GetDispatch() == &DispatchTeleporterLogic && obj->m_screenX == tx
                && obj->m_screenY == ty && record->UserLogic() != NULL) {
                static_cast<CTeleporter*>(record->UserLogic())->ReapplyConfig();
            }
        }
    } while (pos != NULL);
    return 0;
}

RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_GRUNT_PUDDLE);
    SetImageSetByName("GRUNTZ_GRUNTPUDDLE");
    SwitchAnimationByName("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    SET_ANIMATION_ACT("A");
    Hide();
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_pending = true;
    m_placed = false;
}

RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x000408b0, 0x2ac)
void RegisterLogic() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CGruntPuddle>::s_table[id] = static_cast<CActHandler>(&CGruntPuddle::Idle);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CGruntPuddle>::s_table[id2] = static_cast<CActHandler>(&CGruntPuddle::Remove);
}

RVA(0x00040c10, 0x3)
i32 CGruntPuddle::Idle() {
    return 0;
}

RVA(0x00040c30, 0xb3)
i32 CGruntPuddle::Place(i32 playerIndex, i32 colorIndex, b32 animatePlacement, i32 gaugePoints) {
    CWwdSpriteObject* o = m_object;
    m_tileX = o->m_screenX >> TILE_SHIFT_PX;
    m_tileY = o->m_screenY >> TILE_SHIFT_PX;
    m_gaugePoints = gaugePoints;
    m_playerIndex = playerIndex;
    m_colorIndex = colorIndex;
    CShadeTable* shade = g_gameReg->GruntPalettes()->GetShadeTable(colorIndex, 0);
    CWwdSpriteObject* sprite = m_object;
    sprite->SetDrawFill(SHADE_PAL_16, shade);
    m_wwdObject->Show();
    SET_ANIMATION_ACT("B");
    if (animatePlacement == false) {
        m_placed = true;
        m_pending = false;
        SwitchAnimationByName(g_puddleSpriteKey, 0);
    }
    return 1;
}

RVA(0x00040d20, 0xe3)
i32 CGruntPuddle::Remove() {
    if (m_placed != false) {
        CGruntzMgr* reg = g_gameReg;
        i32 ty = m_tileY;
        CMapMgr* grid = reg->GetTileGrid();
        i32 tx = m_tileX;
        i32 flags = grid->CellFlagsAt(tx, ty);
        if ((flags & BRICKZ_BLOCKED_MASK) != 0 || (flags & IDX(CELL_FLAG_SPECIAL)) != 0) {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            CTriggerMgr* manager = g_gameReg->GetTriggerMgr();
            POSITION pos = manager->GetPuddleHeadPosition();
            while (pos != NULL) {
                POSITION current = pos;
                if (manager->GetNextPuddle(pos) == this) {
                    manager->RemovePuddleAt(current);
                    return 0;
                }
            }
        }
    }
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* o = m_wwdObject;
    if (o->m_animationCursor.IsComplete()) {
        if (m_placed == false) {
            SwitchAnimationByName(g_puddleSpriteKey, 0);
            m_placed = true;
            m_pending = false;
        } else {
            o->Hide();
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
            ar->Write(&m_colorIndex, sizeof(m_colorIndex));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_tileX, sizeof(m_tileX));
            ar->Read(&m_tileY, sizeof(m_tileY));
            ar->Read(&m_pending, sizeof(m_pending));
            ar->Read(&m_placed, sizeof(m_placed));
            ar->Read(&m_gaugePoints, sizeof(m_gaugePoints));
            ar->Read(&m_playerIndex, sizeof(m_playerIndex));
            ar->Read(&m_colorIndex, sizeof(m_colorIndex));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* sel = g_gameReg->GruntPalettes()->GetShadeTable(m_colorIndex, 0);
            if (sel == NULL) {
                sel = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
            }
            CGameObject* obj = m_object;
            obj->SetDrawFill(SHADE_PAL_16, sel);
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_TELEPORT);
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    LoadColors();
    ReapplyConfig();
}

RVA(0x000411f0, 0xa0)
void CTeleporter::LoadColors() {
    TeleporterKind kind = static_cast<TeleporterKind>(m_object->GetSmarts());

    if (kind == TELEPORTER_SECRET) {

        if (m_object->GetHealth() == 0) {
            m_object->SetHealth(g_buteMgr.GetInt("Wormhole", "SecretColor", 1));
        }
    } else if (kind == TELEPORTER_SINGLE_USE) {

        if (m_object->GetHealth() == 0) {
            m_object->SetHealth(g_buteMgr.GetInt("Wormhole", "SingleUseColor", 2));
        }
    } else {

        if (m_object->GetHealth() == 0) {
            m_object->SetHealth(g_buteMgr.GetInt("Wormhole", "NormalColor", 4));
        }
    }

    CWwdSpriteObject* s = m_object;
    CShadeTable* colorEntry = g_gameReg->GetLightFxMgr()->GetShadeTable(s->GetHealth());
    s->SetDrawFill(SHADE_DST_BY_SRC_16, colorEntry);
}

RVA(0x000412c0, 0x63)
i32 CTeleporter::ReapplyConfig() {
    SetImageSetByName("GAME_WORMHOLE");
    SwitchAnimationByName("GAME_TELEPORTEROPEN", 0);
    SET_ANIMATION_ACT("A");
    m_armed = true;
    m_tickHandled = false;
    m_wwdObject->Show();
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
    m_armTiming.Serialize(ar, mode, typeId, object);
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
    DispatchRegisteredAct(this, coord);
}

RVA(0x00041680, 0x2ac)
void CTeleporter_RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CTeleporter>::s_table[id] = static_cast<CActHandler>(&CTeleporter::Begin);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CTeleporter>::s_table[id2] = static_cast<CActHandler>(&CTeleporter::Update);
}

RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, g_engineFrameDelta)
    if (!cur->IsComplete()) {
        return 0;
    }

    m_armTiming.Start(m_object->GetLogicRecord()->GetSpeed());
    SwitchAnimationByName("GAME_TELEPORTER", 0);
    SET_ANIMATION_ACT("B");
    return 0;
}

RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* a = m_wwdObject;
    if (a->m_animationCursor.IsComplete()) {
        if (static_cast<TeleporterKind>(m_object->GetSmarts()) == TELEPORTER_SINGLE_USE) {
            a->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        } else {
            a->Hide();
        }
        return 0;
    }

    CGruntzMgr* mgr;
    if (m_tickHandled == false) {
        CWwdSpriteObject* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (::PtInRect(&mgr->m_viewBounds, x, y)) {
            (static_cast<CTriggerMgr*>(mgr->GetTriggerMgr()))->m_teleportWanted = true;
        }
    }
    mgr = g_gameReg;
    if (m_armed == false) {
        return 0;
    }

    CWwdSpriteObject* o = m_object;
    if (o->GetLogicRecord()->GetSpeed() != 0) {
        i64 delta = static_cast<i64>(g_frameTime) - m_armTiming.m_start;
        if (delta >= m_armTiming.m_interval) {
            SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
            m_object->GetLogicRecord()->SetSpeed(0);
            m_tickHandled = true;
            return 0;
        }
    }

    i32 playerIndex;
    i32 unitIndex;
    CGrunt* found = mgr->GetTriggerMgr()
                        ->FindGruntAtPoint(o->m_screenX, o->m_screenY, &playerIndex, &unitIndex, 1);
    if (found == NULL) {
        return 0;
    }

    if (static_cast<TeleporterKind>(m_object->GetSmarts()) == TELEPORTER_SECRET) {
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, true, true);
        g_gameReg->GetGameStats()->m_secretsFound++;
        SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
        CWwdSpriteObject* s = m_object;
        CWwdSpriteObject* spawned = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            s->GetPowerup() * TILE_SIZE_PX + TILE_HALF_PX,
            s->GetDamage() * TILE_SIZE_PX + TILE_HALF_PX,
            0,
            "Teleporter",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        if (spawned != NULL) {
            spawned->SetSmarts(IDX(TELEPORTER_SINGLE_USE));
            spawned->SetHealth(m_object->GetHealth());
            spawned->SetSpeedX(m_object->GetScore());
            spawned->SetSpeedY(m_object->GetPoints());
            spawned->GetLogicRecord()->SetSpeed(0);
        }
    } else {
        CWwdSpriteObject* s = m_object;
        CWwdSpriteObject* spawned = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            s->m_speedX * TILE_SIZE_PX + TILE_HALF_PX,
            s->m_speedY * TILE_SIZE_PX + TILE_HALF_PX,
            0,
            "Wormhole",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        spawned->SetSpeedX(m_object->m_screenX);
        spawned->SetSpeedY(m_object->m_screenY);
        spawned->SetSmarts(m_object->GetHealth());
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, false, false);
        SwitchAnimationByName("GAME_TELEPORTERCLOSE", 0);
    }

    m_armed = false;
    m_tickHandled = true;
    mgr = g_gameReg;
    if (found == mgr->GetTriggerMgr()->SoleSelectedGrunt() && playerIndex == g_curPlayer) {
        CGameObject* g = found->GetSpriteObject();
        (static_cast<CPlay*>(mgr->m_curState))->SetCameraPosition(g->m_screenX, g->m_screenY);
    }
    return 0;
}
