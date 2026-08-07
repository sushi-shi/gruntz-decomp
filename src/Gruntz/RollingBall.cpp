#include <rva.h>

#include <Gruntz/RollingBall.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingDeathTileId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/TriggerMgr.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <string.h>

template<> DATA(0x002461b0)
CActReg CActRegPool<CRollingBall>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// Half a tile per RollingBallTimePerTile ms - retail's divisor at 0x5ea3e8.
DATA(0x001ea3e8)
static const double kRollingBallSpeedNum = 16.0;

static __inline i32 VtblResolve(void* ent) {
    return IDX(static_cast<CTileImageSet*>(ent)->GetCollisionAt(0, 0));
}

RVA_COMPGEN(0x00012f50, 0x1e, ??_GCRollingBall@@UAEPAXI@Z)
RVA_COMPGEN(0x00012f80, 0x44, ??1CRollingBall@@UAE@XZ)

// @early-stop
RVA(0x000af820, 0x40d)
CRollingBall::CRollingBall(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_explodeStart = 0;
    m_explodeWindow = 0;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 snapY = 0x10 + (o->m_screenY & ~TILE_MASK_PX);
    o->m_screenX = snapX;
    m_subX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_subY = static_cast<double>(snapY);
    if (o->m_sortKey != SORTKEY_ROLLING_BALL_BASE + snapY) {
        o->m_sortKey = snapY + SORTKEY_ROLLING_BALL_BASE;
        o->m_flags |= 0x20000;
    }

    CWwdGameObjectA* obj38 = m_wwdObject;
    if (obj38->m_frameSet != NULL) {
        CString name;
        name = obj38->m_frameSet->m_name;
        const char* s;
        s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            o->m_direction = IDX(CARDINAL_NORTH);
            m_stepDirX = 0;
            m_stepDirY = -1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            o->m_direction = IDX(CARDINAL_EAST);
            m_stepDirY = 0;
            m_stepDirX = 1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            o->m_direction = IDX(CARDINAL_SOUTH);
            m_stepDirY = 1;
            m_stepDirX = 0;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            o->m_direction = IDX(CARDINAL_WEST);
            m_stepDirY = 0;
            m_stepDirX = -1;
        }
    }

    i32 time = o->m_animWorker->m_speed;
    if (time == 0) {
        time = g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CGruntzMgr* reg = g_gameReg;
    if (0 != reg->m_isEasyMode && reg->m_gameMode == GAMEMODE_SINGLE && o->m_smarts != 1) {
        time += 1000;
    }
    m_explodeWindow = static_cast<u32>(o->m_points);
    m_explodeStart = static_cast<u32>(g_frameTime);
    m_target.m_y = snapY;
    m_target.m_x = snapY;
    m_explodeLatch = 0;
    m_fallLatch = 0;
    m_moveSpeed = g_slimeSpeedNum / static_cast<double>(static_cast<u32>(time));
    o->m_area.left = 0;
    o->m_area.right = 0;
    o->m_area.top = 0;
    o->m_area.bottom = 0;
    m_moveDelta = 0.0;
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
            if (list != NULL) {
                list->CString::CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CRollingBall>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CRollingBall::Update);
}

// @early-stop
// cl pins 0 in ebp and compares members against it (`cmp mem,ebp`) where retail
// loads and tests (`mov eax,mem; test eax,eax`), which also costs it the register
// retail spends on the i64 timer's high dword; and it lays the fall/sink block
// out after the movement code instead of before it.
RVA(0x000b0140, 0xba8)
i32 CRollingBall::Update() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CWwdGameObjectA* anim = m_wwdObject;
    if (anim->m_animCursor.m_finished != 0 && anim->m_animCursor.m_frameTicksLeft == 0) {
        anim->m_flags |= 0x10000;
        return 0;
    }
    if (m_explodeLatch != 0) {
        return 0;
    }

    CWwdGameObjectA* logic = m_object;
    if (logic->m_points > 0) {
        if (static_cast<i64>(g_frameTime) - m_explodeStart >= m_explodeWindow) {
            m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EXPLOSION");
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLEXPLOSION", 0);
            CMapMgr* map = g_gameReg->m_tileGrid;
            CWwdGameObjectA* lg = m_object;
            i32 cx = lg->m_screenX >> TILE_SHIFT_PX;
            i32 cy = lg->m_screenY >> TILE_SHIFT_PX;
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
            g_gameReg->m_cmdGrid->CellDispatch(hitA, hitB, DEATH_SQUASH, -1);
        }
    }

    CWwdGameObjectA* cur = m_object;
    if (cur->m_screenX == m_target.m_x && cur->m_screenY == m_target.m_y) {

        g_gameReg->m_cmdGrid->WireTileSwitchLogic(0, m_target.m_x, m_target.m_y);
        g_gameReg->m_cmdGrid->ApplySwitch(0, m_target.m_x, m_target.m_y);

        i32 tx = m_target.m_x >> TILE_SHIFT_PX;
        i32 ty = m_target.m_y >> TILE_SHIFT_PX;
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

        if ((terrain & BRICKZ_BLOCKED_MASK) != 0 || (terrain & 2) != 0) {
            CString fall;
            CString explosion;

            CGameLevel* lvl = g_gameReg->m_world->m_level;
            i32 col = m_target.m_y >> TILE_SHIFT_PX;
            i32 row = m_target.m_x >> TILE_SHIFT_PX;
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
            // A TileCollisionKind: the devirtualised CTileImageSet::GetCollisionAt(0, 0).
            i32 act = 0;
            if (raw != UNINIT_FILL && raw != -1) {
                act = VtblResolve(lvl->m_imageSets[raw & 0xffff]);
            }

            switch (static_cast<TileCollisionKind>(act)) {
                case TILEKIND_SPECIAL:
                case TILEKIND_DEATHBRIDGE_UP:
                case TILEKIND_TOGGLEDEATHBRIDGE_UP: {

                    switch (g_gameReg->m_curState->m_levelType) {
                        case AREA_HIGH_ON_SWEETZ:
                        case AREA_HIGH_ROLLERZ:
                        case AREA_GRUNTZ_IN_SPACE:
                            fall = "LEVEL_ROLLINGBALL_FALL";
                            explosion = "LEVEL_ROLLINGBALLFALL";
                            break;
                        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
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
                                    g_gameReg->m_world->m_childGroup->CreateSprite(
                                        0,
                                        px,
                                        py,
                                        SORTKEY_ACTOR_BEHIND,
                                        "Particlez",
                                        0x40003
                                    );
                                if (fx != NULL) {
                                    fx->ApplyName("LEVEL_DEATHSPLASH");
                                    fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        }
                    }
                    m_wwdObject->ApplyName(fall);
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->ApplyLookupGeometry(explosion, 0);
                    if (act != IDX(TILEKIND_SPECIAL)) {
                        m_explodeLatch = 1;
                        return 0;
                    }
                    DWORD perTile =
                        g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 0x3e8);
                    m_moveSpeed = kRollingBallSpeedNum / static_cast<double>(perTile);

                    CMapMgr* board = g_gameReg->m_tileGrid;
                    CWwdGameObjectA* o2 = m_object;
                    i32 bx = o2->m_screenX >> TILE_SHIFT_PX;
                    i32 by = o2->m_screenY >> TILE_SHIFT_PX;
                    i32 sink;
                    if (static_cast<u32>(bx) < board->m_width
                        && static_cast<u32>(by) < board->m_height) {
                        sink = board->m_rowInts[by][bx * 7 + 3];
                    } else {
                        sink = 0;
                    }
                    // NOT a TileCollisionKind and NOT a direction: BrickzCell int 3
                    // is m_tileId, the raw WWD tile-image index, so this value space
                    // is per-tileset (AREA1-4 vs AREA5-8) and has no engine-side
                    // names. The DIRECTION is what the arms carry - each shoreline
                    // image sinks the ball toward its own edge. The same ids with
                    // the same offsets drive CGrunt::LoadGruntMovingDeathConfig.
                    switch (static_cast<MovingDeathTileSetAId>(sink)) {
                        case MOVING_DEATH_A_SE_1:
                            m_target.m_x += 0x10;
                            m_target.m_y += 0x10;
                            break;
                        case MOVING_DEATH_A_S_1:
                        case MOVING_DEATH_A_S_2:
                            m_target.m_y += 0x10;
                            break;
                        case MOVING_DEATH_A_SW_1:
                            m_target.m_x -= 0x10;
                            m_target.m_y += 0x10;
                            break;
                        case MOVING_DEATH_A_SE_2:
                            m_target.m_x += 0x10;
                            m_target.m_y += 0x10;
                            break;
                        case MOVING_DEATH_A_SW_3:
                            m_target.m_x -= 0x10;
                            m_target.m_y += 0x10;
                            break;
                        case MOVING_DEATH_A_E_1:
                            m_target.m_x += 0x10;
                            break;
                        case MOVING_DEATH_A_W_1:
                            m_target.m_x -= 0x10;
                            break;
                        case MOVING_DEATH_A_E_2:
                            m_target.m_x += 0x10;
                            break;
                        case MOVING_DEATH_A_W_2:
                            m_target.m_x -= 0x10;
                            break;
                        case MOVING_DEATH_A_NE_1:
                            m_target.m_x += 0x10;
                            m_target.m_y -= 0x10;
                            break;
                        case MOVING_DEATH_A_NW_2:
                            m_target.m_x -= 0x10;
                            m_target.m_y -= 0x10;
                            break;
                        case MOVING_DEATH_A_NE_3:
                            m_target.m_x += 0x10;
                            m_target.m_y -= 0x10;
                            break;
                        case MOVING_DEATH_A_N_1:
                        case MOVING_DEATH_A_N_2:
                            m_target.m_y -= 0x10;
                            break;
                        case MOVING_DEATH_A_NW_3:
                            m_target.m_x -= 0x10;
                            m_target.m_y -= 0x10;
                            break;
                        default:
                            m_explodeLatch = 1;
                            return 0;
                    }
                    break;
                }

                case TILEKIND_WATER:
                case TILEKIND_SINK_HAZARD:
                case TILEKIND_WATERBRIDGE_UP:
                case TILEKIND_TOGGLEWATERBRIDGE_UP: {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SINK");
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLSINKWATER", 0);
                    CWwdGameObjectA* o = m_object;
                    i32 px = o->m_screenX;
                    i32 py = o->m_screenY;
                    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
                        && py < g_gameReg->m_viewBounds.bottom
                        && py >= g_gameReg->m_viewBounds.top) {
                        CWwdGameObjectA* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            px,
                            py,
                            SORTKEY_ACTOR_BEHIND,
                            "Particlez",
                            0x40003
                        );
                        if (fx != NULL) {
                            fx->ApplyName("GAME_WATER");
                            fx->ApplyLookupGeometry("GAME_WATER", 0);
                        }
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                case TILEKIND_REVEALED_POWERUP: {
                    LevelArea kind = static_cast<LevelArea>(g_gameReg->m_curState->m_levelType);
                    if (kind == AREA_HIGH_ON_SWEETZ || kind == AREA_HIGH_ROLLERZ
                        || kind == AREA_GRUNTZ_IN_SPACE) {
                        m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_FALL");
                        m_value = m_wwdObject->m_animCursor.m_animation;
                        m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLFALL", 0);
                    } else {
                        m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SINK");
                        m_value = m_wwdObject->m_animCursor.m_animation;
                        m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLSINKHOLE", 0);
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                default: {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EXPLOSION");
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->ApplyLookupGeometry("LEVEL_ROLLINGBALLEXPLOSION", 0);
                    m_explodeLatch = 1;
                    return 0;
                }
            }
        }

        CWwdGameObjectA* dirObj = m_object;
        i32 oldDir = dirObj->m_direction;
        if ((terrain & 0x80) != 0) {
            CGameLevel* lvl2 = g_gameReg->m_world->m_level;
            i32 col2 = m_target.m_y >> TILE_SHIFT_PX;
            i32 row2 = m_target.m_x >> TILE_SHIFT_PX;
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
            if (raw2 != UNINIT_FILL && raw2 != -1) {
                act2 = VtblResolve(lvl2->m_imageSets[raw2 & 0xffff]);
            }
            switch (static_cast<TileCollisionKind>(act2)) {
                case TILEKIND_ARROW_UP_A:
                case TILEKIND_ARROW_UP_B:
                    m_object->m_direction = IDX(CARDINAL_NORTH);
                    break;
                case TILEKIND_ARROW_RIGHT_A:
                case TILEKIND_ARROW_RIGHT_B:
                    m_object->m_direction = IDX(CARDINAL_EAST);
                    break;
                case TILEKIND_ARROW_DOWN_A:
                case TILEKIND_ARROW_DOWN_B:
                    m_object->m_direction = IDX(CARDINAL_SOUTH);
                    break;
                case TILEKIND_ARROW_LEFT_A:
                case TILEKIND_ARROW_LEFT_B:
                    m_object->m_direction = IDX(CARDINAL_WEST);
                    break;
            }
        }

        CWwdGameObjectA* dirObj2 = m_object;
        m_subX = 0.0;
        m_subY = 0.0;
        switch (static_cast<CardinalDir>(dirObj2->m_direction)) {
            case CARDINAL_NORTH:
                m_subY = -m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = -1;
                m_target.m_y -= 0x20;
                if (oldDir != dirObj2->m_direction) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_NORTH");
                }
                break;
            case CARDINAL_EAST:
                m_subX = m_moveDelta;
                m_stepDirX = 1;
                m_stepDirY = 0;
                m_target.m_x += 0x20;
                if (oldDir != dirObj2->m_direction) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_EAST");
                }
                break;
            case CARDINAL_WEST:
                m_subX = -m_moveDelta;
                m_stepDirX = -1;
                m_stepDirY = 0;
                m_target.m_x -= 0x20;
                if (oldDir != dirObj2->m_direction) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_WEST");
                }
                break;
            default:
                m_subY = m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = 1;
                m_target.m_y += 0x20;
                if (oldDir != dirObj2->m_direction) {
                    m_wwdObject->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
                }
                break;
        }

        CWwdGameObjectA* out = m_object;
        m_subX = static_cast<double>(out->m_screenX) + m_subX;
        m_moveDelta = 0.0;
        m_subY = static_cast<double>(out->m_screenY) + m_subY;
        CMapMgr* board2 = g_gameReg->m_tileGrid;
        i32 mtx = m_target.m_x >> TILE_SHIFT_PX;
        i32 mty = m_target.m_y >> TILE_SHIFT_PX;
        if (static_cast<u32>(mtx) < board2->m_width && static_cast<u32>(mty) < board2->m_height) {
            board2->m_rowInts[mty][mtx * 7] |= 0x10000000;
        }
    }

    double dt = static_cast<double>(g_frameDelta) * m_moveSpeed;
    i32 nx;
    if (m_stepDirX > 0) {
        double v = dt + m_subX;
        m_subX = v;
        nx = static_cast<i32>(floor(v));
        m_moveDelta = fabs(static_cast<double>(nx) - static_cast<double>(m_target.m_x));
        if (nx > m_target.m_x) {
            nx = m_target.m_x;
        }
    } else if (m_stepDirX < 0) {
        double v = m_subX - dt;
        m_subX = v;
        nx = static_cast<i32>(ceil(v));
        m_moveDelta = fabs(static_cast<double>(nx) - static_cast<double>(m_target.m_x));
        if (nx < m_target.m_x) {
            nx = m_target.m_x;
        }
    } else {
        nx = static_cast<i32>(floor(m_subX));
    }

    i32 ny;
    if (m_stepDirY > 0) {
        double v = dt + m_subY;
        m_subY = v;
        ny = static_cast<i32>(floor(v));
        m_moveDelta = fabs(static_cast<double>(ny) - static_cast<double>(m_target.m_y));
        if (ny > m_target.m_y) {
            ny = m_target.m_y;
        }
    } else if (m_stepDirY < 0) {
        double v = m_subY - dt;
        m_subY = v;
        ny = static_cast<i32>(ceil(v));
        m_moveDelta = fabs(static_cast<double>(ny) - static_cast<double>(m_target.m_y));
        if (ny < m_target.m_y) {
            ny = m_target.m_y;
        }
    } else {
        ny = static_cast<i32>(floor(m_subY));
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
i32 CRollingBall::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }

    // Retail walks one pointer over the two adjacent i64 clocks (lea + add 8),
    // so it stays live across the call in a callee-saved register.
    i64* explode = &m_explodeStart;
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(explode, sizeof(*explode));
            explode++;
            ar->Write(explode, sizeof(*explode));
            break;
        case SERIAL_LOAD:
            ar->Read(explode, sizeof(*explode));
            explode++;
            ar->Read(explode, sizeof(*explode));
            break;
    }

    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_moveSpeed, sizeof(m_moveSpeed));
            ar->Write(&m_subX, sizeof(m_subX));
            ar->Write(&m_subY, sizeof(m_subY));
            ar->Write(&m_stepDirX, sizeof(m_stepDirX));
            ar->Write(&m_stepDirY, sizeof(m_stepDirY));
            ar->Write(&m_target, sizeof(m_target));
            ar->Write(&m_explodeLatch, sizeof(m_explodeLatch));
            ar->Write(&m_fallLatch, sizeof(m_fallLatch));
            ar->Write(&m_moveDelta, sizeof(m_moveDelta));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_moveSpeed, sizeof(m_moveSpeed));
            ar->Read(&m_subX, sizeof(m_subX));
            ar->Read(&m_subY, sizeof(m_subY));
            ar->Read(&m_stepDirX, sizeof(m_stepDirX));
            ar->Read(&m_stepDirY, sizeof(m_stepDirY));
            ar->Read(&m_target, sizeof(m_target));
            ar->Read(&m_explodeLatch, sizeof(m_explodeLatch));
            ar->Read(&m_fallLatch, sizeof(m_fallLatch));
            ar->Read(&m_moveDelta, sizeof(m_moveDelta));
            break;
    }
    return 1;
}
