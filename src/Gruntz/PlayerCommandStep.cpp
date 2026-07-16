// PlayerCommandStep.cpp - CPlay::ExecCommand @0xd1b60, the player-command
// executor. __thiscall(a2..a8) ret 0x1c, returns int. Switches on (a4 & 0xff) -
// the command code 0..10 - and applies it to the addressed grid grunt
// (m_4->m_cmdGrid slot grid, 15-wide rows): spawn-probe (0), move/attack/tool
// variants (2..5,9,10), select/deselect (6,7), and the conversion/pickup
// pre-pass (8). a2 = player id (== g_curPlayer is "local"), a3 = column, a5/a6 =
// pixel target or a second grid cell, a7/a8 spare.
//
// ::CPlay - the two retail callers (CGruntzCommand::ApplyOne/ApplyMask via thunk
// 0x21e4) hand it the CGruntzCmdTarget, and the body's own fields prove the play
// state: m_4 the CGruntzMgr (its +0x0c frameGate / +0x68 cmdGrid), m_c the world
// holder (its m_28 CSndHost +0x30 busy gate), m_2f4 m_cursorFrame, m_2dc m_guts,
// m_36c m_dragInhibit2, m_4f0 the highlight-busy gate - and its two helper thunks
// are real methods on those receivers: 0x17a8 -> CPlay::SetCursorFrame @0xd1b30
// (ecx=this @0xd26d3), 0x213f -> CStatusBarMgr::EnterHlRow @0x106820
// (ecx=[this+0x2dc] @0xd26c6). The grunt-state reset block (clear
// m_arrivalReroll* / m_tileClaimed / m_arrivalState / mask m_arrivalFlags)
// repeats across most cases. All engine helpers are external (reloc-masked).
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/GameRegPtr.h>
#include <Ints.h>

#include <rva.h>
#include <Globals.h>
#include <Gruntz/LeafCue.h>      // canonical LeafCue (PlayIfElapsed)
#include <Gruntz/Grunt.h>        // canonical CGrunt (SetEntrancePos/SetArrivalTarget)
#include <Gruntz/TriggerMgr.h>   // canonical CTriggerMgr (the mgr's m_cmdGrid grid)
#include <Gruntz/Play.h>         // canonical CPlay (the ex-CCmdHandler identity)
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (CPlay::m_4)
#include <Gruntz/StatusBarMgr.h> // CStatusBarMgr::EnterHlRow (m_guts, +0x2dc)

// .rodata string literals (were bare (char*)0xADDR immediates; named so the operand
// relocates like retail's `push offset` instead of an unrelocated `push imm32`).
static const char s_gameBadSelect[] = "GAME_BADSELECT";              // 0x612c28
static const char s_grunt[] = "Grunt";                               // 0x60a9ec
static const char s_playerDefenderRadius[] = "PlayerDefenderRadius"; // 0x60e1ac

// The world grid (m_4->m_cmdGrid) is the real CTriggerMgr (TriggerMgr.h); its
// 15-wide placed-cell grid is the typed m_grid[0x3c] (cells = CGrunt). The
// per-command engine helpers are the real reloc-masked CTriggerMgr methods:
//   PlaceObject 0x6b6d0, ClearCell 0x6e800, CellHitTest 0x6bea0, ApplyTriggerA
//   0x6dae0, ApplyTriggerB 0x6e120, ResetAll 0x78430, ResetCell 0x6bfd0.

// Bute-config manager (g_buteMgr @ VA 0x6453d8 -> RVA 0x2453d8): read the defender-radius
// value via the canonical CButeMgr::GetIntDef (0x171aa0); g_buteMgr from <Bute/ButeMgr.h>.

// The missed-select complaint cue lives at 0x61ab24 (the engine ?g_sndCueTag@@3HA int;
// its address is the LeafCue the Complain path fires).


// Free engine helpers (reloc-masked).
extern "C" {
    void __stdcall GruntCue(CGrunt* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4
    i32 BadSelect(const char* msg);                                           // 0x402cca (__cdecl)
    i32 PickupCheck(i32 a, i32 b, i32 c, i32 d, i32 e);                       // 0x403c6a (__cdecl)
}

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
    CGruntzMgr* mgr = m_4;
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
            if (m_c->m_soundRegistry->m_30 == 0) { // the sound host's busy/emit gate
                if (BadSelect(s_gameBadSelect) != 0) {
                    ((LeafCue*)&g_sndCueTag)->PlayIfElapsed(0, 0, 0, 0);
                }
            }
            return 0;
        }

        case 2: {
            u32 player = static_cast<u8>(a2);
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
            if (g != 0 && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            res = m_4->m_cmdGrid->ClearCell(player, static_cast<u8>(a3), static_cast<u16>(a5), static_cast<u16>(a6), 0);
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            CGrunt* node = (CGrunt*)m_4->m_cmdGrid->CellHitTest(px, py, (i32*)&a4, (i32*)&a8, 5);
            if (node == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
            } else {
                g->SetArrivalTarget(static_cast<i32>(player), px, node->m_10->m_screenX, node->m_10->m_screenY);
            }
            res = (isB == 0) ? m_4->m_cmdGrid->ApplyTriggerA(player, *(i32*)&a4, *(i32*)&a8, 0)
                             : m_4->m_cmdGrid->ApplyTriggerB(player, *(i32*)&a4, *(i32*)&a8, 0);
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
                res = m_4->m_cmdGrid
                          ->ClearCell(player, *(i32*)&a4, *(i32*)&a8, 0, (isB == 0) ? 2 : 3);
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
                    g->m_10->m_extentL = 0;
                    g->m_10->m_extentR = 0;
                    g->m_10->m_extentT = 0;
                    g->m_10->m_extentB = 0;
                    g->SetEntrancePos(1, 1);
                }
                g->m_arrivalNotified = 0;
            }
            return 1;
        }

        case 7: {
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a2) * 0xf + static_cast<u8>(a3)];
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[idx];
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
            CGrunt* g2 = m_4->m_cmdGrid->m_grid[idx];
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
                    m_4->m_cmdGrid->ResetCell(player, static_cast<u8>(a3), 0, 0);
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            CGrunt* g2 = m_4->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_10;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_4->m_cmdGrid->ApplyTriggerA(player, *(i32*)&a7, row, 0);
            if (res != 0) {
                if (res == -1) {
                    res = m_4->m_cmdGrid->ClearCell(player, *(i32*)&a8, *(i32*)&a2, 0, 2);
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
                    if (*(u32*)&a4 != static_cast<u32>(g_curPlayer) && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                    return 1;
                }
                if (static_cast<u32>(g_curPlayer) != *(u32*)&a8 && g->m_entranceCommitted != 0) {
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
            CGrunt* g = m_4->m_cmdGrid->m_grid[static_cast<u8>(a3) + player * 0xf];
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
            CGrunt* g2 = m_4->m_cmdGrid->m_grid[col + row * 0xf];
            if (g2 == 0 || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            CGameObject* m10 = g2->m_10;
            g->SetArrivalTarget(row, col, m10->m_screenX, m10->m_screenY);
            res = m_4->m_cmdGrid->ApplyTriggerB(player, *(i32*)&a7, row, 0);
            if (res != 0) {
                if (res != -1) {
                    if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (*(u32*)&a8 != static_cast<u32>(g_curPlayer) && g->m_entranceCommitted != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = m_4->m_cmdGrid->ClearCell(player, *(i32*)&a8, *(i32*)&a2, 0, 3);
                if (res != 0) {
                    if (static_cast<u8>(a2) != static_cast<u32>(g_curPlayer)) {
                        return 1;
                    }
                    if (static_cast<u32>(g_curPlayer) != *(u32*)&a4 && g->m_entranceCommitted != 0) {
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
