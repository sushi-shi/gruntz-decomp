// StatusBarMgr.cpp - the in-game status-bar per-tab widget builder.
//
// LoadTabSprites (RVA 0x102250, __thiscall, returns int) is the giant per-tab
// builder: it dispatches on the current tab index (m_activeTab, 1..5) and, for that
// tab, creates each CStatusBarItem-derived widget, stamps its retail vtable +
// type tag, configures it from a named sprite-asset key + a geometry rect (built
// as the tab base coords m_10/m_rect14.m_0 plus per-item offsets), and appends it to the
// tab's CPtrList. Built under a /GX EH frame (the items are EH-rolled-back if a
// later configure fails). Owner verified by the member writes:
//   - m_c/m_10/m_rect14.m_0 read at entry (the configure-virtual `this`/base coords)
//   - m_activeTab the tab selector (switch over 1..5)
//   - per-tab CPtrLists at +0x48 (Statz) / +0x64,+0x80 (Gruntz/Resource) /
//     +0x9c (Multiplayer) / +0xb8 (Game), plus many cached item slots
//     (+0x204..+0x520, +0x61c..+0x62c) it stashes the created widgets into.
//   - tab keys are all GAME_STATUSBAR_TABZ_* / GAME_INGAMEICONZ_* assets.
//
// The created widgets are the CANONICAL polymorphic SBI classes (<Gruntz/SBI_*.h>):
// `new CSBI_Image` makes MSVC auto-stamp the retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c,
// catalogued in config/vtable_names.csv), so no manual vtable-pointer stamp is needed.
// This TU used to build them from a PARALLEL hierarchy rooted at a fabricated 15-slot base
// (CSbConfigItem), which made cl emit 60 B vtables here against retail's 48/52 - the same
// mangled name at two byte-lengths, one of which the linker silently discarded. See the
// header note in <Gruntz/StatusBarMgrBuilders.h>.
//
// Switch case -> tab (from the jump table at VA 0x504020):
//   m_activeTab==1 -> Statz   m_activeTab==2 -> Gruntz   m_activeTab==3 -> Resource
//   m_activeTab==4 -> Multiplayer   m_activeTab==5 -> Game
#include <rva.h>
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/CurPlayer.h> // g_curPlayer
#include <Image/ImageSet.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>

#include <Gruntz/SbRect.h>               // the by-value geometry rect the setup virtuals take
#include <Gruntz/SpriteRefTable.h>       // g_gameReg->m_spriteFactory->GetSel (GRUNTOVEN palette)
#include <Gruntz/StatusBarMgrBuilders.h> // canonical CStatusBarMgr + the canonical SBI leaves

// The cmd-grid probed by the warlord-head builders is a CTriggerMgr; ByteTableHas @0x79b30.
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (ByteTableHas)

// The game registry: factory at +0x68/+0x74, a per-player icon table at +0x158
// (stride for the per-player block: 71*8; per-icon stride inside it: 0x238).

// CSbConfigItem::SetDirection (0x0ea0f0) / SetDirectionAlt (0x0ea170) re-homed to
// src/Gruntz/StatusBarTabBuilders.cpp (interval dossier 0x0e8a70: the config-item
// setters sit inside the tab-builders TU, between the side-tab block and
// BuildMultiplayerTabStatusBar).

// ===========================================================================
// CStatusBarMgr::LoadTabSprites  @0x102250
// ===========================================================================
// Per-tab builder. Every Configure passes arg2 = `code` (the saved m_c); `type` =
// tab index (1..5), rect = base coords (bx=m_10/by=m_rect14.m_0) + per-item offsets. The per-item
// create idiom (new/base-ctor/stamp/Configure; on fail delete + bail) is identical at ~37
// sites; the GRUNTOVEN/CHIP/HEAD/SMALLICONZ/WARPSTONE runs are loops. Built under a /GX EH
// frame (the just-created item is EH-rolled-back if a later Configure throws).
//
// @early-stop
// ~83.7% (was 24.1% -> 44.8% -> here); the BODY IS COMPLETE - all five tabs fully
// reconstructed in retail PHYSICAL order (Gruntz/Resource/Multiplayer/Statz/Game):
//   Gruntz    - TITLE + GRUNTOVEN loop + WELL/OVENZ/WELLTEXT/WELLGOO
//   Resource  - TITLE + MAIN/UPPER/WINDOW bg + BELT x3 + GREYCHIPZ + SHREDDER 4x3 grid
//               + MACHINE (CSBI_GruntMachine -> BuildResourceTabStatusBar) + machine
//               foreground (CSBI_ImageInline) + 2 conveyor CSBI_ImageSetAni + NORMCHIPZ
//   Multi     - TITLE + 4 WARLORDHEAD slots + HEAD per-player loop + SMALLICONZ 15-loop
//   Statz     - TITLE + mode-gated 15-iter arrow(ConfigureEx)/grunt-bar loop
//   Game      - TITLE + WARPSTONE Probe-gated run + BuildGameMenu
// The prologue is byte-exact Order-A (this->esi, code spilled [esp+0x10], bx->ebx,
// by->ebp; docs/patterns/gx-this-esi-via-cache-store-pressure.md) and every loop
// (SHREDDER/SMALLICONZ/Statz/HEAD) reproduces retail's ebp=item induction byte-for-byte.
//
// RESIDUALS (all documented regalloc/codegen coin-flips, none a logic error):
//  (1) by<->it swap in the SEQUENTIAL tail items: retail spills `by` to [esp+0x20]
//      GLOBALLY (freeing ebp for the item pointer across the MACHINE-tail sequential
//      items) and reads by from memory there; cl here restores `by` to ebp after the
//      SHREDDER loop and spills `it` to a stack slot instead - the same callee-saved
//      coin-flip as the whole-body allocation (only tips with by permanently evicted).
//      This also holds the frame at 0x30 vs retail 0x34 (the missing by-spill dword).
//  (2) m_368 machine-foreground rect.top: retail reads a precomputed stack slot; cl
//      recomputes `leal 0x1a6(ebp)` inline (value is a best-fit placeholder).
//  (3) per-item store-scheduling / vtable-stamp-position coin-flips in the inline ctors.
// All are matcher.md regalloc/scheduling walls; logic + offsets + call shape are exact.
RVA(0x00102250, 0x1dcd)
i32 CStatusBarMgr::LoadTabSprites() {
    CDDrawSurfaceMgr* code = m_c; // the setup arg2 config host (spilled to [esp+0x10] in retail)
    i32 bx = m_10;                // base x
    i32 by = m_rect14.m_0;        // base y
    // The item locals are typed by what each leaf actually IS. `it` covers every widget
    // that has the slot-11 setup (CSBI_ImageSet / WellGoo / WarlordHead all derive from
    // CSBI_Image); the four below do not fit that base and used to be reached by casting
    // through the fabricated CSbConfigItem: CSBI_GruntMachine and CSBI_StatzTabGruntBar
    // derive straight from CStatusBarItem (no slot 11 at all), and the ani/arrow pair need
    // slots 13/14 that CSBI_Image does not have.
    CSBI_Image* it;
    CSBI_ImageSetAni* ani;
    CSBI_StatzTabArrow* arrow;
    CSBI_GruntMachine* mach;
    CSBI_StatzTabGruntBar* bar;
    SbRect r;
    i32 i;

    // Cases are emitted in retail's PHYSICAL layout order (jump table @0x504020):
    //   m_activeTab==2 Gruntz @0x102296, ==3 Resource @0x102787, ==4 Multiplayer @0x103392,
    //   ==1 Statz @0x1038ef, ==5 Game @0x103bfb. Every Configure passes arg2 = `code`
    //   (retail: the saved m_c @[esp+0x10]).
    switch (m_activeTab) {
        case 2: // ---- Gruntz tab ----
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
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
            m_tabLists[2].AddTail(it);
            // GRUNTOVEN: a vertical column of 5 CSBI_ImageSet oven slots (x fixed at
            // bx+0xe..bx+0x39, y steps by 0x36); each caches its item at *aptr (m_204..)
            // and reads its format source from *bptr (m_224.., stride 0x18), then
            // GetSel + SetAllTypes/Formats. Pointer locals (aptr/bptr/y) match retail's
            // incremented [esp+0x18]/[esp+0x28]/[esp+0x20] induction variables.
            {
                i32* aptr = (i32*)m_slotNotify;  // +0x204, stride 4
                i32* bptr = &m_slots[0].m_value; // +0x224, stride 0x18
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    it = new CSBI_ImageSet;
                    r.left = bx + 0xe;
                    r.top = y - 0x32;
                    r.right = bx + 0x39;
                    r.bottom = y;
                    if (!it->SetupImage(
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
                    m_tabLists[2].AddTail(it);
                    *aptr = reinterpret_cast<i32>(it);
                    i32 sel = g_gameReg->m_spriteFactory->GetSel(
                        *(i32*)((char*)g_gameReg + 0x158 + g_curPlayer * 0x238),
                        0
                    );
                    if (sel == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                    }
                    ((CImageSet*)((CSBI_ImageSet*)it)->m_34)->SetAllTypes(10);
                    ((CImageSet*)((CSBI_ImageSet*)it)->m_34)->SetAllFormats(sel);
                    aptr++;
                    bptr += 6;
                    y += 0x36;
                }
            }
            it = new CSBI_Image;
            r.left = bx + 0x4c;
            r.top = by + 0xc8;
            r.right = bx + 0x97;
            r.bottom = by + 0x1cd;
            if (!it->SetupImage(
                    this,
                    code,
                    0x69,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            m_gaugeNotify = (CSbiGaugeNotify*)it;
            it = new CSBI_Image;
            r.left = bx + 0x1e;
            r.top = by + 0xc4;
            r.right = bx + 0x3d;
            r.bottom = by + 0xcd;
            if (!it->SetupImage(
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
            m_tabLists[2].AddTail(it);
            it = new CSBI_Image;
            r.left = bx + 0x68;
            r.top = by + 0x1cf;
            r.right = bx + 0x87;
            r.bottom = by + 0x1d8;
            if (!it->SetupImage(
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
            m_tabLists[2].AddTail(it);
            it = new CSBI_WellGoo;
            r.left = bx + 0x6e;
            r.top = by + 0xf8;
            r.right = bx + 0xef;
            r.bottom = by + 0x1b3;
            if (!it->SetupImage(
                    this,
                    code,
                    0x6a,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO",
                    m_gauge,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            m_gaugeSink = (CSbiGaugeNotify*)it;
            return 1;

        case 3: // ---- Resource tab ----
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
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
            m_tabLists[3].AddTail(it);
            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0x135;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1be;
            if (!it->SetupImage(
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
            m_tabLists[3].AddTail(it);
            m_notify0 = (CSbiSlotPtr*)it;
            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0xfb;
            r.right = bx + 0x9f;
            r.bottom = by + 0x134;
            if (!it->SetupImage(
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
            m_tabLists[3].AddTail(it);
            m_notify2 = (CSbiSlotPtr*)it;
            it = new CSBI_Image;
            r.left = bx + 0x48;
            r.top = by + 0xd3;
            r.right = bx + 0x67;
            r.bottom = by + 0xf3;
            if (!it->SetupImage(
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
            m_tabLists[3].AddTail(it);
            m_notify3 = (CSbiSlotPtr*)it;
            // BELT: 3 unrolled CSBI_ImageSet belt segments (config-d from m_groupSlots[0].m_value/m_groupSlots[1].m_value/m_groupSlots[2].m_value,
            // cached to m_308/m_30c/m_310).
            it = new CSBI_ImageSet;
            r.left = bx + 0x19;
            r.top = by + 0x11c;
            r.right = bx + 0x3c;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcb,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[0].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[0] = (CSbiSlotPtr*)it;
            it = new CSBI_ImageSet;
            r.left = bx + 0x40;
            r.top = by + 0x11c;
            r.right = bx + 0x63;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcc,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[1].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[1] = (CSbiSlotPtr*)it;
            it = new CSBI_ImageSet;
            r.left = bx + 0x68;
            r.top = by + 0x11c;
            r.right = bx + 0x8b;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcd,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[2].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[2] = (CSbiSlotPtr*)it;
            // GREYCHIPZ: one CSBI_ImageSet whose rect is built from the cached
            // rect members m_itemRectL..m_520 + bx/by; config source m_extraNotifyArg0, id 0xdf.
            it = new CSBI_ImageSet;
            r.left = m_itemRectL + bx;
            r.top = m_itemRectT + by;
            r.right = m_itemRectR + bx;
            r.bottom = m_itemRectB + by;
            if (!it->SetupImage(
                    this,
                    code,
                    0xdf,
                    3,
                    r,
                    "GAME_INGAMEICONZ_GREYCHIPZ",
                    m_extraNotifyArg0,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_extraNotify0 = (CSbiSlotPtr*)it;
            it->m_4 = 0;
            // SHREDDER: a 4-row x 3-column NORMCHIPZ grid. y steps 0x20 per row;
            // the three columns sit at x = bx+0x1d/0x45/0x6d (width 0x17, top y-0x17).
            // The config sources are read off a column-1 pointer (&m_3dc[row]) at
            // -0x60/0/+0x60; the created items cache off a column-1 pointer
            // (&m_4a8[row]) at -0x10/0/+0x10; the id `c` walks 0xd7.. and each column
            // uses c-4/c/c+4. ebp is reused as the item pointer (by is dead here).
            {
                i32* cfgp = &m_hlGrid[4].m_handle;  // +0x3dc, stride 0x18
                i32* cachep = (i32*)&m_hlNotify[4]; // +0x4a8, stride 4
                i32 y = by + 0x155;
                i32 c = 0xd7;
                for (i = 0; i < 4; i++) {
                    it = new CSBI_ImageSet;
                    r.left = bx + 0x1d;
                    r.top = y - 0x17;
                    r.right = bx + 0x34;
                    r.bottom = y;
                    if (!it->SetupImage(
                            this,
                            code,
                            c - 4,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[-24],
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(it);
                    cachep[-4] = reinterpret_cast<i32>(it);
                    it = new CSBI_ImageSet;
                    r.left = bx + 0x45;
                    r.top = y - 0x17;
                    r.right = bx + 0x5c;
                    r.bottom = y;
                    if (!it->SetupImage(
                            this,
                            code,
                            c,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[0],
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(it);
                    cachep[0] = reinterpret_cast<i32>(it);
                    it = new CSBI_ImageSet;
                    r.left = bx + 0x6d;
                    r.top = y - 0x17;
                    r.right = bx + 0x84;
                    r.bottom = y;
                    if (!it->SetupImage(
                            this,
                            code,
                            c + 4,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[24],
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(it);
                    cachep[4] = reinterpret_cast<i32>(it);
                    cfgp += 6;
                    cachep += 1;
                    y += 0x20;
                    c++;
                }
            }
            // MACHINE: a CSBI_GruntMachine built through BuildResourceTabStatusBar
            // (its retail ctor inlines the base -> manual vptr stamp), cross-configured
            // from the two idx sources m_hudRectA_y/m_hudRectB_y.
            mach = new CSBI_GruntMachine;
            r.left = bx;
            r.top = by + 0xc8;
            r.right = bx + 0x9f;
            r.bottom = by + 0xfa;
            if (!mach->BuildResourceTabStatusBar(
                    this,
                    code,
                    0xd1,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND",
                    m_hudRectA_y,
                    m_hudRectB_y
                )) {
                if (mach) {
                    delete mach;
                }
                return 0;
            }
            m_machineDisplay = mach;
            m_tabLists[3].AddTail(mach);
            // Machine foreground: a CSBI_Image whose retail ctor is inlined at the site.
            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0x1a6; // @refine: retail reads a precomputed slot here
            r.right = bx + 0x9f;
            r.bottom = by + 0x1df;
            if (!it->SetupImage(
                    this,
                    code,
                    0xd2,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEFOREGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_notify1 = (CSbiSlotPtr*)it;
            // Conveyor top (CSBI_ImageSetAni via its slot-13 Init).
            ani = new CSBI_ImageSetAni;
            r.left = bx;
            r.top = by + 0x1bf;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1cc;
            if (!ani->Init(
                    this,
                    code,
                    0xce,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_CONVEYORTOP",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                if (ani) {
                    delete ani;
                }
                return 0;
            }
            m_tabLists[3].AddTail(ani);
            // Machine chip well (CSBI_ImageSet, rect from cached m_fallRectL..m_510, cache m_500).
            it = new CSBI_ImageSet;
            r.left = m_fallRectL + bx;
            r.top = m_fallRectT + by;
            r.right = m_fallRectR + bx;
            r.bottom = m_fallRectB + by;
            if (!it->SetupImage(
                    this,
                    code,
                    0xe0,
                    3,
                    r,
                    "GAME_INGAMEICONZ_NORMCHIPZ",
                    m_extraNotifyArg1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_extraNotify1 = (CSbiSlotPtr*)it;
            it->m_4 = 0;
            // Conveyor bottom (CSBI_ImageSetAni via its slot-13 Init).
            ani = new CSBI_ImageSetAni;
            r.left = bx;
            r.top = by + 0x1c7;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1df;
            if (!ani->Init(
                    this,
                    code,
                    0xd0,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_CONVEYORBOTTOM",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                if (ani) {
                    delete ani;
                }
                return 0;
            }
            m_tabLists[3].AddTail(ani);
            return 1;

        case 4: // ---- Multiplayer tab ----
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
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
            m_tabLists[4].AddTail(it);
            // 4 player HEAD slots (CSBI_WarlordHead, y steps 0x43; cached m_61c..m_628).
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0xcf;
            r.right = bx + 0x8e;
            r.bottom = by + 0x10a;
            if (!it->SetupImage(
                    this,
                    code,
                    0x190,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_61c[0] = reinterpret_cast<i32>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x112;
            r.right = bx + 0x8e;
            r.bottom = by + 0x14d;
            if (!it->SetupImage(
                    this,
                    code,
                    0x191,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_61c[1] = reinterpret_cast<i32>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x155;
            r.right = bx + 0x8e;
            r.bottom = by + 0x190;
            if (!it->SetupImage(
                    this,
                    code,
                    0x192,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_61c[2] = reinterpret_cast<i32>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x197;
            r.right = bx + 0x8e;
            r.bottom = by + 0x1d2;
            if (!it->SetupImage(
                    this,
                    code,
                    0x193,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_61c[3] = reinterpret_cast<i32>(it);
            // HEAD loop: for each active player slot (g_gameReg per-player block, stride
            // 0x238) set the head sprite (GetSel) + SetState/ShowFrames on the cached slot.
            {
                i32* slot = m_61c; // +0x61c
                i32 pi = 0;
                i32 off = 0;
                do {
                    i32 sel;
                    if (*(i32*)((char*)g_gameReg + off + 0x178) != 0
                        && *(i32*)((char*)g_gameReg + off + 0x17c) == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(
                            *(i32*)((char*)g_gameReg + off + 0x158),
                            0
                        );
                        if (pi == m_tabCycle) {
                            ((CSBI_WarlordHead*)*slot)->SetState(1);
                        }
                    } else {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                        ((CSBI_WarlordHead*)*slot)->SetState(2);
                    }
                    // The cached m_61c slots ARE the four CSBI_WarlordHead items created
                    // just above; SetState/ShowFrames are that class's own helpers
                    // (0xeb830 / 0xeb740), not the fabricated base's.
                    ((CSBI_WarlordHead*)*slot)->ShowFrames(0xa, (ShadeDescr*)sel);
                    slot++;
                    pi++;
                    off += 0x238;
                } while (off < 0x8e0);
            }
            // SMALLICONZ: 15 per-player CSBI_StatzTabGruntBar stat bars built via
            // BuildMultiplayerTabStatusBar. y steps 0x12; the id p3 = 0x13b + k and
            // the per-slot index p11 = k. bx is reused as the loop counter (its two
            // rect offsets bx+0x17/bx+0x52 are precomputed to stack locals).
            {
                i32 by17 = bx + 0x17;
                i32 by52 = bx + 0x52;
                i32 y = by + 0xd9;
                for (i = 0; i < 15; i++) {
                    bar = new CSBI_StatzTabGruntBar;
                    r.left = by17;
                    r.top = y - 0x11;
                    r.right = by52;
                    r.bottom = y;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            0x13b + i,
                            4,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            m_tabCycle,
                            i,
                            0
                        )) {
                        if (bar) {
                            delete bar;
                        }
                        return 0;
                    }
                    m_tabLists[4].AddTail(bar);
                    y += 0x12;
                }
            }
            return 1;

        case 1: // ---- Statz tab ----
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
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
            m_tabLists[1].AddTail(it);
            // Per-grunt STATZ rows: 15 iterations, each an arrow (CSBI_StatzTabArrow via
            // ConfigureEx + SetDirection/mode) plus a stat bar (CSBI_StatzTabGruntBar via
            // BuildMultiplayerTabStatusBar). The arrow's rect x-span (bx+aOff..bx+cOff) is
            // mode-gated on *this; the id p = 0x13b + k. arrows cache to m_18c[k].
            {
                i32 aOff, cOff;
                if (m_position == 1) {
                    aOff = 0x7d;
                    cOff = 0x95;
                } else {
                    aOff = 0xa;
                    cOff = 0x21;
                }
                i32 arrowL = bx + aOff;
                i32 arrowR = bx + cOff;
                i32 y = by + 0xd9;
                i32* p = m_statFlags; // +0x114 (p[0x1e] reaches m_statObj)
                for (i = 0; i < 15; i++) {
                    i32 id = 0x13b + i;
                    arrow = new CSBI_StatzTabArrow;
                    r.left = arrowL;
                    r.top = y - 0x11;
                    r.right = arrowR;
                    r.bottom = y;
                    if (!arrow->Init(
                            this,
                            code,
                            id - 0xf,
                            1,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_ARROW",
                            -1,
                            -1,
                            0x64,
                            0,
                            0
                        )) {
                        if (arrow) {
                            delete arrow;
                        }
                        return 0;
                    }
                    m_tabLists[1].AddTail(arrow);
                    p[0x1e] = reinterpret_cast<i32>(arrow);
                    if (p[0] != 0) {
                        arrow->SetArrowMode(m_position, 0);
                    } else {
                        arrow->SetDirection(m_position, 0);
                    }
                    bar = new CSBI_StatzTabGruntBar;
                    r.left = bx + 0x28;
                    r.top = y - 0x11;
                    r.right = bx + 0x77;
                    r.bottom = y;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            id,
                            1,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            g_curPlayer,
                            i,
                            1
                        )) {
                        if (bar) {
                            delete bar;
                        }
                        return 0;
                    }
                    m_tabLists[1].AddTail(bar);
                    p++;
                    y += 0x12;
                }
            }
            return 1;

        case 5: // ---- Game tab ----
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
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
            m_tabLists[5].AddTail(it);
            // WARPSTONE: item 0 unconditional, items 1..4 each gated on m_cmdGrid->Probe(i).
            it = new CSBI_ImageSet;
            r.left = bx;
            r.top = by;
            r.right = bx + 0x9f;
            r.bottom = by + 0x7f;
            if (!it->SetupImage(
                    this,
                    code,
                    0x2bc,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[5].AddTail(it);
            if (((CTriggerMgr*)g_gameReg->m_cmdGrid)->ByteTableHas(1)) {
                it = new CSBI_ImageSet;
                r.left = bx + 0x17;
                r.top = by + 0xe;
                r.right = bx + 0x52;
                r.bottom = by + 0x44;
                if (!it->SetupImage(
                        this,
                        code,
                        0x2bd,
                        5,
                        r,
                        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                        2,
                        0
                    )) {
                    if (it) {
                        delete it;
                    }
                    return 0;
                }
                m_tabLists[5].AddTail(it);
                if (((CTriggerMgr*)g_gameReg->m_cmdGrid)->ByteTableHas(2)) {
                    it = new CSBI_ImageSet;
                    r.left = bx + 0x4c;
                    r.top = by + 0xf;
                    r.right = bx + 0x87;
                    r.bottom = by + 0x3e;
                    if (!it->SetupImage(
                            this,
                            code,
                            0x2be,
                            5,
                            r,
                            "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                            3,
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_tabLists[5].AddTail(it);
                    if (((CTriggerMgr*)g_gameReg->m_cmdGrid)->ByteTableHas(3)) {
                        it = new CSBI_ImageSet;
                        r.left = bx + 0x1b;
                        r.top = by + 0x3b;
                        r.right = bx + 0x52;
                        r.bottom = by + 0x71;
                        if (!it->SetupImage(
                                this,
                                code,
                                0x2bf,
                                5,
                                r,
                                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                4,
                                0
                            )) {
                            if (it) {
                                delete it;
                            }
                            return 0;
                        }
                        m_tabLists[5].AddTail(it);
                        if (((CTriggerMgr*)g_gameReg->m_cmdGrid)->ByteTableHas(4)) {
                            it = new CSBI_ImageSet;
                            r.left = bx + 0x4a;
                            r.top = by + 0x35;
                            r.right = bx + 0x89;
                            r.bottom = by + 0x74;
                            if (!it->SetupImage(
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
                            m_tabLists[5].AddTail(it);
                        }
                    }
                }
            }
            BuildGameMenu();
            return 1;
    }
    return 1;
}
