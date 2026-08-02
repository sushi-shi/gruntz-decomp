

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/GameRegistry.h>

#include <rva.h>
#include <Gruntz/GameLevel.h>
#include <math.h>
#include <string.h>
#include <Wap32/ZVec.h>
#include <Rez/FrameClock.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/KitchenSlime.h>
#include <Bute/ButeMgr.h>

template<> DATA(0x002461b0)
CActReg CActRegPool<CRollingBall>::s_table(2000, 2010);

VTBL(CRollingBall, 0x001e86fc);

static const double kMsPerSecond = 1000.0;

static i32 VtblResolve(void* ent) {
    return static_cast<CTileImageSet*>(ent)->GetCollisionAt(0, 0);
}

RVA_COMPGEN(0x00012f50, 0x1e, ??_GCRollingBall@@UAEPAXI@Z)
RVA_COMPGEN(0x00012f80, 0x44, ??1CRollingBall@@UAE@XZ)

// @early-stop
RVA(0x000af820, 0x40d)
CRollingBall::CRollingBall(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_explodeStartLo = 0;
    m_explodeWindowLo = 0;
    m_explodeStartHi = 0;
    m_explodeWindowHi = 0;
    m_value = m_wwdObject->m_1a0.m_14;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = 0x10 + (o->m_screenY & ~0x1f);
    o->m_screenX = snapX;
    m_subX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_subY = static_cast<double>(snapY);
    if (o->m_sortKey != 0x186a0 + snapY) {
        o->m_sortKey = snapY + 0x186a0;
        o->m_flags |= 0x20000;
    }

    CWwdGameObjectA* obj38 = m_wwdObject;
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
        const char* s;
        s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            o->m_12c = 1;
            m_stepDirX = 0;
            m_stepDirY = -1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            o->m_12c = 2;
            m_stepDirY = 0;
            m_stepDirX = 1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            o->m_12c = 3;
            m_stepDirY = 1;
            m_stepDirX = 0;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            o->m_12c = 4;
            m_stepDirY = 0;
            m_stepDirX = -1;
        }
    }

    i32 time = o->m_animWorker->m_bc;
    if (time == 0) {
        time = g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CGruntzMgr* reg = g_gameReg;
    if (0 != reg->m_isEasyMode && reg->m_134 == 1 && o->m_124 != 1) {
        time += 1000;
    }
    m_explodeWindowLo = o->m_118;
    m_explodeWindowHi = 0;
    m_explodeStartLo = g_frameTime;
    m_explodeStartHi = 0;
    m_targetY = snapY;
    m_targetX = snapY;
    m_explodeLatch = 0;
    m_fallLatch = 0;
    m_moveSpeed = g_slimeSpeedNum / static_cast<double>(static_cast<i64>(static_cast<u32>(time)));
    o->m_area.left = 0;
    o->m_area.right = 0;
    o->m_area.top = 0;
    o->m_area.bottom = 0;
    m_moveDeltaHi = 0;
    m_moveDeltaLo = 0;
}

RVA(0x000afde0, 0x102)
void CRollingBall::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CRollingBall>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CRollingBall>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000aff40, 0x18d)
void CRollingBall::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
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
    (*((CActRegPool<CRollingBall>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CRollingBall::Update);
}

RVA(0x000b0140, 0xa7a)
i32 CRollingBall::Update() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);

    CWwdGameObjectA* anim = m_wwdObject;
    if (anim->m_1a0.m_finished != 0 && anim->m_1a0.m_frameTicksLeft == 0) {
        anim->m_flags |= 0x10000;
        return 0;
    }
    if (m_explodeLatch != 0) {
        return 0;
    }

    CWwdGameObjectA* logic = m_object;
    if (logic->m_118 > 0) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_explodeStart64
            >= m_explodeWindow64) {
            m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EXPLOSION");
            m_value = m_wwdObject->m_1a0.m_14;
            m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLEXPLOSION", 0);
            CMapMgr* map = g_gameReg->m_tileGrid;
            CWwdGameObjectA* lg = m_object;
            i32 cx = lg->m_screenX >> 5;
            i32 cy = lg->m_screenY >> 5;
            if (static_cast<u32>(cx) < map->m_width && static_cast<u32>(cy) < map->m_height) {
                map->m_rowInts[cy][cx * 7] &= 0xefffffff;
            }
            m_explodeLatch = 1;
        }
    }

    if (m_fallLatch == 0) {
        CWwdGameObjectA* lg = m_object;
        i32 sx = lg->m_screenX;
        i32 sy = lg->m_screenY;
        if (sx < g_gameReg->m_viewBounds.right && sx >= g_gameReg->m_viewBounds.left
            && sy < g_gameReg->m_viewBounds.bottom && sy >= g_gameReg->m_viewBounds.top) {
            g_gameReg->m_cmdGrid->m_rollingballWanted = 1;
        }
        CWwdGameObjectA* lg2 = m_object;
        i32 hitA;
        i32 hitB;
        if (g_gameReg->m_cmdGrid
                ->FindGruntAt(lg2->m_screenX, lg2->m_screenY, &lg2->m_area, &hitA, &hitB, 0)) {
            g_gameReg->m_cmdGrid->CellDispatch(hitA, hitB, 2, -1);
        }
    }

    CWwdGameObjectA* cur = m_object;
    if (cur->m_screenX == m_targetX && cur->m_screenY == m_targetY) {

        g_gameReg->m_cmdGrid->WireTileSwitchLogic(0, m_targetX, m_targetY);
        g_gameReg->m_cmdGrid->ApplySwitch(0, m_targetX, m_targetY);

        i32 tx = m_targetX >> 5;
        i32 ty = m_targetY >> 5;
        CMapMgr* map = g_gameReg->m_tileGrid;
        if (static_cast<u32>(tx) < map->m_width && static_cast<u32>(ty) < map->m_height) {
            map->m_rowInts[ty][tx * 7] &= 0xefffffff;
        }
        CMapMgr* map2 = g_gameReg->m_tileGrid;
        i32 terrain;
        if (static_cast<u32>(tx) < map2->m_width && static_cast<u32>(ty) < map2->m_height) {
            terrain = map2->m_rowInts[ty][tx * 7];
        } else {
            terrain = 1;
        }

        if ((terrain & 0x939) != 0 || (terrain & 2) != 0) {
            CString fall;
            CString explosion;

            CGameLevel* lvl = g_gameReg->m_world->m_level;
            i32 col = m_targetY >> 5;
            i32 row = m_targetX >> 5;
            if (row < 0) {
                row = 0;
            } else {
                i32 w = lvl->m_mainPlane->m_gridW;
                if (row >= w) {
                    row = w - 1;
                }
            }
            if (col < 0) {
                col = 0;
            } else {
                i32 h = lvl->m_mainPlane->m_gridH;
                if (col >= h) {
                    col = h - 1;
                }
            }
            CDDrawWorkerHost* pl = lvl->m_mainPlane;
            i32 raw = pl->m_tileGrid[pl->m_colOffsets[col] + row];
            i32 act = 0;
            if (raw != static_cast<i32>(0xeeeeeeee) && raw != -1) {
                act = VtblResolve(lvl->m_imageSets[raw & 0xffff]);
            }

            switch (act) {
                case 4:
                case 110:
                case 116: {

                    switch (g_gameReg->m_curState->m_levelType) {
                        case 4:
                        case 5:
                        case 8:
                            fall = "LEVEL_ROLLINGBALL_FALL";
                            explosion = "LEVEL_ROLLINGBALLFALL";
                            break;
                        case 6:
                            fall = "LEVEL_ROLLINGBALL_EXPLOSION";
                            explosion = "LEVEL_ROLLINGBALLEXPLOSION";
                            act = 1;
                            break;
                        default: {
                            fall = "LEVEL_ROLLINGBALL_SINK";
                            explosion = "LEVEL_ROLLINGBALLSINKDEATH";
                            CWwdGameObjectA* o = m_object;
                            i32 px = o->m_screenX;
                            i32 py = o->m_screenY;
                            if (px < g_gameReg->m_viewBounds.right
                                && px >= g_gameReg->m_viewBounds.left
                                && py < g_gameReg->m_viewBounds.bottom
                                && py >= g_gameReg->m_viewBounds.top) {
                                CWwdGameObjectA* fx =
                                    g_gameReg->m_world->m_childGroup
                                        ->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
                                if (fx != 0) {
                                    fx->ApplyName("LEVEL_DEATHSPLASH");
                                    fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        }
                    }
                    m_wwdObject->ApplyName(fall);
                    m_value = m_wwdObject->m_1a0.m_14;
                    m_wwdObject->ApplyLookupGeometry(explosion, 0);
                    if (act != 4) {
                        m_explodeLatch = 1;
                        return 0;
                    }
                    DWORD perTile =
                        g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 0x3e8);
                    m_moveSpeed = kMsPerSecond / static_cast<double>(perTile);

                    CMapMgr* board = g_gameReg->m_tileGrid;
                    CWwdGameObjectA* o2 = m_object;
                    i32 bx = o2->m_screenX >> 5;
                    i32 by = o2->m_screenY >> 5;
                    i32 sink;
                    if (static_cast<u32>(bx) < board->m_width
                        && static_cast<u32>(by) < board->m_height) {
                        sink = board->m_rowInts[by][bx * 7 + 3];
                    } else {
                        sink = 0;
                    }
                    switch (sink) {
                        case 0x68:
                            m_targetX += 0x10;
                            m_targetY += 0x10;
                            break;
                        case 0x69:
                        case 0x6a:
                            m_targetY += 0x10;
                            break;
                        case 0x6b:
                            m_targetX -= 0x10;
                            m_targetY += 0x10;
                            break;
                        case 0x6c:
                            m_targetX += 0x10;
                            m_targetY += 0x10;
                            break;
                        case 0x71:
                            m_targetX -= 0x10;
                            m_targetY += 0x10;
                            break;
                        case 0x73:
                            m_targetX += 0x10;
                            break;
                        case 0x78:
                            m_targetX -= 0x10;
                            break;
                        case 0x7b:
                            m_targetX += 0x10;
                            break;
                        case 0x80:
                            m_targetX -= 0x10;
                            break;
                        case 0x82:
                            m_targetX += 0x10;
                            m_targetY -= 0x10;
                            break;
                        case 0x87:
                            m_targetX -= 0x10;
                            m_targetY -= 0x10;
                            break;
                        case 0x88:
                            m_targetX += 0x10;
                            m_targetY -= 0x10;
                            break;
                        case 0x89:
                        case 0x8a:
                            m_targetY -= 0x10;
                            break;
                        case 0x8b:
                            m_targetX -= 0x10;
                            m_targetY -= 0x10;
                            break;
                        default:
                            m_explodeLatch = 1;
                            return 0;
                    }
                    break;
                }

                case 10:
                case 36:
                case 108:
                case 114: {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SINK");
                    m_value = m_wwdObject->m_1a0.m_14;
                    m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLSINKWATER", 0);
                    CWwdGameObjectA* o = m_object;
                    i32 px = o->m_screenX;
                    i32 py = o->m_screenY;
                    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
                        && py < g_gameReg->m_viewBounds.bottom
                        && py >= g_gameReg->m_viewBounds.top) {
                        CWwdGameObjectA* fx =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
                        if (fx != 0) {
                            fx->ApplyName("GAME_WATER");
                            fx->ApplyLookupGeometry("GAME_WATER", 0);
                        }
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                case 35: {
                    i32 kind = g_gameReg->m_curState->m_levelType;
                    if (kind >= 4 && (kind <= 5 || kind == 8)) {
                        m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_FALL");
                        m_value = m_wwdObject->m_1a0.m_14;
                        m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLFALL", 0);
                    } else {
                        m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SINK");
                        m_value = m_wwdObject->m_1a0.m_14;
                        m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLSINKHOLE", 0);
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                default: {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EXPLOSION");
                    m_value = m_wwdObject->m_1a0.m_14;
                    m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLEXPLOSION", 0);
                    m_explodeLatch = 1;
                    return 0;
                }
            }
        }

        CWwdGameObjectA* dirObj = m_object;
        i32 oldDir = dirObj->m_12c;
        if ((terrain & 0x80) != 0) {
            CGameLevel* lvl2 = g_gameReg->m_world->m_level;
            i32 col2 = m_targetY >> 5;
            i32 row2 = m_targetX >> 5;
            if (row2 < 0) {
                row2 = 0;
            } else {
                i32 w = lvl2->m_mainPlane->m_gridW;
                if (row2 >= w) {
                    row2 = w - 1;
                }
            }
            if (col2 < 0) {
                col2 = 0;
            } else {
                i32 h = lvl2->m_mainPlane->m_gridH;
                if (col2 >= h) {
                    col2 = h - 1;
                }
            }
            CDDrawWorkerHost* pl2 = lvl2->m_mainPlane;
            i32 raw2 = pl2->m_tileGrid[pl2->m_colOffsets[col2] + row2];
            i32 act2 = 0;
            if (raw2 != static_cast<i32>(0xeeeeeeee) && raw2 != -1) {
                act2 = VtblResolve(lvl2->m_imageSets[raw2 & 0xffff]);
            }
            switch (act2) {
                case 11:
                case 15:
                    m_object->m_12c = 1;
                    break;
                case 14:
                case 18:
                    m_object->m_12c = 2;
                    break;
                case 12:
                case 16:
                    m_object->m_12c = 3;
                    break;
                case 13:
                case 17:
                    m_object->m_12c = 4;
                    break;
            }
        }

        CWwdGameObjectA* dirObj2 = m_object;
        m_subXLo = 0;
        m_subYLo = 0;
        m_subXHi = 0;
        m_subYHi = 0;
        switch (dirObj2->m_12c) {
            case 1:
                m_subY = -m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = -1;
                m_targetY -= 0x20;
                if (oldDir != dirObj2->m_12c) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_NORTH");
                }
                break;
            case 2:
                m_subX = m_moveDelta;
                m_stepDirX = 1;
                m_stepDirY = 0;
                m_targetX += 0x20;
                if (oldDir != dirObj2->m_12c) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EAST");
                }
                break;
            case 4:
                m_subX = -m_moveDelta;
                m_stepDirX = -1;
                m_stepDirY = 0;
                m_targetX -= 0x20;
                if (oldDir != dirObj2->m_12c) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_WEST");
                }
                break;
            default:
                m_subY = m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = 1;
                m_targetY += 0x20;
                if (oldDir != dirObj2->m_12c) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
                }
                break;
        }

        CWwdGameObjectA* out = m_object;
        m_subX = static_cast<double>(out->m_screenX) + m_subX;
        m_moveDeltaLo = 0;
        m_moveDeltaHi = 0;
        m_subY = static_cast<double>(out->m_screenY) + m_subY;
        CMapMgr* board2 = g_gameReg->m_tileGrid;
        i32 mtx = m_targetX >> 5;
        i32 mty = m_targetY >> 5;
        if (static_cast<u32>(mtx) < board2->m_width && static_cast<u32>(mty) < board2->m_height) {
            board2->m_rowInts[mty][mtx * 7] |= 0x10000000;
        }
    }

    double dt = static_cast<double>(static_cast<u32>(g_frameDelta)) * m_moveSpeed;
    i32 nx;
    if (m_stepDirX > 0) {
        double v = dt + m_subX;
        m_subX = v;
        nx = __ftol(ceil(v));
        m_moveDelta = fabs(static_cast<double>(nx) - static_cast<double>(m_targetX));
        if (nx > m_targetX) {
            nx = m_targetX;
        }
    } else if (m_stepDirX < 0) {
        double v = m_subX - dt;
        m_subX = v;
        nx = __ftol(floor(v));
        m_moveDelta = fabs(static_cast<double>(nx) - static_cast<double>(m_targetX));
        if (nx < m_targetX) {
            nx = m_targetX;
        }
    } else {
        nx = __ftol(ceil(m_subX));
    }

    i32 ny;
    if (m_stepDirY > 0) {
        double v = dt + m_subY;
        m_subY = v;
        ny = __ftol(ceil(v));
        m_moveDelta = fabs(static_cast<double>(ny) - static_cast<double>(m_targetY));
        if (ny > m_targetY) {
            ny = m_targetY;
        }
    } else if (m_stepDirY < 0) {
        double v = m_subY - dt;
        m_subY = v;
        ny = __ftol(floor(v));
        m_moveDelta = fabs(static_cast<double>(ny) - static_cast<double>(m_targetY));
        if (ny < m_targetY) {
            ny = m_targetY;
        }
    } else {
        ny = __ftol(ceil(m_subY));
    }

    CWwdGameObjectA* fin = m_object;
    fin->m_screenX = nx;
    CWwdGameObjectA* fin2 = m_object;
    fin2->m_screenY = ny;
    CWwdGameObjectA* fin3 = m_object;
    i32 next = fin3->m_screenY + 0x186a0;
    if (fin3->m_sortKey != next) {
        fin3->m_sortKey = next;
        fin3->m_flags |= 0x20000;
    }
    return 0;
}

RVA(0x000b0fe0, 0x1ab)
i32 CRollingBall::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }

    i32* p = &m_explodeStartLo;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 2;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 2;
            ar->Read(p, 8);
            break;
    }

    switch (tag) {
        case 4:
            ar->Write(&m_moveSpeed, 8);
            ar->Write(&m_subX, 8);
            ar->Write(&m_subY, 8);
            ar->Write(&m_stepDirX, 4);
            ar->Write(&m_stepDirY, 4);
            ar->Write(&m_targetX, 8);
            ar->Write(&m_explodeLatch, 4);
            ar->Write(&m_fallLatch, 4);
            ar->Write(&m_moveDeltaLo, 8);
            break;
        case 7:
            ar->Read(&m_moveSpeed, 8);
            ar->Read(&m_subX, 8);
            ar->Read(&m_subY, 8);
            ar->Read(&m_stepDirX, 4);
            ar->Read(&m_stepDirY, 4);
            ar->Read(&m_targetX, 8);
            ar->Read(&m_explodeLatch, 4);
            ar->Read(&m_fallLatch, 4);
            ar->Read(&m_moveDeltaLo, 8);
            break;
    }
    return 1;
}
