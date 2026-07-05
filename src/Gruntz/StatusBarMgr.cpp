// StatusBarMgr.cpp - the in-game status-bar per-tab widget builder.
//
// LoadTabSprites (RVA 0x102250, __thiscall, returns int) is the giant per-tab
// builder: it dispatches on the current tab index (m_10c, 1..5) and, for that
// tab, creates each CStatusBarItem-derived widget, stamps its retail vtable +
// type tag, configures it from a named sprite-asset key + a geometry rect (built
// as the tab base coords m_10/m_14 plus per-item offsets), and appends it to the
// tab's CPtrList. Built under a /GX EH frame (the items are EH-rolled-back if a
// later configure fails). Owner verified by the member writes:
//   - m_c/m_10/m_14 read at entry (the configure-virtual `this`/base coords)
//   - m_10c the tab selector (switch over 1..5)
//   - per-tab CPtrLists at +0x48 (Statz) / +0x64,+0x80 (Gruntz/Resource) /
//     +0x9c (Multiplayer) / +0xb8 (Game), plus many cached item slots
//     (+0x204..+0x520, +0x61c..+0x62c) it stashes the created widgets into.
//   - tab keys are all GAME_STATUSBAR_TABZ_* / GAME_INGAMEICONZ_* assets.
//
// The created widgets are REAL polymorphic subclasses (CSBI_Image / CSBI_ImageSet /
// CSBI_WellGoo, derived from the CSbConfigItem base): `new CSBI_Image` makes MSVC
// auto-stamp the retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c, catalogued in
// config/vtable_names.csv), so no manual vtable-pointer stamp is needed.
//
// Switch case -> tab (from the jump table at VA 0x504020):
//   m_10c==1 -> Statz   m_10c==2 -> Gruntz   m_10c==3 -> Resource
//   m_10c==4 -> Multiplayer   m_10c==5 -> Game
#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>

#include <Gruntz/SbRect.h>        // the by-value geometry rect the Configure virtuals take
#include <Gruntz/SpriteRefTable.h> // g_gameReg->m_spriteFactory->GetSel (GRUNTOVEN palette)
// The retail out-of-line base-ctor call raises the /GX frame; with the esi-relative cache
// stores below giving retail's register pressure, this now lands this->esi/Order-A.
#define CSBCONFIGITEM_OUTOFLINE_CTOR
#include <Gruntz/StatusBarMgrBuilders.h> // CSbConfigItem builder-facet + SBI leaves + CStatusBarMgr

// ---------------------------------------------------------------------------
// The builder-facet base CSbConfigItem, its concrete SBI leaves (CSBI_Image /
// CSBI_ImageSet / CSBI_WellGoo), the icon/factory helpers (CSbItemHelp / CSbFactory
// / CSbIconSet) and the CStatusBarMgr class moved to
// <Gruntz/StatusBarMgrBuilders.h>.
// ---------------------------------------------------------------------------

// The game registry: factory at +0x68/+0x74, a per-player icon table at +0x158
// (stride for the per-player block: 71*8; per-icon stride inside it: 0x238).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // ?g_gameReg@@3PAUCGameReg@@A @ VA 0x64556c

DATA(0x00244c54)
extern i32 g_curPlayer; // DAT_00644c54

// ===========================================================================
// CSbConfigItem::SetDirection  (0x0ea0f0)
// ===========================================================================
// Two boolean selectors (a,b) pick one of four direction tuples, forwarded to
// the +0x38 virtual. Reached via thunk 0x1573 from LoadTabSprites + FUN_00504f90.
RVA(0x000ea0f0, 0x5c)
void CSbConfigItem::SetDirection(i32 a, i32 b) {
    if (a == 0) {
        if (b == 0) {
            ApplyDir(4, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, 1, 0, -1);
        }
    } else {
        if (b == 0) {
            ApplyDir(1, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, -1, 0, -1);
        }
    }
}

// ===========================================================================
// CStatusBarMgr::LoadTabSprites  @0x102250
// ===========================================================================
// Per-tab builder. The Configure args (recovered from every call site) are:
//   Configure(this, code, type, idx, rect, key, flag, 0)
// `type` = tab index, `code` an incrementing per-item id, `idx` 2/3/4/5/1,
// `flag` -1 or small int, `rect` = base + per-item offsets. The per-item create
// idiom (new/ctor/stamp/Configure; on failure delete + bail) is identical at all
// 37 sites; the chip-icon / multiplayer-head / statz-icon runs are loops.
//
// COVERAGE: the entry dispatch + the full GruntzTab run (6 widgets incl. the
// 5-iteration GRUNTOVEN loop) + the four ResourceTab background widgets + the
// title widget of each remaining tab are reconstructed (~17% fuzzy of the 7629 B).
// The per-item idiom lowers to the same new/ctor/stamp/Configure/AddTail shape as
// retail (verified block-by-block). REMAINING (the follow-up to the full match):
// the ResourceTab BELT/CHIP loops (which index the g_gameReg per-player icon
// table at +0x158, stride 0x238) + the MACHINE/SHREDDER ConfigureEx (slot +0x34)
// widgets, the MultiplayerTab HEAD loop, the StatzTab ARROW/SMALLICONZ widgets,
// and the GameTab WARPSTONE loop (5 iterations gated on g_gameReg->m_68->Probe).
//
// The configure call can throw, with the just-created item live for unwind, so
// retail builds this under a /GX EH frame (flags = "eh").
RVA(0x00102250, 0x1dcd)
i32 CStatusBarMgr::LoadTabSprites() {
    i32 code = m_code; // the Configure `code` arg (saved to [esp+0x10] in retail)
    i32 bx = m_10;     // base x
    i32 by = m_14;     // base y
    CSbConfigItem* it;
    SbRect r;
    i32 i;

    // Cases are emitted in retail's PHYSICAL layout order (jump table @0x504020):
    //   m_10c==2 Gruntz @0x102296, ==3 Resource @0x102787, ==4 Multiplayer @0x103392,
    //   ==1 Statz @0x1038ef, ==5 Game @0x103bfb. Every Configure passes arg2 = `code`
    //   (retail: the saved m_code @[esp+0x10]).
    switch (m_10c) {
        case 2: // ---- Gruntz tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    code,
                    0x25c,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            // GRUNTOVEN: a vertical column of 5 CSBI_ImageSet oven slots (x fixed at
            // bx+0xe..bx+0x39, y steps by 0x36); each caches its item at m_204[i] and its
            // format source at m_224[i] (stride 0x18), then GetSel + SetAllTypes/Formats.
            for (i = 0; i < 5; i++) {
                it = (CSbConfigItem*)new CSBI_ImageSet;
                r.left = bx + 0xe;
                r.top = by + 0xcc + i * 0x36;
                r.right = bx + 0x39;
                r.bottom = by + 0xfe + i * 0x36;
                if (!it->Configure(
                        this,
                        code,
                        0x64 + i,
                        2,
                        r,
                        "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                        (&m_224)[i * 6],
                        0
                    )) {
                    if (it) {
                        delete it;
                    }
                    return 0;
                }
                m_64.AddTail(it);
                (&m_204)[i] = (i32)it;
                i32 sel = g_gameReg->m_spriteFactory->GetSel(
                    *(i32*)((char*)g_gameReg + 0x158 + g_curPlayer * 0x238),
                    0
                );
                if (sel == 0) {
                    sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                }
                ((CSbItemHelp*)((CSBI_ImageSet*)it)->m_34)->Init(10);
                ((CSbItemHelp*)((CSBI_ImageSet*)it)->m_34)->Push(sel);
            }
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x4c;
            r.top = by + 0xc8;
            r.right = bx + 0x97;
            r.bottom = by + 0x1cd;
            if (!it->Configure(this, code, 0x69, 2, r, "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL", -1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            m_218 = (i32)it;
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x1e;
            r.top = by + 0xc4;
            r.right = bx + 0x3d;
            r.bottom = by + 0xcd;
            if (!it->Configure(
                    this,
                    code,
                    0x6b,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_OVENZTEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x68;
            r.top = by + 0x1cf;
            r.right = bx + 0x87;
            r.bottom = by + 0x1d8;
            if (!it->Configure(
                    this,
                    code,
                    0x6c,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLTEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            it = (CSbConfigItem*)new CSBI_WellGoo;
            r.left = bx + 0x6e;
            r.top = by + 0xf8;
            r.right = bx + 0xef;
            r.bottom = by + 0x1b3;
            if (!it->Configure(this, code, 0x6a, 2, r, "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO", m_298, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            m_21c = (i32)it;
            return 1;

        case 3: // ---- Resource tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    code,
                    0x25c,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx;
            r.top = by + 0x135;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1be;
            if (!it->Configure(
                    this,
                    code,
                    0xc8,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MAINBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_364 = (i32)it;
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx;
            r.top = by + 0xfb;
            r.right = bx + 0x9f;
            r.bottom = by + 0x134;
            if (!it->Configure(
                    this,
                    code,
                    0xc9,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_UPPERBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_36c = (i32)it;
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x48;
            r.top = by + 0xd3;
            r.right = bx + 0x67;
            r.bottom = by + 0xf3;
            if (!it->Configure(
                    this,
                    code,
                    0xca,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_WINDOWBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_370 = (i32)it;
            return 1;

        case 4: // ---- Multiplayer tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    code,
                    0x25c,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_9c.AddTail(it);
            return 1;

        case 1: // ---- Statz tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    code,
                    0x25c,
                    1,
                    r,
                    "GAME_STATUSBAR_TABZ_STATZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_48.AddTail(it);
            return 1;

        case 5: // ---- Game tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    code,
                    0x25c,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_b8.AddTail(it);
            // WARPSTONE: item 0 unconditional, items 1..4 each gated on m_cmdGrid->Probe(i).
            it = (CSbConfigItem*)new CSBI_ImageSet;
            r.left = bx;
            r.top = by;
            r.right = bx + 0x9f;
            r.bottom = by + 0x7f;
            if (!it->Configure(this, code, 0x2bc, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", 1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_b8.AddTail(it);
            if (((CSbIconSet*)g_gameReg->m_cmdGrid)->Probe(1)) {
                it = (CSbConfigItem*)new CSBI_ImageSet;
                r.left = bx + 0x17;
                r.top = by + 0xe;
                r.right = bx + 0x52;
                r.bottom = by + 0x44;
                if (!it->Configure(this, code, 0x2bd, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", 2, 0)) {
                    if (it) {
                        delete it;
                    }
                    return 0;
                }
                m_b8.AddTail(it);
                if (((CSbIconSet*)g_gameReg->m_cmdGrid)->Probe(2)) {
                    it = (CSbConfigItem*)new CSBI_ImageSet;
                    r.left = bx + 0x4c;
                    r.top = by + 0xf;
                    r.right = bx + 0x87;
                    r.bottom = by + 0x3e;
                    if (!it->Configure(this, code, 0x2be, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", 3, 0)) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_b8.AddTail(it);
                    if (((CSbIconSet*)g_gameReg->m_cmdGrid)->Probe(3)) {
                        it = (CSbConfigItem*)new CSBI_ImageSet;
                        r.left = bx + 0x1b;
                        r.top = by + 0x3b;
                        r.right = bx + 0x52;
                        r.bottom = by + 0x71;
                        if (!it->Configure(this, code, 0x2bf, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", 4, 0)) {
                            if (it) {
                                delete it;
                            }
                            return 0;
                        }
                        m_b8.AddTail(it);
                        if (((CSbIconSet*)g_gameReg->m_cmdGrid)->Probe(4)) {
                            it = (CSbConfigItem*)new CSBI_ImageSet;
                            r.left = bx + 0x4a;
                            r.top = by + 0x35;
                            r.right = bx + 0x89;
                            r.bottom = by + 0x74;
                            if (!it->Configure(
                                    this,
                                    code,
                                    0x2c0,
                                    5,
                                    r,
                                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                    5,
                                    0
                                )) {
                                if (it) {
                                    delete it;
                                }
                                return 0;
                            }
                            m_b8.AddTail(it);
                        }
                    }
                }
            }
            BuildGameMenu();
            return 1;
    }
    return 1;
}
