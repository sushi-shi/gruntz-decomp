// GruntzMgrCmd.cpp - CGruntzMgr::HandleCommand (RVA 0x862f0): the game's
// WM_COMMAND / accelerator + cheat-code dispatcher.  Re-homed out of the
// src/Stub/ApiCallers.cpp winapi grab-bag into its own CGruntzMgr unit so the
// class is not mixed with unrelated api-caller stubs; RVA() binds the symbol by
// address regardless of which file/unit it lives in.
#include <Ints.h>
// <Mfc.h> (superset of <Win32.h>) so the canonical MFC classes below (CGruntzMgr,
// CPlay/CState, CTriggerMgr, CGrunt, CString) enter this TU - the GZ* per-TU views
// are all dissolved onto them.
#include <Mfc.h>

#include <rva.h>
#include <string.h>

// The sound-cue cluster (CSndHost/CSndFinder/CSndEmitter/CSoundCueMgr/SoundStream) and
// the world resource holder (CSpriteFactoryHolder) are the ONE canonical shapes.
#include <Gruntz/GameRegistry.h>
#include <Gruntz/InputState.h>    // CInput54 (+0x54 input/spatial-sound object)
#include <Gruntz/GruntzMgr.h>     // the real polymorphic CGruntzMgr (: WAP32::CGameMgr)
#include <Gruntz/Play.h>          // CState / CPlay (m_curState game-state)
#include <Gruntz/TriggerMgr.h>    // CTriggerMgr (+0x68 command/trigger grid)
#include <Gruntz/Grunt.h>         // CGrunt (PickState result) + CSpriteFactory
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ (+0x48 sound object)

namespace GruntzMgrCmd {
    // ================= RVA 0x862f0 : CGruntzMgr::HandleCommand =================
    // The game's WM_COMMAND / accelerator + cheat-code dispatcher.  A 116-entry
    // "UI command" jump-table switch on the command id, whose default path checks
    // the play-state and then runs the (mode==3-gated) cheat sub-switch.  The
    // near-identical cheat families (item/warp/brick/announce) use macros
    // (ITEMCHEAT/WARP/BRICKPICKUP/BRICKABILITY/PLAYCUE) that expand to the retail
    // per-cheat code; the special cheats (AMBIENT/monologo/save-load/settings) are
    // hand-written.  Reloc-masked engine callees are modeled as bodyless methods/fns.
    //
    // NOTE: this GruntzMgrCmd::CGruntzMgr is a namespaced view of the one true CGruntzMgr
    // (<Gruntz/GruntzMgr.h>), using the canonical GruntzMgr.h member names. VIEW-FOLD IN
    // PROGRESS (the "megafunction excuse" is abolished - this is sequenced, not a wall):
    //   DONE: the whole sound path is canonical (GameRegistry.h -> SoundCue.h). m_world is a
    //   CSpriteFactoryHolder whose m_28 is a CSndHost (finder m_10, stream m_2c==SoundStream,
    //   gate m_30); CueLookup / the finder yield a CSndEmitter (Play @0x1f940, m_10 the
    //   CSoundCueMgr with ConfigureItem @0x1360d0). The former GZGateA/GZGateB/GZStrMap/
    //   GZSound/GZCueMgr/GZGate2c views are deleted (body byte-neutral: megafunction held
    //   67.77%). The SoundStream/m_2c + CSndEmitter::Play additions to the 60-TU-wide
    //   SoundCue.h re-flip the butterfly-sensitive wwdfile CPlaneRender Save/Load pair
    //   100->99.98 (diffuse ripple from a binary-proven-correct shape - accepted per the
    //   cleanup mandate, diagnosed: SoundCue.h type-count via GameRegistry.h->wwdfile).
    //   DEFERRED FOLD (identity map, verified RVAs - promote onto the canonical classes):
    //     GruntzMgrCmd::CGruntzMgr   -> canonical CGruntzMgr (GruntzMgr.h) - HandleCommand
    //       + the ~35 helper methods (PickState 0x355d, PassClick 0x17c1, ...) move onto it
    //     GZLogic (m_curState)       -> CState (State.h); vf10/vf54 are real vtable slots
    //       (need the slot RVAs before declaring them - do NOT invent slots)
    //     GZBoard  (m_cmdGrid @0x68) -> CTriggerMgr (TriggerMgr.h) - m_cells==m_grid+0x1c,
    //       Board18e3/Board3616 are CTriggerMgr methods
    //     GZGrunt  (PickState result)-> CGrunt; GZGruntLevel==grunt->m_2dc active-level obj
    //     GZInput  (m_inputState)    -> CInput54 (InputState.h)
    //     GZMgrSettings (*0x64556c)  -> CGameRegistry (== g_gameReg); the death path is a
    //       real CMapPtrToPtr (Lookup 0x1b8760) hung off m_world->m_8->+0x48
    //   Each needs canonical-header method additions (mostly non-virtual = other-TU-neutral)
    //   + field re-expression; sequenced to keep this session's blast radius bounded.
    // Global thermal-nuclear-war (0x8106) reaches a per-cell death record through the
    // settings singleton and a CMapPtrToPtr; only the touched offsets are modeled.
    struct GZDeath {
        void ResolveDeathAnimation(); // 0x3a1c __thiscall
    };
    struct GZDeathSub {
        char _p0[0x18];
        GZDeath* m_18; // +0x18
    };
    struct GZDeathObj {
        char _p0[0x7c];
        GZDeathSub* m_7c; // +0x7c
    };
    struct GZPtrMap {                            // CMapPtrToPtr view
        i32 Lookup(void* key, GZDeathObj*& out); // 0x1b8760 __thiscall
    };
    struct GZSettingsMapHost { // settings->m_30->m_8
        char _p0[0x48];
        GZPtrMap m_48; // +0x48
    };
    struct GZSettingsSub { // settings->m_30
        char _p0[0x8];
        GZSettingsMapHost* m_8; // +0x8
    };
    // The settings singleton at *0x64556c.
    struct GZMgrSettings {
        char _p0[0x30];
        GZSettingsSub* m_30; // +0x30
        char _p34[0x8c - 0x34];
        void* m_8c; // +0x8c
        void* m_90; // +0x90
        char _p94[0x15c - 0x94];
        void* m_15c; // +0x15c  monologo/death sprite key
    };
    struct GZGruntLevel {   // grunt->m_2dc
        void Fn2982(i32 n); // 0x2982
        void Fn33cd(i32 n); // 0x33cd
    };
    struct GZGrunt {         // from 0x355d
        void SetItem(i32 n); // 0x17a8
        i32 Flip();          // 0x1df2  (ambient variant index)
        void G3904(i32 n);   // 0x3904
        void G3792(i32 n);   // 0x3792
        void G3a85(i32 n);   // 0x3a85
        i32 CanQuickSave();  // 0x3da5
        char _p0[0x2dc];
        GZGruntLevel* m_2dc; // +0x2dc  active-level sub-object
        char _p2e0[0x3f4 - 0x2e0];
        i32* m_3f4; // +0x3f4  timer/state sub-object (i32[] the timer-cheat clears)
    };
    struct GZCell {
        char _p[0x1ec];
        i32 m_1ec; // +0x1ec class-id
        i32 m_1f0; // +0x1f0
        char _p2[0x1fc - 0x1f4];
        void* m_1fc;                                       // +0x1fc
        i32 LoadPickup(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x3c6a
        i32 LoadAbility(i32 n);                            // 0x3c29
    };
    struct GZSel { // grid->m_244->m_8 (selected cell x/y)
        i32 m_0;   // x
        i32 m_4;   // y
    };
    struct GZBoardSel {
        char _p0[8];
        GZSel* m_8; // +0x8
    };
    struct GZBoard {
        char _p0[0x1c];
        GZCell* m_cells[(0x244 - 0x1c) / 4]; // +0x1c: cell-ptr grid [x*15 + y]
        GZBoardSel* m_244;                   // +0x244
        char _p248[0x24c - 0x248];
        i32 m_24c;                    // +0x24c  select-valid flag
        void Board18e3(i32 n);        // 0x18e3
        void Board3616(i32 a, i32 b); // 0x3616
    };
    struct GZLevel { // GZLogic::m_2dc  level sub-object
        char _p0[0x550];
        i32 m_550; // +0x550
        i32 m_554; // +0x554
    };
    struct GZLogic {             // this->m_curState
        i32 vf10();              // vtbl+0x10  play-state accessor
        i32 vf54();              // vtbl+0x54
        void StopChain();        // 0x22d9
        void StartMusic();       // 0x286a
        void ChangeState(i32 n); // 0x201d  (on this actually) - unused here
        void Post36e8(i32 a);    // 0x36e8
        i32 DrawPresent();       // 0x326f
        char _p0[0x1c];
        i32 m_1c; // +0x1c  paused-select gate (PassClick source)
        char _p20[0x40 - 0x20];
        i32 m_40; // +0x40  state notify latch
        char _p44[0x2dc - 0x44];
        GZLevel* m_2dc; // +0x2dc  active level sub-object
        char _p2e0[0x4ec - 0x2e0];
        i32 m_4ec; // +0x4ec
        char _p4f0[0x4f8 - 0x4f0];
        i32 m_4f8; // +0x4f8
    };

    // The manager's +0x44 engine sub-object (only its +0x124 flag is touched here).
    struct GZM44 {
        char _p0[0x124];
        i32 m_124; // +0x124
    };
    // The last-save snapshot record at manager +0xbc.
    struct GZSaveInfo {
        char _p0[4];
        i32 m_4; // +0x04  quick-load target id
        char _p8[0x35 - 0x8];
        char m_35; // +0x35  name buffer (passed to Board2144)
        char _p36[0xf8 - 0x36];
        i32 m_f8; // +0xf8  save-valid flag
        i32 m_fc; // +0xfc  save-present flag
    };

    extern "C" CSndEmitter* __stdcall CueLookup(const char* name); // 0x2cca

    DATA(0x0061ab24)
    extern i32 g_sndCueTag;
    DATA(0x0061ab20)
    extern i32 g_61ab20;
    DATA(0x006455a4)
    extern i32 g_6455a4;
    DATA(0x006455a8)
    extern i32 g_6455a8;
    DATA(0x006455ac)
    extern i32 g_6455ac;
    DATA(0x006455b0)
    extern i32 g_6455b0;
    DATA(0x006455e8)
    extern i32 g_6455e8;
    DATA(0x006455ec)
    extern i32 g_6455ec;
    DATA(0x006455f4)
    extern i32 g_6455f4;
    DATA(0x006455f8)
    extern i32 g_6455f8;
    DATA(0x006bf3c0)
    extern i32 g_time6bf3c0;
    DATA(0x00644c54)
    extern i32 g_classId644c54;
    DATA(0x00648cf0)
    extern i32 g_isHost_648cf0;
    DATA(0x006c44c0)
    extern i32(__cdecl* g_pwsprintfA)(char*, const char*, ...);
    DATA(0x006c44c4)
    extern i32(WINAPI* g_ShowCursor)(i32); // ?g_ShowCursor@@3P6GHH@ZA
    DATA(0x006c44c8)
    extern i32(WINAPI* g_pPostMessageA)(void*, u32, u32, i32);

    // A short CString-like local (bodyless ctor/dtor) so the config command below
    // gets a destructible stack local -> forces the /GX EH frame like retail.
    struct GZStr {
        char* m_p;
        GZStr(const char* s);
        ~GZStr();
        void Empty();             // 0x1b9c69
        void operator=(GZStr& o); // 0x1b9e25
        i32 GetLength() {
            return ((i32*)m_p)[-2];
        } // inline CString::GetLength (header at m_p-8)
    };
    DATA(0x00645524)
    extern GZStr g_str645524; // brick-display CStrings cleared by 0x8068/0x806f
    DATA(0x00645528)
    extern GZStr g_str645528;

    struct GZWndSub {
        char _p0[4];
        void* m_4;
    };
    struct GZObj58; // defined below (m_saveSink's concrete type)
    struct CGruntzMgr {
        char _p0[0x4];
        GZWndSub* m_04; // 0x04
        char _p8[0xc - 0x8];
        i32 m_0c; // 0x0c
        i32 m_10; // 0x10
        i32 m_14; // 0x14
        char _p18[0x2c - 0x18];
        GZLogic* m_curState;           // 0x2c
        CSpriteFactoryHolder* m_world; // 0x30 (canonical: m_28 == CSndHost cue registry)
        char _p34[0x38 - 0x34];
        void* m_38; // 0x38
        char _p3c[0x44 - 0x3c];
        GZM44* m_44;            // 0x44 (engine sub-object; +0x124 flag)
        CGruntzSoundZ* m_sound; // 0x48 (PlayByName @0x138840)
        char _p4c[0x54 - 0x4c];
        CInput54* m_inputState; // 0x54 (world-present toolbar Method1/Method0)
        GZObj58* m_saveSink;    // 0x58 (has +0x18)
        char _p5c[0x68 - 0x5c];
        GZBoard* m_cmdGrid; // 0x68
        char _p6c[0xa0 - 0x6c];
        i32 m_lobbyProbed; // 0xa0
        char _pa4[0xbc - 0xa4];
        GZSaveInfo* m_saveInfoRec; // 0xbc
        char _pc0[0xc8 - 0xc0];
        GZStr m_strWorldFile; // 0xc8
        char _pcc[0x114 - 0xcc];
        i32 m_114; // 0x114
        char _p118[0x128 - 0x118];
        i32 m_128; // 0x128
        char _p12c[0x130 - 0x12c];
        i32 m_130; // 0x130
        i32 m_134; // 0x134

        GZGrunt* PickState();                         // 0x355d
        i32 PassClick(i32 a, i32 b, i32 c);           // 0x17c1
        void AppendChat(const char* s);               // 0x1b54
        void ToggleMsg(const char* s, i32 v);         // 0x4412
        void ReportErr(i32 a, i32 b);                 // 0x346d
        i32 Dispatch(i32 a, i32 b, i32 c, i32 d);     // 0x12d0
        i32 Base092f00();                             // 0x115e
        i32 InPlay2();                                // 0x2a27
        i32 InPlay();                                 // 0x1a46
        i32 CheckPlay();                              // 0x1bd6
        void ChangeState(i32 n);                      // 0x201d
        void FinishLevel(i32 f, i32 flag);            // 0x409d
        i32 GoNext();                                 // 0x2149
        void GoPrev();                                // 0x3260
        void RunModal(const char* s, void* p, i32 n); // 0x2bb7
        void RunLoadGame();                           // 0x29a0
        void LaunchUrl(const char* s);                // 0x235b
        void Func12ee();                              // 0x12ee
        void Func1a3c();                              // 0x1a3c
        void Func196a();                              // 0x196a
        void Func108c();                              // 0x108c
        void Func34ef(i32 n);                         // 0x34ef
        void Func424b();                              // 0x424b
        void Func22a2();                              // 0x22a2
        void Func3f62();                              // 0x3f62
        void Func415b();                              // 0x415b
        i32 Func1cf3();                               // 0x1cf3
        void Sound1388c0(i32 n);                      // 0x1388c0
        void Sound1388f0();                           // 0x1388f0
        i32 HandleCommand(i32 p1, i32 nID, i32 p3);
    };
    // free/cdecl helpers reached from a couple of first-switch bodies
    extern "C" i32 Board2144(CGruntzMgr* self, void* p);                                  // 0x2144
    extern "C" void Board277a(void* a, CGruntzMgr* self, void* b, void* c, i32 d, i32 e); // 0x277a
    DATA(0x0064556c)
    extern GZMgrSettings* g_mgrSettings;
    struct GZObj58 {
        char _p0[0x18];
        i32 m_18; // +0x18  paused-select gate (PassClick source)
        void Set58(i32 n);
        void Reset58();
    }; // 0x4408 / 0x3463

#define PLAYCUE(TAG)                                                                               \
    if (m_world->m_28->m_30 == 0) {                                                                \
        CSndEmitter* _c = CueLookup(TAG);                                                          \
        if (_c)                                                                                    \
            _c->Play(g_sndCueTag, 0, 0, 0);                                                        \
    }
// Cue via the world's own name->sound map (m_28+0x10 finder) with a stack out-ptr; used
// by a handful of cheats instead of the free CueLookup(0x2cca).
#define PLAYCUE_MAP(TAG)                                                                           \
    if (m_world->m_28->m_30 == 0) {                                                                \
        CSndEmitter* _c = 0;                                                                       \
        m_world->m_28->m_10.Lookup(TAG, &_c);                                                      \
        if (_c)                                                                                    \
            _c->Play(g_sndCueTag, 0, 0, 0);                                                        \
    }
#define ITEMCHEAT(N, MSG)                                                                          \
    {                                                                                              \
        GZGrunt* _g = PickState();                                                                 \
        if (!_g)                                                                                   \
            return 0;                                                                              \
        _g->SetItem(N);                                                                            \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChat(MSG);                                                                           \
        return 1;                                                                                  \
    }
#define WARP(N, ERR)                                                                               \
    {                                                                                              \
        m_134 = 1;                                                                                 \
        m_strWorldFile.Empty();                                                                    \
        if (!PassClick((N), 0, 1))                                                                 \
            ReportErr(0x8005, (ERR));                                                              \
        return 1;                                                                                  \
    }
// Grid-select the addressed cell (m_cmdGrid), grant a brick pickup (0x3c6a) and
// announce.  Retail inlines the whole grid walk (no BrickPickup helper call).
#define BRICKPICKUP(ID, MSG)                                                                       \
    {                                                                                              \
        if (!PickState())                                                                          \
            return 0;                                                                              \
        GZCell* _cell =                                                                            \
            m_cmdGrid->m_24c == 1                                                                  \
                ? m_cmdGrid->m_cells[m_cmdGrid->m_244->m_8->m_4 + m_cmdGrid->m_244->m_8->m_0 * 15] \
                : 0;                                                                               \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_1ec != g_classId644c54)                                                       \
            return 0;                                                                              \
        GZCell* _c2 = m_cmdGrid->m_cells[_cell->m_1f0 + _cell->m_1ec * 15];                        \
        i32 _r = (_c2 && _c2->m_1fc) ? _c2->LoadPickup(ID, 0, 0, 0, 1) : 0;                        \
        if (!_r)                                                                                   \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChat(MSG);                                                                           \
        return 1;                                                                                  \
    }
// Grid-select the addressed cell, grant an ability (0x3c29) and announce.
#define BRICKABILITY(N, MSG)                                                                       \
    {                                                                                              \
        if (!PickState())                                                                          \
            return 0;                                                                              \
        GZCell* _cell =                                                                            \
            m_cmdGrid->m_24c == 1                                                                  \
                ? m_cmdGrid->m_cells[m_cmdGrid->m_244->m_8->m_4 + m_cmdGrid->m_244->m_8->m_0 * 15] \
                : 0;                                                                               \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_1ec != g_classId644c54)                                                       \
            return 0;                                                                              \
        if (!_cell->LoadAbility(N))                                                                \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChat(MSG);                                                                           \
        return 1;                                                                                  \
    }
// Level-restart (0x8170/0x8171/0x8173): stop chain, drain cursor, change state,
// restart music, re-raise cursor.  Inlined by retail (not a helper call).
#define RESTART(N)                                                                                 \
    {                                                                                              \
        i32 st = m_curState->vf10();                                                               \
        GZLogic* mus = 0;                                                                          \
        if (st == 5) {                                                                             \
            mus = m_curState;                                                                      \
            m_curState->StopChain();                                                               \
            if (g_ShowCursor(0) >= 0)                                                              \
                while (g_ShowCursor(0) >= 0) {                                                     \
                }                                                                                  \
        }                                                                                          \
        ChangeState(N);                                                                            \
        if (mus) {                                                                                 \
            mus->StartMusic();                                                                     \
            if (g_ShowCursor(1) < 0)                                                               \
                while (g_ShowCursor(1) < 0) {                                                      \
                }                                                                                  \
        }                                                                                          \
        return 1;                                                                                  \
    }
// Simpler restart variant (0x8172): no cursor drain.
#define RESTART2(N)                                                                                \
    {                                                                                              \
        i32 st = m_curState->vf10();                                                               \
        GZLogic* mus = 0;                                                                          \
        if (st == 5) {                                                                             \
            mus = m_curState;                                                                      \
            m_curState->StopChain();                                                               \
        }                                                                                          \
        ChangeState(N);                                                                            \
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
    // (0x8244/0x8245) + throttled-Play Explosionz (0x8247), grunt->m_2dc cheats
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
                if (!PassClick(1, 0, 1)) {
                    ReportErr(0x8005, 0x41e);
                }
                return 1;
            case 0x807f:
                m_strWorldFile.Empty();
                m_134 = 1;
                if (!PassClick(p3, 0, 1)) {
                    ReportErr(0x8005, 0x41f);
                }
                return 1;
            case 0x8174:
                m_strWorldFile.Empty();
                m_134 = 1;
                if (!PassClick(m_saveSink ? m_saveSink->m_18 : 0, 0, 1)) {
                    ReportErr(0x8005, 0x41f);
                }
                return 1;
            case 0x80e3:
                m_134 = 3;
                if (!PassClick(1, 0, 1)) {
                    ReportErr(0x8005, 0x420);
                }
                return 1;
            case 0x80e1:
                Base092f00();
                // fall through to default
            default:
                if (m_curState->vf10() == 3) {
                    switch (nID & 0xffff) {
                        case 0x803b: {
                            if (m_world->m_28->m_30 == 0) {
                                CSndEmitter* _c = CueLookup("GAME_MINORCHEAT");
                                if (_c) {
                                    _c->Play(g_sndCueTag, 0, 0, 0);
                                }
                            }
                            AppendChat("Brian L. Goble is a programming God...");
                            return 1;
                        }
                        case 0x8043:
                            g_6455b0 ^= 1;
                            PLAYCUE("GAME_MINORCHEAT");
                            ToggleMsg("Traitor Mode", g_6455b0);
                            return 1;
                        case 0x804d:
                            g_6455f4 ^= 1;
                            PLAYCUE("GAME_MINORCHEAT");
                            ToggleMsg("Object Count Display", g_6455f4 & 1);
                            return 1;
                        case 0x804c:
                            g_6455f4 ^= 4;
                            PLAYCUE("GAME_MINORCHEAT");
                            ToggleMsg("World Position Display", g_6455f4 & 4);
                            return 1;
                        case 0x804b:
                            g_6455f4 ^= 0x10;
                            PLAYCUE("GAME_MINORCHEAT");
                            ToggleMsg("Frame Rate Display", g_6455f4 & 0x10);
                            return 1;
                        case 0x804e:
                            g_6455f4 ^= 0x20;
                            PLAYCUE("GAME_MINORCHEAT");
                            return 1;
                        case 0x8068:
                            g_6455f4 = (g_6455f4 ^ 0x40) & ~0x100;
                            g_str645524.Empty();
                            g_str645528.Empty();
                            PLAYCUE("GAME_MINORCHEAT");
                            return 1;
                        case 0x806f:
                            g_6455f4 = (g_6455f4 ^ 0x100) & ~0x40;
                            g_str645524.Empty();
                            PLAYCUE("GAME_MINORCHEAT");
                            return 1;
                        case 0x806e:
                            g_6455f4 ^= 0x80;
                            PLAYCUE("GAME_MINORCHEAT");
                            ToggleMsg("Elapsed Time Display", g_6455f4 & 0x80);
                            return 1;
                        case 0x8086: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 1;
                            }
                            if (!InPlay2()) {
                                return 1;
                            }
                            PLAYCUE("GAME_MONOLITH");
                            AppendChat("Monolith Rulez...");
                            if (!m_14) {
                                return 1;
                            }
                            if (g_6455e8) {
                                m_sound->PlayByName("MONOLITH", 1);
                                return 1;
                            }
                            char buf[128];
                            g_pwsprintfA(buf, "AMBIENT%d", _g->Flip());
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
                            if (!PickState()) {
                                return 0;
                            }
                            m_cmdGrid->Board18e3(5);
                            GZMgrSettings* _s = g_mgrSettings;
                            void* _key = _s->m_15c;
                            if (_key) {
                                GZDeathObj* _dr = 0;
                                if (_s->m_30->m_8->m_48.Lookup(_key, _dr) && _dr) {
                                    GZDeath* _d = _dr->m_7c->m_18;
                                    if (_d) {
                                        _d->ResolveDeathAnimation();
                                    }
                                }
                            }
                            PLAYCUE("GAME_MINORCHEAT");
                            AppendChat("Global thermal nuclear war!");
                            return 1;
                        }
                        case 0x8107: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            i32* _t = _g->m_3f4;
                            _t[0x10] = 0;
                            _t[0x11] = 0;
                            _t[0xc] = 0;
                            _t[0xd] = 0;
                            _t[0x12] = 0;
                            _t[0x13] = 0;
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("Ah, who needed that stupid timer anyway?");
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
                            BRICKPICKUP(
                                0x3c,
                                "This is gonna hurt them more than it will hurt you."
                            );
                        case 0x8134:
                            BRICKPICKUP(0x3b, "How did you swallow that?");
                        case 0x8135:
                            BRICKPICKUP(0x37, "There is no running allowed by the pool!");
                        case 0x8136:
                            if (!PickState()) {
                                return 0;
                            }
                            m_cmdGrid->Board3616(-1, 1);
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("How about a little color in your Gruntz?");
                            return 1;
                        case 0x8137: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            _g->G3904(1);
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("Whoah... you should get this monitor fixed.");
                            return 1;
                        }
                        case 0x8138: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            _g->G3792(1);
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("Is is dark in here?");
                            return 1;
                        }
                        case 0x8139: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            _g->G3a85(1);
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("Awww... isn't this little window cute?");
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
                            g_6455f4 ^= 0x400;
                            PLAYCUE("GAME_MINORCHEAT");
                            return 1;
                        case 0x8175:
                            if (m_world->m_28->m_30 == 0) {
                                CSndEmitter* _c = CueLookup("GAME_WAWA");
                                if (_c) {
                                    _c->Play(0x64, 0, 0, 0);
                                }
                            }
                            AppendChat("WA WA WA WA WA WA!");
                            return 1;
                        case 0x807a:
                        case 0x807b:
                        case 0x8246: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            _g->m_2dc->Fn2982(0x1387);
                            AppendChat(
                                "My name is Kevin Lambert.  You typed in my cheat "
                                "code.  Prepare to die."
                            );
                            return 1;
                        }
                        // ---- 4th sub-switch: warp / toggle cheats 0x81a3.. ----
                        case 0x81a3:
                            g_6455ac ^= 1;
                            PLAYCUE("GAME_MAJORCHEAT");
                            ToggleMsg("Goo puddlez", g_6455ac);
                            return 1;
                        case 0x81a4: {
                            GZGrunt* _g = PickState();
                            if (!_g) {
                                return 0;
                            }
                            if (!_g->m_2dc) {
                                return 0;
                            }
                            _g->m_2dc->Fn33cd(0x64);
                            PLAYCUE("GAME_MAJORCHEAT");
                            AppendChat("May your Wellz be full of Goo!");
                            return 1;
                        }
                        case 0x81a5:
                            g_6455a8 ^= 1;
                            PLAYCUE("GAME_MAJORCHEAT");
                            ToggleMsg("Grunt creation", g_6455a8);
                            return 1;
                        case 0x81a6:
                            g_6455a4 ^= 1;
                            PLAYCUE("GAME_MAJORCHEAT");
                            ToggleMsg("Grunt destruction", g_6455a4);
                            return 1;
                        case 0x81a9:
                            PLAYCUE("GAME_MAJORCHEAT");
                            if (m_saveSink) {
                                m_saveSink->Set58(0x20);
                                m_saveSink->Reset58();
                            }
                            AppendChat(
                                "They should call you Cheat Cheatelson from "
                                "Cheatstown Virginia who lives at 1105 Cheat Circle "
                                "just behind the CheatMart superstore."
                            );
                            return 1;
                        case 0x81d6:
                            RunModal("PSYCHE", (void*)0x402649, 0);
                            return 1;
                        case 0x81d7:
                            PLAYCUE("GAME_MAJORCHEAT");
                            m_44->m_124 = 0;
                            AppendChat("Cheatz cleared");
                            return 1;
                        case 0x8240:
                            PLAYCUE("GAME_MINORCHEAT");
                            AppendChat("Warp to Trouble in the Tropicz activated!");
                            m_saveSink->Set58(8);
                            return 1;
                        case 0x8241:
                            PLAYCUE("GAME_MINORCHEAT");
                            AppendChat("Warp to High on Sweetz activated!");
                            m_saveSink->Set58(0xc);
                            return 1;
                        case 0x8242:
                            PLAYCUE("GAME_MINORCHEAT");
                            AppendChat("Warp to High Rollerz activated!");
                            m_saveSink->Set58(0x10);
                            return 1;
                        case 0x8243:
                            PLAYCUE("GAME_MINORCHEAT");
                            AppendChat("Warp to Honey, I Shrunk the Gruntz activated!");
                            m_saveSink->Set58(0x14);
                            return 1;
                        case 0x8244:
                            PLAYCUE_MAP("GAME_MINORCHEAT");
                            AppendChat("Warp to The Miniature Masterz activated!");
                            m_saveSink->Set58(0x18);
                            return 1;
                        case 0x8245:
                            PLAYCUE_MAP("GAME_MINORCHEAT");
                            AppendChat("Warp to Gruntz in Space activated!");
                            m_saveSink->Set58(0x1c);
                            return 1;
                        case 0x8247: {
                            g_6455f8 ^= 1;
                            if (m_world->m_28->m_30 == 0) {
                                CSndEmitter* _c = 0;
                                m_world->m_28->m_10.Lookup("GAME_MAJORCHEAT", &_c);
                                if (_c && g_61ab20) {
                                    i32 now = g_time6bf3c0;
                                    if ((u32)(now - _c->m_14) >= (u32)_c->m_18) {
                                        _c->m_14 = now;
                                        _c->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                                    }
                                }
                            }
                            ToggleMsg("Explosionz", g_6455f8);
                            return 1;
                        }
                    }
                }
                return 0;
            // ---- remaining UI command bodies (physically after the epilogue) ----
            case 0x807e: {
                GZSaveInfo* si = m_saveInfoRec;
                if (!si) {
                    return 1;
                }
                if (!(*(u8*)si & 1)) {
                    return 1;
                }
                m_114 = 1;
                GZStr tmp((const char*)si + 0x75);
                m_strWorldFile = tmp;
                (void)p1;
                if (tmp.GetLength()) {
                    if (si->m_fc) {
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
                if (!PassClick(si->m_4, 0, 1)) {
                    ReportErr(0x8005, 0x421);
                }
                if (!Board2144(this, &si->m_35)) {
                    ReportErr(0x8005, 0x465);
                }
                Func12ee();
                m_114 = 0;
                return 1;
            }
            case 0x80b8:
                return 1;
            case 0x80d7:
                if (m_curState && m_curState->vf10() == 0x11) {
                    m_curState->Post36e8(p3);
                }
                return 1;
            case 0x80ce:
                if (m_curState->vf10() == 3 || m_curState->vf10() == 5) {
                    if (!g_6455ec) {
                        RunLoadGame();
                    }
                }
                return 1;
            case 0x80cf:
                if (m_curState->vf10() == 3) {
                    GZGrunt* _g = PickState();
                    if (((GZGrunt*)_g)->CanQuickSave()) {
                        Func1a3c();
                    }
                }
                return 1;
            case 0x80d8:
                if (m_curState->vf10() == 3) {
                    GZGrunt* _g = PickState();
                    if (((GZGrunt*)_g)->CanQuickSave()) {
                        Func196a();
                    }
                }
                return 1;
            case 0x80d9:
                if (m_curState->vf10() == 3 || m_curState->vf10() == 5) {
                    if (!g_6455ec) {
                        Func108c();
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
                if (m_curState->vf10() == 5 || m_curState->vf10() == 2) {
                    while (g_ShowCursor(1) < 0) {
                    }
                    LaunchUrl("http://www.gruntzgoo.com/");
                }
                return 1;
            case 0x80d2:
                m_134 = 2;
                g_isHost_648cf0 = 0;
                if (Dispatch(0x11, 1, 0, 0)) {
                    return 1;
                }
                if (Dispatch(2, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x424);
                return 1;
            case 0x80d3:
                m_134 = 2;
                g_isHost_648cf0 = 1;
                if (Dispatch(0x11, 1, 0, 0)) {
                    return 1;
                }
                if (Dispatch(2, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x425);
                return 1;
            case 0x8023:
                if (Dispatch(5, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x426);
                return 1;
            case 0x8080:
                if (Dispatch(0xb, 1, 1, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x427);
                return 1;
            case 0x8090:
                if (Dispatch(0xd, 1, 1, p3)) {
                    return 1;
                }
                ReportErr(0x8005, 0x428);
                return 1;
            case 0x8036:
                if (Func1cf3()) {
                    return 1;
                }
                ReportErr(0x8005, 0x429);
                return 1;
            case 0x8021:
                if (Dispatch(8, 1, 0, 0)) {
                    return 1;
                }
                if (Dispatch(5, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x42a);
                return 1;
            case 0x8027:
                if (Dispatch(2, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x42b);
                return 1;
            case 0x8029:
                if (!Dispatch(2, 1, 0, 0)) {
                    ReportErr(0x8005, 0x42c);
                    return 1;
                }
                g_pPostMessageA(m_04->m_4, 0x111, 0x8023, 0);
                return 1;
            case 0x80ab:
                if (Dispatch(0xe, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x42d);
                return 1;
            case 0x8022:
                if (Dispatch(7, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x42e);
                return 1;
            case 0x8007: { // 0x89b97
                i32 st = m_curState->vf10();
                if (st == 3 || st == 0x11) {
                    if (m_curState->m_4f8) {
                        return 1;
                    }
                    if (m_curState->m_4ec) {
                        return 1;
                    }
                    GZLevel* lv = m_curState->m_2dc;
                    if (lv) {
                        if (lv->m_550) {
                            return 1;
                        }
                        if (lv->m_554) {
                            return 1;
                        }
                    }
                    i32 f = m_0c ^ 1;
                    m_0c = f;
                    FinishLevel(f, 1);
                }
                return 1;
            }
            case 0x816e: { // 0x89c19
                i32 st = m_curState->vf10();
                if (st == 3 || st == 0x11) {
                    i32 f = m_0c ^ 1;
                    m_0c = f;
                    FinishLevel(f, 0);
                }
                return 1;
            }
            case 0x8084:
                if (!CheckPlay()) {
                    return 1;
                }
                if (m_curState->DrawPresent()) {
                    return 1;
                }
                ReportErr(0x8005, 0x42f);
                return 1;
            case 0x80b7:
                m_lobbyProbed = 0;
                g_pPostMessageA(m_04->m_4, 0x111, 0x8025, 0);
                return 1;
            case 0x800e: // 0x89c92
                if (!CheckPlay()) {
                    return 1;
                }
                if (m_curState->vf54()) {
                    return 1;
                }
                if (Dispatch(2, 1, 0, 0)) {
                    g_pPostMessageA(m_04->m_4, 0x111, 0x8023, 0);
                    return 1;
                }
                ReportErr(0x8005, 0x430);
                return 1;
            case 0x8042: // 0x89d00
                if (g_6455ec) {
                    return 1;
                }
                Func415b();
                return 1;
            case 0x8075: // 0x89d1e
                if (GoNext()) {
                    return 1;
                }
                ReportErr(0x8007, 0x431);
                return 1;
            case 0x800f: // 0x89d37 -> falls into 0x8006
                if (m_curState->vf10() == 3 || m_curState->vf10() == 0x11) {
                    GoPrev();
                    return 1;
                }
                // fall through
            case 0x8006: // 0x89d62
                m_curState->m_40 = 1;
                if (Dispatch(5, 1, 0, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x432);
                return 1;
            case 0x8008: // 0x89d8d
                Func3f62();
                return 1;
            case 0x8035: { // 0x89d9e
                i32 st = m_curState->vf10();
                if (st == 9 || st == 0xd || st == 0xf || st == 0xe || st == 8 || st == 0xa
                    || st == 0x12 || st == 0x11) {
                    return 1;
                }
                if (Dispatch(9, 1, 1, 0)) {
                    return 1;
                }
                ReportErr(0x8005, 0x433);
                return 1;
            }
            case 0x80e2: { // 0x89e58  CONFIG_SETTINGS modal
                i32 st = m_curState->vf10();
                GZLogic* mus = 0;
                if (st == 5) {
                    mus = m_curState;
                    m_curState->StopChain();
                }
                RunModal("CONFIG_SETTINGS", (void*)0x403ae4, 0);
                if (mus) {
                    mus->StartMusic();
                }
                return 1;
            }
            case 0x800a: { // 0x89e9f  elapsed-time / sound toggle
                if (m_0c) {
                    return 1;
                }
                i32 v = m_14 ^ 1;
                m_14 = v;
                i32 pl = CheckPlay();
                if (!pl) {
                    i32 st = m_curState->vf10();
                    if (st != 0xb && st != 5) {
                        return 1;
                    }
                }
                if (v) {
                    Sound1388c0(1);
                } else {
                    Sound1388f0();
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
                i32 v = m_10 ^ 1;
                m_10 = v;
                g_61ab20 = v;
                if (v == 0) {
                    m_inputState->Disarm(); // 0x29b9->0xbc80 (== CInput54::Disarm, ghidra "Stop")
                } else {
                    m_inputState->Arm(); // 0x18e8->0xbcf0 (== CInput54::Arm, ghidra "Resume")
                }
                return 1;
            }
            case 0x802c: // 0x89f5a
                if (!InPlay()) {
                    return 1;
                }
                Func34ef(0);
                return 1;
            case 0x802a: // 0x89f7c
                if (!InPlay()) {
                    return 1;
                }
                Func424b();
                return 1;
            case 0x802b: // 0x89f9c
                if (!InPlay()) {
                    return 1;
                }
                Func22a2();
                return 1;
            case 0x8070: { // 0x89fbc  world-present toolbar
                Board277a(m_38, this, g_mgrSettings->m_8c, g_mgrSettings->m_90, 0, 0);
                return 1;
            }
            case 0x806b: {
                GZGrunt* _g = PickState();
                if (!_g) {
                    return 1;
                }
                m_strWorldFile = m_strWorldFile; // 0x1b9e25 op=
                if (!PassClick(m_curState->m_1c, 0, 1)) {
                    ReportErr(0x8007, 0x434);
                }
                return 1;
            }
        }
        return 0;
    }
#undef PLAYCUE
#undef ITEMCHEAT
#undef WARP
} // namespace GruntzMgrCmd
