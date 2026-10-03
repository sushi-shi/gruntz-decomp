#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/MainMenuBuilder.h>

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

CRect g_menuTextRect(5, 453, 635, 478);

static char s_menuAreasArea8Title[] = "MENU_AREAS_AREA8TITLE";

static char s_menuAreasArea7Title[] = "MENU_AREAS_AREA7TITLE";

static char s_menuAreasArea6Title[] = "MENU_AREAS_AREA6TITLE";

static char s_menuAreasArea5Title[] = "MENU_AREAS_AREA5TITLE";

static char s_menuAreasArea4Title[] = "MENU_AREAS_AREA4TITLE";

static char s_menuAreasArea3Title[] = "MENU_AREAS_AREA3TITLE";

static char s_menuAreasArea2Title[] = "MENU_AREAS_AREA2TITLE";

static char s_menuAreasArea1Title[] = "MENU_AREAS_AREA1TITLE";

static char s_menuAreasBack[] = "MENU_AREAS_BACK";

static char s_stage4[] = "STAGE4";

static char s_menuAreasStage4[] = "MENU_AREAS_STAGE4";

static char s_stage3[] = "STAGE3";

static char s_menuAreasStage3[] = "MENU_AREAS_STAGE3";

static char s_stage2[] = "STAGE2";

static char s_menuAreasStage2[] = "MENU_AREAS_STAGE2";

static char s_stage1[] = "STAGE1";

static char s_menuAreasStage1[] = "MENU_AREAS_STAGE1";

static char s_menuAreasTrainingtitle[] = "MENU_AREAS_TRAININGTITLE";

static char s_menuQuestzBack[] = "MENU_QUESTZ_BACK";

static char s_menuQuestzArea8[] = "MENU_QUESTZ_AREA8";

static char s_area8[] = "AREA8";

static char s_menuQuestzArea7[] = "MENU_QUESTZ_AREA7";

static char s_area7[] = "AREA7";

static char s_menuQuestzArea6[] = "MENU_QUESTZ_AREA6";

static char s_area6[] = "AREA6";

static char s_menuQuestzArea5[] = "MENU_QUESTZ_AREA5";

static char s_area5[] = "AREA5";

static char s_menuQuestzArea4[] = "MENU_QUESTZ_AREA4";

static char s_area4[] = "AREA4";

static char s_menuQuestzArea3[] = "MENU_QUESTZ_AREA3";

static char s_area3[] = "AREA3";

static char s_menuQuestzArea2[] = "MENU_QUESTZ_AREA2";

static char s_area2[] = "AREA2";

static char s_menuQuestzArea1[] = "MENU_QUESTZ_AREA1";

static char s_area1[] = "AREA1";

static char s_menuQuestzTraining[] = "MENU_QUESTZ_TRAINING";

static char s_menuQuestzTitle[] = "MENU_QUESTZ_TITLE";

static char s_menuMoviezBack[] = "MENU_MOVIEZ_BACK";

static char s_menuMoviezCreditz[] = "MENU_MOVIEZ_CREDITZ";

static char s_final[] = "FINAL";

static char s_menuMoviezFinal[] = "MENU_MOVIEZ_FINAL";

static char s_intro[] = "INTRO";

static char s_menuMoviezIntro[] = "MENU_MOVIEZ_INTRO";

static char s_logo[] = "LOGO";

static char s_menuMoviezLogo[] = "MENU_MOVIEZ_LOGO";

static char s_menuMoviezTitle[] = "MENU_MOVIEZ_TITLE";

static char s_menuMultiplayerBack[] = "MENU_MULTIPLAYER_BACK";

static char s_join[] = "JOIN";

static char s_menuMultiplayerJoin[] = "MENU_MULTIPLAYER_JOIN";

static char s_host[] = "HOST";

static char s_menuMultiplayerHost[] = "MENU_MULTIPLAYER_HOST";

static char s_menuMultiplayerTitle[] = "MENU_MULTIPLAYER_TITLE";

static char s_menuSingleplayerBack[] = "MENU_SINGLEPLAYER_BACK";

static char s_customlevelz[] = "CUSTOMLEVELZ";

static char s_menuSingleplayerCustomlevelz[] = "MENU_SINGLEPLAYER_CUSTOMLEVELZ";

static char s_loadgame[] = "LOADGAME";

static char s_menuSingleplayerLoadgame[] = "MENU_SINGLEPLAYER_LOADGAME";

static char s_battlez[] = "BATTLEZ";

static char s_menuSingleplayerBattlez[] = "MENU_SINGLEPLAYER_BATTLEZ";

static char s_menuSingleplayerQuestz[] = "MENU_SINGLEPLAYER_QUESTZ";

static char s_questz[] = "QUESTZ";

static char s_quickstart[] = "QUICKSTART";

static char s_menuSingleplayerQuickstart[] = "MENU_SINGLEPLAYER_QUICKSTART";

static char s_menuSingleplayerTitle[] = "MENU_SINGLEPLAYER_TITLE";

static char s_quit[] = "QUIT";

static char s_menuMainmenuQuit[] = "MENU_MAINMENU_QUIT";

static char s_menuMainmenuHelp[] = "MENU_MAINMENU_HELP";

static char s_menuMainmenuMoviez[] = "MENU_MAINMENU_MOVIEZ";

static char s_moviez[] = "MOVIEZ";

static char s_menuMainmenuOptionz[] = "MENU_MAINMENU_OPTIONZ";

static char s_menuMainmenuMultiplayer[] = "MENU_MAINMENU_MULTIPLAYER";

static char s_multiplayer[] = "MULTIPLAYER";

static char s_menuMainmenuSingleplayer[] = "MENU_MAINMENU_SINGLEPLAYER";

static char s_singleplayer[] = "SINGLEPLAYER";

static char s_main[] = "MAIN";

static char s_menuMainmenuTitle[] = "MENU_MAINMENU_TITLE";

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
