// CStatusBarMgr.cpp - the in-game status-bar per-tab widget builder.
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
#include <Gruntz/CGameRegistry.h>

#include <Mfc.h>

#include <Gruntz/SbRect.h> // the by-value geometry rect the Configure virtuals take

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// The created widgets are CStatusBarItem subtypes. The builder calls each one's
// concrete ctor right after operator new, then stamps the retail vtable + type
// tag. The configure work goes through two virtual slots:
//   Configure   = vtable slot +0x2c (key + rect + a handful of ints)
//   ConfigureEx = vtable slot +0x34 (the HEAD/ARROW variant: more ints)
// ---------------------------------------------------------------------------

class CStatusBarMgr;

// FACET 1 of the status-bar item family (CSbConfigItem here / CSbBuildItem in
// SBI_SideTabBuild.cpp / CSbDialogItem in SBI_TabzDialogEh.cpp). These are the SAME
// retail class modeled three genuinely-incompatible ways that CANNOT be one C++
// spelling: FACET 1 = REAL C++ virtuals (native `call [edx+0x2c]` Configure/ConfigureEx
// dispatch); FACET 2 = a member-fn-ptr vtable (`vptr->Delete` scalar-dtor dispatch);
// FACET 3 = a manual-stamp base with an OUT-OF-LINE ctor for the /GX ctor-in-flight EH
// frame. One MSVC5 spelling emits only one of the three shapes, so the facets are
// renamed apart (documented) rather than merged.
//
// CSbConfigItem is the shared base "view": polymorphic so MSVC emits native
// __thiscall vtable dispatch (call [edx+0x2c] etc.). The eleven leading placeholder
// virtuals line Configure up to slot +0x2c and ConfigureEx to +0x34; the deleting
// dtor is slot 0. Each concrete tab widget is a REAL derived class (CSBI_Image /
// CSBI_ImageSet / CSBI_WellGoo below), so `new CSBI_Image` makes cl auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable - no manual vtable stamp. The inline base ctor
// zeroes the base fields the retail ??0CStatusBarItem (0x1005d0) cleared.
class CSbConfigItem {
public:
    // INLINE ctor (deliberately). The retail /GX frame here comes from `new CSBI_X`
    // CALLING the base ctor OUT OF LINE (call 0x1005d0/0x101fa0): the opaque may-throw
    // call makes cl register the operator-delete-on-ctor-throw cleanup and raise the
    // frame (proven on the sibling CGameMenuMgr::BuildGameMenu, 0x101580, which is a
    // COMPLETE body: declaring its base ctor out-of-line took it 37%->63% by emitting
    // the exact Order-A /GX prologue). LoadTabSprites, however, is still a large PARTIAL
    // (only the Gruntz/Resource/title cases), so adding the frame here allocates the
    // /GX prologue in the wrong Order-B register layout (this->ebp instead of ->esi)
    // and REGRESSES the partial (23.7%->20.6%). The out-of-line-ctor unblock only pays
    // off once the WHOLE 7629 B body is reconstructed (register pressure then matches
    // retail's this->esi/Order-A). Kept inline until that full redo; see
    // docs/patterns/gx-frame-outofline-ctor.md.
    CSbConfigItem() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
    }
    virtual ~CSbConfigItem(); // +0x00 (scalar deleting dtor)
    virtual void v04();       // +0x04
    virtual void v08();       // +0x08
    virtual void v0c();       // +0x0c
    virtual void v10();       // +0x10
    virtual void v14();       // +0x14
    virtual void v18();       // +0x18
    virtual void v1c();       // +0x1c
    virtual void v20();       // +0x20
    virtual void v24();       // +0x24
    virtual void v28();       // +0x28
    virtual i32 Configure(
        CStatusBarMgr* mgr,
        i32 a,
        i32 b,
        i32 c,
        SbRect rect,
        const char* key,
        i32 d,
        i32 e
    ); // +0x2c
    virtual i32 ConfigureEx(
        CStatusBarMgr* mgr,
        i32 a0,
        SbRect rect,
        const char* key,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f
    );                                                        // +0x34
    virtual void v34_pad();                                   // +0x34 (filler)
    virtual void v38(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4); // +0x38

    // SetDirection (0xea0f0): pick one of four direction tuples from the two
    // boolean selectors and forward to the +0x38 virtual.
    void SetDirection(i32 a, i32 b); // 0x0ea0f0

    i32 m_4; // +0x04
    i32 m_8; // +0x08 type tag (3/4/5/6/7/8/9/0xb)
    char pad[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c  Setup id (filled by Configure)
    i32 m_30; // +0x30
};
SIZE_UNKNOWN(CSbConfigItem);

// The concrete tab-widget subclasses. `new CSBI_Image` makes MSVC auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c) - the vtable-name catalog in
// config/vtable_names.csv names it on the target side (reloc-masked). The inline
// ctor sets the per-tag fields the retail Construct wrote after the base ctor
// (m_8 = tag, m_30 = 0). Trailing padding pins each operator-new size to retail.
class CSBI_Image : public CSbConfigItem { // vtable 0x5eac0c, size 0x34
public:
    CSBI_Image() {
        m_8 = 3;
        m_30 = 0;
    }
};
SIZE(CSBI_Image, 0x34);

class CSBI_ImageSet : public CSbConfigItem { // vtable 0x5eac4c, size 0x3c
public:
    CSBI_ImageSet() {
        m_30 = 0;
        m_8 = 4;
        m_34 = 0;
    }
    i32 m_34; // +0x34
    char _pad38[0x3c - 0x38];
};
SIZE(CSBI_ImageSet, 0x3c);

class CSBI_WellGoo : public CSbConfigItem { // vtable 0x5eadfc, size 0x6c
public:
    CSBI_WellGoo() {
        m_8 = 7;
        m_30 = 0;
    }
    char _pad34[0x6c - 0x34];
};
SIZE(CSBI_WellGoo, 0x6c);

// The shared item helpers driven on a freshly created icon-set item.
class CSbItemHelp {
public:
    void Init(i32 n); // FUN_00552480  @0x152480
    void Push(i32 v); // FUN_00552520  @0x152520
};
SIZE_UNKNOWN(CSbItemHelp);

// The icon/sprite factory the resource/game tabs pull chip + warpstone sprites
// from (g_gameReg.m_74 / m_68); __thiscall on the factory.
class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z); // thunk 0x4165 -> FUN_004e23c0
};
SIZE_UNKNOWN(CSbFactory);
class CSbIconSet {
public:
    i32 Probe(i32 a);        // thunk 0x1582 -> FUN_00479b30
    void SetA(i32 v);        // thunk 0x11e5 -> FUN_004eb830
    void SetB(i32 a, i32 b); // thunk 0x23dd -> FUN_004eb740
    void AddRef(i32 v);      // thunk 0x3b98 -> FUN_004ea170
};
SIZE_UNKNOWN(CSbIconSet);

// The game registry: factory at +0x68/+0x74, a per-player icon table at +0x158
// (stride for the per-player block: 71*8; per-icon stride inside it: 0x238).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // ?g_gameReg@@3PAUCGameReg@@A @ VA 0x64556c

DATA(0x00244c54)
extern i32 g_curPlayer; // DAT_00644c54

// ---------------------------------------------------------------------------
// CStatusBarMgr layout (placeholder fields; only offsets are load-bearing).
// ---------------------------------------------------------------------------
class CStatusBarMgr {
public:
    i32 LoadTabSprites();

    char m_pad00[0xc];
    void* m_c; // +0x0c  the configure-virtual `this`
    i32 m_10;  // +0x10  base x
    i32 m_14;  // +0x14  base y
    char m_pad18[0x48 - 0x18];
    CPtrList m_48; // +0x48  Statz tab list
    CPtrList m_64; // +0x64  Gruntz tab list
    CPtrList m_80; // +0x80  Resource tab list
    CPtrList m_9c; // +0x9c  Multiplayer tab list
    CPtrList m_b8; // +0xb8  Game tab list
    char m_padd4[0x10c - 0xd4];
    i32 m_10c; // +0x10c  current tab selector (1..5)
};
SIZE_UNKNOWN(CStatusBarMgr);

// ===========================================================================
// CSbConfigItem::SetDirection  (0x0ea0f0)
// ===========================================================================
// Two boolean selectors (a,b) pick one of four direction tuples, forwarded to
// the +0x38 virtual. Reached via thunk 0x1573 from LoadTabSprites + FUN_00504f90.
RVA(0x000ea0f0, 0x5c)
void CSbConfigItem::SetDirection(i32 a, i32 b) {
    if (a == 0) {
        if (b == 0) {
            v38(4, -1, 0, 0, -1);
        } else {
            v38(-1, -1, 1, 0, -1);
        }
    } else {
        if (b == 0) {
            v38(1, -1, 0, 0, -1);
        } else {
            v38(-1, -1, -1, 0, -1);
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
    i32 bx = m_10; // base x
    i32 by = m_14; // base y
    CSbConfigItem* it;
    SbRect r;
    i32 i;

    switch (m_10c) {
        case 2: // ---- Gruntz tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    0,
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
            for (i = 0; i < 5; i++) {
                it = (CSbConfigItem*)new CSBI_ImageSet;
                r.left = bx + 0xe + i * 0x36;
                r.top = by + 0xfe;
                r.right = bx + 0x39 + i * 0x36;
                r.bottom = by + 0xfe;
                if (!it->Configure(
                        this,
                        i,
                        0x25c,
                        2,
                        r,
                        "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                        0,
                        0
                    )) {
                    if (it) {
                        delete it;
                    }
                    return 0;
                }
                m_64.AddTail(it);
            }
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x4c;
            r.top = by + 0xc8;
            r.right = bx + 0x97;
            r.bottom = by + 0x1cd;
            if (!it->Configure(this, 0, 0x69, 2, r, "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL", -1, 0)) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x1e;
            r.top = by + 0xc4;
            r.right = bx + 0x3d;
            r.bottom = by + 0xcd;
            if (!it->Configure(
                    this,
                    0,
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
                    0,
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
            r.right = bx + 0x81 + 0x6e;
            r.bottom = by + 0x1b3;
            if (!it->Configure(
                    this,
                    0,
                    0x6a,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO",
                    0,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_64.AddTail(it);
            return 1;

        case 3: // ---- Resource tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    0,
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
                    0,
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
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx;
            r.top = by + 0xfb;
            r.right = bx + 0x9f;
            r.bottom = by + 0x134;
            if (!it->Configure(
                    this,
                    0,
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
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x48;
            r.top = by + 0xd3;
            r.right = bx + 0x67;
            r.bottom = by + 0xf3;
            if (!it->Configure(
                    this,
                    0,
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
            return 1;

        case 1: // ---- Statz tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    0,
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

        case 4: // ---- Multiplayer tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    0,
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

        case 5: // ---- Game tab ----
            it = (CSbConfigItem*)new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->Configure(
                    this,
                    0,
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
            return 1;
    }
    return 1;
}
