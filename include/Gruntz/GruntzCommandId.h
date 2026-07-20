// GruntzCommandId.h - the WM_COMMAND / accelerator + cheat-code id space handled by
// CGruntzMgr::HandleCommand (0x862f0) and offered up the two command virtuals
// (CGameApp::HandleCommand slot 10, CGameMgr::HandleCommand slot 5).
//
// NOTE: `enum GruntzCommand` (this file) is the command-ID space; it is NOT the
// unrelated command-pattern class `CGruntzCommand` in <Gruntz/GruntzCommand.h>
// (the queued per-grunt order object). Distinct identifiers, distinct headers.
//
// UNSCOPED, plain enum: MSVC 5.0 has no C++11 fixed-underlying-type (`enum X : i32`
// is a syntax error on this toolchain), but an unscoped enum is exactly what we
// need - it is int-width (4 bytes == i32 here, so narrowing the command param from
// i32 to GruntzCommand is byte-neutral), and it lets `switch(nID)`, `nID & 0xffff`
// and bare `case kCmd...:` labels all compile. The engine header <Wap32/Wap32.h>
// only needs the TYPE NAME for its two base virtual decls, so it carries a one-line
// MS-style opaque forward declaration (`enum GruntzCommand;`) instead of including
// this game header (engine layering). This full definition is included only by the
// Gruntz TUs that reference the enumerators (GruntzMgrCmd.cpp et al.).
//
// Names are behaviour-derived (from each case body + its announce string); the
// retail resource.h symbols are not available. kCmd* = menu/game commands (outer
// switch); kCheat* = the God-mode-gated cheat sub-switch. Debug warp-accelerator
// levels are named by their target level id (PassClickToPlayState(level)).
#ifndef GRUNTZ_GRUNTZ_GRUNTZCOMMANDID_H
#define GRUNTZ_GRUNTZ_GRUNTZCOMMANDID_H

enum GruntzCommand {
    kCmdNewGame = 0x8005,                            // PassClickToPlayState(1); "New game"
    kCmdNewGameAlt = 0x8024,                         // second accelerator, shares the 0x8005 New-Game body
    kCmdLoadWorld = 0x807f,                          // load world, start at level = lParam
    kCmdContinueAtMaxLevel = 0x8174,                 // load save, jump to m_maxLevel
    kCmdNewGameReplay = 0x80e3,                      // New Game start-mode 3 (m_134=3)
    kCmdSaveGameAs = 0x80e1,                         // SaveGameAs()
    kCheatProgrammingGod = 0x803b,                   // "Brian L. Goble is a programming God..."
    kCheatTraitorMode = 0x8043,                      // toggle Traitor Mode
    kCheatObjectCountDisplay = 0x804d,               // toggle debug obj-count overlay (flag 0x01)
    kCheatWorldPositionDisplay = 0x804c,             // toggle debug world-position overlay (flag 0x04)
    kCheatFrameRateDisplay = 0x804b,                 // toggle debug frame-rate overlay (flag 0x10)
    kCheatDebugFlag20 = 0x804e,                      // toggle debug flag 0x20 (unlabelled overlay, no announce)
    kCheatBrickTextDisplay = 0x8068,                 // toggle brick-text overlay (flag 0x40), clear both brick texts
    kCheatBrickTextAltDisplay = 0x806f,              // toggle alt brick-text overlay (flag 0x100), clear brick text 1
    kCheatElapsedTimeDisplay = 0x806e,               // toggle debug elapsed-time overlay (flag 0x80)
    kCheatMonolith = 0x8086,                         // "Monolith Rulez..."
    kCheatNoOp = 0x8087,                             // handled, no effect (returns 1)
    kCheatBrickGoAway = 0x808d,                      // "Hey, where did you go?" (brick pickup 0x36)
    kCheatGiveBomb = 0x80e5,                         // "Bombz are cool!"
    kCheatGiveBoomerang = 0x80e6,                    // "Boomerangz are cool!"
    kCheatGiveBrickLayer = 0x80e7,                   // "Brick Laying Toolz are cool!"
    kCheatGiveClub = 0x80e8,                         // "Clubz are cool!"
    kCheatGiveGauntlet = 0x80e9,                     // "Gauntletz are cool!"
    kCheatGiveGlove = 0x80ea,                        // "Glovez are cool!"
    kCheatGiveGoober = 0x80eb,                       // "Gooberz are cool!"
    kCheatGiveGravityBoot = 0x80ec,                  // "Gravity Bootz are cool!"
    kCheatGiveGunHat = 0x80ed,                       // "Gun Hatz are cool!"
    kCheatGiveSpongeGun = 0x80ee,                    // "Sponge Gunz are cool!"
    kCheatGiveRock = 0x80ef,                         // "Rockz are cool!"
    kCheatGiveShield = 0x80f0,                       // "Shieldz are cool!"
    kCheatGiveShovel = 0x80f1,                       // "Shovelz are cool!"
    kCheatGiveSpring = 0x80f2,                       // "Springz are cool!"
    kCheatGiveSpyGear = 0x80f3,                      // "Spy Gear is cool!"
    kCheatGiveSword = 0x80f4,                        // "Swordz are cool!"
    kCheatGiveTimeBomb = 0x80f5,                     // "Time Bombz are cool!"
    kCheatGiveToob = 0x80f6,                         // "Toobz are cool!"
    kCheatGiveMagicWand = 0x80f7,                    // "Magic Wandz are cool!"
    kCheatGiveSecret = 0x80f8,                       // "Hey, how did you get this cheat?"
    kCheatGiveWeldersKit = 0x80f9,                   // "Welder's Kitz are cool!"
    kCheatGiveWing = 0x80fa,                         // "Wingz are cool!"
    kCheatGiveBabyWalker = 0x80fb,                   // "Baby-Walkerz are cool!"
    kCheatGiveBeachBall = 0x80fc,                    // "Beach Ballz are cool!"
    kCheatGiveMonsterWheel = 0x80fd,                 // "Monster Wheelz are cool!"
    kCheatGiveGoKart = 0x80fe,                       // "Go-Kartz are cool!"
    kCheatGiveJackInTheBox = 0x80ff,                 // "Jack-In-The-Boxez are cool!"
    kCheatGiveJumpRope = 0x8100,                     // "Jump Ropez are cool!"
    kCheatGivePogoStick = 0x8101,                    // "Pogo Stickz are cool!"
    kCheatGiveScroll = 0x8102,                       // "Scrollz are cool!"
    kCheatGiveSqueakToy = 0x8103,                    // "Squeak Toyz are cool!"
    kCheatGiveYoYo = 0x8104,                         // "Yo-Yoz are cool!"
    kCheatNuke = 0x8106,                             // "Global thermal nuclear war!"
    kCheatKillTimer = 0x8107,                        // "Ah, who needed that stupid timer anyway?"
    kCheatGiveBombBrick = 0x8128,                    // "Bomb Brickz are cool!" (item 0x26)
    kCheatGiveIndestructibleBrick = 0x8129,          // "Indestructible Brickz are cool!" (item 0x25)
    kCheatGiveGauntletBreakerBrick = 0x812b,         // "Gauntlet-Breaker Brickz are cool!" (item 0x23)
    kCheatGiveTeleportBrick = 0x812a,                // "Teleport Brickz are cool!" (item 0x24)
    kCheatBrickAssimilate = 0x8130,                  // "Oh yes, they will be assimilated!" (pickup 0x39)
    kCheatBrickDeath = 0x8131,                       // "Ladies and gentlemen, please welcome... death..." (0x3a)
    kCheatBrickSuperGrunt = 0x8132,                  // "Super Grunt to the rescue!" (pickup 0x38)
    kCheatBrickHurt = 0x8133,                        // "This is gonna hurt them more..." (pickup 0x3c)
    kCheatBrickSwallow = 0x8134,                     // "How did you swallow that?" (pickup 0x3b)
    kCheatBrickNoRunning = 0x8135,                   // "There is no running allowed by the pool!" (pickup 0x37)
    kCheatColorGruntz = 0x8136,                      // "How about a little color in your Gruntz?" (CycleMoveIcons)
    kCheatRegionMonitor = 0x8137,                    // "Whoah... you should get this monitor fixed." (OnRegion3)
    kCheatRegionDark = 0x8138,                       // "Is is dark in here?" (OnRegion1)
    kCheatRegionWindow = 0x8139,                     // "Awww... isn't this little window cute?" (OnRegion2)
    kCheatAbilityFreeze = 0x813c,                    // "Freeze spellz are coooool!" (ability 1)
    kCheatAbilityHeal = 0x813d,                      // "...you too can have the healing power!" (ability 2)
    kCheatAbilityZombie = 0x813e,                    // "Aaahh!  Zombiez!" (ability 3)
    kCheatAbilityParty = 0x813a,                     // "It's party time!" (ability 4)
    kCheatAbilityTeleport = 0x813f,                  // "...where did the teleported Gruntz go?" (ability 5)
    kCheatAbilityRoll = 0x813b,                      // "Rollin, rollin, rollin." (ability 6)
    kCheatDebugFlag400 = 0x816f,                     // toggle debug flag 0x400 (unlabelled overlay, no announce)
    kCheatWawa = 0x8175,                             // "WA WA WA WA WA WA!"
    kCheatKevinLambert = 0x807a,                     // "My name is Kevin Lambert..." (UpdateDestructButton)
    kCheatKevinLambertAlt = 0x807b,                  // shares the Kevin-Lambert body
    kCheatKevinLambertAlt2 = 0x8246,                 // shares the Kevin-Lambert body
    kCheatGooPuddlez = 0x81a3,                       // toggle "Goo puddlez"
    kCheatFillGoo = 0x81a4,                          // "May your Wellz be full of Goo!" (AdvanceGauge +100)
    kCheatGruntCreation = 0x81a5,                    // toggle "Grunt creation"
    kCheatGruntDestruction = 0x81a6,                 // toggle "Grunt destruction"
    kCheatCheatelson = 0x81a9,                       // "...Cheat Cheatelson..." (SetCurLevel 0x20)
    kCheatPsyche = 0x81d6,                           // PSYCHE modal dialog
    kCheatClearCheats = 0x81d7,                      // "Cheatz cleared"
    kCheatWarpTropicz = 0x8240,                      // "Warp to Trouble in the Tropicz activated!" (level 8)
    kCheatWarpSweetz = 0x8241,                       // "Warp to High on Sweetz activated!" (level 0xc)
    kCheatWarpRollerz = 0x8242,                      // "Warp to High Rollerz activated!" (level 0x10)
    kCheatWarpHoneyShrunk = 0x8243,                  // "Warp to Honey, I Shrunk the Gruntz activated!" (level 0x14)
    kCheatWarpMiniatureMasterz = 0x8244,             // "Warp to The Miniature Masterz activated!" (level 0x18)
    kCheatWarpGruntzInSpace = 0x8245,                // "Warp to Gruntz in Space activated!" (level 0x1c)
    kCheatExplosionz = 0x8247,                       // toggle "Explosionz"
    kCmdLoadSavedGame = 0x807e,                      // restore SaveInfo, PassClickToPlayState(levelId), ParseSerial
    kCmdNoOp80b8 = 0x80b8,                           // handled, returns 1
    kCmdMultiConnect = 0x80d7,                       // CMulti::Connect(lParam) when state NONE
    kCmdLoadGameDialog = 0x80ce,                     // open the GAME_LOAD dialog
    kCmdQuickSavePrompt = 0x80cf,                    // LoadSaveMessageSprite if CanQuickSave
    kCmdQuickSave = 0x80d8,                          // Quicksave if CanQuickSave
    kCmdQuickLoad = 0x80d9,                          // Quickload
    kCmdRestartLevel = 0x8170,                       // RESTART(1)
    kCmdRestartWorld = 0x8171,                       // RESTART(2)
    kCmdRestartWorldNoCursor = 0x8172,               // RESTART2(2) (no cursor drain)
    kCmdRestartGame = 0x8173,                        // RESTART(3)
    kCmdWarpLevel1 = 0x800d,                         // WARP(1) primary first-level warp
    kCmdDebugWarpLevel37 = 0x814a,
    kCmdDebugWarpLevel38 = 0x814b,
    kCmdDebugWarpLevel39 = 0x814c,
    kCmdDebugWarpLevel40 = 0x814d,
    kCmdDebugWarpLevel1 = 0x814e,
    kCmdDebugWarpLevel2 = 0x814f,
    kCmdDebugWarpLevel3 = 0x8150,
    kCmdDebugWarpLevel4 = 0x8151,
    kCmdDebugWarpLevel5 = 0x8152,
    kCmdDebugWarpLevel6 = 0x8153,
    kCmdDebugWarpLevel7 = 0x8154,
    kCmdDebugWarpLevel8 = 0x8155,
    kCmdDebugWarpLevel9 = 0x8156,
    kCmdDebugWarpLevel10 = 0x8157,
    kCmdDebugWarpLevel11 = 0x8158,
    kCmdDebugWarpLevel12 = 0x8159,
    kCmdDebugWarpLevel13 = 0x815a,
    kCmdDebugWarpLevel14 = 0x815b,
    kCmdDebugWarpLevel15 = 0x815c,
    kCmdDebugWarpLevel16 = 0x815d,
    kCmdDebugWarpLevel17 = 0x815e,
    kCmdDebugWarpLevel18 = 0x815f,
    kCmdDebugWarpLevel19 = 0x8160,
    kCmdDebugWarpLevel20 = 0x8161,
    kCmdDebugWarpLevel21 = 0x8162,
    kCmdDebugWarpLevel22 = 0x8163,
    kCmdDebugWarpLevel23 = 0x8164,
    kCmdDebugWarpLevel24 = 0x8165,
    kCmdDebugWarpLevel25 = 0x8166,
    kCmdDebugWarpLevel26 = 0x8167,
    kCmdDebugWarpLevel27 = 0x8168,
    kCmdDebugWarpLevel28 = 0x8169,
    kCmdDebugWarpLevel29 = 0x816a,
    kCmdDebugWarpLevel30 = 0x816b,
    kCmdDebugWarpLevel31 = 0x816c,
    kCmdDebugWarpLevel32 = 0x816d,
    kCmdWebSite = 0x8038,                            // launch www.gruntzgoo.com
    kCmdMultiJoin = 0x80d2,                          // join a multiplayer game (state 0x11/2)
    kCmdMultiHost = 0x80d3,                          // host a multiplayer game (state 0x11/2)
    kCmdMainMenu = 0x8023,                           // transition to MENU (state 5)
    kCmdShowCredits = 0x8080,                        // transition to state 0xb
    kCmdShowBooty = 0x8090,                          // transition to state 0xd (lParam-carrying)
    kCmdNextState = 0x8036,                          // SwitchToNextState
    kCmdShowHelp = 0x8021,                           // transition to state 8, else 5
    kCmdAttract = 0x8027,                            // transition to ATTRACT (state 2)
    kCmdReturnToAttract = 0x8029,                    // transition state 2, then post MENU
    kCmdShowState0e = 0x80ab,                        // transition to state 0xe
    kCmdShowState07 = 0x8022,                        // transition to state 7
    kCmdPauseToggle = 0x8007,                        // FinishLevel toggle (pause)
    kCmdFinishLevel = 0x816e,                        // FinishLevel(gate, 0)
    kCmdPresentWorld = 0x8084,                       // DrawWorldPresent
    kCmdLobbyReset = 0x80b7,                         // reset lobby, post 0x8025
    kCmdExitToAttract = 0x800e,                      // transition to state 2, then post MENU
    kCmdCaptureWorld = 0x8042,                       // CaptureWorldFile
    kCmdNextLevel = 0x8075,                          // GoToNextLevel
    kCmdPrevLevel = 0x800f,                          // GoToPrevLevel, then fall into MENU
    kCmdReturnToMenu = 0x8006,                       // m_40=1, transition to MENU (state 5)
    kCmdQuit = 0x8008,                               // DelayedQuit
    kCmdShowBootyState = 0x8035,                     // transition to state 9
    kCmdConfigSettings = 0x80e2,                     // CONFIG_SETTINGS modal
    kCmdToggleMusic = 0x800a,                        // toggle music
    kCmdToggleSound = 0x8009,                        // toggle sound
    kCmdRestoreVideoMode = 0x802c,                   // RestoreVideoMode(0)
    kCmdCheckDisplayBoundsA = 0x802a,                // CheckDisplayBoundsA
    kCmdCheckDisplayBoundsB = 0x802b,                // CheckDisplayBoundsB
    kCmdScreenshot = 0x8070,                         // SaveScreenshot (front surface)
    kCmdReloadLevel = 0x806b,                        // reload current level (PassClickToPlayState(m_levelIndex))
    kCmdDebugWarpLevel101 = 0x81b6,
    kCmdDebugWarpLevel102 = 0x81b7,
    kCmdDebugWarpLevel103 = 0x81b8,
    kCmdDebugWarpLevel104 = 0x81b9,
    kCmdDebugWarpLevel105 = 0x81ba,
    kCmdDebugWarpLevel106 = 0x81bb,
    kCmdDebugWarpLevel107 = 0x81bc,
    kCmdDebugWarpLevel108 = 0x81bd,
    kCmdDebugWarpLevel109 = 0x81be,
    kCmdDebugWarpLevel110 = 0x81bf,
    kCmdDebugWarpLevel111 = 0x81c0,
    kCmdDebugWarpLevel112 = 0x81c1,
    kCmdDebugWarpLevel113 = 0x81c2,
    kCmdDebugWarpLevel114 = 0x81c3,
    kCmdDebugWarpLevel115 = 0x81c4,
    kCmdDebugWarpLevel116 = 0x81c5,
    kCmdDebugWarpLevel117 = 0x81c6,
    kCmdDebugWarpLevel118 = 0x81c7,
    kCmdDebugWarpLevel119 = 0x81c8,
    kCmdDebugWarpLevel120 = 0x81c9,
    kCmdDebugWarpLevel121 = 0x81ca,
    kCmdDebugWarpLevel122 = 0x81cb,
    kCmdDebugWarpLevel123 = 0x81cc,
    kCmdDebugWarpLevel124 = 0x81cd,
    kCmdDebugWarpLevel125 = 0x81ce,
    kCmdDebugWarpLevel126 = 0x81cf,
    kCmdDebugWarpLevel127 = 0x81d0,
    kCmdDebugWarpLevel128 = 0x81d1,
    kCmdDebugWarpLevel129 = 0x81d2,
    kCmdDebugWarpLevel130 = 0x81d3,
    kCmdDebugWarpLevel131 = 0x81d4,
    kCmdDebugWarpLevel132 = 0x81d5,
};

#endif // GRUNTZ_GRUNTZ_GRUNTZCOMMANDID_H
