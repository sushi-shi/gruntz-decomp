// MainMenuBuilder.cpp - the game's main-menu tree builder (C:\Proj\Gruntz). The
// second-biggest backlog function (6157 B): a /GX EH-framed routine that builds
// the 14-page main-menu tree. Each page allocates a menu-page object (op-new +
// the 3-CString/CPtrList ctor), initializes it from a "MENU_<PAGE>" key + label
// (Configure), fills it with items / sub-items (AddItem / AddSubItem, each a key +
// label + flags + index), conditionally disables the multiplayer-gated items
// (when g_multiplayerAvail is set) and the area-progress-gated items (when the
// player has not unlocked that area), and hands the finished page to the menu
// host (RegisterPage). The 14 pages: MAINMENU, SINGLEPLAYER, MULTIPLAYER,
// MOVIEZ, QUESTZ, then the 9 AREAS sub-pages (TRAINING + AREA1..8).
//
// Identity: the stub had it as EngineLabelBacklog::LoadAreaLevelTable, but the
// $SG string set ("MENU_MAINMENU_TITLE", "SINGLEPLAYER", "MULTIPLAYER", ...)
// identifies it as the main-menu definition builder. Reconstructed self-contained.
//
// The page/item classes are the canonical CMenuPage / CMenuItem (headers); the
// builder methods (Configure/AddItem/AddSubItem) and the CRT/MFC machinery
// (operator new/delete, CString, CPtrList) are external no-body callees
// (reloc-masked rel32). The "MENU_*"/label strings are $SG literals
// (reloc-masked against the matched string symbols). The page ctor/dtor are the
// header-inline CMenuPage ones, folded at the 14 new/delete sites like retail.
//
// @early-stop  (body fully aligns; residual is the /GX EH-state tail)
// The 14-page build sequence, the per-item gates, the EH frame and the per-page
// ctors all match retail; the residual is the exception-state threading -
// retail stamps the CPtrList's ctor state at [esp+0x1c] (a second trylevel slot)
// where MSVC5 here stamps [esp+0x18]. That EH-state-index / trylevel-slot
// divergence is the documented /GX wall (docs/seh-eh.md;
// docs/patterns/eh-dtor-vptr-stamp-vs-trylevel-order.md;
// rezalloc-placement-new-no-eh-frame.md, topic:eh/topic:wall). Deferred to the
// final sweep.

#include <Gruntz/GameRegistry.h>  // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Mfc.h> // MFC superset of <Win32.h> (afx first): <Gruntz/SoundCue.h> now needs the
#include <rva.h>

typedef u32 u32;

DATA(0x00245d88)
RECT g_menuTextRect = {0}; // 0x245d88  (owner-TU definition)

#include <Gruntz/MenuPage.h> // canonical CMenuItem (Disable [6], the AddItem product)
#include <Gruntz/ChatBox.h>  // the menu host CChatBox (RegisterPage == AddNode @0x182ba0)
#include <Gruntz/MainMenuBuilder.h>
#include <Io/SaveGame.h> // CSaveGame (m_saveSink: CheckMagic + m_curLevel progress)

static i32 RegisterPage(CChatBox* menu, CMenuPage* page) {
    return menu->AddNode(page);
}

static char s_MAIN[] = "MAIN";
static char s_MENU_MAINMENU_TITLE[] = "MENU_MAINMENU_TITLE";
static char s_SINGLEPLAYER[] = "SINGLEPLAYER";
static char s_MENU_MAINMENU_SINGLEPLAYER[] = "MENU_MAINMENU_SINGLEPLAYER";
static char s_MULTIPLAYER[] = "MULTIPLAYER";
static char s_MENU_MAINMENU_MULTIPLAYER[] = "MENU_MAINMENU_MULTIPLAYER";
static char s_OPTIONZ[] = "OPTIONZ";
static char s_MENU_MAINMENU_OPTIONZ[] = "MENU_MAINMENU_OPTIONZ";
static char s_MOVIEZ[] = "MOVIEZ";
static char s_MENU_MAINMENU_MOVIEZ[] = "MENU_MAINMENU_MOVIEZ";
static char s_HELP[] = "HELP";
static char s_MENU_MAINMENU_HELP[] = "MENU_MAINMENU_HELP";
static char s_QUIT[] = "QUIT";
static char s_MENU_MAINMENU_QUIT[] = "MENU_MAINMENU_QUIT";
static char s_MENU_SINGLEPLAYER_TITLE[] = "MENU_SINGLEPLAYER_TITLE";
static char s_QUICKSTART[] = "QUICKSTART";
static char s_MENU_SINGLEPLAYER_QUICKSTART[] = "MENU_SINGLEPLAYER_QUICKSTART";
static char s_QUESTZ[] = "QUESTZ";
static char s_MENU_SINGLEPLAYER_QUESTZ[] = "MENU_SINGLEPLAYER_QUESTZ";
static char s_BATTLEZ[] = "BATTLEZ";
static char s_MENU_SINGLEPLAYER_BATTLEZ[] = "MENU_SINGLEPLAYER_BATTLEZ";
static char s_LOADGAME[] = "LOADGAME";
static char s_MENU_SINGLEPLAYER_LOADGAME[] = "MENU_SINGLEPLAYER_LOADGAME";
static char s_CUSTOMLEVELZ[] = "CUSTOMLEVELZ";
static char s_MENU_SINGLEPLAYER_CUSTOMLEVELZ[] = "MENU_SINGLEPLAYER_CUSTOMLEVELZ";
static char s_BACK[] = "BACK";
static char s_MENU_SINGLEPLAYER_BACK[] = "MENU_SINGLEPLAYER_BACK";
static char s_MENU_MULTIPLAYER_TITLE[] = "MENU_MULTIPLAYER_TITLE";
static char s_HOST[] = "HOST";
static char s_MENU_MULTIPLAYER_HOST[] = "MENU_MULTIPLAYER_HOST";
static char s_JOIN[] = "JOIN";
static char s_MENU_MULTIPLAYER_JOIN[] = "MENU_MULTIPLAYER_JOIN";
static char s_MENU_MULTIPLAYER_BACK[] = "MENU_MULTIPLAYER_BACK";
static char s_MENU_MOVIEZ_TITLE[] = "MENU_MOVIEZ_TITLE";
static char s_LOGO[] = "LOGO";
static char s_MENU_MOVIEZ_LOGO[] = "MENU_MOVIEZ_LOGO";
static char s_INTRO[] = "INTRO";
static char s_MENU_MOVIEZ_INTRO[] = "MENU_MOVIEZ_INTRO";
static char s_FINAL[] = "FINAL";
static char s_MENU_MOVIEZ_FINAL[] = "MENU_MOVIEZ_FINAL";
static char s_CREDITZ[] = "CREDITZ";
static char s_MENU_MOVIEZ_CREDITZ[] = "MENU_MOVIEZ_CREDITZ";
static char s_MENU_MOVIEZ_BACK[] = "MENU_MOVIEZ_BACK";
static char s_MENU_QUESTZ_TITLE[] = "MENU_QUESTZ_TITLE";
static char s_TRAINING[] = "TRAINING";
static char s_MENU_QUESTZ_TRAINING[] = "MENU_QUESTZ_TRAINING";
static char s_AREA1[] = "AREA1";
static char s_MENU_QUESTZ_AREA1[] = "MENU_QUESTZ_AREA1";
static char s_AREA2[] = "AREA2";
static char s_MENU_QUESTZ_AREA2[] = "MENU_QUESTZ_AREA2";
static char s_AREA3[] = "AREA3";
static char s_MENU_QUESTZ_AREA3[] = "MENU_QUESTZ_AREA3";
static char s_AREA4[] = "AREA4";
static char s_MENU_QUESTZ_AREA4[] = "MENU_QUESTZ_AREA4";
static char s_AREA5[] = "AREA5";
static char s_MENU_QUESTZ_AREA5[] = "MENU_QUESTZ_AREA5";
static char s_AREA6[] = "AREA6";
static char s_MENU_QUESTZ_AREA6[] = "MENU_QUESTZ_AREA6";
static char s_AREA7[] = "AREA7";
static char s_MENU_QUESTZ_AREA7[] = "MENU_QUESTZ_AREA7";
static char s_AREA8[] = "AREA8";
static char s_MENU_QUESTZ_AREA8[] = "MENU_QUESTZ_AREA8";
static char s_MENU_QUESTZ_BACK[] = "MENU_QUESTZ_BACK";
static char s_MENU_AREAS_TRAININGTITLE[] = "MENU_AREAS_TRAININGTITLE";
static char s_STAGE1[] = "STAGE1";
static char s_MENU_AREAS_STAGE1[] = "MENU_AREAS_STAGE1";
static char s_STAGE2[] = "STAGE2";
static char s_MENU_AREAS_STAGE2[] = "MENU_AREAS_STAGE2";
static char s_STAGE3[] = "STAGE3";
static char s_MENU_AREAS_STAGE3[] = "MENU_AREAS_STAGE3";
static char s_STAGE4[] = "STAGE4";
static char s_MENU_AREAS_STAGE4[] = "MENU_AREAS_STAGE4";
static char s_MENU_AREAS_BACK[] = "MENU_AREAS_BACK";
static char s_MENU_AREAS_AREA1TITLE[] = "MENU_AREAS_AREA1TITLE";
static char s_MENU_AREAS_AREA2TITLE[] = "MENU_AREAS_AREA2TITLE";
static char s_MENU_AREAS_AREA3TITLE[] = "MENU_AREAS_AREA3TITLE";
static char s_MENU_AREAS_AREA4TITLE[] = "MENU_AREAS_AREA4TITLE";
static char s_MENU_AREAS_AREA5TITLE[] = "MENU_AREAS_AREA5TITLE";
static char s_MENU_AREAS_AREA6TITLE[] = "MENU_AREAS_AREA6TITLE";
static char s_MENU_AREAS_AREA7TITLE[] = "MENU_AREAS_AREA7TITLE";
static char s_MENU_AREAS_AREA8TITLE[] = "MENU_AREAS_AREA8TITLE";

RVA(0x000a1190, 0x29)
void SetMenuTextRect() {
    g_menuTextRect.left = 5;
    g_menuTextRect.top = 453;
    g_menuTextRect.right = 635;
    g_menuTextRect.bottom = 478;
}

RVA(0x000a11d0, 0x180d)
i32 BuildMainMenuTree(CChatBox* menu, i32) {
    if (menu == 0) {
        return 0;
    }

    CMenuPage* page;
    CMenuItem* it;
    i32 progress;
    // ---- page 1 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_MAIN, s_MENU_MAINMENU_TITLE, 0, 0) == 0) {
        delete page;
        return 0;
    }
    it = page->AddItem(s_SINGLEPLAYER, s_MENU_MAINMENU_SINGLEPLAYER, 0, s_SINGLEPLAYER, 0);
    if (g_multiplayerAvail != 0) {
        it->Disable(3);
    }
    page->AddItem(s_MULTIPLAYER, s_MENU_MAINMENU_MULTIPLAYER, 0, s_MULTIPLAYER, 0);
    page->AddItem(s_OPTIONZ, s_MENU_MAINMENU_OPTIONZ, 0x80e2, 0, 0);
    it = page->AddItem(s_MOVIEZ, s_MENU_MAINMENU_MOVIEZ, 0, s_MOVIEZ, 0);
    if (g_multiplayerAvail != 0) {
        it->Disable(3);
    }
    page->AddItem(s_HELP, s_MENU_MAINMENU_HELP, 0x8035, 0, 0);
    page->AddItem(s_QUIT, s_MENU_MAINMENU_QUIT, 0x8008, 0, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 2 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_SINGLEPLAYER, s_MENU_SINGLEPLAYER_TITLE, s_MAIN, 0) == 0) {
        delete page;
        return 0;
    }
    page->AddItem(s_QUICKSTART, s_MENU_SINGLEPLAYER_QUICKSTART, 0x8174, 0, 0);
    page->AddItem(s_QUESTZ, s_MENU_SINGLEPLAYER_QUESTZ, 0, s_QUESTZ, 0);
    page->AddItem(s_BATTLEZ, s_MENU_SINGLEPLAYER_BATTLEZ, 0x80e1, 0, 0);
    page->AddItem(s_LOADGAME, s_MENU_SINGLEPLAYER_LOADGAME, 0x80ce, 0, 0);
    page->AddItem(s_CUSTOMLEVELZ, s_MENU_SINGLEPLAYER_CUSTOMLEVELZ, 0x8042, 0, 0);
    page->AddItem(s_BACK, s_MENU_SINGLEPLAYER_BACK, 0, s_MAIN, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 3 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_MULTIPLAYER, s_MENU_MULTIPLAYER_TITLE, s_MAIN, 0) == 0) {
        delete page;
        return 0;
    }
    it = page->AddItem(s_HOST, s_MENU_MULTIPLAYER_HOST, 0x80d3, 0, 0);
    if (g_multiplayerAvail != 0) {
        it->Disable(3);
    }
    page->AddItem(s_JOIN, s_MENU_MULTIPLAYER_JOIN, 0x80d2, 0, 0);
    page->AddItem(s_BACK, s_MENU_MULTIPLAYER_BACK, 0, s_MAIN, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 4 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_MOVIEZ, s_MENU_MOVIEZ_TITLE, s_MAIN, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddItem(s_LOGO, s_MENU_MOVIEZ_LOGO, 0x8170, 0, 0);
    page->AddItem(s_INTRO, s_MENU_MOVIEZ_INTRO, 0x8171, 0, 0);
    it = page->AddItem(s_FINAL, s_MENU_MOVIEZ_FINAL, 0x8173, 0, 0);
    if (g_gameReg->m_saveSink->CheckMagic() == 0) {
        it->Disable(3);
    }
    page->AddItem(s_CREDITZ, s_MENU_MOVIEZ_CREDITZ, 0x8021, 0, 0);
    page->AddItem(s_BACK, s_MENU_MOVIEZ_BACK, 0, s_MAIN, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 5 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_QUESTZ, s_MENU_QUESTZ_TITLE, s_SINGLEPLAYER, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddItem(s_TRAINING, s_MENU_QUESTZ_TRAINING, 0, s_TRAINING, 0);
    page->AddSubItem(s_AREA1, s_MENU_QUESTZ_AREA1, 0x8149, 0x1, 0, s_AREA1, 0);
    it = page->AddSubItem(s_AREA2, s_MENU_QUESTZ_AREA2, 0x8149, 0x2, 0, s_AREA2, 0);
    if (progress > 0x24 || progress < 0x4) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA3, s_MENU_QUESTZ_AREA3, 0x8149, 0x3, 0, s_AREA3, 0);
    if (progress > 0x24 || progress < 0x8) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA4, s_MENU_QUESTZ_AREA4, 0x8149, 0x4, 0, s_AREA4, 0);
    if (progress > 0x24 || progress < 0xc) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA5, s_MENU_QUESTZ_AREA5, 0x8149, 0x5, 0, s_AREA5, 0);
    if (progress > 0x24 || progress < 0x10) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA6, s_MENU_QUESTZ_AREA6, 0x8149, 0x6, 0, s_AREA6, 0);
    if (progress > 0x24 || progress < 0x14) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA7, s_MENU_QUESTZ_AREA7, 0x8149, 0x7, 0, s_AREA7, 0);
    if (progress > 0x24 || progress < 0x18) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_AREA8, s_MENU_QUESTZ_AREA8, 0x8149, 0x8, 0, s_AREA8, 0);
    if (progress > 0x24 || progress < 0x1c) {
        it->Disable(3);
    }
    page->AddItem(s_BACK, s_MENU_QUESTZ_BACK, 0, s_SINGLEPLAYER, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 6 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_TRAINING, s_MENU_AREAS_TRAININGTITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x25, 0, 0, 0);
    page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x26, 0, 0, 0);
    page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x27, 0, 0, 0);
    page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x28, 0, 0, 0);
    page->AddItem(s_BACK, s_MENU_AREAS_BACK, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 7 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA1, s_MENU_AREAS_AREA1TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x1, 0, 0, 0);
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x2, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x3, 0, 0, 0);
    if (progress > 0x24 || progress < 0x2) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x4, 0, 0, 0);
    if (progress > 0x24 || progress < 0x3) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 8 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA2, s_MENU_AREAS_AREA2TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x5, 0, 0, 0);
    if (progress > 0x24 || progress < 0x4) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x6, 0, 0, 0);
    if (progress > 0x24 || progress < 0x5) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x7, 0, 0, 0);
    if (progress > 0x24 || progress < 0x6) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x8, 0, 0, 0);
    if (progress > 0x24 || progress < 0x7) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 9 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA3, s_MENU_AREAS_AREA3TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x9, 0, 0, 0);
    if (progress > 0x24 || progress < 0x8) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0xa, 0, 0, 0);
    if (progress > 0x24 || progress < 0x9) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0xb, 0, 0, 0);
    if (progress > 0x24 || progress < 0xa) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0xc, 0, 0, 0);
    if (progress > 0x24 || progress < 0xb) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 10 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA4, s_MENU_AREAS_AREA4TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0xd, 0, 0, 0);
    if (progress > 0x24 || progress < 0xc) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0xe, 0, 0, 0);
    if (progress > 0x24 || progress < 0xd) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0xf, 0, 0, 0);
    if (progress > 0x24 || progress < 0xe) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x10, 0, 0, 0);
    if (progress > 0x24 || progress < 0xf) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 11 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA5, s_MENU_AREAS_AREA5TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x11, 0, 0, 0);
    if (progress > 0x24 || progress < 0x10) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x12, 0, 0, 0);
    if (progress > 0x24 || progress < 0x11) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x13, 0, 0, 0);
    if (progress > 0x24 || progress < 0x12) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x14, 0, 0, 0);
    if (progress > 0x24 || progress < 0x13) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 12 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA6, s_MENU_AREAS_AREA6TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x15, 0, 0, 0);
    if (progress > 0x24 || progress < 0x14) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x16, 0, 0, 0);
    if (progress > 0x24 || progress < 0x15) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x17, 0, 0, 0);
    if (progress > 0x24 || progress < 0x16) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x18, 0, 0, 0);
    if (progress > 0x24 || progress < 0x17) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 13 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA7, s_MENU_AREAS_AREA7TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x19, 0, 0, 0);
    if (progress > 0x24 || progress < 0x18) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x1a, 0, 0, 0);
    if (progress > 0x24 || progress < 0x19) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x1b, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1a) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x1c, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1b) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (RegisterPage(menu, page) == 0) {
        return 0;
    }
    // ---- page 14 ----
    page = new CMenuPage;
    if (page->Configure(menu, s_AREA8, s_MENU_AREAS_AREA8TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, 0x807f, 0x1d, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1c) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, 0x807f, 0x1e, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1d) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, 0x807f, 0x1f, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1e) {
        it->Disable(3);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, 0x807f, 0x20, 0, 0, 0);
    if (progress > 0x24 || progress < 0x1f) {
        it->Disable(3);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    return RegisterPage(menu, page) != 0;
}
