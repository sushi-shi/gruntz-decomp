#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateRecord.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileCoordMacros.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
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

static inline i32 TileFlags(const char* rec) {

    Pix16CPtr r;
    r.m_chars = rec;
    return *r.m_dwords;
}

static __inline i32 s_CanCommitToyMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    i32 mtx = moveX >> TILE_SHIFT_PX;
    i32 mty = moveY >> TILE_SHIFT_PX;
    i32 arr = g->m_arrivalFlags | 0x20000000;
    if (tx != mtx || ty != mty) {
        if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        i32* tgt = &board->m_rowInts[mty][mtx * 7];
        i32 tflags = *tgt;
        i32 hit = arr & tflags;
        if (hit & 0x20000000) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask | 0x18000482;
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        i32 dx = mtx - tx;
        i32 dy = mty - ty;
        if (dx == 0 || dy == 0) {
            return 1;
        }
        char* cur = board->m_rowBytes[ty] + tx * 7 * 4;

        Pix16Ptr row;
        row.m_dwords = tgt;
        char* tg = row.m_chars;
        if (dx > 0 && dy > 0) {
            if ((cur[0x1d] & 0x20) || (cur[board->m_width * 7 * 4 + 1] & 0x20)
                || (TileFlags(tg - 0x1c) & 0x2000)
                || (TileFlags(tg - board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if ((cur[-0x1b] & 0x20) || (cur[board->m_width * 7 * 4 + 1] & 0x20)
                || (TileFlags(tg + 0x1c) & 0x2000)
                || (TileFlags(tg - board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if ((cur[0x1d] & 0x20) || (TileFlags(cur - board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg - 0x1c) & 0x2000)
                || (TileFlags(tg + board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if ((cur[-0x1b] & 0x20) || (TileFlags(cur - board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg + 0x1c) & 0x2000)
                || (TileFlags(tg + board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        }
    }
    return 1;
}

static __inline i32 s_CanCommitBagMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    i32 mtx = moveX >> TILE_SHIFT_PX;
    i32 mty = moveY >> TILE_SHIFT_PX;
    i32 arr = g->m_arrivalFlags | 0x20000000;
    if (tx != mtx || ty != mty) {
        if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        i32* tgt = &board->m_rowInts[mty][mtx * 7];
        i32 tflags = *tgt;
        i32 hit = arr & tflags;
        if (hit & 0x20000000) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask | 0x18000482;
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        i32 dx = mtx - tx;
        i32 dy = mty - ty;
        if (dx == 0 || dy == 0) {
            return 1;
        }
        char* cur = board->m_rowBytes[ty] + tx * 7 * 4;

        Pix16Ptr row;
        row.m_dwords = tgt;
        char* tg = row.m_chars;
        if (dx > 0 && dy > 0) {
            if ((TileFlags(cur + 0x1c) & 0x2000)
                || (TileFlags(cur + board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg - 0x1c) & 0x2000)
                || (TileFlags(tg - board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if ((TileFlags(cur - 0x1c) & 0x2000)
                || (TileFlags(cur + board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg + 0x1c) & 0x2000)
                || (TileFlags(tg - board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if ((TileFlags(cur + 0x1c) & 0x2000)
                || (TileFlags(cur - board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg - 0x1c) & 0x2000)
                || (TileFlags(tg + board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if ((TileFlags(cur - 0x1c) & 0x2000)
                || (TileFlags(cur - board->m_width * 7 * 4) & 0x2000)
                || (TileFlags(tg + 0x1c) & 0x2000)
                || (TileFlags(tg + board->m_width * 7 * 4) & 0x2000)) {
                return 0;
            }
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
        RECT a;                                                                                    \
        a.left = -1;                                                                               \
        a.top = -1;                                                                                \
        a.right = 1;                                                                               \
        a.bottom = 1;                                                                              \
        m_toyRectA = a;                                                                            \
        a.left = 0;                                                                                \
        a.top = 0;                                                                                 \
        a.right = 0;                                                                               \
        a.bottom = 0;                                                                              \
        m_toyRectB = a;                                                                            \
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

    g_gameReg->m_curState->BuildAssetNamespacePrefixes(name, 1, 1, 0);

    i32 code = g_gameReg->m_tileGrid->m_rowInts[m_lastTilePx.m_y >> TILE_SHIFT_PX]
                                               [(m_lastTilePx.m_x >> TILE_SHIFT_PX) * 7 + 4];
    TileCollisionKind tileKind = static_cast<TileCollisionKind>(code);
    if (tileKind == TILEKIND_CHECKPOINT || tileKind == TILEKIND_CHECKPOINT_UP) {
        if (GRUNT_SELF_AT_SAVED_SCREEN_POS) {

            m_tileMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        }
    }
    return 1;
}
RVA(0x000511b0, 0x246)
void CGrunt::PlayMoveSound(i32 x, i32 y) {
    CWwdGameObjectA* h = m_object;
    i32 dy = y - h->m_screenY;
    i32 dx = x - h->m_screenX;
    i32 cx = h->m_screenX;

    if (dx == 0) {
        if (y > h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirSouth);
        } else if (y < h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirNorth);
        }
        return;
    }

    float ratio = static_cast<float>(dy) / dx;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (y > h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirSouth);
        } else {
            PlaySound(1000, g_gruntMoveDirNorth);
        }
        return;
    }
    if (ratio <= g_slopePosHalf && ratio >= g_slopeNegHalf) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirEast);
        } else {
            PlaySound(1000, g_gruntMoveDirWest);
        }
        return;
    }
    if (ratio > g_slopePosHalf) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirSouthEast);
        } else {
            PlaySound(1000, g_gruntMoveDirNorthWest);
        }
        return;
    }
    if (ratio < g_slopeNegHalf) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirNorthEast);
        } else {
            PlaySound(1000, g_gruntMoveDirSouthWest);
        }
    }
}

RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (m_combatActive == 0 && m_stamina >= STAMINA_FULL && m_entranceActive == 0) {
        return 1;
    }
    return 0;
}

RVA(0x000514e0, 0x1e)
void CGrunt::PlayMoveSoundAtTile(i32 tx, i32 ty) {
    PlayMoveSound(tx * 0x20 + 0x10, ty * 0x20 + 0x10);
}

// @early-stop
RVA(0x00051510, 0x20f)
i32 CGrunt::IsDropReady(i32 a) {
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 x = m_commitPx.m_x >> TILE_SHIFT_PX;
        i32 y = m_commitPx.m_y >> TILE_SHIFT_PX;
        i32 owner;
        if (static_cast<u32>(x) < static_cast<u32>(board->m_width)
            && static_cast<u32>(y) < static_cast<u32>(board->m_height)) {
            owner = board->m_rowInts[y][x * 7 + 1];
        } else {
            owner = -1;
        }
        if (owner != -1) {
            return 0;
        }
    }

    CWwdGameObjectA* object = m_object;
    i32 lastX = m_lastTilePx.m_x;
    if (object->m_screenX == lastX) {
        i32 lastY = m_lastTilePx.m_y;
        if (object->m_screenY == lastY) {
            return 0;
        }
    }

    if (m_coordList.GetCount() != 0) {
        Coord* coord = 0;
        CoordPoolNode* node = g_coordPool.m_freeHead;
        i32 coordX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 coordY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        if (node->m_next != NULL) {
            coord = &node->m_coord;
            coord->m_x = coordX;
            coord->m_y = coordY;
            g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
        }
        m_coordList.AddHead(coord);
    }

    m_object->m_screenX = m_commitPx.m_x;
    m_object->m_screenY = m_commitPx.m_y;
    object = m_object;
    if (object->m_sortKey != object->m_screenY + 0x186a0) {
        object->m_sortKey = object->m_screenY + 0x186a0;
        i32 flags = object->m_flags;
        object->m_flags = flags | 0x20000;
    }

    i32 oldY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 oldX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 newX = m_commitPx.m_x >> TILE_SHIFT_PX;
    i32 newY = m_commitPx.m_y >> TILE_SHIFT_PX;
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        board->m_rows[oldY][oldX].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
        board->m_rows[oldY][oldX].m_occupantId = -1;
    }
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 ownerLo = m_tileOwnerLo;
        i32 ownerHi = m_tileOwnerHi;
        board->m_rows[newY][newX].m_flags |= BRICKZ_CELL_OCCUPIED;
        board->m_rows[newY][newX].m_occupantId = (ownerHi << 8) | ownerLo;
    }

    m_lastTilePx.m_x = m_commitPx.m_x;
    m_lastTilePx.m_y = m_commitPx.m_y;
    m_commitPx.m_x = m_entrancePx.m_x;
    m_commitPx.m_y = m_entrancePx.m_y;
    m_tileMoveCommitted = 1;

    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        m_arrivalPending = 0;
    }
    return 1;
}

RVA(0x000517b0, 0x7d)
void CGrunt::SnapToLastTile(i32 a) {
    m_object->m_screenX = m_lastTilePx.m_x;
    m_object->m_screenY = m_lastTilePx.m_y;
    CWwdGameObjectA* h = m_object;
    SET_SORT_KEY_IF_CHANGED(h, h->m_screenY + 0x186a0)
    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {

        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        m_arrivalPending = 0;
    }
}

// @early-stop
// retail stores m_lastTilePx into r1.left/r1.top before the reach-rect copy
// (two loads + two stores we do not emit); every spelling of a dead field
// store is eliminated by cl, so the source of those four is still unknown.
RVA(0x00051850, 0x165)
i32 CGrunt::RectContains(i32 x, i32 y) {
    x >>= TILE_SHIFT_PX;
    y >>= TILE_SHIFT_PX;
    i32 dx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 dy = m_lastTilePx.m_y >> TILE_SHIFT_PX;

    RECT r1 = m_reachRect;
    RECT r2 = m_reachExclusionRect;
    r1.left += dx;
    r1.top += dy;
    r1.right += dx;
    r1.bottom += dy;
    r1.right++;
    r1.bottom++;
    r2.left += dx;
    r2.top += dy;
    r2.right += dx;
    r2.bottom += dy;

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {

            if (CGameLevel::PointInRect(&r1, x, y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }

    if (CGameLevel::PointInRect(&r1, x, y)) {

        if (x >= r2.right || x < r2.left || y >= r2.bottom || y < r2.top) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00051a20, 0x17d)
i32 CGrunt::RectContainsGated(i32 x, i32 y) {
    x >>= TILE_SHIFT_PX;
    y >>= TILE_SHIFT_PX;
    i32 dx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 dy = m_lastTilePx.m_y >> TILE_SHIFT_PX;

    RECT r1 = m_toyRectA;
    RECT r2 = m_toyRectB;
    r1.left += dx;
    r1.top += dy;
    r1.right += dx;
    r1.bottom += dy;
    r1.right++;
    r1.bottom++;
    r2.left += dx;
    r2.top += dy;
    r2.right += dx;
    r2.bottom += dy;

    if (m_vehiclePickupType == PICKUP_NONE) {
        return 0;
    }

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            if (CGameLevel::PointInRect(&r1, x, y)) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (CGameLevel::PointInRect(&r1, x, y)) {

        if (x >= r2.right || x < r2.left || y >= r2.bottom || y < r2.top) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
// The fixed-arrow and current-direction switches repeat the cardinal cases;
// cl folds their identical bodies so both jump tables target one set of loads.
// The toy probes narrow flags to bytes while the fallback keeps dwords.
RVA(0x00051c00, 0xd20)
i32 CGrunt::StepCompassMove() {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 x = m_lastTilePx.m_x;
    i32 y = m_lastTilePx.m_y;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;
    i32 result = 0;
    i32 moveX;
    i32 moveY;
    GruntDirectionCell voice;

    if (board->CellFlagsAt(tx, ty) & 0x80) {

        i32 cmd = board->m_rowInts[ty][tx * 7 + 4];
        switch (static_cast<TileCollisionKind>(cmd)) {
            case TILEKIND_ARROW_UP_A:
            case TILEKIND_ARROW_UP_B:
                y -= 0x20;
                moveY = y;
                moveX = x;
                voice = g_gruntMoveDirNorth;
                break;
            case TILEKIND_ARROW_RIGHT_A:
            case TILEKIND_ARROW_RIGHT_B:
                x += 0x20;
                moveX = x;
                moveY = y;
                voice = g_gruntMoveDirEast;
                break;
            case TILEKIND_ARROW_DOWN_A:
            case TILEKIND_ARROW_DOWN_B:
                y += 0x20;
                moveX = x;
                moveY = y;
                voice = g_gruntMoveDirSouth;
                break;
            case TILEKIND_ARROW_LEFT_A:
            case TILEKIND_ARROW_LEFT_B:
                x -= 0x20;
                moveX = x;
                moveY = y;
                voice = g_gruntMoveDirWest;
                break;
            case TILEKIND_ARROW_CURRENT:
                switch (m_entranceCell.direction) {
                    case DIR_NORTH:
                        y -= 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirNorth;
                        break;
                    case DIR_EAST:
                        x += 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirEast;
                        break;
                    case DIR_SOUTH:
                        y += 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirSouth;
                        break;
                    case DIR_WEST:
                        x -= 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirWest;
                        break;
                    case DIR_NORTHEAST:
                        x += 0x20;
                        y -= 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirNorthEast;
                        break;
                    case DIR_SOUTHEAST:
                        x += 0x20;
                        y += 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirSouthEast;
                        break;
                    case DIR_SOUTHWEST:
                        x -= 0x20;
                        y += 0x20;
                        moveX = x;
                        moveY = y;
                        voice = g_gruntMoveDirSouthWest;
                        break;
                    case DIR_NORTHWEST:
                        x -= 0x20;
                        y -= 0x20;
                        moveX = x;
                        voice = g_gruntMoveDirNorthWest;
                        moveY = y;
                        break;
                    default:
                        moveX = x;
                        moveY = y;
                        break;
                }
                break;
            default:
                moveX = x;
                moveY = y;
                break;
        }
        i32 mtx = moveX >> TILE_SHIFT_PX;
        i32 mty = moveY >> TILE_SHIFT_PX;
        i32 tflags = board->CellFlagsAt(mtx, mty);
        if ((tflags & 0x20000000) && !(tflags & 0x80)) {

            i32 owner;
            if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
                || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
                owner = -1;
            } else {
                owner = board->m_rowInts[mty][mtx * 7 + 1];
            }
            m_tileMgr->CellDispatch((owner >> 8) & 0xff, owner & 0xff, DEATH_SQUASH, m_tileOwnerHi);
        }
        goto commit;
    }

    if (m_toyTileIndex > 0) {
        CString str;
        // The bias is LOAD-BEARING: switching on the domain directly and using
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
            g_buteMgr.GetDwordDef(const_cast<char*>(static_cast<LPCTSTR>(str)), s_ToyTiles, 1);
        if (m_toyTileIndex < toyCount) {
            switch (m_entranceCell.direction) {
                case DIR_NORTH:
                    moveX = x;
                    voice = g_gruntMoveDirNorth;
                    moveY = y - 0x20;
                    break;
                case DIR_NORTHEAST:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    moveX = x + 0x20;
                    moveY = y;
                    voice = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    moveX = x;
                    voice = g_gruntMoveDirSouth;
                    moveY = y + 0x20;
                    break;
                case DIR_SOUTHWEST:
                    moveX = x - 0x20;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouthWest;
                    break;
                case DIR_WEST:
                    moveX = x - 0x20;
                    moveY = y;
                    voice = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorthWest;
                    break;
                default:
                    moveX = x;
                    moveY = y;
                    break;
            }
            result = s_CanCommitToyMove(this, moveX, moveY, x, y);
            if (result == 0) {
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
            // retail keeps the degenerate empty-bag arm: cl cannot prove
            // count != 0 here, so both rand() calls survive
            i32 last = bag.GetUpperBound();
            i32 count = last + 1;
            i32 idx;
            if (count == 0) {
                idx = (rand() & 1) != 0 ? 0 : last;
            } else {
                idx = rand() % count;
            }
            i32 dir = bag.GetAt(idx);
            moveX = x;
            moveY = y;
            switch (static_cast<GruntDirection>(dir)) {
                case DIR_NORTH:
                    moveX = x;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorth;
                    break;
                case DIR_NORTHEAST:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorthEast;
                    break;
                case DIR_EAST:
                    moveX = x + 0x20;
                    moveY = y;
                    voice = g_gruntMoveDirEast;
                    break;
                case DIR_SOUTHEAST:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouthEast;
                    break;
                case DIR_SOUTH:
                    moveX = x;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouth;
                    break;
                case DIR_SOUTHWEST:
                    moveX = x - 0x20;
                    voice = g_gruntMoveDirSouthWest;
                    moveY = y + 0x20;
                    break;
                case DIR_WEST:
                    moveX = x - 0x20;
                    moveY = y;
                    voice = g_gruntMoveDirWest;
                    break;
                case DIR_NORTHWEST:
                    voice = g_gruntMoveDirNorthWest;
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    break;
            }
            result = s_CanCommitBagMove(this, moveX, moveY, x, y);
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
    m_tileMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    PlaySound(0x3e8, voice);
    m_commitPx = m_lastTilePx;
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        i32 ox = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        b->m_rowBytes[oy][ox * 7 * 4 + 3] &= 0xdf;
        b->m_rowInts[oy][ox * 7 + 1] = -1;
    }
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        i32 nx = moveX >> TILE_SHIFT_PX;
        i32 ny = moveY >> TILE_SHIFT_PX;
        i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        b->m_rowBytes[ny][nx * 7 * 4 + 3] |= 0x20;
        b->m_rowInts[ny][nx * 7 + 1] = owner;
    }
    m_lastTilePx.m_x = moveX;
    m_lastTilePx.m_y = moveY;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    m_toyTileIndex += 1;
    return 1;
}

// @early-stop
RVA(0x00052c70, 0x1e0)
i32 CGrunt::ClaimSwitchTile() {
    Coord tile = m_lastTilePx;
    Coord next;
    switch (m_entranceCell.direction) {
        case DIR_NORTHEAST:
            next.m_x = tile.m_x + 0x20;
            next.m_y = tile.m_y - 0x20;
            break;
        case DIR_EAST:
            next.m_x = tile.m_x + 0x20;
            next.m_y = tile.m_y;
            break;
        case DIR_SOUTHEAST:
            next.m_x = tile.m_x + 0x20;
            next.m_y = tile.m_y + 0x20;
            break;
        case DIR_SOUTHWEST:
            next.m_x = tile.m_x - 0x20;
            // fall through
        case DIR_SOUTH:
            next.m_y = tile.m_y + 0x20;
            break;
        case DIR_WEST:
            next.m_x = tile.m_x - 0x20;
            next.m_y = tile.m_y;
            break;
        case DIR_NORTHWEST:
            next.m_x = tile.m_x - 0x20;
            // fall through
        case DIR_NORTH:
            next.m_y = tile.m_y - 0x20;
            break;
    }

    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    i32 tx = next.m_x >> TILE_SHIFT_PX;
    i32 ty = next.m_y >> TILE_SHIFT_PX;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        flags = 1;
    } else {
        flags = b->m_rowInts[ty][tx * 7];
    }
    if ((flags & 0x20000939) || (flags & 0x80)) {
        return 0;
    }

    m_tileMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);

    m_commitPx = m_lastTilePx;
    CGruntzMapMgr* gb = g_gameReg->m_tileGrid;
    i32 oldTx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 oldTy = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    gb->m_rows[oldTy][oldTx].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
    gb->m_rows[oldTy][oldTx].m_occupantId = -1;

    CGruntzMapMgr* nb = g_gameReg->m_tileGrid;
    i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
    nb->m_rowBytes[ty][tx * 7 * 4 + 3] |= 0x20;
    nb->m_rowInts[ty][tx * 7 + 1] = owner;

    m_lastTilePx = next;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    return 1;
}

RVA(0x00052ed0, 0x42)
i32 CGrunt::SetArrivalTarget(i32 a, i32 b, i32 c, i32 d) {
    Coord cell;
    cell.m_x = a;
    cell.m_y = b;
    m_arrivalCell = cell;
    m_arrivalActive = 1;
    m_defenderPx.m_x = (c & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_defenderPx.m_y = (d & ~TILE_MASK_PX) + TILE_HALF_PX;
    return 1;
}

// @early-stop
RVA(0x00052f40, 0x4b)
void CGrunt::ConsiderArrival(i32 a) {
    CWwdGameObjectA* h = m_object;
    Coord tile = m_lastTilePx;
    i32 tx = tile.m_x;
    i32 ty = tile.m_y;
    DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(h, px, py)
    if (PIXEL_PAIR_NOT_AT_POSITION(px, py, tx, ty)) {
        if (IsDropReady(a)) {
            return;
        }
    }
    SnapToLastTile(a);
}

// @early-stop
RVA(0x00052fb0, 0x96e)
i32 CGrunt::TryTeleportToCell(i32 tileX, i32 tileY, i32 useSecretColor, i32 spawnWormhole) {
    if (m_entranceCommitted == 0) {
        return 1;
    }
    i32 flags = g_gameReg->m_tileGrid->CellFlagsAt(tileX, tileY);
    if ((flags & 0xd39) || (flags & 0x82)) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "A") != 0);
    if (!eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "D") != 0);
    if (!eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eq) {

        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
        if (m_entranceReason != PICKUP_BOMB) {
            goto applyTail;
        }
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "G") == 0);
    if (!eq) {
        eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "L") == 0);
        if (!eq) {
            eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "P") == 0);
            if (!eq) {
                eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "O") == 0);
                if (eq) {

                    SnapToLastTile(1);
                    m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                    goto applyTail;
                }
                eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "Q") == 0);
                if (eq) {
                    return 1;
                }
                eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "J") == 0);
                if (eq) {

                    m_entranceActive = 0;
                    eq = (strcmp(*g_typeColl.GetNameRecord(m_prevAnimSetNode), "D") == 0);
                    if (eq) {
                        if (m_poweredUp != 0 && m_neighborValid == 0) {
                            RESET_GRUNT_POWERED_STATE
                        }
                        m_tileMoveCommitted = 0;
                        m_prevAnimSetNode = m_objAux->m_actKey;
                        m_objAux->m_actKey = ActFindId("D");
                        SwitchAnimation(m_poseWalk);

                        GruntDirectionCell cell = m_entranceCell;
                        i32 col = cell.column + cell.row * 2;
                        i32 base = cell.row + col;
                        char* nm = m_cells[base].WalkName().GetBuffer(0);
                        m_wwdObject->ApplyName(nm);
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
                    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                    ActNameConstructGrownSlots();
                    eq = (strcmp(*rec, "N") == 0);
                }
                if (eq) {

                    CWwdGameObjectA* h = m_object;
                    i32 savedX = m_lastTilePx.m_x;
                    i32 savedY = m_lastTilePx.m_y;
                    DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(h, px, py)
                    i32 redo = 1;
                    if (PIXEL_PAIR_NOT_AT_POSITION(px, py, savedX, savedY)) {
                        if (IsDropReady(1)) {
                            m_coordToggle = (m_coordToggle == 0);
                            redo = 0;
                        }
                    }
                    SnapToLastTile(1);
                    if (redo == 0) {
                        goto applyTail;
                    }
                    m_prevAnimSetNode = m_objAux->m_actKey;
                    m_objAux->m_actKey = ActFindId("D");
                    SetupTubeAnim(m_coordToggle);
                    goto applyTail;
                }
                {
                    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                    ActNameConstructGrownSlots();
                    eq = (strcmp(*rec, "M") == 0);
                }
                if (eq) {
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
                    return 1;
                }
                goto applyTail;
            }
        }
    }

idleReseed:

    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 z = m_object->m_screenY + 0x186a0;
        CWwdGameObjectA* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, z)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopStruckSlotSound();

applyTail:

    if (m_wingzEnabled != 0) {
        LoadWingzGruntSprites(0);
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        RESET_GRUNT_POWERED_STATE
    }
    m_tileMgr->ApplySwitch(this, m_object->m_screenX, m_object->m_screenY);
    {
        DECLARE_TILE_CENTER_PIXEL_PAIR(spawnPx, spawnPy, tileX, tileY)
        m_object->m_screenX = spawnPx;
        m_object->m_screenY = spawnPy;
        {
            CGruntzMapMgr* board = g_gameReg->m_tileGrid;
            i32 gx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
            i32 gy = m_lastTilePx.m_y >> TILE_SHIFT_PX;
            board->m_rowInts[gy][gx * 7] &= ~0x20000000;
            board->m_rowInts[gy][gx * 7 + 1] = -1;
            m_lastTilePx.m_x = -1;
            m_lastTilePx.m_y = -1;
        }
        SetEntrancePos(1, 1);
        if (CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS_INLINE_POOL(this)
        }
        if (m_arrivalState == AI_BATTLEZ_PATH) {
            m_defenderState = AISTATE_SEEK;
            m_routeMaskC = 0;
        }
        if (spawnWormhole != 0) {
            CWwdGameObjectA* spawned =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, spawnPx, spawnPy, 0, "Wormhole", 0x40003);
            if (spawned != NULL) {
                if (useSecretColor != 0) {
                    spawned->m_smarts = g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1);
                } else {
                    spawned->m_smarts = g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3);
                }
            }
        }
    }
    BuildEntranceAnimation(GRUNT_ENTRANCE_WORMHOLE);
    return 1;
}

RVA(0x00053b80, 0x340)
i32 CGrunt::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (ar == NULL) {
        return 0;
    }

    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }

    if (CWapX::Chain(ar, mode, typeId, pObj) == 0) {
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
            m_tileMgr = g_gameReg->m_cmdGrid;
            break;
    }
    m_entranceCell.Serialize(ar, mode, typeId, pObj);
    SerializeClockPair(ar, mode, &m_toyClock);
    SerializeClockPair(ar, mode, &m_idleAnchor);
    SerializeClockPair(ar, mode, &m_idleTimer);
    SerializeClockPair(ar, mode, &m_entranceClock64);
    SerializeClockPair(ar, mode, &m_flashClock64);
    SerializeClockPair(ar, mode, &m_attackClock64);
    SerializeClockPair(ar, mode, &m_combatClock64);
    SerializeClockPair(ar, mode, &m_hudRetireClock64);
    m_wingzTiming.Serialize(ar, mode, typeId, pObj);
    m_conversionTiming.Serialize(ar, mode, typeId, pObj);
    m_shimmerTiming.Serialize(ar, mode, typeId, pObj);
    m_arrivalVoiceTiming.Serialize(ar, mode, typeId, pObj);
    m_arrivalRerollTiming.Serialize(ar, mode, typeId, pObj);
    m_holdTiming.Serialize(ar, mode, typeId, pObj);
    return 1;
}

RVA(0x00053f90, 0x11d0)
i32 CGrunt::Save(CFileMemBase* ar) {
    if (!ar) {
        return 0;
    }

    CDDrawSurfaceMgr* mgr = m_animWorker->m_ownerCtx;
    if (!mgr) {
        return 0;
    }
    i32 n;
    char buf[SERIAL_NAME_LEN];
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_selectedSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_toySprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_healthSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_staminaSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_toyTimeSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_wingzTimeSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_powerupSprite;
        if (sp) {
            tmp = sp->m_objectId;
        }
        ar->Write(&tmp, sizeof(tmp));
    }
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    strcpy(buf, static_cast<const char*>(m_animSetName));
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    strcpy(buf, m_frameSetName);
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    strcpy(buf, m_deathFrameSetName);
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = m_poseWalk;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseAttack, GRUNT_ATTACK1);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseAttack, GRUNT_ATTACK2);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = m_poseAttackIdle;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseStruck, GRUNT_STRUCK1);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseStruck, GRUNT_STRUCK2);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseIdle, GRUNT_IDLE1);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseIdle, GRUNT_IDLE2);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseIdle, GRUNT_IDLE3);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseIdle, GRUNT_IDLE4);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseIdle, GRUNT_IDLE5);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = m_poseDeath;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseToy, GRUNT_TOY1);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseToy, GRUNT_TOY2);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseToy, GRUNT_TOY_BREAK);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseItem, GRUNT_ITEM1);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = AT(m_poseItem, GRUNT_ITEM2);
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    g_serialCounter++;
    memset(buf, 0, SERIAL_NAME_LEN);
    {
        CAniElement* id = m_pickupGeoSrc;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(id));
        }
    }
    ar->Write(buf, SERIAL_NAME_LEN);
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
    ar->Write(&m_tileOwnerHi, sizeof(m_tileOwnerHi));
    ar->Write(&m_tileOwnerLo, sizeof(m_tileOwnerLo));
    ar->Write(&m_moveIcon, sizeof(m_moveIcon));
    ar->Write(&m_savedMoveIcon, sizeof(m_savedMoveIcon));
    ar->Write(&m_entranceCommitted, sizeof(m_entranceCommitted));
    ar->Write(&m_neighborCell, sizeof(m_neighborCell));
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
    ar->Write(&m_toyRectA, sizeof(m_toyRectA));
    ar->Write(&m_toyRectB, sizeof(m_toyRectB));
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
    ar->Write(&m_killerSlot, sizeof(m_killerSlot));
    ar->Write(&m_tileClaimed, sizeof(m_tileClaimed));
    ar->Write(&m_deathAnimStarted, sizeof(m_deathAnimStarted));
    ar->Write(&m_pendingTriggerPx, sizeof(m_pendingTriggerPx));
    ar->Write(&m_routeMaskA, sizeof(m_routeMaskA));
    ar->Write(&m_routeMaskC, sizeof(m_routeMaskC));
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
        n = m_coordList.GetCount();
        ar->Write(&n, sizeof(n));
        POSITION cpos = m_coordList.GetHeadPosition();
        while (cpos != NULL) {
            ar->Write(m_coordList.GetNext(cpos), 8);
        }
    }
    {
        n = m_payloads.GetCount();
        ar->Write(&n, sizeof(n));
        POSITION pos = m_payloads.GetHeadPosition();
        while (pos != NULL) {
            ar->Write(m_payloads.GetNext(pos), 0x2c);
        }
    }
    return 1;
}
