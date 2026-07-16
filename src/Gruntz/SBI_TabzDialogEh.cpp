// SBI_TabzDialogEh.cpp - CTabzBuilder::BuildTabzDialog (0x10a340), the TABZ_DIALOG
// end-of-level dialog builder (C:\Proj\Gruntz).
//
// ITS OWN TU, AGAIN. This function had its own retail obj; the wave1-E one-file merge
// folded it into SBI_RectOnly.cpp, and that merge was itself a matching bug:
//
//   MSVC5 has exactly ONE base-ctor spelling per TU. Retail CALLS the out-of-line
//   ??0CStatusBarItem (0x1005d0) at THIS function's `new CSBI_Image` /
//   `new CSBI_MenuItemDlg` / `new CSBI_ImageSet` sites, but INLINES the base ctor at
//   CStatusBarMgr::BuildStatusBarTabs's `new` sites. Two objs, two spellings - so once
//   both lived in one TU, whichever spelling we picked made the other function wrong.
//   Measured, merged: knob OFF -> BuildTabzDialog 75.39 / BuildStatusBarTabs 71.58;
//   knob ON -> 83.72 / 67.34. Un-merged, each TU gets its own spelling and both are
//   right: SBI_ITEM_OWN_CTOR is ON here (the retail base-ctor CALL) and OFF in
//   SBI_RectOnly.cpp (the retail inlined base ctor).
//
// The un-merge was blocked until the CSBI_RectOnly/CStatusBarMgr host split, because
// SBI_RectOnly.cpp also defined ??0CSBI_RectOnly (0x101fa0) - whose retail spelling
// INLINES the base ctor, so turning the knob on there cratered that 100% function to
// 25.5%. The split moved ??0CSBI_RectOnly out to SBI_RectOnlyBase.cpp, which removed
// the last objector.
//
// SBI_ITEM_OWN_CTOR only DECLARES CStatusBarItem's ctor out-of-line here; the one body
// is supplied by src/Gruntz/StatusBarItem.cpp at its retail RVA 0x1005d0, so the call
// binds and nothing is duplicated.
#define SBI_ITEM_OWN_CTOR // out-of-line base ctor => retail's `call ??0CStatusBarItem`

#include <Mfc.h> // afx-first umbrella (CObject / CPtrList / ::CopyRect)
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GruntzMgr.h>          // the *0x24556c singleton (CGruntzMgr)
#include <Gruntz/SbiTabzDialogViews.h> // CSBI_Image/_MenuItemDlg/_ImageSet leaves + CTabzBuilder

extern "C" CGruntzMgr* g_gameReg;

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
    ::CopyRect(&dst, &src);
    i32 cx = dst.left + (dst.right - dst.left) / 2;
    i32 cy = dst.top + (dst.bottom - dst.top) / 2;

    if (m_554 != 0) {
        // ---- confirm dialog: AREYOUSURE + YES/NO ----
        CSBI_Image* areYouSure = new CSBI_Image;
        if (!areYouSure->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(areYouSure);

        CSBI_MenuItemDlg* yes = new CSBI_MenuItemDlg;
        if (!yes->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(yes);
        m_1fc = yes;

        CSBI_MenuItemDlg* no = new CSBI_MenuItemDlg;
        if (!no->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(no);
        m_200 = no;
        return 1;
    }

    // ---- main tabz dialog: DIALOG then a mission/mode decision tree ----
    CSBI_Image* dialog = new CSBI_Image;
    if (!dialog->SetupImage(
            (CStatusBarMgr*)this,
            (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
    m_d4.AddTail(dialog);

    i32 reason = ((TabzGmFactory*)g_gameReg->m_cmdGrid)->m_3ec;

    if (((TabzGmFactory*)g_gameReg->m_cmdGrid)->m_288 == 1) {
        // mission accomplished
        CSBI_ImageSet* status = new CSBI_ImageSet;
        if (!status->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(status);

        CSBI_ImageSet* rsn = new CSBI_ImageSet;
        if (!rsn->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(rsn);

        if (g_gameReg->m_134 == 1) {
            CSBI_MenuItemDlg* next = new CSBI_MenuItemDlg;
            if (!next->SetupImage(
                    (CStatusBarMgr*)this,
                    (CDDrawSurfaceMgr*)
                        m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
            m_d4.AddTail(next);
            m_1f4 = next;

            CSBI_MenuItemDlg* quit = new CSBI_MenuItemDlg;
            if (!quit->SetupImage(
                    (CStatusBarMgr*)this,
                    (CDDrawSurfaceMgr*)
                        m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
            m_d4.AddTail(quit);
            m_1f8 = quit;
        } else {
            CSBI_MenuItemDlg* statz = new CSBI_MenuItemDlg;
            if (!statz->SetupImage(
                    (CStatusBarMgr*)this,
                    (CDDrawSurfaceMgr*)
                        m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
            m_d4.AddTail(statz);
            m_1f8 = statz;
        }
        return 1;
    }

    // mission not complete
    CSBI_ImageSet* status = new CSBI_ImageSet;
    if (!status->SetupImage(
            (CStatusBarMgr*)this,
            (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
    m_d4.AddTail(status);

    CSBI_ImageSet* rsn = new CSBI_ImageSet;
    if (!rsn->SetupImage(
            (CStatusBarMgr*)this,
            (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
    m_d4.AddTail(rsn);

    if (g_gameReg->m_134 == 1) {
        CSBI_MenuItemDlg* replay = new CSBI_MenuItemDlg;
        if (!replay->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(replay);
        m_1f4 = replay;

        CSBI_MenuItemDlg* quit = new CSBI_MenuItemDlg;
        if (!quit->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(quit);
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
        CSBI_MenuItemDlg* observe = new CSBI_MenuItemDlg;
        if (!observe->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(observe);
        m_1f4 = observe;
        m_578 = 1;

        CSBI_MenuItemDlg* statz = new CSBI_MenuItemDlg;
        if (!statz->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(statz);
        m_1f8 = statz;
    } else {
        m_578 = 0;
        CSBI_MenuItemDlg* statz = new CSBI_MenuItemDlg;
        if (!statz->SetupImage(
                (CStatusBarMgr*)this,
                (CDDrawSurfaceMgr*)m_c, // TabzSub cross-view (status-bar lane; unresolved facet)
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
        m_d4.AddTail(statz);
        m_1f8 = statz;
    }
    return 1;
}
