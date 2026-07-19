// GruntzMgrCmd.cpp - CGruntzMgr::HandleCommand (RVA 0x862f0): the game's
// WM_COMMAND / accelerator + cheat-code dispatcher - the binary's single largest
// function, defined here in its own TU (the class lives in <Gruntz/GruntzMgr.h>;
// GruntzMgr.cpp holds the other manager methods).
//
// FULLY CANONICAL: the former `namespace GruntzMgrCmd` view-world (a namespaced
// CGruntzMgr shadow + the GZ* per-TU views: GZLogic/GZBoard/GZGrunt/GZGruntLevel/
// GZLevel/GZInput/GZSoundZ/GZMgrSettings/GZDeath*/GZPtrMap/GZSel/GZCell/GZStr/
//   this            = ::CGruntzMgr (GruntzMgr.h; base WAP32::CGameMgr gives
//                     m_gameWnd/m_frameGate/m_soundEnabled/m_musicEnabled)
//   m_curState      = CState (Update() slot 4 == the old vf10; Vslot15 == vf54);
//                     downcast per proven state id: CPlay (3), CMenuState (5,
//                     GameMode.h StopMusicChain/StartMusic), CMulti (0x11, Connect)
//   PickPlayOrPausedState() (thunk 0x355d) yields the CPlay the cheats drive
//                     (SetCursorFrame/Flip/OnRegion1-3/CanQuickSave/m_frameMarker)
//   m_cmdGrid       = CTriggerMgr (ClearRowAndRefresh 0x18e3, CycleMoveIcons 0x3616,
//                     m_grid/m_recHead/m_recCount)
//   m_world->m_soundRegistry   = CSndHost (SoundCue.h; CueLookup @0x05b7e0 was the fake
//                     stdcall "CueLookup" free fn - it is a real CSndHost thiscall)
//   m_sound         = CGruntzSoundZ (Restart 0x1388c0 / StopAll 0x1388f0)
//   m_saveSink      = the CSaveGame (Io/SaveGame.h), m_saveInfoRec = SaveInfo (SaveInfo.h)
//   *0x64556c       = g_gameReg (CGameRegistry; the death cheat's key is
//                     m_focusSlots[0].m_0c, its map CDDrawChildGroup::m_map48)
// Grid cells are CGrunt (m_tileOwnerHi/Lo @+0x1ec/+0x1f0, LoadPickupSprites
// @0x65e80, LoadGruntAbilityTuning @0x57100); the CTmCell==CGrunt retype in
// TriggerMgr.h is deferred (owned by a parallel worker) - the two casts below
// carry the evidence. CPlay::m_guts' concrete type is CStatusBarMgr
// (UpdateDestructButton @0x10bc30 / AdvanceGauge @0x105750) - the member retype
// is deferred to the Play.cpp reconciliation.
#include <Ints.h>
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/SoundState.h>  // g_sndEnabled/g_sndCueTag
#include <Gruntz/TraitorMode.h> // g_traitorMode
#include <Gruntz/LeafCue.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Mfc.h>

#include <rva.h>
#include <string.h>

#include <Gruntz/GameRegistry.h>     // CGameRegistry (g_gameReg) + CDDrawSurfaceMgr
#include <Gruntz/GruntzCommandId.h>  // GruntzCommand enum (nID param + the case labels)
#include <Gruntz/GruntzMgr.h>        // the real CGruntzMgr (this) + SaveInfo.h + SoundCue.h
#include <Gruntz/CheatMgr.h>         // CCheatMgr (m_cheatMgr->m_124 - the "Cheatz cleared" flag)
#include <Gruntz/Play.h>             // CPlay (the cheat receiver) + CTimer (m_frameMarker)
#include <Gruntz/GameMode.h>         // CMenuState (StopMusicChain/StartMusic; state 5)
#include <Gruntz/Multi.h>            // CMulti (Connect; state 0x11)
#include <Gruntz/TriggerMgr.h>       // CTriggerMgr (m_cmdGrid)
#include <Wwd/WwdGameObjectFamily.h> // CWwdGameObjectE (the wide-object family base)
#include <Gruntz/Grunt.h>            // CGrunt (grid cells) + GruntObjEntry (death chain)
#include <Gruntz/StatusBarMgr.h>     // CStatusBarMgr (the play state's +0x2dc guts receiver)
#include <Dsndmgr/GruntzSoundZ.h>    // CGruntzSoundZ (m_sound)
#include <Gruntz/WorldSoundSet.h>    // CWorldSoundSet (m_inputState @+0x54; Stop/Resume)

// The *0x24556c game-manager singleton. Declared here (it used to arrive from
// <Gruntz/Play.h>, whose header-level decl was removed so each TU can pick the view /
// real class it needs -- see the note in Play.h). Type unchanged for this TU.

// node IS the MFC CPtrList CNode, and `((CTmNode*)GetHeadPosition())->m_pt` is
// spelled with the real accessor `reinterpret_cast<CTrigPoint*>(m_recList.GetHead())` below - the
// identical two loads (m_pNodeHead, then ->data), byte-proven.)

// The game-manager singleton global comes in WwdGameReg-typed via Grunt.h
// (WwdGameReg.h) - the grunt-facet view of the same *0x64556c object this class
// IS. The two sites below that need the CGameRegistry facet (m_focusSlots /
// m_modeW) bridge-cast; the WwdGameReg->CGameRegistry view unification is a
// deferred Grunt.h-scale fold.
// Cheat toggles (named from their own ShowToggleMessage strings; DEFINED extern "C"
// in GruntzMgr.cpp with their .bss band).
extern "C" u32 g_gruntDestruction; // "Grunt destruction"
extern "C" u32 g_gruntCreation;    // "Grunt creation"
extern "C" u32 g_gooPuddlez;       // "Goo puddlez"
extern "C" i32 g_monologoShown;    // the MONOLITH logo is on screen (LoadMonologoSprite)
// The CD-prompt result gate (0x6455ec; def: StartUpPrompt.cpp). The old local
// hex-named alias ("load/quicksave-UI suppress gate") was this same cell: no CD
// present suppresses the save/load UI.
extern "C" i32 g_cdPromptResult;
extern i32 g_debugDisplayFlags; // bits: 1 obj count, 4 world pos, 0x10 frame rate,
                                // 0x20/0x400 ?, 0x40/0x100 brick text, 0x80 elapsed time
extern "C" u32 g_explosionz;    // "Explosionz"
// the draw-clock mirror (here: the 0x8247 cue-cooldown throttle). Was a C++-mangled
// ?g_time6bf3c0@@3HA - a divergent symbol for a cell 9 other TUs share.
extern i32 g_isHost_648cf0;
extern i32(__cdecl* g_pwsprintfA)(char*, const char*, ...);

// The two brick-display strings the 0x8068/0x806f cheats clear (real CStrings;
// the "brick text" debug overlay renders them).
extern CString g_brickText1;
extern CString g_brickText2;

// ParseSerial (@0x0d210) + SerialObjectFactory (@0x0d2a0) carved out to
// src/Gruntz/SerialObjectFactory.cpp in REHOME D9 (their contiguous 0xd210-0xec24
// .text block is a separate obj). HandleCommand's WM_COMMAND 0x807e path calls
// ParseSerial cross-TU (declared here, defined there).
i32 ParseSerial(CGameRegistry* mgr, char* s); // 0x0d210 (SerialObjectFactory.cpp)

// The screenshot dispatch chain (thunk 0x277a -> the 6-arg __cdecl forwarder
// Fwd114ec0 @0x114ec0 -> the guarded Fwd114f00 @0x114f00 -> SaveScreenshot
// @0x114ff0 on the front surface). Both forwarders were carved out to
// src/Gruntz/Fwd114ec0.cpp in REHOME D9 (their 0x114ec0-0x114f3e block is a
// separate obj); HandleCommand's WM_COMMAND 0x8070 path calls Fwd114ec0 cross-TU
// (declared here, defined there). Params are SaveScreenshot's own tail.
void Fwd114ec0(Utils::RegistryHelper* bute, CGruntzMgr* mgr, i32 w, i32 h, char* name, void* arg7);

// Play a named cue when the world's cue host is not muted: resolve via the
// host's CueLookup (a real CSndHost __thiscall @0x05b7e0 - ecx is the host at
// every retail site because the m_emitGate gate test just loaded it).
#define PLAYCUE(TAG)                                                                               \
    if (m_world->m_soundRegistry->m_emitGate == 0) {                                               \
        LeafCue* _c =                                                                              \
            reinterpret_cast<LeafCue*>(m_world->m_soundRegistry->Lookup_05b7e0(TAG));       \
        if (_c)                                                                                    \
            _c->PlayIfElapsed(g_sndCueTag, 0, 0, 0);                                               \
    }
// Cue via the host's finder (m_10, CSndFinder) with a stack out-ptr; used by a
// handful of cheats instead of CueLookup.
#define PLAYCUE_MAP(TAG)                                                                           \
    if (m_world->m_soundRegistry->m_emitGate == 0) {                                               \
        LeafCue* _c = 0;                                                                           \
        m_world->m_soundRegistry->m_10.Lookup(TAG, (void*&)_c); /* CMapStringToPtr (0x1b8438) */   \
        if (_c)                                                                                    \
            _c->PlayIfElapsed(g_sndCueTag, 0, 0, 0);                                               \
    }
#define ITEMCHEAT(N, MSG)                                                                          \
    {                                                                                              \
        CPlay* _g = PickPlayOrPausedState();                                                       \
        if (!_g)                                                                                   \
            return 0;                                                                              \
        _g->SetCursorFrame(N);                                                                     \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }
#define WARP(N, ERR)                                                                               \
    {                                                                                              \
        m_134 = 1;                                                                                 \
        m_strWorldFile.Empty();                                                                    \
        if (!PassClickToPlayState((N), 0, 1))                                                      \
            ReportError(0x8005, (ERR));                                                            \
        return 1;                                                                                  \
    }
// Grid-select the addressed cell (m_cmdGrid), grant a brick pickup and announce.
// The grid cells are CGrunt (CTmCell==CGrunt; TriggerMgr.h retype deferred).
// Retail inlines the whole grid walk (no BrickPickup helper call).
#define BRICKPICKUP(ID, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell = m_cmdGrid->m_recList.GetCount() == 1                                       \
                            ? reinterpret_cast<CGrunt*>(                                           \
                                  m_cmdGrid->m_grid                                                \
                                      [reinterpret_cast<CTrigPoint*>(                              \
                                           m_cmdGrid->m_recList.GetHead())                         \
                                           ->y                                                     \
                                       + reinterpret_cast<CTrigPoint*>(                            \
                                             m_cmdGrid->m_recList.GetHead())                       \
                                                 ->x                                               \
                                             * 15])                                                \
                            : 0;                                                                   \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_tileOwnerHi != g_curPlayer)                                                   \
            return 0;                                                                              \
        CGrunt* _c2 =                                                                              \
            reinterpret_cast<CGrunt*>(                                                             \
                m_cmdGrid->m_grid[_cell->m_tileOwnerLo + _cell->m_tileOwnerHi * 15]);              \
        i32 _r = (_c2 && _c2->m_entranceCommitted) ? _c2->LoadPickupSprites(ID, 0, 0, 0, 1) : 0;   \
        if (!_r)                                                                                   \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }
// Grid-select the addressed cell, grant an ability and announce.
#define BRICKABILITY(N, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell = m_cmdGrid->m_recList.GetCount() == 1                                       \
                            ? reinterpret_cast<CGrunt*>(                                           \
                                  m_cmdGrid->m_grid                                                \
                                      [reinterpret_cast<CTrigPoint*>(                              \
                                           m_cmdGrid->m_recList.GetHead())                         \
                                           ->y                                                     \
                                       + reinterpret_cast<CTrigPoint*>(                            \
                                             m_cmdGrid->m_recList.GetHead())                       \
                                                 ->x                                               \
                                             * 15])                                                \
                            : 0;                                                                   \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_tileOwnerHi != g_curPlayer)                                                   \
            return 0;                                                                              \
        if (!_cell->LoadGruntAbilityTuning(N))                                                     \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }
// Level-restart (0x8170/0x8171/0x8173): stop the menu music chain, drain cursor,
// change state, restart music, re-raise cursor. Inlined by retail. The state==5
// object is the CMenuState (GameMode.h).
#define RESTART(N)                                                                                 \
    {                                                                                              \
        i32 st = m_curState->Update();                                                             \
        CMenuState* mus = 0;                                                                       \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                                         \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                                           \
            if (::ShowCursor(0) >= 0)                                                              \
                while (::ShowCursor(0) >= 0) {                                                     \
                }                                                                                  \
        }                                                                                          \
        ChangeState_8fab0(N);                                                                      \
        if (mus) {                                                                                 \
            mus->StartMusic();                                                                     \
            if (::ShowCursor(1) < 0)                                                               \
                while (::ShowCursor(1) < 0) {                                                      \
                }                                                                                  \
        }                                                                                          \
        return 1;                                                                                  \
    }
// Simpler restart variant (0x8172): no cursor drain.
#define RESTART2(N)                                                                                \
    {                                                                                              \
        i32 st = m_curState->Update();                                                             \
        CMenuState* mus = 0;                                                                       \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                                         \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                                           \
        }                                                                                          \
        ChangeState_8fab0(N);                                                                      \
        if (mus)                                                                                   \
            mus->StartMusic();                                                                     \
        return 1;                                                                                  \
    }

// @early-stop  (~67.8%, up from 39.1% - dispatch cracked, cheat block reconstructed)
// Complete two-level dispatcher (117 outer UI cases + the mode==3 cheat
// sub-switch).  The OUTER dispatch matches retail byte-for-byte (packed byte-index
// table; see docs/patterns/switch-density-byte-index-table-vs-tree.md).  Phase 2
// reconstructed the mode-gated cheat block (0x86403..0x8875c, ~58% of the 15706-byte
// fn) from templated macros to the REAL bodies, verified per-cheat vs the delinked
// target: AMBIENT%d wsprintf + char[128] buffer (0x8086), the inlined grid-cell
// select for all brick pickup/ability cheats (0x808d/0x8130-0x813f), the settings/
// CMapPtrToPtr death lookup (0x8106), the map-cue (CMapStringToOb::Lookup) warps
// (0x8244/0x8245) + throttled-Play Explosionz (0x8247), grunt->m_defenderRadius cheats
// (0x8246/0x81a4), the 0x8068/0x806f global-CString clears, WAWA's Play(0x64), and
// per-cheat case order/ids/cue-strings corrected against the jump tables.  The
// char[128] AMBIENT buffer made retail's whole-function frame emerge (sub esp,0x84
// now, was none).
//
// RESIDUAL (parked ~32%): the documented holistic megafunction frame/regalloc wall
// (docs/patterns/megafunction-cached-locals-vs-reload-regalloc.md).  Retail's frame
// is sub esp,0x94 = char[128] buffer(0x80) + FIVE distinct 4-byte out-ptr/CString
// slots (esp+0xc/0x10/0x14/0x18/0x1c) that MSVC5 keeps SEPARATE from the buffer;
// my wine-MSVC5 COALESCES those disjoint-scope out-params ONTO the AMBIENT buffer's
// space (frame sub esp,0x84 = buffer + 1 slot), so every special-cheat esp offset
// (the AMBIENT lea [esp+0x20], the map-cue [esp+0x1c/0x10], etc.) sits 0x10 lower
// than retail, and retail's ebx=1 constant pin (push ebx) never appears (my writes
// use immediate 'mov [mem],1').  These are source-uncontrollable MSVC5 stack-packing
// + regalloc coin-flips, same class as CGamePlayInput::DispatchKey's 78.5% cap.
RVA(0x000862f0, 0x3d5a)
i32 CGruntzMgr::HandleCommand(i32 notifyCode, GruntzCommand nID, i32 lParam) {
    switch (nID) {
        case kCmdNewGame:
        case kCmdNewGameAlt:
            m_134 = 1;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(0x8005, 0x41e);
            }
            return 1;
        case kCmdLoadWorld:
            m_strWorldFile.Empty();
            m_134 = 1;
            if (!PassClickToPlayState(lParam, 0, 1)) {
                ReportError(0x8005, 0x41f);
            }
            return 1;
        case kCmdContinueAtMaxLevel:
            m_strWorldFile.Empty();
            m_134 = 1;
            // retail: `mov eax,[esi+0x58]; mov ecx,[eax+0x18]` - NO null test, and the
            // field is m_maxLevel (+0x18), not m_curLevel (the ternary +
            // offset were both wrong).
            if (!PassClickToPlayState(m_saveSink->m_maxLevel, 0, 1)) {
                ReportError(0x8005, 0x41f);
            }
            return 1;
        case kCmdNewGameReplay:
            m_134 = 3;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(0x8005, 0x420);
            }
            return 1;
        case kCmdSaveGameAs:
            SaveGameAs();
            // fall through to default
        default:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                switch (nID & 0xffff) {
                    case kCheatProgrammingGod: {
                        if (m_world->m_soundRegistry->m_emitGate == 0) {
                            LeafCue* _c =
                                (LeafCue*)(static_cast<CDDrawSubMgrLeafScan*>(m_world->m_soundRegistry))
                                    ->Lookup_05b7e0("GAME_MINORCHEAT");
                            if (_c) {
                                _c->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("Brian L. Goble is a programming God...");
                        return 1;
                    }
                    case kCheatTraitorMode:
                        g_traitorMode ^= 1;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Traitor Mode", g_traitorMode);
                        return 1;
                    case kCheatObjectCountDisplay:
                        g_debugDisplayFlags ^= 1;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Object Count Display", g_debugDisplayFlags & 1);
                        return 1;
                    case kCheatWorldPositionDisplay:
                        g_debugDisplayFlags ^= 4;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("World Position Display", g_debugDisplayFlags & 4);
                        return 1;
                    case kCheatFrameRateDisplay:
                        g_debugDisplayFlags ^= 0x10;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Frame Rate Display", g_debugDisplayFlags & 0x10);
                        return 1;
                    case kCheatDebugFlag20:
                        g_debugDisplayFlags ^= 0x20;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case kCheatBrickTextDisplay:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ 0x40) & ~0x100;
                        g_brickText1.Empty();
                        g_brickText2.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case kCheatBrickTextAltDisplay:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ 0x100) & ~0x40;
                        g_brickText1.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case kCheatElapsedTimeDisplay:
                        g_debugDisplayFlags ^= 0x80;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Elapsed Time Display", g_debugDisplayFlags & 0x80);
                        return 1;
                    case kCheatMonolith: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 1;
                        }
                        if (!LoadMonologoSprite()) {
                            return 1;
                        }
                        PLAYCUE("GAME_MONOLITH");
                        AppendChatMessage("Monolith Rulez...");
                        if (!m_musicEnabled) {
                            return 1;
                        }
                        if (g_monologoShown) {
                            m_sound->PlayByName("MONOLITH", 1);
                            return 1;
                        }
                        char buf[128];
                        g_pwsprintfA(buf, "AMBIENT%d", _g->GetAmbientId()); // 0xda200 (canonical)
                        m_sound->PlayByName(buf, 1);
                        return 1;
                    }
                    case kCheatNoOp:
                        return 1;
                    case kCheatBrickGoAway:
                        BRICKPICKUP(0x36, "Hey, where did you go?");
                    // ---- item cheats 0x80e5..0x8104 (SetItem N, announce) ----
                    case kCheatGiveBomb:
                        ITEMCHEAT(1, "Bombz are cool!");
                    case kCheatGiveBoomerang:
                        ITEMCHEAT(2, "Boomerangz are cool!");
                    case kCheatGiveBrickLayer:
                        ITEMCHEAT(3, "Brick Laying Toolz are cool!");
                    case kCheatGiveClub:
                        ITEMCHEAT(4, "Clubz are cool!");
                    case kCheatGiveGauntlet:
                        ITEMCHEAT(5, "Gauntletz are cool!");
                    case kCheatGiveGlove:
                        ITEMCHEAT(6, "Glovez are cool!");
                    case kCheatGiveGoober:
                        ITEMCHEAT(7, "Gooberz are cool!");
                    case kCheatGiveGravityBoot:
                        ITEMCHEAT(8, "Gravity Bootz are cool!");
                    case kCheatGiveGunHat:
                        ITEMCHEAT(9, "Gun Hatz are cool!");
                    case kCheatGiveSpongeGun:
                        ITEMCHEAT(0xa, "Sponge Gunz are cool!");
                    case kCheatGiveRock:
                        ITEMCHEAT(0xb, "Rockz are cool!");
                    case kCheatGiveShield:
                        ITEMCHEAT(0xc, "Shieldz are cool!");
                    case kCheatGiveShovel:
                        ITEMCHEAT(0xd, "Shovelz are cool!");
                    case kCheatGiveSpring:
                        ITEMCHEAT(0xe, "Springz are cool!");
                    case kCheatGiveSpyGear:
                        ITEMCHEAT(0xf, "Spy Gear is cool!");
                    case kCheatGiveSword:
                        ITEMCHEAT(0x10, "Swordz are cool!");
                    case kCheatGiveTimeBomb:
                        ITEMCHEAT(0x11, "Time Bombz are cool!");
                    case kCheatGiveToob:
                        ITEMCHEAT(0x12, "Toobz are cool!");
                    case kCheatGiveMagicWand:
                        ITEMCHEAT(0x13, "Magic Wandz are cool!");
                    case kCheatGiveSecret:
                        ITEMCHEAT(0x14, "Hey, how did you get this cheat?");
                    case kCheatGiveWeldersKit:
                        ITEMCHEAT(0x15, "Welder's Kitz are cool!");
                    case kCheatGiveWing:
                        ITEMCHEAT(0x16, "Wingz are cool!");
                    case kCheatGiveBabyWalker:
                        ITEMCHEAT(0x17, "Baby-Walkerz are cool!");
                    case kCheatGiveBeachBall:
                        ITEMCHEAT(0x18, "Beach Ballz are cool!");
                    case kCheatGiveMonsterWheel:
                        ITEMCHEAT(0x19, "Monster Wheelz are cool!");
                    case kCheatGiveGoKart:
                        ITEMCHEAT(0x1a, "Go-Kartz are cool!");
                    case kCheatGiveJackInTheBox:
                        ITEMCHEAT(0x1b, "Jack-In-The-Boxez are cool!");
                    case kCheatGiveJumpRope:
                        ITEMCHEAT(0x1c, "Jump Ropez are cool!");
                    case kCheatGivePogoStick:
                        ITEMCHEAT(0x1d, "Pogo Stickz are cool!");
                    case kCheatGiveScroll:
                        ITEMCHEAT(0x1e, "Scrollz are cool!");
                    case kCheatGiveSqueakToy:
                        ITEMCHEAT(0x1f, "Squeak Toyz are cool!");
                    case kCheatGiveYoYo:
                        ITEMCHEAT(0x20, "Yo-Yoz are cool!");
                    case kCheatNuke: {
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_cmdGrid->ClearRowAndRefresh(5);
                        CGameRegistry* _s =
                            (CGameRegistry*)g_gameReg; // dual-view bridge (same object)
                        void* _key = (void*)_s->m_focusSlots[0].m_0c; // death/monologo sprite key
                        if (_key) {
                            CWwdGameObjectE* _dr = 0;
                            if (_s->m_world->m_childGroup->m_map48.Lookup((void*)_key, (void*&)_dr)
                                && _dr) {
                                // the entry's inner receiver is the grunt logic (thunk
                                // 0x3a1c -> CGrunt::ResolveDeathAnimation @0x455f0);
                                // AnimWorkerObj::m_logic holds the bound grunt logic leaf
                                CGrunt* _d = (CGrunt*)_dr->m_7c->m_logic;
                                if (_d) {
                                    _d->ResolveDeathAnimation();
                                }
                            }
                        }
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Global thermal nuclear war!");
                        return 1;
                    }
                    case kCheatKillTimer: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        CTimer* _t = _g->m_frameMarker;
                        _t->m_40 = 0;
                        _t->m_44 = 0;
                        _t->m_accumLo = 0;
                        _t->m_accumHi = 0;
                        _t->m_running = 0;
                        _t->m_currentMs = 0;
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Ah, who needed that stupid timer anyway?");
                        return 1;
                    }
                    case kCheatGiveBombBrick:
                        ITEMCHEAT(0x26, "Bomb Brickz are cool!");
                    case kCheatGiveIndestructibleBrick:
                        ITEMCHEAT(0x25, "Indestructible Brickz are cool!");
                    case kCheatGiveGauntletBreakerBrick:
                        ITEMCHEAT(0x23, "Gauntlet-Breaker Brickz are cool!");
                    case kCheatGiveTeleportBrick:
                        ITEMCHEAT(0x24, "Teleport Brickz are cool!");
                    // ---- "pickup brick" cheats: grid-select a cell, LoadPickup(id) ----
                    case kCheatBrickAssimilate:
                        BRICKPICKUP(0x39, "Oh yes, they will be assimilated!");
                    case kCheatBrickDeath:
                        BRICKPICKUP(
                            0x3a,
                            "Ladies and gentlemen, please welcome... death... "
                            "He'll be here all week."
                        );
                    case kCheatBrickSuperGrunt:
                        BRICKPICKUP(0x38, "Super Grunt to the rescue!");
                    case kCheatBrickHurt:
                        BRICKPICKUP(0x3c, "This is gonna hurt them more than it will hurt you.");
                    case kCheatBrickSwallow:
                        BRICKPICKUP(0x3b, "How did you swallow that?");
                    case kCheatBrickNoRunning:
                        BRICKPICKUP(0x37, "There is no running allowed by the pool!");
                    case kCheatColorGruntz:
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_cmdGrid->CycleMoveIcons(-1, 1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("How about a little color in your Gruntz?");
                        return 1;
                    case kCheatRegionMonitor: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion3(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Whoah... you should get this monitor fixed.");
                        return 1;
                    }
                    case kCheatRegionDark: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion1(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Is is dark in here?");
                        return 1;
                    }
                    case kCheatRegionWindow: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion2(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Awww... isn't this little window cute?");
                        return 1;
                    }
                    case kCheatAbilityFreeze:
                        BRICKABILITY(1, "Freeze spellz are coooooooooooooooooool!");
                    case kCheatAbilityHeal:
                        BRICKABILITY(2, "For only $9.95, you too can have the healing power!");
                    case kCheatAbilityZombie:
                        BRICKABILITY(3, "Aaahh!  Zombiez!");
                    case kCheatAbilityParty:
                        BRICKABILITY(4, "It's party time!");
                    case kCheatAbilityTeleport:
                        BRICKABILITY(5, "Oh where oh where did the teleported Gruntz go?");
                    case kCheatAbilityRoll:
                        BRICKABILITY(6, "Rollin, rollin, rollin.");
                    case kCheatDebugFlag400:
                        g_debugDisplayFlags ^= 0x400;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case kCheatWawa:
                        if (m_world->m_soundRegistry->m_emitGate == 0) {
                            LeafCue* _c =
                                (LeafCue*)(static_cast<CDDrawSubMgrLeafScan*>(m_world->m_soundRegistry))
                                    ->Lookup_05b7e0("GAME_WAWA");
                            if (_c) {
                                _c->PlayIfElapsed(0x64, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("WA WA WA WA WA WA!");
                        return 1;
                    case kCheatKevinLambert:
                    case kCheatKevinLambertAlt:
                    case kCheatKevinLambertAlt2: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        // the guts/UI subsystem's concrete methods are CStatusBarMgr's (@0x10bc30) -
                        // retype CPlay::m_guts pending the Play.cpp reconciliation
                        ((CStatusBarMgr*)_g->m_guts)->UpdateDestructButton(0x1387);
                        AppendChatMessage(
                            "My name is Kevin Lambert.  You typed in my cheat "
                            "code.  Prepare to die."
                        );
                        return 1;
                    }
                    // ---- 4th sub-switch: warp / toggle cheats 0x81a3.. ----
                    case kCheatGooPuddlez:
                        g_gooPuddlez ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Goo puddlez", g_gooPuddlez);
                        return 1;
                    case kCheatFillGoo: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        if (!_g->m_guts) {
                            return 0;
                        }
                        ((CStatusBarMgr*)_g->m_guts)->AdvanceGauge(0x64); // +100 goo
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("May your Wellz be full of Goo!");
                        return 1;
                    }
                    case kCheatGruntCreation:
                        g_gruntCreation ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt creation", g_gruntCreation);
                        return 1;
                    case kCheatGruntDestruction:
                        g_gruntDestruction ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt destruction", g_gruntDestruction);
                        return 1;
                    case kCheatCheatelson:
                        PLAYCUE("GAME_MAJORCHEAT");
                        if (m_saveSink) {
                            m_saveSink->SetCurLevel(0x20);
                            m_saveSink->SetMagic(); // 0xe56b0 (the view's "Set")
                        }
                        AppendChatMessage(
                            "They should call you Cheat Cheatelson from "
                            "Cheatstown Virginia who lives at 1105 Cheat Circle "
                            "just behind the CheatMart superstore."
                        );
                        return 1;
                    case kCheatPsyche:
                        RunModalDialog(
                            "PSYCHE",
                            (void*)0x402649,
                            0
                        ); // bare imm matches the target (LAB_, no reloc/symbol)
                        return 1;
                    case kCheatClearCheats:
                        PLAYCUE("GAME_MAJORCHEAT");
                        m_cheatMgr->m_124 = 0;
                        AppendChatMessage("Cheatz cleared");
                        return 1;
                    case kCheatWarpTropicz:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Trouble in the Tropicz activated!");
                        m_saveSink->SetCurLevel(8);
                        return 1;
                    case kCheatWarpSweetz:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High on Sweetz activated!");
                        m_saveSink->SetCurLevel(0xc);
                        return 1;
                    case kCheatWarpRollerz:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High Rollerz activated!");
                        m_saveSink->SetCurLevel(0x10);
                        return 1;
                    case kCheatWarpHoneyShrunk:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Honey, I Shrunk the Gruntz activated!");
                        m_saveSink->SetCurLevel(0x14);
                        return 1;
                    case kCheatWarpMiniatureMasterz:
                        PLAYCUE_MAP("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to The Miniature Masterz activated!");
                        m_saveSink->SetCurLevel(0x18);
                        return 1;
                    case kCheatWarpGruntzInSpace:
                        PLAYCUE_MAP("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Gruntz in Space activated!");
                        m_saveSink->SetCurLevel(0x1c);
                        return 1;
                    case kCheatExplosionz: {
                        g_explosionz ^= 1;
                        if (m_world->m_soundRegistry->m_emitGate == 0) {
                            void* _c_ob = 0;
                            m_world->m_soundRegistry->m_10.Lookup("GAME_MAJORCHEAT", _c_ob);
                            LeafCue* _c = (LeafCue*)_c_ob;
                            if (_c && g_sndEnabled) {
                                i32 now = g_killCueClock;
                                if (static_cast<u32>((now - _c->m_14)) >= static_cast<u32>(_c->m_18)) {
                                    _c->m_14 = now;
                                    _c->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                                }
                            }
                        }
                        ShowToggleMessage("Explosionz", g_explosionz);
                        return 1;
                    }
                }
            }
            return 0;
        // ---- remaining UI command bodies (physically after the epilogue) ----
        case kCmdLoadSavedGame: {
            SaveInfo* si = m_saveInfoRec;
            if (!si) {
                return 1;
            }
            if (!(si->m_flags & 1)) {
                return 1;
            }
            m_114 = 1;
            CString tmp(si->m_levelName);
            m_strWorldFile = tmp;
            (void)notifyCode;
            if (tmp.GetLength()) {
                if (si->m_isWon) {
                    if (si->m_f8) {
                        m_128 = 0;
                        m_130 = 1;
                        m_134 = 3;
                    } else {
                        m_128 = 1;
                        m_130 = 0;
                        m_134 = 3;
                    }
                } else {
                    m_134 = 1;
                    m_130 = 1;
                }
            } else {
                m_134 = 1;
                m_130 = 0;
            }
            if (!PassClickToPlayState(si->m_levelId, 0, 1)) {
                ReportError(0x8005, 0x421);
            }
            if (!ParseSerial(reinterpret_cast<CGameRegistry*>(this), si->m_serial)) {
                ReportError(0x8005, 0x465);
            }
            CheckSavedMode();
            m_114 = 0;
            return 1;
        }
        case kCmdNoOp80b8:
            return 1;
        case kCmdMultiConnect:
            if (m_curState && m_curState->Update() == GAMESTATE_NONE) {
                static_cast<CMulti*>(m_curState)->Connect(lParam);
            }
            return 1;
        case kCmdLoadGameDialog:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_cdPromptResult) {
                    RunLoadGameDialog();
                }
            }
            return 1;
        case kCmdQuickSavePrompt:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                if (_g->CanQuickSave()) {
                    LoadSaveMessageSprite();
                }
            }
            return 1;
        case kCmdQuickSave:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                if (_g->CanQuickSave()) {
                    Quicksave();
                }
            }
            return 1;
        case kCmdQuickLoad:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_cdPromptResult) {
                    Quickload();
                }
            }
            return 1;
        case kCmdRestartLevel:
            RESTART(1);
        case kCmdRestartWorld:
            RESTART(2);
        case kCmdRestartWorldNoCursor:
            RESTART2(2);
        case kCmdRestartGame:
            RESTART(3);
        case kCmdWarpLevel1:
            WARP(1, 0x422);
        case kCmdDebugWarpLevel37:
            WARP(0x25, 0x45a);
        case kCmdDebugWarpLevel38:
            WARP(0x26, 0x45b);
        case kCmdDebugWarpLevel39:
            WARP(0x27, 0x45c);
        case kCmdDebugWarpLevel40:
            WARP(0x28, 0x45d);
        case kCmdDebugWarpLevel1:
            WARP(1, 0x45e);
        case kCmdDebugWarpLevel2:
            WARP(2, 0x45f);
        case kCmdDebugWarpLevel3:
            WARP(3, 0x460);
        case kCmdDebugWarpLevel4:
            WARP(4, 0x461);
        case kCmdDebugWarpLevel5:
            WARP(5, 0x462);
        case kCmdDebugWarpLevel6:
            WARP(6, 0x45f);
        case kCmdDebugWarpLevel7:
            WARP(7, 0x460);
        case kCmdDebugWarpLevel8:
            WARP(8, 0x461);
        case kCmdDebugWarpLevel9:
            WARP(9, 0x462);
        case kCmdDebugWarpLevel10:
            WARP(0xa, 0x463);
        case kCmdDebugWarpLevel11:
            WARP(0xb, 0x464);
        case kCmdDebugWarpLevel12:
            WARP(0xc, 0x465);
        case kCmdDebugWarpLevel13:
            WARP(0xd, 0x466);
        case kCmdDebugWarpLevel14:
            WARP(0xe, 0x467);
        case kCmdDebugWarpLevel15:
            WARP(0xf, 0x468);
        case kCmdDebugWarpLevel16:
            WARP(0x10, 0x469);
        case kCmdDebugWarpLevel17:
            WARP(0x11, 0x46a);
        case kCmdDebugWarpLevel18:
            WARP(0x12, 0x46b);
        case kCmdDebugWarpLevel19:
            WARP(0x13, 0x46c);
        case kCmdDebugWarpLevel20:
            WARP(0x14, 0x46d);
        case kCmdDebugWarpLevel21:
            WARP(0x15, 0x46e);
        case kCmdDebugWarpLevel22:
            WARP(0x16, 0x46f);
        case kCmdDebugWarpLevel23:
            WARP(0x17, 0x470);
        case kCmdDebugWarpLevel24:
            WARP(0x18, 0x471);
        case kCmdDebugWarpLevel25:
            WARP(0x19, 0x472);
        case kCmdDebugWarpLevel26:
            WARP(0x1a, 0x473);
        case kCmdDebugWarpLevel27:
            WARP(0x1b, 0x474);
        case kCmdDebugWarpLevel28:
            WARP(0x1c, 0x475);
        case kCmdDebugWarpLevel29:
            WARP(0x1d, 0x476);
        case kCmdDebugWarpLevel30:
            WARP(0x1e, 0x477);
        case kCmdDebugWarpLevel31:
            WARP(0x1f, 0x478);
        case kCmdDebugWarpLevel32:
            WARP(0x20, 0x479);
        case kCmdDebugWarpLevel101:
            WARP(0x65, 0x45e);
        case kCmdDebugWarpLevel102:
            WARP(0x66, 0x45f);
        case kCmdDebugWarpLevel103:
            WARP(0x67, 0x460);
        case kCmdDebugWarpLevel104:
            WARP(0x68, 0x461);
        case kCmdDebugWarpLevel105:
            WARP(0x69, 0x462);
        case kCmdDebugWarpLevel106:
            WARP(0x6a, 0x45f);
        case kCmdDebugWarpLevel107:
            WARP(0x6b, 0x460);
        case kCmdDebugWarpLevel108:
            WARP(0x6c, 0x461);
        case kCmdDebugWarpLevel109:
            WARP(0x6d, 0x462);
        case kCmdDebugWarpLevel110:
            WARP(0x6e, 0x463);
        case kCmdDebugWarpLevel111:
            WARP(0x6f, 0x464);
        case kCmdDebugWarpLevel112:
            WARP(0x70, 0x465);
        case kCmdDebugWarpLevel113:
            WARP(0x71, 0x466);
        case kCmdDebugWarpLevel114:
            WARP(0x72, 0x467);
        case kCmdDebugWarpLevel115:
            WARP(0x73, 0x468);
        case kCmdDebugWarpLevel116:
            WARP(0x74, 0x469);
        case kCmdDebugWarpLevel117:
            WARP(0x75, 0x46a);
        case kCmdDebugWarpLevel118:
            WARP(0x76, 0x46b);
        case kCmdDebugWarpLevel119:
            WARP(0x77, 0x46c);
        case kCmdDebugWarpLevel120:
            WARP(0x78, 0x46d);
        case kCmdDebugWarpLevel121:
            WARP(0x79, 0x46e);
        case kCmdDebugWarpLevel122:
            WARP(0x7a, 0x46f);
        case kCmdDebugWarpLevel123:
            WARP(0x7b, 0x470);
        case kCmdDebugWarpLevel124:
            WARP(0x7c, 0x471);
        case kCmdDebugWarpLevel125:
            WARP(0x7d, 0x472);
        case kCmdDebugWarpLevel126:
            WARP(0x7e, 0x473);
        case kCmdDebugWarpLevel127:
            WARP(0x7f, 0x474);
        case kCmdDebugWarpLevel128:
            WARP(0x80, 0x475);
        case kCmdDebugWarpLevel129:
            WARP(0x81, 0x476);
        case kCmdDebugWarpLevel130:
            WARP(0x82, 0x477);
        case kCmdDebugWarpLevel131:
            WARP(0x83, 0x478);
        case kCmdDebugWarpLevel132:
            WARP(0x84, 0x479);
        case kCmdWebSite:
            if (m_curState->Update() == GAMESTATE_MENU
                || m_curState->Update() == GAMESTATE_ATTRACT) {
                while (::ShowCursor(1) < 0) {
                }
                LaunchWebBrowser(const_cast<char*>("http://www.gruntzgoo.com/"));
            }
            return 1;
        case kCmdMultiJoin:
            m_134 = 2;
            g_isHost_648cf0 = 0;
            if (TransitionState(0x11, 1, 0, 0)) {
                return 1;
            }
            if (TransitionState(2, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x424);
            return 1;
        case kCmdMultiHost:
            m_134 = 2;
            g_isHost_648cf0 = 1;
            if (TransitionState(0x11, 1, 0, 0)) {
                return 1;
            }
            if (TransitionState(2, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x425);
            return 1;
        case kCmdMainMenu:
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x426);
            return 1;
        case kCmdShowCredits:
            if (TransitionState(0xb, 1, 1, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x427);
            return 1;
        case kCmdShowBooty:
            if (TransitionState(0xd, 1, 1, lParam)) {
                return 1;
            }
            ReportError(0x8005, 0x428);
            return 1;
        case kCmdNextState:
            if (SwitchToNextState()) {
                return 1;
            }
            ReportError(0x8005, 0x429);
            return 1;
        case kCmdShowHelp:
            if (TransitionState(8, 1, 0, 0)) {
                return 1;
            }
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42a);
            return 1;
        case kCmdAttract:
            if (TransitionState(2, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42b);
            return 1;
        case kCmdReturnToAttract:
            if (!TransitionState(2, 1, 0, 0)) {
                ReportError(0x8005, 0x42c);
                return 1;
            }
            ::PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
            return 1;
        case kCmdShowState0e:
            if (TransitionState(0xe, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42d);
            return 1;
        case kCmdShowState07:
            if (TransitionState(7, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42e);
            return 1;
        case kCmdPauseToggle: { // 0x89b97
            i32 st = m_curState->Update();
            if (st == GAMESTATE_PLAY || st == GAMESTATE_NONE) {
                CPlay* ps = static_cast<CPlay*>(m_curState); // id-proven downcast (CMulti : CPlay)
                if (ps->m_inGame) {
                    return 1;
                }
                if (ps->m_renderDisabled) {
                    return 1;
                }
                if (ps->m_guts) {
                    if (ps->m_guts->m_toggleActive) {
                        return 1;
                    }
                    if (ps->m_guts->m_toggleHandle) {
                        return 1;
                    }
                }
                i32 f = m_frameGate ^ 1;
                m_frameGate = f;
                FinishLevel(f, 1);
            }
            return 1;
        }
        case kCmdFinishLevel: { // 0x89c19
            i32 st = m_curState->Update();
            if (st == GAMESTATE_PLAY || st == GAMESTATE_NONE) {
                i32 f = m_frameGate ^ 1;
                m_frameGate = f;
                FinishLevel(f, 0);
            }
            return 1;
        }
        case kCmdPresentWorld:
            if (!CheckPlayState()) {
                return 1;
            }
            if (static_cast<CPlay*>(m_curState)->DrawWorldPresent()) {
                return 1;
            }
            ReportError(0x8005, 0x42f);
            return 1;
        case kCmdLobbyReset:
            m_lobbyProbed = 0;
            ::PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8025, 0);
            return 1;
        case kCmdExitToAttract: // 0x89c92
            if (!CheckPlayState()) {
                return 1;
            }
            if (m_curState->Vslot15()) {
                return 1;
            }
            if (TransitionState(2, 1, 0, 0)) {
                ::PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
                return 1;
            }
            ReportError(0x8005, 0x430);
            return 1;
        case kCmdCaptureWorld: // 0x89d00
            if (g_cdPromptResult) {
                return 1;
            }
            CaptureWorldFile();
            return 1;
        case kCmdNextLevel: // 0x89d1e
            if (GoToNextLevel()) {
                return 1;
            }
            ReportError(0x8007, 0x431);
            return 1;
        case kCmdPrevLevel: // 0x89d37 -> falls into 0x8006
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_NONE) {
                GoToPrevLevel();
                return 1;
            }
            // fall through
        case kCmdReturnToMenu: // 0x89d62
            m_curState->m_40 = 1;
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x432);
            return 1;
        case kCmdQuit: // 0x89d8d
            DelayedQuit();
            return 1;
        case kCmdShowBootyState: { // 0x89d9e
            i32 st = m_curState->Update();
            if (st == 9 || st == 0xd || st == 0xf || st == 0xe || st == GAMESTATE_CREDITS
                || st == GAMESTATE_BOOTY || st == GAMESTATE_MULTIBOOTY || st == GAMESTATE_NONE) {
                return 1;
            }
            if (TransitionState(9, 1, 1, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x433);
            return 1;
        }
        case kCmdConfigSettings: { // 0x89e58  CONFIG_SETTINGS modal
            i32 st = m_curState->Update();
            CMenuState* mus = 0;
            if (st == GAMESTATE_MENU) {
                mus = static_cast<CMenuState*>(m_curState);
                (static_cast<CMenuState*>(m_curState))->StopMusicChain();
            }
            RunModalDialog(
                "CONFIG_SETTINGS",
                (void*)0x403ae4,
                0
            ); // bare imm matches the target (LAB_)
            if (mus) {
                mus->StartMusic();
            }
            return 1;
        }
        case kCmdToggleMusic: { // 0x89e9f  elapsed-time / sound toggle
            if (m_frameGate) {
                return 1;
            }
            i32 v = m_musicEnabled ^ 1;
            m_musicEnabled = v;
            i32 pl = CheckPlayState();
            if (!pl) {
                i32 st = m_curState->Update();
                if (st != 0xb && st != GAMESTATE_MENU) {
                    return 1;
                }
            }
            if (v) {
                m_sound->Restart(1);
            } else {
                m_sound->StopAll();
            }
            return 1;
        }
        case kCmdToggleSound: { // 0x89f08  world-position display toggle
            if (m_world) {
                SoundStream* p = m_world->m_soundRegistry->m_2c;
                if (p) {
                    p->Stop();
                }
            }
            i32 v = m_soundEnabled ^ 1;
            m_soundEnabled = v;
            g_sndEnabled = v;
            if (v == 0) {
                m_inputState->Stop(); // 0x29b9->0xbc80 CWorldSoundSet::Stop
            } else {
                m_inputState->Resume(); // 0x18e8->0xbcf0 CWorldSoundSet::Resume
            }
            return 1;
        }
        case kCmdRestoreVideoMode: // 0x89f5a
            if (!IsInPlayState()) {
                return 1;
            }
            RestoreVideoMode(0);
            return 1;
        case kCmdCheckDisplayBoundsA: // 0x89f7c
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsA();
            return 1;
        case kCmdCheckDisplayBoundsB: // 0x89f9c
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsB();
            return 1;
        case kCmdScreenshot: { // 0x89fbc  screenshot (front surface -> SaveScreenshot)
            Fwd114ec0(
                m_settings,
                this,
                ((CGameRegistry*)g_gameReg)->m_modeW,
                ((CGameRegistry*)g_gameReg)->m_modeH,
                0,
                0
            );
            return 1;
        }
        case kCmdReloadLevel: {
            CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
            if (!_g) {
                return 1;
            }
            m_strWorldFile = m_strWorldFile; // 0x1b9e25 op=
            if (!PassClickToPlayState(m_curState->m_levelIndex, 0, 1)) {
                ReportError(0x8007, 0x434);
            }
            return 1;
        }
    }
    return 0;
}
#undef PLAYCUE
#undef PLAYCUE_MAP
#undef ITEMCHEAT
#undef WARP
#undef BRICKPICKUP
#undef BRICKABILITY
#undef RESTART
#undef RESTART2

// (Fwd114ec0 @0x114ec0 + Fwd114f00 @0x114f00 carved out to src/Gruntz/Fwd114ec0.cpp
// in REHOME D9 - see the forward-declaration note above.)
