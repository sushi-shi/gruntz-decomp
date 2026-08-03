#include <rva.h>

#include <Gruntz/Wormhole.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/BattlezData.h>
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
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

template<> DATA(0x00244660)
CActReg CActRegPool<CWormhole>::s_table(2000, 2010);
template<> DATA(0x002445e8)
CActReg CActRegPool<CGruntPuddle>::s_table(2000, 2010);
template<> DATA(0x002446b0)
CActReg CActRegPool<CTeleporter>::s_table(2000, 2010);

VTBL(CGruntPuddle, 0x001e8124);
VTBL(CWormhole, 0x001e817c);
DATA(0x0020c1c0)
char g_puddleSpriteKey[] = "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2";

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(v, msg, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = ActNameSlots();
    while (n-- != 0) {
        if (list != NULL) {
            list->CString::~CString();
        }
        list++;
    }
}

RVA_COMPGEN(0x00010950, 0x1e, ??_GCWormhole@@UAEPAXI@Z)
RVA_COMPGEN(0x00010980, 0x44, ??1CWormhole@@UAE@XZ)

RVA_COMPGEN(0x00010ce0, 0x1e, ??_GCGruntPuddle@@UAEPAXI@Z)
RVA_COMPGEN(0x00010d10, 0x44, ??1CGruntPuddle@@UAE@XZ)

RVA_COMPGEN(0x00010da0, 0x1e, ??_GCTeleporter@@UAEPAXI@Z)
RVA_COMPGEN(0x00010dd0, 0x44, ??1CTeleporter@@UAE@XZ)

// @early-stop
RVA(0x0003fc70, 0x1db)
CWormhole::CWormhole(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 0x2000002;
    m_wwdObject->ApplyName("GAME_WORMHOLE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_WORMHOLE", 0);
    if (m_object->m_sortKey != 0x1869f) {
        m_object->m_sortKey = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    i32 kind = m_object->m_smarts;
    CShadeTable* color;
    if (kind == -1) {
        color =
            g_gameReg->m_logicPump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
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
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CWormhole>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CWormhole::SpawnPartners);
}

RVA(0x000403b0, 0xa5)
i32 CWormhole::SpawnPartners() {

    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CWwdGameObjectA* g = m_wwdObject;
    if (g->m_animCursor.m_finished == 0 || g->m_animCursor.m_frameTicksLeft != 0) {
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

// @early-stop
RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    if (m_object->m_sortKey != 0xa) {
        m_object->m_sortKey = 0xa;
        m_object->m_flags |= 0x20000;
    }
    m_wwdObject->ApplyName("GRUNTZ_GRUNTPUDDLE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_stateFlags |= 1;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntPuddle::Idle);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntPuddle::Remove);
}

RVA(0x00040c10, 0x3)
i32 CGruntPuddle::Idle() {
    return 0;
}

RVA(0x00040c30, 0xb3)

i32 CGruntPuddle::Place(i32 gruntType, i32 placeIndex, i32 color, i32 a3) {
    CWwdGameObjectA* o = m_object;
    m_tileX = o->m_screenX >> 5;
    m_tileY = o->m_screenY >> 5;
    m_placeArg3 = a3;
    m_gruntType = gruntType;
    m_placeIndex = placeIndex;
    CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(placeIndex, 0);
    CWwdGameObjectA* obj = m_object;
    obj->m_drawActive = 1;
    obj->m_drawFillCmd = SHADE_PAL_16;
    obj->m_drawFillArg = rec;
    m_wwdObject->m_stateFlags &= ~1;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    if (placeIndex == 0) {
        m_placed = 1;
        m_pending = 0;
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry(g_puddleSpriteKey, 0);
    }
    return 1;
}

// @early-stop
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
        if ((flags & 0x939) != 0 || (flags & 0x2) != 0) {
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
    if (o->m_animCursor.m_finished != 0 && o->m_animCursor.m_frameTicksLeft == 0) {
        if (m_placed != 0) {
            o->m_stateFlags |= 1;
        } else {
            m_value = o->m_animCursor.m_animation;
            o->ApplyLookupGeometry(g_puddleSpriteKey, 0);
            m_placed = 1;
            m_pending = 0;
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
            ar->Write(&m_tileX, 4);
            ar->Write(&m_tileY, 4);
            ar->Write(&m_pending, 4);
            ar->Write(&m_placed, 4);
            ar->Write(&m_placeArg3, 4);
            ar->Write(&m_gruntType, 4);
            ar->Write(&m_placeIndex, 4);
            break;
        case SERIAL_LOAD:
            ar->Read(&m_tileX, 4);
            ar->Read(&m_tileY, 4);
            ar->Read(&m_pending, 4);
            ar->Read(&m_placed, 4);
            ar->Read(&m_placeArg3, 4);
            ar->Read(&m_gruntType, 4);
            ar->Read(&m_placeIndex, 4);
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

RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_armClock = 0;
    m_interval = 0;
    m_wwdObject->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0x1869f) {
        m_object->m_sortKey = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    LoadColors();
    ReapplyConfig();
}

RVA(0x000411f0, 0xa0)
void CTeleporter::LoadColors() {

    if (m_object->m_smarts == 2) {

        if (m_object->m_health == 0) {
            m_object->m_health = g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1);
        }
    } else if (m_object->m_smarts == 1) {

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
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_TELEPORTEROPEN", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_armed = 1;
    m_tickHandled = 0;
    m_wwdObject->m_stateFlags &= ~1;
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
    if (tag != SERIAL_SAVE) {
        if (tag == SERIAL_LOAD) {
            ar->Read(&m_armClock, 8);
            ar->Read(&m_interval, 8);
        }
    } else {
        ar->Write(&m_armClock, 8);
        ar->Write(&m_interval, 8);
    }
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_armed, 4);
            ar->Write(&m_tickHandled, 4);
            break;
        case SERIAL_LOAD:
            ar->Read(&m_armed, 4);
            ar->Read(&m_tickHandled, 4);
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CTeleporter::Begin);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CTeleporter::Update);
}

// @early-stop
RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    if (m_wwdObject->m_animCursor.m_finished == 0) {
        return 0;
    }
    if (m_wwdObject->m_animCursor.m_frameTicksLeft != 0) {
        return 0;
    }

    m_interval = static_cast<u32>(m_object->m_animWorker->m_speed);
    m_armClock = static_cast<u32>(g_frameTime);
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_object->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    return 0;
}

// @early-stop
RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* a = m_wwdObject;
    if (a->m_animCursor.m_finished != 0 && a->m_animCursor.m_frameTicksLeft == 0) {
        if (m_object->m_smarts == 1) {
            a->m_flags |= 0x10000;
        } else {
            a->m_stateFlags |= 1;
        }
        return 0;
    }

    CGruntzMgr* mgr;
    if (m_tickHandled == 0) {
        CWwdGameObjectA* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (x < mgr->m_viewBounds.right && x >= mgr->m_viewBounds.left
            && y < mgr->m_viewBounds.bottom && y >= mgr->m_viewBounds.top) {
            (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_teleportWanted = 1;
        }
    }
    mgr = g_gameReg;
    if (m_armed == 0) {
        return 0;
    }

    CWwdGameObjectA* o = m_object;
    if (o->m_animWorker->m_speed != 0) {
        i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_armClock;
        if (delta >= m_interval) {
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
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

    if (m_object->m_smarts == 2) {
        found->TryTeleportToCell(m_object->m_speedX, m_object->m_speedY, 1, 1);
        g_gameReg->m_scoreHud->m_secretsFound++;
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
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
            spawned->m_smarts = 1;
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
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
    }

    m_armed = 0;
    m_tickHandled = 1;
    mgr = g_gameReg;
    CGrunt* current;
    if ((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetCount() != 1) {
        current = NULL;
    } else {
        i32* pair =
            static_cast<i32*>((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetHead());
        i32 row = pair[0];
        i32 col = pair[1];
        current = (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_grid[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}

VTBL(CTeleporter, 0x001e80cc);
