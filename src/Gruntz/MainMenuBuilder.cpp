#include <rva.h>

#include <Gruntz/MainMenuBuilder.h>

#include <Mfc.h>

#include <Gruntz/ChatBox.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/StartUpPrompt.h>
#include <Io/SaveGame.h>

typedef u32 u32;

DATA(0x00245d88)
RECT g_menuTextRect = {0};

DATA(0x00211a98)
static char s_MAIN[] = "MAIN";
DATA(0x00211aa0)
static char s_MENU_MAINMENU_TITLE[] = "MENU_MAINMENU_TITLE";
DATA(0x00211a88)
static char s_SINGLEPLAYER[] = "SINGLEPLAYER";
DATA(0x00211a68)
static char s_MENU_MAINMENU_SINGLEPLAYER[] = "MENU_MAINMENU_SINGLEPLAYER";
DATA(0x00211a58)
static char s_MULTIPLAYER[] = "MULTIPLAYER";
DATA(0x00211a38)
static char s_MENU_MAINMENU_MULTIPLAYER[] = "MENU_MAINMENU_MULTIPLAYER";
DATA(0x00211a10)
static char s_OPTIONZ[] = "OPTIONZ";
DATA(0x00211a1c)
static char s_MENU_MAINMENU_OPTIONZ[] = "MENU_MAINMENU_OPTIONZ";
DATA(0x00211a08)
static char s_MOVIEZ[] = "MOVIEZ";
DATA(0x002119ec)
static char s_MENU_MAINMENU_MOVIEZ[] = "MENU_MAINMENU_MOVIEZ";
static char s_HELP[] = "HELP";
DATA(0x002119d4)
static char s_MENU_MAINMENU_HELP[] = "MENU_MAINMENU_HELP";
DATA(0x002119b4)
static char s_QUIT[] = "QUIT";
DATA(0x002119bc)
static char s_MENU_MAINMENU_QUIT[] = "MENU_MAINMENU_QUIT";
DATA(0x00211998)
static char s_MENU_SINGLEPLAYER_TITLE[] = "MENU_SINGLEPLAYER_TITLE";
DATA(0x00211964)
static char s_QUICKSTART[] = "QUICKSTART";
DATA(0x00211974)
static char s_MENU_SINGLEPLAYER_QUICKSTART[] = "MENU_SINGLEPLAYER_QUICKSTART";
DATA(0x0021195c)
static char s_QUESTZ[] = "QUESTZ";
DATA(0x0021193c)
static char s_MENU_SINGLEPLAYER_QUESTZ[] = "MENU_SINGLEPLAYER_QUESTZ";
DATA(0x00211910)
static char s_BATTLEZ[] = "BATTLEZ";
DATA(0x0021191c)
static char s_MENU_SINGLEPLAYER_BATTLEZ[] = "MENU_SINGLEPLAYER_BATTLEZ";
DATA(0x002118e4)
static char s_LOADGAME[] = "LOADGAME";
DATA(0x002118f0)
static char s_MENU_SINGLEPLAYER_LOADGAME[] = "MENU_SINGLEPLAYER_LOADGAME";
DATA(0x002118ac)
static char s_CUSTOMLEVELZ[] = "CUSTOMLEVELZ";
DATA(0x002118bc)
static char s_MENU_SINGLEPLAYER_CUSTOMLEVELZ[] = "MENU_SINGLEPLAYER_CUSTOMLEVELZ";
DATA(0x00211888)
static char s_BACK[] = "BACK";
DATA(0x00211890)
static char s_MENU_SINGLEPLAYER_BACK[] = "MENU_SINGLEPLAYER_BACK";
DATA(0x0021186c)
static char s_MENU_MULTIPLAYER_TITLE[] = "MENU_MULTIPLAYER_TITLE";
DATA(0x00211848)
static char s_HOST[] = "HOST";
DATA(0x00211850)
static char s_MENU_MULTIPLAYER_HOST[] = "MENU_MULTIPLAYER_HOST";
DATA(0x00211824)
static char s_JOIN[] = "JOIN";
DATA(0x0021182c)
static char s_MENU_MULTIPLAYER_JOIN[] = "MENU_MULTIPLAYER_JOIN";
DATA(0x00211808)
static char s_MENU_MULTIPLAYER_BACK[] = "MENU_MULTIPLAYER_BACK";
DATA(0x002117f0)
static char s_MENU_MOVIEZ_TITLE[] = "MENU_MOVIEZ_TITLE";
DATA(0x002117d4)
static char s_LOGO[] = "LOGO";
DATA(0x002117dc)
static char s_MENU_MOVIEZ_LOGO[] = "MENU_MOVIEZ_LOGO";
DATA(0x002117b4)
static char s_INTRO[] = "INTRO";
DATA(0x002117bc)
static char s_MENU_MOVIEZ_INTRO[] = "MENU_MOVIEZ_INTRO";
DATA(0x00211794)
static char s_FINAL[] = "FINAL";
DATA(0x0021179c)
static char s_MENU_MOVIEZ_FINAL[] = "MENU_MOVIEZ_FINAL";
DATA(0x0020ce90)
static char s_CREDITZ[] = "CREDITZ";
DATA(0x0021177c)
static char s_MENU_MOVIEZ_CREDITZ[] = "MENU_MOVIEZ_CREDITZ";
DATA(0x00211768)
static char s_MENU_MOVIEZ_BACK[] = "MENU_MOVIEZ_BACK";
DATA(0x00211750)
static char s_MENU_QUESTZ_TITLE[] = "MENU_QUESTZ_TITLE";
DATA(0x00211744)
static char s_TRAINING[] = "TRAINING";
DATA(0x00211728)
static char s_MENU_QUESTZ_TRAINING[] = "MENU_QUESTZ_TRAINING";
DATA(0x00211720)
static char s_AREA1[] = "AREA1";
DATA(0x00211708)
static char s_MENU_QUESTZ_AREA1[] = "MENU_QUESTZ_AREA1";
DATA(0x00211700)
static char s_AREA2[] = "AREA2";
DATA(0x002116e8)
static char s_MENU_QUESTZ_AREA2[] = "MENU_QUESTZ_AREA2";
DATA(0x002116e0)
static char s_AREA3[] = "AREA3";
DATA(0x002116c8)
static char s_MENU_QUESTZ_AREA3[] = "MENU_QUESTZ_AREA3";
DATA(0x002116c0)
static char s_AREA4[] = "AREA4";
DATA(0x002116a8)
static char s_MENU_QUESTZ_AREA4[] = "MENU_QUESTZ_AREA4";
DATA(0x002116a0)
static char s_AREA5[] = "AREA5";
DATA(0x00211688)
static char s_MENU_QUESTZ_AREA5[] = "MENU_QUESTZ_AREA5";
DATA(0x00211680)
static char s_AREA6[] = "AREA6";
DATA(0x00211668)
static char s_MENU_QUESTZ_AREA6[] = "MENU_QUESTZ_AREA6";
DATA(0x00211660)
static char s_AREA7[] = "AREA7";
DATA(0x00211648)
static char s_MENU_QUESTZ_AREA7[] = "MENU_QUESTZ_AREA7";
DATA(0x00211640)
static char s_AREA8[] = "AREA8";
DATA(0x00211628)
static char s_MENU_QUESTZ_AREA8[] = "MENU_QUESTZ_AREA8";
DATA(0x00211614)
static char s_MENU_QUESTZ_BACK[] = "MENU_QUESTZ_BACK";
DATA(0x002115f4)
static char s_MENU_AREAS_TRAININGTITLE[] = "MENU_AREAS_TRAININGTITLE";
DATA(0x002115d4)
static char s_STAGE1[] = "STAGE1";
DATA(0x002115dc)
static char s_MENU_AREAS_STAGE1[] = "MENU_AREAS_STAGE1";
DATA(0x002115b4)
static char s_STAGE2[] = "STAGE2";
DATA(0x002115bc)
static char s_MENU_AREAS_STAGE2[] = "MENU_AREAS_STAGE2";
DATA(0x00211594)
static char s_STAGE3[] = "STAGE3";
DATA(0x0021159c)
static char s_MENU_AREAS_STAGE3[] = "MENU_AREAS_STAGE3";
DATA(0x00211574)
static char s_STAGE4[] = "STAGE4";
DATA(0x0021157c)
static char s_MENU_AREAS_STAGE4[] = "MENU_AREAS_STAGE4";
DATA(0x00211560)
static char s_MENU_AREAS_BACK[] = "MENU_AREAS_BACK";
DATA(0x00211544)
static char s_MENU_AREAS_AREA1TITLE[] = "MENU_AREAS_AREA1TITLE";
DATA(0x00211528)
static char s_MENU_AREAS_AREA2TITLE[] = "MENU_AREAS_AREA2TITLE";
DATA(0x0021150c)
static char s_MENU_AREAS_AREA3TITLE[] = "MENU_AREAS_AREA3TITLE";
DATA(0x002114f0)
static char s_MENU_AREAS_AREA4TITLE[] = "MENU_AREAS_AREA4TITLE";
DATA(0x002114d4)
static char s_MENU_AREAS_AREA5TITLE[] = "MENU_AREAS_AREA5TITLE";
DATA(0x002114b8)
static char s_MENU_AREAS_AREA6TITLE[] = "MENU_AREAS_AREA6TITLE";
DATA(0x0021149c)
static char s_MENU_AREAS_AREA7TITLE[] = "MENU_AREAS_AREA7TITLE";
DATA(0x00211480)
static char s_MENU_AREAS_AREA8TITLE[] = "MENU_AREAS_AREA8TITLE";

RVA(0x000a1190, 0x29)
void SetMenuTextRect() {
    g_menuTextRect.left = 5;
    g_menuTextRect.top = 453;
    g_menuTextRect.right = 635;
    g_menuTextRect.bottom = 478;
}

// @identity-TODO BuildMainMenuTree - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (1 fns) came from the static library. It belongs to another compiland.

// @early-stop
RVA(0x000a11d0, 0x180d)
i32 BuildMainMenuTree(CChatBox* menu, i32) {
    if (menu == NULL) {
        return 0;
    }

    CMenuPage* page;
    CMenuItem* it;
    i32 progress;

    page = new CMenuPage;
    if (page->Configure(menu, s_MAIN, s_MENU_MAINMENU_TITLE, 0, 0) == 0) {
        delete page;
        return 0;
    }
    it = page->AddItem(s_SINGLEPLAYER, s_MENU_MAINMENU_SINGLEPLAYER, 0, s_SINGLEPLAYER, 0);
    if (g_cdPromptResult != 0) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddItem(s_MULTIPLAYER, s_MENU_MAINMENU_MULTIPLAYER, 0, s_MULTIPLAYER, 0);
    page->AddItem(s_OPTIONZ, s_MENU_MAINMENU_OPTIONZ, 0x80e2, 0, 0);
    it = page->AddItem(s_MOVIEZ, s_MENU_MAINMENU_MOVIEZ, 0, s_MOVIEZ, 0);
    if (g_cdPromptResult != 0) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddItem(s_HELP, s_MENU_MAINMENU_HELP, 0x8035, 0, 0);
    page->AddItem(s_QUIT, s_MENU_MAINMENU_QUIT, 0x8008, 0, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

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
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_MULTIPLAYER, s_MENU_MULTIPLAYER_TITLE, s_MAIN, 0) == 0) {
        delete page;
        return 0;
    }
    it = page->AddItem(s_HOST, s_MENU_MULTIPLAYER_HOST, 0x80d3, 0, 0);
    if (g_cdPromptResult != 0) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddItem(s_JOIN, s_MENU_MULTIPLAYER_JOIN, 0x80d2, 0, 0);
    page->AddItem(s_BACK, s_MENU_MULTIPLAYER_BACK, 0, s_MAIN, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

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
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddItem(s_CREDITZ, s_MENU_MOVIEZ_CREDITZ, 0x8021, 0, 0);
    page->AddItem(s_BACK, s_MENU_MOVIEZ_BACK, 0, s_MAIN, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_QUESTZ, s_MENU_QUESTZ_TITLE, s_SINGLEPLAYER, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddItem(s_TRAINING, s_MENU_QUESTZ_TRAINING, 0, s_TRAINING, 0);
    page->AddSubItem(s_AREA1, s_MENU_QUESTZ_AREA1, 0x8149, AREA_ROCKY_ROADZ, 0, s_AREA1, 0);
    it = page->AddSubItem(s_AREA2, s_MENU_QUESTZ_AREA2, 0x8149, AREA_GRUNTZICLEZ, 0, s_AREA2, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x4) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(
        s_AREA3,
        s_MENU_QUESTZ_AREA3,
        0x8149,
        AREA_TROUBLE_IN_THE_TROPICZ,
        0,
        s_AREA3,
        0
    );
    if (progress > QUESTLEVEL_LAST || progress < 0x8) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_AREA4, s_MENU_QUESTZ_AREA4, 0x8149, AREA_HIGH_ON_SWEETZ, 0, s_AREA4, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xc) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_AREA5, s_MENU_QUESTZ_AREA5, 0x8149, AREA_HIGH_ROLLERZ, 0, s_AREA5, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x10) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(
        s_AREA6,
        s_MENU_QUESTZ_AREA6,
        0x8149,
        AREA_HONEY_I_SHRUNK_THE_GRUNTZ,
        0,
        s_AREA6,
        0
    );
    if (progress > QUESTLEVEL_LAST || progress < 0x14) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(
        s_AREA7,
        s_MENU_QUESTZ_AREA7,
        0x8149,
        AREA_MINIATURE_MASTERZ,
        0,
        s_AREA7,
        0
    );
    if (progress > QUESTLEVEL_LAST || progress < 0x18) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it =
        page->AddSubItem(s_AREA8, s_MENU_QUESTZ_AREA8, 0x8149, AREA_GRUNTZ_IN_SPACE, 0, s_AREA8, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1c) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddItem(s_BACK, s_MENU_QUESTZ_BACK, 0, s_SINGLEPLAYER, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_TRAINING, s_MENU_AREAS_TRAININGTITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddSubItem(
        s_STAGE1,
        s_MENU_AREAS_STAGE1,
        IDX(CMD_LOAD_WORLD),
        QUESTLEVEL_TRAINING_FIRST,
        0,
        0,
        0
    );
    page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x26, 0, 0, 0);
    page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x27, 0, 0, 0);
    page->AddSubItem(
        s_STAGE4,
        s_MENU_AREAS_STAGE4,
        IDX(CMD_LOAD_WORLD),
        QUESTLEVEL_TRAINING_LAST,
        0,
        0,
        0
    );
    page->AddItem(s_BACK, s_MENU_AREAS_BACK, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA1, s_MENU_AREAS_AREA1TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x1, 0, 0, 0);
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x2, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x3, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x2) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x4, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x3) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA2, s_MENU_AREAS_AREA2TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x5, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x4) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x6, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x5) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x7, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x6) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x8, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x7) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA3, s_MENU_AREAS_AREA3TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x9, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x8) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0xa, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x9) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0xb, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xa) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0xc, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xb) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA4, s_MENU_AREAS_AREA4TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0xd, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xc) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0xe, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xd) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0xf, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xe) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x10, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0xf) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA5, s_MENU_AREAS_AREA5TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x11, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x10) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x12, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x11) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x13, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x12) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x14, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x13) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA6, s_MENU_AREAS_AREA6TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x15, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x14) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x16, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x15) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x17, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x16) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x18, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x17) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA7, s_MENU_AREAS_AREA7TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x19, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x18) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x1a, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x19) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x1b, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1a) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x1c, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1b) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    if (menu->AddNode(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menu, s_AREA8, s_MENU_AREAS_AREA8TITLE, s_QUESTZ, 0) == 0) {
        delete page;
        return 0;
    }
    progress = g_gameReg->m_saveSink->m_curLevel;
    it = page->AddSubItem(s_STAGE1, s_MENU_AREAS_STAGE1, IDX(CMD_LOAD_WORLD), 0x1d, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1c) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE2, s_MENU_AREAS_STAGE2, IDX(CMD_LOAD_WORLD), 0x1e, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1d) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE3, s_MENU_AREAS_STAGE3, IDX(CMD_LOAD_WORLD), 0x1f, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1e) {
        it->Disable(MENUSTATE_DISABLED);
    }
    it = page->AddSubItem(s_STAGE4, s_MENU_AREAS_STAGE4, IDX(CMD_LOAD_WORLD), 0x20, 0, 0, 0);
    if (progress > QUESTLEVEL_LAST || progress < 0x1f) {
        it->Disable(MENUSTATE_DISABLED);
    }
    page->AddSubItem(s_BACK, s_MENU_AREAS_BACK, 0x8149, 0, 0, s_QUESTZ, 0);
    return menu->AddNode(page) != 0;
}
