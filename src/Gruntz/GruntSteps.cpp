#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateRecord.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirectionOffset.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA_DYNINIT(0x00047760, 0x1a, g_gruntMoveDirNorth)
DATA(0x002448d8)
GruntDirectionCell g_gruntMoveDirNorth = GruntDirectionCell(0, 1, DIR_NORTH);
RVA_DYNINIT(0x00047790, 0x5, g_gruntMoveDirNorthEast)
RVA_DYNINIT(0x000477b0, 0x1a, g_gruntMoveDirNorthEast)
DATA(0x00244908)
GruntDirectionCell g_gruntMoveDirNorthEast = GruntDirectionCell(0, 2, DIR_NORTHEAST);
RVA_DYNINIT(0x000477e0, 0x5, g_gruntMoveDirEast)
RVA_DYNINIT(0x00047800, 0x1f, g_gruntMoveDirEast)
DATA(0x002448c8)
GruntDirectionCell g_gruntMoveDirEast = GruntDirectionCell(1, 2, DIR_EAST);
RVA_DYNINIT(0x00047830, 0x5, g_gruntMoveDirSouthEast)
RVA_DYNINIT(0x00047850, 0x1a, g_gruntMoveDirSouthEast)
DATA(0x00244928)
GruntDirectionCell g_gruntMoveDirSouthEast = GruntDirectionCell(2, 2, DIR_SOUTHEAST);
RVA_DYNINIT(0x000479c0, 0x5, g_gruntMoveDirCenter)
RVA_DYNINIT(0x000479e0, 0x1a, g_gruntMoveDirCenter)
DATA(0x00244938)
GruntDirectionCell g_gruntMoveDirCenter = GruntDirectionCell(1, 1, DIR_CENTER);
RVA_DYNINIT(0x00047880, 0x5, g_gruntMoveDirSouth)
RVA_DYNINIT(0x000478a0, 0x1f, g_gruntMoveDirSouth)
DATA(0x002448e8)
GruntDirectionCell g_gruntMoveDirSouth = GruntDirectionCell(2, 1, DIR_SOUTH);
RVA_DYNINIT(0x000478d0, 0x5, g_gruntMoveDirSouthWest)
RVA_DYNINIT(0x000478f0, 0x1f, g_gruntMoveDirSouthWest)
DATA(0x00244948)
GruntDirectionCell g_gruntMoveDirSouthWest = GruntDirectionCell(2, 0, DIR_SOUTHWEST);
RVA_DYNINIT(0x00047920, 0x5, g_gruntMoveDirWest)
RVA_DYNINIT(0x00047940, 0x1f, g_gruntMoveDirWest)
DATA(0x002448f8)
GruntDirectionCell g_gruntMoveDirWest = GruntDirectionCell(1, 0, DIR_WEST);
RVA_DYNINIT(0x00047970, 0x5, g_gruntMoveDirNorthWest)
RVA_DYNINIT(0x00047990, 0x17, g_gruntMoveDirNorthWest)
DATA(0x00244918)
GruntDirectionCell g_gruntMoveDirNorthWest = GruntDirectionCell(0, 0, DIR_NORTHWEST);

static char s_EntranceSafeTime[] = "EntranceSafeTime";

DATA(0x0020dbf8)
static char s_ToyTiles[] = "ToyTiles";

static inline i32 DiagonalRouteBlocked(CMapMgr* board, const Coord& source, const Coord& target) {
    Coord horizontalStep(target.m_x > source.m_x ? 1 : -1, 0);
    Coord verticalStep(0, target.m_y > source.m_y ? 1 : -1);
    Coord sourceHorizontal = source + horizontalStep;
    Coord sourceVertical = source + verticalStep;
    Coord targetHorizontal = target - horizontalStep;
    Coord targetVertical = target - verticalStep;
    return (board->CellFlagsAt(sourceHorizontal.m_x, sourceHorizontal.m_y)
            & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(sourceVertical.m_x, sourceVertical.m_y) & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(targetHorizontal.m_x, targetHorizontal.m_y)
               & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(targetVertical.m_x, targetVertical.m_y)
               & BRICKZ_CELL_ROUTE_MASKB);
}

static __inline i32 s_CanCommitToyMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    Coord source(sourceX, sourceY);
    ScreenTile(&source);
    Coord move(moveX, moveY);
    ScreenTile(&move);
    i32 arr = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (source != move) {
        if (static_cast<u32>(move.m_x) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(move.m_y) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        BrickzCell* targetCell = &board->m_rows[move.m_y][move.m_x];
        i32 tflags = targetCell->m_flags;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask
                       | IDX(
                           CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_SPIKES
                           | CELL_FLAG_STATIC_HAZARD | CELL_FLAG_ROLLING_BALL
                       );
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        Coord delta = move - source;
        if (delta.m_x == 0 || delta.m_y == 0) {
            return 1;
        }
        if (DiagonalRouteBlocked(board, source, move)) {
            return 0;
        }
    }
    return 1;
}

static __inline i32 s_CanCommitBagMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    Coord source(sourceX, sourceY);
    ScreenTile(&source);
    Coord move(moveX, moveY);
    ScreenTile(&move);
    i32 arr = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (source != move) {
        if (static_cast<u32>(move.m_x) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(move.m_y) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        BrickzCell* targetCell = &board->m_rows[move.m_y][move.m_x];
        i32 tflags = targetCell->m_flags;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask
                       | IDX(
                           CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_SPIKES
                           | CELL_FLAG_STATIC_HAZARD | CELL_FLAG_ROLLING_BALL
                       );
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        Coord delta = move - source;
        if (delta.m_x == 0 || delta.m_y == 0) {
            return 1;
        }
        if (DiagonalRouteBlocked(board, source, move)) {
            return 0;
        }
    }
    return 1;
}

static __inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, i64* pair) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(pair, sizeof(*pair));
            ar->Write(pair + 1, sizeof(*pair));
            break;
        case SERIAL_LOAD:
            ar->Read(pair, sizeof(*pair));
            ar->Read(pair + 1, sizeof(*pair));
            break;
    }
}

RVA(0x00050ca0, 0x2b)
i32 CGrunt::LoadTypeTableClearMove(PickupType typeId) {

    i32 r = LoadGruntTypeTable(typeId, 0, 0, 0);
    m_entrancePickup = PICKUP_INVALID;
    m_helpCueId = 0;
    return r;
}

// @early-stop
RVA(0x00050ce0, 0x3c4)
i32 CGrunt::LoadVehicleGruntSprites(PickupType kind) {
    m_vehiclePickupType = kind;
    m_entrancePickup = PICKUP_INVALID;

    CString name;

#define REGION_INIT()                                                                              \
    do {                                                                                           \
        CRect a(-1, -1, 1, 1);                                                                     \
        m_vehicleContactRect = a;                                                                  \
        a.SetRectEmpty();                                                                          \
        m_vehicleContactExclusionRect = a;                                                         \
    } while (0)
    switch (kind) {
        case PICKUP_BABYWALKER:
            REGION_INIT();
            name = "BABYWALKERGRUNT";
            break;
        case PICKUP_BEACHBALL:
            REGION_INIT();
            name = "BEACHBALLGRUNT";
            break;
        case PICKUP_BIGWHEEL:
            REGION_INIT();
            name = "BIGWHEELGRUNT";
            break;
        case PICKUP_GOKART:
            REGION_INIT();
            name = "GOKARTGRUNT";
            break;
        case PICKUP_JACKINTHEBOX:
            REGION_INIT();
            name = "JACKINTHEBOXGRUNT";
            break;
        case PICKUP_JUMPROPE:
            REGION_INIT();
            name = "JUMPROPEGRUNT";
            break;
        case PICKUP_POGOSTICK:
            REGION_INIT();
            name = "POGOSTICKGRUNT";
            break;
        case PICKUP_SCROLL:
            REGION_INIT();
            name = "SCROLLGRUNT";
            break;
        case PICKUP_SQUEAKTOY:
            REGION_INIT();
            name = "SQUEAKTOYGRUNT";
            break;
        case PICKUP_YOYO:
            REGION_INIT();
            name = "YOYOGRUNT";
            break;
        default:
            break;
    }
#undef REGION_INIT

    g_gameReg->m_curState->BuildAssetNamespacePrefixes(name, 1, 1, NULL);

    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    TileCollisionKind tileKind = g_gameReg->m_tileGrid->m_rows[tile.m_y][tile.m_x].m_typeCode;
    if (tileKind == TILEKIND_CHECKPOINT || tileKind == TILEKIND_CHECKPOINT_UP) {
        if (GRUNT_AT_SAVED_SCREEN_POS(this)) {

            m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        }
    }
    return 1;
}
RVA(0x000511b0, 0x246)
void CGrunt::FaceTowardPixel(i32 x, i32 y) {
    CWwdSpriteObject* h = m_object;
    Coord target(x, y);
    Coord position = h->ScreenPos();
    Coord delta = target - position;

    if (delta.m_x == 0) {
        if (target.m_y > position.m_y) {
            SetFacing(1000, g_gruntMoveDirSouth);
        } else if (target.m_y < position.m_y) {
            SetFacing(1000, g_gruntMoveDirNorth);
        }
        return;
    }

    float ratio = static_cast<float>(delta.m_y) / delta.m_x;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (target.m_y > position.m_y) {
            SetFacing(1000, g_gruntMoveDirSouth);
        } else {
            SetFacing(1000, g_gruntMoveDirNorth);
        }
        return;
    }
    if (ratio <= g_slopePosHalf && ratio >= g_slopeNegHalf) {
        if (target.m_x > position.m_x) {
            SetFacing(1000, g_gruntMoveDirEast);
        } else {
            SetFacing(1000, g_gruntMoveDirWest);
        }
        return;
    }
    if (ratio > g_slopePosHalf) {
        if (target.m_x > position.m_x) {
            SetFacing(1000, g_gruntMoveDirSouthEast);
        } else {
            SetFacing(1000, g_gruntMoveDirNorthWest);
        }
        return;
    }
    if (ratio < g_slopeNegHalf) {
        if (target.m_x > position.m_x) {
            SetFacing(1000, g_gruntMoveDirNorthEast);
        } else {
            SetFacing(1000, g_gruntMoveDirSouthWest);
        }
    }
}

RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (m_combatActive == false && m_stamina >= STAMINA_FULL && m_entranceActive == false) {
        return 1;
    }
    return 0;
}

RVA(0x000514e0, 0x1e)
void CGrunt::FaceTowardTile(i32 tileX, i32 tileY) {
    Coord tile(tileX, tileY);
    TileCenter(&tile);
    FaceTowardPixel(tile.m_x, tile.m_y);
}

// @early-stop
RVA(0x00051510, 0x20f)
i32 CGrunt::IsDropReady(i32 clearArrivalState) {
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        Coord commit = m_commitPx;
        ScreenTile(&commit);
        i32 owner;
        if (static_cast<u32>(commit.m_x) < static_cast<u32>(board->m_width)
            && static_cast<u32>(commit.m_y) < static_cast<u32>(board->m_height)) {
            owner = board->m_rows[commit.m_y][commit.m_x].m_occupantId;
        } else {
            owner = -1;
        }
        if (owner != -1) {
            return 0;
        }
    }

    CWwdSpriteObject* object = m_object;
    Coord position = object->ScreenPos();
    if (position == m_lastTilePx) {
        return 0;
    }

    if (m_coordList.GetCount() != 0) {
        Coord* coord = NULL;
        CoordPoolNode* node = g_coordPool.m_freeHead;
        Coord lastTile = m_lastTilePx;
        ScreenTile(&lastTile);
        if (node->m_next != NULL) {
            coord = &node->m_coord;
            *coord = lastTile;
            g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
        }
        m_coordList.AddHead(coord);
    }

    m_object->SetScreenPos(m_commitPx);
    object = m_object;
    if (object->m_sortKey != object->m_screenPosition.m_y + 0x186a0) {
        object->m_sortKey = object->m_screenPosition.m_y + 0x186a0;
        i32 flags = object->m_flags;
        object->m_flags = flags | IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
    }

    Coord oldTile = m_lastTilePx;
    ScreenTile(&oldTile);
    Coord newTile = m_commitPx;
    ScreenTile(&newTile);
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        board->m_rows[oldTile.m_y][oldTile.m_x].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
        board->m_rows[oldTile.m_y][oldTile.m_x].m_occupantId = -1;
    }
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 unitIndex = m_unitIndex;
        i32 playerIndex = m_playerIndex;
        board->m_rows[newTile.m_y][newTile.m_x].m_flags |= BRICKZ_CELL_OCCUPIED;
        board->m_rows[newTile.m_y][newTile.m_x].m_occupantId = (playerIndex << 8) | unitIndex;
    }

    m_lastTilePx = m_commitPx;
    m_commitPx = m_entrancePx;
    m_tileMoveCommitted = true;

    SetEntrancePos(clearArrivalState, 1);
    if (m_arrivalPending != false) {
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        m_arrivalPending = false;
    }
    return 1;
}

RVA(0x000517b0, 0x7d)
void CGrunt::SnapToLastTile(i32 clearArrivalState) {
    m_object->SetScreenPos(m_lastTilePx);
    CWwdSpriteObject* h = m_object;
    SET_SORT_KEY_IF_CHANGED(h, h->m_screenPosition.m_y + 0x186a0)
    SetEntrancePos(clearArrivalState, 1);
    if (m_arrivalPending != false) {

        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        m_arrivalPending = false;
    }
}

RVA(0x00051850, 0x165)
i32 CGrunt::RectContains(i32 x, i32 y) {
    Coord offset = LastTilePx();
    ScreenTile(&offset);
    Coord point(x, y);
    ScreenTile(&point);

    CRect r1 = m_reachRect;
    CRect r2 = m_reachExclusionRect;
    r1.OffsetRect(offset.m_x, offset.m_y);
    r1.InflateRect(0, 0, 1, 1);
    r2.OffsetRect(offset.m_x, offset.m_y);

    if (r1.IsRectEmpty() || r2.IsRectEmpty()) {
        if (r2.IsRectEmpty()) {

            if (::PtInRect(&r1, point.m_x, point.m_y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }

    if (::PtInRect(&r1, point.m_x, point.m_y)) {

        if (!::PtInRect(&r2, point.m_x, point.m_y)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00051a20, 0x17d)
i32 CGrunt::VehicleContactContains(i32 x, i32 y) {
    Coord offset = LastTilePx();
    ScreenTile(&offset);
    Coord point(x, y);
    ScreenTile(&point);

    CRect r1 = m_vehicleContactRect;
    CRect r2 = m_vehicleContactExclusionRect;
    r1.OffsetRect(offset.m_x, offset.m_y);
    r1.InflateRect(0, 0, 1, 1);
    r2.OffsetRect(offset.m_x, offset.m_y);

    if (m_vehiclePickupType == PICKUP_NONE) {
        return 0;
    }

    if (r1.IsRectEmpty() || r2.IsRectEmpty()) {
        if (r2.IsRectEmpty()) {
            if (::PtInRect(&r1, point.m_x, point.m_y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (::PtInRect(&r1, point.m_x, point.m_y)) {

        if (!::PtInRect(&r2, point.m_x, point.m_y)) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00051c00, 0xd20)
i32 CGrunt::StepCompassMove() {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    Coord position = m_lastTilePx;
    Coord tile = position;
    ScreenTile(&tile);
    i32 result = 0;
    Coord move;
    GruntDirectionCell facing;

    if (board->CellFlagsAt(tile.m_x, tile.m_y) & IDX(CELL_FLAG_ARROW)) {

        TileCollisionKind cmd = board->m_rows[tile.m_y][tile.m_x].m_typeCode;
        switch (cmd) {
            case TILEKIND_ARROW_UP_A:
            case TILEKIND_ARROW_UP_B:
                facing = g_gruntMoveDirNorth;
                move = position + GruntDirectionPixelOffset(facing);
                break;
            case TILEKIND_ARROW_RIGHT_A:
            case TILEKIND_ARROW_RIGHT_B:
                facing = g_gruntMoveDirEast;
                move = position + GruntDirectionPixelOffset(facing);
                break;
            case TILEKIND_ARROW_DOWN_A:
            case TILEKIND_ARROW_DOWN_B:
                facing = g_gruntMoveDirSouth;
                move = position + GruntDirectionPixelOffset(facing);
                break;
            case TILEKIND_ARROW_LEFT_A:
            case TILEKIND_ARROW_LEFT_B:
                facing = g_gruntMoveDirWest;
                move = position + GruntDirectionPixelOffset(facing);
                break;
            case TILEKIND_ARROW_CURRENT:
                switch (m_entranceCell.direction) {
                    case DIR_NORTH:
                        facing = g_gruntMoveDirNorth;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_EAST:
                        facing = g_gruntMoveDirEast;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_SOUTH:
                        facing = g_gruntMoveDirSouth;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_WEST:
                        facing = g_gruntMoveDirWest;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_NORTHEAST:
                        facing = g_gruntMoveDirNorthEast;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_SOUTHEAST:
                        facing = g_gruntMoveDirSouthEast;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_SOUTHWEST:
                        facing = g_gruntMoveDirSouthWest;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    case DIR_NORTHWEST:
                        facing = g_gruntMoveDirNorthWest;
                        move = position + GruntDirectionPixelOffset(facing);
                        break;
                    default:
                        move = position;
                        break;
                }
                break;
            default:
                move = position;
                break;
        }
        Coord moveTile = move;
        ScreenTile(&moveTile);
        i32 tflags = board->CellFlagsAt(moveTile.m_x, moveTile.m_y);
        if ((tflags & BRICKZ_CELL_OCCUPIED) && !(tflags & IDX(CELL_FLAG_ARROW))) {

            i32 owner;
            if (static_cast<u32>(moveTile.m_x) >= static_cast<u32>(board->m_width)
                || static_cast<u32>(moveTile.m_y) >= static_cast<u32>(board->m_height)) {
                owner = -1;
            } else {
                owner = board->m_rows[moveTile.m_y][moveTile.m_x].m_occupantId;
            }
            m_triggerMgr->StartUnitDeath(
                (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK,
                owner & GRUNT_IDENTITY_COMPONENT_MASK,
                DEATH_SQUASH,
                m_playerIndex
            );
        }
        goto commit;
    }

    if (m_toyTileIndex > 0) {
        CString str;
        switch (m_entranceReason) {
            case PICKUP_BABYWALKER:
                str = "BABYWALKERGRUNT";
                break;
            case PICKUP_BIGWHEEL:
                str = "BIGWHEELGRUNT";
                break;
            case PICKUP_GOKART:
                str = "GOKARTGRUNT";
                break;
            case PICKUP_POGOSTICK:
                str = "POGOSTICKGRUNT";
                break;
            default:
                break;
        }
        u32 toyCount =
            g_buteMgr.GetDword(const_cast<char*>(static_cast<LPCTSTR>(str)), s_ToyTiles, 1);
        if (m_toyTileIndex < toyCount) {
            switch (m_entranceCell.direction) {
                case DIR_NORTH:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorth);
                    facing = g_gruntMoveDirNorth;
                    break;
                case DIR_NORTHEAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorthEast);
                    facing = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirEast);
                    facing = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouthEast);
                    facing = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouth);
                    facing = g_gruntMoveDirSouth;
                    break;
                case DIR_SOUTHWEST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouthWest);
                    facing = g_gruntMoveDirSouthWest;
                    break;
                case DIR_WEST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirWest);
                    facing = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorthWest);
                    facing = g_gruntMoveDirNorthWest;
                    break;
                default:
                    move = position;
                    break;
            }
            result = s_CanCommitToyMove(this, move.m_x, move.m_y, position.m_x, position.m_y);
            if (0 == result) {
                m_toyTileIndex = 0;
            }
        } else {
            m_toyTileIndex = 0;
        }
    }
    if (result != 0) {
        goto commit;
    }

    {
        CByteArray bag;
        bag.SetAtGrow(bag.GetSize(), 1);
        bag.SetAtGrow(bag.GetSize(), 2);
        bag.SetAtGrow(bag.GetSize(), 3);
        bag.SetAtGrow(bag.GetSize(), 4);
        bag.SetAtGrow(bag.GetSize(), 5);
        bag.SetAtGrow(bag.GetSize(), 6);
        bag.SetAtGrow(bag.GetSize(), 7);
        bag.SetAtGrow(bag.GetSize(), 8);
        while (bag.GetSize() > 0) {
            i32 last = bag.GetUpperBound();
            i32 idx = GetRandom(0, last);
            i32 dir = bag.GetAt(idx);
            move = position;
            switch (static_cast<GruntDirection>(dir)) {
                case DIR_NORTH:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorth);
                    facing = g_gruntMoveDirNorth;
                    break;
                case DIR_NORTHEAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorthEast);
                    facing = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirEast);
                    facing = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouthEast);
                    facing = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouth);
                    facing = g_gruntMoveDirSouth;
                    break;
                case DIR_SOUTHWEST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirSouthWest);
                    facing = g_gruntMoveDirSouthWest;
                    break;
                case DIR_WEST:
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirWest);
                    facing = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    facing = g_gruntMoveDirNorthWest;
                    move = position + GruntDirectionPixelOffset(g_gruntMoveDirNorthWest);
                    break;
            }
            result = s_CanCommitBagMove(this, move.m_x, move.m_y, position.m_x, position.m_y);
            if (result != 0) {
                break;
            }
            bag.RemoveAt(idx, 1);
        }
        if (result == 0) {
            return 0;
        }
    }

commit:
    m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    SetFacing(0x3e8, facing);
    m_commitPx = m_lastTilePx;
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        Coord oldTile = m_lastTilePx;
        ScreenTile(&oldTile);
        b->m_rows[oldTile.m_y][oldTile.m_x].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
        b->m_rows[oldTile.m_y][oldTile.m_x].m_occupantId = -1;
    }
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        Coord newTile = move;
        ScreenTile(&newTile);
        i32 owner = (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
        b->m_rows[newTile.m_y][newTile.m_x].m_flags |= BRICKZ_CELL_OCCUPIED;
        b->m_rows[newTile.m_y][newTile.m_x].m_occupantId = owner;
    }
    m_lastTilePx = move;
    ComputeFacing(1.0);
    m_arrivalPending = true;
    m_toyTileIndex += 1;
    return 1;
}

// @early-stop
RVA(0x00052c70, 0x1e0)
i32 CGrunt::ClaimSwitchTile() {
    Coord tile = LastTilePx();
    Coord next = tile + GruntDirectionPixelOffset(m_entranceCell);

    CGruntzMapMgr* b = g_gameReg->GetTileGrid();
    Coord nextTile = next;
    ScreenTile(&nextTile);
    i32 flags = b->CellFlagsAt(nextTile.m_x, nextTile.m_y);
    if ((flags
         & (BRICKZ_CELL_OCCUPIED
            | IDX(
                CELL_FLAG_SOLID | CELL_FLAG_BRIDGE | CELL_FLAG_GRUNT_ENTRANCE_AREA
                | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD
            )))
        || (flags & IDX(CELL_FLAG_ARROW))) {
        return 0;
    }

    m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);

    m_commitPx = m_lastTilePx;
    CGruntzMapMgr* gb = g_gameReg->GetTileGrid();
    Coord oldTile = m_lastTilePx;
    ScreenTile(&oldTile);
    gb->m_rows[oldTile.m_y][oldTile.m_x].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
    gb->m_rows[oldTile.m_y][oldTile.m_x].m_occupantId = -1;

    CGruntzMapMgr* nb = g_gameReg->GetTileGrid();
    i32 owner = (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
    nb->m_rows[nextTile.m_y][nextTile.m_x].m_flags |= BRICKZ_CELL_OCCUPIED;
    nb->m_rows[nextTile.m_y][nextTile.m_x].m_occupantId = owner;

    m_lastTilePx = next;
    ComputeFacing(1.0);
    m_arrivalPending = true;
    return 1;
}

RVA(0x00052ed0, 0x42)
i32 CGrunt::SetArrivalTarget(
    i32 targetPlayerIndex,
    i32 targetUnitIndex,
    i32 targetPxX,
    i32 targetPxY
) {
    Coord cell(targetPlayerIndex, targetUnitIndex);
    m_arrivalCell = cell;
    m_arrivalActive = true;
    m_defenderPx.Set(targetPxX, targetPxY);
    SnapTileCenter(&m_defenderPx);
    return 1;
}

// @early-stop
RVA(0x00052f40, 0x4b)
void CGrunt::ConsiderArrival(i32 clearArrivalState) {
    CWwdSpriteObject* h = m_object;
    Coord tile = m_lastTilePx;
    Coord pixel = h->ScreenPos();
    SnapTileCenter(&pixel);
    if (pixel != tile) {
        if (IsDropReady(clearArrivalState)) {
            return;
        }
    }
    SnapToLastTile(clearArrivalState);
}

// @early-stop
RVA(0x00052fb0, 0x96e)
i32 CGrunt::TryTeleportToCell(i32 tileX, i32 tileY, b32 useSecretColor, b32 spawnWormhole) {
    if (m_entranceCommitted == false) {
        return 1;
    }
    Coord tile(tileX, tileY);
    i32 flags = g_gameReg->m_tileGrid->CellFlagsAt(tile.m_x, tile.m_y);
    if ((flags
         & IDX(
             CELL_FLAG_SOLID | CELL_FLAG_BRIDGE | CELL_FLAG_GRUNT_ENTRANCE_AREA
             | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_WATER | CELL_FLAG_SPIKES
             | CELL_FLAG_SINK_HAZARD
         ))
        || (flags & IDX(CELL_FLAG_SPECIAL | CELL_FLAG_ARROW))) {
        return 0;
    }

    bool eq;
    eq = ANIMATION_ACT_DIFFERS("A");
    if (!eq) {
        goto applyTail;
    }
    eq = ANIMATION_ACT_DIFFERS("D");
    if (!eq) {
        goto applyTail;
    }
    eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {

        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
        }
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
        if (m_entranceReason != PICKUP_BOMB) {
            goto applyTail;
        }
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        return 1;
    }
    eq = ANIMATION_ACT_EQUALS("G");
    if (!eq) {
        eq = ANIMATION_ACT_EQUALS("L");
        if (!eq) {
            eq = ANIMATION_ACT_EQUALS("P");
            if (!eq) {
                eq = ANIMATION_ACT_EQUALS("O");
                if (eq) {

                    SnapToLastTile(1);
                    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                    goto applyTail;
                }
                eq = ANIMATION_ACT_EQUALS("Q");
                if (eq) {
                    return 1;
                }
                eq = ANIMATION_ACT_EQUALS("J");
                if (eq) {

                    m_entranceActive = false;
                    eq = (strcmp(*g_typeColl.GetNameRecord(m_previousAnimationActId), "D") == 0);
                    if (eq) {
                        if (m_poweredUp != false && m_neighborValid == false) {
                            RESET_GRUNT_POWERED_STATE(this)
                        }
                        m_tileMoveCommitted = false;
                        SET_ANIMATION_ACT("D");
                        SwitchAnimation(m_poseWalk);

                        char* nm = EntranceCell()->WalkName().GetBuffer(0);
                        SetImageSetByName(nm);
                    } else {
                        ResetEntranceAnimation(1, 0, 0);
                    }

                    PickupType mode = m_entrancePickup;
                    if (mode >= PICKUP_POWERUPZ_FIRST) {
                        LoadGruntTypeTable(mode, 1, 0, 1);
                        m_entrancePickup = PICKUP_INVALID;
                        m_helpCueId = 0;
                        goto applyTail;
                    }
                    if (mode >= PICKUP_BRICKZ_FIRST) {
                        m_brickPickupType = mode;
                        m_entrancePickup = PICKUP_INVALID;
                        goto applyTail;
                    }
                    if (mode >= PICKUP_TOYZ_FIRST) {
                        LoadVehicleGruntSprites(mode);
                        goto applyTail;
                    }
                    LoadGruntTypeTable(mode, 1, 0, 1);
                    m_entrancePickup = PICKUP_INVALID;
                    goto applyTail;
                }
                {
                    CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                    ActNameConstructGrownSlots();
                    eq = (strcmp(*rec, "N") == 0);
                }
                if (eq) {

                    CWwdSpriteObject* h = m_object;
                    Coord saved = m_lastTilePx;
                    Coord pixel = h->ScreenPos();
                    SnapTileCenter(&pixel);
                    i32 redo = 1;
                    if (pixel != saved) {
                        if (IsDropReady(1)) {
                            m_coordToggle = (m_coordToggle == false);
                            redo = 0;
                        }
                    }
                    SnapToLastTile(1);
                    if (redo == 0) {
                        goto applyTail;
                    }
                    SET_ANIMATION_ACT("D");
                    SetupTubeAnim(m_coordToggle);
                    goto applyTail;
                }
                {
                    CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                    ActNameConstructGrownSlots();
                    eq = (strcmp(*rec, "M") == 0);
                }
                if (eq) {
                    m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
                    return 1;
                }
                goto applyTail;
            }
        }
    }

idleReseed:

    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 z = m_object->m_screenPosition.m_y + 0x186a0;
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, z)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopVehicleLoopSound();

applyTail:

    if (m_wingzEnabled != false) {
        LoadWingzGruntSprites(false);
    }
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    m_triggerMgr->ApplySwitch(this, m_object->m_screenPosition.m_x, m_object->m_screenPosition.m_y);
    {
        Coord spawn = tile;
        TileCenter(&spawn);
        m_object->SetScreenPos(spawn);
        {
            CGruntzMapMgr* board = g_gameReg->m_tileGrid;
            Coord oldTile = m_lastTilePx;
            ScreenTile(&oldTile);
            BrickzCell& oldCell = board->m_rows[oldTile.m_y][oldTile.m_x];
            oldCell.m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
            oldCell.m_occupantId = -1;
            m_lastTilePx.Set(-1, -1);
        }
        SetEntrancePos(1, 1);
        if (CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS_EXPANDED(this)
        }
        if (m_arrivalState == AI_BATTLEZ_PATH) {
            m_defenderState = AISTATE_SEEK;
            m_routePassableMask = 0;
        }
        if (spawnWormhole != false) {
            CWwdSpriteObject* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                spawn.m_x,
                spawn.m_y,
                0,
                "Wormhole",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            if (spawned != NULL) {
                if (useSecretColor != false) {
                    spawned->m_smarts = g_buteMgr.GetInt("Wormhole", "SecretColor", 1);
                } else {
                    spawned->m_smarts = g_buteMgr.GetInt("Wormhole", "EntranceColor", 3);
                }
            }
        }
    }
    BuildEntranceAnimation(GRUNT_ENTRANCE_WORMHOLE);
    return 1;
}

RVA(0x00053b80, 0x340)
i32 CGrunt::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }

    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)

    if (CWapX::SerializeAnimationState(ar, mode, typeId, object) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:

            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:

            if (LoadStateRecord(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            m_triggerMgr = g_gameReg->m_triggerMgr;
            break;
    }
    m_entranceCell.Serialize(ar, mode, typeId, object);
    SerializeClockPair(ar, mode, &m_toyClock);
    SerializeClockPair(ar, mode, &m_idleAnchor);
    SerializeClockPair(ar, mode, &m_idleTimer);
    SerializeClockPair(ar, mode, &m_entranceClock64);
    SerializeClockPair(ar, mode, &m_flashClock64);
    SerializeClockPair(ar, mode, &m_attackClock64);
    SerializeClockPair(ar, mode, &m_combatClock64);
    SerializeClockPair(ar, mode, &m_hudRetireClock64);
    m_wingzTiming.Serialize(ar, mode, typeId, object);
    m_conversionTiming.Serialize(ar, mode, typeId, object);
    m_shimmerTiming.Serialize(ar, mode, typeId, object);
    m_arrivalVoiceTiming.Serialize(ar, mode, typeId, object);
    m_arrivalRerollTiming.Serialize(ar, mode, typeId, object);
    m_holdTiming.Serialize(ar, mode, typeId, object);
    return 1;
}

RVA(0x00053f90, 0x11d0)
i32 CGrunt::Save(CFileMemBase* ar) {
    if (!ar) {
        return 0;
    }

    CDDrawSurfaceMgr* world = m_ownerLogicRecord->m_ownerCtx;
    if (!world) {
        return 0;
    }
    i32 count;
    char nameBuffer[SERIAL_NAME_LEN];
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_selectedSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_toySprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_healthSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_staminaSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_toyTimeSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_wingzTimeSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    {
        i32 spriteObjectId = 0;
        CWwdSpriteObject* sprite = m_powerupSprite;
        if (sprite) {
            spriteObjectId = sprite->m_objectId;
        }
        ar->Write(&spriteObjectId, sizeof(spriteObjectId));
    }
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    strcpy(nameBuffer, static_cast<const char*>(m_animSetName));
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    strcpy(nameBuffer, m_frameSetName);
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    strcpy(nameBuffer, m_deathFrameSetName);
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = m_poseWalk;
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseAttack, GRUNT_ATTACK1);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseAttack, GRUNT_ATTACK2);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = m_poseAttackIdle;
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseStruck, GRUNT_STRUCK1);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseStruck, GRUNT_STRUCK2);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseIdle, GRUNT_IDLE1);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseIdle, GRUNT_IDLE2);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseIdle, GRUNT_IDLE3);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseIdle, GRUNT_IDLE4);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseIdle, GRUNT_IDLE5);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = m_poseDeath;
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseToy, GRUNT_TOY1);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseToy, GRUNT_TOY2);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseToy, GRUNT_TOY_BREAK);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseItem, GRUNT_ITEM1);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = AT(m_poseItem, GRUNT_ITEM2);
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(nameBuffer, 0, SERIAL_NAME_LEN);
    {
        CAniElement* animation = m_pickupGeoSrc;
        if (animation) {
            strcpy(nameBuffer, world->m_animRegistry->FindAnimationKey(animation));
        }
    }
    ar->Write(nameBuffer, SERIAL_NAME_LEN);
    ar->Write(&m_reserved18c, sizeof(m_reserved18c));
    ar->Write(&m_toyBlendPct, sizeof(m_toyBlendPct));
    ar->Write(&m_brickPickupType, sizeof(m_brickPickupType));
    ar->Write(&m_entranceReason, sizeof(m_entranceReason));
    ar->Write(&m_vehiclePickupType, sizeof(m_vehiclePickupType));
    ar->Write(&m_toolId, sizeof(m_toolId));
    ar->Write(&m_entrancePickup, sizeof(m_entrancePickup));
    ar->Write(&m_helpCueId, sizeof(m_helpCueId));
    ar->Write(&m_reserved1a8, sizeof(m_reserved1a8));
    ar->Write(&m_reserved1ac, sizeof(m_reserved1ac));
    ar->Write(&m_reserved1b0, sizeof(m_reserved1b0));
    ar->Write(&m_reserved1b4, sizeof(m_reserved1b4));
    ar->Write(&m_arrived, sizeof(m_arrived));
    ar->Write(&m_entrancePx, sizeof(m_entrancePx));
    ar->Write(&m_lastTilePx, sizeof(m_lastTilePx));
    ar->Write(&m_commitPx, sizeof(m_commitPx));
    ar->Write(&m_reserved1dc, sizeof(m_reserved1dc));
    ar->Write(&m_entranceActive, sizeof(m_entranceActive));
    ar->Write(&m_arrivalPending, sizeof(m_arrivalPending));
    ar->Write(&m_playerIndex, sizeof(m_playerIndex));
    ar->Write(&m_unitIndex, sizeof(m_unitIndex));
    ar->Write(&m_moveIcon, sizeof(m_moveIcon));
    ar->Write(&m_savedMoveIcon, sizeof(m_savedMoveIcon));
    ar->Write(&m_entranceCommitted, sizeof(m_entranceCommitted));
    ar->Write(&m_neighborPlayerIndex, sizeof(m_neighborPlayerIndex) + sizeof(m_neighborUnitIndex));
    ar->Write(&m_attackTargetPx, sizeof(m_attackTargetPx));
    ar->Write(&m_reserved210, sizeof(m_reserved210));
    ar->Write(&m_struckPose, sizeof(m_struckPose));
    ar->Write(&m_combatActive, sizeof(m_combatActive));
    ar->Write(&m_neighborValid, sizeof(m_neighborValid));
    ar->Write(&m_poweredUp, sizeof(m_poweredUp));
    ar->Write(&m_daFlag, sizeof(m_daFlag));
    ar->Write(&m_entranceStamped, sizeof(m_entranceStamped));
    ar->Write(&m_bombRunActive, sizeof(m_bombRunActive));
    ar->Write(&m_arrivalActive, sizeof(m_arrivalActive));
    ar->Write(&m_reachRect, sizeof(m_reachRect));
    ar->Write(&m_reachExclusionRect, sizeof(m_reachExclusionRect));
    ar->Write(&m_vehicleContactRect, sizeof(m_vehicleContactRect));
    ar->Write(&m_vehicleContactExclusionRect, sizeof(m_vehicleContactExclusionRect));
    ar->Write(&m_health, sizeof(m_health));
    ar->Write(&m_stamina, sizeof(m_stamina));
    ar->Write(&m_toyTime, sizeof(m_toyTime));
    ar->Write(&m_wingzTime, sizeof(m_wingzTime));
    ar->Write(&m_moveSpeed, sizeof(m_moveSpeed));
    ar->Write(&m_reserved418, sizeof(m_reserved418));
    ar->Write(&m_reserved42c, sizeof(m_reserved42c));
    ar->Write(&m_reserved430, sizeof(m_reserved430));
    ar->Write(&m_startingItemId, sizeof(m_startingItemId));
    ar->Write(&m_recordedFrameTick, sizeof(m_recordedFrameTick));
    ar->Write(&m_arrivalState, sizeof(m_arrivalState));
    ar->Write(&m_defenderState, sizeof(m_defenderState));
    ar->Write(&m_battleState, sizeof(m_battleState));
    ar->Write(&m_defenderRadius, sizeof(m_defenderRadius));
    ar->Write(&m_defenderQueuePosition, sizeof(m_defenderQueuePosition));
    ar->Write(&m_defenderPickupType, sizeof(m_defenderPickupType));
    ar->Write(&m_dwell, sizeof(m_dwell));
    ar->Write(&m_arrivalCell, sizeof(m_arrivalCell));
    ar->Write(&m_defenderPx, sizeof(m_defenderPx));
    ar->Write(&m_toolConfigured, sizeof(m_toolConfigured));
    ar->Write(&m_neighborScanEnabled, sizeof(m_neighborScanEnabled));
    ar->Write(&m_tileMoveCommitted, sizeof(m_tileMoveCommitted));
    ar->Write(&m_reserved3dc, sizeof(m_reserved3dc));
    ar->Write(&m_moveTile, sizeof(m_moveTile));
    ar->Write(&m_arrivalPhase, sizeof(m_arrivalPhase));
    ar->Write(&m_timePerTile, sizeof(m_timePerTile));
    ar->Write(&m_movePosition.x, sizeof(m_movePosition.x));
    ar->Write(&m_movePosition.y, sizeof(m_movePosition.y));
    ar->Write(&m_reserved8d0, sizeof(m_reserved8d0));
    ar->Write(&m_coordToggle, sizeof(m_coordToggle));
    ar->Write(&m_wingzEnabled, sizeof(m_wingzEnabled));
    ar->Write(&m_freezeDelayDone, sizeof(m_freezeDelayDone));
    ar->Write(&m_freezeUnfrozen, sizeof(m_freezeUnfrozen));
    ar->Write(&m_resetApplied, sizeof(m_resetApplied));
    ar->Write(&m_arrivalFlags, sizeof(m_arrivalFlags));
    ar->Write(&m_passableMask, sizeof(m_passableMask));
    ar->Write(&m_gruntKind, sizeof(m_gruntKind));
    ar->Write(&m_entranceArmed, sizeof(m_entranceArmed));
    ar->Write(&m_deathType, sizeof(m_deathType));
    ar->Write(&m_entranceDropActive, sizeof(m_entranceDropActive));
    ar->Write(&m_hasExtent, sizeof(m_hasExtent));
    ar->Write(&m_unusedBattleCell, sizeof(m_unusedBattleCell));
    ar->Write(&m_cellRemovalNotified, sizeof(m_cellRemovalNotified));
    ar->Write(&m_pendingTrigger, sizeof(m_pendingTrigger));
    ar->Write(&m_killerPlayerIndex, sizeof(m_killerPlayerIndex));
    ar->Write(&m_tileClaimed, sizeof(m_tileClaimed));
    ar->Write(&m_deathAnimStarted, sizeof(m_deathAnimStarted));
    ar->Write(&m_pendingTriggerPx, sizeof(m_pendingTriggerPx));
    ar->Write(&m_routeBlockedMask, sizeof(m_routeBlockedMask));
    ar->Write(&m_routePassableMask, sizeof(m_routePassableMask));
    ar->Write(&m_moveVariantOverride, sizeof(m_moveVariantOverride));
    ar->Write(&m_moveKind, sizeof(m_moveKind));
    ar->Write(&m_moveVariant, sizeof(m_moveVariant));
    ar->Write(&m_coordRetryCount, sizeof(m_coordRetryCount));
    ar->Write(&m_toyTileIndex, sizeof(m_toyTileIndex));
    ar->Write(&m_blockedVoicePending, sizeof(m_blockedVoicePending));
    ar->Write(&m_powerupDuration, sizeof(m_powerupDuration));
    ar->Write(&m_warpstoneAnchorIndex, sizeof(m_warpstoneAnchorIndex));
    ar->Write(&m_lowStaminaCued, sizeof(m_lowStaminaCued));
    ar->Write(&m_targetTeam, sizeof(m_targetTeam));
    ar->Write(&m_arrivalTargetPx, sizeof(m_arrivalTargetPx));

    {
        i32 row, col;
        for (row = 0; row < 3; row++) {
            for (col = 0; col < 3; col++) {
                if (m_cells[3 * row + col].SerializeStrings(ar) == 0) {
                    return 0;
                }
            }
        }
    }

    {
        count = m_coordList.GetCount();
        ar->Write(&count, sizeof(count));
        POSITION cpos = m_coordList.GetHeadPosition();
        while (cpos != NULL) {
            ar->Write(m_coordList.GetNext(cpos), 8);
        }
    }
    {
        count = m_payloads.GetCount();
        ar->Write(&count, sizeof(count));
        POSITION pos = m_payloads.GetHeadPosition();
        while (pos != NULL) {
            ar->Write(m_payloads.GetNext(pos), 0x2c);
        }
    }
    return 1;
}
