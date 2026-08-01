#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>

#include <rva.h>
#include <Gruntz/LeafCue.h>               // canonical LeafCue (PlayIfElapsed)
#include <Gruntz/Grunt.h>                 // canonical CGrunt (SetEntrancePos/SetArrivalTarget)
#include <Gruntz/TriggerMgr.h>            // canonical CTriggerMgr (the mgr's m_cmdGrid grid)
#include <Gruntz/Play.h>                  // canonical CPlay (the ex-CCmdHandler identity)
#include <Gruntz/GruntzMgr.h>             // canonical CGruntzMgr (CPlay::m_4)
#include <Gruntz/StatusBarMgr.h>          // CStatusBarMgr::EnterHlRow (m_guts, +0x2dc)
#include <Gruntz/SoundState.h>            // ex Globals.h transitive
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan::Lookup (m_world->m_soundRegistry)

static const char s_gameBadSelect[] = "GAME_BADSELECT";              // 0x612c28
static const char s_grunt[] = "Grunt";                               // 0x60a9ec
static const char s_playerDefenderRadius[] = "PlayerDefenderRadius"; // 0x60e1ac
// The seven command words are i32: `ret 0x1c` pins the 7-dword arity, and retail's
// case-3/4 probe call reuses the `cmdKind` and `extraByte` PARAMETER HOMES as
// CellHitTest's two out-params (`lea edx,[esp+0x24]` / `lea edx,[esp+0x28]` at
// 0xd1f4b/0xd1f44), so those slots are read back whole after the probe. Every narrow
// use is an explicit mask that still lowers to retail's `and reg,0xff` / `0xffff`.
//
// PARAMETER NAMES come from the two Select overrides that call this,
// CGruntzSingleCommand::Select @0x24140 / CGruntzMultiCommand::Select @0x24190:
//   ExecCommand(m_targetIndex, m_10, m_5, m_8, m_a, m_11, m_targetType)
// and CGruntzCommand::SetParamsEx @0x23e60 names those seven fields
// (targetIndex, cmdKind, targetType, posX, posY, gruntIndex, extraByte).
//
// ARM ORDER IS RETAIL'S. The 11-slot table at 0x4d2790 lands the arms in .text as
// 0, 2, 6, 7, 3, 9, 4, 10, 8, 5, then the shared default at 0xd2783 - that is the
// source order and it is load-bearing. Case 1's slot points at the default block.
//
// CASES 3 AND 4 ARE SEPARATE ARMS (0xd1eb7 vs 0xd2175), not one arm behind a
// `cmdKind & 4` discriminator: 4 carries an extra `m_entranceActive` guard that 3
// does not, and dispatches ApplyTriggerB/ClearCell-mode-3 where 3 uses
// ApplyTriggerA/mode-2. Same for the 9 / 10 pair. cl tail-merges what is genuinely
// common (the ClearCell prologue at 0xd22ad is shared by 3 and 4; the 0x324 cue tail
// at 0xd2488/0xd2495 is shared across four arms).
//
// targetType (arg7, [esp+0x2c]) is never read: retail threads the trigger/clear
// coordinates through the masked posX/posY pair and the CellHitTest out-params.
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
            // The spawn probe reuses the gate's cached mgr (nothing intervenes, so cl
            // CSEs the `[this+4]` load); every later grid touch re-reads m_mgr.
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
                    // retail re-loads the grid from g_gameReg (0x64556c), not mgr.
                    g_gameReg->m_cmdGrid->ResetAll();
                }
                return 1;
            }
            if (m_world->m_soundRegistry->m_emitGate == 0) { // the sound host's busy/emit gate
                // 0x402cca jmps to 0x05b7e0 == CDDrawSubMgrLeafScan::Lookup, and retail's
                // `call 0x2cca` at 0xd1bf5 runs on the ecx the gate above already loaded
                // ([ebx+0xc]->+0x28 == m_world->m_soundRegistry). Lookup returns CObject*,
                // so the cue is a plain single-inheritance downcast.
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
            // retail 0xd1c9d branches AWAY on res != 0, i.e. the res == 0 arm is the
            // fallthrough - the source tests `== 0` first.
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g == 0
                    || g->m_entranceCommitted == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer) || g == 0 || g->m_entranceCommitted == 0) {
                return 1;
            }
            GruntCue(g, 0x323, -1, 0, -1, -1);
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
                    g->m_defenderX = g->m_lastTilePxX;
                    g->m_tileClaimed = 1;
                    g->m_defenderY = g->m_lastTilePxY;
                    // The 21-slot BYTE index table at 0x4d27cc feeds a FOUR-entry target
                    // table at 0x4d27bc: {2}, {9,10,11} and {0x15,0x16} each get their own
                    // index value even though cl folded the three identical bodies onto one
                    // address. Merging them into a single case group collapses the byte
                    // table to two values and loses the shape.
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
                    g->m_arrivalCol = -1;
                    g->m_arrivalState = 4;
                    g->m_defenderState = 0;
                    g->m_arrivalRow = -1;
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
            // retail 0xd1e62 gates on m_entranceCommitted (+0x1fc), NOT m_tileClaimed:
            // the clear runs unconditionally once the grunt is committed.
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
            // the probe writes its two outputs back into the cmdKind / extraByte homes
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
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, px, py, 2);
            if (res != 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
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
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, sx, sy, 2);
            if (res != 0) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
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
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, px, py, 3);
            if (res != 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
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
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res != -1) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, gruntIndex, sx, sy, 3);
            if (res != 0) {
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(row)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                GruntCue(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 8: {
            u32 player = static_cast<u8>(targetIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_4f0 = 0;
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
            i32 live = (g_gameReg->m_134 != 1);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[idx];
            i32 r;
            if (g2 == 0 || g2->m_entranceCommitted == 0) {
                r = 0;
            } else {
                r = PickupCheck(static_cast<u8>(extraByte), 0, 0, 0, live);
            }
            if (r != 0) {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, gruntIndex, 0, 0);
                }
                sel = 1;
            }
            if (player == static_cast<u32>(g_curPlayer)) {
                m_dragInhibit2 = 0;
                m_guts->EnterHlRow(sel, m_cursorFrame); // 0x213f, ecx = m_guts (+0x2dc)
                SetCursorFrame(0);                      // 0x17a8, ecx = this
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

    // 0xd2783 - the shared default block; the table's case-1 slot points here too.
    return 1;
}
