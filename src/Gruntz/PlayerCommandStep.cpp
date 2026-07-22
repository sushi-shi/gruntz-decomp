#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>

#include <rva.h>
#include <Globals.h>
#include <Gruntz/LeafCue.h>      // canonical LeafCue (PlayIfElapsed)
#include <Gruntz/Grunt.h>        // canonical CGrunt (SetEntrancePos/SetArrivalTarget)
#include <Gruntz/TriggerMgr.h>   // canonical CTriggerMgr (the mgr's m_cmdGrid grid)
#include <Gruntz/Play.h>         // canonical CPlay (the ex-CCmdHandler identity)
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (CPlay::m_4)
#include <Gruntz/StatusBarMgr.h> // CStatusBarMgr::EnterHlRow (m_guts, +0x2dc)

static const char s_gameBadSelect[] = "GAME_BADSELECT";              // 0x612c28
static const char s_grunt[] = "Grunt";                               // 0x60a9ec
static const char s_playerDefenderRadius[] = "PlayerDefenderRadius"; // 0x60e1ac


// @early-stop
// Reconstructed against the REAL engine classes (xref-recovered, no fake views): the
// handler is CGruntzMgr, the world->m_68 grid is CTriggerMgr (PlaceObject/ClearCell/
// CellHitTest/ApplyTriggerA/ApplyTriggerB/ResetAll/ResetCell), the cells are CGrunt, and
// the movement target is CGrunt::SetArrivalTarget (called on the grunt g, NOT the handler
// - the prior this-receiver was a bug). De-hoisted to match retail's register discipline
// (localP + grid read lazily per-case so `world` stays in a reg across the switch;
// case-0 Refresh reloads g_gameReg->m_68; the address-taken CellHitTest outputs reuse
// the ret-0x1c'd &a4/&a8 param slots) + a permuter operand-order pass. 15.9%->~23.7%.
// Residual is a global-regalloc wall MSVC5 will not steer from C source: retail pins
// `this` in ebx and g in esi with NO frame, whereas the correct g-receiver makes cl
// contend g against this and park `this` in ebp + spill it + reserve a 4-B frame for the
// shared case-3/4 discriminator (retail emits cases 3/4 as two full separate blocks with
// no runtime discriminator; splitting them here regresses because MSVC5 tail-merges the
// identical suffixes). The 11-case logic + grunt-state resets + cell lookups are
// byte-faithful. Final-sweep permuter candidate (pure /O2 regalloc residue).
RVA(0x000d1b60, 0xc2f)
i32 CPlay::ExecCommand(char a2, char a3, char a4, i16 a5, i16 a6, char a7, char a8) {
    CGruntzMgr* mgr = m_mgr;
    if (mgr->m_frameGate != 0) {
        return 0;
    }
    i32 res;

    // grid is re-derived per case as this->m_4->m_cmdGrid: retail keeps `this` in a
    // callee-saved reg and re-reads m_4 inside each case (only case 0 reuses the
    // gate's cached `mgr` in eax); caching it across the whole switch would pin it
    // in a callee-saved reg and spill `this`. CSE collapses the repeats within a case.
    switch (static_cast<u8>(a4)) {
        default:
            return 1;

        case 0: {
            // case 0 reuses the gate's cached mgr (eax) for the spawn probe.
            i32 r = mgr->m_cmdGrid->PlaceObject(
                static_cast<u8>(a2),
                static_cast<u16>(a5),
                static_cast<u16>(a6),
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
                if (static_cast<u8>(a2) == static_cast<u32>(g_curPlayer)) {
                    // retail re-loads the grid from g_gameReg (0x64556c), not mgr.
                    g_gameReg->m_cmdGrid->ResetAll();
                }
                return 1;
            }
            if (m_world->m_soundRegistry->m_emitGate == 0) { // the sound host's busy/emit gate
                if (BadSelect(s_gameBadSelect) != 0) {
                    (reinterpret_cast<LeafCue*>(&g_sndCueTag))->PlayIfElapsed(0, 0, 0, 0);
                }
            }
            return 0;
        }

        case 2: {
            u32 player = static_cast<u8>(a2);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
            if (g != 0 && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ClearCell(player, static_cast<u8>(a3), static_cast<u16>(a5), static_cast<u16>(a6), 0);
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
            // Capture the command discriminator before PathProbe overwrites a4 (retail
            // threads the path-probe outputs back through the &a4/&a8 param slots).
            // Read bit 2 (set for cmd 4, clear for cmd 3) so it is NOT CSE'd with the
            // switch selector `a4 & 0xff` (which would spill the selector + add a frame).
            i32 isB = a4 & 4;
            u32 player = static_cast<u8>(a2);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            u32 px = static_cast<u16>(a5);
            u32 py = static_cast<u16>(a6);
            CGrunt* node = static_cast<CGrunt*>(m_mgr->m_cmdGrid->CellHitTest(px, py, reinterpret_cast<i32*>(&a4), reinterpret_cast<i32*>(&a8), 5));
            if (node == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
            } else {
                g->SetArrivalTarget(static_cast<i32>(player), px, node->m_object->m_screenX, node->m_object->m_screenY);
            }
            res = (isB == 0) ? m_mgr->m_cmdGrid->ApplyTriggerA(player, *reinterpret_cast<i32*>(&a4), *reinterpret_cast<i32*>(&a8), 0)
                             : m_mgr->m_cmdGrid->ApplyTriggerB(player, *reinterpret_cast<i32*>(&a4), *reinterpret_cast<i32*>(&a8), 0);
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
                res = m_mgr->m_cmdGrid
                          ->ClearCell(player, *reinterpret_cast<i32*>(&a4), *reinterpret_cast<i32*>(&a8), 0, (isB == 0) ? 2 : 3);
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
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
            u32 player = static_cast<u8>(a2);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_4f0 = 0;
            }
            i32 idx = static_cast<u8>(a3) + player * 0xf;
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
                r = PickupCheck(static_cast<u8>(a7), 0, 0, 0, g_gameReg->m_134 != 1);
            }
            i32 sel;
            if (r == 0) {
                sel = 0;
            } else {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, static_cast<u8>(a3), 0, 0);
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
            u32 player = static_cast<u8>(a2);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            u32 row = static_cast<u16>(a5), col = static_cast<u16>(a6);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_object;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, *reinterpret_cast<i32*>(&a7), row, 0);
            if (res != 0) {
                if (res == -1) {
                    res = m_mgr->m_cmdGrid->ClearCell(player, *reinterpret_cast<i32*>(&a8), *reinterpret_cast<i32*>(&a2), 0, 2);
                    if (res == 0) {
                        if (static_cast<u32>(g_curPlayer) != player || g->m_entranceCommitted == 0) {
                            return 0;
                        }
                        GruntCue(g, 0x324, -1, 0, -1, -1);
                        return 0;
                    }
                    if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (*reinterpret_cast<u32*>(&a4) != static_cast<u32>(g_curPlayer) && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                    return 1;
                }
                if (static_cast<u32>(g_curPlayer) != *reinterpret_cast<u32*>(&a8) && g->m_entranceCommitted != 0) {
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
            u32 player = static_cast<u8>(a2);
            CGrunt* g = m_mgr->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            u32 row = static_cast<u16>(a5), col = static_cast<u16>(a6);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_object;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, *reinterpret_cast<i32*>(&a7), row, 0);
            if (res != 0) {
                if (res != -1) {
                    if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (*reinterpret_cast<u32*>(&a8) != static_cast<u32>(g_curPlayer) && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = m_mgr->m_cmdGrid->ClearCell(player, *reinterpret_cast<i32*>(&a8), *reinterpret_cast<i32*>(&a2), 0, 3);
                if (res != 0) {
                    if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (static_cast<u32>(g_curPlayer) != *reinterpret_cast<u32*>(&a4) && g->m_entranceCommitted != 0) {
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
