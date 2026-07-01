// SBI_TabzDialogEh.cpp - the CSBI_RectOnly-host "build the TABZ_DIALOG" factory
// (RVA 0x10a340, 3019 B). Re-homed here from src/Stub/CSBI_Image.cpp (where the
// rtti-vptr heuristic had mislabeled it ~CSBI_Image / Method_10a340) into a
// dedicated /GX (flags="eh") unit so the retail ctor-in-flight EH frame can be
// reproduced.
//
// The host (this=ebx) is a CSBI_RectOnly-family status bar. It gates on m_550,
// copies the m_c sub-object's RECT (m_c->m_24 + 0x10) via the ?g_pCopyRect global
// function pointer, derives the rect centre (cx,cy), then builds one of two
// dialogs by creating status-bar items, configuring each via vtable slot 0x2c
// (the 8-arg setup), appending it to the +0xd4 CObList, and stashing per-command
// items into m_1f4/m_1f8/m_1fc/m_200. Every `new` is a ctor-in-flight /GX scope:
// the out-of-line base ctor is a throw point, so the half-built item is registered
// for delete-on-throw (the incrementing [esp+0x48] trylevel state machine 0..0xb).
//
// Item build idiom (identical to CGameMenuMgr::BuildGameMenu 0x101580): operator
// new(0x34|0x3c) -> out-of-line base ctor -> manual vtable stamp (g_vtbl_t3 /
// g_vtbl_menuItem / g_vtbl_t4, reloc-masked DATA externs) + type tag m_8=3/2/4 +
// per-item field clears -> the slot-0x2c setup call. On setup==0: scalar-delete
// (slot 0) + return 0. On success: m_d4.AddTail(item) + the per-command store.
//
// @early-stop
// Big /GX TABZ_DIALOG builder; ~83.9% fuzzy, LOGIC COMPLETE. The ctor-in-flight /GX
// frame (push -1 / fs:0), the incrementing [esp+N] trylevel state machine, the
// inline operator-new + base-ctor `call` + vtable stamp + field clears at every
// site, the full m_288 (mission-complete) / m_134 (test-mode) / 4-player
// active-count branch tree, every item type+cmd+asset-key+slot-store and the
// scalar-delete fail cleanup are all reproduced. Dominant residuals (verified vs
// llvm-objdump -dr) are documented, non-source-steerable walls:
//  (A) frame size `sub esp,0x3c` vs retail `sub esp,0x30` - retail packs its locals
//      into 3 fewer dwords; every [esp+N] slot is shifted (gx-scoped-local-eh-frame-
//      size.md). Removing the g_mgrSettings cache already took it 0x40->0x3c.
//  (B) `this` regalloc: retail pins this in ebx (cx=esi, cy=edi); our cl picks esi
//      (cx=edi, cy=ebx) - a whole-function register permutation -> DIFF_ARG on most
//      base-pointer operands. Not steerable from source.
//  (C) the ctor-inlining coin-flip: retail emits the out-of-line base-ctor `call`
//      (0x22c0/0x1e88) for the first ~8 items but FULLY INLINES it (m_4/m_24/m_28=0,
//      no trylevel) for the last ~4 (QUITTOMAINMENU/OBSERVE/STATZ) - one `new Item`
//      spelling can only do one or the other.
//  (D) the interwoven shared setup/fail tails (retail tail-merges the delete+return-0
//      epilogues at 0x10a51e/0x10ad3a/0x10aec8/0x10adc6) + the running esi/edi rect
//      strength-reduction (each corner a running add, not an absolute cx+delta) -
//      objdiff alignment desyncs across the fail ladder (big-seh-fuzzy-desync.md,
//      gx-state-machine-scalar-delete-cleanup.md).
// Re-attack after the CSBI_* item ctors are matched (would fix A+C).
#include <rva.h>

#include <Ints.h>
#include <Mfc.h>

// Throwing global operator new (engine _RezAlloc @0x1b9b46); no body -> reloc-masked.
void* operator new(u32 n);

// The by-value geometry rect the slot-0x2c setup takes (arg5..8). Built as a temp
// per call (sub esp,0x10 + four stores). A trivial 4-arg ctor keeps it constructed
// in place in the argument slot.
struct SbRect {
    i32 x0, y0, x1, y1;
    SbRect(i32 a, i32 b, i32 c, i32 d) : x0(a), y0(b), x1(c), y1(d) {}
};

// The retail item vtables (manual-stamp model; the vtable CONTENTS live in other
// unmatched TUs so we stamp the retail addresses directly). Reloc-masked DATA().
DATA(0x001eac0c)
extern void* g_vtbl_t3; // 0x5eac0c  (CSBI_Image, tag 3)
DATA(0x001eab4c)
extern void* g_vtbl_menuItem; // 0x5eab4c  (CSBI_MenuItem, tag 2)
DATA(0x001eac4c)
extern void* g_vtbl_t4; // 0x5eac4c  (CSBI_ImageSet, tag 4)

// The ?g_pCopyRect@@3P6GXPAUtagRECT@@PBU1@@ZA global fn-pointer (VA 0x6c44bc): a
// __stdcall RECT copier called `call ds:[g_pCopyRect]`. Reloc-masked DATA().
DATA(0x002c44bc)
extern void(__stdcall* g_pCopyRect)(RECT* dst, const RECT* src);

// ---------------------------------------------------------------------------
// The status-bar item family. Base carries the manual vtable pointer + the tag;
// the base ctor is OUT-OF-LINE (declared-only) so `new Item` emits the retail
// out-of-line ctor `call` and, being a throw point, the ctor-in-flight /GX frame.
// The derived ctors are inline (they land at each new-site: stamp + field clears).
// Non-polymorphic (manual stamp), so dispatch goes through the SbView cast below.
// ---------------------------------------------------------------------------
class CSbItem {
public:
    CSbItem();  // out-of-line -> the 0x22c0/0x1e88 base-ctor call (throwing -> /GX)
    ~CSbItem(); // declared-only -> keeps the new-expression's delete-on-throw edge

    void* m_vptr; // +0x00  manual-stamp vtable pointer
    i32 m_4;      // +0x04
    i32 m_8;      // +0x08  type tag
    char _pad0c[0x30 - 0x0c];
    i32 m_30; // +0x30
}; // size 0x34

// tag 3 image item (0x34 bytes): stamp t3, m_8=3, clear m_30.
class CItemT3 : public CSbItem {
public:
    CItemT3() {
        *(void**)this = &g_vtbl_t3;
        m_8 = 3;
        m_30 = 0;
    }
}; // size 0x34
SIZE_UNKNOWN(CItemT3);

// tag 2 menu item (0x3c bytes): stamp menuItem, m_8=2, clear m_34/m_30/m_38.
class CItemMenu : public CSbItem {
public:
    CItemMenu() {
        *(void**)this = &g_vtbl_menuItem;
        m_8 = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
    i32 m_34; // +0x34
    i32 m_38; // +0x38
}; // size 0x3c
SIZE_UNKNOWN(CItemMenu);

// tag 4 image-set item (0x3c bytes): clear m_30, stamp t4, m_8=4, clear m_34.
class CItemT4 : public CSbItem {
public:
    CItemT4() {
        m_30 = 0;
        *(void**)this = &g_vtbl_t4;
        m_8 = 4;
        m_34 = 0;
    }
    i32 m_34; // +0x34
    i32 m_38; // +0x38
}; // size 0x3c
SIZE_UNKNOWN(CItemT4);

// Polymorphic VIEW used only for the two vtable dispatches (never instantiated, so
// no ??_7 emitted): the scalar-deleting dtor at slot 0 (the fail cleanup) and the
// 8-arg Setup at slot 0x2c. Casting an item to this lowers `it->Setup(...)` to the
// retail `mov edx,[it]; call [edx+0x2c]` __thiscall dispatch. Same idiom as the
// sibling TUs' CAniElemView / CSbMenuItem.
class SbView {
public:
    virtual void ScalarDtor(i32 flag); // +0x00
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
    virtual i32
    Setup(void* mgr, void* sub, i32 type, i32 idx, SbRect rc, const char* key, i32 flag, i32 e); // +0x2c
};

// ---------------------------------------------------------------------------
// The g_mgrSettings singleton chain (DATA 0x64556c, RVA 0x24556c). Only the fields
// this builder reads are modeled: the mission-complete selector (m_68->m_288), the
// reason code (m_68->m_3ec), the test-mode gate (m_134) and the 4-player active
// table at +0x174 (stride 0x238).
// ---------------------------------------------------------------------------
struct TabzGmFactory {
    char _00[0x288];
    i32 m_288; // +0x288  mission-complete selector
    char _28c[0x3ec - 0x28c];
    i32 m_3ec; // +0x3ec  reason code
};
SIZE_UNKNOWN(TabzGmFactory);
struct TabzPlayer {
    i32 m_174; // rel +0x00 (abs +0x174)
    i32 m_178; // rel +0x04
    i32 m_17c; // rel +0x08
    char _pad[0x238 - 0xc];
};
SIZE_UNKNOWN(TabzPlayer);
struct TabzGameReg {
    char _00[0x68];
    TabzGmFactory* m_68; // +0x68
    char _6c[0x134 - 0x6c];
    i32 m_134; // +0x134  test-mode gate
    char _138[0x174 - 0x138];
    TabzPlayer m_players[4]; // +0x174  active-player table
};
SIZE_UNKNOWN(TabzGameReg);
DATA(0x0024556c)
extern TabzGameReg* g_mgrSettings;

// The host sub-object at +0xc: a two-hop RECT holder (m_c->m_24 + 0x10 = RECT).
struct TabzRectHolder {
    char _00[0x10];
    RECT m_10; // +0x10
};
SIZE_UNKNOWN(TabzRectHolder);
struct TabzSub {
    char _00[0x24];
    TabzRectHolder* m_24; // +0x24
};
SIZE_UNKNOWN(TabzSub);

// ---------------------------------------------------------------------------
// The builder host (a CSBI_RectOnly-family status bar). Placeholder fields; only
// the offsets are load-bearing.
// ---------------------------------------------------------------------------
class CTabzBuilder {
public:
    i32 BuildTabzDialog(); // 0x10a340

    char _00[0x0c];
    TabzSub* m_c; // +0x0c
    char _10[0xd4 - 0x10];
    CObList m_d4; // +0xd4  item list (AddTail)
    char _padd4[0x1f4 - (0xd4 + sizeof(CObList))];
    CSbItem* m_1f4; // +0x1f4
    CSbItem* m_1f8; // +0x1f8
    CSbItem* m_1fc; // +0x1fc
    CSbItem* m_200; // +0x200
    char _204[0x550 - 0x204];
    i32 m_550; // +0x550  active gate
    i32 m_554; // +0x554  confirm-dialog selector
    char _558[0x578 - 0x558];
    i32 m_578; // +0x578  observe/statz-only flag
};
SIZE_UNKNOWN(CTabzBuilder);

// ===========================================================================
// CTabzBuilder::BuildTabzDialog  @0x10a340
// ===========================================================================
RVA(0x0010a340, 0xbcb)
i32 CTabzBuilder::BuildTabzDialog() {
    if (m_550 == 0) {
        return 1;
    }

    RECT src = m_c->m_24->m_10;
    RECT dst;
    g_pCopyRect(&dst, &src);
    i32 cx = dst.left + (dst.right - dst.left) / 2;
    i32 cy = dst.top + (dst.bottom - dst.top) / 2;

    if (m_554 != 0) {
        // ---- confirm dialog: AREYOUSURE + YES/NO ----
        CItemT3* areYouSure = new CItemT3;
        if (!((SbView*)areYouSure)
                 ->Setup(
                     this,
                     m_c,
                     0x321,
                     6,
                     SbRect(cx - 0x5e, cy - 0x3c, cx + 0x5e, cy + 0x3d),
                     "GAME_STATUSBAR_TABZ_DIALOG_AREYOUSURE",
                     -1,
                     0
                 )) {
            if (areYouSure) {
                ((SbView*)areYouSure)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)areYouSure);

        CItemMenu* yes = new CItemMenu;
        if (!((SbView*)yes)
                 ->Setup(
                     this,
                     m_c,
                     0x327,
                     6,
                     SbRect(cx - 0x45, cy + 0x11, cx - 0x12, cy + 0x28),
                     "GAME_STATUSBAR_TABZ_DIALOG_YES",
                     -1,
                     0
                 )) {
            if (yes) {
                ((SbView*)yes)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)yes);
        m_1fc = yes;

        CItemMenu* no = new CItemMenu;
        if (!((SbView*)no)
                 ->Setup(
                     this,
                     m_c,
                     0x328,
                     6,
                     SbRect(cx + 0xd, cy + 0x11, cx + 0x40, cy + 0x28),
                     "GAME_STATUSBAR_TABZ_DIALOG_NO",
                     -1,
                     0
                 )) {
            if (no) {
                ((SbView*)no)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)no);
        m_200 = no;
        return 1;
    }

    // ---- main tabz dialog: DIALOG then a mission/mode decision tree ----
    CItemT3* dialog = new CItemT3;
    if (!((SbView*)dialog)
             ->Setup(
                 this,
                 m_c,
                 0x321,
                 6,
                 SbRect(cx - 0x8e, cy - 0x48, cx + 0x8e, cy + 0x48),
                 "GAME_STATUSBAR_TABZ_DIALOG",
                 -1,
                 0
             )) {
        if (dialog) {
            ((SbView*)dialog)->ScalarDtor(1);
        }
        return 0;
    }
    m_d4.AddTail((CObject*)dialog);

    i32 reason = g_mgrSettings->m_68->m_3ec;

    if (g_mgrSettings->m_68->m_288 == 1) {
        // mission accomplished
        CItemT4* status = new CItemT4;
        if (!((SbView*)status)
                 ->Setup(
                     this,
                     m_c,
                     0x322,
                     6,
                     SbRect(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
                     "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
                     1,
                     0
                 )) {
            if (status) {
                ((SbView*)status)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)status);

        CItemT4* rsn = new CItemT4;
        if (!((SbView*)rsn)
                 ->Setup(
                     this,
                     m_c,
                     0x326,
                     6,
                     SbRect(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
                     "GAME_STATUSBAR_TABZ_DIALOG_REASON",
                     reason,
                     0
                 )) {
            if (rsn) {
                ((SbView*)rsn)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)rsn);

        if (g_mgrSettings->m_134 == 1) {
            CItemMenu* next = new CItemMenu;
            if (!((SbView*)next)
                     ->Setup(
                         this,
                         m_c,
                         0x324,
                         6,
                         SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                         "GAME_STATUSBAR_TABZ_DIALOG_PLAYNEXTLEVEL",
                         -1,
                         0
                     )) {
                if (next) {
                    ((SbView*)next)->ScalarDtor(1);
                }
                return 0;
            }
            m_d4.AddTail((CObject*)next);
            m_1f4 = next;

            CItemMenu* quit = new CItemMenu;
            if (!((SbView*)quit)
                     ->Setup(
                         this,
                         m_c,
                         0x325,
                         6,
                         SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                         "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                         -1,
                         0
                     )) {
                if (quit) {
                    ((SbView*)quit)->ScalarDtor(1);
                }
                return 0;
            }
            m_d4.AddTail((CObject*)quit);
            m_1f8 = quit;
        } else {
            CItemMenu* statz = new CItemMenu;
            if (!((SbView*)statz)
                     ->Setup(
                         this,
                         m_c,
                         0x325,
                         6,
                         SbRect(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
                         "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                         -1,
                         0
                     )) {
                if (statz) {
                    ((SbView*)statz)->ScalarDtor(1);
                }
                return 0;
            }
            m_d4.AddTail((CObject*)statz);
            m_1f8 = statz;
        }
        return 1;
    }

    // mission not complete
    CItemT4* status = new CItemT4;
    if (!((SbView*)status)
             ->Setup(
                 this,
                 m_c,
                 0x322,
                 6,
                 SbRect(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
                 "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
                 2,
                 0
             )) {
        if (status) {
            ((SbView*)status)->ScalarDtor(1);
        }
        return 0;
    }
    m_d4.AddTail((CObject*)status);

    CItemT4* rsn = new CItemT4;
    if (!((SbView*)rsn)
             ->Setup(
                 this,
                 m_c,
                 0x326,
                 6,
                 SbRect(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
                 "GAME_STATUSBAR_TABZ_DIALOG_REASON",
                 reason,
                 0
             )) {
        if (rsn) {
            ((SbView*)rsn)->ScalarDtor(1);
        }
        return 0;
    }
    m_d4.AddTail((CObject*)rsn);

    if (g_mgrSettings->m_134 == 1) {
        CItemMenu* replay = new CItemMenu;
        if (!((SbView*)replay)
                 ->Setup(
                     this,
                     m_c,
                     0x324,
                     6,
                     SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                     "GAME_STATUSBAR_TABZ_DIALOG_REPLAYLEVEL",
                     -1,
                     0
                 )) {
            if (replay) {
                ((SbView*)replay)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)replay);
        m_1f4 = replay;

        CItemMenu* quit = new CItemMenu;
        if (!((SbView*)quit)
                 ->Setup(
                     this,
                     m_c,
                     0x325,
                     6,
                     SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                     "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                     -1,
                     0
                 )) {
            if (quit) {
                ((SbView*)quit)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)quit);
        m_1f8 = quit;
        return 1;
    }

    // count active players (m_178!=0 && m_17c==0 && m_174==0) over the 4 slots.
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_mgrSettings->m_players[i].m_178 != 0 && g_mgrSettings->m_players[i].m_17c == 0 && g_mgrSettings->m_players[i].m_174 == 0) {
            count++;
        }
    }

    if (count >= 2) {
        CItemMenu* observe = new CItemMenu;
        if (!((SbView*)observe)
                 ->Setup(
                     this,
                     m_c,
                     0x324,
                     6,
                     SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                     "GAME_STATUSBAR_TABZ_DIALOG_OBSERVE",
                     -1,
                     0
                 )) {
            if (observe) {
                ((SbView*)observe)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)observe);
        m_1f4 = observe;
        m_578 = 1;

        CItemMenu* statz = new CItemMenu;
        if (!((SbView*)statz)
                 ->Setup(
                     this,
                     m_c,
                     0x325,
                     6,
                     SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                     "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                     -1,
                     0
                 )) {
            if (statz) {
                ((SbView*)statz)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)statz);
        m_1f8 = statz;
    } else {
        m_578 = 0;
        CItemMenu* statz = new CItemMenu;
        if (!((SbView*)statz)
                 ->Setup(
                     this,
                     m_c,
                     0x325,
                     6,
                     SbRect(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
                     "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                     -1,
                     0
                 )) {
            if (statz) {
                ((SbView*)statz)->ScalarDtor(1);
            }
            return 0;
        }
        m_d4.AddTail((CObject*)statz);
        m_1f8 = statz;
    }
    return 1;
}
