#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>
#include <Lith/BDefs.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>

#include <limits.h>

// @early-stop
RVA(0x00077790, 0x4f0)
void CMapMgr::ComputeCellFlags(i32 x, i32 y, i32 tileId) {

    BrickzCell* cell = &m_rows[y][x];
    CGameLevel* level = m_attrMgr->m_level;

    Coord position(x, y);
    Coord clamped = position;
    clamped.Max(Coord(0, 0));
    clamped.Min(
        Coord(level->m_mainPlane->m_tileGridSize.cx - 1, level->m_mainPlane->m_tileGridSize.cy - 1)
    );
    i32 id = level->m_mainPlane
                 ->m_tileHandles[level->m_mainPlane->m_tileRowOffsets[clamped.m_y] + clamped.m_x];
    TileCollisionKind typeCode;
    if (id == UNINIT_FILL || id == -1) {
        typeCode = TILEKIND_PASSABLE;
    } else {
        typeCode = (static_cast<CTileImageSet*>(
                        level->m_imageSets.GetAt(id & WWD_TILE_IMAGE_SET_INDEX_MASK)
                    ))
                       ->GetCollisionAt(0, 0);
    }
    i32 oldFlags = cell->m_flags;
    i32 edgeBit = oldFlags & BRICKZ_CELL_OCCUPIED;
    i32 keep = oldFlags & CELL_FLAGS_PRESERVED_ON_TERRAIN_UPDATE;

    switch (typeCode) {
        case TILEKIND_SOLID:
            cell->m_flags = IDX(CELL_FLAG_SOLID);
            break;
        case TILEKIND_WATER:
            cell->m_flags = IDX(CELL_FLAG_WATER);
            break;
        case TILEKIND_TOGGLEWATERBRIDGE_UP:
            cell->m_flags = IDX(CELL_FLAG_WATER | CELL_FLAG_TOGGLE_BRIDGE);
            break;
        case TILEKIND_SINK_HAZARD:
            cell->m_flags = IDX(CELL_FLAG_SINK_HAZARD);
            break;
        case TILEKIND_CHECKPOINTPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_WHITEPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_ORANGEPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_BLACKPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_GREENPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_REDPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_PURPLEPYRAMID_DOWN:
            cell->m_flags =
                IDX(CELL_FLAG_BRIDGE | CELL_FLAG_PATH_BLOCKER | CELL_FLAG_LOWERED_PYRAMID);
            break;
        case TILEKIND_GAUNTLET_ROCK_A:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER);
            break;
        case TILEKIND_GAUNTLET_ROCK_B:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER);
            break;
        case TILEKIND_GIANT_ROCK:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER);
            break;
        case TILEKIND_GAUNTLET_BRICK_A:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER
                    | CELL_FLAG_GAUNTLET_BRICK);
            break;
        case TILEKIND_GAUNTLET_BRICK_B:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER
                    | CELL_FLAG_GAUNTLET_BRICK);
            break;
        case TILEKIND_GAUNTLET_BRICK_C:
            cell->m_flags =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_PATH_BLOCKER
                    | CELL_FLAG_GAUNTLET_BRICK);
            break;
        case TILEKIND_HIDDEN_POWERUP:
            cell->m_flags = IDX(CELL_FLAG_HIDDEN_POWERUP);
            break;
        case TILEKIND_AI_PATH_BLOCKER:
            cell->m_flags = IDX(CELL_FLAG_SOLID | CELL_FLAG_PATH_BLOCKER);
            break;
        case TILEKIND_WATERBRIDGE_UP:
            cell->m_flags = IDX(CELL_FLAG_BRIDGE | CELL_FLAG_WATER);
            break;
        case TILEKIND_DEATHBRIDGE_UP:
            cell->m_flags = IDX(CELL_FLAG_SPECIAL | CELL_FLAG_BRIDGE);
            break;
        case TILEKIND_DEATH:
            cell->m_flags = IDX(CELL_FLAG_SPECIAL);
            break;
        case TILEKIND_REVEALED_POWERUP:
            cell->m_flags = IDX(CELL_FLAG_REVEALED_POWERUP | CELL_FLAG_SPECIAL);
            break;
        case TILEKIND_COVERED_POWERUP:
            cell->m_flags = IDX(CELL_FLAG_COVERED_POWERUP);
            break;
        case TILEKIND_TOGGLEDEATHBRIDGE_UP:
            cell->m_flags = IDX(CELL_FLAG_SPECIAL | CELL_FLAG_TOGGLE_BRIDGE);
            break;
        case TILEKIND_SWITCH_A:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SWITCH_A_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SWITCH_B:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SWITCH_B_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_MULTI_SWITCH:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_MULTI_SWITCH_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SWITCH_C:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SWITCH_C_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_EXCLUSIVE_SWITCH:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_EXCLUSIVE_SWITCH_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SECRET_SWITCH:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_SECRET_SWITCH_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_TIME_SWITCH:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_TIME_SWITCH_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_CHECKPOINT:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_CHECKPOINT_UP:
            cell->m_flags = IDX(CELL_FLAG_TRIGGER);
            break;
        case TILEKIND_ARROW_UP_A:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_DOWN_A:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_LEFT_A:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_RIGHT_A:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_UP_B:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_DOWN_B:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_LEFT_B:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_RIGHT_B:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_ARROW_CURRENT:
            cell->m_flags = IDX(CELL_FLAG_ARROW);
            break;
        case TILEKIND_SPIKES:
            cell->m_flags = IDX(CELL_FLAG_SPIKES);
            break;
        default:
            cell->m_flags = (tileId == -1) ? IDX(CELL_FLAG_SPECIAL) : 0;
            break;
    }
    if (edgeBit != 0) {
        cell->m_flags |= BRICKZ_CELL_OCCUPIED;
    }
    cell->m_flags |= keep;
    cell->m_tileId = tileId;
    cell->m_typeCode = typeCode;

    for (i32 c = x - 1; c <= x + 1; c++) {
        for (i32 r = y - 1; r <= y + 1; r++) {
            if (r < 0 || static_cast<u32>(r) >= m_height) {
                continue;
            }
            if (c <= 0 || static_cast<u32>(c) >= m_width) {
                continue;
            }
            BrickzCell* nc = &m_rows[r][c];
            i32 nf = nc->m_flags & ~IDX(CELL_FLAG_WATER_DIAGONAL_PASSAGE);
            nc->m_flags = nf;
            if ((nf & IDX(CELL_FLAG_WATER)) == 0) {
                continue;
            }
            BrickzCell* up = NULL;
            BrickzCell* down = NULL;
            BrickzCell* right = NULL;
            BrickzCell* left = NULL;
            BrickzCell* ur = NULL;
            BrickzCell* ul = NULL;
            BrickzCell* dr = NULL;
            BrickzCell* dl = NULL;
            if (r > 0) {
                up = nc - m_width;
            }
            if (static_cast<u32>(r) < m_height - 1) {
                down = nc + m_width;
            }
            if (static_cast<u32>(c) < m_width - 1) {
                right = nc + 1;
            }
            if (c > 0) {
                left = nc - 1;
            }
            if (up && right) {
                ur = up + 1;
            }
            if (up && left) {
                ul = up - 1;
            }
            if (down && right) {
                dr = down + 1;
            }
            if (down && left) {
                dl = down - 1;
            }
            if (up && down && !(up->m_flags & BRICKZ_BLOCKED_MASK)
                && !(down->m_flags & BRICKZ_BLOCKED_MASK)) {
                goto setbit;
            }
            if (right && left && !(right->m_flags & BRICKZ_BLOCKED_MASK)
                && !(left->m_flags & BRICKZ_BLOCKED_MASK)) {
                goto setbit;
            }
            if (ur && dl && !(ur->m_flags & BRICKZ_BLOCKED_MASK)
                && !(dl->m_flags & BRICKZ_BLOCKED_MASK)) {
                goto setbit;
            }
            if (ul && dr && !(ul->m_flags & BRICKZ_BLOCKED_MASK)
                && !(dr->m_flags & BRICKZ_BLOCKED_MASK)) {
            setbit:
                nc->m_flags = nf | IDX(CELL_FLAG_WATER_DIAGONAL_PASSAGE);
            }
        }
    }
}

RVA(0x00077dc0, 0x1d)
void CDDrawWorkerHost::SetCell(i32 x, i32 y, i32 id) {
    SET_WORKER_HOST_CELL(this, x, y, id);
}

static inline RECT TileNeighborhood(CGrunt* grunt) {
    i32 halfBox = grunt->m_defenderRadius + grunt->m_reachRect.right + 1;
    Coord tile;
    grunt->GetScreenTile(&tile);
    RECT box;
    SetRect(
        &box,
        tile.m_x - halfBox,
        tile.m_y - halfBox,
        tile.m_x + halfBox + 1,
        tile.m_y + halfBox + 1
    );
    return box;
}

RVA(0x00077df0, 0x13d)
CGrunt* CTriggerMgr::FindNearestEnemy(CGrunt* w) {
    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    Coord lastTilePx = w->LastTilePx();
    ScreenTile(&lastTilePx);
    i32 i = 0;
    CGrunt** rowPtr = m_units;
    for (; i < TM_PLAYER_COUNT; i++, rowPtr += TM_UNITS_PER_PLAYER) {
        if (i != w->m_playerIndex) {
            CGrunt** colPtr = rowPtr;
            i32 j = TM_UNITS_PER_PLAYER;
            do {
                CGrunt* cell = *colPtr;
                if (cell && cell->m_entranceCommitted != false
                    && cell->m_gruntKind != GRUNT_GHOST) {
                    Coord cellTile;
                    cell->GetScreenTile(&cellTile);
                    i32 dist = cellTile.DistSqr(lastTilePx);
                    if (dist < bestDist) {
                        best = cell;
                        bestDist = dist;
                    }
                }
                colPtr++;
            } while (--j != 0);
        }
    }
    RECT rc = TileNeighborhood(w);
    if (best) {
        Coord bestPos;
        best->GetScreenTile(&bestPos);
        if (!::PtInRect(&rc, bestPos.m_x, bestPos.m_y)) {
            best = NULL;
        }
    }
    return best;
}
