// PlayerCommandStep.cpp - 0xd1b60, the player-command dispatch (CGruntzMgr-side).
// __thiscall(a2..a8) ret 0x1c, returns int. Switches on (a4 & 0xff) - the command
// code 0..10 - and applies it to the addressed grid grunt (m_4->m_68 slot grid,
// 15-wide rows): spawn-probe (0), move/attack/tool variants (2..5,9,10), select/
// deselect (6,7), and the conversion/pickup pre-pass (8). a2 = player id (== g_644c54
// is "local"), a3 = column, a5/a6 = pixel target or a second grid cell, a7/a8 spare.
// The grunt-state reset block (clear +0x308.. / +0x420 / mask +0x248) repeats across
// most cases. All engine helpers + the manager/registry globals are external
// (reloc-masked); the grunt/grid/this field bags are raw-offset addressed as retail.
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Ints.h>

#include <rva.h>
#include <Globals.h>
#include <Gruntz/LeafCue.h>    // canonical LeafCue (PlayIfElapsed_01f940)
#include <Gruntz/Grunt.h>      // canonical CGrunt (SetEntrancePos/SetArrivalTarget)
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (the world->m_68 tile-object grid)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

// .rodata string literals (were bare (char*)0xADDR immediates; named so the operand
// relocates like retail's `push offset` instead of an unrelocated `push imm32`).
static const char s_gameBadSelect[] = "GAME_BADSELECT";              // 0x612c28
static const char s_grunt[] = "Grunt";                               // 0x60a9ec
static const char s_playerDefenderRadius[] = "PlayerDefenderRadius"; // 0x60e1ac

// The world grid (m_4->m_68) is the real CTriggerMgr (TriggerMgr.h); its 15-wide
// placed-cell grid lives at +0x1c (stride 4), reached cell-by-cell (the header models
// the cells as opaque per-TU). GC(g)[idx] yields the CGrunt* cell. The per-command
// engine helpers are the real reloc-masked CTriggerMgr methods (recovered via xref):
//   PlaceObject 0x6b6d0, ClearCell 0x6e800, CellHitTest 0x6bea0, ApplyTriggerA 0x6dae0,
//   ApplyTriggerB 0x6e120, ResetAll 0x78430, ResetCell 0x6bfd0.
#define GC(g) ((CGrunt**)((char*)(g) + 0x1c))

// This handler object (CGruntzMgr): m_4 = world (->m_68 grid, +0xc gate), m_c chain.
struct CCmdHandler {
    i32 Dispatch(u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8); // 0xd1b60
    void NotifySelect(i32 a);                                             // 0x4017a8
    void Defended(i32 a, i32 b);                                          // 0x40213f
};

// Bute-config manager (g_buteMgr @ VA 0x6453d8 -> RVA 0x2453d8): read the defender-radius
// value via the canonical CButeMgr::GetIntDef (0x171aa0, include/Bute/ButeMgr.h).
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The missed-select complaint cue lives at 0x61ab24 (the engine ?g_sndCueTag@@3HA int;
// its address is the LeafCue the Complain path fires).
DATA(0x0021ab24)
extern i32 g_sndCueTag;

DATA(0x00244c54)
extern i32 g_localPlayer; // g_644c54
DATA(0x0024556c)
extern "C" char* g_gameReg; // ->m_134

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
i32 CCmdHandler::Dispatch(u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
    char* world = P(this, 4);
    if (F(world, 0xc) != 0) {
        return 0;
    }
    i32 res;

// grid is re-derived per case as this->m_4->m_68: retail keeps `this` in a callee-
// saved reg and re-reads m_4 inside each case (only case 0 reuses the gate's cached
// `world` in eax); caching `world` across the whole switch would pin it in a callee-
// saved reg and spill `this`. CSE collapses the repeats within a case.
#define grid ((CTriggerMgr*)P(P(this, 4), 0x68))
    switch (a4 & 0xff) {
        default:
            return 1;

        case 0: {
            // case 0 reuses the gate's cached world (eax) for the spawn probe.
            i32 r = ((CTriggerMgr*)P(world, 0x68))
                        ->PlaceObject(
                            a2 & 0xff,
                            a5 & 0xffff,
                            a6 & 0xffff,
                            100000,
                            2,
                            (i32)g_renderCtx,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0
                        );
            if (r != -1) {
                if ((a2 & 0xff) == (u32)g_localPlayer) {
                    // retail re-loads the grid from g_gameReg (0x64556c), not world.
                    ((CTriggerMgr*)P(g_gameReg, 0x68))->ResetAll();
                }
                return 1;
            }
            if (F(F(P(this, 0xc), 0x28), 0x30) == 0) {
                if (BadSelect(s_gameBadSelect) != 0) {
                    ((LeafCue*)&g_sndCueTag)->PlayIfElapsed_01f940(0, 0, 0, 0);
                }
            }
            return 0;
        }

        case 2: {
            a2 &= 0xff;
            CGrunt* g = GC(grid)[(a3 & 0xff) + a2 * 0xf];
            if (g != 0 && F(g, 0x1fc) != 0) {
                F(g, 0x230) = 0;
            }
            res = grid->ClearCell(a2, a3 & 0xff, a5 & 0xffff, a6 & 0xffff, 0);
            if (res != 0) {
                if (a2 != (u32)g_localPlayer) {
                    return 1;
                }
                if (g != 0 && F(g, 0x1fc) != 0) {
                    GruntCue(g, 0x323, -1, 0, -1, -1);
                }
                return 1;
            }
            if (a2 != (u32)g_localPlayer || g == 0 || F(g, 0x1fc) == 0) {
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
            u32 player = a2 & 0xff;
            CGrunt* g = GC(grid)[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0) {
                return 0;
            }
            if (isB != 0 && F(g, 0x1e4) != 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            a6 &= 0xffff;
            a5 &= 0xffff;
            char* node = (char*)grid->CellHitTest(a5, a6, (i32*)&a4, (i32*)&a8, 5);
            if (node == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
            } else {
                g->SetArrivalTarget(
                    (i32)player,
                    a5,
                    F(P(node, 0x10), 0x5c),
                    F(P(node, 0x10), 0x60)
                );
            }
            res = (isB == 0) ? grid->ApplyTriggerA(player, a4, a8, 0)
                             : grid->ApplyTriggerB(player, a4, a8, 0);
            if (res != 0) {
                if (res != -1) {
                    if (player != (u32)g_localPlayer) {
                        return 1;
                    }
                    if (F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = grid->ClearCell(player, a4, a8, 0, (isB == 0) ? 2 : 3);
                if (res != 0) {
                    if (player != (u32)g_localPlayer) {
                        return 1;
                    }
                    if (F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (player != (u32)g_localPlayer || F(g, 0x1fc) == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != (u32)g_localPlayer) {
                return 0;
            }
            res = F(g, 0x1fc);
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 5: {
            CGrunt* g = GC(grid)[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g == 0 || F(g, 0x1fc) == 0 || F(g, 0x1e4) != 0) {
                return 0;
            }
            g->SetEntrancePos(1, 1);
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            return 1;
        }

        case 6: {
            CGrunt* g = GC(grid)[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g != 0) {
                if (F(g, 0x420) != 1) {
                    F(g, 0x308) = 0;
                    F(g, 0x310) = 0;
                    F(g, 0x30c) = 0;
                    F(g, 0x314) = 0;
                    F(g, 0x300) = F(g, 0x17c);
                    F(g, 0x420) = 1;
                    F(g, 0x304) = F(g, 0x180);
                    switch (F(g, 0x170)) {
                        case 2:
                        case 9:
                        case 10:
                        case 0xb:
                        case 0x15:
                        case 0x16:
                            F(g, 0x2dc) = 1;
                            break;
                        default:
                            F(g, 0x2dc) =
                                g_buteMgr.GetIntDef(s_grunt, s_playerDefenderRadius, 3) + 1;
                    }
                    F(g, 0x248) |= 0x18040402;
                    F(g, 0x2f0) = -1;
                    F(g, 0x2d0) = 4;
                    F(g, 0x2d4) = 0;
                    F(g, 0x2f4) = -1;
                    F(g, 0x230) = 0;
                    F(P(g, 0x10), 0x134) = 0;
                    F(P(g, 0x10), 0x13c) = 0;
                    F(P(g, 0x10), 0x138) = 0;
                    F(P(g, 0x10), 0x140) = 0;
                    g->SetEntrancePos(1, 1);
                }
                F(g, 0x464) = 0;
            }
            return 1;
        }

        case 7: {
            CGrunt* g = GC(grid)[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g == 0 || F(g, 0x420) == 0) {
                return 1;
            }
            F(g, 0x308) = 0;
            F(g, 0x310) = 0;
            F(g, 0x30c) = 0;
            F(g, 0x314) = 0;
            F(g, 0x420) = 0;
            F(g, 0x2d0) = 0;
            F(g, 0x248) &= 0xe7fbfbfd;
            g->SetEntrancePos(1, 1);
            return 1;
        }

        case 8: {
            a2 &= 0xff;
            if (a2 == (u32)g_localPlayer) {
                F(this, 0x4f0) = 0;
            }
            i32 idx = (a3 & 0xff) + a2 * 0xf;
            CGrunt* g = GC(grid)[idx];
            if (g != 0 && F(g, 0x1fc) != 0 && F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            CGrunt* g2 = GC((CTriggerMgr*)P(P(this, 4), 0x68))[idx];
            i32 r;
            if (g2 == 0 || F(g2, 0x1fc) == 0) {
                r = 0;
            } else {
                r = PickupCheck(a7 & 0xff, 0, 0, 0, F(g_gameReg, 0x134) != 1);
            }
            i32 sel;
            if (r == 0) {
                sel = 0;
            } else {
                if (a2 == (u32)g_localPlayer) {
                    grid->ResetCell(a2, a3 & 0xff, 0, 0);
                }
                sel = 1;
            }
            if (a2 == (u32)g_localPlayer) {
                F(this, 0x36c) = 0;
                Defended(sel, F(this, 0x2f4));
                NotifySelect(0);
            }
            return r;
        }

        case 9: {
            u32 player = a2 & 0xff;
            CGrunt* g = GC(grid)[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            u32 row = a5 & 0xffff, col = a6 & 0xffff;
            CGrunt* g2 = GC((CTriggerMgr*)P(P(this, 4), 0x68))[col + row * 0xf];
            if (g2 == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
                return 0;
            }
            char* m10 = P(g2, 0x10);
            g->SetArrivalTarget(row, col, F(m10, 0x5c), F(m10, 0x60));
            res = grid->ApplyTriggerA(player, a7, row, 0);
            if (res != 0) {
                if (res == -1) {
                    res = grid->ClearCell(player, a8, a2, 0, 2);
                    if (res == 0) {
                        if ((u32)g_localPlayer != player || F(g, 0x1fc) == 0) {
                            return 0;
                        }
                        GruntCue(g, 0x324, -1, 0, -1, -1);
                        return 0;
                    }
                    if ((a2 & 0xff) != (u32)g_localPlayer) {
                        return 1;
                    }
                    if (a4 != (u32)g_localPlayer && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if ((a2 & 0xff) != (u32)g_localPlayer) {
                    return 1;
                }
                if ((u32)g_localPlayer != a8 && F(g, 0x1fc) != 0) {
                    GruntCue(g, 0x325, -1, 0, -1, -1);
                }
                return 1;
            }
            if (player != (u32)g_localPlayer) {
                return 0;
            }
            res = F(g, 0x1fc);
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 10: {
            u32 player = a2 & 0xff;
            CGrunt* g = GC(grid)[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0 || F(g, 0x1e4) != 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            u32 row = a5 & 0xffff, col = a6 & 0xffff;
            CGrunt* g2 = GC((CTriggerMgr*)P(P(this, 4), 0x68))[col + row * 0xf];
            if (g2 == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
                return 0;
            }
            char* m10 = P(g2, 0x10);
            g->SetArrivalTarget(row, col, F(m10, 0x5c), F(m10, 0x60));
            res = grid->ApplyTriggerB(player, a7, row, 0);
            if (res != 0) {
                if (res != -1) {
                    if ((a2 & 0xff) != (u32)g_localPlayer) {
                        return 1;
                    }
                    if (a8 != (u32)g_localPlayer && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = grid->ClearCell(player, a8, a2, 0, 3);
                if (res != 0) {
                    if ((a2 & 0xff) != (u32)g_localPlayer) {
                        return 1;
                    }
                    if ((u32)g_localPlayer != a4 && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if ((u32)g_localPlayer != player || F(g, 0x1fc) == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != (u32)g_localPlayer) {
                return 0;
            }
            res = F(g, 0x1fc);
            break;
        }
    }

    if (res == 0) {
        return 0;
    }
    return 0;
}
#undef grid
SIZE_UNKNOWN(CCmdHandler);
