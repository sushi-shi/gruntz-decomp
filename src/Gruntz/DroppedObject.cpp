#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/ObjectDropper.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Wap32/zBitVec.h>
#include <Io/FileMem.h>
#include <Gruntz/DroppedObject.h>
#include <Gruntz/DroppedObjectShadow.h>
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/TypeKeyColl.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/State.h>

#include <string.h>
#include <Gruntz/ActName.h>
#include <Gruntz/XferArchive.h>
#include <rva.h>
#include <rva.h>

VTBL(CDroppedObjectShadow, 0x001e787c);
VTBL(CDroppedObject, 0x001e78d4);
VTBL(CObjectDropper, 0x001e7a9c);
DATA(0x001ea9f0)
const double g_objDropDiv = 32.0;
DATA(0x001eaa00)
double g_dropFallBias = -0.5;

template<> DATA(0x0024be90)
CActReg CActRegPool<CObjectDropper>::s_table(2000, 2010);
template<> DATA(0x0024bed8)
CActReg CActRegPool<CDroppedObject>::s_table(2000, 2010);
template<> DATA(0x0024bf00)
CActReg CActRegPool<CDroppedObjectShadow>::s_table(2000, 2010);

struct CString;

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return g_typeColl.Elem(id);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
        return g_typeColl.Elem(id);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

static inline CActHandler* DropLookup(i32 coord) {
    return (CActRegPool<CDroppedObject>::s_table.ResolveEntry(coord));
}

static inline CString* ActNameLookupCallReport(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return g_typeColl.Elem(id);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
        return g_typeColl.Elem(id);
    }
    g_typeColl.Report(g_errOutOfMem, 0xc);
    return g_typeColl.Scratch();
}

typedef enum DropperDir {
    DROPDIR_NORTH = 1,
    DROPDIR_EAST = 2,
    DROPDIR_SOUTH = 3,
    DROPDIR_WEST = 4
} DropperDir;

RVA_COMPGEN(0x000124c0, 0x1e, ??_GCObjectDropper@@UAEPAXI@Z)
RVA_COMPGEN(0x000124f0, 0x44, ??1CObjectDropper@@UAE@XZ)

RVA_COMPGEN(0x00012580, 0x1e, ??_GCDroppedObject@@UAEPAXI@Z)
RVA_COMPGEN(0x000125b0, 0x44, ??1CDroppedObject@@UAE@XZ)

RVA_COMPGEN(0x00012640, 0x1e, ??_GCDroppedObjectShadow@@UAEPAXI@Z)
RVA_COMPGEN(0x00012670, 0x44, ??1CDroppedObjectShadow@@UAE@XZ)

RVA(0x000c5630, 0xf4)
i32 CreateObjectDropper(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (static_cast<u32>(aux->ActKey())) {
        case 0: {
            aux->SetActKey(0x3e8);
            CObjectDropper* h = new CObjectDropper(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
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
    switch (static_cast<u32>(aux->ActKey())) {
        case 0: {
            aux->SetActKey(0x3e8);
            CDroppedObject* h = new CDroppedObject(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
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
    switch (static_cast<u32>(aux->ActKey())) {
        case 0: {
            aux->SetActKey(0x3e8);
            CDroppedObjectShadow* h = new CDroppedObjectShadow(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x000c59f0, 0x3e3)
CObjectDropper::CObjectDropper(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_lastDropTime = 0;
    m_dropInterval = 0;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_OBJECTDROPPER", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_posY = static_cast<double>(snapY);
    if (o->m_sortKey != 0xcf851) {
        o->m_sortKey = 0xcf851;
        o->m_flags |= 0x20000;
    }

    CWwdGameObjectA* obj38 = m_38;
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            o->m_12c = DROPDIR_NORTH;
            m_travelDx = 0;
            m_travelDy = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            o->m_12c = DROPDIR_EAST;
            m_travelDx = 1;
            m_travelDy = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            o->m_12c = DROPDIR_SOUTH;
            m_travelDx = 0;
            m_travelDy = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            o->m_12c = DROPDIR_WEST;
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = 0;
    m_lastDropTileX = -1;
    m_lastDropTileY = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<i64>(static_cast<u32>(time)));
    if (g_gameReg->m_134 == 1) {
        m_scrollMode = 1;
    }
    CShadeTable* sel = g_gameReg->m_logicPump->m_tables[5];
    o->m_drawActive = 1;
    o->m_drawFillCmd = 7;
    o->m_drawFillArg = sel;
    m_lastDropTime = 0;
    m_dropInterval = 0;
    o->m_area.left = 1;
    o->m_area.right = 1;
    o->m_area.top = 1;
    o->m_area.bottom = 1;
}

RVA(0x000c5f80, 0x102)
void CObjectDropper::FireActivation(i32 actId) {
    if ((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId)))) != 0) {
        (this->*((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId))))))();
    }
}

RVA(0x000c60e0, 0x18d)
void CObjectDropper::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (cnt-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CObjectDropper::Update);
}

RVA(0x000c62e0, 0x2dd)
i32 CObjectDropper::Update() {
    if (static_cast<i64>(g_frameTime) - m_lastDropTime >= m_dropInterval) {
        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_134 != 1) {
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
            if (found != 0) {
                if (m_lastDropTileX != tx || m_lastDropTileY != ty) {
                    if (m_scrollMode == 0 || tx == 0) {
                        CGameObject* fo = found->m_object;
                        i32 fx = fo->m_screenX;
                        i32 fy = fo->m_screenY;
                        CMapMgr* plane = g_gameReg->m_tileGrid;
                        i32 cx = fx >> 5;
                        i32 cy = fy >> 5;
                        u32 flags;
                        if (static_cast<u32>(cx) >= static_cast<u32>(plane->m_width)
                            || static_cast<u32>(cy) >= static_cast<u32>(plane->m_height)) {
                            flags = 1;
                        } else {

                            flags = static_cast<u32>(plane->m_rows[cy][cx].m_0);
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

    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    double drift = static_cast<double>(g_frameDelta) * m_speed;
    if (m_travelDx > 0) {
        m_posX += drift;
        if (m_posX >= static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_wrapW)) {
            m_posX = 0.0;
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
i32 CObjectDropper::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }

    i64* p = &m_lastDropTime;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 1;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 1;
            ar->Read(p, 8);
            break;
    }

    switch (tag) {
        case 4:
            ar->Write(&m_speed, 8);
            ar->Write(&m_posX, 8);
            ar->Write(&m_posY, 8);
            ar->Write(&m_travelDx, 4);
            ar->Write(&m_travelDy, 4);
            ar->Write(&m_lastDropTileX, 4);
            ar->Write(&m_lastDropTileY, 4);
            ar->Write(&m_scrollMode, 4);
            break;
        case 7:
            ar->Read(&m_speed, 8);
            ar->Read(&m_posX, 8);
            ar->Read(&m_posY, 8);
            ar->Read(&m_travelDx, 4);
            ar->Read(&m_travelDy, 4);
            ar->Read(&m_lastDropTileX, 4);
            ar->Read(&m_lastDropTileY, 4);
            ar->Read(&m_scrollMode, 4);
            break;
        case 8: {
            CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
            CWwdGameObjectA* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c68b0, 0x1f5)
CDroppedObject::CDroppedObject(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_OBJECT");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECT", 0);
    m_38->m_flags |= 0x2000002;
    i32 adjY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_landY = adjY;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = adjY - g_buteMgr.GetIntDef("Hazardz", "DroppedObjectYOffset", 0x140);
    m_fallY = static_cast<double>(m_object->m_screenY);
    if (m_object->m_sortKey != 0xcf851) {
        m_object->m_sortKey = 0xcf851;
        m_object->m_flags |= 0x20000;
    }
    m_timePerTile =
        32.0
        / static_cast<double>(
            static_cast<u32>(g_buteMgr.GetDwordDef("Hazardz", "DroppedObjectTimePerTile", 0x3e8))
        );
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id)) =

        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::ActA);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "B";
        g_typeCounter++;
    }
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::ActB);
}

// @early-stop
RVA(0x000c7090, 0x21b)
i32 CDroppedObject::ActA() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    m_fallY = static_cast<double>(g_frameDelta) * m_timePerTile + m_fallY;
    i32 landed = static_cast<i32>((m_fallY - g_dropFallBias));
    if (landed > m_landY) {
        i32 x = m_object->m_screenX;
        CMapMgr* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            i32 cx = x >> 5;
            i32 cy = m_landY >> 5;
            if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
                && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
                cell = g->m_rows[cy][cx].m_0;
            } else {
                cell = 1;
            }
        }
        if ((cell & 0x900) == 0) {
            if (cell & 2) {
                if (cell == 0x40) {
                    m_38->m_flags |= 0x10000;
                } else {
                    switch (g_gameReg->m_curState->m_levelType) {
                        case 4:
                        case 5:
                        case 8:
                            m_38->m_flags |= 0x10000;
                            // fall through
                        case 7:
                        default:
                            if (x < g_gameReg->m_viewBounds.right
                                && x >= g_gameReg->m_viewBounds.left
                                && m_landY < g_gameReg->m_viewBounds.bottom
                                && m_landY >= g_gameReg->m_viewBounds.top) {
                                CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    x,
                                    m_landY,
                                    0xcf84f,
                                    "Particlez",
                                    0x40003
                                );
                                if (s != 0) {
                                    s->ApplyName("LEVEL_DEATHSPLASH");
                                    s->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        case 6:
                            break;
                    }
                }
            }
        } else {
            if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
                && m_landY < g_gameReg->m_viewBounds.bottom
                && m_landY >= g_gameReg->m_viewBounds.top) {
                CWwdGameObjectA* s =
                    g_gameReg->m_world->m_childGroup
                        ->CreateSprite(0, x, m_landY, 0xcf84f, "Particlez", 0x40003);
                if (s != 0) {
                    s->ApplyName("GAME_WATER");
                    s->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
        }
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTHIT", 0);
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("B");
        g_gameReg->m_cmdGrid->CombatCue(m_object->m_screenX, m_landY, 1, 7, -1);
        return 0;
    }
    m_object->m_screenY = landed;
    return 0;
}

RVA(0x000c7350, 0x39)
i32 CDroppedObject::UserLogicVfunc5() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_finished != 0 && m_38->m_1a0.m_frameTicksLeft == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_timePerTile, 8);
            ar->Write(&m_fallY, 8);
            ar->Write(&m_landY, 4);
            break;
        case 7:
            ar->Read(&m_timePerTile, 8);
            ar->Read(&m_fallY, 8);
            ar->Read(&m_landY, 4);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x000c7490, 0x1a6)
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_SHADOW");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTSHADOW", 0);
    m_38->m_flags |= 0x2000002;
    m_object->m_drawFillArg = g_gameReg->m_logicPump->m_tables[5];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    if (m_object->m_sortKey != 0xcf84f) {
        m_object->m_sortKey = 0xcf84f;
        m_object->m_flags |= 0x20000;
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (cnt-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObjectShadow::Advance);
}

// @early-stop
RVA(0x000c7ab0, 0x67)
i32 CDroppedObjectShadow::Advance() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) == 2) {
        CWwdGameObjectA* o = m_object;
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, o->m_screenX, o->m_screenY, 0, "DroppedObject", 0x40003);
    }
    if (m_38->m_1a0.m_finished != 0 && m_38->m_1a0.m_frameTicksLeft == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c7b40, 0x76)
i32 CDroppedObjectShadow::SerializeMove(CFileMemBase* ar, i32 mode, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, mode, c, d)) {
        return 0;
    }
    if (!Chain(ar, mode, c, d)) {
        return 0;
    }
    if (mode == 8) {
        CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = fill;
    }
    return 1;
}

RVA(0x000c7be0, 0x5)
i32 CDroppedObject::ActB() {
    return UserLogicVfunc5();
}
