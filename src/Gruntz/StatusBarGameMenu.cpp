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
#include <Gruntz/CGameRegistry.h>

#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

class CGameMenuMgr;

// The GAMETAB widget base. Polymorphic so MSVC emits native __thiscall vtable
// dispatch (call [edx+0x2c] / [edx+0x30]); the concrete CSBI_ImageSet / CSBI_MenuItem
// leaves below auto-stamp the retail vtables. Eleven leading placeholder virtuals line
// Configure up to slot 0x2c. Slot 0 is the scalar-deleting dtor (the fail `delete it`).
// The inline base ctor zeroes the base fields the retail base ctor cleared.
class CSbMenuItem {
public:
    // OUT-OF-LINE ctor (declaration only): retail `new CSBI_X` CALLS the base ctor out
    // of line (call 0x101fa0), so the opaque may-throw call makes cl register the
    // `new`-expression operator-delete-on-ctor-throw cleanup and raise the /GX frame.
    // Folding it inline let cl prove no-throw -> no frame -> 0% (every byte shifted).
    // Reloc-masked call target, so one shared base ctor pairs with retail's per-class
    // ctors. See docs/patterns/gx-frame-outofline-ctor.md.
    CSbMenuItem();
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

    i32 m_enabled; // +0x04  enabled flag (zeroed to disable the widget)
    i32 m_tag;     // +0x08  type tag (2 / 4)
    char pad0c[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    char pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
};
SIZE_UNKNOWN(CSbMenuItem);

// The concrete widget leaves. `new CSBI_ImageSet` / `new CSBI_MenuItem` makes MSVC
// auto-stamp the retail ??_7CSBI_ImageSet@@6B@ (0x5eac4c) / ??_7CSBI_MenuItem@@6B@
// (0x5eab4c) vtables (catalogued in config/vtable_names.csv) - no manual stamp. The
// inline ctor sets the per-tag fields the retail mk* helper wrote after the ctor
// (m_tag = tag, m_34 = m_30 = m_38 = 0). Both are 0x3c bytes (the base CSbMenuItem size).
class CSBI_ImageSet : public CSbMenuItem { // vtable 0x5eac4c, tag 4
public:
    CSBI_ImageSet() {
        m_tag = 4;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
};
SIZE(CSBI_ImageSet, 0x3c);
class CSBI_MenuItem : public CSbMenuItem { // vtable 0x5eab4c, tag 2
public:
    CSBI_MenuItem() {
        m_tag = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
};
SIZE(CSBI_MenuItem, 0x3c);

// The game registry singleton (?g_gameReg, DATA 0x64556c). Only the fields the
// builder touches are modeled.
struct CGmFactory {
    char m_pad[0x288];
    i32 m_variant; // +0x288  MISSIONSTATUS variant selector
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
    i32 m_code;  // +0x0c  configure `code` arg
    i32 m_baseX; // +0x10  base x
    i32 m_baseY; // +0x14  base y
    char m_pad18[0xb8 - 0x18];
    CPtrList m_items; // +0xb8  Game-tab widget list (AddTail)
    char m_padd4[0x110 - (0xb8 + sizeof(CPtrList))];
    i32 m_briefingGate; // +0x110  briefing/MISSIONSTATUS gate (==0x1fb)
    char m_pad114[0x1dc - 0x114];
    CSbMenuItem* m_slotResume;   // +0x1dc  RESUME/PAUSE slot
    CSbMenuItem* m_slotLoad;     // +0x1e0  LOAD slot
    CSbMenuItem* m_slotSave;     // +0x1e4  SAVE slot
    CSbMenuItem* m_slotSettings; // +0x1e8  SETTINGS slot
    CSbMenuItem* m_slotHelp;     // +0x1ec  HELP slot
    CSbMenuItem* m_slotQuit;     // +0x1f0  QUIT slot
    char m_pad1f4[0x354 - 0x1f4];
    i32 m_showResume; // +0x354  show-RESUME gate
    char m_pad358[0x558 - 0x358];
    i32 m_558;           // +0x558
    i32 m_destructState; // +0x55c  DESTRUCT-button state
    char m_pad560[0x570 - 0x560];
    CSbMenuItem* m_slotDestruct; // +0x570  DESTRUCT/MISSIONSTATUS slot
};
SIZE_UNKNOWN(CGameMenuMgr);

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
        if (m_showResume != 0 && g_gameReg->m_c != 0) {
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
    i32 variant = (((CGmFactory*)g_gameReg->m_68)->m_variant == 1) ? 1 : 2;
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
