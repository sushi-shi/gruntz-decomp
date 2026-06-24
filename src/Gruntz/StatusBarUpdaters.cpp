#include <rva.h>
#include <Bute/ButeMgr.h>
// StatusBarUpdaters.cpp - the per-widget HUD status-bar updaters and switch-tile
// sprite loaders (C:\Proj\Gruntz). They live on the big in-game game-mode object
// (the EngineLabelBacklog placeholder the rest of the backlog hangs off) and
// share two engine idioms:
//
//   * the elapsed-time clamp: a 64-bit `g_645588 - tab->ts` clamped at 0, then
//     divided by a CButeMgr-configured delay to drive an animation frame index;
//   * the "advance status-bar tab" tail: a named-sprite Lookup through the global
//     status-bar mgr (g_gameReg->m_30->m_28->m_10 -> CSpriteHashTable::Lookup),
//     a draw-clock window check (g_6bf3c0 - tab->m_14 >= tab->m_18), and a
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

// CRT sqrt - intrinsified to an inline fsqrt under VC5 /O2.
extern "C" double sqrt(double);

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table (same shape as SpriteLoaders).
// Lookup() hashes the class-name key and writes the found sprite through *ppOut.
// ---------------------------------------------------------------------------
// The engine sprite (animation) object: a frame-pointer table at +0x14 and the
// inclusive valid frame range [m_64..m_68] (same as SpriteLoaders).
struct CSprite {
    char m_pad00[0x14];
    i32** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  first valid frame
    i32 m_68; // +0x68  last valid frame
};
class CSpriteHashTable {
public:
    i32 Lookup(const char* szName, CSprite** ppOut);
};

// CStatusBarMgr::ConfigureItem (the shared status-bar push helper @0x1360d0;
// external/no-body so the `call rel32` reloc-masks). __thiscall, ret 0x10.
class CStatusBarMgr {
public:
    i32 ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3);
};

// The status-bar item the named Lookup resolves: it holds the CStatusBarMgr to
// push the configuration into at +0x10, a draw-clock latch (+0x14) and a window
// width (+0x18).
struct CStatusBarTab {
    char m_pad00[0x10];
    CStatusBarMgr* m_10; // +0x10  the mgr ConfigureItem pushes into
    u32 m_14;            // +0x14  draw-clock latch
    u32 m_18;            // +0x18  window width
};

// The status-bar holder reached through the registry: its embedded hash table is
// at +0x10 (the `add ecx,0x10` before Lookup addresses it) and +0x30 gates the
// live surface.
struct CStatusBarHolder {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10  embedded name->sprite hash table
    char m_pad14[0x30 - 0x14];
    i32 m_30; // +0x30  live-surface gate
};

// The map tile grid reached via m_30->m_24->m_5c (two parallel tables: a cell
// state table at +0x20 and a row-offset table at +0x24).
struct CTileGrid {
    char m_pad00[0x20];
    i32* m_20; // +0x20  cell-state table
    i32* m_24; // +0x24  row-offset table
};
// The tile-system notifier at registry +0x70.
struct CTileNotifier {
    void Notify(i32 x, i32 y, i32 state);
};
// The registry's +0x30 holder: it carries the tile-grid holder (+0x24 -> +0x5c
// grid) and the status-bar holder (+0x28).
struct CRegHolder {
    char m_pad00[0x24];
    struct M24 {
        char m_pad00[0x5c];
        CTileGrid* m_5c;
    }* m_24;                // +0x24 -> +0x5c grid
    CStatusBarHolder* m_28; // +0x28
};
// The single typed view of the game-manager singleton (*0x24556c) this TU uses:
// the resource holder (+0x30), the group table (+0x68), the tile notifier (+0x70)
// and the view-bounds rectangle (+0x13c..+0x148).
struct CGameReg {
    char m_pad00[0x30];
    CRegHolder* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    i32* m_68; // +0x68  group-record table
    char m_pad6c[0x70 - 0x6c];
    CTileNotifier* m_70; // +0x70
    char m_pad74[0x13c - 0x74];
    i32 m_13c; // +0x13c  view min X
    i32 m_140; // +0x140  view min Y
    i32 m_144; // +0x144  view max X
    i32 m_148; // +0x148  view max Y
};
DATA(0x0024556c)
extern CGameReg* g_gameReg; // the game-manager singleton

// ---------------------------------------------------------------------------
// EngineLabelBacklog - the placeholder class the backlog stubs hang off (modeled
// as a class so the member-fn mangling falls out, mirroring IconLoaders.cpp).
// The status-bar updaters live on the big game-mode object: a 5-slot array of
// 0x18-byte tab records at +0x220 with a parallel pointer array at +0x204.
// ---------------------------------------------------------------------------
struct CTabRec {
    i32 m_0; // +0x00  state (1 active, 2 finished)
    i32 m_4; // +0x04  last animation frame index
    u32 m_8; // +0x08  start-clock lo
    u32 m_c; // +0x0c  start-clock hi
    char m_pad10[0x18 - 0x10];
};

// The parallel widget objects with a thiscall virtual at vtbl+0x30 (set-frame,
// slot 12). Modeled with a padded virtual interface so `call [vtbl+0x30]` falls
// out as a __thiscall (this=ecx, one stack arg).
struct CTabWidget {
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void s0b();
    virtual void SetFrame(i32 frame); // slot 12 (+0x30)
};

// The destruct-button countdown block on the game-mode object (overlaid at
// +0x558): a 3-state machine (0 idle / 1 warning-down / 2 warning-up) with a
// frame counter, a 64-bit retrigger clock, the warning delay, and the widget.
struct CDestructBlock {
    i32 m_558;         // +0x558  state (0/1/2)
    i32 m_55c;         // +0x55c  frame counter
    u32 m_560;         // +0x560  retrigger-clock lo
    u32 m_564;         // +0x564  retrigger-clock hi
    u32 m_568;         // +0x568  warning delay lo
    u32 m_56c;         // +0x56c  warning delay hi (always 0)
    CTabWidget* m_570; // +0x570  the warning widget
};

// The chip-grinder rect-target widget at m_500: its +0x14..+0x20 screen rect is
// stamped each step from the scroll origin (m_10/m_14) and the grinder extents.
struct CGrinderRect {
    char m_pad00[0x14];
    i32 m_14; // +0x14  left
    i32 m_18; // +0x18  top
    i32 m_1c; // +0x1c  right
    i32 m_20; // +0x20  bottom
};

// The per-tab toggle item reached through this[idx*4 + 0x150]: SetField0 stamps
// the toggle value (+0x44), m_4 is its active flag.
struct CStatzTabItem {
    char m_pad00[0x4];
    i32 m_4; // +0x04  active flag
    char m_pad08[0x44 - 0x08];
    i32 m_44; // +0x44  toggle value
};

class EngineLabelBacklog {
public:
    void UpdateGruntOvenStatusBar();
    void UpdateDestructButtonStatusBar();
    void UpdateChipGrinderStatusBar();
    void ChipGrinderFinishStep(); // thunk_FUN_00506a00 (external, reloc-masked)
    i32 LoadStatzTabToggleSprite(i32 value, i32 idx);
    void LoadSwitchDownSprite();
    void LoadSwitchUpSprite();
    i32 UpdateWarpStoneStatusBar(i32 a0, i32 phase, i32 srcX, i32 srcY);

    // The switch tile-trigger object: m_8/m_c are its grid coords, m_14 its
    // down/up state flag.
    char m_pad00[0x8];
    i32 m_8; // +0x08  tile X
    i32 m_c; // +0x0c  tile Y
    char m_pad10[0x14 - 0x10];
    i32 m_14; // +0x14  down(1)/up(0) flag
    char m_pad18[0x204 - 0x18];
    CTabWidget* m_slots[5]; // +0x204  the 5 grunt-oven tab widgets
    char m_pad218[0x220 - 0x218];
    CTabRec m_tabs[5]; // +0x220  the 5 cooking tabs
    char m_pad298[0x558 - 0x298];
    CDestructBlock m_destruct; // +0x558  the destruct-button block
};

// The toggle's sub-helper reached through this[idx*4 + 0x18c] (thunk_FUN_004ea170,
// external/reloc-masked); __thiscall, two args (this->m_0 and the active flag).
struct CStatzTabSub {
    void Toggle(i32 stateId, i32 on);
};

// ===========================================================================
// EngineLabelBacklog::UpdateGruntOvenStatusBar @0x105310
// ===========================================================================
//
// Walks the 5 grunt-oven cooking tabs: while a tab is COOKING (m_0==1) it derives
// the cooking-progress frame index from the elapsed clock / GruntOvenDelay, caps
// at 0x1a (completion - flips m_0 to 2 and runs the COOKINGCOMPLETE advance), and
// pushes the new frame into the widget when it changes (the +0x30 virtual).
RVA(0x00105310, 0x11a)
void EngineLabelBacklog::UpdateGruntOvenStatusBar() {
    CTabWidget** slot = m_slots;
    CTabRec* tab = m_tabs;
    i32 n = 5;
    do {
        if (tab->m_0 == 1) {
            i64 d = (i64)(u32)g_645588 - *(i64*)&tab->m_8;
            i32 elapsed = (d >= 0) ? (i32)d : 0;
            u32 delay = g_buteMgr.GetDwordDef("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = (i32)((u32)elapsed / delay) + 1;
            if (frame >= 0x1a) {
                tab->m_0 = 2;
                frame = 0x1a;
                CStatusBarHolder* h = g_gameReg->m_30->m_28;
                if (h->m_30 == 0) {
                    CSprite* spr = 0;
                    h->m_10map.Lookup("GAME_COOKINGCOMPLETE", &spr);
                    if (spr) {
                        CStatusBarTab* t = (CStatusBarTab*)spr;
                        if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                            t->m_14 = g_6bf3c0;
                            t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
                        }
                    }
                }
            }
            if (frame != tab->m_4) {
                tab->m_4 = frame;
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
RVA(0x0010b320, 0x167)
void EngineLabelBacklog::UpdateDestructButtonStatusBar() {
    CDestructBlock* b = &m_destruct;
    switch (b->m_558) {
        case 1: {
            i64 d = (i64)(u32)g_645588 - *(i64*)&b->m_560;
            if (d >= *(i64*)&b->m_568) {
                if (++b->m_55c >= 6) {
                    b->m_55c = 6;
                    b->m_558 = 2;
                }
                b->m_568 = g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                b->m_56c = 0;
                b->m_560 = g_645588;
                b->m_564 = 0;
                CTabWidget* w = b->m_570;
                if (w) {
                    w->SetFrame(b->m_55c);
                }
            }
            break;
        }
        case 2: {
            i64 d = (i64)(u32)g_645588 - *(i64*)&b->m_560;
            if (d >= *(i64*)&b->m_568) {
                if (--b->m_55c <= 2) {
                    b->m_55c = 2;
                    b->m_558 = 1;
                }
                b->m_568 = g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                b->m_56c = 0;
                b->m_560 = g_645588;
                b->m_564 = 0;
                CTabWidget* w = b->m_570;
                if (w) {
                    w->SetFrame(b->m_55c);
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
                    CStatusBarHolder* h = g_gameReg->m_30->m_28;
                    if (h->m_30 == 0) {
                        CSprite* spr = 0;
                        h->m_10map.Lookup("GAME_REZGRINDING", &spr);
                        if (spr) {
                            CStatusBarTab* t = (CStatusBarTab*)spr;
                            if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                                t->m_14 = g_6bf3c0;
                                t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
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
                i32* p = &w->m_14;
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
RVA(0x00104e60, 0xed)
i32 EngineLabelBacklog::LoadStatzTabToggleSprite(i32 value, i32 idx) {
    i32* m = (i32*)this;
    if (m[idx + 0x114 / 4] == value) {
        return 1;
    }

    i32 slot = idx + 15 * g_644c54;
    if (*(i32*)((char*)g_gameReg->m_68 + slot * 4 + 0x1c) == 0) {
        return 0;
    }

    CStatzTabItem* item = (CStatzTabItem*)m[idx + 0x150 / 4];
    i32 one = 1;
    if (item) {
        item->m_44 = value;
        item->m_4 = one;
        if (m[0x10c / 4] == one) {
            ((CStatzTabSub*)m[idx + 0x18c / 4])->Toggle(m[0], one);
            CStatusBarHolder* h = g_gameReg->m_30->m_28;
            if (h->m_30 == 0) {
                CSprite* spr = 0;
                h->m_10map.Lookup("GAME_STATZTABTOGGLE", &spr);
                if (spr) {
                    CStatusBarTab* t = (CStatusBarTab*)spr;
                    if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                        t->m_14 = g_6bf3c0;
                        t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
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
// counter in the map grid (grid->m_20[grid->m_24[m_c] + m_8]) and notifies the
// tile system, then - if the switch tile is on-screen (its pixel rect inside the
// view bounds) and the status bar surface is live - runs the GAME_SWITCHDOWN
// status-bar advance. Latches m_14 = 1 (down). __thiscall.
RVA(0x00110570, 0xfb)
void EngineLabelBacklog::LoadSwitchDownSprite() {
    CTileGrid* g = g_gameReg->m_30->m_24->m_5c;
    i32 v = g->m_20[g->m_24[m_c] + m_8] + 1;
    CTileGrid* g2 = g_gameReg->m_30->m_24->m_5c;
    g2->m_20[g2->m_24[m_c] + m_8] = v;
    g_gameReg->m_70->Notify(m_8, m_c, v);

    i32 px = (m_8 << 5) + 0x10;
    i32 py = (m_c << 5) + 0x10;
    if (px < g_gameReg->m_144 && px >= g_gameReg->m_13c && py < g_gameReg->m_148
        && py >= g_gameReg->m_140) {
        CStatusBarHolder* h = g_gameReg->m_30->m_28;
        if (h->m_30 == 0) {
            CSprite* spr = 0;
            h->m_10map.Lookup("GAME_SWITCHDOWN", &spr);
            if (spr) {
                CStatusBarTab* t = (CStatusBarTab*)spr;
                if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                    t->m_14 = g_6bf3c0;
                    t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
                }
            }
        }
    }
    m_14 = 1;
}

// ===========================================================================
// EngineLabelBacklog::LoadSwitchUpSprite @0x1106b0
// ===========================================================================
//
// The UP mirror of LoadSwitchDownSprite: decrements the cell-state counter, runs
// the GAME_SWITCHUP advance, and latches m_14 = 0 (up). __thiscall.
RVA(0x001106b0, 0xf4)
void EngineLabelBacklog::LoadSwitchUpSprite() {
    CTileGrid* g = g_gameReg->m_30->m_24->m_5c;
    i32 v = g->m_20[g->m_24[m_c] + m_8] - 1;
    CTileGrid* g2 = g_gameReg->m_30->m_24->m_5c;
    g2->m_20[g2->m_24[m_c] + m_8] = v;
    g_gameReg->m_70->Notify(m_8, m_c, v);

    i32 px = (m_8 << 5) + 0x10;
    i32 py = (m_c << 5) + 0x10;
    if (px < g_gameReg->m_144 && px >= g_gameReg->m_13c && py < g_gameReg->m_148
        && py >= g_gameReg->m_140) {
        CStatusBarHolder* h = g_gameReg->m_30->m_28;
        if (h->m_30 == 0) {
            CSprite* spr = 0;
            h->m_10map.Lookup("GAME_SWITCHUP", &spr);
            if (spr) {
                CStatusBarTab* t = (CStatusBarTab*)spr;
                if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                    t->m_14 = g_6bf3c0;
                    t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
                }
            }
        }
    }
    m_14 = 0;
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
RVA(0x00109bd0, 0x1b5)
i32 EngineLabelBacklog::UpdateWarpStoneStatusBar(i32 a0, i32 phase, i32 srcX, i32 srcY) {
    i32* m = (i32*)this;
    m[0x3c / 4] = a0;

    CSprite* spr = 0;
    i32 n = phase + 1;
    g_gameReg->m_30->m_28->m_10map.Lookup("GAME_STATUSBAR_TABZ_GAMETAB_WARP", &spr);
    i32* frame = (spr && n >= spr->m_64 && n <= spr->m_68) ? spr->m_14[n] : 0;
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

    CStatusBarHolder* h = g_gameReg->m_30->m_28;
    if (h->m_30 == 0) {
        CSprite* fly = 0;
        h->m_10map.Lookup("GAME_WARPSTONEFLY", &fly);
        if (fly) {
            CStatusBarTab* t = (CStatusBarTab*)fly;
            if (g_61ab20 != 0 && g_6bf3c0 - t->m_14 >= t->m_18) {
                t->m_14 = g_6bf3c0;
                t->m_10->ConfigureItem(g_61ab24, 0, 0, 0);
            }
        }
    }

    *(double*)&m[0x10 / 4] = (double)dxv;
    *(double*)&m[0x18 / 4] = (double)dyv;
    return 1;
}
