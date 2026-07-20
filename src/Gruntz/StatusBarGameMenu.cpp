// StatusBarGameMenu.cpp - the in-game GAMETAB menu builder (RVA 0x101580).
//
// This was mislabeled `CSBI_ImageSet::CSBI_ImageSet` by the rtti-vptr heuristic; it
// is NOT a constructor. It is the status-bar manager's "build the GAME-tab menu"
// method: it creates each GAMETAB widget (RESUME/PAUSE/LOAD/SAVE/SETTINGS/HELP/
// QUIT/DESTRUCT, or MISSIONSTATUS in the briefing variant), stamps its retail vtable
// + type tag, configures it from a GAME_STATUSBAR_TABZ_GAMETAB_* asset key + a
// geometry rect (tab base coords m_baseX/m_baseY plus per-item offsets), appends it
// to the Game-tab CPtrList m_items, and stashes the created widget into a per-command
// slot (m_tabSprite5..m_tabSprite10, m_slotDestruct). Built under a /GX EH frame (the
// just-created item is rolled back if its Configure throws). Identical create idiom
// to CStatusBarMgr::LoadTabSprites.
//
// Owner verified by the member writes: m_code (the configure `code` arg), the base
// coords m_baseX/m_baseY, the m_items Game CPtrList, the m_itemKind==0x1fb gate,
// the m_hitTestDisabled gate, the m_destructWarnActive/m_modeState/m_slotDestruct destruct-button
// state, and the per-command slot stores. The widgets are REAL polymorphic
// CSBI_ImageSet / CSBI_MenuItem leaves, so `new CSBI_ImageSet` makes cl auto-stamp
// the retail vtable (config/vtable_names.csv); no manual stamp. Field names are
// placeholders; only the OFFSETS + code bytes matter.
//
// @early-stop
// ~63% /GX menu builder; LOGIC COMPLETE. The /GX EH frame + Order-A prologue are now
// reproduced (Phase B leaf-ctor unblock, 37%->63%): the item ctors are NOT inlined
// destructible-member ctors as previously believed - they are TINY (verified: base
// ctor 0x1005d0 / CSBI_RectOnly ctor 0x101fa0 just zero four fields + stamp a vtable)
// and retail CALLS them OUT OF LINE (call 0x1e88). That opaque may-throw call is what
// makes cl register the `new`-expression operator-delete-on-ctor-throw cleanup and
// raise the frame. Declaring CSBI_Image's ctor out-of-line (below) emits that exact
// `call ??0CSbMenuItem; stamp derived vtable+tag` shape and the frame with this->esi/
// Order-A. The briefing branch is also now the sunk (`!=` fall-through) block matching
// retail's `je` layout. RESIDUAL WALLS (deep MSVC lowering, not steerable from source):
//   (1) new-expression TEMP unification - retail stores the operator-new result
//       directly into the `it` slot BEFORE the ctor (one slot; the EH cleanup + failure
//       `delete it` share it), while cl keeps a separate cleanup temp then copies to
//       `it` after the ctor -> +8 B frame (sub 0x20 vs retail 0x18), shifting every
//       [esp+N]. Tried per-item scoped locals and exact-derived-type locals; neither
//       fuses the temp.
//   (2) EH state numbering is rotated +2 (retail main items = states 2..8, ours 0..6)
//       because retail numbers the sunk briefing block's cleanup first in source order;
//       our `!=` fall-through numbers it last. Briefing-first source gives the right
//       numbering but the WRONG (inline) layout.
// Both are documented codegen walls (docs/patterns/gx-frame-outofline-ctor.md);
// re-attack in the final sweep.
#include <rva.h>
#include <Gruntz/TriggerMgr.h> // m_cmdGrid's real class (m_phase/m_3ec)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>

#include <Mfc.h>
#include <Gruntz/GameRegistry.h>

#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)
#include <Gruntz/GameMenuMgrBuilders.h> // CGmFactory (the registry factory view)
#include <Gruntz/StatusBarMgr.h>        // canonical CStatusBarMgr (== the ex-CGameMenuMgr)
#include <Gruntz/SBI_MenuItem.h>        // canonical CSBI_MenuItem (12 slots, vtbl 0x5eab4c)
#include <Gruntz/SBI_ImageSet.h>        // canonical CSBI_ImageSet  (13 slots, vtbl 0x5eac4c)

RVA(0x00101580, 0x806)
void CStatusBarMgr::BuildGameMenu() {
    CDDrawSurfaceMgr* code = m_c; // the setup arg2 config host
    i32 bx = m_10;
    i32 by = m_rect14.m_0;
    CSBI_Image* it;
    SbRect r;

    // Non-briefing path is the fall-through (retail `je` sinks the briefing block to
    // the end): the `!=` gate keeps the common menu inline and the MISSIONSTATUS
    // widget out of line.
    if (m_itemKind != 0x1fb) {
        // ---- RESUME or PAUSE in the first slot ----
        if (m_hitTestDisabled != 0 && g_gameReg->m_frameGate != 0) {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    0x1f4,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_RESUME",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return;
            }
            m_tabLists[5].AddTail(it);
        } else {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    0x1f4,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return;
            }
            m_tabLists[5].AddTail(it);
        }
        m_tabSprite5 = static_cast<CSBI_MenuItem*>(it);

        // ---- LOAD ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x125;
        r.right = bx + 0x9f;
        r.bottom = by + 0x13c;
        if (!it->SetupImage(this, code, 0x1f5, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_LOAD", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite6 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- SAVE ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0xfd;
        r.right = bx + 0x9f;
        r.bottom = by + 0x114;
        if (!it->SetupImage(this, code, 0x1f6, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_SAVE", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite7 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- SETTINGS ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x14d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x164;
        if (!it->SetupImage(
                this,
                code,
                0x1f7,
                5,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite8 = static_cast<CSBI_MenuItem*>(it);

        // ---- HELP ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x175;
        r.right = bx + 0x9f;
        r.bottom = by + 0x18c;
        if (!it->SetupImage(this, code, 0x1f8, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_HELP", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite9 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- QUIT (inlined ctor in retail) ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x19d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x1b4;
        if (!it->SetupImage(this, code, 0x1f9, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_QUIT", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite10 = static_cast<CSBI_MenuItem*>(it);

        // ---- DESTRUCT (CSBI_ImageSet, tag 4) ----
        it = new CSBI_ImageSet;
        r.left = bx + 0x22;
        r.top = by + 0x1be;
        r.right = bx + 0x7d;
        r.bottom = by + 0x1d6;
        if (!it->SetupImage(
                this,
                code,
                0x1fc,
                5,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                m_modeState,
                0
            )) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_modeNotify = static_cast<CSBI_ImageSet*>(it); // RESOLVED: retail's push-0x34 agrees (base==target, 12x0x34/14x0x3c) - the field holds BOTH classes over time; the +0x30 Notify only fires in ImageSet-holding states
        if (g_gameReg->m_134 != 1) {
            it->m_enabled = 0;
            m_modeState = 7;
            m_destructWarnActive = 0;
            m_modeNotify->Notify(7);
        }
        return;
    }

    // ---- briefing variant: a single MISSIONSTATUS widget ----
    it = new CSBI_ImageSet;
    i32 variant = (g_gameReg->m_cmdGrid->m_phase == 1) ? 1 : 2; // MISSIONSTATUS variant = the round phase
    r.left = bx;
    r.top = by + 0xd7;
    r.right = bx + 0x9f;
    r.bottom = by + 0xec;
    if (!it->SetupImage(
            this,
            code,
            0x1fb,
            5,
            r,
            "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
            variant,
            0
        )) {
        if (it) {
            delete it;
        }
        return;
    }
    m_tabLists[5].AddTail(it);
}
