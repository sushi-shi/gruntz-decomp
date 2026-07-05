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
// Per-tab builder. Every Configure passes arg2 = `code` (the saved m_code); `type` =
// tab index (1..5), rect = base coords (bx=m_10/by=m_14) + per-item offsets. The per-item
// create idiom (new/base-ctor/stamp/Configure; on fail delete + bail) is identical at ~37
// sites; the GRUNTOVEN/CHIP/HEAD/SMALLICONZ/WARPSTONE runs are loops. Built under a /GX EH
// frame (the just-created item is EH-rolled-back if a later Configure throws).
//
// @early-stop
// ~44.8% (was 24.1%); REGISTER LAYOUT NOW MATCHES retail's Order-A (this->esi, bx->ebx,
// by->ebp). THE UNLOCK (docs/patterns/gx-this-esi-via-cache-store-pressure.md): the out-of-line
// CSbConfigItem base ctor (CSBCONFIGITEM_OUTOFLINE_CTOR) raises the /GX frame, but on the
// bare partial it lands this->ebp/edi (Order-B) - retail keeps `code` in MEMORY [esp+0x10]
// and dedicates edi to the zero-constant, which needs the WHOLE body's register pressure.
// Reconstructing the esi-relative cache stores (m_204/m_218/m_224/m_364..m_628) + the Game
// WARPSTONE run + Resource BELT + Multiplayer HEAD slots/loop supplied that pressure: cl
// spills `code`, dedicates edi=0, and puts `this` in esi - byte-exact retail prologue. Cases
// are emitted in retail PHYSICAL order (Gruntz/Resource/Multiplayer/Statz/Game). COMPLETE
// cases: Gruntz (TITLE+GRUNTOVEN loop+WELL/OVENZ/WELLTEXT/WELLGOO), Game (TITLE+WARPSTONE
// Probe-gated run+BuildGameMenu). PARTIAL: Resource (through BELT), Multiplayer (through HEAD
// loop), Statz (TITLE).
//
// RESIDUAL (the ConfigureEx/cross-call/ebp-reuse tails - each raises %, none needs a new idea,
// just the byte-level decode of the ebp-reuse loops): ResourceTab SHREDDER (GREYCHIPZ/NORMCHIPZ
// ConfigureEx loop, ebp=item) + MACHINE (CSBI_GruntMachine 0x48 -> BuildResourceTabStatusBar
// cross-call, inline ctor + manual vtable 0x5eadbc); MultiplayerTab WARLORDHEAD (0x88 ->
// BuildMultiplayerTabStatusBar); StatzTab mode-gate + SMALLICONZ 15-iter loop (CSBI_StatzTab-
// GruntBar ConfigureEx + SetA/SetDirection + WARLORDHEAD). Stack frame 0x28 vs retail 0x34
// (the missing ebp-reuse loop induction locals shift [esp+N] - closes as those loops land).
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
            // bx+0xe..bx+0x39, y steps by 0x36); each caches its item at *aptr (m_204..)
            // and reads its format source from *bptr (m_224.., stride 0x18), then
            // GetSel + SetAllTypes/Formats. Pointer locals (aptr/bptr/y) match retail's
            // incremented [esp+0x18]/[esp+0x28]/[esp+0x20] induction variables.
            {
                i32* aptr = &m_204;
                i32* bptr = &m_224;
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    it = (CSbConfigItem*)new CSBI_ImageSet;
                    r.left = bx + 0xe;
                    r.top = y - 0x32;
                    r.right = bx + 0x39;
                    r.bottom = y;
                    if (!it->Configure(
                            this,
                            code,
                            0x64 + i,
                            2,
                            r,
                            "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                            *bptr,
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_64.AddTail(it);
                    *aptr = (i32)it;
                    i32 sel = g_gameReg->m_spriteFactory->GetSel(
                        *(i32*)((char*)g_gameReg + 0x158 + g_curPlayer * 0x238),
                        0
                    );
                    if (sel == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                    }
                    ((CSbItemHelp*)((CSBI_ImageSet*)it)->m_34)->Init(10);
                    ((CSbItemHelp*)((CSBI_ImageSet*)it)->m_34)->Push(sel);
                    aptr++;
                    bptr += 6;
                    y += 0x36;
                }
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
            // BELT: 3 unrolled CSBI_ImageSet belt segments (config-d from m_2c4/m_2dc/m_2f4,
            // cached to m_308/m_30c/m_310).
            it = (CSbConfigItem*)new CSBI_ImageSet;
            r.left = bx + 0x19;
            r.top = by + 0x11c;
            r.right = bx + 0x3c;
            r.bottom = by + 0x130;
            if (!it->Configure(this, code, 0xcb, 3, r, "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT", m_2c4, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_308 = (i32)it;
            it = (CSbConfigItem*)new CSBI_ImageSet;
            r.left = bx + 0x40;
            r.top = by + 0x11c;
            r.right = bx + 0x63;
            r.bottom = by + 0x130;
            if (!it->Configure(this, code, 0xcc, 3, r, "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT", m_2dc, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_30c = (i32)it;
            it = (CSbConfigItem*)new CSBI_ImageSet;
            r.left = bx + 0x68;
            r.top = by + 0x11c;
            r.right = bx + 0x8b;
            r.bottom = by + 0x130;
            if (!it->Configure(this, code, 0xcd, 3, r, "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT", m_2f4, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_80.AddTail(it);
            m_310 = (i32)it;
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
            // 4 player HEAD slots (CSBI_MultiSlot, y steps 0x43; cached m_61c..m_628).
            it = (CSbConfigItem*)new CSBI_MultiSlot;
            r.left = bx + 0x53;
            r.top = by + 0xcf;
            r.right = bx + 0x8e;
            r.bottom = by + 0x10a;
            if (!it->Configure(this, code, 0x190, 4, r, "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1", 1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_9c.AddTail(it);
            m_61c = (i32)it;
            it = (CSbConfigItem*)new CSBI_MultiSlot;
            r.left = bx + 0x53;
            r.top = by + 0x112;
            r.right = bx + 0x8e;
            r.bottom = by + 0x14d;
            if (!it->Configure(this, code, 0x191, 4, r, "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2", 1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_9c.AddTail(it);
            m_620 = (i32)it;
            it = (CSbConfigItem*)new CSBI_MultiSlot;
            r.left = bx + 0x53;
            r.top = by + 0x155;
            r.right = bx + 0x8e;
            r.bottom = by + 0x190;
            if (!it->Configure(this, code, 0x192, 4, r, "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3", 1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_9c.AddTail(it);
            m_624 = (i32)it;
            it = (CSbConfigItem*)new CSBI_MultiSlot;
            r.left = bx + 0x53;
            r.top = by + 0x197;
            r.right = bx + 0x8e;
            r.bottom = by + 0x1d2;
            if (!it->Configure(this, code, 0x193, 4, r, "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4", 1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_9c.AddTail(it);
            m_628 = (i32)it;
            // HEAD loop: for each active player slot (g_gameReg per-player block, stride
            // 0x238) set the head sprite (GetSel) + SetState/ShowFrames on the cached slot.
            {
                i32* slot = &m_61c;
                i32 pi = 0;
                i32 off = 0;
                do {
                    char* pp = (char*)g_gameReg + off;
                    if (*(i32*)(pp + 0x178) != 0 && *(i32*)(pp + 0x17c) == 0) {
                        i32 sel = g_gameReg->m_spriteFactory->GetSel(*(i32*)(pp + 0x158), 0);
                        if (pi != m_62c) {
                            sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                        }
                        ((CSbConfigItem*)*slot)->SetState(2);
                        ((CSbConfigItem*)*slot)->ShowFrames(0xa, sel);
                    }
                    slot++;
                    pi++;
                    off += 0x238;
                } while (off < 0x8e0);
            }
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
