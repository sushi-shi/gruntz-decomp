#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>

#include <limits.h>

// @early-stop
RVA(0x00077660, 0x4f0)
void CMapMgr::ComputeCellFlags(i32 x, i32 y, i32 tileId) {

    BrickzCell* cell = &m_rows[y][x];
    CGameLevel* level = m_attrMgr->m_level;

    i32 cx = x;
    if (x < 0) {
        cx = 0;
    } else if (x >= level->m_mainPlane->m_tileColumns) {
        cx = level->m_mainPlane->m_tileColumns - 1;
    }
    i32 cy = y;
    if (y < 0) {
        cy = 0;
    } else if (y >= level->m_mainPlane->m_tileRows) {
        cy = level->m_mainPlane->m_tileRows - 1;
    }
    i32 id = level->m_mainPlane->m_tileHandles[level->m_mainPlane->m_tileRowOffsets[cy] + cx];
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
    i32 keep = oldFlags & 0x1bf40000;

    switch (typeCode) {
        case TILEKIND_SOLID:
            cell->m_flags = 0x1;
            break;
        case TILEKIND_WATER:
            cell->m_flags = 0x100;
            break;
        case TILEKIND_TOGGLEWATERBRIDGE_UP:
            cell->m_flags = 0x300;
            break;
        case TILEKIND_SINK_HAZARD:
            cell->m_flags = 0x800;
            break;
        case TILEKIND_CHECKPOINTPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_WHITEPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_ORANGEPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_BLACKPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_GREENPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_REDPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_PURPLEPYRAMID_DOWN:
            cell->m_flags = 0x4002008;
            break;
        case TILEKIND_GAUNTLET_ROCK_A:
            cell->m_flags = 0x2021;
            break;
        case TILEKIND_GAUNTLET_ROCK_B:
            cell->m_flags = 0x2021;
            break;
        case TILEKIND_GIANT_ROCK:
            cell->m_flags = 0x2021;
            break;
        case TILEKIND_GAUNTLET_BRICK_A:
            cell->m_flags = 0x6021;
            break;
        case TILEKIND_GAUNTLET_BRICK_B:
            cell->m_flags = 0x6021;
            break;
        case TILEKIND_GAUNTLET_BRICK_C:
            cell->m_flags = 0x6021;
            break;
        case TILEKIND_HIDDEN_POWERUP:
            cell->m_flags = IDX(CELL_FLAG_HIDDEN_POWERUP);
            break;
        case TILEKIND_AI_PATH_BLOCKER:
            cell->m_flags = 0x2001;
            break;
        case TILEKIND_WATERBRIDGE_UP:
            cell->m_flags = 0x108;
            break;
        case TILEKIND_DEATHBRIDGE_UP:
            cell->m_flags = 0xa;
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
            cell->m_flags = 0x202;
            break;
        case TILEKIND_SWITCH_A:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SWITCH_A_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SWITCH_B:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SWITCH_B_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_MULTI_SWITCH:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_MULTI_SWITCH_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SWITCH_C:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SWITCH_C_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_EXCLUSIVE_SWITCH:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_EXCLUSIVE_SWITCH_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SECRET_SWITCH:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_SECRET_SWITCH_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_TIME_SWITCH:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_TIME_SWITCH_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_CHECKPOINT:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_CHECKPOINT_UP:
            cell->m_flags = 0x4;
            break;
        case TILEKIND_ARROW_UP_A:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_DOWN_A:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_LEFT_A:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_RIGHT_A:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_UP_B:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_DOWN_B:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_LEFT_B:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_RIGHT_B:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_ARROW_CURRENT:
            cell->m_flags = 0x80;
            break;
        case TILEKIND_SPIKES:
            cell->m_flags = 0x400;
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
            i32 nf = nc->m_flags & ~0x1000;
            nc->m_flags = nf;
            if ((nf & 0x100) == 0) {
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
                nc->m_flags = nf | 0x1000;
            }
        }
    }
}

RVA(0x00077c90, 0x1d)
void CDDrawWorkerHost::SetCell(i32 x, i32 y, i32 id) {
    SET_WORKER_HOST_CELL(this, x, y, id);
}

RVA(0x00077cc0, 0x13d)
CGrunt* CTriggerMgr::FindNearestEnemy(CGrunt* w) {
    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    i32 tileX = w->m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 tileY = w->m_lastTilePx.m_y >> TILE_SHIFT_PX;
    CGrunt** rowPtr = m_units;
    for (i32 i = 0; i < 4; i++) {
        if (i != w->m_playerIndex) {
            CGrunt** colPtr = rowPtr;
            i32 j = 15;
            do {
                CGrunt* cell = *colPtr;
                if (cell && cell->m_entranceCommitted != false
                    && cell->m_gruntKind != GRUNT_GHOST) {
                    i32 dx = (cell->m_object->m_screenX >> TILE_SHIFT_PX) - tileX;
                    i32 dy = (cell->m_object->m_screenY >> TILE_SHIFT_PX) - tileY;
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        best = cell;
                        bestDist = dist;
                    }
                }
                colPtr++;
            } while (--j != 0);
        }
        rowPtr += 15;
    }
    i32 k = w->m_reachRect.right + w->m_defenderRadius + 1;
    i32 px = w->m_object->m_screenX >> TILE_SHIFT_PX;
    i32 py = w->m_object->m_screenY >> TILE_SHIFT_PX;
    RECT rc;
    rc.left = px - k;
    rc.top = py - k;
    rc.right = px + k + 1;
    rc.bottom = py + k + 1;
    if (best) {
        POINT pt;
        pt.x = best->m_object->m_screenX >> TILE_SHIFT_PX;
        pt.y = best->m_object->m_screenY >> TILE_SHIFT_PX;
        if (!PtInRect(&rc, pt)) {
            best = NULL;
        }
    }
    return best;
}
