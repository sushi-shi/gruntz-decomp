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
#include <Gruntz/BattlezData.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
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
    m_wwdObject->ApplyName("GAME_WORMHOLE");
    SwitchGeometry("GAME_WORMHOLE", 0);
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TELEPORT)
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    i32 kind = m_object->m_smarts;
    CShadeTable* color;
    if (kind == -1) {
        CLightFxMgr* pump = g_gameReg->m_logicPump;
        color = pump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
    } else {
        color = g_gameReg->m_logicPump->m_tables[kind];
    }
    CWwdGameObjectA* s = m_object;
    s->m_drawActive = 1;
    s->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    s->m_drawFillArg = color;
}

RVA(0x0003fed0, 0xa9)
i32 CWormhole::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag == SERIAL_POSTLOAD) {

        i32 kind = m_object->m_smarts;
        CShadeTable* color;
        if (kind == -1) {

            CLightFxMgr* pump = g_gameReg->m_logicPump;
            color = pump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
        } else {
            color = g_gameReg->m_logicPump->m_tables[kind];
        }

        CWwdGameObjectA* s = m_object;
        s->m_drawActive = 1;
        s->m_drawFillCmd = SHADE_DST_BY_SRC_16;
        s->m_drawFillArg = color;
    }
    return 1;
}

RVA(0x00040050, 0x102)
void CWormhole::FireActivation(i32 idx) {
    if (*CActRegPool<CWormhole>::s_table.ResolveEntry(idx) != 0) {
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

    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CWwdGameObjectA* g = m_wwdObject;
    if (!IsAniCursorComplete(&g->m_animCursor)) {
        return 0;
    }
    g->m_flags |= 0x10000;

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
            AnimWorkerObj* aux = obj->m_animWorker;
            if (aux->m_notify == &CreateTeleporter && obj->m_screenX == tx && obj->m_screenY == ty
                && aux->m_logic != NULL) {
                static_cast<CTeleporter*>(aux->m_logic)->ReapplyConfig();
            }
        }
    } while (pos != NULL);
    return 0;
}

RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(2);
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_PUDDLE)
    m_wwdObject->ApplyName("GRUNTZ_GRUNTPUDDLE");
    SwitchGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_pending = 1;
    m_placed = 0;
}

RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CGruntPuddle>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
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
i32 CGruntPuddle::Place(i32 gruntType, i32 placeIndex, i32 color, i32 placeConfig) {
    CWwdGameObjectA* o = m_object;
    m_tileX = o->m_screenX >> TILE_SHIFT_PX;
    m_tileY = o->m_screenY >> TILE_SHIFT_PX;
    m_placeConfig = placeConfig;
    m_gruntType = gruntType;
    m_placeIndex = placeIndex;
    CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(placeIndex, 0);
    CWwdGameObjectA* obj = m_object;
    obj->m_drawActive = 1;
    obj->m_drawFillCmd = SHADE_PAL_16;
    obj->m_drawFillArg = rec;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    if (color == 0) {
        m_placed = 1;
        m_pending = 0;
        SwitchGeometry(g_puddleSpriteKey, 0);
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
        i32 flags;
        if (static_cast<u32>(tx) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(ty) < static_cast<u32>(grid->m_height)) {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        } else {
            flags = 1;
        }
        if ((flags & BRICKZ_BLOCKED_MASK) != 0 || (flags & 0x2) != 0) {
            m_wwdObject->m_flags |= 0x10000;
            CPtrList& list = g_gameReg->m_cmdGrid->m_baseList;
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
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_wwdObject;
    if (IsAniCursorComplete(&o->m_animCursor)) {
        if (m_placed == 0) {
            SwitchGeometry(g_puddleSpriteKey, 0);
            m_placed = 1;
            m_pending = 0;
        } else {
            o->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
    }
    return 0;
}

RVA(0x00040e50, 0x170)
i32 CGruntPuddle::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_tileX, sizeof(m_tileX));
            ar->Write(&m_tileY, sizeof(m_tileY));
            ar->Write(&m_pending, sizeof(m_pending));
            ar->Write(&m_placed, sizeof(m_placed));
            ar->Write(&m_placeConfig, sizeof(m_placeConfig));
            ar->Write(&m_gruntType, sizeof(m_gruntType));
            ar->Write(&m_placeIndex, sizeof(m_placeIndex));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_tileX, sizeof(m_tileX));
            ar->Read(&m_tileY, sizeof(m_tileY));
            ar->Read(&m_pending, sizeof(m_pending));
            ar->Read(&m_placed, sizeof(m_placed));
            ar->Read(&m_placeConfig, sizeof(m_placeConfig));
            ar->Read(&m_gruntType, sizeof(m_gruntType));
            ar->Read(&m_placeIndex, sizeof(m_placeIndex));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(m_placeIndex, 0);
            if (sel == NULL) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CGameObject* obj = m_object;
            obj->m_drawFillArg = sel;
            obj->m_drawActive = 1;
            obj->m_drawFillCmd = SHADE_PAL_16;
            break;
        }
    }
    return 1;
}

// @early-stop
// Sole residue: the two tile-snap statements. Retail colours the m_object
// pointer ecx and the loaded coordinate eax, which lets cl pick the 2-byte
// `and al,0xe0`; we get the pair the other way round and spend `and ecx,-0x20`.
// docs/patterns/inplace-tile-snap-register-pair-is-canonical.md.
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_armClock = 0;
    m_interval = 0;
    SetObjectFlags(0x2000002);
    CWwdGameObjectA* o = m_object;
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

    CWwdGameObjectA* s = m_object;
    CShadeTable* colorEntry = g_gameReg->m_logicPump->m_tables[s->m_health];
    s->m_drawActive = 1;
    s->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    s->m_drawFillArg = colorEntry;
}

RVA(0x000412c0, 0x63)
i32 CTeleporter::ReapplyConfig() {
    m_wwdObject->ApplyName("GAME_WORMHOLE");
    SwitchGeometry("GAME_TELEPORTEROPEN", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_armed = 1;
    m_tickHandled = 0;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x00041350, 0xee)
i32 CTeleporter::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    // one cursor over the adjacent m_armClock/m_interval pair - retail hoists it
    // above the arms and advances it, rather than re-lea'ing each member.
    i64* clocks = &m_armClock;
    if (tag != SERIAL_SAVE) {
        if (tag == SERIAL_LOAD) {
            ar->Read(clocks, sizeof(*clocks));
            ar->Read(clocks + 1, sizeof(*clocks));
        }
    } else {
        ar->Write(clocks, sizeof(*clocks));
        ar->Write(clocks + 1, sizeof(*clocks));
    }
    switch (tag) {
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
    if ((*e) != 0) {
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
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CAniAdvanceCursor* cur = &m_wwdObject->m_animCursor;
    if (cur->m_finished == 0) {
        return 0;
    }
    if (cur->m_frameTicksLeft != 0) {
        return 0;
    }

    m_interval = static_cast<u32>(m_object->m_animWorker->m_speed);
    m_armClock = static_cast<u32>(g_frameTime);
    SwitchGeometry("GAME_TELEPORTER", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    return 0;
}

RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* a = m_wwdObject;
    if (IsAniCursorComplete(&a->m_animCursor)) {
        if (static_cast<TeleporterKind>(m_object->m_smarts) == TELEPORTER_SINGLE_USE) {
            a->m_flags |= 0x10000;
        } else {
            a->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 0;
    }

    CGruntzMgr* mgr;
    if (m_tickHandled == 0) {
        CWwdGameObjectA* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (CGameLevel::PointInRect(&mgr->m_viewBounds, x, y)) {
            (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_teleportWanted = 1;
        }
    }
    mgr = g_gameReg;
    if (m_armed == 0) {
        return 0;
    }

    CWwdGameObjectA* o = m_object;
    if (o->m_animWorker->m_speed != 0) {
        i64 delta = static_cast<i64>(g_frameTime) - m_armClock;
        if (delta >= m_interval) {
            SwitchGeometry("GAME_TELEPORTERCLOSE", 0);
            m_object->m_animWorker->m_speed = 0;
            m_tickHandled = 1;
            return 0;
        }
    }

    i32 outA;
    i32 outB;
    CGrunt* found = mgr->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &outB, &outA, 1);
    if (found == NULL) {
        return 0;
    }

    if (static_cast<TeleporterKind>(m_object->m_smarts) == TELEPORTER_SECRET) {
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, 1, 1);
        g_gameReg->m_scoreHud->m_secretsFound++;
        SwitchGeometry("GAME_TELEPORTERCLOSE", 0);
        CWwdGameObjectA* s = m_object;
        CWwdGameObjectA* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
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
            spawned->m_animWorker->m_speed = 0;
        }
    } else {
        CWwdGameObjectA* s = m_object;
        CWwdGameObjectA* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
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
        SwitchGeometry("GAME_TELEPORTERCLOSE", 0);
    }

    m_armed = 0;
    m_tickHandled = 1;
    mgr = g_gameReg;
    CGrunt* current;
    if ((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetCount() != 1) {
        current = NULL;
    } else {
        Coord* rec = (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->HeadRec();
        current =
            (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_grid[rec->m_x * TM_GRID_COLS + rec->m_y];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}
