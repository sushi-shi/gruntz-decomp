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
// The created subclasses' virtuals live in other TUs, so their vtables cannot be
// reproduced here; the vtable address is stamped directly (DATA() externs,
// reloc-masked) as a transitional workaround, NOT dev code.
//
// Switch case -> tab (from the jump table at VA 0x504020):
//   m_10c==1 -> Statz   m_10c==2 -> Gruntz   m_10c==3 -> Resource
//   m_10c==4 -> Multiplayer   m_10c==5 -> Game
#include <rva.h>

#include <Mfc.h>

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

// The created widgets are CStatusBarItem subtypes. The builder calls each one's
// concrete ctor right after operator new, then stamps the retail vtable + type
// tag. The configure work goes through two virtual slots:
//   Configure   = vtable slot +0x2c (key + rect + a handful of ints)
//   ConfigureEx = vtable slot +0x34 (the HEAD/ARROW variant: more ints)
struct SbRect {
    int left;
    int top;
    int right;
    int bottom;
};

class CStatusBarMgr;

// CStatusBarItem subtype "view" - polymorphic so MSVC emits native __thiscall
// vtable dispatch (call [edx+0x2c] etc.). The vtable VALUES are stamped directly
// to the retail CSBI_* tables after construction (the subclass virtuals live in
// other TUs, so the compiler-emitted vtable can't be reproduced); the eleven
// leading placeholder virtuals line Configure up to slot +0x2c and ConfigureEx
// to +0x34. Deleting dtor is slot 0.
class CSbItem {
public:
    virtual ~CSbItem(); // +0x00 (scalar deleting dtor)
    virtual void v04(); // +0x04
    virtual void v08(); // +0x08
    virtual void v0c(); // +0x0c
    virtual void v10(); // +0x10
    virtual void v14(); // +0x14
    virtual void v18(); // +0x18
    virtual void v1c(); // +0x1c
    virtual void v20(); // +0x20
    virtual void v24(); // +0x24
    virtual void v28(); // +0x28
    virtual int Configure(
        CStatusBarMgr* mgr,
        int a,
        int b,
        int c,
        SbRect rect,
        const char* key,
        int d,
        int e
    ); // +0x2c
    virtual int ConfigureEx(
        CStatusBarMgr* mgr,
        int a0,
        SbRect rect,
        const char* key,
        int b,
        int c,
        int d,
        int e,
        int f
    ); // +0x34

    int m_4; // +0x04
    int m_8; // +0x08 type tag (3/4/5/6/7/8/9/0xb)
    char pad[0x24 - 0x0c];
    int m_24; // +0x24
    int m_28; // +0x28
    int m_30; // +0x30
    int m_34; // +0x34
};

// The concrete ctors (reached via thunks; __thiscall on the raw item pointer).
void __fastcall Sbi_CtorBase(CSbItem* p);     // ??0CStatusBarItem@@QAE@XZ (thunk 0x22c0)
void __fastcall Sbi_CtorImageSet(CSbItem* p); // ??0CSBI_ImageSet@@QAE@XZ (thunk 0x1e88)
void __fastcall Sbi_CtorImgList(CSbItem* p);  // ??0... (thunk 0x315c)

// The shared item helpers driven on a freshly created icon-set item.
class CSbItemHelp {
public:
    void Init(int n); // FUN_00552480  @0x152480
    void Push(int v); // FUN_00552520  @0x152520
};

// The icon/sprite factory the resource/game tabs pull chip + warpstone sprites
// from (g_gameReg.m_74 / m_68); __thiscall on the factory.
class CSbFactory {
public:
    void* GetByIndex(int idx, int z); // thunk 0x4165 -> FUN_004e23c0
};
class CSbIconSet {
public:
    int Probe(int a);        // thunk 0x1582 -> FUN_00479b30
    void SetA(int v);        // thunk 0x11e5 -> FUN_004eb830
    void SetB(int a, int b); // thunk 0x23dd -> FUN_004eb740
    void AddRef(int v);      // thunk 0x3b98 -> FUN_004ea170
    void AddRef0(int v);     // thunk 0x1573 -> FUN_004ea0f0
};

// The game registry: factory at +0x68/+0x74, a per-player icon table at +0x158
// (stride for the per-player block: 71*8; per-icon stride inside it: 0x238).
struct CGameReg {
    char m_pad[0x68];
    CSbFactory* m_68; // +0x68
    char m_pad6c[0x74 - 0x6c];
    CSbFactory* m_74; // +0x74
    char m_pad78[0x158 - 0x78];
    int m_158; // +0x158 base of the icon table
};
DATA(0x24556c)
extern CGameReg* g_gameReg; // ?g_gameReg@@3PAUCGameReg@@A @ VA 0x64556c

DATA(0x244c54)
extern int g_curPlayer; // DAT_00644c54

// The eight concrete CSBI subclass vtables (retail addresses; stamped directly).
DATA(0x1eac0c)
extern void* g_vtbl_t3[]; // 0x5eac0c (CSBI_RectOnly, tag 3)
DATA(0x1eac4c)
extern void* g_vtbl_t4[]; // 0x5eac4c (CSBI_ImageSet, tag 4)
DATA(0x1eac94)
extern void* g_vtbl_t5[]; // 0x5eac94 (tag 5)
DATA(0x1eace4)
extern void* g_vtbl_t6[]; // 0x5eace4 (tag 6)
DATA(0x1eadfc)
extern void* g_vtbl_t7[]; // 0x5eadfc (tag 7)
DATA(0x1ead6c)
extern void* g_vtbl_t8[]; // 0x5ead6c (tag 8)
DATA(0x1eadbc)
extern void* g_vtbl_t9[]; // 0x5eadbc (tag 9)
DATA(0x1ead24)
extern void* g_vtbl_tb[]; // 0x5ead24 (tag 0xb)

// ---------------------------------------------------------------------------
// CStatusBarMgr layout (placeholder fields; only offsets are load-bearing).
// ---------------------------------------------------------------------------
class CStatusBarMgr {
public:
    int LoadTabSprites();

    char m_pad00[0xc];
    void* m_c; // +0x0c  the configure-virtual `this`
    int m_10;  // +0x10  base x
    int m_14;  // +0x14  base y
    char m_pad18[0x48 - 0x18];
    CPtrList m_48; // +0x48  Statz tab list
    CPtrList m_64; // +0x64  Gruntz tab list
    CPtrList m_80; // +0x80  Resource tab list
    CPtrList m_9c; // +0x9c  Multiplayer tab list
    CPtrList m_b8; // +0xb8  Game tab list
    char m_padd4[0x10c - 0xd4];
    int m_10c; // +0x10c  current tab selector (1..5)
};

// ===========================================================================
// CStatusBarMgr::LoadTabSprites  @0x102250
// ===========================================================================
// new + concrete ctor + manual vtable/tag stamp (clearing m_30). The ctor and
// vtable address vary by tag; the null-guard + ctor + stamp idiom is identical
// at every one of the ~37 call sites.
static CSbItem* mk(unsigned int sz, void* vtbl, int tag) {
    CSbItem* p = (CSbItem*)operator new(sz);
    if (p) {
        Sbi_CtorBase(p);
        *(void**)p = vtbl;
        p->m_8 = tag;
        p->m_30 = 0;
    } else {
        p = 0;
    }
    return p;
}

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
RVA(0x102250, 0x1dcd)
int CStatusBarMgr::LoadTabSprites() {
    int bx = m_10; // base x
    int by = m_14; // base y
    CSbItem* it;
    SbRect r;
    int i;

    switch (m_10c) {
        case 2: // ---- Gruntz tab ----
            it = mk(0x34, g_vtbl_t3, 3);
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
                it = mk(0x3c, g_vtbl_t4, 4);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x6c, g_vtbl_t7, 7);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
            it = mk(0x34, g_vtbl_t3, 3);
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
