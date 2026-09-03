#include <rva.h>

#include <Gruntz/RollingBall.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDirectionOffset.h>
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
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MovingDeathTileId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
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
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    Coord snappedPosition = m_object->ScreenPos();
    SnapTileCenter(&snappedPosition);
    m_object->SetScreenPos(snappedPosition);
    m_subPosition.Init(snappedPosition);
    CWwdSpriteObject* snapped = m_object;
    SET_SORT_KEY_IF_CHANGED(snapped, SORTKEY_ROLLING_BALL_BASE + snappedPosition.m_y)
    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = name;
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_stepDirection.Set(0, -1);
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_stepDirection.Set(1, 0);
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_stepDirection.Set(0, 1);
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_stepDirection.Set(-1, 0);
        }
    }

    i32 time = m_object->m_logicRecord->m_speed;
    if (time == 0) {
        time = g_buteMgr.GetDword("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CGruntzMgr* reg = g_gameReg;
    if (false != reg->m_isEasyMode && reg->m_gameMode == GAMEMODE_QUESTZ
        && m_object->m_smarts != 1) {
        time += 1000;
    }
    m_explodeWindow = static_cast<u32>(m_object->m_points);
    m_explodeStart = static_cast<u32>(g_frameTime);
    m_target = snappedPosition;
    m_explodeLatch = false;
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
    if (m_explodeLatch != false) {
        return 0;
    }

    CWwdSpriteObject* logic = m_object;
    if (logic->m_points > 0) {
        if (static_cast<i64>(g_frameTime) - m_explodeStart >= m_explodeWindow) {
            SetImageSetByName("LEVEL_ROLLINGBALL_EXPLOSION");
            SwitchAnimationByName("LEVEL_ROLLINGBALLEXPLOSION", 0);
            CMapMgr* map = g_gameReg->m_tileGrid;
            CWwdSpriteObject* lg = m_object;
            Coord tile = lg->ScreenPos();
            ScreenTile(&tile);
            if (static_cast<u32>(tile.m_x) < map->m_width
                && static_cast<u32>(tile.m_y) < map->m_height) {
                map->m_rows[tile.m_y][tile.m_x].m_flags &= ~IDX(CELL_FLAG_ROLLING_BALL);
            }
            m_explodeLatch = true;
        }
    }

    if (m_fallLatch == 0) {
        CWwdSpriteObject* lg = m_object;
        Coord position = lg->ScreenPos();
        if (::PtInRect(&g_gameReg->m_viewBounds, position.m_x, position.m_y)) {
            g_gameReg->m_triggerMgr->m_rollingballWanted = true;
        }
        CWwdSpriteObject* lg2 = m_object;
        i32 playerIndex;
        i32 unitIndex;
        if (g_gameReg->m_triggerMgr->FindGruntAt(
                lg2->m_screenPosition.m_x,
                lg2->m_screenPosition.m_y,
                &lg2->m_area,
                &playerIndex,
                &unitIndex,
                NULL
            )) {
            g_gameReg->m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_SQUASH, -1);
        }
    }

    CWwdSpriteObject* cur = m_object;
    if (cur->ScreenPos() == m_target) {

        g_gameReg->m_triggerMgr->WireTileSwitchLogic(NULL, m_target.m_x, m_target.m_y);
        g_gameReg->m_triggerMgr->ApplySwitch(NULL, m_target.m_x, m_target.m_y);

        Coord targetTile = m_target;
        ScreenTile(&targetTile);
        CMapMgr* map = g_gameReg->m_tileGrid;
        if (static_cast<u32>(targetTile.m_x) < map->m_width
            && static_cast<u32>(targetTile.m_y) < map->m_height) {
            map->m_rows[targetTile.m_y][targetTile.m_x].m_flags &= ~IDX(CELL_FLAG_ROLLING_BALL);
        }
        CMapMgr* map2 = g_gameReg->m_tileGrid;
        i32 terrain = map2->CellFlagsAt(targetTile.m_x, targetTile.m_y);

        if ((terrain & BRICKZ_BLOCKED_MASK) != 0 || (terrain & IDX(CELL_FLAG_SPECIAL)) != 0) {
            CString fall;
            CString explosion;

            CGameLevel* lvl = g_gameReg->m_world->m_level;
            Coord clampedTile = targetTile;
            clampedTile.Max(Coord(0, 0));
            clampedTile.Min(Coord(
                lvl->m_mainPlane->m_tileGridSize.cx - 1,
                lvl->m_mainPlane->m_tileGridSize.cy - 1
            ));
            CDDrawWorkerHost* pl = lvl->m_mainPlane;
            i32 raw = pl->m_tileHandles[pl->m_tileRowOffsets[clampedTile.m_y] + clampedTile.m_x];
            i32 act;
            if (raw != UNINIT_FILL && raw != -1) {
                act = VtblResolve(
                    static_cast<CTileImageSet*>(
                        lvl->m_imageSets[raw & WWD_TILE_IMAGE_SET_INDEX_MASK]
                    )
                );
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
                            Coord position = o->ScreenPos();
                            if (::PtInRect(&g_gameReg->m_viewBounds, position.m_x, position.m_y)) {
                                CWwdSpriteObject* fx =
                                    g_gameReg->m_world->m_childGroup->CreateSprite(
                                        0,
                                        position.m_x,
                                        position.m_y,
                                        SORTKEY_ACTOR_BEHIND,
                                        "Particlez",
                                        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
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
                        m_explodeLatch = true;
                        return 0;
                    }
                    DWORD perTile = g_buteMgr.GetDword("Hazardz", "RollingBallTimePerTile", 0x3e8);
                    m_moveSpeed = kRollingBallSpeedNum / static_cast<double>(perTile);

                    CMapMgr* board = g_gameReg->m_tileGrid;
                    CWwdSpriteObject* o2 = m_object;
                    Coord sinkTile = o2->ScreenPos();
                    ScreenTile(&sinkTile);
                    i32 sink;
                    if (static_cast<u32>(sinkTile.m_x) < board->m_width
                        && static_cast<u32>(sinkTile.m_y) < board->m_height) {
                        sink = board->m_rows[sinkTile.m_y][sinkTile.m_x].m_tileId;
                    } else {
                        sink = 0;
                    }
                    switch (static_cast<MovingDeathTileSetAId>(sink)) {
                        case MOVING_DEATH_A_SE_1:
                            m_target += Coord(0x10, 0x10);
                            break;
                        case MOVING_DEATH_A_S_1:
                        case MOVING_DEATH_A_S_2:
                            m_target += Coord(0, 0x10);
                            break;
                        case MOVING_DEATH_A_SW_1:
                            m_target += Coord(-0x10, 0x10);
                            break;
                        case MOVING_DEATH_A_SE_2:
                            m_target += Coord(0x10, 0x10);
                            break;
                        case MOVING_DEATH_A_SW_3:
                            m_target += Coord(-0x10, 0x10);
                            break;
                        case MOVING_DEATH_A_E_1:
                            m_target += Coord(0x10, 0);
                            break;
                        case MOVING_DEATH_A_W_1:
                            m_target += Coord(-0x10, 0);
                            break;
                        case MOVING_DEATH_A_E_2:
                            m_target += Coord(0x10, 0);
                            break;
                        case MOVING_DEATH_A_W_2:
                            m_target += Coord(-0x10, 0);
                            break;
                        case MOVING_DEATH_A_NE_1:
                            m_target += Coord(0x10, -0x10);
                            break;
                        case MOVING_DEATH_A_NW_2:
                            m_target += Coord(-0x10, -0x10);
                            break;
                        case MOVING_DEATH_A_NE_3:
                            m_target += Coord(0x10, -0x10);
                            break;
                        case MOVING_DEATH_A_N_1:
                        case MOVING_DEATH_A_N_2:
                            m_target += Coord(0, -0x10);
                            break;
                        case MOVING_DEATH_A_NW_3:
                            m_target += Coord(-0x10, -0x10);
                            break;
                        default:
                            m_explodeLatch = true;
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
                    Coord position = o->ScreenPos();
                    if (::PtInRect(&g_gameReg->m_viewBounds, position.m_x, position.m_y)) {
                        CWwdSpriteObject* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            position.m_x,
                            position.m_y,
                            SORTKEY_ACTOR_BEHIND,
                            "Particlez",
                            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                        );
                        if (fx != NULL) {
                            fx->SetImageSetByName("GAME_WATER");
                            fx->SetAnimationByName("GAME_WATER", 0);
                        }
                    }
                    m_explodeLatch = true;
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
                    m_explodeLatch = true;
                    return 0;
                }

                default: {
                    SetImageSetByName("LEVEL_ROLLINGBALL_EXPLOSION");
                    SwitchAnimationByName("LEVEL_ROLLINGBALLEXPLOSION", 0);
                    m_explodeLatch = true;
                    return 0;
                }
            }
        }

        CWwdSpriteObject* dirObj = m_object;
        i32 oldDir = dirObj->m_direction;
        if ((terrain & IDX(CELL_FLAG_ARROW)) != 0) {
            CGameLevel* lvl2 = g_gameReg->m_world->m_level;
            Coord clampedTile = targetTile;
            clampedTile.Max(Coord(0, 0));
            clampedTile.Min(Coord(
                lvl2->m_mainPlane->m_tileGridSize.cx - 1,
                lvl2->m_mainPlane->m_tileGridSize.cy - 1
            ));
            CDDrawWorkerHost* pl2 = lvl2->m_mainPlane;
            i32 raw2 = pl2->m_tileHandles[pl2->m_tileRowOffsets[clampedTile.m_y] + clampedTile.m_x];
            i32 act2;
            if (raw2 != UNINIT_FILL && raw2 != -1) {
                act2 = VtblResolve(
                    static_cast<CTileImageSet*>(
                        lvl2->m_imageSets[raw2 & WWD_TILE_IMAGE_SET_INDEX_MASK]
                    )
                );
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
        CardinalDir movementDirection = static_cast<CardinalDir>(dirObj2->m_direction);
        if (movementDirection != CARDINAL_NORTH && movementDirection != CARDINAL_EAST
            && movementDirection != CARDINAL_WEST) {
            movementDirection = CARDINAL_SOUTH;
        }
        Coord stepDirection = CardinalDirectionOffset(movementDirection, 1);
        m_stepDirection = stepDirection;
        m_subPosition = DoubleVector2(stepDirection) * m_moveDelta;
        m_target += stepDirection * TILE_SIZE_PX;
        switch (movementDirection) {
            case CARDINAL_NORTH:
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_NORTH");
                }
                break;
            case CARDINAL_EAST:
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_EAST");
                }
                break;
            case CARDINAL_WEST:
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_WEST");
                }
                break;
            default:
                if (oldDir != dirObj2->m_direction) {
                    SetImageSetByName("LEVEL_ROLLINGBALL_SOUTH");
                }
                break;
        }

        Coord screenPosition = m_object->ScreenPos();
        DoubleVector2 origin(screenPosition);
        m_subPosition += origin;
        m_moveDelta = 0.0;
        CMapMgr* board2 = g_gameReg->m_tileGrid;
        Coord reservedTile = m_target;
        ScreenTile(&reservedTile);
        if (static_cast<u32>(reservedTile.m_x) < board2->m_width
            && static_cast<u32>(reservedTile.m_y) < board2->m_height) {
            board2->m_rows[reservedTile.m_y][reservedTile.m_x].m_flags |=
                IDX(CELL_FLAG_ROLLING_BALL);
        }
    }

    double dt = static_cast<double>(g_frameDelta) * m_moveSpeed;
    Coord nextPosition;
    if (m_stepDirection.m_x > 0) {
        double v = dt + m_subPosition.x;
        m_subPosition.x = v;
        nextPosition.m_x = static_cast<i32>(floor(v));
        m_moveDelta =
            fabs(static_cast<double>(nextPosition.m_x) - static_cast<double>(m_target.m_x));
        if (nextPosition.m_x > m_target.m_x) {
            nextPosition.m_x = m_target.m_x;
        }
    } else if (m_stepDirection.m_x < 0) {
        double v = m_subPosition.x - dt;
        m_subPosition.x = v;
        nextPosition.m_x = static_cast<i32>(ceil(v));
        m_moveDelta =
            fabs(static_cast<double>(nextPosition.m_x) - static_cast<double>(m_target.m_x));
        if (nextPosition.m_x < m_target.m_x) {
            nextPosition.m_x = m_target.m_x;
        }
    } else {
        nextPosition.m_x = static_cast<i32>(floor(m_subPosition.x));
    }

    if (m_stepDirection.m_y > 0) {
        double v = dt + m_subPosition.y;
        m_subPosition.y = v;
        nextPosition.m_y = static_cast<i32>(floor(v));
        m_moveDelta =
            fabs(static_cast<double>(nextPosition.m_y) - static_cast<double>(m_target.m_y));
        if (nextPosition.m_y > m_target.m_y) {
            nextPosition.m_y = m_target.m_y;
        }
    } else if (m_stepDirection.m_y < 0) {
        double v = m_subPosition.y - dt;
        m_subPosition.y = v;
        nextPosition.m_y = static_cast<i32>(ceil(v));
        m_moveDelta =
            fabs(static_cast<double>(nextPosition.m_y) - static_cast<double>(m_target.m_y));
        if (nextPosition.m_y < m_target.m_y) {
            nextPosition.m_y = m_target.m_y;
        }
    } else {
        nextPosition.m_y = static_cast<i32>(floor(m_subPosition.y));
    }

    CWwdSpriteObject* fin = m_object;
    fin->SetScreenPos(nextPosition);
    CWwdSpriteObject* fin3 = m_object;
    i32 next = fin3->m_screenPosition.m_y + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(fin3, next)
    return 0;
}

RVA(0x000b0fe0, 0x1ab)
i32 CRollingBall::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)

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
            ar->Write(&m_subPosition.x, sizeof(m_subPosition.x));
            ar->Write(&m_subPosition.y, sizeof(m_subPosition.y));
            ar->Write(&m_stepDirection.m_x, sizeof(m_stepDirection.m_x));
            ar->Write(&m_stepDirection.m_y, sizeof(m_stepDirection.m_y));
            ar->Write(&m_target, sizeof(m_target));
            ar->Write(&m_explodeLatch, sizeof(m_explodeLatch));
            ar->Write(&m_fallLatch, sizeof(m_fallLatch));
            ar->Write(&m_moveDelta, sizeof(m_moveDelta));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_moveSpeed, sizeof(m_moveSpeed));
            ar->Read(&m_subPosition.x, sizeof(m_subPosition.x));
            ar->Read(&m_subPosition.y, sizeof(m_subPosition.y));
            ar->Read(&m_stepDirection.m_x, sizeof(m_stepDirection.m_x));
            ar->Read(&m_stepDirection.m_y, sizeof(m_stepDirection.m_y));
            ar->Read(&m_target, sizeof(m_target));
            ar->Read(&m_explodeLatch, sizeof(m_explodeLatch));
            ar->Read(&m_fallLatch, sizeof(m_fallLatch));
            ar->Read(&m_moveDelta, sizeof(m_moveDelta));
            break;
    }
    return 1;
}
