#define SBI_ITEM_OWN_CTOR // out-of-line base ctor => retail's `call ??0CStatusBarItem`

#include <Mfc.h> // afx-first umbrella (CObject / CPtrList / ::CopyRect)
#include <Gruntz/GameRegMfcPtr.h>
#include <Ints.h>
#include <rva.h>
#include <Gruntz/TriggerMgr.h>         // m_cmdGrid's real class (m_phase/m_3ec)
#include <Gruntz/GruntzMgr.h>          // the *0x24556c singleton (CGruntzMgr)
#include <Gruntz/SbiTabzDialogViews.h> // the CSBI_Image / CSBI_MenuItem / CSBI_ImageSet leaves
#include <Gruntz/StatusBarMgr.h>       // the REAL host: this fn is a CStatusBarMgr method
#include <Gruntz/SBI_MenuItem.h>       // canonical CSBI_MenuItem (StatusBarMgr.h only fwd-decls it)
#include <Gruntz/GameLevel.h>          // m_c->m_level->m_planeCtx (the dialog-centering rect)

RVA(0x0010a340, 0xbcb)
i32 CStatusBarMgr::BuildTabzDialog() {
    if (m_toggleActive == 0) {
        return 1;
    }

    // The centering rect: m_c (CDDrawSurfaceMgr) -> m_level (+0x24) -> m_planeCtx (+0x10).
    // Retail @0x10a36e: mov eax,[ebx+0xc] / mov eax,[eax+0x24] / add eax,0x10, then four
    // dword loads - the member-wise copy of the 4-int rect below lowers to exactly those.
    const LevelCoordRect& lr = m_c->m_level->m_planeCtx;
    RECT src;
    src.left = lr.left;
    src.top = lr.top;
    src.right = lr.right;
    src.bottom = lr.bottom;
    RECT dst;
    ::CopyRect(&dst, &src);
    i32 cx = dst.left + (dst.right - dst.left) / 2;
    i32 cy = dst.top + (dst.bottom - dst.top) / 2;

    if (m_toggleHandle != 0) {
        // ---- confirm dialog: AREYOUSURE + YES/NO ----
        CSBI_Image* areYouSure = new CSBI_Image;
        if (!areYouSure->SetupImage(
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
        m_tabLists[6].AddTail(areYouSure);

        CSBI_MenuItem* yes = new CSBI_MenuItem;
        if (!yes->SetupImage(
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
        m_tabLists[6].AddTail(yes);
        m_tabSprite13 = yes;

        CSBI_MenuItem* no = new CSBI_MenuItem;
        if (!no->SetupImage(
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
        m_tabLists[6].AddTail(no);
        m_tabSprite14 = no;
        return 1;
    }

    // ---- main tabz dialog: DIALOG then a mission/mode decision tree ----
    CSBI_Image* dialog = new CSBI_Image;
    if (!dialog->SetupImage(
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
    m_tabLists[6].AddTail(dialog);

    i32 reason = g_gameReg->m_cmdGrid->m_3ec;

    if (g_gameReg->m_cmdGrid->m_phase == 1) {
        // mission accomplished
        CSBI_ImageSet* status = new CSBI_ImageSet;
        if (!status->SetupImage(
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
        m_tabLists[6].AddTail(status);

        CSBI_ImageSet* rsn = new CSBI_ImageSet;
        if (!rsn->SetupImage(
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
        m_tabLists[6].AddTail(rsn);

        if (g_gameReg->m_134 == 1) {
            CSBI_MenuItem* next = new CSBI_MenuItem;
            if (!next->SetupImage(
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
            m_tabLists[6].AddTail(next);
            m_tabSprite11 = next;

            CSBI_MenuItem* quit = new CSBI_MenuItem;
            if (!quit->SetupImage(
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
            m_tabLists[6].AddTail(quit);
            m_tabSprite12 = quit;
        } else {
            CSBI_MenuItem* statz = new CSBI_MenuItem;
            if (!statz->SetupImage(
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
            m_tabLists[6].AddTail(statz);
            m_tabSprite12 = statz;
        }
        return 1;
    }

    // mission not complete
    CSBI_ImageSet* status = new CSBI_ImageSet;
    if (!status->SetupImage(
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
    m_tabLists[6].AddTail(status);

    CSBI_ImageSet* rsn = new CSBI_ImageSet;
    if (!rsn->SetupImage(
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
    m_tabLists[6].AddTail(rsn);

    if (g_gameReg->m_134 == 1) {
        CSBI_MenuItem* replay = new CSBI_MenuItem;
        if (!replay->SetupImage(
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
        m_tabLists[6].AddTail(replay);
        m_tabSprite11 = replay;

        CSBI_MenuItem* quit = new CSBI_MenuItem;
        if (!quit->SetupImage(
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
        m_tabLists[6].AddTail(quit);
        m_tabSprite12 = quit;
        return 1;
    }

    // count active players (m_178!=0 && m_17c==0 && m_174==0) over the 4 slots.
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_gameReg->m_options[i].m_joined != 0 && g_gameReg->m_options[i].m_doneFlag == 0
            && g_gameReg->m_options[i].m_clearedRound == 0) {
            count++;
        }
    }

    if (count >= 2) {
        CSBI_MenuItem* observe = new CSBI_MenuItem;
        if (!observe->SetupImage(
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
        m_tabLists[6].AddTail(observe);
        m_tabSprite11 = observe;
        m_578 = 1;

        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
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
        m_tabLists[6].AddTail(statz);
        m_tabSprite12 = statz;
    } else {
        m_578 = 0;
        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
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
        m_tabLists[6].AddTail(statz);
        m_tabSprite12 = statz;
    }
    return 1;
}
