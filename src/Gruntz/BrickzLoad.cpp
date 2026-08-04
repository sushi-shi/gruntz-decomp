#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>

static __inline i32 PickA(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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
    return (roll > t4) ? BRICKTILE_BLACK_1 : BRICKTILE_GOLD_1;
}
static __inline i32 PickB(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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
static __inline i32 PickC(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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

RVA(0x000810f0, 0x8b4)
i32 CGruntzMapMgr::LoadAttributes(i32 width, i32 height) {
    m_attrMgr = g_gameReg->m_world;
    CDDrawWorkerHost* grid = m_attrMgr->m_level->m_mainPlane;
    if (grid == NULL) {
        return 0;
    }
    AllocGrid(width, height, 0);
    m_reserved90 = 0;

    i32 t1 = g_buteMgr.GetInt("Brickz", "Brown");
    i32 t2 = t1 + g_buteMgr.GetInt("Brickz", "Red");
    i32 t3 = t2 + g_buteMgr.GetInt("Brickz", "Blue");
    i32 t4 = t3 + g_buteMgr.GetInt("Brickz", "Gold");
    i32 total = t4 + g_buteMgr.GetInt("Brickz", "Black");

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
                if (tileId == 0x105) {
                    tileId = 0x101;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x101;
                } else if (tileId == 0x106) {
                    tileId = 0x103;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x103;
                }
            }

            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
                switch (tileId) {
                    case BRICKTILE_BROWN_1:
                    case BRICKTILE_RED_1:
                    case BRICKTILE_BLUE_1:
                    case BRICKTILE_GOLD_1:
                    case BRICKTILE_BLACK_1:
                        tileId = PickA(total, t1, t2, t3, t4);
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
                        tileId = PickB(total, t1, t2, t3, t4);
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
                        tileId = PickC(total, t1, t2, t3, t4);
                        break;
                    default:
                        break;
                }
                grid->m_tileGrid[grid->m_colOffsets[col] + row] = tileId;
            }

            i32 typeCode = m_attrMgr->m_level->LookupTile(row, col);
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
                case TILEKIND_SOFT:
                    cell->m_flags = 0x1;
                    break;
                case TILEKIND_SPECIAL:
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
                case 0x20:
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
                case 0x24:
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
                case 0x9a:
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
                for (i32 r = row - 1; r <= row + 1; r++) {
                    if (r < 0 || static_cast<u32>(r) >= static_cast<u32>(m_width)) {
                        continue;
                    }
                    for (i32 c = col - 1; c <= col + 1; c++) {
                        if (c < 0 || static_cast<u32>(c) >= static_cast<u32>(m_height)) {
                            continue;
                        }
                        BrickzCell* nc = &m_rows[c][r];
                        i32 nf = nc->m_flags & ~0x1000;
                        nc->m_flags = nf;
                        if ((nf & 0x100) == 0) {
                            continue;
                        }
                        BrickzCell* up = (r != 0) ? nc - 1 : 0;
                        BrickzCell* down = (r < colCount - 1) ? nc + 1 : 0;
                        BrickzCell* right =
                            (c < static_cast<i32>(m_height) - 1) ? nc + colCount : 0;
                        BrickzCell* left = (c != 0) ? nc - colCount : 0;
                        BrickzCell* ur = (up && right) ? up + colCount : 0;
                        BrickzCell* dl = (down && left) ? down - colCount : 0;
                        BrickzCell* ul = (up && left) ? up - colCount : 0;
                        BrickzCell* dr = (down && right) ? down + colCount : 0;
                        bool set = false;
                        if (up && down && !(up->m_flags & BRICKZ_BLOCKED_MASK)
                            && !(down->m_flags & BRICKZ_BLOCKED_MASK)) {
                            set = true;
                        } else if (right && left && !(right->m_flags & BRICKZ_BLOCKED_MASK)
                                   && !(left->m_flags & BRICKZ_BLOCKED_MASK)) {
                            set = true;
                        } else if (ur && dl && !(ur->m_flags & BRICKZ_BLOCKED_MASK)
                                   && !(dl->m_flags & BRICKZ_BLOCKED_MASK)) {
                            set = true;
                        } else if (ul && dr && !(ul->m_flags & BRICKZ_BLOCKED_MASK)
                                   && !(dr->m_flags & BRICKZ_BLOCKED_MASK)) {
                            set = true;
                        }
                        if (set) {
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
        obj = static_cast<CGameObject*>(mgr->m_list.GetNext(mgr->m_walkCursor));
    } else {
        obj = NULL;
    }
    while (obj != NULL) {

        if (obj->m_animWorker->m_notify == &CreateExitTrigger) {
            i32 tileX = (obj->m_screenX + (obj->m_screenX >> 31 & TILE_MASK_PX)) >> TILE_SHIFT_PX;
            i32 tileY = (obj->m_screenY + (obj->m_screenY >> 31 & TILE_MASK_PX)) >> TILE_SHIFT_PX;
            for (i32 xo = -1; xo < 2; xo++) {
                for (i32 yo = -1; yo < 2; yo++) {

                    Coord* elem = 0;
                    if (g_coordPool.m_freeHead->m_next != NULL) {
                        elem = &g_coordPool.m_freeHead->m_coord;
                        elem->m_x = tileX + xo;
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
        if (mgr->m_walkCursor != NULL) {
            obj = static_cast<CGameObject*>(mgr->m_list.GetNext(mgr->m_walkCursor));
        } else {
            obj = NULL;
        }
    }

    m_dirty = 1;
    return 1;
}
