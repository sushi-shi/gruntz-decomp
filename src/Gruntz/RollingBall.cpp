#include <rva.h>

#include <Gruntz/RollingBall.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
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
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <string.h>

RVA_DYNINIT(0x000afd40, 0xa, CActRegPool<CRollingBall>::s_table)
RVA_DYNINIT(0x000afd60, 0x15, CActRegPool<CRollingBall>::s_table)
RVA_DYNINIT(0x000afd90, 0xe, CActRegPool<CRollingBall>::s_table)
RVA_DYNINIT(0x000afdb0, 0x1f, CActRegPool<CRollingBall>::s_table)
template<> DATA(0x002461b0)
CActReg CActRegPool<CRollingBall>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// Half a tile per RollingBallTimePerTile ms - retail's divisor at 0x5ea3e8.
DATA(0x001ea3e8)
static const double kRollingBallSpeedNum = 16.0;

static __inline i32 VtblResolve(CTileImageSet* imageSet) {
    return IDX(imageSet->GetCollisionAt(0, 0));
}

RVA_COMPGEN(0x00012f50, 0x1e, ??_GCRollingBall@@UAEPAXI@Z)
RVA_COMPGEN(0x00012f80, 0x44, ??1CRollingBall@@UAE@XZ)

RVA(0x000af820, 0x40d)
CRollingBall::CRollingBall(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj), m_explodeStart(0), m_explodeWindow(0) {
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(0x2000002);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_subX, m_subY)
    CWwdSpriteObject* snapped = m_object;
    SET_SORT_KEY_IF_CHANGED(snapped, SORTKEY_ROLLING_BALL_BASE + snapY)
    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = name;
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_stepDirX = 0;
            m_stepDirY = -1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_stepDirX = 1;
            m_stepDirY = 0;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_stepDirX = 0;
            m_stepDirY = 1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_stepDirX = -1;
            m_stepDirY = 0;
        }
    }

    i32 time = m_object->m_logicRecord->m_speed;
    if (time == 0) {
        time = g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CGruntzMgr* reg = g_gameReg;
    if (0 != reg->m_isEasyMode && reg->m_gameMode == GAMEMODE_QUESTZ && m_object->m_smarts != 1) {
        time += 1000;
    }
    m_explodeWindow = static_cast<u32>(m_object->m_points);
    m_explodeStart = static_cast<u32>(g_frameTime);
    m_target.Set(snapX, snapY);
    m_explodeLatch = 0;
    m_fallLatch = 0;
    m_moveSpeed = g_slimeSpeedNum / static_cast<double>(static_cast<u32>(time));
    CLEAR_OBJECT_AREA
    m_moveDelta = 0.0;
}

RVA(0x000afde0, 0x102)
void CRollingBall::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CRollingBall>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CRollingBall>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000aff40, 0x18d)
void CRollingBall::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CRollingBall>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CRollingBall::Update);
}

RVA(0x000b0140, 0xba8)
i32 CRollingBall::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    CWwdSpriteObject* anim = m_wwdObject;
    if (IsAniCursorComplete(&anim->m_animationCursor)) {
        anim->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        return 0;
    }
    if (m_explodeLatch != 0) {
        return 0;
    }

    CWwdSpriteObject* logic = m_object;
    if (logic->m_points > 0) {
        if (static_cast<i64>(g_frameTime) - m_explodeStart >= m_explodeWindow) {
            SetImageSetByName("LEVEL_ROLLINGBALL_EXPLOSION");
            SwitchAnimationByName("LEVEL_ROLLINGBALLEXPLOSION", 0);
            CMapMgr* map = g_gameReg->m_tileGrid;
            CWwdSpriteObject* lg = m_object;
            i32 cx = lg->m_screenX >> TILE_SHIFT_PX;
            i32 cy = lg->m_screenY >> TILE_SHIFT_PX;
            if (static_cast<u32>(cx) < map->m_width && static_cast<u32>(cy) < map->m_height) {
                map->m_rowInts[cy][cx * 7] &= 0xefffffff;
            }
            m_explodeLatch = 1;
        }
    }

    if (m_fallLatch == 0) {
        CWwdSpriteObject* lg = m_object;
        i32 sx = lg->m_screenX;
        i32 sy = lg->m_screenY;
        if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, sx, sy)) {
            g_gameReg->m_triggerMgr->m_rollingballWanted = 1;
        }
        CWwdSpriteObject* lg2 = m_object;
        i32 playerIndex;
        i32 unitIndex;
        if (g_gameReg->m_triggerMgr->FindGruntAt(
                lg2->m_screenX,
                lg2->m_screenY,
                &lg2->m_area,
                &playerIndex,
                &unitIndex,
                NULL
            )) {
            g_gameReg->m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_SQUASH, -1);
        }
    }

    CWwdSpriteObject* cur = m_object;
    if (cur->m_screenX == m_target.m_x && cur->m_screenY == m_target.m_y) {

        g_gameReg->m_triggerMgr->WireTileSwitchLogic(NULL, m_target.m_x, m_target.m_y);
        g_gameReg->m_triggerMgr->ApplySwitch(NULL, m_target.m_x, m_target.m_y);

        i32 tx = m_target.m_x >> TILE_SHIFT_PX;
        i32 ty = m_target.m_y >> TILE_SHIFT_PX;
        CMapMgr* map = g_gameReg->m_tileGrid;
        if (static_cast<u32>(tx) < map->m_width && static_cast<u32>(ty) < map->m_height) {
            map->m_rowInts[ty][tx * 7] &= 0xefffffff;
        }
        CMapMgr* map2 = g_gameReg->m_tileGrid;
        i32 terrain = map2->CellFlagsAt(tx, ty);

        if ((terrain & BRICKZ_BLOCKED_MASK) != 0 || (terrain & 2) != 0) {
            CString fall;
            CString explosion;

            CGameLevel* lvl = g_gameReg->m_world->m_level;
            i32 tileY = m_target.m_y >> TILE_SHIFT_PX;
            i32 tileX = m_target.m_x >> TILE_SHIFT_PX;
            if (tileX < 0) {
                tileX = 0;
            } else {
                i32 w = lvl->m_mainPlane->m_tileColumns;
                if (tileX >= w) {
                    tileX = w - 1;
                }
            }
            if (tileY < 0) {
                tileY = 0;
            } else {
                i32 h = lvl->m_mainPlane->m_tileRows;
                if (tileY >= h) {
                    tileY = h - 1;
                }
            }
            CDDrawWorkerHost* pl = lvl->m_mainPlane;
            i32 raw = pl->m_tileHandles[pl->m_tileRowOffsets[tileY] + tileX];
            // A TileCollisionKind: the devirtualised CTileImageSet::GetCollisionAt(0, 0).
            i32 act;
            if (raw != UNINIT_FILL && raw != -1) {
                act = VtblResolve(static_cast<CTileImageSet*>(lvl->m_imageSets[raw & 0xffff]));
            } else {
                act = 0;
            }

            switch (static_cast<TileCollisionKind>(act)) {
                case TILEKIND_DEATH:
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
                            CWwdSpriteObject* o = m_object;
                            i32 px = o->m_screenX;
                            i32 py = o->m_screenY;
                            if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, px, py)) {
                                CWwdSpriteObject* fx =
                                    g_gameReg->m_world->m_childGroup->CreateSprite(
                                        0,
                                        px,
                                        py,
                                        SORTKEY_ACTOR_BEHIND,
                                        "Particlez",
                                        0x40003
                                    );
                                if (fx != NULL) {
                                    fx->SetImageSetByName("LEVEL_DEATHSPLASH");
                                    fx->SetAnimationByName("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        }
                    }
                    SetImageSetByName(fall);
                    SwitchAnimationByName(explosion, 0);
                    if (act != IDX(TILEKIND_DEATH)) {
                        m_explodeLatch = 1;
                        return 0;
                    }
                    DWORD perTile =
                        g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 0x3e8);
                    m_moveSpeed = kRollingBallSpeedNum / static_cast<double>(perTile);

                    CMapMgr* board = g_gameReg->m_tileGrid;
                    CWwdSpriteObject* o2 = m_object;
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
                    SetImageSetByName("LEVEL_ROLLINGBALL_SINK");
                    SwitchAnimationByName("LEVEL_ROLLINGBALLSINKWATER", 0);
                    CWwdSpriteObject* o = m_object;
                    i32 px = o->m_screenX;
                    i32 py = o->m_screenY;
                    if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, px, py)) {
                        CWwdSpriteObject* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            px,
                            py,
                            SORTKEY_ACTOR_BEHIND,
                            "Particlez",
                            0x40003
                        );
                        if (fx != NULL) {
                            fx->SetImageSetByName("GAME_WATER");
                            fx->SetAnimationByName("GAME_WATER", 0);
                        }
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                case TILEKIND_REVEALED_POWERUP: {
                    switch (static_cast<LevelArea>(g_gameReg->m_curState->m_levelType)) {
                        case AREA_HIGH_ON_SWEETZ:
                        case AREA_HIGH_ROLLERZ:
                        case AREA_GRUNTZ_IN_SPACE:
                            SetImageSetByName("LEVEL_ROLLINGBALL_FALL");
                            SwitchAnimationByName("LEVEL_ROLLINGBALLFALL", 0);
                            break;
                        default:
                            SetImageSetByName("LEVEL_ROLLINGBALL_SINK");
                            SwitchAnimationByName("LEVEL_ROLLINGBALLSINKHOLE", 0);
                            break;
                    }
                    m_explodeLatch = 1;
                    return 0;
                }

                default: {
                    SetImageSetByName("LEVEL_ROLLINGBALL_EXPLOSION");
                    SwitchAnimationByName("LEVEL_ROLLINGBALLEXPLOSION", 0);
                    m_explodeLatch = 1;
                    return 0;
                }
            }
        }

        CWwdSpriteObject* dirObj = m_object;
        i32 oldDir = dirObj->m_direction;
        if ((terrain & 0x80) != 0) {
            CGameLevel* lvl2 = g_gameReg->m_world->m_level;
            i32 tileY2 = ty;
            i32 tileX2 = tx;
            if (tileX2 < 0) {
                tileX2 = 0;
            } else {
                i32 w = lvl2->m_mainPlane->m_tileColumns;
                if (tileX2 >= w) {
                    tileX2 = w - 1;
                }
            }
            if (tileY2 < 0) {
                tileY2 = 0;
            } else {
                i32 h = lvl2->m_mainPlane->m_tileRows;
                if (tileY2 >= h) {
                    tileY2 = h - 1;
                }
            }
            CDDrawWorkerHost* pl2 = lvl2->m_mainPlane;
            i32 raw2 = pl2->m_tileHandles[pl2->m_tileRowOffsets[tileY2] + tileX2];
            i32 act2;
            if (raw2 != UNINIT_FILL && raw2 != -1) {
                act2 = VtblResolve(static_cast<CTileImageSet*>(lvl2->m_imageSets[raw2 & 0xffff]));
            } else {
                act2 = 0;
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

        CWwdSpriteObject* dirObj2 = m_object;
        m_subX = 0.0;
        m_subY = 0.0;
        switch (static_cast<CardinalDir>(dirObj2->m_direction)) {
            case CARDINAL_NORTH:
                m_subY = -m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = -1;
                m_target.Set(m_target.m_x, m_target.m_y - 0x20);
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_NORTH");
                }
                break;
            case CARDINAL_EAST:
                m_subX = m_moveDelta;
                m_stepDirX = 1;
                m_stepDirY = 0;
                m_target.Set(m_target.m_x + 0x20, m_target.m_y);
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_EAST");
                }
                break;
            case CARDINAL_WEST:
                m_subX = -m_moveDelta;
                m_stepDirX = -1;
                m_stepDirY = 0;
                m_target.Set(m_target.m_x - 0x20, m_target.m_y);
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_WEST");
                }
                break;
            default:
                m_subY = m_moveDelta;
                m_stepDirX = 0;
                m_stepDirY = 1;
                m_target.Set(m_target.m_x, m_target.m_y + 0x20);
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_SOUTH");
                }
                break;
        }

        CWwdSpriteObject* out = m_object;
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

    CWwdSpriteObject* fin = m_object;
    fin->m_screenX = nx;
    CWwdSpriteObject* fin2 = m_object;
    fin2->m_screenY = ny;
    CWwdSpriteObject* fin3 = m_object;
    i32 next = fin3->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(fin3, next)
    return 0;
}

RVA(0x000b0fe0, 0x1ab)
i32 CRollingBall::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_CHAIN_OR_RETURN(ar, mode, typeId, object)

    // Retail walks one pointer over the two adjacent i64 clocks (lea + add 8),
    // so it stays live across the call in a callee-saved register.
    i64* explode = &m_explodeStart;
    switch (mode) {
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

    switch (mode) {
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
