// GruntzMgrCmd.cpp - CGruntzMgr::HandleCommand (RVA 0x862f0): the game's
// WM_COMMAND / accelerator + cheat-code dispatcher - the binary's single largest
// function, defined here in its own TU (the class lives in <Gruntz/GruntzMgr.h>;
// GruntzMgr.cpp holds the other manager methods).
//
// FULLY CANONICAL: the former `namespace GruntzMgrCmd` view-world (a namespaced
// CGruntzMgr shadow + the GZ* per-TU views: GZLogic/GZBoard/GZGrunt/GZGruntLevel/
// GZLevel/GZInput/GZSoundZ/GZMgrSettings/GZDeath*/GZPtrMap/GZSel/GZCell/GZStr/
// GZSaveInfo/GZObj58/GZWndSub/GZM44) is DISSOLVED onto the one true classes:
//   this            = ::CGruntzMgr (GruntzMgr.h; base WAP32::CGameMgr gives
//                     m_gameWnd/m_frameGate/m_soundEnabled/m_musicEnabled)
//   m_curState      = CState (Update() slot 4 == the old vf10; Vslot15 == vf54);
//                     downcast per proven state id: CPlay (3), CMenuState (5,
//                     GameMode.h StopMusicChain/StartMusic), CMulti (0x11, Connect)
//   PickPlayOrPausedState() (thunk 0x355d) yields the CPlay the cheats drive
//                     (SetCursorFrame/Flip/OnRegion1-3/CanQuickSave/m_frameMarker)
//   m_cmdGrid       = CTriggerMgr (ClearRowAndRefresh 0x18e3, CycleMoveIcons 0x3616,
//                     m_grid/m_recHead/m_recCount)
//   m_world->m_28   = CSndHost (SoundCue.h; CueLookup @0x05b7e0 was the fake
//                     stdcall "CueLookup" free fn - it is a real CSndHost thiscall)
//   m_sound         = CGruntzSoundZ (Restart 0x1388c0 / StopAll 0x1388f0)
//   m_saveSink      = SaveSink58, m_saveInfoRec = SaveInfo (SaveInfo.h)
//   *0x64556c       = g_gameReg (CGameRegistry; the death cheat's key is
//                     m_focusSlots[0].m_0c, its map CSpriteFactory::m_objMap)
// Grid cells are CGrunt (m_tileOwnerHi/Lo @+0x1ec/+0x1f0, LoadPickupSprites
// @0x65e80, LoadGruntAbilityTuning @0x57100); the CTmCell==CGrunt retype in
// TriggerMgr.h is deferred (owned by a parallel worker) - the two casts below
// carry the evidence. CPlay::m_guts' concrete type is CStatusBarMgr
// (UpdateDestructButton @0x10bc30 / AdvanceGauge @0x105750) - the member retype
// is deferred to the Play.cpp reconciliation.
#include <Ints.h>
#include <Gruntz/LeafCue.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Mfc.h>

#include <rva.h>
#include <string.h>

#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg) + CSpriteFactoryHolder
#include <Gruntz/GruntzMgr.h>     // the real CGruntzMgr (this) + SaveInfo.h + SoundCue.h
#include <Gruntz/Play.h>          // CPlay (the cheat receiver) + CTimer (m_frameMarker)
#include <Gruntz/GameMode.h>      // CMenuState (StopMusicChain/StartMusic; state 5)
#include <Gruntz/Multi.h>         // CMulti (Connect; state 0x11)
#include <Gruntz/TriggerMgr.h>    // CTriggerMgr (m_cmdGrid)
#include <Gruntz/Grunt.h>         // CGrunt (grid cells) + GruntObjEntry (death chain)
#include <Gruntz/StatusBarMgr.h>  // CStatusBarMgr (the play state's +0x2dc guts receiver)
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ (m_sound)
#include <Gruntz/WorldSoundSet.h> // CWorldSoundSet (m_inputState @+0x54; Stop/Resume)

// The *0x24556c game-manager singleton. Declared here (it used to arrive from
// <Gruntz/Play.h>, whose header-level decl was removed so each TU can pick the view /
// real class it needs -- see the note in Play.h). Type unchanged for this TU.
extern "C" CGameRegistry* g_gameReg;

// The record-list node (CTriggerMgr::m_recHead): the MFC CPtrList node shape
// {next, prev, data}; the data payload of a record node is the placed (x,y)
// pair (CTrigPoint). Completes TriggerMgr.h's forward declaration for this TU
// (the header's sanctioned per-TU completion pattern).
struct CTmNode {
    CTmNode* m_next;  // +0x00
    CTmNode* m_prev;  // +0x04
    CTrigPoint* m_pt; // +0x08  the record's placed (x,y)
};

// The game-manager singleton global comes in WwdGameReg-typed via Grunt.h
// (WwdGameReg.h) - the grunt-facet view of the same *0x64556c object this class
// IS. The two sites below that need the CGameRegistry facet (m_focusSlots /
// m_modeW) bridge-cast; the WwdGameReg->CGameRegistry view unification is a
// deferred Grunt.h-scale fold.
DATA(0x0021ab24)
extern i32 g_sndCueTag;
DATA(0x0021ab20)
extern i32 g_sndEnabled; // ?g_sndEnabled@@3HA (mirrors m_soundEnabled)
// Cheat toggles (named from their own ShowToggleMessage strings).
DATA(0x002455a4)
extern i32 g_gruntDestruction; // "Grunt destruction"
DATA(0x002455a8)
extern i32 g_gruntCreation; // "Grunt creation"
DATA(0x002455ac)
extern i32 g_gooPuddlez; // "Goo puddlez"
DATA(0x002455b0)
extern i32 g_traitorMode; // "Traitor Mode"
DATA(0x002455e8)
extern i32 g_monologoShown; // the MONOLITH logo is on screen (LoadMonologoSprite)
DATA(0x002455ec)
extern i32 g_6455ec; // load/quicksave-UI suppress gate (role unproven)
DATA(0x002455f4)
extern i32 g_debugDisplayFlags; // bits: 1 obj count, 4 world pos, 0x10 frame rate,
                                // 0x20/0x400 ?, 0x40/0x100 brick text, 0x80 elapsed time
DATA(0x002455f8)
extern i32 g_explosionz; // "Explosionz"
DATA(0x002bf3c0)
// the draw-clock mirror (here: the 0x8247 cue-cooldown throttle). Was a C++-mangled
// ?g_time6bf3c0@@3HA - a divergent symbol for a cell 9 other TUs share.
extern "C" u32 g_killCueClock;
DATA(0x00244c54)            // RVA (was VA-typo 0x644c54, which shadowed the canonical _g_644c54)
extern "C" i32 g_curPlayer; // the magic group/kind id (grid-cheat gate; == TriggerMgr's)
DATA(0x00248cf0) // RVA (was VA-typo 0x648cf0, which shadowed multi's canonical 0x248cf0 binding)
extern i32 g_isHost_648cf0;
extern i32(__cdecl* g_pwsprintfA)(char*, const char*, ...);

// The two brick-display strings the 0x8068/0x806f cheats clear (real CStrings;
// the "brick text" debug overlay renders them).
DATA(0x00245524)
extern CString g_brickText1;
DATA(0x00245528)
extern CString g_brickText2;

// ParseSerial (@0x0d210) + SerialObjectFactory (@0x0d2a0) carved out to
// src/Gruntz/SerialObjectFactory.cpp in REHOME D9 (their contiguous 0xd210-0xec24
// .text block is a separate obj). HandleCommand's WM_COMMAND 0x807e path calls
// ParseSerial cross-TU (declared here, defined there).
i32 ParseSerial(CGameRegistry* mgr, char* s); // 0x0d210 (SerialObjectFactory.cpp)

// The world-present toolbar builder chain (thunk 0x277a -> the 6-arg __cdecl
// forwarder Fwd114ec0 @0x114ec0 -> the guarded forwarder Fwd114f00 @0x114f00). Both
// forwarders were carved out to src/Gruntz/Fwd114ec0.cpp in REHOME D9 (their
// 0x114ec0-0x114f3e block is a separate obj); HandleCommand's WM_COMMAND 0x8070 path
// calls Fwd114ec0 cross-TU (declared here, defined there). The real param shapes are
// unrecovered; the i32 tuple mirrors the forwarder.
void Fwd114ec0(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x114ec0 (Fwd114ec0.cpp)

// Play a named cue when the world's cue host is not muted: resolve via the
// host's CueLookup (a real CSndHost __thiscall @0x05b7e0 - ecx is the host at
// every retail site because the m_emitGate gate test just loaded it).
#define PLAYCUE(TAG)                                                                               \
    if (m_world->m_28->m_emitGate == 0) {                                                          \
        LeafCue* _c = (LeafCue*)((CDDrawSubMgrLeafScan*)m_world->m_28)->Lookup_05b7e0(TAG);        \
        if (_c)                                                                                    \
            _c->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);                                        \
    }
// Cue via the host's finder (m_10, CSndFinder) with a stack out-ptr; used by a
// handful of cheats instead of CueLookup.
#define PLAYCUE_MAP(TAG)                                                                           \
    if (m_world->m_28->m_emitGate == 0) {                                                          \
        LeafCue* _c = 0;                                                                           \
        m_world->m_28->m_10.Lookup(TAG, (void*&)_c); /* CMapStringToPtr (0x1b8438) */              \
        if (_c)                                                                                    \
            _c->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);                                        \
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
        CGrunt* _cell =                                                                            \
            m_cmdGrid->m_recList.GetCount() == 1                                                   \
                ? (CGrunt*)m_cmdGrid->m_grid                                                       \
                      [((CTmNode*)m_cmdGrid->m_recList.GetHeadPosition())->m_pt->y                 \
                       + ((CTmNode*)m_cmdGrid->m_recList.GetHeadPosition())->m_pt->x * 15]         \
                : 0;                                                                               \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_tileOwnerHi != g_curPlayer)                                                   \
            return 0;                                                                              \
        CGrunt* _c2 =                                                                              \
            (CGrunt*)m_cmdGrid->m_grid[_cell->m_tileOwnerLo + _cell->m_tileOwnerHi * 15];          \
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
        CGrunt* _cell =                                                                            \
            m_cmdGrid->m_recList.GetCount() == 1                                                   \
                ? (CGrunt*)m_cmdGrid->m_grid                                                       \
                      [((CTmNode*)m_cmdGrid->m_recList.GetHeadPosition())->m_pt->y                 \
                       + ((CTmNode*)m_cmdGrid->m_recList.GetHeadPosition())->m_pt->x * 15]         \
                : 0;                                                                               \
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
            mus = (CMenuState*)m_curState;                                                         \
            ((CMenuState*)m_curState)->StopMusicChain();                                           \
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
            mus = (CMenuState*)m_curState;                                                         \
            ((CMenuState*)m_curState)->StopMusicChain();                                           \
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
i32 CGruntzMgr::HandleCommand(i32 p1, i32 nID, i32 p3) {
    switch (nID) {
        case 0x8005:
        case 0x8024:
            m_134 = 1;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(0x8005, 0x41e);
            }
            return 1;
        case 0x807f:
            m_strWorldFile.Empty();
            m_134 = 1;
            if (!PassClickToPlayState(p3, 0, 1)) {
                ReportError(0x8005, 0x41f);
            }
            return 1;
        case 0x8174:
            m_strWorldFile.Empty();
            m_134 = 1;
            if (!PassClickToPlayState(m_saveSink ? m_saveSink->m_curLevel : 0, 0, 1)) {
                ReportError(0x8005, 0x41f);
            }
            return 1;
        case 0x80e3:
            m_134 = 3;
            if (!PassClickToPlayState(1, 0, 1)) {
                ReportError(0x8005, 0x420);
            }
            return 1;
        case 0x80e1:
            SaveGameAs();
            // fall through to default
        default:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                switch (nID & 0xffff) {
                    case 0x803b: {
                        if (m_world->m_28->m_emitGate == 0) {
                            LeafCue* _c = (LeafCue*)((CDDrawSubMgrLeafScan*)m_world->m_28)
                                              ->Lookup_05b7e0("GAME_MINORCHEAT");
                            if (_c) {
                                _c->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("Brian L. Goble is a programming God...");
                        return 1;
                    }
                    case 0x8043:
                        g_traitorMode ^= 1;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Traitor Mode", g_traitorMode);
                        return 1;
                    case 0x804d:
                        g_debugDisplayFlags ^= 1;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Object Count Display", g_debugDisplayFlags & 1);
                        return 1;
                    case 0x804c:
                        g_debugDisplayFlags ^= 4;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("World Position Display", g_debugDisplayFlags & 4);
                        return 1;
                    case 0x804b:
                        g_debugDisplayFlags ^= 0x10;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Frame Rate Display", g_debugDisplayFlags & 0x10);
                        return 1;
                    case 0x804e:
                        g_debugDisplayFlags ^= 0x20;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case 0x8068:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ 0x40) & ~0x100;
                        g_brickText1.Empty();
                        g_brickText2.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case 0x806f:
                        g_debugDisplayFlags = (g_debugDisplayFlags ^ 0x100) & ~0x40;
                        g_brickText1.Empty();
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case 0x806e:
                        g_debugDisplayFlags ^= 0x80;
                        PLAYCUE("GAME_MINORCHEAT");
                        ShowToggleMessage("Elapsed Time Display", g_debugDisplayFlags & 0x80);
                        return 1;
                    case 0x8086: {
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
                    case 0x8087:
                        return 1;
                    case 0x808d:
                        BRICKPICKUP(0x36, "Hey, where did you go?");
                    // ---- item cheats 0x80e5..0x8104 (SetItem N, announce) ----
                    case 0x80e5:
                        ITEMCHEAT(1, "Bombz are cool!");
                    case 0x80e6:
                        ITEMCHEAT(2, "Boomerangz are cool!");
                    case 0x80e7:
                        ITEMCHEAT(3, "Brick Laying Toolz are cool!");
                    case 0x80e8:
                        ITEMCHEAT(4, "Clubz are cool!");
                    case 0x80e9:
                        ITEMCHEAT(5, "Gauntletz are cool!");
                    case 0x80ea:
                        ITEMCHEAT(6, "Glovez are cool!");
                    case 0x80eb:
                        ITEMCHEAT(7, "Gooberz are cool!");
                    case 0x80ec:
                        ITEMCHEAT(8, "Gravity Bootz are cool!");
                    case 0x80ed:
                        ITEMCHEAT(9, "Gun Hatz are cool!");
                    case 0x80ee:
                        ITEMCHEAT(0xa, "Sponge Gunz are cool!");
                    case 0x80ef:
                        ITEMCHEAT(0xb, "Rockz are cool!");
                    case 0x80f0:
                        ITEMCHEAT(0xc, "Shieldz are cool!");
                    case 0x80f1:
                        ITEMCHEAT(0xd, "Shovelz are cool!");
                    case 0x80f2:
                        ITEMCHEAT(0xe, "Springz are cool!");
                    case 0x80f3:
                        ITEMCHEAT(0xf, "Spy Gear is cool!");
                    case 0x80f4:
                        ITEMCHEAT(0x10, "Swordz are cool!");
                    case 0x80f5:
                        ITEMCHEAT(0x11, "Time Bombz are cool!");
                    case 0x80f6:
                        ITEMCHEAT(0x12, "Toobz are cool!");
                    case 0x80f7:
                        ITEMCHEAT(0x13, "Magic Wandz are cool!");
                    case 0x80f8:
                        ITEMCHEAT(0x14, "Hey, how did you get this cheat?");
                    case 0x80f9:
                        ITEMCHEAT(0x15, "Welder's Kitz are cool!");
                    case 0x80fa:
                        ITEMCHEAT(0x16, "Wingz are cool!");
                    case 0x80fb:
                        ITEMCHEAT(0x17, "Baby-Walkerz are cool!");
                    case 0x80fc:
                        ITEMCHEAT(0x18, "Beach Ballz are cool!");
                    case 0x80fd:
                        ITEMCHEAT(0x19, "Monster Wheelz are cool!");
                    case 0x80fe:
                        ITEMCHEAT(0x1a, "Go-Kartz are cool!");
                    case 0x80ff:
                        ITEMCHEAT(0x1b, "Jack-In-The-Boxez are cool!");
                    case 0x8100:
                        ITEMCHEAT(0x1c, "Jump Ropez are cool!");
                    case 0x8101:
                        ITEMCHEAT(0x1d, "Pogo Stickz are cool!");
                    case 0x8102:
                        ITEMCHEAT(0x1e, "Scrollz are cool!");
                    case 0x8103:
                        ITEMCHEAT(0x1f, "Squeak Toyz are cool!");
                    case 0x8104:
                        ITEMCHEAT(0x20, "Yo-Yoz are cool!");
                    case 0x8106: {
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_cmdGrid->ClearRowAndRefresh(5);
                        CGameRegistry* _s =
                            (CGameRegistry*)g_gameReg; // dual-view bridge (same object)
                        void* _key = (void*)_s->m_focusSlots[0].m_0c; // death/monologo sprite key
                        if (_key) {
                            GruntObjEntry* _dr = 0;
                            if (((CMapPtrToPtr*)&_s->m_world->m_8->m_objMap)
                                    ->Lookup((void*)_key, (void*&)_dr)
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
                    case 0x8107: {
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
                    case 0x8128:
                        ITEMCHEAT(0x26, "Bomb Brickz are cool!");
                    case 0x8129:
                        ITEMCHEAT(0x25, "Indestructible Brickz are cool!");
                    case 0x812b:
                        ITEMCHEAT(0x23, "Gauntlet-Breaker Brickz are cool!");
                    case 0x812a:
                        ITEMCHEAT(0x24, "Teleport Brickz are cool!");
                    // ---- "pickup brick" cheats: grid-select a cell, LoadPickup(id) ----
                    case 0x8130:
                        BRICKPICKUP(0x39, "Oh yes, they will be assimilated!");
                    case 0x8131:
                        BRICKPICKUP(
                            0x3a,
                            "Ladies and gentlemen, please welcome... death... "
                            "He'll be here all week."
                        );
                    case 0x8132:
                        BRICKPICKUP(0x38, "Super Grunt to the rescue!");
                    case 0x8133:
                        BRICKPICKUP(0x3c, "This is gonna hurt them more than it will hurt you.");
                    case 0x8134:
                        BRICKPICKUP(0x3b, "How did you swallow that?");
                    case 0x8135:
                        BRICKPICKUP(0x37, "There is no running allowed by the pool!");
                    case 0x8136:
                        if (!PickPlayOrPausedState()) {
                            return 0;
                        }
                        m_cmdGrid->CycleMoveIcons(-1, 1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("How about a little color in your Gruntz?");
                        return 1;
                    case 0x8137: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion3(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Whoah... you should get this monitor fixed.");
                        return 1;
                    }
                    case 0x8138: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion1(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Is is dark in here?");
                        return 1;
                    }
                    case 0x8139: {
                        CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                        if (!_g) {
                            return 0;
                        }
                        _g->OnRegion2(1);
                        PLAYCUE("GAME_MAJORCHEAT");
                        AppendChatMessage("Awww... isn't this little window cute?");
                        return 1;
                    }
                    case 0x813c:
                        BRICKABILITY(1, "Freeze spellz are coooooooooooooooooool!");
                    case 0x813d:
                        BRICKABILITY(2, "For only $9.95, you too can have the healing power!");
                    case 0x813e:
                        BRICKABILITY(3, "Aaahh!  Zombiez!");
                    case 0x813a:
                        BRICKABILITY(4, "It's party time!");
                    case 0x813f:
                        BRICKABILITY(5, "Oh where oh where did the teleported Gruntz go?");
                    case 0x813b:
                        BRICKABILITY(6, "Rollin, rollin, rollin.");
                    case 0x816f:
                        g_debugDisplayFlags ^= 0x400;
                        PLAYCUE("GAME_MINORCHEAT");
                        return 1;
                    case 0x8175:
                        if (m_world->m_28->m_emitGate == 0) {
                            LeafCue* _c = (LeafCue*)((CDDrawSubMgrLeafScan*)m_world->m_28)
                                              ->Lookup_05b7e0("GAME_WAWA");
                            if (_c) {
                                _c->PlayIfElapsed_01f940(0x64, 0, 0, 0);
                            }
                        }
                        AppendChatMessage("WA WA WA WA WA WA!");
                        return 1;
                    case 0x807a:
                    case 0x807b:
                    case 0x8246: {
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
                    case 0x81a3:
                        g_gooPuddlez ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Goo puddlez", g_gooPuddlez);
                        return 1;
                    case 0x81a4: {
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
                    case 0x81a5:
                        g_gruntCreation ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt creation", g_gruntCreation);
                        return 1;
                    case 0x81a6:
                        g_gruntDestruction ^= 1;
                        PLAYCUE("GAME_MAJORCHEAT");
                        ShowToggleMessage("Grunt destruction", g_gruntDestruction);
                        return 1;
                    case 0x81a9:
                        PLAYCUE("GAME_MAJORCHEAT");
                        if (m_saveSink) {
                            m_saveSink->SetCurLevel(0x20);
                            m_saveSink->Set();
                        }
                        AppendChatMessage(
                            "They should call you Cheat Cheatelson from "
                            "Cheatstown Virginia who lives at 1105 Cheat Circle "
                            "just behind the CheatMart superstore."
                        );
                        return 1;
                    case 0x81d6:
                        RunModalDialog(
                            "PSYCHE",
                            (void*)0x402649,
                            0
                        ); // bare imm matches the target (LAB_, no reloc/symbol)
                        return 1;
                    case 0x81d7:
                        PLAYCUE("GAME_MAJORCHEAT");
                        m_hudGuard->m_124 = 0;
                        AppendChatMessage("Cheatz cleared");
                        return 1;
                    case 0x8240:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Trouble in the Tropicz activated!");
                        m_saveSink->SetCurLevel(8);
                        return 1;
                    case 0x8241:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High on Sweetz activated!");
                        m_saveSink->SetCurLevel(0xc);
                        return 1;
                    case 0x8242:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to High Rollerz activated!");
                        m_saveSink->SetCurLevel(0x10);
                        return 1;
                    case 0x8243:
                        PLAYCUE("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Honey, I Shrunk the Gruntz activated!");
                        m_saveSink->SetCurLevel(0x14);
                        return 1;
                    case 0x8244:
                        PLAYCUE_MAP("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to The Miniature Masterz activated!");
                        m_saveSink->SetCurLevel(0x18);
                        return 1;
                    case 0x8245:
                        PLAYCUE_MAP("GAME_MINORCHEAT");
                        AppendChatMessage("Warp to Gruntz in Space activated!");
                        m_saveSink->SetCurLevel(0x1c);
                        return 1;
                    case 0x8247: {
                        g_explosionz ^= 1;
                        if (m_world->m_28->m_emitGate == 0) {
                            void* _c_ob = 0;
                            m_world->m_28->m_10.Lookup("GAME_MAJORCHEAT", _c_ob);
                            LeafCue* _c = (LeafCue*)_c_ob;
                            if (_c && g_sndEnabled) {
                                i32 now = g_killCueClock;
                                if ((u32)(now - _c->m_14) >= (u32)_c->m_18) {
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
        case 0x807e: {
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
            (void)p1;
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
            if (!ParseSerial((CGameRegistry*)this, si->m_serial)) {
                ReportError(0x8005, 0x465);
            }
            CheckSavedMode();
            m_114 = 0;
            return 1;
        }
        case 0x80b8:
            return 1;
        case 0x80d7:
            if (m_curState && m_curState->Update() == GAMESTATE_NONE) {
                ((CMulti*)m_curState)->Connect(p3);
            }
            return 1;
        case 0x80ce:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_6455ec) {
                    RunLoadGameDialog();
                }
            }
            return 1;
        case 0x80cf:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                if (_g->CanQuickSave()) {
                    LoadSaveMessageSprite();
                }
            }
            return 1;
        case 0x80d8:
            if (m_curState->Update() == GAMESTATE_PLAY) {
                CPlay* _g = PickPlayOrPausedState(); // FindStateById(3) - the PLAY state
                if (_g->CanQuickSave()) {
                    Quicksave();
                }
            }
            return 1;
        case 0x80d9:
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MENU) {
                if (!g_6455ec) {
                    Quickload();
                }
            }
            return 1;
        case 0x8170:
            RESTART(1);
        case 0x8171:
            RESTART(2);
        case 0x8172:
            RESTART2(2);
        case 0x8173:
            RESTART(3);
        case 0x800d:
            WARP(1, 0x422);
        case 0x814a:
            WARP(0x25, 0x45a);
        case 0x814b:
            WARP(0x26, 0x45b);
        case 0x814c:
            WARP(0x27, 0x45c);
        case 0x814d:
            WARP(0x28, 0x45d);
        case 0x814e:
            WARP(1, 0x45e);
        case 0x814f:
            WARP(2, 0x45f);
        case 0x8150:
            WARP(3, 0x460);
        case 0x8151:
            WARP(4, 0x461);
        case 0x8152:
            WARP(5, 0x462);
        case 0x8153:
            WARP(6, 0x45f);
        case 0x8154:
            WARP(7, 0x460);
        case 0x8155:
            WARP(8, 0x461);
        case 0x8156:
            WARP(9, 0x462);
        case 0x8157:
            WARP(0xa, 0x463);
        case 0x8158:
            WARP(0xb, 0x464);
        case 0x8159:
            WARP(0xc, 0x465);
        case 0x815a:
            WARP(0xd, 0x466);
        case 0x815b:
            WARP(0xe, 0x467);
        case 0x815c:
            WARP(0xf, 0x468);
        case 0x815d:
            WARP(0x10, 0x469);
        case 0x815e:
            WARP(0x11, 0x46a);
        case 0x815f:
            WARP(0x12, 0x46b);
        case 0x8160:
            WARP(0x13, 0x46c);
        case 0x8161:
            WARP(0x14, 0x46d);
        case 0x8162:
            WARP(0x15, 0x46e);
        case 0x8163:
            WARP(0x16, 0x46f);
        case 0x8164:
            WARP(0x17, 0x470);
        case 0x8165:
            WARP(0x18, 0x471);
        case 0x8166:
            WARP(0x19, 0x472);
        case 0x8167:
            WARP(0x1a, 0x473);
        case 0x8168:
            WARP(0x1b, 0x474);
        case 0x8169:
            WARP(0x1c, 0x475);
        case 0x816a:
            WARP(0x1d, 0x476);
        case 0x816b:
            WARP(0x1e, 0x477);
        case 0x816c:
            WARP(0x1f, 0x478);
        case 0x816d:
            WARP(0x20, 0x479);
        case 0x81b6:
            WARP(0x65, 0x45e);
        case 0x81b7:
            WARP(0x66, 0x45f);
        case 0x81b8:
            WARP(0x67, 0x460);
        case 0x81b9:
            WARP(0x68, 0x461);
        case 0x81ba:
            WARP(0x69, 0x462);
        case 0x81bb:
            WARP(0x6a, 0x45f);
        case 0x81bc:
            WARP(0x6b, 0x460);
        case 0x81bd:
            WARP(0x6c, 0x461);
        case 0x81be:
            WARP(0x6d, 0x462);
        case 0x81bf:
            WARP(0x6e, 0x463);
        case 0x81c0:
            WARP(0x6f, 0x464);
        case 0x81c1:
            WARP(0x70, 0x465);
        case 0x81c2:
            WARP(0x71, 0x466);
        case 0x81c3:
            WARP(0x72, 0x467);
        case 0x81c4:
            WARP(0x73, 0x468);
        case 0x81c5:
            WARP(0x74, 0x469);
        case 0x81c6:
            WARP(0x75, 0x46a);
        case 0x81c7:
            WARP(0x76, 0x46b);
        case 0x81c8:
            WARP(0x77, 0x46c);
        case 0x81c9:
            WARP(0x78, 0x46d);
        case 0x81ca:
            WARP(0x79, 0x46e);
        case 0x81cb:
            WARP(0x7a, 0x46f);
        case 0x81cc:
            WARP(0x7b, 0x470);
        case 0x81cd:
            WARP(0x7c, 0x471);
        case 0x81ce:
            WARP(0x7d, 0x472);
        case 0x81cf:
            WARP(0x7e, 0x473);
        case 0x81d0:
            WARP(0x7f, 0x474);
        case 0x81d1:
            WARP(0x80, 0x475);
        case 0x81d2:
            WARP(0x81, 0x476);
        case 0x81d3:
            WARP(0x82, 0x477);
        case 0x81d4:
            WARP(0x83, 0x478);
        case 0x81d5:
            WARP(0x84, 0x479);
        case 0x8038:
            if (m_curState->Update() == GAMESTATE_MENU
                || m_curState->Update() == GAMESTATE_ATTRACT) {
                while (::ShowCursor(1) < 0) {
                }
                LaunchWebBrowser("http://www.gruntzgoo.com/");
            }
            return 1;
        case 0x80d2:
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
        case 0x80d3:
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
        case 0x8023:
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x426);
            return 1;
        case 0x8080:
            if (TransitionState(0xb, 1, 1, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x427);
            return 1;
        case 0x8090:
            if (TransitionState(0xd, 1, 1, p3)) {
                return 1;
            }
            ReportError(0x8005, 0x428);
            return 1;
        case 0x8036:
            if (SwitchToNextState()) {
                return 1;
            }
            ReportError(0x8005, 0x429);
            return 1;
        case 0x8021:
            if (TransitionState(8, 1, 0, 0)) {
                return 1;
            }
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42a);
            return 1;
        case 0x8027:
            if (TransitionState(2, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42b);
            return 1;
        case 0x8029:
            if (!TransitionState(2, 1, 0, 0)) {
                ReportError(0x8005, 0x42c);
                return 1;
            }
            ::PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
            return 1;
        case 0x80ab:
            if (TransitionState(0xe, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42d);
            return 1;
        case 0x8022:
            if (TransitionState(7, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x42e);
            return 1;
        case 0x8007: { // 0x89b97
            i32 st = m_curState->Update();
            if (st == GAMESTATE_PLAY || st == GAMESTATE_NONE) {
                CPlay* ps = (CPlay*)m_curState; // id-proven downcast (CMulti : CPlay)
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
        case 0x816e: { // 0x89c19
            i32 st = m_curState->Update();
            if (st == GAMESTATE_PLAY || st == GAMESTATE_NONE) {
                i32 f = m_frameGate ^ 1;
                m_frameGate = f;
                FinishLevel(f, 0);
            }
            return 1;
        }
        case 0x8084:
            if (!CheckPlayState()) {
                return 1;
            }
            if (((CPlay*)m_curState)->DrawWorldPresent()) {
                return 1;
            }
            ReportError(0x8005, 0x42f);
            return 1;
        case 0x80b7:
            m_lobbyProbed = 0;
            ::PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x8025, 0);
            return 1;
        case 0x800e: // 0x89c92
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
        case 0x8042: // 0x89d00
            if (g_6455ec) {
                return 1;
            }
            CaptureWorldFile();
            return 1;
        case 0x8075: // 0x89d1e
            if (GoToNextLevel()) {
                return 1;
            }
            ReportError(0x8007, 0x431);
            return 1;
        case 0x800f: // 0x89d37 -> falls into 0x8006
            if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_NONE) {
                GoToPrevLevel();
                return 1;
            }
            // fall through
        case 0x8006: // 0x89d62
            m_curState->m_40 = 1;
            if (TransitionState(5, 1, 0, 0)) {
                return 1;
            }
            ReportError(0x8005, 0x432);
            return 1;
        case 0x8008: // 0x89d8d
            DelayedQuit();
            return 1;
        case 0x8035: { // 0x89d9e
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
        case 0x80e2: { // 0x89e58  CONFIG_SETTINGS modal
            i32 st = m_curState->Update();
            CMenuState* mus = 0;
            if (st == GAMESTATE_MENU) {
                mus = (CMenuState*)m_curState;
                ((CMenuState*)m_curState)->StopMusicChain();
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
        case 0x800a: { // 0x89e9f  elapsed-time / sound toggle
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
        case 0x8009: { // 0x89f08  world-position display toggle
            if (m_world) {
                SoundStream* p = m_world->m_28->m_2c;
                if (p) {
                    p->Stop();
                }
            }
            i32 v = m_soundEnabled ^ 1;
            m_soundEnabled = v;
            g_sndEnabled = v;
            if (v == 0) {
                m_inputState->Stop(); // 0x29b9->0xbc80 CWorldSoundSet::Stop (was CInput54::Disarm)
            } else {
                m_inputState->Resume(); // 0x18e8->0xbcf0 CWorldSoundSet::Resume (was CInput54::Arm)
            }
            return 1;
        }
        case 0x802c: // 0x89f5a
            if (!IsInPlayState()) {
                return 1;
            }
            RestoreVideoMode(0);
            return 1;
        case 0x802a: // 0x89f7c
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsA();
            return 1;
        case 0x802b: // 0x89f9c
            if (!IsInPlayState()) {
                return 1;
            }
            CheckDisplayBoundsB();
            return 1;
        case 0x8070: { // 0x89fbc  world-present toolbar
            Fwd114ec0(
                (i32)m_settings,
                (i32)this,
                ((CGameRegistry*)g_gameReg)->m_modeW,
                ((CGameRegistry*)g_gameReg)->m_modeH,
                0,
                0
            );
            return 1;
        }
        case 0x806b: {
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
