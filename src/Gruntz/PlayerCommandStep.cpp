#include <Gruntz/GruntSpawnConfig.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>

#include <rva.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/SoundState.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>

static const char s_gameBadSelect[] = "GAME_BADSELECT";
static const char s_grunt[] = "Grunt";
static const char s_playerDefenderRadius[] = "PlayerDefenderRadius";

// @early-stop
RVA(0x000d1b60, 0xc90)
i32 CPlay::ExecCommand(
    i32 targetIndex,
    i32 gruntIndex,
    i32 cmdKind,
    i32 posX,
    i32 posY,
    i32 extraByte,
    i32 targetType
) {
    CGruntzMgr* mgr = m_mgr;
    if (mgr->m_frameGate != 0) {
        return 0;
    }
    i32 res;

    switch (static_cast<u8>(cmdKind)) {
        case 0: {

            i32 r = mgr->m_cmdGrid->PlaceObject(
                static_cast<u8>(targetIndex),
                static_cast<u16>(posX),
                static_cast<u16>(posY),
                100000,
                2,
                g_groupSentinel,
                0,
                0,
                0,
                0,
                0,
                0,
                0
            );
            if (r != -1) {
                if (static_cast<u8>(targetIndex) == static_cast<u32>(g_curPlayer)) {

                    g_gameReg->m_cmdGrid->ResetAll();
                }
                return 1;
            }
            if (m_world->m_soundRegistry->m_emitGate == 0) {

                LeafCue* cue =
                    static_cast<LeafCue*>(m_world->m_soundRegistry->Lookup(s_gameBadSelect));
                if (cue != 0) {
                    cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            return 0;
        }

        case 2: {
            u32 player = static_cast<u8>(targetIndex);
            u32 gi = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gi + player * 0xf];
            if (g != 0 && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid
                      ->ClearCell(player, gi, static_cast<u16>(posX), static_cast<u16>(posY), 0);

            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g == 0
                    || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer) || g == 0 || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case 6: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g != 0) {
                if (g->m_tileClaimed != 1) {
                    g->m_arrivalRerollLo = 0;
                    g->m_arrivalRerollWindowLo = 0;
                    g->m_arrivalRerollHi = 0;
                    g->m_arrivalRerollWindowHi = 0;
                    g->m_defenderPx.m_x = g->m_lastTilePx.m_x;
                    g->m_tileClaimed = 1;
                    g->m_defenderPx.m_y = g->m_lastTilePx.m_y;

                    switch (g->m_entranceReason) {
                        case 2:
                            g->m_defenderRadius = 1;
                            break;
                        case 9:
                        case 10:
                        case 0xb:
                            g->m_defenderRadius = 1;
                            break;
                        case 0x15:
                        case 0x16:
                            g->m_defenderRadius = 1;
                            break;
                        default:
                            g->m_defenderRadius =
                                g_buteMgr.GetIntDef(s_grunt, s_playerDefenderRadius, 3) + 1;
                    }
                    g->m_arrivalFlags |= 0x18040402;
                    g->m_arrivalCell.m_x = -1;
                    g->m_arrivalState = 4;
                    g->m_defenderState = 0;
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

        case 7: {

            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g == 0 || g->m_entranceCommitted == 0) {
                return 1;
            }
            g->m_arrivalRerollLo = 0;
            g->m_arrivalRerollWindowLo = 0;
            g->m_arrivalRerollHi = 0;
            g->m_arrivalRerollWindowHi = 0;
            g->m_tileClaimed = 0;
            g->m_arrivalState = 0;
            g->m_arrivalFlags &= 0xe7fbfbfd;
            g->SetEntrancePos(1, 1);
            return 1;
        }

        case 3: {
            u32 player = static_cast<u8>(targetIndex);
            gruntIndex = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gruntIndex + player * 0xf];
            if (g == 0 || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(posX);
            i32 py = static_cast<u16>(posY);

            CGrunt* node = m_mgr->m_cmdGrid->CellHitTest(px, py, &cmdKind, &extraByte, 5);
            if (node != 0 && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    cmdKind,
                    extraByte,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gruntIndex, px, py);
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
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, px, py, 2);
            if (res != 0) {
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

        case 9: {
            u32 player = static_cast<u8>(targetIndex);
            gruntIndex = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gruntIndex + player * 0xf];
            if (g == 0 || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 row = static_cast<u16>(posX);
            i32 col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(row, col, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gruntIndex, sx, sy);
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
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, sx, sy, 2);
            if (res != 0) {
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

        case 4: {
            u32 player = static_cast<u8>(targetIndex);
            gruntIndex = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gruntIndex + player * 0xf];
            if (g == 0 || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(posX);
            i32 py = static_cast<u16>(posY);
            CGrunt* node = m_mgr->m_cmdGrid->CellHitTest(px, py, &cmdKind, &extraByte, 5);
            if (node != 0 && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    cmdKind,
                    extraByte,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gruntIndex, px, py);
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
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, px, py, 3);
            if (res != 0) {
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

        case 10: {
            u32 player = static_cast<u8>(targetIndex);
            gruntIndex = static_cast<u8>(gruntIndex);
            CGrunt* g = mgr->m_cmdGrid->m_grid[gruntIndex + player * 0xf];
            if (g == 0 || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 row = static_cast<u16>(posX);
            i32 col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(row, col, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gruntIndex, sx, sy);
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
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, sx, sy, 3);
            if (res != 0) {
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

        case 8: {
            u32 player = static_cast<u8>(targetIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_playerCommandPending = 0;
            }
            gruntIndex = static_cast<u8>(gruntIndex);
            i32 idx = gruntIndex + player * 0xf;
            CGrunt* g = mgr->m_cmdGrid->m_grid[idx];
            if (g != 0 && g->m_entranceCommitted != 0 && g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 sel = 0;
            i32 live = (g_gameReg->m_gameMode != 1);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[idx];
            i32 r;
            if (g2 == 0 || g2->m_entranceCommitted == 0) {
                r = 0;
            } else {
                r = g2->LoadPickupSprites(static_cast<u8>(extraByte), 0, 0, 0, live);
            }
            if (r != 0) {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, gruntIndex, 0, 0);
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

        case 5: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g == 0 || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            g->SetEntrancePos(1, 1);
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = 0;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            return 1;
        }
    }

    return 1;
}
