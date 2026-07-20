#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h>      // g_sndEnabled/g_sndCueTag
#include <Gruntz/GameRegistry.h>    // g_gameReg singleton (0x24556c) canonical view
#include <Dsndmgr/DirectSoundMgr.h> // the ONE DSoundCloneInst shape (ConfigureItem @0x1360d0)
#include <Gruntz/LeafCue.h>         // the canonical cue record (was the CStatusBarTab view)
#include <Gruntz/StatusBarUpdatersViews.h> // referent views + EngineLabelBacklog host
#include <Gruntz/GameLevel.h>              // CGameLevel (m_world->m_level) -> m_mainPlane tile grid
#include <Gruntz/TileTriggerSwitchLogic.h> // real owner of SwitchDown/SwitchUp @0x110570/0x1106b0

extern "C" u32 g_killCueClock; // draw-clock mirror

// ===========================================================================
// CTileTriggerSwitchLogic::SwitchDown @0x110570  (base vtable slot 2)
// ===========================================================================
//
// The switch-logic slot-2 virtual: drives the switch tile into its DOWN state. It
// bumps the switch tile's cell-state counter in the map grid
// (plane->m_tileGrid[plane->m_colOffsets[y] + x]) and notifies the tile system, then -
// if the switch tile is on-screen (its pixel rect inside the view bounds) and the
// status bar surface is live - runs the GAME_SWITCHDOWN status-bar advance. Latches
// the switch state (+0x14) = 1 (down). __thiscall, returns 1. The leaf overrides
// (CTileSecretTriggerSwitchLogic/CTileTimeTriggerSwitchLogic::SwitchDown) call this base and
// normalize the result to a bool. Re-homed off the EngineLabelBacklog placeholder to
// its real owner (the leaf callers at 0x112820/0x112840 bind here).
// switch coords: m_08 == tile X, m_key0c == tile Y, m_linkGate (+0x14) == down/up state.
// @early-stop
// ~72% CSE/regalloc wall: the int(1) return (was void) is now correct, but retail
// RE-DERIVES `g_gameReg->m_world->m_level->m_mainPlane` for the store leg (pinning
// g_gameReg in edi), while our cl CSEs the two identical grid chains into one eax
// (keeping grid, not g_gameReg, in a callee-saved reg). The whole register layout
// cascades from that CSE choice. No source spelling defeats MSVC5's CSE of the two
// identical multi-level loads without an intervening store. Logic byte-correct.
RVA(0x00110570, 0xfb)
i32 CTileTriggerSwitchLogic::SwitchDown() {
    CLevelPlane* g = g_gameReg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_key0c] + m_08] + 1;
    CLevelPlane* g2 = g_gameReg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_key0c] + m_08] = v;
    g_gameReg->m_tileGrid->Notify(m_08, m_key0c, v);

    i32 px = (m_08 << 5) + 0x10;
    i32 py = (m_key0c << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CSndHost* h = g_gameReg->m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_10.Lookup("GAME_SWITCHDOWN", spr_ob);
            LeafCue* spr = static_cast<LeafCue*>(spr_ob);
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
    CLevelPlane* g = g_gameReg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_key0c] + m_08] - 1;
    CLevelPlane* g2 = g_gameReg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_key0c] + m_08] = v;
    g_gameReg->m_tileGrid->Notify(m_08, m_key0c, v);

    i32 px = (m_08 << 5) + 0x10;
    i32 py = (m_key0c << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CSndHost* h = g_gameReg->m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_10.Lookup("GAME_SWITCHUP", spr_ob);
            LeafCue* spr = static_cast<LeafCue*>(spr_ob);
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
