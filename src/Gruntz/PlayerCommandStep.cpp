#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>

#include <stddef.h>

// @early-stop
RVA(0x000d1b60, 0xc90)
i32 CPlay::ExecCommand(
    u8 targetIndex,
    char gruntIndex,
    GZ_ENUM_STORAGE(PlayerCommandKind, char) cmdKind,
    i16 posX,
    i16 posY,
    char extraByte,
    u8 targetType
) {
    CGruntzMgr* mgr = m_mgr;
    if (mgr->m_frameGate != 0) {
        return 0;
    }
    i32 res;
    i32 hitRow;
    i32 hitCol;

    switch (static_cast<u8>(cmdKind)) {
        case PLAYERCMD_PLACE_GRUNT: {
            u32 currentPlayer = static_cast<u32>(g_curPlayer);

            i32 r = mgr->m_cmdGrid->PlaceObject(
                static_cast<u8>(targetIndex),
                static_cast<u16>(posX),
                static_cast<u16>(posY),
                100000,
                GRUNT_ENTRANCE_DROP,
                g_groupSentinel,
                0,
                0,
                0,
                0,
                0,
                0,
                0
            );
            if (r == -1) {
                if (m_world->m_soundRegistry->m_emitGate == 0) {
                    LeafCue* cue =
                        static_cast<LeafCue*>(m_world->m_soundRegistry->Lookup("GAME_BADSELECT"));
                    if (cue != NULL) {
                        cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
                return 0;
            }
            if (static_cast<u8>(targetIndex) == currentPlayer) {
                g_gameReg->m_cmdGrid->ResetAll();
            }
            return 1;
        }

        case PLAYERCMD_MOVE: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g != NULL && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid
                      ->ClearCell(player, gi, static_cast<u16>(posX), static_cast<u16>(posY), 0);
            u32 currentPlayer = static_cast<u32>(g_curPlayer);

            if (!res) {
                if (player != currentPlayer || g == NULL || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != currentPlayer || g == NULL || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_GUARD_BEGIN: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g != NULL) {
                if (g->m_tileClaimed != 1) {
                    g->m_arrivalRerollLo = 0;
                    g->m_arrivalRerollWindowLo = 0;
                    g->m_arrivalRerollHi = 0;
                    g->m_arrivalRerollWindowHi = 0;
                    g->m_defenderPx.m_x = g->m_lastTilePx.m_x;
                    g->m_tileClaimed = 1;
                    g->m_defenderPx.m_y = g->m_lastTilePx.m_y;

                    switch (g->m_entranceReason) {
                        case PICKUP_BOOMERANG:
                            g->m_defenderRadius = 1;
                            break;
                        case PICKUP_GUNHAT:
                        case PICKUP_NERFGUN:
                        case PICKUP_ROCK:
                            g->m_defenderRadius = 1;
                            break;
                        case PICKUP_WELDER:
                        case PICKUP_WINGZ:
                            g->m_defenderRadius = 1;
                            break;
                        default:
                            g->m_defenderRadius =
                                g_buteMgr.GetIntDef("Grunt", "PlayerDefenderRadius", 3) + 1;
                    }
                    g->m_arrivalFlags |= 0x18040402;
                    g->m_arrivalCell.m_x = -1;
                    g->m_arrivalState = AI_DEFENDER;
                    g->m_defenderState = AISTATE_SEEK;
                    g->m_arrivalCell.m_y = -1;
                    g->m_arrivalActive = 0;
                    g->m_object->m_extent.left = 0;
                    g->m_object->m_extent.right = 0;
                    g->m_object->m_extent.top = 0;
                    g->m_object->m_extent.bottom = 0;
                    g->SetEntrancePos(1, 1);
                }
                g->m_arrivalNotified = 0;
            }
            return 1;
        }

        case PLAYERCMD_GUARD_END: {

            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            // Gate on m_tileClaimed (+0x420), NOT m_entranceCommitted (+0x1fc): retail
            // reads the SAME slot it is about to clear five instructions later, i.e.
            // "if this grunt is not guarding, do nothing".  Gating on the always-set
            // committed flag ran the whole guard teardown - m_arrivalState = AI_NONE,
            // the 0xe7fbfbfd flag mask and SetEntrancePos(1,1) - on any grunt, so a
            // stray GUARD_END cancelled whatever that grunt was actually doing.
            if (g == NULL || g->m_tileClaimed == 0) {
                return 1;
            }
            g->m_arrivalRerollLo = 0;
            g->m_arrivalRerollWindowLo = 0;
            g->m_arrivalRerollHi = 0;
            g->m_arrivalRerollWindowHi = 0;
            g->m_tileClaimed = 0;
            g->m_arrivalState = AI_NONE;
            g->m_arrivalFlags &= 0xe7fbfbfd;
            g->SetEntrancePos(1, 1);
            return 1;
        }

        case PLAYERCMD_USE_TOOL_AT_POINT: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(posX);
            i32 py = static_cast<u16>(posY);

            CGrunt* node = m_mgr->m_cmdGrid->CellHitTest(px, py, &hitRow, &hitCol, TM_GRID_ROW_ALL);
            if (node != NULL && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    hitRow,
                    hitCol,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gi, px, py, 2);
            if (res) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case PLAYERCMD_USE_TOOL_ON_GRUNT: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 row = static_cast<u16>(posX);
            i32 col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == NULL || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(row, col, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gi, sx, sy, 2);
            if (res) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case PLAYERCMD_USE_TOY_AT_POINT: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(posX);
            i32 py = static_cast<u16>(posY);
            CGrunt* node = m_mgr->m_cmdGrid->CellHitTest(px, py, &hitRow, &hitCol, TM_GRID_ROW_ALL);
            if (node != NULL && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    hitRow,
                    hitCol,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gi, px, py, 3);
            if (res) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case PLAYERCMD_USE_TOY_ON_GRUNT: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 row = static_cast<u16>(posX);
            i32 col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == NULL || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(row, col, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gi, sx, sy, 3);
            if (res) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case PLAYERCMD_GIVE_TOOL: {
            u32 player = static_cast<u8>(targetIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_playerCommandPending = 0;
            }
            u32 gi = static_cast<u8>(gruntIndex);
            i32 idx = gi + player * 0xf;
            CGrunt* g = mgr->m_cmdGrid->m_grid[idx];
            if (g != NULL && g->m_entranceCommitted != 0 && g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 sel = 0;
            i32 live = (g_gameReg->m_gameMode != GAMEMODE_SINGLE);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[idx];
            i32 r;
            if (g2 == NULL || g2->m_entranceCommitted == 0) {
                r = 0;
            } else {
                r = g2->LoadPickupSprites(static_cast<PickupType>(extraByte & 0xff), 0, 0, 0, live);
            }
            if (r != 0) {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, gi, 0, 0);
                }
                sel = 1;
            }
            if (player == static_cast<u32>(g_curPlayer)) {
                m_dragInhibit2 = 0;
                m_guts->EnterHlRow(sel, m_cursorFrame);
                SetCursorFrame(0);
            }
            return r;
        }

        case PLAYERCMD_STOP: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            g->SetEntrancePos(1, 1);
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            return 1;
        }
    }

    return 1;
}
