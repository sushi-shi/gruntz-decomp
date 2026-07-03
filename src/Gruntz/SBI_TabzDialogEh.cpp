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
// new(0x34|0x3c) -> out-of-line base ctor -> compiler-stamped retail vtable
// (??_7CSBI_Image / ??_7CSBI_MenuItem / ??_7CSBI_ImageSet, auto-named via
// config/vtable_names.csv) + type tag m_8=3/2/4 + per-item field clears -> the
// slot-0x2c setup call. On setup==0: `delete item` (the slot-0 scalar-deleting dtor)
// + return 0. On success: m_d4.AddTail(item) + the per-command store.
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

#include <Gruntz/GruntzMgr.h> // canonical MFC-side g_gameReg singleton view (CGruntzMgr)
#include <Gruntz/SbRect.h>    // the by-value geometry rect the slot-0x2c setup takes (arg5..8);
                              // built as a temp per call (sub esp,0x10 + four stores) via its ctor

// Throwing global operator new (engine _RezAlloc @0x1b9b46); no body -> reloc-masked.
void* operator new(u32 n);

// The ?g_pCopyRect@@3P6GXPAUtagRECT@@PBU1@@ZA global fn-pointer (VA 0x6c44bc): a
// __stdcall RECT copier called `call ds:[g_pCopyRect]`. Reloc-masked DATA().
DATA(0x002c44bc)
extern void(WINAPI* g_pCopyRect)(RECT* dst, const RECT* src);

#include <Gruntz/SbiTabzDialogViews.h> // CSbDialogItem builder-facet + SBI leaves + Tabz* + CTabzBuilder

// The CSbDialogItem builder-facet base, its concrete SBI leaves (CSBI_Image /
// CSBI_MenuItem / CSBI_ImageSet), the singleton/active-level views (TabzGmFactory /
// TabzPlayer), the RECT-holder views (TabzRectHolder / TabzSub) and the CTabzBuilder
// host moved to <Gruntz/SbiTabzDialogViews.h>.

// The canonical MFC-side CGruntzMgr view of the singleton (*0x24556c). The +0x68
// active-level/mission object (m_cmdGrid, cast to TabzGmFactory) and the +0x174
// per-player table (stride 0x238, cast to TabzPlayer) are reached via local cast /
// raw offset; the +0x134 test-mode gate matches directly.
DATA(0x0024556c)
extern CGruntzMgr* g_gameReg;

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
        CSBI_Image* areYouSure = new CSBI_Image;
        if (!areYouSure->Setup(
                this,
                m_c,
                0x321,
                6,
                SbRect(cx - 0x5e, cy - 0x3c, cx + 0x5e, cy + 0x3d),
                "GAME_STATUSBAR_TABZ_DIALOG_AREYOUSURE",
                -1,
                0
            )) {
            delete areYouSure;
            return 0;
        }
        m_d4.AddTail((CObject*)areYouSure);

        CSBI_MenuItem* yes = new CSBI_MenuItem;
        if (!yes->Setup(
                this,
                m_c,
                0x327,
                6,
                SbRect(cx - 0x45, cy + 0x11, cx - 0x12, cy + 0x28),
                "GAME_STATUSBAR_TABZ_DIALOG_YES",
                -1,
                0
            )) {
            delete yes;
            return 0;
        }
        m_d4.AddTail((CObject*)yes);
        m_1fc = yes;

        CSBI_MenuItem* no = new CSBI_MenuItem;
        if (!no->Setup(
                this,
                m_c,
                0x328,
                6,
                SbRect(cx + 0xd, cy + 0x11, cx + 0x40, cy + 0x28),
                "GAME_STATUSBAR_TABZ_DIALOG_NO",
                -1,
                0
            )) {
            delete no;
            return 0;
        }
        m_d4.AddTail((CObject*)no);
        m_200 = no;
        return 1;
    }

    // ---- main tabz dialog: DIALOG then a mission/mode decision tree ----
    CSBI_Image* dialog = new CSBI_Image;
    if (!dialog->Setup(
            this,
            m_c,
            0x321,
            6,
            SbRect(cx - 0x8e, cy - 0x48, cx + 0x8e, cy + 0x48),
            "GAME_STATUSBAR_TABZ_DIALOG",
            -1,
            0
        )) {
        delete dialog;
        return 0;
    }
    m_d4.AddTail((CObject*)dialog);

    i32 reason = ((TabzGmFactory*)g_gameReg->m_cmdGrid)->m_3ec;

    if (((TabzGmFactory*)g_gameReg->m_cmdGrid)->m_288 == 1) {
        // mission accomplished
        CSBI_ImageSet* status = new CSBI_ImageSet;
        if (!status->Setup(
                this,
                m_c,
                0x322,
                6,
                SbRect(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
                "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
                1,
                0
            )) {
            delete status;
            return 0;
        }
        m_d4.AddTail((CObject*)status);

        CSBI_ImageSet* rsn = new CSBI_ImageSet;
        if (!rsn->Setup(
                this,
                m_c,
                0x326,
                6,
                SbRect(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
                "GAME_STATUSBAR_TABZ_DIALOG_REASON",
                reason,
                0
            )) {
            delete rsn;
            return 0;
        }
        m_d4.AddTail((CObject*)rsn);

        if (g_gameReg->m_134 == 1) {
            CSBI_MenuItem* next = new CSBI_MenuItem;
            if (!next->Setup(
                    this,
                    m_c,
                    0x324,
                    6,
                    SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                    "GAME_STATUSBAR_TABZ_DIALOG_PLAYNEXTLEVEL",
                    -1,
                    0
                )) {
                delete next;
                return 0;
            }
            m_d4.AddTail((CObject*)next);
            m_1f4 = next;

            CSBI_MenuItem* quit = new CSBI_MenuItem;
            if (!quit->Setup(
                    this,
                    m_c,
                    0x325,
                    6,
                    SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                    "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                    -1,
                    0
                )) {
                delete quit;
                return 0;
            }
            m_d4.AddTail((CObject*)quit);
            m_1f8 = quit;
        } else {
            CSBI_MenuItem* statz = new CSBI_MenuItem;
            if (!statz->Setup(
                    this,
                    m_c,
                    0x325,
                    6,
                    SbRect(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
                    "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                    -1,
                    0
                )) {
                delete statz;
                return 0;
            }
            m_d4.AddTail((CObject*)statz);
            m_1f8 = statz;
        }
        return 1;
    }

    // mission not complete
    CSBI_ImageSet* status = new CSBI_ImageSet;
    if (!status->Setup(
            this,
            m_c,
            0x322,
            6,
            SbRect(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
            "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
            2,
            0
        )) {
        delete status;
        return 0;
    }
    m_d4.AddTail((CObject*)status);

    CSBI_ImageSet* rsn = new CSBI_ImageSet;
    if (!rsn->Setup(
            this,
            m_c,
            0x326,
            6,
            SbRect(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
            "GAME_STATUSBAR_TABZ_DIALOG_REASON",
            reason,
            0
        )) {
        delete rsn;
        return 0;
    }
    m_d4.AddTail((CObject*)rsn);

    if (g_gameReg->m_134 == 1) {
        CSBI_MenuItem* replay = new CSBI_MenuItem;
        if (!replay->Setup(
                this,
                m_c,
                0x324,
                6,
                SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_REPLAYLEVEL",
                -1,
                0
            )) {
            delete replay;
            return 0;
        }
        m_d4.AddTail((CObject*)replay);
        m_1f4 = replay;

        CSBI_MenuItem* quit = new CSBI_MenuItem;
        if (!quit->Setup(
                this,
                m_c,
                0x325,
                6,
                SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                -1,
                0
            )) {
            delete quit;
            return 0;
        }
        m_d4.AddTail((CObject*)quit);
        m_1f8 = quit;
        return 1;
    }

    // count active players (m_178!=0 && m_17c==0 && m_174==0) over the 4 slots.
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        if (((TabzPlayer*)((char*)g_gameReg + 0x174))[i].m_178 != 0
            && ((TabzPlayer*)((char*)g_gameReg + 0x174))[i].m_17c == 0
            && ((TabzPlayer*)((char*)g_gameReg + 0x174))[i].m_174 == 0) {
            count++;
        }
    }

    if (count >= 2) {
        CSBI_MenuItem* observe = new CSBI_MenuItem;
        if (!observe->Setup(
                this,
                m_c,
                0x324,
                6,
                SbRect(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_OBSERVE",
                -1,
                0
            )) {
            delete observe;
            return 0;
        }
        m_d4.AddTail((CObject*)observe);
        m_1f4 = observe;
        m_578 = 1;

        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->Setup(
                this,
                m_c,
                0x325,
                6,
                SbRect(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                -1,
                0
            )) {
            delete statz;
            return 0;
        }
        m_d4.AddTail((CObject*)statz);
        m_1f8 = statz;
    } else {
        m_578 = 0;
        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->Setup(
                this,
                m_c,
                0x325,
                6,
                SbRect(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                -1,
                0
            )) {
            delete statz;
            return 0;
        }
        m_d4.AddTail((CObject*)statz);
        m_1f8 = statz;
    }
    return 1;
}
