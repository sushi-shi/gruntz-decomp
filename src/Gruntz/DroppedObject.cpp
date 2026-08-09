#include <rva.h>

#include <Gruntz/DroppedObject.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/CombatCueKind.h>
#include <Gruntz/DroppedObjectShadow.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/State.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <string.h>

DATA(0x001ea9f0)
const double g_objDropDiv = 32.0;
DATA(0x001eaa00)
const double g_dropFallBias = -0.5;

template<> DATA(0x0024be90)
CActReg CActRegPool<CObjectDropper>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024bed8)
CActReg CActRegPool<CDroppedObject>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024bf00)
CActReg CActRegPool<CDroppedObjectShadow>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

struct CString;

static inline CActHandler* DropLookup(i32 coord) {
    return (CActRegPool<CDroppedObject>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x000124c0, 0x1e, ??_GCObjectDropper@@UAEPAXI@Z)
RVA_COMPGEN(0x000124f0, 0x44, ??1CObjectDropper@@UAE@XZ)

RVA_COMPGEN(0x00012580, 0x1e, ??_GCDroppedObject@@UAEPAXI@Z)
RVA_COMPGEN(0x000125b0, 0x44, ??1CDroppedObject@@UAE@XZ)

RVA_COMPGEN(0x00012640, 0x1e, ??_GCDroppedObjectShadow@@UAEPAXI@Z)
RVA_COMPGEN(0x00012670, 0x44, ??1CDroppedObjectShadow@@UAE@XZ)

RVA(0x000c5630, 0xf4)
i32 CreateObjectDropper(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (aux->WorkerAct()) {
        case ACT_UNINITIALISED: {
            aux->SetWorkerAct(ACT_LIVE);
            CObjectDropper* h = new CObjectDropper(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            aux->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            aux->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            aux->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            aux->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            aux->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c5770, 0xf1)
i32 CreateDroppedObject(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (aux->WorkerAct()) {
        case ACT_UNINITIALISED: {
            aux->SetWorkerAct(ACT_LIVE);
            CDroppedObject* h = new CDroppedObject(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            aux->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            aux->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            aux->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            aux->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            aux->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c58b0, 0xf1)
i32 CreateDroppedObjectShadow(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (aux->WorkerAct()) {
        case ACT_UNINITIALISED: {
            aux->SetWorkerAct(ACT_LIVE);
            CDroppedObjectShadow* h = new CDroppedObjectShadow(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            aux->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            aux->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            aux->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            aux->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            aux->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop
// only residue: cl schedules the CObjectDropper vptr store one pair of i64 zero-stores
// earlier than retail, which renames two registers downstream.
RVA(0x000c59f0, 0x3e3)
CObjectDropper::CObjectDropper(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_lastDropTime = 0;
    m_dropInterval = 0;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_OBJECTDROPPER", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;

    i32 snapX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 snapY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenX = snapX;
    m_object->m_screenY = snapY;
    m_posX = static_cast<double>(snapX);
    m_posY = static_cast<double>(snapY);
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_ACTOR_FRONT) {
        o->m_sortKey = SORTKEY_ACTOR_FRONT;
        o->m_flags |= 0x20000;
    }

    CDDrawWorker* frameSet = m_wwdObject->m_frameSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_travelDx = 0;
            m_travelDy = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_travelDx = 1;
            m_travelDy = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_travelDx = 0;
            m_travelDy = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = 0;
    m_lastDropTileX = -1;
    m_lastDropTileY = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<u32>(time));
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_scrollMode = 1;
    }
    CShadeTable* sel = g_gameReg->m_logicPump->m_tables[5];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    m_object->m_drawFillArg = sel;
    m_lastDropTime = 0;
    m_dropInterval = 0;
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
}

RVA(0x000c5f80, 0x102)
void CObjectDropper::FireActivation(i32 actId) {
    if ((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId)))) != 0) {
        (this->*((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId))))))();
    }
}

RVA(0x000c60e0, 0x18d)
void CObjectDropper::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CObjectDropper::Update);
}

RVA(0x000c62e0, 0x2dd)
i32 CObjectDropper::Update() {
    if (static_cast<i64>(g_frameTime) - m_lastDropTime >= m_dropInterval) {
        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
            CWwdGameObjectA* o = m_object;
            RECT box;
            box.left = o->m_screenX - o->m_layer->m_anchorX + 7;
            box.right = o->m_screenX + o->m_layer->m_anchorX - 7;
            box.top = o->m_screenY - o->m_layer->m_anchorY + 7;
            box.bottom = o->m_screenY + o->m_layer->m_anchorY - 7;
            i32 tx;
            i32 ty;
            CGrunt* found =
                g_gameReg->m_cmdGrid
                    ->FindGruntAt(o->m_screenX, o->m_screenY, &o->m_area, &tx, &ty, &box);
            if (found != NULL) {
                if (m_lastDropTileX != tx || m_lastDropTileY != ty) {
                    if (m_scrollMode == 0 || tx == 0) {
                        CGameObject* fo = found->m_object;
                        i32 fx = fo->m_screenX;
                        i32 fy = fo->m_screenY;
                        CMapMgr* plane = g_gameReg->m_tileGrid;
                        i32 cx = fx >> TILE_SHIFT_PX;
                        i32 cy = fy >> TILE_SHIFT_PX;
                        u32 flags;
                        if (static_cast<u32>(cx) >= static_cast<u32>(plane->m_width)
                            || static_cast<u32>(cy) >= static_cast<u32>(plane->m_height)) {
                            flags = 1;
                        } else {

                            flags = static_cast<u32>(plane->m_rows[cy][cx].m_flags);
                        }
                        if ((flags & 2) == 0) {
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, fx, fy, 0, "DroppedObjectShadow", 0x40003);
                            m_lastDropTileX = tx;
                            m_lastDropTileY = ty;
                            m_dropInterval =
                                g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperDelay", 1000);
                            m_lastDropTime = g_frameTime;
                        }
                    }
                }
            }
        }
    }

    m_wwdObject->m_animCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    double drift = static_cast<double>(g_frameDelta) * m_speed;
    if (m_travelDx > 0) {
        m_posX += drift;
        if (m_posX >= static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_wrapW)) {
            m_posX = DATA_COMPGEN(0x001ea9f8, fp_1ea9f8, 0.0);
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDx < 0) {
        m_posX -= drift;
        if (m_posX < 0.0) {
            m_posX = static_cast<double>((g_gameReg->m_world->m_level->m_mainPlane->m_wrapW - 1));
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }
    if (m_travelDy > 0) {
        m_posY += drift;
        if (m_posY > static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_wrapH)) {
            m_posY = 0.0;
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDy < 0) {
        m_posY -= drift;
        if (m_posY < 0.0) {
            m_posY = static_cast<double>((g_gameReg->m_world->m_level->m_mainPlane->m_wrapH - 1));
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }

    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    return 0;
}

RVA(0x000c6680, 0x1b4)
i32 CObjectDropper::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }

    SerBandPair(ar, tag, &m_dropTiming);

    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_speed, sizeof(m_speed));
            ar->Write(&m_posX, sizeof(m_posX));
            ar->Write(&m_posY, sizeof(m_posY));
            ar->Write(&m_travelDx, sizeof(m_travelDx));
            ar->Write(&m_travelDy, sizeof(m_travelDy));
            ar->Write(&m_lastDropTileX, sizeof(m_lastDropTileX));
            ar->Write(&m_lastDropTileY, sizeof(m_lastDropTileY));
            ar->Write(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_speed, sizeof(m_speed));
            ar->Read(&m_posX, sizeof(m_posX));
            ar->Read(&m_posY, sizeof(m_posY));
            ar->Read(&m_travelDx, sizeof(m_travelDx));
            ar->Read(&m_travelDy, sizeof(m_travelDy));
            ar->Read(&m_lastDropTileX, sizeof(m_lastDropTileX));
            ar->Read(&m_lastDropTileY, sizeof(m_lastDropTileY));
            ar->Read(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
            CWwdGameObjectA* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c68b0, 0x1f5)
CDroppedObject::CDroppedObject(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->ApplyName("LEVEL_OBJECTDROPPER_OBJECT");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_DROPPEDOBJECT", 0);
    m_wwdObject->m_flags |= 0x2000002;
    i32 adjY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_landY = adjY;
    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = adjY - g_buteMgr.GetIntDef("Hazardz", "DroppedObjectYOffset", 0x140);
    m_fallY = static_cast<double>(m_object->m_screenY);
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_ACTOR_FRONT) {
        o->m_sortKey = SORTKEY_ACTOR_FRONT;
        o->m_flags |= 0x20000;
    }
    m_timePerTile =
        g_objDropDiv
        / static_cast<double>(g_buteMgr.GetDwordDef("Hazardz", "DroppedObjectTimePerTile", 0x3e8));
}

RVA(0x000c6bd0, 0x102)
void CDroppedObject::FireActivation(i32 coord) {
    CActHandler* e = DropLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = DropLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000c6d30, 0x2ac)
void CDroppedObject::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id)) =

        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceFall);

    ACT_NAME_ID(id2, "B")
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceImpactAnimation);
}

RVA(0x000c7090, 0x230)
i32 CDroppedObject::AdvanceFall() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    m_fallY = static_cast<double>(g_frameDelta) * m_timePerTile + m_fallY;
    i32 landed = static_cast<i32>((m_fallY - g_dropFallBias));
    if (landed > m_landY) {
        i32 x = m_object->m_screenX;
        CMapMgr* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            i32 cx = x >> TILE_SHIFT_PX;
            i32 cy = m_landY >> TILE_SHIFT_PX;
            if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
                && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
                cell = g->m_rows[cy][cx].m_flags;
            } else {
                cell = 1;
            }
        }
        if ((cell & 0x900) == 0) {
            if (cell & IDX(CELL_FLAG_SPECIAL)) {
                if (cell == IDX(CELL_FLAG_REVEALED_POWERUP)) {
                    m_wwdObject->m_flags |= 0x10000;
                } else {
                    switch (g_gameReg->m_curState->m_levelType) {
                        case AREA_HIGH_ON_SWEETZ:
                        case AREA_HIGH_ROLLERZ:
                        case AREA_GRUNTZ_IN_SPACE:
                            m_wwdObject->m_flags |= 0x10000;
                            // fall through
                        case AREA_MINIATURE_MASTERZ:
                        default:
                            if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                                CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    x,
                                    m_landY,
                                    SORTKEY_ACTOR_BEHIND,
                                    "Particlez",
                                    0x40003
                                );
                                if (s != NULL) {
                                    s->ApplyName("LEVEL_DEATHSPLASH");
                                    s->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
                            break;
                    }
                }
            }
        } else {
            if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                CWwdGameObjectA* s =
                    g_gameReg->m_world->m_childGroup
                        ->CreateSprite(0, x, m_landY, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
                if (s != NULL) {
                    s->ApplyName("GAME_WATER");
                    s->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
        }
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTHIT", 0);
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("B");
        g_gameReg->m_cmdGrid->CombatCue(m_object->m_screenX, m_landY, 1, CUE_SQUASH, -1);
        return 0;
    }
    m_object->m_screenY = landed;
    return 0;
}

RVA(0x000c7350, 0x39)
i32 CDroppedObject::AdvanceAnimation() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_timePerTile, sizeof(m_timePerTile));
            ar->Write(&m_fallY, sizeof(m_fallY));
            ar->Write(&m_landY, sizeof(m_landY));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_timePerTile, sizeof(m_timePerTile));
            ar->Read(&m_fallY, sizeof(m_fallY));
            ar->Read(&m_landY, sizeof(m_landY));
            break;
    }
    return 1;
}

RVA(0x000c7490, 0x1a6)
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->ApplyName("LEVEL_OBJECTDROPPER_SHADOW");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTSHADOW", 0);
    m_wwdObject->m_flags |= 0x2000002;
    CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
    CWwdGameObjectA* draw = m_object;
    draw->m_drawActive = 1;
    draw->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    draw->m_drawFillArg = fill;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_ACTOR_BEHIND) {
        o->m_sortKey = SORTKEY_ACTOR_BEHIND;
        o->m_flags |= 0x20000;
    }
}

RVA(0x000c7750, 0x102)
void CDroppedObjectShadow::FireActivation(i32 coord) {
    if ((*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(coord)))) != 0) {
        (this->*((*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(coord))))))();
    }
}

RVA(0x000c78b0, 0x18d)
void CDroppedObjectShadow::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObjectShadow::Advance);
}

// @early-stop
RVA(0x000c7ab0, 0x67)
i32 CDroppedObjectShadow::Advance() {
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) == WWDDRAW_EFFECT_FRAME) {
        CWwdGameObjectA* o = m_object;
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, o->m_screenX, o->m_screenY, 0, "DroppedObject", 0x40003);
    }
    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c7b40, 0x76)
i32 CDroppedObjectShadow::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, mode, c, d)) {
        return 0;
    }
    if (!Chain(ar, mode, c, d)) {
        return 0;
    }
    if (mode == SERIAL_POSTLOAD) {
        CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
        o->m_drawFillArg = fill;
    }
    return 1;
}

RVA(0x000c7be0, 0x5)
i32 CDroppedObject::AdvanceImpactAnimation() {
    return AdvanceAnimation();
}
