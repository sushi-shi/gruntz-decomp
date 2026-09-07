#include <rva.h>

#include <Gruntz/MainMenuBuilder.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HelpState.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/MenuTree.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/StartUpPrompt.h>
#include <Io/SaveGame.h>

typedef u32 u32;

RVA_DYNINIT(0x000a1190, 0x29, g_menuTextRect)
DATA(0x00245d88)
CRect g_menuTextRect(5, 453, 635, 478);

DATA(0x00211480)
static char s_menuAreasArea8Title[] = "MENU_AREAS_AREA8TITLE";
DATA(0x0021149c)
static char s_menuAreasArea7Title[] = "MENU_AREAS_AREA7TITLE";
DATA(0x002114b8)
static char s_menuAreasArea6Title[] = "MENU_AREAS_AREA6TITLE";
DATA(0x002114d4)
static char s_menuAreasArea5Title[] = "MENU_AREAS_AREA5TITLE";
DATA(0x002114f0)
static char s_menuAreasArea4Title[] = "MENU_AREAS_AREA4TITLE";
DATA(0x0021150c)
static char s_menuAreasArea3Title[] = "MENU_AREAS_AREA3TITLE";
DATA(0x00211528)
static char s_menuAreasArea2Title[] = "MENU_AREAS_AREA2TITLE";
DATA(0x00211544)
static char s_menuAreasArea1Title[] = "MENU_AREAS_AREA1TITLE";
DATA(0x00211560)
static char s_menuAreasBack[] = "MENU_AREAS_BACK";
DATA(0x00211574)
static char s_stage4[] = "STAGE4";
DATA(0x0021157c)
static char s_menuAreasStage4[] = "MENU_AREAS_STAGE4";
DATA(0x00211594)
static char s_stage3[] = "STAGE3";
DATA(0x0021159c)
static char s_menuAreasStage3[] = "MENU_AREAS_STAGE3";
DATA(0x002115b4)
static char s_stage2[] = "STAGE2";
DATA(0x002115bc)
static char s_menuAreasStage2[] = "MENU_AREAS_STAGE2";
DATA(0x002115d4)
static char s_stage1[] = "STAGE1";
DATA(0x002115dc)
static char s_menuAreasStage1[] = "MENU_AREAS_STAGE1";
DATA(0x002115f4)
static char s_menuAreasTrainingtitle[] = "MENU_AREAS_TRAININGTITLE";
DATA(0x00211614)
static char s_menuQuestzBack[] = "MENU_QUESTZ_BACK";
DATA(0x00211628)
static char s_menuQuestzArea8[] = "MENU_QUESTZ_AREA8";
DATA(0x00211640)
static char s_area8[] = "AREA8";
DATA(0x00211648)
static char s_menuQuestzArea7[] = "MENU_QUESTZ_AREA7";
DATA(0x00211660)
static char s_area7[] = "AREA7";
DATA(0x00211668)
static char s_menuQuestzArea6[] = "MENU_QUESTZ_AREA6";
DATA(0x00211680)
static char s_area6[] = "AREA6";
DATA(0x00211688)
static char s_menuQuestzArea5[] = "MENU_QUESTZ_AREA5";
DATA(0x002116a0)
static char s_area5[] = "AREA5";
DATA(0x002116a8)
static char s_menuQuestzArea4[] = "MENU_QUESTZ_AREA4";
DATA(0x002116c0)
static char s_area4[] = "AREA4";
DATA(0x002116c8)
static char s_menuQuestzArea3[] = "MENU_QUESTZ_AREA3";
DATA(0x002116e0)
static char s_area3[] = "AREA3";
DATA(0x002116e8)
static char s_menuQuestzArea2[] = "MENU_QUESTZ_AREA2";
DATA(0x00211700)
static char s_area2[] = "AREA2";
DATA(0x00211708)
static char s_menuQuestzArea1[] = "MENU_QUESTZ_AREA1";
DATA(0x00211720)
static char s_area1[] = "AREA1";
DATA(0x00211728)
static char s_menuQuestzTraining[] = "MENU_QUESTZ_TRAINING";
DATA(0x00211750)
static char s_menuQuestzTitle[] = "MENU_QUESTZ_TITLE";
DATA(0x00211768)
static char s_menuMoviezBack[] = "MENU_MOVIEZ_BACK";
DATA(0x0021177c)
static char s_menuMoviezCreditz[] = "MENU_MOVIEZ_CREDITZ";
DATA(0x00211794)
static char s_final[] = "FINAL";
DATA(0x0021179c)
static char s_menuMoviezFinal[] = "MENU_MOVIEZ_FINAL";
DATA(0x002117b4)
static char s_intro[] = "INTRO";
DATA(0x002117bc)
static char s_menuMoviezIntro[] = "MENU_MOVIEZ_INTRO";
DATA(0x002117d4)
static char s_logo[] = "LOGO";
DATA(0x002117dc)
static char s_menuMoviezLogo[] = "MENU_MOVIEZ_LOGO";
DATA(0x002117f0)
static char s_menuMoviezTitle[] = "MENU_MOVIEZ_TITLE";
DATA(0x00211808)
static char s_menuMultiplayerBack[] = "MENU_MULTIPLAYER_BACK";
DATA(0x00211824)
static char s_join[] = "JOIN";
DATA(0x0021182c)
static char s_menuMultiplayerJoin[] = "MENU_MULTIPLAYER_JOIN";
DATA(0x00211848)
static char s_host[] = "HOST";
DATA(0x00211850)
static char s_menuMultiplayerHost[] = "MENU_MULTIPLAYER_HOST";
DATA(0x0021186c)
static char s_menuMultiplayerTitle[] = "MENU_MULTIPLAYER_TITLE";
DATA(0x00211890)
static char s_menuSingleplayerBack[] = "MENU_SINGLEPLAYER_BACK";
DATA(0x002118ac)
static char s_customlevelz[] = "CUSTOMLEVELZ";
DATA(0x002118bc)
static char s_menuSingleplayerCustomlevelz[] = "MENU_SINGLEPLAYER_CUSTOMLEVELZ";
DATA(0x002118e4)
static char s_loadgame[] = "LOADGAME";
DATA(0x002118f0)
static char s_menuSingleplayerLoadgame[] = "MENU_SINGLEPLAYER_LOADGAME";
DATA(0x00211910)
static char s_battlez[] = "BATTLEZ";
DATA(0x0021191c)
static char s_menuSingleplayerBattlez[] = "MENU_SINGLEPLAYER_BATTLEZ";
DATA(0x0021193c)
static char s_menuSingleplayerQuestz[] = "MENU_SINGLEPLAYER_QUESTZ";
DATA(0x0021195c)
static char s_questz[] = "QUESTZ";
DATA(0x00211964)
static char s_quickstart[] = "QUICKSTART";
DATA(0x00211974)
static char s_menuSingleplayerQuickstart[] = "MENU_SINGLEPLAYER_QUICKSTART";
DATA(0x00211998)
static char s_menuSingleplayerTitle[] = "MENU_SINGLEPLAYER_TITLE";
DATA(0x002119b4)
static char s_quit[] = "QUIT";
DATA(0x002119bc)
static char s_menuMainmenuQuit[] = "MENU_MAINMENU_QUIT";
DATA(0x002119d4)
static char s_menuMainmenuHelp[] = "MENU_MAINMENU_HELP";
DATA(0x002119ec)
static char s_menuMainmenuMoviez[] = "MENU_MAINMENU_MOVIEZ";
DATA(0x00211a08)
static char s_moviez[] = "MOVIEZ";
DATA(0x00211a1c)
static char s_menuMainmenuOptionz[] = "MENU_MAINMENU_OPTIONZ";
DATA(0x00211a38)
static char s_menuMainmenuMultiplayer[] = "MENU_MAINMENU_MULTIPLAYER";
DATA(0x00211a58)
static char s_multiplayer[] = "MULTIPLAYER";
DATA(0x00211a68)
static char s_menuMainmenuSingleplayer[] = "MENU_MAINMENU_SINGLEPLAYER";
DATA(0x00211a88)
static char s_singleplayer[] = "SINGLEPLAYER";
DATA(0x00211a98)
static char s_main[] = "MAIN";
DATA(0x00211aa0)
static char s_menuMainmenuTitle[] = "MENU_MAINMENU_TITLE";

RVA(0x000a11d0, 0x180d)
i32 BuildMainMenuTree(CMenuTree* menuTree, i32) {
    if (menuTree == NULL) {
        return 0;
    }

    CMenuPage* page;
    CMenuItem* item;
    QuestLevel questProgress;

    page = new CMenuPage;
    if (page->Configure(menuTree, s_main, s_menuMainmenuTitle, NULL, MENU_PAGE_FLAGS_NONE) == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_singleplayer,
        s_menuMainmenuSingleplayer,
        0,
        s_singleplayer,
        MENU_ITEM_FLAGS_NONE
    );
    if (g_cdPromptResult != false) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(s_multiplayer, s_menuMainmenuMultiplayer, 0, s_multiplayer, MENU_ITEM_FLAGS_NONE);
    page->AddItem("OPTIONZ", s_menuMainmenuOptionz, 0x80e2, NULL, MENU_ITEM_FLAGS_NONE);
    item = page->AddItem(s_moviez, s_menuMainmenuMoviez, 0, s_moviez, MENU_ITEM_FLAGS_NONE);
    if (g_cdPromptResult != false) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(g_titleBuf, s_menuMainmenuHelp, 0x8035, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem(s_quit, s_menuMainmenuQuit, 0x8008, NULL, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(
            menuTree,
            s_singleplayer,
            s_menuSingleplayerTitle,
            s_main,
            MENU_PAGE_FLAGS_NONE
        )
        == 0) {
        delete page;
        return 0;
    }
    page->AddItem(s_quickstart, s_menuSingleplayerQuickstart, 0x8174, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem(s_questz, s_menuSingleplayerQuestz, 0, s_questz, MENU_ITEM_FLAGS_NONE);
    page->AddItem(s_battlez, s_menuSingleplayerBattlez, 0x80e1, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem(s_loadgame, s_menuSingleplayerLoadgame, 0x80ce, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem(
        s_customlevelz,
        s_menuSingleplayerCustomlevelz,
        0x8042,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    page->AddItem("BACK", s_menuSingleplayerBack, 0, s_main, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(
            menuTree,
            s_multiplayer,
            s_menuMultiplayerTitle,
            s_main,
            MENU_PAGE_FLAGS_NONE
        )
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(s_host, s_menuMultiplayerHost, 0x80d3, NULL, MENU_ITEM_FLAGS_NONE);
    if (g_cdPromptResult != false) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(s_join, s_menuMultiplayerJoin, 0x80d2, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem("BACK", s_menuMultiplayerBack, 0, s_main, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_moviez, s_menuMoviezTitle, s_main, MENU_PAGE_FLAGS_NONE) == 0) {
        delete page;
        return 0;
    }
    page->AddItem(s_logo, s_menuMoviezLogo, 0x8170, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem(s_intro, s_menuMoviezIntro, 0x8171, NULL, MENU_ITEM_FLAGS_NONE);
    item = page->AddItem(s_final, s_menuMoviezFinal, 0x8173, NULL, MENU_ITEM_FLAGS_NONE);
    if (g_gameReg->m_saveGame->CheckMagic() == 0) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem("CREDITZ", s_menuMoviezCreditz, 0x8021, NULL, MENU_ITEM_FLAGS_NONE);
    page->AddItem("BACK", s_menuMoviezBack, 0, s_main, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_questz, s_menuQuestzTitle, s_singleplayer, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    questProgress = g_gameReg->m_saveGame->CurrentLevel();
    page->AddItem("TRAINING", s_menuQuestzTraining, 0, "TRAINING", MENU_ITEM_FLAGS_NONE);
    page->AddItem(
        s_area1,
        s_menuQuestzArea1,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_ROCKY_ROADZ),
        0,
        s_area1,
        MENU_ITEM_FLAGS_NONE
    );
    item = page->AddItem(
        s_area2,
        s_menuQuestzArea2,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_GRUNTZICLEZ),
        0,
        s_area2,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA1_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area3,
        s_menuQuestzArea3,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_TROUBLE_IN_THE_TROPICZ),
        0,
        s_area3,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA2_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area4,
        s_menuQuestzArea4,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_HIGH_ON_SWEETZ),
        0,
        s_area4,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA3_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area5,
        s_menuQuestzArea5,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_HIGH_ROLLERZ),
        0,
        s_area5,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA4_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area6,
        s_menuQuestzArea6,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_HONEY_I_SHRUNK_THE_GRUNTZ),
        0,
        s_area6,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA5_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area7,
        s_menuQuestzArea7,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_MINIATURE_MASTERZ),
        0,
        s_area7,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA6_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_area8,
        s_menuQuestzArea8,
        IDX(CMD_SET_QUEST_AREA),
        IDX(AREA_GRUNTZ_IN_SPACE),
        0,
        s_area8,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA7_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem("BACK", s_menuQuestzBack, 0, s_singleplayer, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(
            menuTree,
            "TRAINING",
            s_menuAreasTrainingtitle,
            s_questz,
            MENU_PAGE_FLAGS_NONE
        )
        == 0) {
        delete page;
        return 0;
    }
    page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_TRAINING_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_TRAINING_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_TRAINING_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_TRAINING_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    page->AddItem("BACK", s_menuAreasBack, 0, s_questz, MENU_ITEM_FLAGS_NONE);
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area1, s_menuAreasArea1Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA1_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA1_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA1_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA1_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA1_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA1_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA1_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area2, s_menuAreasArea2Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA2_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA1_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA2_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA2_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA2_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA2_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA2_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA2_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area3, s_menuAreasArea3Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA3_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA2_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA3_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA3_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA3_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA3_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA3_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA3_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area4, s_menuAreasArea4Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA4_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA3_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA4_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA4_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA4_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA4_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA4_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA4_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area5, s_menuAreasArea5Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA5_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA4_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA5_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA5_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA5_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA5_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA5_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA5_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area6, s_menuAreasArea6Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA6_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA5_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA6_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA6_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA6_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA6_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA6_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA6_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area7, s_menuAreasArea7Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA7_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA6_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA7_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA7_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA7_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA7_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA7_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA7_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    if (menuTree->AddPage(page) == 0) {
        return 0;
    }

    page = new CMenuPage;
    if (page->Configure(menuTree, s_area8, s_menuAreasArea8Title, s_questz, MENU_PAGE_FLAGS_NONE)
        == 0) {
        delete page;
        return 0;
    }
    item = page->AddItem(
        s_stage1,
        s_menuAreasStage1,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA8_STAGE1),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA7_STAGE4_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage2,
        s_menuAreasStage2,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA8_STAGE2),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA8_STAGE1_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage3,
        s_menuAreasStage3,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA8_STAGE3),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA8_STAGE2_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    item = page->AddItem(
        s_stage4,
        s_menuAreasStage4,
        IDX(CMD_LOAD_WORLD),
        IDX(QUESTLEVEL_AREA8_STAGE4),
        0,
        NULL,
        MENU_ITEM_FLAGS_NONE
    );
    if (questProgress > QUESTLEVEL_LAST || questProgress < QUESTLEVEL_AREA8_STAGE3_END) {
        item->SetState(MENUSTATE_DISABLED);
    }
    page->AddItem(
        "BACK",
        s_menuAreasBack,
        IDX(CMD_SET_QUEST_AREA),
        0,
        0,
        s_questz,
        MENU_ITEM_FLAGS_NONE
    );
    return menuTree->AddPage(page) != 0;
}
