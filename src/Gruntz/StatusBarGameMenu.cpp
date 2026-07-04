// StatusBarGameMenu.cpp - the in-game GAMETAB menu builder (RVA 0x101580).
//
// This was mislabeled `CSBI_ImageSet::CSBI_ImageSet` by the rtti-vptr heuristic; it
// is NOT a constructor. It is the status-bar manager's "build the GAME-tab menu"
// method: it creates each GAMETAB widget (RESUME/PAUSE/LOAD/SAVE/SETTINGS/HELP/
// QUIT/DESTRUCT, or MISSIONSTATUS in the briefing variant), stamps its retail vtable
// + type tag, configures it from a GAME_STATUSBAR_TABZ_GAMETAB_* asset key + a
// geometry rect (tab base coords m_baseX/m_baseY plus per-item offsets), appends it
// to the Game-tab CPtrList m_items, and stashes the created widget into a per-command
// slot (m_slotResume..m_slotQuit, m_slotDestruct). Built under a /GX EH frame (the
// just-created item is rolled back if its Configure throws). Identical create idiom
// to CStatusBarMgr::LoadTabSprites.
//
// Owner verified by the member writes: m_code (the configure `code` arg), the base
// coords m_baseX/m_baseY, the m_items Game CPtrList, the m_briefingGate==0x1fb gate,
// the m_showResume gate, the m_558/m_destructState/m_slotDestruct destruct-button
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
// raise the frame. Declaring CSbMenuItem's ctor out-of-line (below) emits that exact
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

#include <Mfc.h>
#include <Gruntz/GameRegistry.h>

#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)
#include <Gruntz/GameMenuMgrBuilders.h> // CSbMenuItem builder-facet + SBI leaves + CGameMenuMgr

// ---------------------------------------------------------------------------
// The builder-facet base CSbMenuItem, its concrete SBI leaves (CSBI_ImageSet /
// CSBI_MenuItem), the registry factory view CGmFactory and the CGameMenuMgr class
// moved to <Gruntz/GameMenuMgrBuilders.h>.
// ---------------------------------------------------------------------------
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ===========================================================================
// CGameMenuMgr::BuildGameMenu  @0x101580
// ===========================================================================
RVA(0x00101580, 0x806)
void CGameMenuMgr::BuildGameMenu() {
    i32 code = m_code;
    i32 bx = m_baseX;
    i32 by = m_baseY;
    CSbMenuItem* it;
    SbRect r;

    // Non-briefing path is the fall-through (retail `je` sinks the briefing block to
    // the end): the `!=` gate keeps the common menu inline and the MISSIONSTATUS
    // widget out of line.
    if (m_briefingGate != 0x1fb) {
        // ---- RESUME or PAUSE in the first slot ----
        if (m_showResume != 0 && g_gameReg->m_frameGate != 0) {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->Configure(
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
            m_items.AddTail(it);
        } else {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->Configure(
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
            m_items.AddTail(it);
        }
        m_slotResume = it;

        // ---- LOAD ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x125;
        r.right = bx + 0x9f;
        r.bottom = by + 0x13c;
        if (!it->Configure(this, code, 0x1f5, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_LOAD", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_items.AddTail(it);
        m_slotLoad = it;
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- SAVE ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0xfd;
        r.right = bx + 0x9f;
        r.bottom = by + 0x114;
        if (!it->Configure(this, code, 0x1f6, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_SAVE", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_items.AddTail(it);
        m_slotSave = it;
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- SETTINGS ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x14d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x164;
        if (!it->Configure(
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
        m_items.AddTail(it);
        m_slotSettings = it;

        // ---- HELP ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x175;
        r.right = bx + 0x9f;
        r.bottom = by + 0x18c;
        if (!it->Configure(this, code, 0x1f8, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_HELP", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_items.AddTail(it);
        m_slotHelp = it;
        if (g_gameReg->m_134 == 2) {
            it->m_enabled = 0;
        }

        // ---- QUIT (inlined ctor in retail) ----
        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x19d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x1b4;
        if (!it->Configure(this, code, 0x1f9, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_QUIT", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_items.AddTail(it);
        m_slotQuit = it;

        // ---- DESTRUCT (CSBI_ImageSet, tag 4) ----
        it = new CSBI_ImageSet;
        r.left = bx + 0x22;
        r.top = by + 0x1be;
        r.right = bx + 0x7d;
        r.bottom = by + 0x1d6;
        if (!it->Configure(
                this,
                code,
                0x1fc,
                5,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                m_destructState,
                0
            )) {
            if (it) {
                delete it;
            }
            return;
        }
        m_items.AddTail(it);
        m_slotDestruct = it;
        if (g_gameReg->m_134 != 1) {
            it->m_enabled = 0;
            m_destructState = 7;
            m_558 = 0;
            m_slotDestruct->Activate(7);
        }
        return;
    }

    // ---- briefing variant: a single MISSIONSTATUS widget ----
    it = new CSBI_ImageSet;
    i32 variant = (((CGmFactory*)g_gameReg->m_cmdGrid)->m_variant == 1) ? 1 : 2;
    r.left = bx;
    r.top = by + 0xd7;
    r.right = bx + 0x9f;
    r.bottom = by + 0xec;
    if (!it->Configure(
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
    m_items.AddTail(it);
}
