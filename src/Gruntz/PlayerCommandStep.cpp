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

// @early-stop
RVA(0x000d1b60, 0xc2f)
// The seven command words are i32, not the narrow char/i16 they used to be modelled
// as (2026-07-29). PROOF: retail's case-3 probe call at 0xd1f6b pushes
// `lea edx,[esp+0x28]` (0xd1f44) and `lea edx,[esp+0x24]` (0xd1f4b) - after the two
// intervening pushes those are the `extraByte` and `cmdKind` PARAMETER HOMES - so the
// probe writes i32 OUTPUTS into them and the code reads them back whole. `ret 0x1c`
// pins the 7-dword arity either way, and every narrow use is an explicit mask
// (`static_cast<u8>(targetIndex)` etc.) that still lowers to retail's `movzx`. The
// twelve `*(i32*)&aN` puns the narrow model needed are gone, and the fn measured
// 21.63 -> 22.19 on the change.
//
// PARAMETER NAMES (2026-07-29) come from the two Select overrides that call this,
// CGruntzSingleCommand::Select @0x24140 / CGruntzMultiCommand::Select @0x24190:
//   ExecCommand(m_targetIndex, m_10, m_5, m_8, m_a, m_11, m_targetType)
// and CGruntzCommand::SetParamsEx @0x23e60 names those seven fields
// (targetIndex, cmdKind, targetType, posX, posY, gruntIndex, extraByte). The body
// agrees: retail's dispatch is `mov ecx,[esp+0x1c]; and ecx,0xff; cmp ecx,0xa; ja` -
// the switch runs on the THIRD stack arg, i.e. cmdKind; targetIndex is the row the
// grid stride 0xf multiplies and the word compared against g_curPlayer; gruntIndex is
// the column; posX/posY are the u16-masked pixel pair fed to CellHitTest.
//
// The still-open modelling debt is the SLOT ASSIGNMENT: retail reuses the cmdKind and
// extraByte PARAMETER HOMES as CellHitTest's two out-params, so every read of those
// slots AFTER the probe sees the probe's outputs. This source models them as separate
// `col`/`row` locals, so the post-probe ClearCell below still reads the pre-probe
// cmdKind/targetType - visible now that the names differ. At 0xd1fa0 retail's
// post-probe grid call also takes FOUR args (targetIndex&0xff, gruntIndex&0xff, and
// the two spilled masked halves) where the body below passes five, and it reads
// extraByte's home where the body reads targetType's. That is a body-reconstruction
// job, not a naming or cast job - left for the matcher lane.
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

    // grid is re-derived per case as this->m_4->m_cmdGrid: retail keeps `this` in a
    // callee-saved reg and re-reads m_4 inside each case (only case 0 reuses the
    // gate's cached `mgr` in eax); caching it across the whole switch would pin it
    // in a callee-saved reg and spill `this`. CSE collapses the repeats within a case.
    switch (static_cast<u8>(cmdKind)) {
        default:
            return 1;

        case 0: {
            // case 0 reuses the gate's cached mgr (eax) for the spawn probe.
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
                // The ex-`extern "C" i32 BadSelect(const char*)` was an ILT
                // placeholder: 0x402cca jmps to 0x05b7e0 ==
                // CDDrawSubMgrLeafScan::Lookup, and retail's `call 0x2cca` at
                // 0xd1bf5 runs on the ecx the gate above already loaded
                // ([ebx+0xc]->+0x28 == m_world->m_soundRegistry). Lookup returns
                // CObject*, so the cue is a plain single-inheritance downcast
                // (LeafCue : CLoadable : CWapObj : CObject).
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
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(gruntIndex) + player * 0xf];
            if (g != 0 && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ClearCell(
                player,
                static_cast<u8>(gruntIndex),
                static_cast<u16>(posX),
                static_cast<u16>(posY),
                0
            );
            if (res != 0) {
                if (player != static_cast<u32>(g_curPlayer)) {
                    return 1;
                }
                if (g != 0 && g->m_entranceCommitted != 0) {
                    GruntCue(g, 0x323, -1, 0, -1, -1);
                }
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g == 0 || g->m_entranceCommitted == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 3:
        case 4: {
            // Capture the command discriminator before PathProbe overwrites cmdKind (retail
            // threads the path-probe outputs back through the &cmdKind/&targetType param slots).
            // Read bit 2 (set for cmd 4, clear for cmd 3) so it is NOT CSE'd with the
            // switch selector `cmdKind & 0xff` (which would spill the selector + add a frame).
            i32 isB = cmdKind & 4;
            u32 player = static_cast<u8>(targetIndex);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(gruntIndex) + player * 0xf];
            if (g == 0 || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (isB != 0 && g->m_entranceActive != 0) {
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
            u32 px = static_cast<u16>(posX);
            u32 py = static_cast<u16>(posY);
            i32 col = cmdKind;
            i32 row = targetType;
            CGrunt* node =
                static_cast<CGrunt*>(m_mgr->m_cmdGrid->CellHitTest(px, py, &col, &row, 5));
            if (node == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
            } else {
                g->SetArrivalTarget(
                    static_cast<i32>(player),
                    px,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            }
            res = (isB == 0) ? m_mgr->m_cmdGrid->ApplyTriggerA(player, col, row, 0)
                             : m_mgr->m_cmdGrid->ApplyTriggerB(player, col, row, 0);
            if (res != 0) {
                if (res != -1) {
                    if (player != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res =
                    m_mgr->m_cmdGrid->ClearCell(player, cmdKind, targetType, 0, (isB == 0) ? 2 : 3);
                if (res != 0) {
                    if (player != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer)) {
                return 0;
            }
            res = g->m_entranceCommitted;
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 5: {
            CGrunt* g =
                m_mgr->m_cmdGrid
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

        case 6: {
            CGrunt* g =
                m_mgr->m_cmdGrid
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
                    switch (g->m_entranceReason) {
                        case 2:
                        case 9:
                        case 10:
                        case 0xb:
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
            CGrunt* g =
                m_mgr->m_cmdGrid
                    ->m_grid[static_cast<u8>(targetIndex) * 0xf + static_cast<u8>(gruntIndex)];
            if (g == 0 || g->m_tileClaimed == 0) {
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

        case 8: {
            u32 player = static_cast<u8>(targetIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_4f0 = 0;
            }
            i32 idx = static_cast<u8>(gruntIndex) + player * 0xf;
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[idx];
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
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[idx];
            i32 r;
            if (g2 == 0 || g2->m_entranceCommitted == 0) {
                r = 0;
            } else {
                r = PickupCheck(static_cast<u8>(extraByte), 0, 0, 0, g_gameReg->m_134 != 1);
            }
            i32 sel;
            if (r == 0) {
                sel = 0;
            } else {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, static_cast<u8>(gruntIndex), 0, 0);
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

        case 9: {
            u32 player = static_cast<u8>(targetIndex);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(gruntIndex) + player * 0xf];
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
            u32 row = static_cast<u16>(posX), col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_object;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, extraByte, row, 0);
            if (res != 0) {
                if (res == -1) {
                    res = m_mgr->m_cmdGrid->ClearCell(player, targetType, targetIndex, 0, 2);
                    if (res == 0) {
                        if (static_cast<u32>(g_curPlayer) != player
                            || g->m_entranceCommitted == 0) {
                            return 0;
                        }
                        GruntCue(g, 0x324, -1, 0, -1, -1);
                        return 0;
                    }
                    if (static_cast<u8>(targetIndex) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (static_cast<u32>(cmdKind) != static_cast<u32>(g_curPlayer)
                        && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (static_cast<u8>(targetIndex) != static_cast<u32>(g_curPlayer)) {
                    return 1;
                }
                if (static_cast<u32>(g_curPlayer) != static_cast<u32>(targetType)
                    && g->m_entranceCommitted != 0) {
                    GruntCue(g, 0x325, -1, 0, -1, -1);
                }
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer)) {
                return 0;
            }
            res = g->m_entranceCommitted;
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 10: {
            u32 player = static_cast<u8>(targetIndex);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(gruntIndex) + player * 0xf];
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
            u32 row = static_cast<u16>(posX), col = static_cast<u16>(posY);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_object;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, extraByte, row, 0);
            if (res != 0) {
                if (res != -1) {
                    if (static_cast<u8>(targetIndex) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (static_cast<u32>(targetType) != static_cast<u32>(g_curPlayer)
                        && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = m_mgr->m_cmdGrid->ClearCell(player, targetType, targetIndex, 0, 3);
                if (res != 0) {
                    if (static_cast<u8>(targetIndex) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (static_cast<u32>(g_curPlayer) != static_cast<u32>(cmdKind)
                        && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (static_cast<u32>(g_curPlayer) != player || g->m_entranceCommitted == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer)) {
                return 0;
            }
            res = g->m_entranceCommitted;
            break;
        }
    }

    if (res == 0) {
        return 0;
    }
    return 0;
}
