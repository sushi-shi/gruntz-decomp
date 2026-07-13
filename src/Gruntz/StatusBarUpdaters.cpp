#include <rva.h>
#include <Gruntz/GameRegistry.h>           // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/SoundCueMgr.h>            // the ONE CSoundCueMgr shape (ConfigureItem @0x1360d0)
#include <Gruntz/LeafCue.h>                // the canonical cue record (was the CStatusBarTab view)
#include <Gruntz/StatusBarUpdatersViews.h> // referent views + EngineLabelBacklog host
#include <Gruntz/TileTriggerSwitchLogic.h> // real owner of SwitchDown/SwitchUp @0x110570/0x1106b0

// StatusBarUpdaters.cpp - the switch-tile sprite loaders (C:\Proj\Gruntz). The five
// in-game status-bar updaters that used to live here (UpdateGruntOven/DestructButton/
// ChipGrinder/WarpStone + LoadStatzTabToggleSprite, RVAs 0x104e60-0x10b320) moved to
// src/Gruntz/SBI_RectOnly.cpp in the wave1-E one-TU merge (interval dossier
// 0x104d60-0x10bc14); the two fns left (0x110570/0x1106b0) sit in the 0x110430
// tiletriggerderivedctors interval and await their own re-home.
//
// They live on the big in-game game-mode object (the EngineLabelBacklog placeholder
// the rest of the backlog hangs off) and share the engine's "advance status-bar tab"
// tail: a named-sprite Lookup through the global status-bar mgr
// (g_gameReg->m_world->m_statusBar->m_10 -> CSndFinder::Lookup), a draw-clock window
// check (g_killCueClock - t->m_drawClock >= t->m_window), and a
// CStatusBarMgr::ConfigureItem push (the shared 0x1360d0 helper, reloc-masked).
//
// Only offsets / code bytes are load-bearing; names are placeholders.

// The draw-clock mirror global (canonical in CPlay.h / surfacemgr).
extern "C" u32 g_killCueClock; // draw-clock mirror

// The two paired status-bar globals the advance tail reads (external delinked
// DATA symbols, reloc-masked): g_sndEnabled gates the push, g_sndCueTag is the value.
extern i32 g_sndEnabled; // DAT_0061ab20
extern i32 g_sndCueTag;  // DAT_0061ab24

// The canonical CGameRegistry view of the singleton (*0x24556c). The resource
// holder (+0x30 -> CRegHolder) is cast locally at the deref sites; the tile
// notifier (+0x70) is the canonical CTileGrid (Notify facet), reached without a
// cast, and the view-bounds rectangle scalars (+0x13c..+0x148) match directly.
extern "C" CGameRegistry* g_gameReg; // the game-manager singleton

// ===========================================================================
// CTileTriggerSwitchLogic::SwitchDown @0x110570  (base vtable slot 2)
// ===========================================================================
//
// The switch-logic slot-2 virtual: drives the switch tile into its DOWN state. It
// bumps the switch tile's cell-state counter in the map grid
// (grid->m_cellState[grid->m_rowOffset[y] + x]) and notifies the tile system, then -
// if the switch tile is on-screen (its pixel rect inside the view bounds) and the
// status bar surface is live - runs the GAME_SWITCHDOWN status-bar advance. Latches
// the switch state (+0x14) = 1 (down). __thiscall, returns 1. The leaf overrides
// (CTileSecretTriggerSwitchLogic/CTileTimeTriggerSwitchLogic::SwitchDown) call this base and
// normalize the result to a bool. Re-homed off the EngineLabelBacklog placeholder to
// its real owner (the leaf callers at 0x112820/0x112840 bind here).
// switch coords: m_08 == tile X, m_key0c == tile Y, m_linkGate (+0x14) == down/up state.
// @early-stop
// ~72% CSE/regalloc wall: the int(1) return (was void) is now correct, but retail
// RE-DERIVES `g_gameReg->m_world->m_tileHolder->m_grid` for the store leg (pinning
// g_gameReg in edi), while our cl CSEs the two identical grid chains into one eax
// (keeping grid, not g_gameReg, in a callee-saved reg). The whole register layout
// cascades from that CSE choice. No source spelling defeats MSVC5's CSE of the two
// identical multi-level loads without an intervening store. Logic byte-correct.
RVA(0x00110570, 0xfb)
i32 CTileTriggerSwitchLogic::SwitchDown() {
    CMapTileGrid* g = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    i32 v = g->m_cellState[g->m_rowOffset[m_key0c] + m_08] + 1;
    CMapTileGrid* g2 = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    g2->m_cellState[g2->m_rowOffset[m_key0c] + m_08] = v;
    g_gameReg->m_tileGrid->Notify(m_08, m_key0c, v);

    i32 px = (m_08 << 5) + 0x10;
    i32 py = (m_key0c << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CSndHost* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_10.Lookup("GAME_SWITCHDOWN", spr_ob);
            LeafCue* spr = (LeafCue*)spr_ob;
            if (spr) {
                if (g_sndEnabled != 0 && g_killCueClock - spr->m_14 >= spr->m_18) {
                    spr->m_14 = g_killCueClock;
                    spr->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }
    m_linkGate = 1;
    return 1;
}

// ===========================================================================
// CTileTriggerSwitchLogic::SwitchUp @0x1106b0  (base vtable slot 3)
// ===========================================================================
//
// The UP mirror of SwitchDown (slot 3): decrements the cell-state counter, runs the
// GAME_SWITCHUP advance, and latches the switch state (+0x14) = 0 (up). __thiscall,
// returns 1. The leaf override (CTileTimeTriggerSwitchLogic::SwitchUp @0x112860) calls this
// base and normalizes to a bool.
// @early-stop
// ~70% CSE/regalloc wall (same as SwitchDown): int(1) return now correct; residual is the
// grid-chain CSE that pins grid instead of g_gameReg. Logic exact.
RVA(0x001106b0, 0xf4)
i32 CTileTriggerSwitchLogic::SwitchUp() {
    CMapTileGrid* g = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    i32 v = g->m_cellState[g->m_rowOffset[m_key0c] + m_08] - 1;
    CMapTileGrid* g2 = ((CRegHolder*)g_gameReg->m_world)->m_tileHolder->m_grid;
    g2->m_cellState[g2->m_rowOffset[m_key0c] + m_08] = v;
    g_gameReg->m_tileGrid->Notify(m_08, m_key0c, v);

    i32 px = (m_08 << 5) + 0x10;
    i32 py = (m_key0c << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CSndHost* h = ((CRegHolder*)g_gameReg->m_world)->m_statusBar;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_10.Lookup("GAME_SWITCHUP", spr_ob);
            LeafCue* spr = (LeafCue*)spr_ob;
            if (spr) {
                if (g_sndEnabled != 0 && g_killCueClock - spr->m_14 >= spr->m_18) {
                    spr->m_14 = g_killCueClock;
                    spr->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }
    m_linkGate = 0;
    return 1;
}
