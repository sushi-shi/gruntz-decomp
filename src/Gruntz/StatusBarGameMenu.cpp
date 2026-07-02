// StatusBarGameMenu.cpp - the in-game GAMETAB menu builder (RVA 0x101580).
//
// This was mislabeled `CSBI_ImageSet::CSBI_ImageSet` by the rtti-vptr heuristic; it
// is NOT a constructor. It is the status-bar manager's "build the GAME-tab menu"
// method: it creates each GAMETAB widget (RESUME/PAUSE/LOAD/SAVE/SETTINGS/HELP/
// QUIT/DESTRUCT, or MISSIONSTATUS in the briefing variant), stamps its retail vtable
// + type tag, configures it from a GAME_STATUSBAR_TABZ_GAMETAB_* asset key + a
// geometry rect (tab base coords m_10/m_14 plus per-item offsets), appends it to the
// Game-tab CPtrList at +0xb8, and stashes the created widget into a per-command slot
// (m_1dc..m_1f0, m_570). Built under a /GX EH frame (the just-created item is rolled
// back if its Configure throws). Identical create idiom to CStatusBarMgr::LoadTabSprites.
//
// Owner verified by the member writes: m_c (the configure `code` arg), the base coords
// m_10/m_14, the +0xb8 Game CPtrList, the m_110==0x1fb briefing gate, the m_354
// "show RESUME" gate, the m_558/m_55c/m_570 destruct-button state, and the per-command
// slot stores. The widget subclasses' virtuals live in other TUs, so their vtables are
// stamped directly (DATA() externs, reloc-masked) - a transitional workaround, NOT dev
// code. Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
//
// @early-stop
// ~37% /GX menu builder; LOGIC COMPLETE, parked for the final sweep / a leaf-first redo
// once the item ctors are matched. The front gate (m_110==0x1fb briefing branch, the
// m_354 + gameReg->m_c RESUME/PAUSE select), every Configure call (the by-value rect +
// 11-arg slot-0x2c dispatch with the exact GAMETAB asset keys, type codes 0x1f4..0x1fc
// and per-item rects), the +0xb8 AddTail, the per-command slot stores (m_1dc..m_570),
// the gameReg->m_134 mode gates and the slot-0 scalar-delete failure cleanup are all
// reconstructed faithfully. The DOMINANT residual is structural: retail carries a /GX
// EH frame (push -1 / fs:0 + an incrementing per-item state machine) because the item
// constructors (CSBI_ImageSet ctor 0x101fa0, CStatusBarItem ctor 0x1005d0) are INLINED
// and construct destructible CString/CPtrList members, registering each just-created
// item for delete-on-throw. Those ctor bodies live in other (unmatched) TUs, so they
// are modeled as external thunks (Sbi_CtorImageSet/Sbi_CtorBase, reloc-masked) - with
// no inlined destructible temp, our cl emits NO /GX frame, shifting every byte. Until
// the item ctors are reconstructed the frame can't be reproduced. Secondary walls: the
// per-block ctor-inlining coin-flip (0x1e88 vs 0x22c0 vs fully-inlined) and the
// DESTRUCT/MISSIONSTATUS base-coord-advance rect scheduling. Re-attack in the final
// sweep after the CSBI_* ctors land.
#include <rva.h>

#include <Mfc.h>
#include <Gruntz/CGameRegistry.h>

#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

class CGameMenuMgr;

// The GAMETAB widget view. Polymorphic so MSVC emits native __thiscall vtable
// dispatch (call [edx+0x2c] / [edx+0x30]); the vtable VALUES are stamped directly to
// the retail CSBI_* tables after construction. Eleven leading placeholder virtuals
// line Configure up to slot 0x2c. Slot 0 is the scalar-deleting dtor (used by the
// EH cleanup's `delete it`).
class CSbMenuItem {
public:
    virtual ~CSbMenuItem(); // +0x00
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual i32 Configure(
        CGameMenuMgr* mgr,
        i32 code,
        i32 type,
        i32 idx,
        SbRect rect,
        const char* key,
        i32 flag,
        i32 e
    );                            // +0x2c
    virtual void Activate(i32 a); // +0x30

    void* m_vptr; // +0x00
    i32 m_4;      // +0x04
    i32 m_8;      // +0x08  type tag (2 / 4)
    char pad0c[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    char pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
};
SIZE_UNKNOWN(CSbMenuItem);

// The two concrete base ctors (reached via ILT thunks; __thiscall on the raw item).
class CSBI_ImageSet : public CSbMenuItem {
public:
    CSBI_ImageSet(); // ??0CSBI_ImageSet@@QAE@XZ (thunk 0x1e88 -> 0x101fa0)
};
class CStatusBarItem : public CSbMenuItem {
public:
    CStatusBarItem(); // ??0CStatusBarItem@@QAE@XZ (thunk 0x22c0 -> 0x1005d0)
};

// The retail vtables (manual-stamp model; stamped directly). Reloc-masked DATA().
// 0x1eab4c/0x1eac4c realized as ??_7CSBI_MenuItem@@6B@ / ??_7CSBI_ImageSet@@6B@ (their
// dtor *Eh.cpp TUs emit them). DATA pins removed so the compiler vtables win the RVAs;
// the manual stamps below stay as reloc-masked refs (vptr-middle inline-ctor wall).
extern void* g_vtbl_menuItem; // 0x5eab4c (CSBI_MenuItem, tag 2)
extern void* g_vtbl_t4;       // 0x5eac4c (CSBI_ImageSet, tag 4)

// The game registry singleton (?g_gameReg, DATA 0x64556c). Only the fields the
// builder touches are modeled.
struct CGmFactory {
    char m_pad[0x288];
    i32 m_288; // +0x288  MISSIONSTATUS variant selector
};
SIZE_UNKNOWN(CGmFactory);
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CGameMenuMgr layout (placeholder fields; only offsets are load-bearing).
// ---------------------------------------------------------------------------
class CGameMenuMgr {
public:
    void BuildGameMenu(); // 0x101580

    char m_pad00[0xc];
    i32 m_c;  // +0x0c  configure `code` arg
    i32 m_10; // +0x10  base x
    i32 m_14; // +0x14  base y
    char m_pad18[0xb8 - 0x18];
    CPtrList m_b8; // +0xb8  Game-tab widget list (AddTail)
    char m_padd4[0x110 - (0xb8 + sizeof(CPtrList))];
    i32 m_110; // +0x110  briefing/MISSIONSTATUS gate (==0x1fb)
    char m_pad114[0x1dc - 0x114];
    CSbMenuItem* m_1dc; // +0x1dc  RESUME/PAUSE slot
    CSbMenuItem* m_1e0; // +0x1e0  LOAD slot
    CSbMenuItem* m_1e4; // +0x1e4  SAVE slot
    CSbMenuItem* m_1e8; // +0x1e8  SETTINGS slot
    CSbMenuItem* m_1ec; // +0x1ec  HELP slot
    CSbMenuItem* m_1f0; // +0x1f0  QUIT slot
    char m_pad1f4[0x354 - 0x1f4];
    i32 m_354; // +0x354  show-RESUME gate
    char m_pad358[0x558 - 0x358];
    i32 m_558; // +0x558
    i32 m_55c; // +0x55c  DESTRUCT-button state
    char m_pad560[0x570 - 0x560];
    CSbMenuItem* m_570; // +0x570  DESTRUCT/MISSIONSTATUS slot
};
SIZE_UNKNOWN(CGameMenuMgr);

// new + concrete ctor + manual vtable/tag stamp + the three field clears. The ctor
// and vtable address vary by tag; the null-guard idiom is identical at every call.
static CSbMenuItem* mkImageSet(void* vtbl, i32 tag) {
    CSbMenuItem* p = new CSBI_ImageSet;
    *(void**)p = vtbl;
    p->m_8 = tag;
    p->m_34 = 0;
    p->m_30 = 0;
    p->m_38 = 0;
    return p;
}
static CSbMenuItem* mkBase(void* vtbl, i32 tag) {
    CSbMenuItem* p = new CStatusBarItem;
    *(void**)p = vtbl;
    p->m_8 = tag;
    p->m_34 = 0;
    p->m_30 = 0;
    p->m_38 = 0;
    return p;
}

// ===========================================================================
// CGameMenuMgr::BuildGameMenu  @0x101580
// ===========================================================================
RVA(0x00101580, 0x806)
void CGameMenuMgr::BuildGameMenu() {
    i32 code = m_c;
    i32 bx = m_10;
    i32 by = m_14;
    CSbMenuItem* it;
    SbRect r;

    if (m_110 == 0x1fb) {
        // ---- briefing variant: a single MISSIONSTATUS widget ----
        it = mkImageSet(&g_vtbl_t4, 4);
        i32 variant = (((CGmFactory*)g_gameReg->m_68)->m_288 == 1) ? 1 : 2;
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
        m_b8.AddTail(it);
        return;
    }

    // ---- RESUME or PAUSE in the first slot ----
    if (m_354 != 0 && g_gameReg->m_c != 0) {
        it = mkImageSet(&g_vtbl_menuItem, 2);
        r.left = bx;
        r.top = by + 0xd5;
        r.right = bx + 0x9f;
        r.bottom = by + 0xec;
        if (!it->Configure(this, code, 0x1f4, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_RESUME", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_b8.AddTail(it);
    } else {
        it = mkImageSet(&g_vtbl_menuItem, 2);
        r.left = bx;
        r.top = by + 0xd5;
        r.right = bx + 0x9f;
        r.bottom = by + 0xec;
        if (!it->Configure(this, code, 0x1f4, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_b8.AddTail(it);
    }
    m_1dc = it;

    // ---- LOAD ----
    it = mkImageSet(&g_vtbl_menuItem, 2);
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
    m_b8.AddTail(it);
    m_1e0 = it;
    if (g_gameReg->m_134 == 2) {
        it->m_4 = 0;
    }

    // ---- SAVE ----
    it = mkBase(&g_vtbl_menuItem, 2);
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
    m_b8.AddTail(it);
    m_1e4 = it;
    if (g_gameReg->m_134 == 2) {
        it->m_4 = 0;
    }

    // ---- SETTINGS ----
    it = mkBase(&g_vtbl_menuItem, 2);
    r.left = bx;
    r.top = by + 0x14d;
    r.right = bx + 0x9f;
    r.bottom = by + 0x164;
    if (!it->Configure(this, code, 0x1f7, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS", -1, 0)) {
        if (it) {
            delete it;
        }
        return;
    }
    m_b8.AddTail(it);
    m_1e8 = it;

    // ---- HELP ----
    it = mkBase(&g_vtbl_menuItem, 2);
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
    m_b8.AddTail(it);
    m_1ec = it;
    if (g_gameReg->m_134 == 2) {
        it->m_4 = 0;
    }

    // ---- QUIT (inlined ctor in retail) ----
    it = mkImageSet(&g_vtbl_menuItem, 2);
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
    m_b8.AddTail(it);
    m_1f0 = it;

    // ---- DESTRUCT (CSBI_ImageSet, tag 4) ----
    it = mkImageSet(&g_vtbl_t4, 4);
    r.left = bx + 0x22;
    r.top = by + 0x1be;
    r.right = bx + 0x7d;
    r.bottom = by + 0x1d6;
    if (!it->Configure(this, code, 0x1fc, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT", m_55c, 0)) {
        if (it) {
            delete it;
        }
        return;
    }
    m_b8.AddTail(it);
    m_570 = it;
    if (g_gameReg->m_134 != 1) {
        it->m_4 = 0;
        m_55c = 7;
        m_558 = 0;
        m_570->Activate(7);
    }
}
