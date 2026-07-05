#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>           // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/SoundCueMgr.h>            // the ONE CSoundCueMgr shape (ConfigureItem @0x1360d0)
#include <Gruntz/Sprite.h>                 // CSprite (frame-data value) + CSpriteHashTable
#include <Gruntz/StatusBarUpdatersViews.h> // referent views + EngineLabelBacklog host

#include <math.h> // sqrt - intrinsified to inline fsqrt under VC5 /O2
// StatusBarUpdaters.cpp - the per-widget HUD status-bar updaters and switch-tile
// sprite loaders (C:\Proj\Gruntz). They live on the big in-game game-mode object
// (the EngineLabelBacklog placeholder the rest of the backlog hangs off) and
// share two engine idioms:
//
//   * the elapsed-time clamp: a 64-bit `g_645588 - tab->ts` clamped at 0, then
//     divided by a CButeMgr-configured delay to drive an animation frame index;
//   * the "advance status-bar tab" tail: a named-sprite Lookup through the global
//     status-bar mgr (g_gameReg->m_world->m_statusBar->m_10map -> CSpriteHashTable::Lookup),
//     a draw-clock window check (g_6bf3c0 - t->m_drawClock >= t->m_window), and a
//     CStatusBarMgr::ConfigureItem push (the shared 0x1360d0 helper, reloc-masked).
//
// Only offsets / code bytes are load-bearing; names are placeholders.

// The frame clock + draw-clock mirror globals (canonical in CPlay.h / surfacemgr).
extern "C" {
    extern u32 g_645588; // the running game clock
    extern u32 g_6bf3c0; // draw-clock mirror
}

// The two paired status-bar globals the advance tail reads (external delinked
// DATA symbols, reloc-masked): g_61ab20 gates the push, g_61ab24 is the value.
extern i32 g_61ab20; // DAT_0061ab20
extern i32 g_61ab24; // DAT_0061ab24

// g_buteMgr - the attribute manager singleton (butemgr unit).
extern CButeMgr g_buteMgr;

// g_644c54 - a level/group base index the StatzTab toggle keys off (CPlay.h).
extern i32 g_644c54;

// CSprite (frame-data value) + CSpriteHashTable now come from <Gruntz/Sprite.h>.

// CSoundCueMgr::ConfigureItem (the shared push helper @0x1360d0, external/no-body so
// the `call rel32` reloc-masks) is modeled in <Gruntz/SoundCueMgr.h>.

// CStatusBarTab/CStatusBarHolder/CMapTileGrid/CRegHolder moved to
// <Gruntz/StatusBarUpdatersViews.h>.
// The canonical CGameRegistry view of the singleton (*0x24556c). The resource
// holder (+0x30 -> CRegHolder) and group table (+0x68) are void*/CResMgr* in the
// canonical layout, so this TU casts them locally at the deref sites; the tile
// notifier (+0x70) is the canonical CTileGrid (Notify facet), reached without a
// cast, and the view-bounds rectangle scalars (+0x13c..+0x148) match directly.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // the game-manager singleton

// CTabRec/CTabWidget/CDestructBlock/CGrinderRect/CStatzTabItem/EngineLabelBacklog/
// CStatzTabSub moved to <Gruntz/StatusBarUpdatersViews.h>.

// ===========================================================================
// EngineLabelBacklog::UpdateGruntOvenStatusBar @0x105310
// ===========================================================================
//
// Walks the 5 grunt-oven cooking tabs: while a tab is COOKING (m_state==1) it derives
// the cooking-progress frame index from the elapsed clock / GruntOvenDelay, caps
// at 0x1a (completion - flips m_state to 2 and runs the COOKINGCOMPLETE advance), and
// pushes the new frame into the widget when it changes (the +0x30 virtual).
// @early-stop
// ~79.9%: logic + every store/offset/advance-tail is byte-faithful. Residual is the
// 64-bit signed-clamp `elapsed = (d>=0)?(i32)d:0` codegen: retail emits the un-folded
// `cmp hi,ebx(0); jg; jl; cmp lo,ebx; jae` compare then a branch-select `xor esi,esi;
// jmp / mov esi,eax(lo)` keeping the raw lo in a temp, whereas this toolchain FOLDS it
// to the sbb sign-flag (`js`) and fuses lo directly into the elapsed reg (esi) - i.e.
// cl here is MORE optimized than retail's exact MSVC5 build. A toolchain-microversion
// codegen wall, not source-steerable; deferred to the final sweep.
RVA(0x00105310, 0x11a)
void EngineLabelBacklog::UpdateGruntOvenStatusBar() {
    CTabWidget** slot = m_slots;
    CTabRec* tab = m_tabs;
    i32 n = 5;
    do {
        if (tab->m_state == 1) {
            i64 d = (i64)(u32)g_645588 - *(i64*)&tab->m_startLo;
            i32 elapsed = (d >= 0) ? (i32)d : 0;
            u32 delay = g_buteMgr.GetDwordDef("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = (i32)((u32)elapsed / delay) + 1;
            if (frame >= 0x1a) {
                tab->m_state = 2;
                frame = 0x1a;
                CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
                if (h->m_surfaceGate == 0) {
                    CSprite* spr = 0;
                    h->m_10map.Lookup("GAME_COOKINGCOMPLETE", &spr);
                    if (spr) {
                        CStatusBarTab* t = (CStatusBarTab*)spr;
                        if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                            t->m_drawClock = g_6bf3c0;
                            t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
                        }
                    }
                }
            }
            if (frame != tab->m_frame) {
                tab->m_frame = frame;
                CTabWidget* w = *slot;
                if (w) {
                    w->SetFrame(frame);
                }
            }
        }
        ++slot;
        ++tab;
    } while (--n != 0);
}

// ===========================================================================
// EngineLabelBacklog::UpdateDestructButtonStatusBar @0x10b320
// ===========================================================================
//
// The destruct-button warning blinker: in state 1 it counts the warning frame UP
// toward 6 (then latches state 2), in state 2 it counts DOWN toward 2 (then
// latches state 1); each step is gated on the retrigger clock having elapsed past
// DestructButtonWarningDelay, after which the 64-bit retrigger clock is restamped
// and the new frame pushed into the widget (the +0x30 virtual). State 0 = idle.
// @early-stop
// ~94.7%: logic + the 64-bit compare + every field store are byte-exact. The sole
// residual (in BOTH symmetric cases) is a store-scheduling coin-flip: retail emits
// `mov [+560],retriggerLo; mov [+564],0; mov ecx,[+570](widget)` in source order,
// while MSVC's scheduler hoists the widget load between the two retrigger stores and
// defers the `m_retriggerHi=0` store past it. Tried a single 64-bit retrigger store
// and an inlined widget test; neither pins the store order. Not source-steerable;
// deferred to the final sweep.
RVA(0x0010b320, 0x167)
void EngineLabelBacklog::UpdateDestructButtonStatusBar() {
    CDestructBlock* b = &m_destruct;
    switch (b->m_state) {
        case 1: {
            i64 d = (i64)(u32)g_645588 - *(i64*)&b->m_retriggerLo;
            if (d >= *(i64*)&b->m_delayLo) {
                if (++b->m_frame >= 6) {
                    b->m_frame = 6;
                    b->m_state = 2;
                }
                b->m_delayLo =
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                b->m_delayHi = 0;
                b->m_retriggerLo = g_645588;
                b->m_retriggerHi = 0;
                CTabWidget* w = b->m_widget;
                if (w) {
                    w->SetFrame(b->m_frame);
                }
            }
            break;
        }
        case 2: {
            i64 d = (i64)(u32)g_645588 - *(i64*)&b->m_retriggerLo;
            if (d >= *(i64*)&b->m_delayLo) {
                if (--b->m_frame <= 2) {
                    b->m_frame = 2;
                    b->m_state = 1;
                }
                b->m_delayLo =
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                b->m_delayHi = 0;
                b->m_retriggerLo = g_645588;
                b->m_retriggerHi = 0;
                CTabWidget* w = b->m_widget;
                if (w) {
                    w->SetFrame(b->m_frame);
                }
            }
            break;
        }
    }
}

// ===========================================================================
// EngineLabelBacklog::UpdateChipGrinderStatusBar @0x1076a0
// ===========================================================================
//
// Drives the rez chip-grinder conveyor while it is RUNNING (m_4e8 != 0): it pulls
// the FallingItem delay/speed (then the ShredderDelay/Speed once the conveyor
// reaches the shredder at m_510 >= 0x1bf, where it also runs the one-shot
// REZGRINDING status-bar advance and flips to phase 2), advances the two conveyor
// extents (m_508/m_510) by the speed each time the retrigger clock elapses, and
// re-stamps the grinder rect-target widget (m_500) from the scroll origin. When
// the conveyor runs out (m_508 >= 0x1c7) it stops (m_4e8 = 0). A final
// ChipGrinderFinishStep runs while the widget is live and a step happened.
// @early-stop
// ~80.7%: instruction count matches retail exactly (164==164); logic + all offsets/
// stores/advance-tail are byte-faithful. Residual is a pervasive zero-register-pinning
// role swap (docs/patterns/zero-register-pinning.md): retail pins 0 in ebx and the
// phase-constant 3 in edi, this toolchain pins 0 in edi + spills a second zero into
// ebp - a 1-instr phase shift cascading through every `mov [field],0` store. A
// regalloc coin-flip, not source-steerable; deferred to the final sweep.
RVA(0x001076a0, 0x1f3)
void EngineLabelBacklog::UpdateChipGrinderStatusBar() {
    i32* m = (i32*)this;
    if (m[0x4e8 / 4] == 0) {
        return;
    }

    i32 stepped = 0;
    if (m[0x4e8 / 4] == 1 || m[0x4e8 / 4] == 2) {
        u32 delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 4);

        if (m[0x508 / 4] >= 0x1c7) {
            m[0x4e8 / 4] = 0;
            m[0x4ec / 4] = 0;
        } else if (m[0x510 / 4] >= 0x1bf) {
            if (m[0x4e8 / 4] != 2) {
                if (m[0x10c / 4] == 3 && m[0] != 2) {
                    CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
                    if (h->m_surfaceGate == 0) {
                        CSprite* spr = 0;
                        h->m_10map.Lookup("GAME_REZGRINDING", &spr);
                        if (spr) {
                            CStatusBarTab* t = (CStatusBarTab*)spr;
                            if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                                t->m_drawClock = g_6bf3c0;
                                t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
                            }
                        }
                    }
                }
                m[0x4e8 / 4] = 2;
            }
            delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemShredderSpeed", 2);
        }

        i64 d = (i64)(u32)g_645588 - *(i64*)&m[0x4f0 / 4];
        if (d >= *(i64*)&m[0x4f8 / 4]) {
            i32 newLo = m[0x508 / 4] + speed;
            m[0x508 / 4] = newLo;
            i32 newHi = m[0x510 / 4] + speed;
            m[0x510 / 4] = newHi;
            CGrinderRect* w = (CGrinderRect*)m[0x500 / 4];
            if (w) {
                i32 sx = m[0x10 / 4];
                i32 sy = m[0x14 / 4];
                i32* p = &w->m_left;
                p[0] = m[0x504 / 4] + sx;
                p[1] = sy + newLo;
                p[2] = m[0x50c / 4] + sx;
                p[3] = sy + newHi;
            }
            m[0x4f8 / 4] = delay;
            m[0x4fc / 4] = 0;
            m[0x4f0 / 4] = g_645588;
            m[0x4f4 / 4] = 0;
            stepped = 1;
        }
    }

    if (m[0x500 / 4] != 0 && stepped) {
        ChipGrinderFinishStep();
    }
}

// ===========================================================================
// EngineLabelBacklog::LoadStatzTabToggleSprite @0x104e60
// ===========================================================================
//
// Toggles the per-statz-tab indicator `idx` to `value`: a no-op if it already
// holds `value`; otherwise, gated on the tab's group-record being live, it stamps
// the toggle item (this[idx]+0x150), kicks the tab sub-helper when the view mode
// is 3, runs the STATZTABTOGGLE status-bar advance, and latches the new value.
// __thiscall ret 8. Always returns 1.

// RegUnitTable moved to <Gruntz/StatusBarUpdatersViews.h>.
// @early-stop
// ~80.8%: logic + offsets + the advance-tail are byte-faithful. Residual is a
// constant/register-pinning coin-flip: retail keeps a 4th callee-saved reg (ebp) live
// and PINS the constant 1 in ecx, reusing it for `item->m_active=1`, the `==1` gate
// and the Toggle(...,1) arg (`mov ecx,1; ... push ecx`); this toolchain uses fewer
// registers and emits the 1 as inline immediates instead. Already spelled with a
// shared `i32 one=1` local, which MSVC5 declines to keep in a register - a regalloc
// pressure coin-flip, not source-steerable; deferred to the final sweep.
RVA(0x00104e60, 0xed)
i32 EngineLabelBacklog::LoadStatzTabToggleSprite(i32 value, i32 idx) {
    i32* m = (i32*)this;
    if (m[idx + 0x114 / 4] == value) {
        return 1;
    }

    i32 slot = idx + 15 * g_644c54;
    // m_68 is the registry's poly per-mode slot (void* in the shared view); in the
    // in-game status-bar context it is always the unit-record table. One authentic
    // downcast to the concrete view, then cast-free field access.
    RegUnitTable* units = (RegUnitTable*)g_gameReg->m_cmdGrid;
    if (units->m_slots[slot] == 0) {
        return 0;
    }

    CStatzTabItem* item = (CStatzTabItem*)m[idx + 0x150 / 4];
    i32 one = 1;
    if (item) {
        item->m_toggleValue = value;
        item->m_active = one;
        if (m[0x10c / 4] == one) {
            ((CStatzTabSub*)m[idx + 0x18c / 4])->Toggle(m[0], one);
            CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
            if (h->m_surfaceGate == 0) {
                CSprite* spr = 0;
                h->m_10map.Lookup("GAME_STATZTABTOGGLE", &spr);
                if (spr) {
                    CStatusBarTab* t = (CStatusBarTab*)spr;
                    if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                        t->m_drawClock = g_6bf3c0;
                        t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
                    }
                }
            }
        }
    }
    m[idx + 0x114 / 4] = value;
    return 1;
}

// ===========================================================================
// EngineLabelBacklog::LoadSwitchDownSprite @0x110570
// ===========================================================================
//
// Drives a tile switch into its DOWN state: it bumps the switch tile's cell-state
// counter in the map grid (grid->m_20[grid->m_24[m_switchTileY] + m_switchTileX]) and notifies the
// tile system, then - if the switch tile is on-screen (its pixel rect inside the
// view bounds) and the status bar surface is live - runs the GAME_SWITCHDOWN
// status-bar advance. Latches m_14 = 1 (down). __thiscall, returns 1.
// @early-stop
// ~72% CSE/regalloc wall: the int(1) return (was void) is now correct, but retail
// RE-DERIVES `g_gameReg->m_world->m_tileHolder->m_grid` for the store leg (pinning
// g_gameReg in edi), while our cl CSEs the two identical grid chains into one eax
// (keeping grid, not g_gameReg, in a callee-saved reg). The whole register layout
// cascades from that CSE choice. No source spelling defeats MSVC5's CSE of the two
// identical multi-level loads without an intervening store. Logic byte-correct.
RVA(0x00110570, 0xfb)
i32 EngineLabelBacklog::LoadSwitchDownSprite() {
    CMapTileGrid* g = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    i32 v = g->m_cellState[g->m_rowOffset[m_switchTileY] + m_switchTileX] + 1;
    CMapTileGrid* g2 = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    g2->m_cellState[g2->m_rowOffset[m_switchTileY] + m_switchTileX] = v;
    g_gameReg->m_tileGrid->Notify(m_switchTileX, m_switchTileY, v);

    i32 px = (m_switchTileX << 5) + 0x10;
    i32 py = (m_switchTileY << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
        if (h->m_surfaceGate == 0) {
            CSprite* spr = 0;
            h->m_10map.Lookup("GAME_SWITCHDOWN", &spr);
            if (spr) {
                CStatusBarTab* t = (CStatusBarTab*)spr;
                if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                    t->m_drawClock = g_6bf3c0;
                    t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
                }
            }
        }
    }
    m_switchState = 1;
    return 1;
}

// ===========================================================================
// EngineLabelBacklog::LoadSwitchUpSprite @0x1106b0
// ===========================================================================
//
// The UP mirror of LoadSwitchDownSprite: decrements the cell-state counter, runs
// the GAME_SWITCHUP advance, and latches m_14 = 0 (up). __thiscall, returns 1.
// @early-stop
// ~70% CSE/regalloc wall (same as LoadSwitchDownSprite): int(1) return now correct;
// residual is the grid-chain CSE that pins grid instead of g_gameReg. Logic exact.
RVA(0x001106b0, 0xf4)
i32 EngineLabelBacklog::LoadSwitchUpSprite() {
    CMapTileGrid* g = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    i32 v = g->m_cellState[g->m_rowOffset[m_switchTileY] + m_switchTileX] - 1;
    CMapTileGrid* g2 = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    g2->m_cellState[g2->m_rowOffset[m_switchTileY] + m_switchTileX] = v;
    g_gameReg->m_tileGrid->Notify(m_switchTileX, m_switchTileY, v);

    i32 px = (m_switchTileX << 5) + 0x10;
    i32 py = (m_switchTileY << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
        if (h->m_surfaceGate == 0) {
            CSprite* spr = 0;
            h->m_10map.Lookup("GAME_SWITCHUP", &spr);
            if (spr) {
                CStatusBarTab* t = (CStatusBarTab*)spr;
                if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                    t->m_drawClock = g_6bf3c0;
                    t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
                }
            }
        }
    }
    m_switchState = 0;
    return 1;
}

// ===========================================================================
// EngineLabelBacklog::UpdateWarpStoneStatusBar @0x109bd0
// ===========================================================================
//
// Sets up the warp-stone "fly" animation toward the warp tab. It records arg0 at
// m_3c, resolves the frame for (phase+1) out of the GAME_STATUSBAR_TABZ_GAMETAB_WARP
// sprite (m_38), and on success computes the screen target (m_4/m_8) for the phase
// (a per-phase pixel offset off the tab base m_3c->m_10/m_14), the euclidean
// distance to the source (srcX/srcY), and the per-axis fly velocity scaled by
// FlyTime, then runs the GAME_WARPSTONEFLY status-bar advance. __thiscall ret 0x10.
// @early-stop
// ~81.2%: logic + the sqrt/fly-velocity FP block + the advance-tail are byte-faithful.
// Residuals are two regalloc/scheduling coin-flips: (1) the prologue orders the
// `mov [esp+X],0` stack-init vs the `lea ecx,[esp+X]` differently, and (2) the frame
// lookup `(spr && n in range) ? spr->m_frames[n] : 0` keeps the loaded pointer in a
// temp (eax) and branch-selects into edi in retail, where this toolchain fuses the
// load directly into edi (`mov edi,[ecx+4*edi]`). Same select-register-fusion family
// as the 64-bit clamp; not source-steerable; deferred to the final sweep.
RVA(0x00109bd0, 0x1b5)
i32 EngineLabelBacklog::UpdateWarpStoneStatusBar(i32 a0, i32 phase, i32 srcX, i32 srcY) {
    i32* m = (i32*)this;
    m[0x3c / 4] = a0;

    CSprite* spr = 0;
    i32 n = phase + 1;
    ((CRegHolder*)g_gameReg->m_world)
        ->m_statusBar->m_10map.Lookup("GAME_STATUSBAR_TABZ_GAMETAB_WARP", &spr);
    i32* frame =
        (spr && n >= spr->m_firstFrame && n <= spr->m_lastFrame) ? spr->m_frames.m_pData[n] : 0;
    m[0x38 / 4] = (i32)frame;
    if (frame == 0) {
        return 1;
    }

    m[0] = phase;
    i32 cx, dy;
    switch (phase) {
        case 2:
            cx = 0x69;
            dy = 0x26;
            break;
        case 3:
            cx = 0x65;
            dy = 0x50;
            break;
        case 4:
            cx = 0x69;
            dy = 0x54;
            break;
        default:
            cx = 0x34;
            dy = 0x29;
            break;
    }

    i32* base = (i32*)m[0x3c / 4];
    i32 tx = base[0x10 / 4] + cx;
    m[0x4 / 4] = tx;
    i32 ty = base[0x14 / 4] + dy;
    m[0x8 / 4] = ty;

    i32 dxv = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = dxv * dxv + dyv * dyv;
    double dist = sqrt((double)dist2);
    u32 flyTime = g_buteMgr.GetDwordDef("WarpStone", "FlyTime", 0x5dc);

    *(double*)&m[0x20 / 4] = (double)flyTime / dist;
    *(double*)&m[0x28 / 4] = (double)dist2 / dist;
    *(double*)&m[0x30 / 4] = (double)dxv / dist;

    CStatusBarHolder* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
    if (h->m_surfaceGate == 0) {
        CSprite* fly = 0;
        h->m_10map.Lookup("GAME_WARPSTONEFLY", &fly);
        if (fly) {
            CStatusBarTab* t = (CStatusBarTab*)fly;
            if (g_61ab20 != 0 && g_6bf3c0 - t->m_drawClock >= t->m_window) {
                t->m_drawClock = g_6bf3c0;
                t->m_cueMgr->ConfigureItem(g_61ab24, 0, 0, 0);
            }
        }
    }

    *(double*)&m[0x10 / 4] = (double)dxv;
    *(double*)&m[0x18 / 4] = (double)dyv;
    return 1;
}
