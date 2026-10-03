#include <StdAfx.h>

#include <Ints.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateRecord.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntActionInline.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/MapTraversalInline.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileCoordMacros.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <RectMacros.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

GruntDirectionCell g_gruntMoveDirNorth = GruntDirectionCell(0, 1, DIR_NORTH);

GruntDirectionCell g_gruntMoveDirNorthEast = GruntDirectionCell(0, 2, DIR_NORTHEAST);

GruntDirectionCell g_gruntMoveDirEast = GruntDirectionCell(1, 2, DIR_EAST);

GruntDirectionCell g_gruntMoveDirSouthEast = GruntDirectionCell(2, 2, DIR_SOUTHEAST);

GruntDirectionCell g_gruntMoveDirCenter = GruntDirectionCell(1, 1, DIR_CENTER);

GruntDirectionCell g_gruntMoveDirSouth = GruntDirectionCell(2, 1, DIR_SOUTH);

GruntDirectionCell g_gruntMoveDirSouthWest = GruntDirectionCell(2, 0, DIR_SOUTHWEST);

GruntDirectionCell g_gruntMoveDirWest = GruntDirectionCell(1, 0, DIR_WEST);

GruntDirectionCell g_gruntMoveDirNorthWest = GruntDirectionCell(0, 0, DIR_NORTHWEST);

static char s_entranceSafeTime[] = "EntranceSafeTime";

static char s_toyTiles[] = "ToyTiles";

i32 CGrunt::LoadTypeTableClearMove(PickupType typeId) {

    i32 r = LoadGruntTypeTable(typeId, 0, 0, 0);
    m_entrancePickup = PICKUP_INVALID;
    m_helpCueId = 0;
    return r;
}

i32 CGrunt::LoadVehicleGruntSprites(PickupType kind) {
    m_vehiclePickupType = kind;
    m_entrancePickup = PICKUP_INVALID;

    CString name;

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

    TileCollisionKind tileKind = g_gameReg->m_tileGrid->CellTypeAt(
        m_lastTilePx.m_x >> TILE_SHIFT_PX,
        m_lastTilePx.m_y >> TILE_SHIFT_PX
    );
    if (tileKind == TILEKIND_CHECKPOINT || tileKind == TILEKIND_CHECKPOINT_UP) {
        if (IsGruntAtSavedScreenPos(this)) {
            Coord tile = LastTilePx();
            m_triggerMgr->ApplySwitch(this, tile.m_x, tile.m_y);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        }
    }
    return 1;
}

void CGrunt::FaceTowardPixel(i32 x, i32 y) {
    CWwdSpriteObject* h = m_object;
    i32 dy = y - h->m_screenY;
    i32 dx = x - h->m_screenX;
    i32 cx = h->m_screenX;

    if (dx == 0) {
        if (y > h->m_screenY) {
            SetFacing(1000, g_gruntMoveDirSouth);
        } else if (y < h->m_screenY) {
            SetFacing(1000, g_gruntMoveDirNorth);
        }
        return;
    }

    float ratio = static_cast<float>(dy) / dx;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (y > h->m_screenY) {
            SetFacing(1000, g_gruntMoveDirSouth);
        } else {
            SetFacing(1000, g_gruntMoveDirNorth);
        }
        return;
    }
    if (ratio <= g_slopePosHalf && ratio >= g_slopeNegHalf) {
        if (x > cx) {
            SetFacing(1000, g_gruntMoveDirEast);
        } else {
            SetFacing(1000, g_gruntMoveDirWest);
        }
        return;
    }
    if (ratio > g_slopePosHalf) {
        if (x > cx) {
            SetFacing(1000, g_gruntMoveDirSouthEast);
        } else {
            SetFacing(1000, g_gruntMoveDirNorthWest);
        }
        return;
    }
    if (ratio < g_slopeNegHalf) {
        if (x > cx) {
            SetFacing(1000, g_gruntMoveDirNorthEast);
        } else {
            SetFacing(1000, g_gruntMoveDirSouthWest);
        }
    }
}

i32 CGrunt::CanShowStamina() {
    if (m_combatActive == false && m_stamina >= STAMINA_FULL && m_entranceActive == false) {
        return 1;
    }
    return 0;
}

void CGrunt::FaceTowardTile(i32 tileX, i32 tileY) {
    FaceTowardPixel(tileX * 0x20 + 0x10, tileY * 0x20 + 0x10);
}

i32 CGrunt::IsDropReady(i32 clearArrivalState) {
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 x = m_commitPx.m_x >> TILE_SHIFT_PX;
        i32 y = m_commitPx.m_y >> TILE_SHIFT_PX;
        i32 owner = board->OccupantAt(x, y);
        if (owner != -1) {
            return 0;
        }
    }

    CWwdSpriteObject* object = m_object;
    i32 lastX = m_lastTilePx.m_x;
    if (object->m_screenX == lastX) {
        i32 lastY = m_lastTilePx.m_y;
        if (object->m_screenY == lastY) {
            return 0;
        }
    }

    if (!m_coordList.IsEmpty()) {
        Coord tile;
        tile.Set(m_lastTilePx.m_x >> TILE_SHIFT_PX, m_lastTilePx.m_y >> TILE_SHIFT_PX);
        m_coordList.AddHead(g_coordPool.PopCopy(tile));
    }

    SET_SCREEN_POS(m_object, m_commitPx.m_x, m_commitPx.m_y);
    object = m_object;
    if (object->m_sortKey != object->m_screenY + 0x186a0) {
        object->m_sortKey = object->m_screenY + 0x186a0;
        i32 flags = object->m_flags;
        object->m_flags = flags | IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
    }

    i32 oldY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 oldX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 newX = m_commitPx.m_x >> TILE_SHIFT_PX;
    i32 newY = m_commitPx.m_y >> TILE_SHIFT_PX;
    g_gameReg->m_tileGrid->ReleaseCellOccupancy(oldX, oldY);
    g_gameReg->m_tileGrid->AcquireCellOccupancy(newX, newY, m_playerIndex, m_unitIndex);

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

void CGrunt::SnapToLastTile(i32 clearArrivalState) {
    SET_SCREEN_POS(m_object, m_lastTilePx.m_x, m_lastTilePx.m_y);
    CWwdSpriteObject* h = m_object;
    SET_SORT_KEY_IF_CHANGED(h, h->m_screenY + 0x186a0)
    SetEntrancePos(clearArrivalState, 1);
    if (m_arrivalPending != false) {

        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        m_arrivalPending = false;
    }
}

i32 CGrunt::RectContains(i32 x, i32 y) {
    i32 dx = LastTilePx().m_x >> TILE_SHIFT_PX;
    i32 dy = LastTilePx().m_y >> TILE_SHIFT_PX;
    x >>= TILE_SHIFT_PX;
    y >>= TILE_SHIFT_PX;

    RECT r1 = m_reachRect;
    RECT r2 = m_reachExclusionRect;
    OFFSET_RECT_COMPONENTS(r1, dx, dy);
    r1.right++;
    r1.bottom++;
    OFFSET_RECT_COMPONENTS(r2, dx, dy);

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {

            if (::PtInRect(&r1, x, y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }

    if (::PtInRect(&r1, x, y)) {

        if (!::PtInRect(&r2, x, y)) {
            return 1;
        }
    }
    return 0;
}

i32 CGrunt::VehicleContactContains(i32 x, i32 y) {
    i32 dx = LastTilePx().m_x >> TILE_SHIFT_PX;
    i32 dy = LastTilePx().m_y >> TILE_SHIFT_PX;
    x >>= TILE_SHIFT_PX;
    y >>= TILE_SHIFT_PX;

    RECT r1 = m_vehicleContactRect;
    RECT r2 = m_vehicleContactExclusionRect;
    OFFSET_RECT_COMPONENTS(r1, dx, dy);
    r1.right++;
    r1.bottom++;
    OFFSET_RECT_COMPONENTS(r2, dx, dy);

    if (m_vehiclePickupType == PICKUP_NONE) {
        return 0;
    }

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            if (::PtInRect(&r1, x, y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (::PtInRect(&r1, x, y)) {

        if (!::PtInRect(&r2, x, y)) {
            return 1;
        }
    }
    return 0;
}

i32 CGrunt::StepCompassMove() {
    CGruntzMapMgr* board = g_gameReg->GetTileGrid();
    Coord tile = LastTilePx();
    Coord sourceCell = tile;
    ScreenTile(&sourceCell);
    i32 result = 0;
    Coord next;
    GruntDirectionCell facing;

    if (board->CellFlagsAt(sourceCell.m_x, sourceCell.m_y) & 0x80) {

        TileCollisionKind cmd = board->m_rows[sourceCell.m_y][sourceCell.m_x].m_typeCode;
        switch (cmd) {
            case TILEKIND_ARROW_UP_A:
            case TILEKIND_ARROW_UP_B:
                tile.m_y -= 0x20;
                next = tile;
                facing = g_gruntMoveDirNorth;
                break;
            case TILEKIND_ARROW_RIGHT_A:
            case TILEKIND_ARROW_RIGHT_B:
                tile.m_x += 0x20;
                next = tile;
                facing = g_gruntMoveDirEast;
                break;
            case TILEKIND_ARROW_DOWN_A:
            case TILEKIND_ARROW_DOWN_B:
                tile.m_y += 0x20;
                next = tile;
                facing = g_gruntMoveDirSouth;
                break;
            case TILEKIND_ARROW_LEFT_A:
            case TILEKIND_ARROW_LEFT_B:
                tile.m_x -= 0x20;
                next = tile;
                facing = g_gruntMoveDirWest;
                break;
            case TILEKIND_ARROW_CURRENT:
                switch (m_entranceCell.m_direction) {
                    case DIR_NORTH:
                        tile.m_y -= 0x20;
                        next = tile;
                        facing = g_gruntMoveDirNorth;
                        break;
                    case DIR_EAST:
                        tile.m_x += 0x20;
                        next = tile;
                        facing = g_gruntMoveDirEast;
                        break;
                    case DIR_SOUTH:
                        tile.m_y += 0x20;
                        next = tile;
                        facing = g_gruntMoveDirSouth;
                        break;
                    case DIR_WEST:
                        tile.m_x -= 0x20;
                        next = tile;
                        facing = g_gruntMoveDirWest;
                        break;
                    case DIR_NORTHEAST:
                        tile.m_x += 0x20;
                        tile.m_y -= 0x20;
                        next = tile;
                        facing = g_gruntMoveDirNorthEast;
                        break;
                    case DIR_SOUTHEAST:
                        tile.m_x += 0x20;
                        tile.m_y += 0x20;
                        next = tile;
                        facing = g_gruntMoveDirSouthEast;
                        break;
                    case DIR_SOUTHWEST:
                        tile.m_x -= 0x20;
                        tile.m_y += 0x20;
                        next = tile;
                        facing = g_gruntMoveDirSouthWest;
                        break;
                    case DIR_NORTHWEST:
                        tile.m_x -= 0x20;
                        tile.m_y -= 0x20;
                        next = tile;
                        facing = g_gruntMoveDirNorthWest;
                        break;
                    default:
                        next = tile;
                        break;
                }
                break;
            default:
                next = tile;
                break;
        }
        i32 mtx = next.m_x >> TILE_SHIFT_PX;
        i32 mty = next.m_y >> TILE_SHIFT_PX;
        i32 tflags = board->CellFlagsAt(mtx, mty);
        if ((tflags & BRICKZ_CELL_OCCUPIED) && !(tflags & 0x80)) {

            i32 owner = board->OccupantAt(mtx, mty);
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
            g_buteMgr.GetDword(const_cast<char*>(static_cast<LPCTSTR>(str)), s_toyTiles, 1);
        if (m_toyTileIndex < toyCount) {
            switch (m_entranceCell.m_direction) {
                case DIR_NORTH:
                    next.Set(tile.m_x, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorth;
                    break;
                case DIR_NORTHEAST:
                    next.Set(tile.m_x + 0x20, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    next.Set(tile.m_x + 0x20, tile.m_y);
                    facing = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    next.Set(tile.m_x + 0x20, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    next.Set(tile.m_x, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouth;
                    break;
                case DIR_SOUTHWEST:
                    next.Set(tile.m_x - 0x20, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouthWest;
                    break;
                case DIR_WEST:
                    next.Set(tile.m_x - 0x20, tile.m_y);
                    facing = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    next.Set(tile.m_x - 0x20, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorthWest;
                    break;
                default:
                    next = tile;
                    break;
            }
            CMapMgr* grid = g_gameReg->GetTileGrid();
            i32 blockedMask = m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
            if (grid->CanStepBetween(
                    sourceCell.m_x,
                    sourceCell.m_y,
                    next.m_x >> TILE_SHIFT_PX,
                    next.m_y >> TILE_SHIFT_PX,
                    blockedMask,
                    m_passableMask | 0x18000482
                )
                != 0) {
                result = 1;
            } else {
                m_toyTileIndex = 0;
            }
        } else {
            next = tile;
            m_toyTileIndex = 0;
        }
    } else {
        next = tile;
    }
    if (result != 0) {
        goto commit;
    }

    {
        CByteArray bag;
        bag.Add(1);
        bag.Add(2);
        bag.Add(3);
        bag.Add(4);
        bag.Add(5);
        bag.Add(6);
        bag.Add(7);
        bag.Add(8);
        while (result == 0 && bag.GetSize() > 0) {
            i32 idx = GetRandom(0, bag.GetUpperBound());
            i32 dir = bag.GetAt(idx);
            switch (static_cast<GruntDirection>(dir)) {
                case DIR_NORTH:
                    next.Set(tile.m_x, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorth;
                    break;
                case DIR_NORTHEAST:
                    next.Set(tile.m_x + 0x20, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    next.Set(tile.m_x + 0x20, tile.m_y);
                    facing = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    next.Set(tile.m_x + 0x20, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    next.Set(tile.m_x, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouth;
                    break;
                case DIR_SOUTHWEST:
                    next.Set(tile.m_x - 0x20, tile.m_y + 0x20);
                    facing = g_gruntMoveDirSouthWest;
                    break;
                case DIR_WEST:
                    next.Set(tile.m_x - 0x20, tile.m_y);
                    facing = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    next.Set(tile.m_x - 0x20, tile.m_y - 0x20);
                    facing = g_gruntMoveDirNorthWest;
                    break;
            }
            CMapMgr* grid = g_gameReg->GetTileGrid();
            i32 blockedMask = m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
            if (grid->CanStepBetween(
                    sourceCell.m_x,
                    sourceCell.m_y,
                    next.m_x >> TILE_SHIFT_PX,
                    next.m_y >> TILE_SHIFT_PX,
                    blockedMask,
                    m_passableMask | 0x18000482
                )
                != 0) {
                result = 1;
            } else {
                bag.RemoveAt(idx, 1);
            }
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
        CGruntzMapMgr* b = g_gameReg->GetTileGrid();
        i32 ox = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        b->ReleaseCellOccupancy(ox, oy);
    }
    {
        CGruntzMapMgr* b = g_gameReg->GetTileGrid();
        i32 nx = next.m_x >> TILE_SHIFT_PX;
        i32 ny = next.m_y >> TILE_SHIFT_PX;
        b->AcquireCellOccupancy(nx, ny, m_playerIndex, m_unitIndex);
    }
    m_lastTilePx = next;
    ComputeFacing(1.0);
    m_arrivalPending = true;
    m_toyTileIndex += 1;
    return 1;
}

i32 CGrunt::ClaimSwitchTile() {
    Coord tile = LastTilePx();
    Coord next;
    switch (m_entranceCell.m_direction) {
        case DIR_NORTH:
            next.Set(tile.m_x, tile.m_y - 0x20);
            break;
        case DIR_NORTHEAST:
            next.Set(tile.m_x + 0x20, tile.m_y - 0x20);
            break;
        case DIR_EAST:
            next.Set(tile.m_x + 0x20, tile.m_y);
            break;
        case DIR_SOUTHEAST:
            next.Set(tile.m_x + 0x20, tile.m_y + 0x20);
            break;
        case DIR_SOUTH:
            next.Set(tile.m_x, tile.m_y + 0x20);
            break;
        case DIR_SOUTHWEST:
            next.Set(tile.m_x - 0x20, tile.m_y + 0x20);
            break;
        case DIR_WEST:
            next.Set(tile.m_x - 0x20, tile.m_y);
            break;
        case DIR_NORTHWEST:
            next.Set(tile.m_x - 0x20, tile.m_y - 0x20);
            break;
    }

    Coord cell = next;
    ScreenTile(&cell);
    i32 flags = g_gameReg->GetTileGrid()->CellFlagsAt(cell.m_x, cell.m_y);
    if ((flags & 0x20000939) || (flags & 0x80)) {
        return 0;
    }

    m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);

    m_commitPx = m_lastTilePx;
    Coord oldCell = LastTilePx();
    ScreenTile(&oldCell);
    g_gameReg->GetTileGrid()->ReleaseCellOccupancy(oldCell.m_x, oldCell.m_y);
    g_gameReg->GetTileGrid()->AcquireCellOccupancy(cell.m_x, cell.m_y, m_playerIndex, m_unitIndex);

    m_lastTilePx = next;
    ComputeFacing(1.0);
    m_arrivalPending = true;
    return 1;
}

i32 CGrunt::SetArrivalTarget(
    i32 targetPlayerIndex,
    i32 targetUnitIndex,
    i32 targetPxX,
    i32 targetPxY
) {
    Coord cell;
    cell.Set(targetPlayerIndex, targetUnitIndex);
    m_arrivalCell = cell;
    m_arrivalActive = true;
    m_defenderPx.Set(
        (targetPxX & ~TILE_MASK_PX) + TILE_HALF_PX,
        (targetPxY & ~TILE_MASK_PX) + TILE_HALF_PX
    );
    return 1;
}

void CGrunt::ConsiderArrival(i32 clearArrivalState) {
    CWwdSpriteObject* h = m_object;
    Coord tile = LastTilePx();
    i32 tx = tile.m_x;
    i32 ty = tile.m_y;
    DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(h, px, py)
    if (PIXEL_PAIR_NOT_AT_POSITION(px, py, tx, ty)) {
        if (IsDropReady(clearArrivalState)) {
            return;
        }
    }
    SnapToLastTile(clearArrivalState);
}

i32 CGrunt::TryTeleportToCell(i32 tileX, i32 tileY, b32 useSecretColor, b32 spawnWormhole) {
    if (m_entranceCommitted == false) {
        return 1;
    }
    i32 flags = g_gameReg->m_tileGrid->CellFlagsAt(tileX, tileY);
    if ((flags & 0xd39) || (flags & 0x82)) {
        return 0;
    }

    bool eq;
    eq = IsNotAnimationAct("A");
    if (!eq) {
        goto applyTail;
    }
    eq = IsNotAnimationAct("D");
    if (!eq) {
        goto applyTail;
    }
    eq = IsAnimationAct("I");
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->VoiceMgr()->StopVoice(m_object->m_objectId);
        }
        ClearMoveTileFx(this);
        if (m_entranceReason != PICKUP_BOMB) {
            goto applyTail;
        }
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        return 1;
    }
    if (GRUNT_IS_USING_TOY(eq)) {
        goto idleReseed;
    }
    if (SettleActiveKnockback()) {
        goto applyTail;
    }
    eq = IsAnimationAct("Q");
    if (eq) {
        return 1;
    }
    if (APPLY_ACTIVE_ENTRANCE_PICKUP(eq)) {
        goto applyTail;
    }

    eq = GetAnimationActName() == "N";
    if (eq) {
        SettleTubeMove();
        goto applyTail;
    }
    if (TERMINATE_ACTIVE_BOMB_RUN(eq)) {
        return 1;
    }
    goto applyTail;

idleReseed:
    RestoreToolAfterToyUse(1);

applyTail:

    if (m_wingzEnabled != false) {
        LoadWingzGruntSprites(false);
    }
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    m_triggerMgr->ApplySwitch(this, m_object->m_screenX, m_object->m_screenY);
    {
        DECLARE_TILE_CENTER_PIXEL_PAIR(spawnPx, spawnPy, tileX, tileY)
        SET_SCREEN_POS(m_object, spawnPx, spawnPy);
        g_gameReg->m_tileGrid->ReleaseCellOccupancy(
            m_lastTilePx.m_x >> TILE_SHIFT_PX,
            m_lastTilePx.m_y >> TILE_SHIFT_PX
        );
        m_lastTilePx.Set(-1, -1);
        SetEntrancePos(1, 1);
        this->RecycleCoords();
        if (m_arrivalState == AI_BATTLEZ_PATH) {
            m_defenderState = AISTATE_SEEK;
            m_routePassableMask = 0;
        }
        if (spawnWormhole != false) {
            CWwdSpriteObject* spawned = g_gameReg->World()->ChildGroup()->CreateSprite(
                0,
                spawnPx,
                spawnPy,
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
    SerializeClockPair(ar, mode, &m_toyTiming);
    SerializeClockPair(ar, mode, &m_idleDelayTiming);
    SerializeClockPair(ar, mode, &m_idleWindowTiming);
    SerializeClockPair(ar, mode, &m_entranceTiming);
    SerializeClockPair(ar, mode, &m_flashTiming);
    SerializeClockPair(ar, mode, &m_attackTiming);
    SerializeClockPair(ar, mode, &m_combatTiming);
    SerializeClockPair(ar, mode, &m_hudRetireTiming);
    m_wingzTiming.Serialize(ar, mode, typeId, object);
    m_conversionTiming.Serialize(ar, mode, typeId, object);
    m_shimmerTiming.Serialize(ar, mode, typeId, object);
    m_arrivalVoiceTiming.Serialize(ar, mode, typeId, object);
    m_arrivalRerollTiming.Serialize(ar, mode, typeId, object);
    m_holdTiming.Serialize(ar, mode, typeId, object);
    return 1;
}

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
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, m_poseWalk);
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseAttack, GRUNT_ATTACK1));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseAttack, GRUNT_ATTACK2));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, m_poseAttackIdle);
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseStruck, GRUNT_STRUCK1));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseStruck, GRUNT_STRUCK2));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseIdle, GRUNT_IDLE1));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseIdle, GRUNT_IDLE2));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseIdle, GRUNT_IDLE3));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseIdle, GRUNT_IDLE4));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseIdle, GRUNT_IDLE5));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, m_poseDeath);
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseToy, GRUNT_TOY1));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseToy, GRUNT_TOY2));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseToy, GRUNT_TOY_BREAK));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseItem, GRUNT_ITEM1));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, AT(m_poseItem, GRUNT_ITEM2));
    SERIAL_WRITE_ANIMATION(ar, world, nameBuffer, m_pickupGeoSrc);
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
    ar->Write(&m_movePosX, sizeof(m_movePosX));
    ar->Write(&m_movePosY, sizeof(m_movePosY));
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
