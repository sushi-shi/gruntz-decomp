#include <rva.h>

#include <Gruntz/GruntzMgrCmd.h>

#include <Mfc.h>

#include <Dsndmgr/MidiManager.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameText.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StartUpPrompt.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/VideoConfig.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/WorldSoundSet.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <string.h>

#define PLAYCUE(TAG)                                                                               \
    if (m_world->m_soundRegistry->m_silentMode == 0) {                                             \
        SoundCue* _c = static_cast<SoundCue*>(m_world->m_soundRegistry->Lookup(TAG));              \
        if (_c)                                                                                    \
            _c->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                                      \
    }
#define PLAYCUE_MAP(TAG, VAR)                                                                      \
    {                                                                                              \
        SoundCueRegistry* _reg = m_world->m_soundRegistry;                                         \
        if (_reg->m_silentMode == 0) {                                                             \
            VAR = 0;                                                                               \
            MapLookup(_reg->m_cues, TAG, VAR);                                                     \
            if (VAR)                                                                               \
                VAR->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                                 \
        }                                                                                          \
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
        m_gameMode = GAMEMODE_QUESTZ;                                                              \
        m_strWorldFile.Empty();                                                                    \
        if (!PassClickToPlayState((N), 0, 1))                                                      \
            ReportError(IDX(IDS_SET_GAME_STATE), (ERR));                                           \
        return 1;                                                                                  \
    }
#define BRICKPICKUP(ID, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell =                                                                            \
            m_triggerMgr->m_recList.GetCount() != 1                                                \
                ? 0                                                                                \
                : m_triggerMgr                                                                     \
                      ->m_units[m_triggerMgr->HeadRec()->m_y + m_triggerMgr->HeadRec()->m_x * 15]; \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_playerIndex != g_curPlayer)                                                   \
            return 0;                                                                              \
        CGrunt* _c2 = m_triggerMgr->m_units[_cell->m_unitIndex + _cell->m_playerIndex * 15];       \
        i32 _r = (_c2 && _c2->m_entranceCommitted) ? _c2->LoadPickupSprites(ID, 0, 0, 0, 1) : 0;   \
        if (!_r)                                                                                   \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }
#define BRICKABILITY(N, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell =                                                                            \
            m_triggerMgr->m_recList.GetCount() != 1                                                \
                ? 0                                                                                \
                : m_triggerMgr                                                                     \
                      ->m_units[m_triggerMgr->HeadRec()->m_y + m_triggerMgr->HeadRec()->m_x * 15]; \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_playerIndex != g_curPlayer)                                                   \
            return 0;                                                                              \
        if (!_cell->LoadGruntAbilityTuning(N))                                                     \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }
#define RESTART(N)                                                                                 \
    {                                                                                              \
        CMenuState* mus = 0;                                                                       \
        GameStateId st = m_curState->Update();                                                     \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                            \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                              \
            while (ShowCursor(0) >= 0) {                                                           \
            }                                                                                      \
        }                                                                                          \
        PlayMovieEntry(N);                                                                         \
        if (mus) {                                                                                 \
            mus->StartMusic();                                                                     \
            while (ShowCursor(1) < 0) {                                                            \
            }                                                                                      \
        }                                                                                          \
        return 1;                                                                                  \
    }
#define RESTART2(N)                                                                                \
    {                                                                                              \
        CMenuState* mus = 0;                                                                       \
        GameStateId st = m_curState->Update();                                                     \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                            \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                              \
        }                                                                                          \
        PlayMovieEntry(N);                                                                         \
        if (mus)                                                                                   \
            mus->StartMusic();                                                                     \
        return 1;                                                                                  \
    }

// @early-stop
// The frame matches retail exactly (`sub esp,0x94`) and so does every parameter
// displacement, and the single `ret` is retail's. What is left is dominated by
// ONE cl decision we have not been able to steer: which duplicate blocks get
// cross-jumped. Retail folds far more `return 1;`
// sites into a shared `mov eax,1` block than we do (110 `mov eax,1` in retail vs 122
// here), so at a dozen guards retail emits `je <shared ret-1>` where cl emits
// `jne <continue> / mov eax,1 / jmp <epilogue>` inline - and it goes the OTHER way for
// `ReportError`, which retail leaves inline at two sites (6 call relocs vs our 4).
// The branch polarity that follows from it is also why retail can `push eax` for a
// known-zero argument where we must `push 0`.
// Smaller residue, each re-derived from the current build:
//   * `(g_debugDisplayFlags ^ DEBUG_DISPLAY_TIMING_ALTERNATE)
//     & ~DEBUG_DISPLAY_TIMING` at CHEAT_BRICK_TEXT_ALT_DISPLAY is
//     emitted `and`-then-`xor`: cl sorts the two disjoint bit operations by constant
//     magnitude (the sibling arm's timing/alternate-timing expression is already in
//     that order and matches). Splitting it into two statements on the global is
//     byte-identical.
//   * CMD_SCREENSHOT: retail loads `m_modeSize.cx` TWICE (once for the push, once for
//     the temp's home slot) where cl reuses the one register for both.
//   * three of the thirteen `_cell` grid-index sites (BRICKPICKUP(0x39) and the last
//     two BRICKABILITY arms) use retail's `add edx,edi` instead of `add ecx,edx`. It is
//     not the source order - swapping the addends flips all thirteen and costs 0.04 -
//     and it is what makes the diff misalign eight near-identical macro expansions.
// Note that the last two diff hunks are NOT code: they are the jump table, the byte
// index table and the pooled string literals disassembled as instructions.
// Measured and REJECTED, so nobody re-runs them: rewriting the CHEAT_MONOLITH early
// returns into nested `if`s (`if (X) { rest } return 1;`) takes the branch count to
// 315 vs retail's 313 - a structural difference, so the early-return spelling is the
// right one and the four polarity flips are layout, not shape. Compiling the unit with
// /G5, /G4, /G3 or /GB is byte-identical (the processor flag does not reach this
// function's scheduling at all), /Ob2 is byte-identical to the /O2 default, and /Oa,
// /Ow and /Ox are all worse.

RVA(0x000862f0, 0x4369)
i32 CGruntzMgr::HandleCommand(i32 notifyCode, GruntzCommandId nID, i32 lParam) {
    switch (nID) {
        case CMD_NEW_GAME:
        case CMD_NEW_GAME_ALT:
            m_gameMode = GAMEMODE_QUESTZ;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x41e);
            }
            return 1;
        case CMD_LOAD_WORLD:
            m_gameMode = GAMEMODE_QUESTZ;
            m_strWorldFile.Empty();
            if (!PassClickToPlayState(lParam, 0, 1)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x41f);
            }
            return 1;
        case CMD_CONTINUE_AT_MAX_LEVEL:
            m_gameMode = GAMEMODE_QUESTZ;
            m_strWorldFile.Empty();
            if (!PassClickToPlayState(IDX(m_saveGame->m_maxLevel), 0, 1)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x41f);
            }
            return 1;
        case CMD_START_BATTLEZ_GAME:
            m_gameMode = GAMEMODE_BATTLEZ;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x420);
            }
            return 1;
        case CMD_OPEN_BATTLEZ_SETUP:
            OpenBattlezSetup();
            // fall through to default
        default:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                // The four `Lookup`/`MapLookup` out-parameters below live HERE, at the
                // scope that encloses the cheat switch, not inside their own arms:
                // retail gives each of them its own dword slot, and cl packs locals
                // declared in disjoint nested scopes onto the same slot. Together with
                // the tagSIZE temp in CMD_SCREENSHOT they are the 0x10 bytes that make
                // the frame `sub esp,0x94` instead of `0x84`.
                CGameObject* _dr;
                SoundCue* _cueMiniature;
                SoundCue* _cueSpace;
                SoundCue* _c;
                switch (static_cast<GruntzCommandId>(IDX(nID) & 0xffff)) {
                    case CHEAT_PROGRAMMING_GOD: {
                        if (m_world->m_soundRegistry->m_silentMode == 0) {
                            SoundCue* _c = static_cast<SoundCue*>(
                                (static_cast<SoundCueRegistry*>(m_world->m_soundRegistry))
                                    ->Lookup("GAME_MINORCHEAT")
                            );
                            if (_c) {
                                _c->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("Brian L. Goble is a programming God...");
                        return 1;
                    }
                    case CHEAT_TRAITOR_MODE:
                        g_traitorMode ^= 1;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Traitor Mode", g_traitorMode);
                        return 1;
                    case CHEAT_OBJECT_COUNT_DISPLAY:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_OBJECT_COUNT;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage(
                            "Object Count Display",
                            HAS(g_debugDisplayFlags, DEBUG_DISPLAY_OBJECT_COUNT)
                        );
                        return 1;
                    case CHEAT_WORLD_POSITION_DISPLAY:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_WORLD_POSITION;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage(
                            "World Position Display",
                            HAS(g_debugDisplayFlags, DEBUG_DISPLAY_WORLD_POSITION)
                        );
                        return 1;
                    case CHEAT_FRAME_RATE_DISPLAY:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_FRAME_RATE;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage(
                            "Frame Rate Display",
                            HAS(g_debugDisplayFlags, DEBUG_DISPLAY_FRAME_RATE)
                        );
                        return 1;
                    case CHEAT_DEBUG_FLAG20:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_SUPPRESS;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case CHEAT_BRICK_TEXT_DISPLAY:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ DEBUG_DISPLAY_TIMING)
                                              & ~DEBUG_DISPLAY_TIMING_ALTERNATE;
                        g_brickText1.Empty();
                        g_brickText2.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case CHEAT_BRICK_TEXT_ALT_DISPLAY:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ DEBUG_DISPLAY_TIMING_ALTERNATE)
                                              & ~DEBUG_DISPLAY_TIMING;
                        g_brickText1.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case CHEAT_ELAPSED_TIME_DISPLAY:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_ELAPSED_TIME;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage(
                            "Elapsed Time Display",
                            HAS(g_debugDisplayFlags, DEBUG_DISPLAY_ELAPSED_TIME)
                        );
                        return 1;
                    case CHEAT_MONOLITH: {
                        CPlay* playState = PickPlayOrPausedState();
                        if (!playState) {
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
                            m_midi->PlaySequence("MONOLITH", 1);
                            return 1;
                        }
                        char sequenceName[128];
                        wsprintfA(sequenceName, "AMBIENT%d", playState->GetAmbientId());
                        m_midi->PlaySequence(sequenceName, 1);
                        return 1;
                    }
                    case CHEAT_NO_OP:
                        return 1;
                    case CHEAT_BRICK_GO_AWAY:
                        BRICKPICKUP(PICKUP_GHOST, "Hey, where did you go?");

                    case CHEAT_GIVE_BOMB:
                        ITEMCHEAT(1, "Bombz are cool!");
                    case CHEAT_GIVE_BOOMERANG:
                        ITEMCHEAT(2, "Boomerangz are cool!");
                    case CHEAT_GIVE_BRICK_LAYER:
                        ITEMCHEAT(3, "Brick Laying Toolz are cool!");
                    case CHEAT_GIVE_CLUB:
                        ITEMCHEAT(4, "Clubz are cool!");
                    case CHEAT_GIVE_GAUNTLET:
                        ITEMCHEAT(5, "Gauntletz are cool!");
                    case CHEAT_GIVE_GLOVE:
                        ITEMCHEAT(6, "Glovez are cool!");
                    case CHEAT_GIVE_GOOBER:
                        ITEMCHEAT(7, "Gooberz are cool!");
                    case CHEAT_GIVE_GRAVITY_BOOT:
                        ITEMCHEAT(8, "Gravity Bootz are cool!");
                    case CHEAT_GIVE_GUN_HAT:
                        ITEMCHEAT(9, "Gun Hatz are cool!");
                    case CHEAT_GIVE_SPONGE_GUN:
                        ITEMCHEAT(0xa, "Sponge Gunz are cool!");
                    case CHEAT_GIVE_ROCK:
                        ITEMCHEAT(0xb, "Rockz are cool!");
                    case CHEAT_GIVE_SHIELD:
                        ITEMCHEAT(0xc, "Shieldz are cool!");
                    case CHEAT_GIVE_SHOVEL:
                        ITEMCHEAT(0xd, "Shovelz are cool!");
                    case CHEAT_GIVE_SPRING:
                        ITEMCHEAT(0xe, "Springz are cool!");
                    case CHEAT_GIVE_SPY_GEAR:
                        ITEMCHEAT(0xf, "Spy Gear is cool!");
                    case CHEAT_GIVE_SWORD:
                        ITEMCHEAT(0x10, "Swordz are cool!");
                    case CHEAT_GIVE_TIME_BOMB:
                        ITEMCHEAT(0x11, "Time Bombz are cool!");
                    case CHEAT_GIVE_TOOB:
                        ITEMCHEAT(0x12, "Toobz are cool!");
                    case CHEAT_GIVE_MAGIC_WAND:
                        ITEMCHEAT(0x13, "Magic Wandz are cool!");
                    // 0x14 == PICKUP_WARPSTONE. The private CHEATZ.TXT names this
                    // code MPWARPSTONEZ, and the item list is alphabetical, so 0x14
                    // sits exactly between Wandz (0x13) and Welder's Kitz (0x15).
                    // Retail's message is the developers acknowledging that the code
                    // for it was withheld from the shipped ATTRIBUTEZ cheat table.
                    case CHEAT_GIVE_WARPSTONE:
                        ITEMCHEAT(0x14, "Hey, how did you get this cheat?");
                    case CHEAT_GIVE_WELDERS_KIT:
                        ITEMCHEAT(0x15, "Welder's Kitz are cool!");
                    case CHEAT_GIVE_WING:
                        ITEMCHEAT(0x16, "Wingz are cool!");
                    case CHEAT_GIVE_BABY_WALKER:
                        ITEMCHEAT(0x17, "Baby-Walkerz are cool!");
                    case CHEAT_GIVE_BEACH_BALL:
                        ITEMCHEAT(0x18, "Beach Ballz are cool!");
                    case CHEAT_GIVE_MONSTER_WHEEL:
                        ITEMCHEAT(0x19, "Monster Wheelz are cool!");
                    case CHEAT_GIVE_GO_KART:
                        ITEMCHEAT(0x1a, "Go-Kartz are cool!");
                    case CHEAT_GIVE_JACK_IN_THE_BOX:
                        ITEMCHEAT(0x1b, "Jack-In-The-Boxez are cool!");
                    case CHEAT_GIVE_JUMP_ROPE:
                        ITEMCHEAT(0x1c, "Jump Ropez are cool!");
                    case CHEAT_GIVE_POGO_STICK:
                        ITEMCHEAT(0x1d, "Pogo Stickz are cool!");
                    case CHEAT_GIVE_SCROLL:
                        ITEMCHEAT(0x1e, "Scrollz are cool!");
                    case CHEAT_GIVE_SQUEAK_TOY:
                        ITEMCHEAT(0x1f, "Squeak Toyz are cool!");
                    case CHEAT_GIVE_YO_YO:
                        ITEMCHEAT(0x20, "Yo-Yoz are cool!");
                    case CHEAT_NUKE: {
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_triggerMgr->StartPlayerDefeatSequence(5);
                        i32 _key = g_gameReg->m_players[0].m_warlordObjectId;
                        if (_key) {
                            _dr = NULL;
                            if (MapLookupById(
                                    g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                    _key,
                                    _dr
                                )
                                && _dr) {
                                CWarlord* _d =
                                    static_cast<CWarlord*>(_dr->m_logicRecord->m_userLogic);
                                if (_d) {
                                    _d->ResolveDeathAnimation();
                                }
                            }
                        }
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Global thermal nuclear war!");
                        return 1;
                    }
                    case CHEAT_KILL_TIMER: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }
                        CTimer* _t = _g->m_levelTimer;
                        _t->m_unusedStamp.m_lo = 0;
                        _t->m_unusedStamp.m_hi = 0;
                        _t->m_accum.m_lo = 0;
                        _t->m_accum.m_hi = 0;
                        _t->m_running = 0;
                        _t->m_currentMs = 0;
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Ah, who needed that stupid timer anyway?");
                        return 1;
                    }
                    case CHEAT_GIVE_BOMB_BRICK:
                        ITEMCHEAT(0x26, "Bomb Brickz are cool!");
                    case CHEAT_GIVE_INDESTRUCTIBLE_BRICK:
                        ITEMCHEAT(0x25, "Indestructible Brickz are cool!");
                    case CHEAT_GIVE_GAUNTLET_BREAKER_BRICK:
                        ITEMCHEAT(0x23, "Gauntlet-Breaker Brickz are cool!");
                    case CHEAT_GIVE_TELEPORT_BRICK:
                        ITEMCHEAT(0x24, "Teleport Brickz are cool!");

                    case CHEAT_BRICK_ASSIMILATE:
                        BRICKPICKUP(PICKUP_CONVERSION, "Oh yes, they will be assimilated!");
                    case CHEAT_BRICK_DEATH:
                        BRICKPICKUP(
                            PICKUP_DEATHTOUCH,
                            "Ladies and gentlemen, please welcome... death... "
                            "He'll be here all week."
                        );
                    case CHEAT_BRICK_SUPER_GRUNT:
                        BRICKPICKUP(PICKUP_INVULNERABILITY, "Super Grunt to the rescue!");
                    case CHEAT_BRICK_HURT:
                        BRICKPICKUP(
                            PICKUP_REACTIVEARMOR,
                            "This is gonna hurt them more than it will hurt you."
                        );
                    case CHEAT_BRICK_SWALLOW:
                        BRICKPICKUP(PICKUP_ROIDZ, "How did you swallow that?");
                    case CHEAT_BRICK_NO_RUNNING:
                        BRICKPICKUP(PICKUP_SUPERSPEED, "There is no running allowed by the pool!");
                    case CHEAT_COLOR_GRUNTZ:
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_triggerMgr->CycleMoveIcons(-1, 1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("How about a little color in your Gruntz?");
                        return 1;
                    case CHEAT_REGION_MONITOR: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }
                        _g->SetMonitorCurse(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Whoah... you should get this monitor fixed.");
                        return 1;
                    }
                    case CHEAT_REGION_DARK: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }
                        _g->SetDarknessCurse(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Is is dark in here?");
                        return 1;
                    }
                    case CHEAT_REGION_WINDOW: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }
                        _g->SetTinyViewportCurse(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Awww... isn't this little window cute?");
                        return 1;
                    }
                    case CHEAT_ABILITY_FREEZE:
                        BRICKABILITY(1, "Freeze spellz are coooooooooooooooooool!");
                    case CHEAT_ABILITY_HEAL:
                        BRICKABILITY(2, "For only $9.95, you too can have the healing power!");
                    case CHEAT_ABILITY_ZOMBIE:
                        BRICKABILITY(3, "Aaahh!  Zombiez!");
                    case CHEAT_ABILITY_PARTY:
                        BRICKABILITY(4, "It's party time!");
                    case CHEAT_ABILITY_TELEPORT:
                        BRICKABILITY(5, "Oh where oh where did the teleported Gruntz go?");
                    case CHEAT_ABILITY_ROLL:
                        BRICKABILITY(6, "Rollin, rollin, rollin.");
                    case CHEAT_DEBUG_FLAG400:
                        g_debugDisplayFlags ^= DEBUG_DISPLAY_BUILD_INFO;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case CHEAT_WAWA:
                        if (m_world->m_soundRegistry->m_silentMode == 0) {
                            SoundCue* _c = static_cast<SoundCue*>(
                                (static_cast<SoundCueRegistry*>(m_world->m_soundRegistry))
                                    ->Lookup("GAME_WAWA")
                            );
                            if (_c) {
                                _c->PlayIfElapsed(0x64, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("WA WA WA WA WA WA!");
                        return 1;
                    case CHEAT_KEVIN_LAMBERT:
                    case CHEAT_KEVIN_LAMBERT_ALT:
                    case CHEAT_KEVIN_LAMBERT_ALT2: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }

                        (static_cast<CStatusBarMgr*>(_g->m_statusBar))
                            ->StartDestructWarning(0x1387);
                        AppendChatMessage(
                            "My name is Kevin Lambert.  You typed in my cheat "
                            "code.  Prepare to die."
                        );
                        return 1;
                    }

                    case CHEAT_GOO_PUDDLEZ:
                        g_gooPuddlez ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Goo puddlez", g_gooPuddlez);
                        return 1;
                    case CHEAT_FILL_GOO: {
                        CPlay* _g = PickPlayOrPausedState();
                        if (!_g) {
                            return 0;
                        }
                        if (!_g->m_statusBar) {
                            return 0;
                        }
                        (static_cast<CStatusBarMgr*>(_g->m_statusBar))->AdvanceGauge(0x64);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("May your Wellz be full of Goo!");
                        return 1;
                    }
                    case CHEAT_GRUNT_CREATION:
                        g_gruntCreation ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt creation", g_gruntCreation);
                        return 1;
                    case CHEAT_GRUNT_DESTRUCTION:
                        g_gruntDestruction ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt destruction", g_gruntDestruction);
                        return 1;
                    case CHEAT_CHEATELSON:
                        PLAYCUE("GAME_MAJORCHEAT");
                        if (m_saveGame) {
                            m_saveGame->SetCurLevel(QUESTLEVEL_CAMPAIGN_LAST);
                            m_saveGame->SetMagic();
                        }
                        AppendChatMessage(
                            "They should call you Cheat Cheatelson from "
                            "Cheatstown Virginia who lives at 1105 Cheat Circle "
                            "just behind the CheatMart superstore."
                        );
                        return 1;
                    case CHEAT_PSYCHE:

                        RunModalDialog("PSYCHE", PsycheDialogProc, 0);
                        return 1;
                    case CHEAT_CLEAR_CHEATS:
                        PLAYCUE("GAME_MAJORCHEAT");
                        m_cheatMgr->m_cheatsUsed = 0;
                        AppendChatMessage("Cheatz cleared");
                        return 1;
                    case CHEAT_WARP_TROPICZ:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Trouble in the Tropicz activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA2_STAGE4);
                        return 1;
                    case CHEAT_WARP_SWEETZ:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High on Sweetz activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA3_STAGE4);
                        return 1;
                    case CHEAT_WARP_ROLLERZ:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High Rollerz activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA4_STAGE4);
                        return 1;
                    case CHEAT_WARP_HONEY_SHRUNK:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Honey, I Shrunk the Gruntz activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA5_STAGE4);
                        return 1;
                    case CHEAT_WARP_MINIATURE_MASTERZ:
                        PLAYCUE_MAP("GAME_MINORCHEAT", _cueMiniature);
                        AppendChatMessage("Warp to The Miniature Masterz activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA6_STAGE4);
                        return 1;
                    case CHEAT_WARP_GRUNTZ_IN_SPACE:
                        PLAYCUE_MAP("GAME_MINORCHEAT", _cueSpace);
                        AppendChatMessage("Warp to Gruntz in Space activated!");
                        m_saveGame->SetCurLevel(QUESTLEVEL_AREA7_STAGE4);
                        return 1;
                    case CHEAT_EXPLOSIONZ: {
                        g_explosionz ^= 1;
                        SoundCueRegistry* _reg = m_world->m_soundRegistry;
                        if (_reg->m_silentMode == 0) {
                            _c = NULL;
                            MapLookup(_reg->m_cues, "GAME_MAJORCHEAT", _c);
                            if (_c) {
                                PlaySoundCueIfElapsed(_c, g_soundVolumePercent, 0, 0, 0);
                            }
                        }
                        ShowToggleMessage("Explosionz", g_explosionz);
                        return 1;
                    }
                }
            }
            return 0;

        case CMD_LOAD_SAVED_GAME: {
            SaveSlot* si = m_saveInfoRec;
            if (!si) {
                return 1;
            }
            if (!(si->m_flags & 1)) {
                return 1;
            }
            m_loadingSaveGame = 1;
            CString tmp(si->m_levelName);
            m_strWorldFile = tmp;
            static_cast<void>(notifyCode);
            if (tmp.GetLength()) {
                if (si->m_isBattlez) {
                    if (si->m_isCustom) {
                        m_isBuiltInBattlezLevel = 0;
                        m_isCustomLevel = 1;
                        m_gameMode = GAMEMODE_BATTLEZ;
                    } else {
                        m_isBuiltInBattlezLevel = 1;
                        m_isCustomLevel = 0;
                        m_gameMode = GAMEMODE_BATTLEZ;
                    }
                } else {
                    m_gameMode = GAMEMODE_QUESTZ;
                    m_isCustomLevel = 1;
                }
            } else {
                m_gameMode = GAMEMODE_QUESTZ;
                m_isCustomLevel = 0;
            }
            if (!PassClickToPlayState(si->m_levelId, 0, 1)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x421);
            }
            if (!ParseSerial(this, si->m_serial)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x465);
            }
            CheckSavedMode();
            m_loadingSaveGame = 0;
            return 1;
        }
        case CMD_NO_OP80_8:
            return 1;
        case CMD_MULTI_CONNECT:
            if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {
                static_cast<CMulti*>(m_curState)->Connect(lParam);
            }
            return 1;
        case CMD_LOAD_GAME_DIALOG:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_cdPromptResult) {
                    RunLoadGameDialog();
                }
            }
            return 1;
        case CMD_QUICK_SAVE_PROMPT:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState();
                if (_g->CanQuickSave()) {
                    LoadSaveMessageSprite();
                }
            }
            return 1;
        case CMD_QUICK_SAVE:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState();
                if (_g->CanQuickSave()) {
                    Quicksave();
                }
            }
            return 1;
        case CMD_QUICK_LOAD:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_cdPromptResult) {
                    Quickload();
                }
            }
            return 1;
        case CMD_RESTART_LEVEL:
            RESTART(1);
        case CMD_RESTART_WORLD:
            RESTART(2);
        case CMD_RESTART_WORLD_NO_CURSOR:
            RESTART2(2);
        case CMD_RESTART_GAME:
            RESTART(3);
        case CMD_WARP_LEVEL1:
            WARP(1, 0x422);
        case CMD_DEBUG_WARP_LEVEL37:
            WARP(0x25, 0x45a);
        case CMD_DEBUG_WARP_LEVEL38:
            WARP(0x26, 0x45b);
        case CMD_DEBUG_WARP_LEVEL39:
            WARP(0x27, 0x45c);
        case CMD_DEBUG_WARP_LEVEL40:
            WARP(0x28, 0x45d);
        case CMD_DEBUG_WARP_LEVEL1:
            WARP(1, 0x45e);
        case CMD_DEBUG_WARP_LEVEL2:
            WARP(2, 0x45f);
        case CMD_DEBUG_WARP_LEVEL3:
            WARP(3, 0x460);
        case CMD_DEBUG_WARP_LEVEL4:
            WARP(4, 0x461);
        case CMD_DEBUG_WARP_LEVEL5:
            WARP(5, 0x462);
        case CMD_DEBUG_WARP_LEVEL6:
            WARP(6, 0x45f);
        case CMD_DEBUG_WARP_LEVEL7:
            WARP(7, 0x460);
        case CMD_DEBUG_WARP_LEVEL8:
            WARP(8, 0x461);
        case CMD_DEBUG_WARP_LEVEL9:
            WARP(9, 0x462);
        case CMD_DEBUG_WARP_LEVEL10:
            WARP(0xa, 0x463);
        case CMD_DEBUG_WARP_LEVEL11:
            WARP(0xb, 0x464);
        case CMD_DEBUG_WARP_LEVEL12:
            WARP(0xc, 0x465);
        case CMD_DEBUG_WARP_LEVEL13:
            WARP(0xd, 0x466);
        case CMD_DEBUG_WARP_LEVEL14:
            WARP(0xe, 0x467);
        case CMD_DEBUG_WARP_LEVEL15:
            WARP(0xf, 0x468);
        case CMD_DEBUG_WARP_LEVEL16:
            WARP(0x10, 0x469);
        case CMD_DEBUG_WARP_LEVEL17:
            WARP(0x11, 0x46a);
        case CMD_DEBUG_WARP_LEVEL18:
            WARP(0x12, 0x46b);
        case CMD_DEBUG_WARP_LEVEL19:
            WARP(0x13, 0x46c);
        case CMD_DEBUG_WARP_LEVEL20:
            WARP(0x14, 0x46d);
        case CMD_DEBUG_WARP_LEVEL21:
            WARP(0x15, 0x46e);
        case CMD_DEBUG_WARP_LEVEL22:
            WARP(0x16, 0x46f);
        case CMD_DEBUG_WARP_LEVEL23:
            WARP(0x17, 0x470);
        case CMD_DEBUG_WARP_LEVEL24:
            WARP(0x18, 0x471);
        case CMD_DEBUG_WARP_LEVEL25:
            WARP(0x19, 0x472);
        case CMD_DEBUG_WARP_LEVEL26:
            WARP(0x1a, 0x473);
        case CMD_DEBUG_WARP_LEVEL27:
            WARP(0x1b, 0x474);
        case CMD_DEBUG_WARP_LEVEL28:
            WARP(0x1c, 0x475);
        case CMD_DEBUG_WARP_LEVEL29:
            WARP(0x1d, 0x476);
        case CMD_DEBUG_WARP_LEVEL30:
            WARP(0x1e, 0x477);
        case CMD_DEBUG_WARP_LEVEL31:
            WARP(0x1f, 0x478);
        case CMD_DEBUG_WARP_LEVEL32:
            WARP(0x20, 0x479);
        case CMD_DEBUG_WARP_LEVEL101:
            WARP(0x65, 0x45e);
        case CMD_DEBUG_WARP_LEVEL102:
            WARP(0x66, 0x45f);
        case CMD_DEBUG_WARP_LEVEL103:
            WARP(0x67, 0x460);
        case CMD_DEBUG_WARP_LEVEL104:
            WARP(0x68, 0x461);
        case CMD_DEBUG_WARP_LEVEL105:
            WARP(0x69, 0x462);
        case CMD_DEBUG_WARP_LEVEL106:
            WARP(0x6a, 0x45f);
        case CMD_DEBUG_WARP_LEVEL107:
            WARP(0x6b, 0x460);
        case CMD_DEBUG_WARP_LEVEL108:
            WARP(0x6c, 0x461);
        case CMD_DEBUG_WARP_LEVEL109:
            WARP(0x6d, 0x462);
        case CMD_DEBUG_WARP_LEVEL110:
            WARP(0x6e, 0x463);
        case CMD_DEBUG_WARP_LEVEL111:
            WARP(0x6f, 0x464);
        case CMD_DEBUG_WARP_LEVEL112:
            WARP(0x70, 0x465);
        case CMD_DEBUG_WARP_LEVEL113:
            WARP(0x71, 0x466);
        case CMD_DEBUG_WARP_LEVEL114:
            WARP(0x72, 0x467);
        case CMD_DEBUG_WARP_LEVEL115:
            WARP(0x73, 0x468);
        case CMD_DEBUG_WARP_LEVEL116:
            WARP(0x74, 0x469);
        case CMD_DEBUG_WARP_LEVEL117:
            WARP(0x75, 0x46a);
        case CMD_DEBUG_WARP_LEVEL118:
            WARP(0x76, 0x46b);
        case CMD_DEBUG_WARP_LEVEL119:
            WARP(0x77, 0x46c);
        case CMD_DEBUG_WARP_LEVEL120:
            WARP(0x78, 0x46d);
        case CMD_DEBUG_WARP_LEVEL121:
            WARP(0x79, 0x46e);
        case CMD_DEBUG_WARP_LEVEL122:
            WARP(0x7a, 0x46f);
        case CMD_DEBUG_WARP_LEVEL123:
            WARP(0x7b, 0x470);
        case CMD_DEBUG_WARP_LEVEL124:
            WARP(0x7c, 0x471);
        case CMD_DEBUG_WARP_LEVEL125:
            WARP(0x7d, 0x472);
        case CMD_DEBUG_WARP_LEVEL126:
            WARP(0x7e, 0x473);
        case CMD_DEBUG_WARP_LEVEL127:
            WARP(0x7f, 0x474);
        case CMD_DEBUG_WARP_LEVEL128:
            WARP(0x80, 0x475);
        case CMD_DEBUG_WARP_LEVEL129:
            WARP(0x81, 0x476);
        case CMD_DEBUG_WARP_LEVEL130:
            WARP(0x82, 0x477);
        case CMD_DEBUG_WARP_LEVEL131:
            WARP(0x83, 0x478);
        case CMD_DEBUG_WARP_LEVEL132:
            WARP(0x84, 0x479);
        case CMD_WEB_SITE:
            if (m_curState->Update() == GAMESTATE_MENU
                || m_curState->Update() == GAMESTATE_ATTRACT) {
                while (ShowCursor(1) < 0) {
                }
                LaunchWebBrowser(const_cast<char*>("http://www.gruntzgoo.com/"));
            }
            return 1;
        case CMD_MULTI_JOIN:
            m_gameMode = GAMEMODE_MULTIPLAYER;
            g_hostServicesMode = 0;
            if (!TransitionState(GAMESTATE_MULTI, 1, 0, 0)
                && !TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x424);
            }
            return 1;
        case CMD_MULTI_HOST:
            m_gameMode = GAMEMODE_MULTIPLAYER;
            g_hostServicesMode = 1;
            if (!TransitionState(GAMESTATE_MULTI, 1, 0, 0)
                && !TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x425);
            }
            return 1;
        case CMD_MAIN_MENU:
            if (!TransitionState(GAMESTATE_MENU, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x426);
            }
            return 1;
        case CMD_SHOW_CREDITS:
            if (!TransitionState(GAMESTATE_CREDITS_OVER_CURRENT, 1, 1, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x427);
            }
            return 1;
        case CMD_SHOW_BOOTY:
            if (!TransitionState(GAMESTATE_BOOTY_OVER_CURRENT, 1, 1, lParam)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x428);
            }
            return 1;
        case CMD_NEXT_STATE:
            if (!SwitchToNextState()) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x429);
            }
            return 1;
        case CMD_SHOW_HELP:
            if (!TransitionState(GAMESTATE_CREDITS, 1, 0, 0)
                && !TransitionState(GAMESTATE_MENU, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42a);
            }
            return 1;
        case CMD_ATTRACT:
            if (!TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42b);
            }
            return 1;
        case CMD_RETURN_TO_ATTRACT:
            if (!TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42c);
                return 1;
            }
            PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            return 1;
        case CMD_SHOW_STATE0:
            if (!TransitionState(GAMESTATE_SPLASH, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42d);
            }
            return 1;
        case CMD_SHOW_STATE07:
            if (!TransitionState(GAMESTATE_DEMO, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42e);
            }
            return 1;
        case CMD_PAUSE_TOGGLE: {
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
                CPlay* ps = static_cast<CPlay*>(m_curState);
                if (ps->m_inGame) {
                    return 1;
                }
                if (ps->m_renderDisabled) {
                    return 1;
                }
                if (ps->m_statusBar) {
                    if (ps->m_statusBar->m_levelOverlayActive) {
                        return 1;
                    }
                    if (ps->m_statusBar->m_quitConfirmationActive) {
                        return 1;
                    }
                }
                m_frameGate ^= 1;
                i32 f = m_frameGate;
                FinishLevel(f, 1);
            }
            return 1;
        }
        case CMD_FINISH_LEVEL: {
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
                m_frameGate ^= 1;
                i32 f = m_frameGate;
                FinishLevel(f, 0);
            }
            return 1;
        }
        case CMD_PRESENT_WORLD:
            if (!CheckPlayState()) {
                return 1;
            }
            if (!static_cast<CPlay*>(m_curState)->DrawWorldPresent()) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x42f);
            }
            return 1;
        case CMD_LOBBY_RESET:
            m_lobbyProbed = 0;
            PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8025, 0);
            return 1;
        case CMD_EXIT_TO_ATTRACT:
            if (!CheckPlayState()) {
                return 1;
            }
            if (m_curState->CompleteLevel()) {
                return 1;
            }
            if (!TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x430);
                return 1;
            }
            PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            return 1;
        case CMD_CAPTURE_WORLD:
            if (g_cdPromptResult) {
                return 1;
            }
            CaptureWorldFile();
            return 1;
        case CMD_NEXT_LEVEL:
            if (!GoToNextLevel()) {
                ReportError(IDX(IDS_CHANGE_LEVEL), 0x431);
            }
            return 1;
        case CMD_PREV_LEVEL:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
                GoToPrevLevel();
                return 1;
            }
            // fall through
        case CMD_RETURN_TO_MENU:
            m_curState->m_notifyLatch = 1;
            if (!TransitionState(GAMESTATE_MENU, 1, 0, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x432);
            }
            return 1;
        case CMD_QUIT:
            DelayedQuit();
            return 1;
        case CMD_SHOW_BOOTY_STATE: {
            if (m_curState->Update() == GAMESTATE_HELP
                || m_curState->Update() == GAMESTATE_BOOTY_OVER_CURRENT
                || m_curState->Update() == GAMESTATE_RESERVED_0F
                || m_curState->Update() == GAMESTATE_SPLASH
                || m_curState->Update() == GAMESTATE_CREDITS
                || m_curState->Update() == GAMESTATE_BOOTY
                || m_curState->Update() == GAMESTATE_MULTIBOOTY
                || m_curState->Update() == GAMESTATE_MULTI) {
                return 1;
            }
            if (!TransitionState(GAMESTATE_HELP, 1, 1, 0)) {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x433);
            }
            return 1;
        }
        case CMD_CONFIG_SETTINGS: {
            GameStateId st = m_curState->Update();
            CMenuState* mus;
            if (st == GAMESTATE_MENU) {
                mus = static_cast<CMenuState*>(m_curState);
                (static_cast<CMenuState*>(m_curState))->StopMusicChain();
            } else {
                mus = NULL;
            }

            RunModalDialog("CONFIG_SETTINGS", GameOptionsDlgProc, 0);
            if (mus) {
                mus->StartMusic();
            }
            return 1;
        }
        case CMD_TOGGLE_MUSIC: {
            if (m_frameGate) {
                return 1;
            }
            m_musicEnabled ^= 1;
            i32 enabled = m_musicEnabled;
            i32 isPlayState = CheckPlayState();
            if (!isPlayState) {
                if (m_curState->Update() != GAMESTATE_CREDITS_OVER_CURRENT
                    && m_curState->Update() != GAMESTATE_MENU) {
                    return 1;
                }
            }
            if (enabled) {
                m_midi->RestartCurrent(1);
            } else {
                m_midi->PauseCurrent();
            }
            return 1;
        }
        case CMD_TOGGLE_SOUND: {
            if (m_world) {
                SoundStream* soundStream = m_world->m_soundRegistry->m_soundStream;
                if (soundStream) {
                    soundStream->StopAllStreams();
                }
            }
            m_soundEnabled ^= 1;
            g_soundEnabled = m_soundEnabled;
            i32 soundEnabled = m_soundEnabled;
            if (soundEnabled != 0) {
                m_worldSounds->Resume();
            } else {
                m_worldSounds->Stop();
            }
            return 1;
        }
        case CMD_RESTORE_VIDEO_MODE:
            if (!IsInPlayState()) {
                return 1;
            }
            RestoreVideoMode(0);
            return 1;
        case CMD_CHECK_DISPLAY_BOUNDS_A:
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsA();
            return 1;
        case CMD_CHECK_DISPLAY_BOUNDS_B:
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsB();
            return 1;
        case CMD_SCREENSHOT: {
            SaveFrontBufferShot(
                m_settings,
                this,
                g_gameReg->GetModeSize().cx,
                g_gameReg->GetModeSize().cy,
                NULL,
                0
            );
            return 1;
        }
        case CMD_RELOAD_LEVEL: {
            CPlay* _g = PickPlayOrPausedState();
            if (!_g) {
                return 1;
            }
            if (!PassClickToPlayState(m_curState->m_levelIndex, 0, 1)) {
                ReportError(IDX(IDS_CHANGE_LEVEL), 0x434);
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
