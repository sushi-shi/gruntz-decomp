#include <StdAfx.h>

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BrickStackInline.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BrickzNeighborMacros.h>
#include <Gruntz/BridgeTileId.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>

RVA(0x000810f0, 0xa80)
i32 CGruntzMapMgr::BuildCellAttributes(i32 width, i32 height) {
    m_attrMgr = g_gameReg->m_world;
    CDDrawWorkerHost* grid = m_attrMgr->m_level->m_mainPlane;
    if (grid == NULL) {
        return 0;
    }
    AllocGrid(width, height, NULL);
    m_reserved90 = 0;

    i32 totalWeight = g_buteMgr.GetInt("Brickz", "Brown");
    i32 brownThreshold = totalWeight;
    totalWeight += g_buteMgr.GetInt("Brickz", "Red");
    i32 redThreshold = totalWeight;
    totalWeight += g_buteMgr.GetInt("Brickz", "Blue");
    i32 blueThreshold = totalWeight;
    totalWeight += g_buteMgr.GetInt("Brickz", "Gold");
    i32 goldThreshold = totalWeight;
    totalWeight += g_buteMgr.GetInt("Brickz", "Black");

    BrickzCell* cell = m_cellPool;
    for (u32 tileY = 0; tileY < m_height; tileY++) {
        for (u32 tileX = 0; tileX < m_width; tileX++, cell++) {
            i32 tileId = grid->m_tileHandles[grid->m_tileRowOffsets[tileY] + tileX];
            if (tileId != -1) {
                tileId &= 0xffff;
            }
            cell->m_flags = 0;
            cell->m_occupantId = -1;
            cell->m_objectId = 0;
            cell->m_tileId = -1;

            if (g_gameReg->m_isEasyMode != false && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                BridgeTileId bridgeTile = static_cast<BridgeTileId>(tileId);
                if (bridgeTile == BRIDGETILE_WATER_UP_ALT) {
                    tileId = IDX(BRIDGETILE_WATER_UP);
                    SET_WORKER_HOST_CELL(grid, tileX, tileY, IDX(BRIDGETILE_WATER_UP));
                } else if (bridgeTile == BRIDGETILE_DEATH_UP_ALT) {
                    tileId = IDX(BRIDGETILE_DEATH_UP);
                    SET_WORKER_HOST_CELL(grid, tileX, tileY, IDX(BRIDGETILE_DEATH_UP));
                }
            }

            if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                switch (static_cast<BrickTileId>(tileId)) {
                    case BRICKTILE_BROWN_1:
                    case BRICKTILE_RED_1:
                    case BRICKTILE_BLUE_1:
                    case BRICKTILE_GOLD_1:
                    case BRICKTILE_BLACK_1:
                        tileId = IDX(PickOneBrickStack(
                            totalWeight,
                            brownThreshold,
                            redThreshold,
                            blueThreshold,
                            goldThreshold
                        ));
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
                        tileId = IDX(PickTwoBrickStack(
                            totalWeight,
                            brownThreshold,
                            redThreshold,
                            blueThreshold,
                            goldThreshold
                        ));
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
                        tileId = IDX(PickThreeBrickStack(
                            totalWeight,
                            brownThreshold,
                            redThreshold,
                            blueThreshold,
                            goldThreshold
                        ));
                        break;
                    default:
                        break;
                }
                SET_WORKER_HOST_CELL(grid, tileX, tileY, tileId);
            }

            TileCollisionKind typeCode = m_attrMgr->m_level->LookupTile(tileX, tileY);
            i32 oldFlags = cell->m_flags;
            i32 keep = oldFlags & 0x1bf40000;
            i32 edgeBit = oldFlags & BRICKZ_CELL_OCCUPIED;
            switch (typeCode) {
                case TILEKIND_SOLID:
                    cell->m_flags = 0x1;
                    break;
                case TILEKIND_DEATH:
                    cell->m_flags = IDX(CELL_FLAG_SPECIAL);
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
                    cell->m_flags = IDX(CELL_FLAG_COVERED_POWERUP);
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
                    cell->m_flags = IDX(CELL_FLAG_HIDDEN_POWERUP);
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
                    cell->m_flags = (tileId == -1) ? IDX(CELL_FLAG_SPECIAL) : 0;
                    break;
            }
            if (edgeBit != 0) {
                cell->m_flags |= BRICKZ_CELL_OCCUPIED;
            }
            cell->m_flags |= keep;
            cell->m_tileId = tileId;
            cell->m_typeCode = typeCode;

            for (i32 neighborTileX = static_cast<i32>(tileX) - 1;
                 neighborTileX <= static_cast<i32>(tileX) + 1;
                 neighborTileX++) {
                for (i32 neighborTileY = static_cast<i32>(tileY) - 1;
                     neighborTileY <= static_cast<i32>(tileY) + 1;
                     neighborTileY++) {
                    if (neighborTileY < 0
                        || static_cast<u32>(neighborTileY) >= static_cast<u32>(m_height)
                        || neighborTileX <= 0
                        || static_cast<u32>(neighborTileX) >= static_cast<u32>(m_width)) {
                        continue;
                    }
                    BrickzCell* nc = &m_rows[neighborTileY][neighborTileX];
                    i32 nf = nc->m_flags & ~0x1000;
                    nc->m_flags = nf;
                    if ((nf & 0x100) == 0) {
                        continue;
                    }
                    DECLARE_BRICKZ_NEIGHBORS(nc, neighborTileX, neighborTileY)
                    if (BRICKZ_OPPOSITE_NEIGHBORS_OPEN) {
                        nc->m_flags = nf | 0x1000;
                    }
                }
            }
        }
    }

    CDDrawChildGroup* childGroup = g_gameReg->m_world->m_childGroup;
    for (CGameObject* obj = childGroup->FirstChild(); obj != NULL;
         obj = g_gameReg->m_world->m_childGroup->NextChild()) {

        if (obj->m_logicRecord->m_dispatch == &DispatchExitTriggerLogic) {
            i32 tileX = obj->m_screenX / TILE_SIZE_PX;
            i32 tileY = obj->m_screenY / TILE_SIZE_PX;
            for (i32 xo = -1; xo < 2; xo++) {
                i32 cx = tileX + xo;
                for (i32 yo = -1; yo < 2; yo++) {

                    Coord cell;
                    m_arr.Add(g_coordPool.PopCopy(*cell.Set(cx, tileY + yo)));
                }
            }
            for (i32 k = 0; k < m_arr.GetSize(); k++) {
                Coord* elem = static_cast<Coord*>(m_arr[k]);
                if (elem != NULL && static_cast<u32>(elem->m_x) < static_cast<u32>(m_width)
                    && static_cast<u32>(elem->m_y) < static_cast<u32>(m_height)) {
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_flags = 0x10;
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_tileId = 0;

                    g_coordPool.Push(elem);
                }
            }
            m_arr.SetSize(0, -1);
        }
    }

    m_dirty = true;
    return 1;
}
