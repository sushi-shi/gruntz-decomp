#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BridgeTileId.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>

static __inline BrickTileId PickA(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return BRICKTILE_BROWN_1;
    }
    if (roll <= t2) {
        return BRICKTILE_RED_1;
    }
    if (roll <= t3) {
        return BRICKTILE_BLUE_1;
    }
    if (roll <= t4) {
        return BRICKTILE_GOLD_1;
    }
    return BRICKTILE_BLACK_1;
}
static __inline BrickTileId PickB(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return BRICKTILE_BROWN_2;
    }
    if (roll <= t2) {
        return (rand() % 100 + 1 <= 0x32) ? BRICKTILE_RED_2_TOP : BRICKTILE_RED_2_LOW;
    }
    if (roll <= t3) {
        return (rand() % 100 + 1 <= 0x32) ? BRICKTILE_BLUE_2_TOP : BRICKTILE_BLUE_2_LOW;
    }
    if (roll <= t4) {
        return (rand() % 100 + 1 <= 0x32) ? BRICKTILE_GOLD_2_TOP : BRICKTILE_GOLD_2_LOW;
    }
    return (rand() % 100 + 1 <= 0x32) ? BRICKTILE_BLACK_2_TOP : BRICKTILE_BLACK_2_LOW;
}
static __inline BrickTileId PickC(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return BRICKTILE_BROWN_3;
    }
    if (roll <= t2) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return BRICKTILE_RED_3_LOW;
        }
        return (r > 0x190) ? BRICKTILE_RED_3_TOP : BRICKTILE_RED_3_MID;
    }
    if (roll <= t3) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return BRICKTILE_BLUE_3_LOW;
        }
        return (r > 0x190) ? BRICKTILE_BLUE_3_TOP : BRICKTILE_BLUE_3_MID;
    }
    if (roll <= t4) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return BRICKTILE_GOLD_3_LOW;
        }
        return (r > 0x190) ? BRICKTILE_GOLD_3_TOP : BRICKTILE_GOLD_3_MID;
    }
    i32 r = rand() % 0x258 + 1;
    if (r <= 0xc8) {
        return BRICKTILE_BLACK_3_LOW;
    }
    return (r > 0x190) ? BRICKTILE_BLACK_3_TOP : BRICKTILE_BLACK_3_MID;
}

// @early-stop
// Calls and all 122 relocations agree; the residue is branch/join layout in the tile
// switch. The exit-trigger neighbour walk is settled: retail's `cdq / and edx,0x1f /
// add` is cl's own `/ TILE_SIZE_PX`, and `cx` is a real outer-loop variable - retail
// homes the x offset at esp+0x58 precisely because its register carries `tileX + xo`.
RVA(0x000810f0, 0xa80)
i32 CGruntzMapMgr::LoadAttributes(i32 width, i32 height) {
    m_attrMgr = g_gameReg->m_world;
    CDDrawWorkerHost* grid = m_attrMgr->m_level->m_mainPlane;
    if (grid == NULL) {
        return 0;
    }
    AllocGrid(width, height, NULL);
    m_reserved90 = 0;

    i32 total = g_buteMgr.GetInt("Brickz", "Brown");
    i32 t1 = total;
    total += g_buteMgr.GetInt("Brickz", "Red");
    i32 t2 = total;
    total += g_buteMgr.GetInt("Brickz", "Blue");
    i32 t3 = total;
    total += g_buteMgr.GetInt("Brickz", "Gold");
    i32 t4 = total;
    total += g_buteMgr.GetInt("Brickz", "Black");

    BrickzCell* cell = m_cellPool;
    for (u32 col = 0; col < m_height; col++) {
        for (u32 row = 0; row < m_width; row++, cell++) {
            i32 tileId = grid->m_tileGrid[grid->m_colOffsets[col] + row];
            if (tileId != -1) {
                tileId &= 0xffff;
            }
            cell->m_flags = 0;
            cell->m_occupantId = -1;
            cell->m_objectId = 0;
            cell->m_tileId = -1;

            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                BridgeTileId bridgeTile = static_cast<BridgeTileId>(tileId);
                if (bridgeTile == BRIDGETILE_WATER_UP_ALT) {
                    tileId = IDX(BRIDGETILE_WATER_UP);
                    SET_WORKER_HOST_CELL(grid, row, col, IDX(BRIDGETILE_WATER_UP));
                } else if (bridgeTile == BRIDGETILE_DEATH_UP_ALT) {
                    tileId = IDX(BRIDGETILE_DEATH_UP);
                    SET_WORKER_HOST_CELL(grid, row, col, IDX(BRIDGETILE_DEATH_UP));
                }
            }

            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
                switch (static_cast<BrickTileId>(tileId)) {
                    case BRICKTILE_BROWN_1:
                    case BRICKTILE_RED_1:
                    case BRICKTILE_BLUE_1:
                    case BRICKTILE_GOLD_1:
                    case BRICKTILE_BLACK_1:
                        tileId = IDX(PickA(total, t1, t2, t3, t4));
                        break;
                    case BRICKTILE_BROWN_2:
                    case BRICKTILE_RED_2_LOW:
                    case BRICKTILE_RED_2_TOP:
                    case BRICKTILE_BLUE_2_LOW:
                    case BRICKTILE_BLUE_2_TOP:
                    case BRICKTILE_GOLD_2_LOW:
                    case BRICKTILE_GOLD_2_TOP:
                    case BRICKTILE_BLACK_2_LOW:
                    case BRICKTILE_BLACK_2_TOP:
                        tileId = IDX(PickB(total, t1, t2, t3, t4));
                        break;
                    case BRICKTILE_BROWN_3:
                    case BRICKTILE_RED_3_LOW:
                    case BRICKTILE_RED_3_MID:
                    case BRICKTILE_RED_3_TOP:
                    case BRICKTILE_BLUE_3_LOW:
                    case BRICKTILE_BLUE_3_MID:
                    case BRICKTILE_BLUE_3_TOP:
                    case BRICKTILE_GOLD_3_LOW:
                    case BRICKTILE_GOLD_3_MID:
                    case BRICKTILE_GOLD_3_TOP:
                    case BRICKTILE_BLACK_3_LOW:
                    case BRICKTILE_BLACK_3_MID:
                    case BRICKTILE_BLACK_3_TOP:
                        tileId = IDX(PickC(total, t1, t2, t3, t4));
                        break;
                    default:
                        break;
                }
                SET_WORKER_HOST_CELL(grid, row, col, tileId);
            }

            TileCollisionKind typeCode = m_attrMgr->m_level->LookupTile(row, col);
            i32 oldFlags = cell->m_flags;
            i32 keep = oldFlags & 0x1bf40000;
            i32 edgeBit = oldFlags & 0x20000000;
            // The switch key is a TileCollisionKind - CTileImageSet::GetCollisionAt(0, 0)
            // for the cell's tile - so each arm is one tile kind's cell-flag word. Retail
            // wrote it biased (`switch (typeCode - 1)`), which cl 5.0 normalises: the
            // unbiased form below compiles to byte-identical .text (verified with
            // llvm-objdump -s --section=.text on this very obj).
            // 0x20, 0x24 and 0x9a stay literals: nothing in the tree names them. 0x9a
            // sits right after GAUNTLET_BRICK_A/B/C in this very switch, but it sets
            // m_flags = 0x2001 where all three of those set 0x6021, so it is NOT a fourth
            // of that band and must not be named as one. 0x24
            // does sink a rolling ball like water (CRollingBall::Update groups it with
            // TILEKIND_WATER) but it gets its own cell bit 0x800, not water's 0x100, so
            // calling it water would be a guess.
            switch (typeCode) {
                case TILEKIND_SOLID:
                    cell->m_flags = 0x1;
                    break;
                case TILEKIND_DEATH:
                    cell->m_flags = 0x2;
                    break;
                case TILEKIND_WATER:
                    cell->m_flags = 0x100;
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
                case TILEKIND_GAUNTLET_ROCK_A:
                    cell->m_flags = 0x2021;
                    break;
                case TILEKIND_GAUNTLET_ROCK_B:
                    cell->m_flags = 0x2021;
                    break;
                case TILEKIND_SPIKES:
                    cell->m_flags = 0x400;
                    break;
                case TILEKIND_GIANT_ROCK:
                    cell->m_flags = 0x2021;
                    break;
                case TILEKIND_COVERED_POWERUP:
                    cell->m_flags = 0x10000;
                    break;
                case TILEKIND_REVEALED_POWERUP:
                    cell->m_flags = 0x42;
                    break;
                case TILEKIND_SINK_HAZARD:
                    cell->m_flags = 0x800;
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
                case TILEKIND_WATERBRIDGE_UP:
                    cell->m_flags = 0x108;
                    break;
                case TILEKIND_DEATHBRIDGE_UP:
                    cell->m_flags = 0xa;
                    break;
                case TILEKIND_TOGGLEWATERBRIDGE_UP:
                    cell->m_flags = 0x300;
                    break;
                case TILEKIND_TOGGLEDEATHBRIDGE_UP:
                    cell->m_flags = 0x202;
                    break;
                case TILEKIND_HIDDEN_POWERUP:
                    cell->m_flags = 0x8000;
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
                case TILEKIND_AI_PATH_BLOCKER:
                    cell->m_flags = 0x2001;
                    break;
                default:
                    cell->m_flags = (tileId == -1) ? 2 : 0;
                    break;
            }
            if (edgeBit != 0) {
                cell->m_flags |= 0x20000000;
            }
            cell->m_flags |= keep;
            cell->m_tileId = tileId;
            cell->m_typeCode = typeCode;

            if ((cell->m_flags & 0x100) != 0) {
                i32 colCount = m_width;
                for (i32 r = static_cast<i32>(row) - 1; r <= static_cast<i32>(row) + 1; r++) {
                    if (r <= 0 || static_cast<u32>(r) >= static_cast<u32>(m_width)) {
                        continue;
                    }
                    for (i32 c = static_cast<i32>(col) - 1; c <= static_cast<i32>(col) + 1; c++) {
                        if (c < 0 || static_cast<u32>(c) >= static_cast<u32>(m_height)) {
                            continue;
                        }
                        BrickzCell* nc = &m_rows[c][r];
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
                        if (c > 0) {
                            up = nc - colCount;
                        }
                        if (static_cast<u32>(c) < static_cast<u32>(m_height - 1)) {
                            down = nc + colCount;
                        }
                        if (static_cast<u32>(r) < static_cast<u32>(colCount - 1)) {
                            right = nc + 1;
                        }
                        if (r > 0) {
                            left = nc - 1;
                        }
                        if (up != NULL && right != NULL) {
                            ur = up + 1;
                        }
                        if (up != NULL && left != NULL) {
                            ul = up - 1;
                        }
                        if (down != NULL && right != NULL) {
                            dr = down + 1;
                        }
                        if (down != NULL && left != NULL) {
                            dl = down - 1;
                        }
                        if ((up && down && !(up->m_flags & BRICKZ_BLOCKED_MASK)
                             && !(down->m_flags & BRICKZ_BLOCKED_MASK))
                            || (right && left && !(right->m_flags & BRICKZ_BLOCKED_MASK)
                                && !(left->m_flags & BRICKZ_BLOCKED_MASK))
                            || (ur && dl && !(ur->m_flags & BRICKZ_BLOCKED_MASK)
                                && !(dl->m_flags & BRICKZ_BLOCKED_MASK))
                            || (ul && dr && !(ul->m_flags & BRICKZ_BLOCKED_MASK)
                                && !(dr->m_flags & BRICKZ_BLOCKED_MASK))) {
                            nc->m_flags = nf | 0x1000;
                        }
                    }
                }
            }
        }
    }

    CDDrawChildGroup* mgr = g_gameReg->m_world->m_childGroup;
    mgr->m_walkCursor = mgr->m_list.GetHeadPosition();
    CGameObject* obj;
    if (mgr->m_walkCursor != NULL) {
        obj = mgr->NextChild(mgr->m_walkCursor);
    } else {
        obj = NULL;
    }
    while (obj != NULL) {

        if (obj->m_animWorker->m_notify == &CreateExitTrigger) {
            i32 tileX = obj->m_screenX / TILE_SIZE_PX;
            i32 tileY = obj->m_screenY / TILE_SIZE_PX;
            for (i32 xo = -1; xo < 2; xo++) {
                i32 cx = tileX + xo;
                for (i32 yo = -1; yo < 2; yo++) {

                    Coord* elem = NULL;
                    if (g_coordPool.m_freeHead->m_next != NULL) {
                        elem = &g_coordPool.m_freeHead->m_coord;
                        elem->m_x = cx;
                        elem->m_y = tileY + yo;
                        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                    }
                    m_arr.SetAtGrow(m_arr.GetSize(), elem);
                }
            }
            for (i32 k = 0; k < m_arr.GetSize(); k++) {
                Coord* elem = static_cast<Coord*>(m_arr[k]);
                if (elem != NULL && static_cast<u32>(elem->m_x) < static_cast<u32>(m_width)
                    && static_cast<u32>(elem->m_y) < static_cast<u32>(m_height)) {
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_flags = 0x10;
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_tileId = 0;

                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
        }
        CDDrawChildGroup* nextMgr = g_gameReg->m_world->m_childGroup;
        if (nextMgr->m_walkCursor != NULL) {
            obj = nextMgr->NextChild(nextMgr->m_walkCursor);
        } else {
            obj = NULL;
        }
    }

    m_dirty = 1;
    return 1;
}
