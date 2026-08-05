#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_MenuItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>

RVA_COMPGEN(0x001005d0, 0x17, ??0CStatusBarItem@@QAE@XZ)

// @early-stop
// Retail CALLS ??0CStatusBarItem@@QAE@XZ at each `new` site; the in-class ctor
// body lets cl splice it (and drop its dead stores) instead.
RVA(0x0010a340, 0xbcb)
i32 CStatusBarMgr::BuildTabzDialog() {
    if (m_toggleActive == 0) {
        return 1;
    }

    const LevelCoordRect& lr = m_world->m_level->m_planeCtx;
    RECT src;
    src.left = lr.left;
    src.top = lr.top;
    src.right = lr.right;
    src.bottom = lr.bottom;
    RECT dst;
    CopyRect(&dst, &src);
    i32 cx = dst.left + (dst.right - dst.left) / 2;
    i32 cy = dst.top + (dst.bottom - dst.top) / 2;

    if (m_toggleHandle != 0) {

        CSBI_Image* areYouSure = new CSBI_Image;
        if (!areYouSure->SetupImage(
                this,
                m_world,
                0x321,
                6,
                SbGeom(cx - 0x5e, cy - 0x3c, cx + 0x5e, cy + 0x3d),
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
                m_world,
                0x327,
                6,
                SbGeom(cx - 0x45, cy + 0x11, cx - 0x12, cy + 0x28),
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
                m_world,
                0x328,
                6,
                SbGeom(cx + 0xd, cy + 0x11, cx + 0x40, cy + 0x28),
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

    CSBI_Image* dialog = new CSBI_Image;
    if (!dialog->SetupImage(
            this,
            m_world,
            0x321,
            6,
            SbGeom(cx - 0x8e, cy - 0x48, cx + 0x8e, cy + 0x48),
            "GAME_STATUSBAR_TABZ_DIALOG",
            -1,
            0
        )) {
        delete dialog;
        return 0;
    }
    m_tabLists[6].AddTail(dialog);

    i32 reason = g_gameReg->m_cmdGrid->m_finishReasonFrame;

    if (g_gameReg->m_cmdGrid->m_phase == 1) {

        CSBI_ImageSet* status = new CSBI_ImageSet;
        if (!status->SetupImage(
                this,
                m_world,
                0x322,
                6,
                SbGeom(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
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
                m_world,
                0x326,
                6,
                SbGeom(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
                "GAME_STATUSBAR_TABZ_DIALOG_REASON",
                reason,
                0
            )) {
            delete rsn;
            return 0;
        }
        m_tabLists[6].AddTail(rsn);

        if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            CSBI_MenuItem* next = new CSBI_MenuItem;
            if (!next->SetupImage(
                    this,
                    m_world,
                    0x324,
                    6,
                    SbGeom(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
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
                    m_world,
                    0x325,
                    6,
                    SbGeom(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
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
                    m_world,
                    0x325,
                    6,
                    SbGeom(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
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

    CSBI_ImageSet* status = new CSBI_ImageSet;
    if (!status->SetupImage(
            this,
            m_world,
            0x322,
            6,
            SbGeom(cx - 0x8e, cy - 0x31, cx + 0x8d, cy - 0x16),
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
            m_world,
            0x326,
            6,
            SbGeom(cx - 0x7c, cy - 0x11, cx + 0x73, cy + 0x4),
            "GAME_STATUSBAR_TABZ_DIALOG_REASON",
            reason,
            0
        )) {
        delete rsn;
        return 0;
    }
    m_tabLists[6].AddTail(rsn);

    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        CSBI_MenuItem* replay = new CSBI_MenuItem;
        if (!replay->SetupImage(
                this,
                m_world,
                0x324,
                6,
                SbGeom(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
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
                m_world,
                0x325,
                6,
                SbGeom(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
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
                m_world,
                0x324,
                6,
                SbGeom(cx - 0x7d, cy + 0x17, cx - 0xe, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_OBSERVE",
                -1,
                0
            )) {
            delete observe;
            return 0;
        }
        m_tabLists[6].AddTail(observe);
        m_tabSprite11 = observe;
        m_observerTabAvailable = 1;

        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
                this,
                m_world,
                0x325,
                6,
                SbGeom(cx, cy + 0x17, cx + 0x6f, cy + 0x32),
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
        m_observerTabAvailable = 0;
        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
                this,
                m_world,
                0x325,
                6,
                SbGeom(cx - 0x39, cy + 0x17, cx + 0x36, cy + 0x32),
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
