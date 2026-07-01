// GruntzMgr.h - CGruntzMgr, the Gruntz game manager (C:\Proj\Gruntz). It is the
// REAL derived game manager: `CGruntzMgr : public WAP32::CGameMgr`, 0xa30 bytes,
// with its own vftable (??_7CGruntzMgr@@6B@ @0x5e9b64). CGruntzApp::Initialize-
// GameManager (@0x080a20) does `new CGruntzMgr` => operator new(0xa30) + the
// CGruntzMgr ctor.  The base WAP32::CGameMgr is the genuine 0x2c engine class;
// all the 0xa30 of per-game state lives HERE.
//
// Only the offsets the matched methods touch are load-bearing:
//   +0xc8 CString, +0xd0 CD drive-letter cache (char) / +0xd4 probed flag,
//   +0xd8 CByteArray, +0xec/+0xf0 CString, +0x150 a 0x238-byte options object
//   (ctor/dtor are out-of-line NAFXCW-style FUN_0051f5a0/FUN_0051f640 calls,
//   reloc-masked). The member subobjects' destructible nature is what gives the
//   ctor/dtor their /GX C++ EH frame.
#ifndef GRUNTZ_GRUNTZ_GRUNTZMGR_H
#define GRUNTZ_GRUNTZ_GRUNTZMGR_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include <Wap32/Wap32.h>
#include <Gruntz/CString.h>
#include <Gruntz/GameLevel.h> // CByteArray
#include <Gruntz/CState.h>    // CState (m_curState game-state; Update() at slot 4)

// The 0x238-byte options/registry-backed sub-object embedded at +0x150. Its
// ctor (FUN_0051f5a0) takes (this, 0x238, 4, &thunk_FUN_00431250, &LoadOptions)
// and its dtor (FUN_0051f640) takes (this, 0x238, 4, &LoadOptions); both are
// out-of-line so the calls reloc-mask. Only the size (0x238) + that it is a
// destructible member (EH state 4) are load-bearing.
struct CGruntzMgrOptions {
    CGruntzMgrOptions();
    ~CGruntzMgrOptions();
    char m_pad[0x238];
};

// Minimal sound/DirectSound-ish object held at CGruntzMgr +0x48. Its two
// reloc-masked __thiscall helpers (this in ecx) only need the right call shape;
// the inner object at +0x1c carries the per-frame busy poll.
struct CGruntzSoundInnerZ {
    char m_pad0[0x48];
    i32 m_48;     // +0x48  per-bank restart flag
    i32 IsBusy(); // FUN_00538f60 (this) -> ret (busy state-id 4/0x10)
};

// A typed VIEW of the state-stack array at CGruntzMgr +0xd8. The member itself
// stays a destructible CByteArray (so the ctor/dtor's EH-state numbering is
// unchanged); the accessor methods reinterpret &m_stateStack as this CObArray-shaped
// view (CObject vptr at +0x00, m_pData at +0x04, m_nSize at +0x08, ...). The
// stored elements are CState* (the pushed game states) whose Update() (slot 4,
// +0x10) reports the state id; SetSize/SetAtGrow/RemoveAt are the out-of-line
// NAFXCW helpers (reloc-masked).
struct CStateStackZ {
    void* m_vptr;                        // +0x00 CObject vptr
    CState** m_pData;                    // +0x04 element store
    i32 m_nSize;                         // +0x08 live count
    i32 m_nMaxSize;                      // +0x0c
    i32 m_nGrowBy;                       // +0x10
    void SetSize(i32 n, i32 growBy);     // @0x5b4f75  (clear via SetSize(0, -1))
    void SetAtGrow(i32 i, CState* elem); // @0x5b5144
    void RemoveAt(i32 i, i32 n);         // @0x5b5200
};
struct CGruntzSoundZ {
    void StopBank(i32 flag);    // FUN_00538900 (this, 1)  -> ret 4
    void StopAll();             // FUN_005388f0 (this)
    void StopBank2();           // FUN_00538920 (this)     -> ret 4 (busy-driven stop)
    i32 Restart_1388c0(i32 a1); // FUN_005388c0 (this, 1) re-launch current bank
    i32 GetMusicVolume();       // FUN_005389c0 (this) -> current music volume (UnknownClose save)
    char m_pad0[0x1c];          // +0x00..+0x1c
    CGruntzSoundInnerZ* m_1c;   // +0x1c  inner object IsBusy/StopAll deref / m_pCurrent
};

// The level/world object held at CGruntzMgr +0x30 (the loaded map + its active
// CWorld view). Reached as `m_world->...`; every method is reloc-masked. +0x24 is
// the active world view (the scroll/camera holder the FP scaler reads); +0x28 a
// sub-controller whose +0x2c object carries a teardown thiscall; +0x520 a 4-slot
// status array the paused-state poll walks (status id at each slot's +0x20).
struct CWorldSub2c {
    void Teardown(); // FUN_00537a80 (this)
};
struct CWorldSub28 {
    char m_pad[0x2c];
    CWorldSub2c* m_2c; // +0x2c
};
struct CWorldView;
// A polymorphic sub-object held in the world at +0x1c, dispatched through a
// pointer-to-pointer (`*m_1c`) then vtbl slot 10 (+0x28).
struct CWorldDispatch {
    struct Vtbl {
        void* s0[10];
        void(__stdcall* Slot0a)(CWorldDispatch*); // +0x28
    }* vtbl;
};
// The world's +0x4 sub-object exposes a map-index field at +0x14.
struct CWorldSub4 {
    char m_pad0[0x14];
    i32 m_14; // +0x14
};
struct CWorldZ {
    char m_pad0[0x4];
    CWorldSub4* m_4; // +0x04
    char m_pad8[0x1c - 0x8];
    CWorldDispatch** m_1c; // +0x1c
    char m_pad20[0x24 - 0x20];
    CWorldView* m_24;  // +0x24  active world view
    CWorldSub28* m_28; // +0x28
};

// Minimal IDirectPlayLobby-shaped COM surface used by
// InitializeLobbyConnectionSettings: only the two vtable slots it calls are
// needed - Release (slot 2, +0x8) and GetConnectionSettings (slot 8, +0x20,
// called twice in the size-probe / fill-buffer idiom). The call rel32/IAT
// displacements reloc-mask; only the slot offsets + arg shapes are load-bearing.
struct IDirectPlayLobbyZ {
    struct Vtbl {
        void *slot0, *slot1;
        i32(__stdcall* Release)(IDirectPlayLobbyZ*); // +0x08
        void *slot3, *slot4, *slot5, *slot6, *slot7;
        i32(__stdcall* GetConnectionSettings)(
            IDirectPlayLobbyZ*,
            u32 appId,
            void* lpData,
            u32* lpdwSize
        ); // +0x20
    }* vtbl;
};

class CGruntzMgr : public WAP32::CGameMgr {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE; // vtbl slot 0 (own vftable 0x5e9b64)
    // The ??_G scalar-deleting destructor (vtable slot 0 entry the retail vtable
    // holds): run the dtor body, then operator delete when the low flag bit is set;
    // returns this. Modeled by hand (MSVC's own ??_G mangling differs from the retail
    // label the delinker emits, so this carries the RVA).
    void* ScalarDeletingDtor(u32 flags); // @0x083330

    // Manager-owned methods reconstructed in GruntzMgr.cpp.
    void UnknownClose() OVERRIDE; // @0x0855e0 (member teardown)
    void AccrueScoreTime();       // @0x0861e0 (per-state HUD time/score accrual + state push)
    void OnCheckpointReached();   // @0x08e6c0 (checkpoint modal -> WM_COMMAND 0x80cf)
    void DelayedQuit();           // @0x08f530 (menu-activate delay spin -> WM_CLOSE)
    i32 SaveGameAs();             // @0x092f00 (save-as name dialog -> WM_COMMAND 0x80e3)
    void ReportError(WPARAM wParam, LPARAM lParam); // @0x08dc60  -> m_8->vtbl[0x1c]
    char GetGruntzDriveLetter();                    // @0x08fa70  (memoised CD letter)
    i32 IsInPlayState();                            // @0x08fa40  (m_curState && CheckPlayState())
    // @0x08f340 (/GX): when the live state is playable (Update() in {5,2,3,7}),
    // capture the current world-file name from the game window into m_strWorldFile,
    // clear the score/state slots (m_128/m_12c), and PostMessageA WM_COMMAND 0x8005.
    i32 CaptureWorldFile();
    i32 InitializeLobbyConnectionSettings();   // @0x08eca0 (DirectPlay lobby connect)
    CString BuildMoviePath(i32 movie);         // @0x08ff30 (per-movie path on the CD)
    void PerFrameTick();                       // @0x08f620 (per-frame draw-clock tick)
    void AdvanceFrame(i32 doDraw, i32 unused); // @0x08f6a0 (the per-frame advance gate)
    i32 CheckPlayState();                      // @0x08ec50 (m_curState->Update()==3||==0x11)
    i32 RestoreVideoMode(i32 save);            // @0x08ddd0 (re-assert 640x480; save on hit)
    i32 SetVideoMode(i32 w, i32 h, i32 flag);  // @0x08df00 (mode-switch the display; stubbed)
    // Two play-state cursor/resolution guards: resolve a screen point via the
    // world coord-resolver (m_world->m_1c) and, if it falls off the expected
    // field, re-assert 640x480 + ReportError. A = upper-bound (x>0x514) variant,
    // B = lower-bound (x<0x140 || y<0xc8) variant.
    i32 CheckDisplayBoundsA(); // @0x08e1d0
    i32 CheckDisplayBoundsB(); // @0x08e2b0

    // Per-state notification forwarders: dispatch (a,b[,c]) into the live state's
    // vtable slot 11..20, returning 0 with no state (0x8d9d0..0x8dbe0).
    i32 NotifyState0b(i32 a, i32 b);        // @0x08d9d0 -> m_curState slot 11 (+0x2c)
    i32 NotifyState0c(i32 a, i32 b);        // @0x08da00 -> m_curState slot 12 (+0x30)
    i32 NotifyState0d(i32 a, i32 b);        // @0x08da30 -> m_curState slot 13 (+0x34)
    i32 NotifyState0e(i32 a, i32 b, i32 c); // @0x08da60 -> m_curState slot 14 (+0x38)
    i32 NotifyState0f(i32 a, i32 b, i32 c); // @0x08daa0 -> m_curState slot 15 (+0x3c)
    i32 NotifyState10(i32 a, i32 b, i32 c); // @0x08dae0 -> m_curState slot 16 (+0x40)
    i32 NotifyState11(i32 a, i32 b, i32 c); // @0x08db20 -> m_curState slot 17 (+0x44)
    i32 NotifyState12(i32 a, i32 b, i32 c); // @0x08db60 -> m_curState slot 18 (+0x48)
    i32 NotifyState13(i32 a, i32 b, i32 c); // @0x08dba0 -> m_curState slot 19 (+0x4c)
    i32 NotifyState14(i32 a, i32 b, i32 c); // @0x08dbe0 -> m_curState slot 20 (+0x50)

    // State-stack accessors (the CState* array at +0xd8).
    CState* TopState();             // @0x090980 (last pushed state)
    void PushState(CState* s);      // @0x0909b0 (SetAtGrow append)
    i32 PopTopIfMatches(CState* s); // @0x0909e0 (RemoveAt last; old top == s)
    void ClearStateStack();         // @0x090a50 (delete all, SetSize(0,-1))
    i32 CheckMovieFileExists();     // @0x090aa0 (FileExists(m_strMoviePath))
    CState* FindStateById(i32 id);  // @0x092900 (live + stack search by Update id)

    // SetVideoMode (0x08df00) reloc-masked CGruntzMgr siblings (all thiscall):
    void Step1db6();                 // @0x1db6 thunk  post-mode resync
    void Step3d23();                 // @0x3d23 thunk  post-mode resync
    void ReportMapTooSmall(char* s); // @0x417e thunk  surface the modal text
    void LogLine(char* s);           // @0x1b54 thunk  log the resolution line

    // Level cycle + debug layer toggles (proximity-attributed, reconstructed).
    i32 GoToNextLevel();              // @0x08d850 (PLAY: advance current level index, wrap)
    i32 GoToPrevLevel();              // @0x08d910 (PLAY: retreat current level index, wrap)
    i32 ToggleObjectLayer();          // @0x08efe0 (toggle the "current" object layer visible bit)
    i32 ToggleHeightLayer();          // @0x08f060 (toggle the m_5c sub-layer visible bit)
    i32 ToggleBaseLayer();            // @0x08f0b0 (toggle layer[0]'s visible bit)
    i32 PollUnlessIdle();             // @0x08f2f0 (CheckPlayState() unless state idle (5); ret 0)
    i32 AppendChatMessage(char* msg); // @0x08f9c0 (-> m_5c chat-log insert)
    i32 ShowToggleMessage(char* itemName, i32 on); // @0x08f9f0 ("<item> is ON/OFF")

    i32 IsMoviePathValid();        // @0x0901d0 (bool-normalized FileExists(m_strMoviePath))
    void ReportWorldStatus(i32 a); // @0x090ac0 (map m_world->m_38 status to ReportError)
    i32 LoadMonologoSprite();      // @0x090d10 (PLAY-only: find/toggle/create the MONOLITH logo)
    i32 SetGruntColor(i32 sink, i32 key, i32 idx); // @0x0910d0 (recolor a sink cell)
    i32 SetColorDepth(i32 depth);    // @0x091170 (set the packed g_severusCounterB color by depth)
    i32 LoadWorldMode(i32 mode);     // @0x091a40 (switch the world video/color mode + reload)
    i32 ResetWorldState(i32 notify); // @0x091e20 (idle/exit-prep the world, reset cursor)
    void StopBankIfActive();         // @0x092000 (if m_sound && m_14: m_sound->StopAll())
    void StopBank0IfActive();        // @0x092030 (if m_sound && m_14: m_sound->StopBank(0))

    i32 StoreInputState(i32 v); // @0x091a10 (store m_inputStateVal, forward to m_timer)
    void StoreInputFlag(i32 v); // @0x0919d0 (store m_inputFlag, mirror to g_61ab24 + m_inputState)
    void UnloadSoundChain();    // @0x08f740 (m_world->m_28->m_2c teardown + StopBank2)
    void ClearOptionsSlots();   // @0x092ec0 (zero the 4 options slots' +0x20/+0x24)
    CString GetWorldFileName(); // @0x0928c0 (return a copy of m_strWorldFile)
    i32 AdvanceOptionsCycle();  // @0x0933e0 (round-robin tick of the options slots)
    i32 SyncOptionsState();     // @0x093170 (reload each options slot's config; dual-slot)
    void SetCellHeight(i32 r, i32 c, i32 v);          // @0x111ec0 (write the world height grid)
    i32 PassClickToPlayState(i32 a0, i32 a1, i32 a2); // @0x08d780
    i32 SwitchToNextState();                          // @0x08d6a0
    // @0x08b960: the /GX state-machine factory - tear down the current state
    // (push it or scalar-delete + drain the stack), then build the new game-state
    // object for `stateId` (switch/new + ctor + vtable), install it, run its
    // slot-1 activate; ret 1 (0 on new/activate failure). Lives in an eh sibling TU.
    i32 TransitionState(i32 stateId, i32 a2, i32 keepCurrent, i32 a4);
    void FlushStateStack();     // @0x090a50 (scalar-delete + drain the pushed state stack)
    void EnterModalUI(i32 arg); // @0x08ef10
    i32 ExitModalUI(class CModalDialog* dlg, i32 notify);   // @0x0903f0
    i32 FinishLevel(i32 full, i32 stopBank);                // @0x08e980
    i32 FillSaveInfo(struct SaveInfo* dst, void* snapshot); // @0x0927b0
    i32 SaveState(class CSerializerZ* ar);                  // @0x093620
    i32 LoadState(class CSerializerZ* ar);                  // @0x093920 (deserialize counterpart)
    void UpdateScoreHud();                                  // @0x0860b0
    i32 BroadcastCmd(i32 a0, i32 cmd, i32 a2, i32 a3);      // @0x093460
    void RecomputeViewScale();                              // @0x08f7f0
    i32 PrepCmd4(i32 a0);                                   // reloc-masked sibling (cmd-4 arm gate)
    i32 PrepCmd7(i32 a0);                                   // reloc-masked sibling (cmd-7 arm gate)
    struct CActiveObj* GetActiveObj(); // reloc-masked sibling (active game object)
    void RunWinHook();                 // reloc-masked sibling (win/level-complete hook)
    i32 CheckLevelActive();            // reloc-masked sibling (level-active predicate)
    void* GetSaveSource();             // reloc-masked sibling (live source-state ptr)
    CString GetLevelName();            // reloc-masked sibling (current level name)
    // A sibling state-transition pusher reached by PassClickToPlayState's reloc-
    // masked 4-arg call (deferred body / matched elsewhere).
    i32 ChangeToPlayState(i32 a, i32 b, i32 c, i32 d);
    // Reloc-masked CGruntzMgr siblings reached from SwitchToNextState.
    CState* MakeNextState();       // build/find the next state to switch to
    void ActivateState(CState* s); // install + activate the new live state
    void PostSwitchHook();         // post-switch app hook

    // Reloc-masked CGruntzMgr sibling reached from ResetWorldState (the post-mode-
    // reload state transition pusher @0x08b960).
    void SwitchModeState(i32 a, i32 b, i32 c, i32 d);

    // Reloc-masked CGruntzMgr siblings reached from LoadWorldMode (the rez-path
    // builder + the rez-row resolver that fills a CString and returns the row ptr).
    i32 MakeRezPath();                // FUN_0049???? (this) -> path-built ok
    i32* ResolveRezRow(CString* out); // thunk_FUN_00485500 (this, &out) -> row

    // Clock / global helpers.
    void SetGameClock(i32 now, i32 delta, i32 abs); // @0x08f7b0 (mirror the 5 clock globals)
    void ResetClockGlobals();                       // @0x08f4f0 (zero the 7 clock globals)
    i32 TickStateMgrs();                            // @0x0920b0 (drive two engine singletons)
    void SetRunState(i32 v);           // @0x092340 (set base m_10 run-state + side-effects)
    i32 CheckSavedMode();              // @0x08de70 (saved==live mode test)
    i32 IsLobbyHostReady();            // @0x091500 (m_curState/m_8/m_modalBusy null-chain)
    i32 RunFromState();                // @0x090200 (thin wrapper -> ChangeState_8fab0(1))
    CState* PickPlayOrPausedState();   // @0x092990 (FindStateById(3))
    CState* PickPausedThenPlayState(); // @0x0929b0 (FindStateById(0x11)|| (3))

    // The modal-dialog runner sibling (a __thiscall (template, dlgProc, flag) ->
    // i32; reloc-masked). The leading Ghidra label winapi_090260_DialogBoxParamA
    // is the engine's DialogBoxParamA wrapper.
    i32 RunModalDialog(const char* tmpl, void* dlgProc, i32 flag); // @0x090260

    // Sound/level-loaded sync (@0x0923b0): set the base level-loaded flag (m_14)
    // and, when it changes and a sound bank is bound, drive the bank.
    void SetSoundLevelState(i32 loaded);
    i32 RunLoadGameDialog();       // @0x092500 (GAME_LOAD dialog; ret 1)
    i32 Quicksave();               // @0x092530 (quicksave the game; /GX)
    i32 RunDebugGruntTypeDialog(); // @0x0929e0 (DEBUG_GRUNTTYPE dialog when PLAY)
    // @0x092d50 - if in play state and the options slot is not yet loaded, assign
    // its name CString. 7 raw args (only the slot index + the value string used).
    i32 LoadOptionsSlotName(i32 slot, i32 a2, i32 a3, i32 a4, i32 a5, const char* val, i32 a7);
    i32 CountReadyOptionsSlots(i32 anyState);   // @0x092e30 (count loaded/armed slots)
    struct OptionsSlot* FindOptionsSlot(i32 x); // @0x092e80 (slot whose m_18 == x)
    // Sibling reached by Quicksave (reloc-masked): plays the save-feedback sprite.
    i32 LoadSaveMessageSprite();

    // @0x093be0 (/GX) - "is this a Battlez map file" predicate: open `path`, and if
    // it has at least a full 0x5f4-byte WWD header, read it and test whether
    // "Battlez" appears past the 0x10-byte magic. `this` is unused; the path arg is
    // taken by value (callee destroys it). ret 4.
    i32 IsBattlezMapFile(CString path);

    // Larger sibling reached by RunFromState's reloc-masked call; body still a
    // @stub in GruntzMgr.cpp (migrated from src/Stub/Discovered.cpp) so the call
    // binds to a CGruntzMgr symbol at 0x08fab0.
    i32 ChangeState_8fab0(i32 arg); // @0x08fab0 (deferred / stubbed)

    // --- members (offsets relative to `this`; base CGameMgr occupies 0x00..0x2c) ---
    CState* m_curState;               // +0x2c  current game-state (Update() -> state id)
    CWorldZ* m_world;                 // +0x30  loaded world/map object (also a draw gate)
    i32 m_34, m_38, m_3c, m_40, m_44; // +0x34..+0x44  (+0x44 -> HudGuard44 first-frame guard)
    CGruntzSoundZ* m_sound;           // +0x48  sound/bank object (StopBank/StopAll)
    i32 m_4c, m_50;                   // +0x4c, +0x50
    // +0x54..+0x78 sub-controllers (4-byte object pointers; each is reached only
    // through a reloc-masked thiscall on a TU-local view, so kept i32-wide):
    i32 m_inputState;     // +0x54  input/state object (Method0/Method1/StoreFlag)
    i32 m_saveSink;       // +0x58  save-record sink (SaveSink58::Store)
    i32 m_5c;             // +0x5c
    i32 m_timer;          // +0x60  per-frame timer/poll controller (Stop/Tick; +0x2c mirror)
    i32 m_64;             // +0x64
    i32 m_cmdGrid;        // +0x68  world delta-table grid + command sink (Reset/Flush)
    i32 m_cmdSubMgr;      // +0x6c  command sub-manager sink
    i32 m_cmdNotify;      // +0x70  command sink (vtbl slot 1) + cell-height notify
    i32 m_74, m_78;       // +0x74, +0x78
    i32 m_scoreHud;       // +0x7c  HUD/score accumulator + command sink
    i32 m_numRuns;        // +0x80  "Num_Runs"   (launch counter; UnknownClose WriteInt)
    i32 m_numMovies;      // +0x84  "Num_Movies" (movie-playback counter)
    i32 m_colorDepth;     // +0x88  live color depth (bpp): 8/16(=HiColor)/24 (=0x10 in ctor)
    i32 m_modeW, m_modeH; // +0x8c, +0x90  live video mode (w, h)
    i32 m_savedModeW, m_savedModeH; // +0x94, +0x98  saved/last-good mode (w, h)
    i32 m_lobbyResult;              // +0x9c  lobby-connect success flag (1/0)
    i32 m_lobbyProbed;              // +0xa0  one-shot lobby-connect guard
    i32 m_a4, m_a8;                 // +0xa4, +0xa8
    i32 m_modalBusy;                // +0xac  modal-UI/cursor-busy gate
    i32 m_b0, m_b4;                 // +0xb0, +0xb4
    i32 m_isCheckpointPrompts;      // +0xb8  "Checkpoint_Prompts" enable (=1 in ctor)
    i32 m_saveInfoRec;              // +0xbc  last FillSaveInfo dst record
    IDirectPlayLobbyZ* m_lobby;     // +0xc0  the lobby interface (Released/recreated)
    void* m_connSettings;           // +0xc4  the launch connection-settings buffer
    CString m_strWorldFile;         // +0xc8  world file name (EH state 0)
    i32 m_cc;                       // +0xcc  (=0x1e in ctor)
    char m_driveLetter;             // +0xd0  cached CD drive letter
    char m_padD1[3];                // +0xd1
    i32 m_driveLetterProbed;        // +0xd4  drive-letter probed flag
    CByteArray m_stateStack;        // +0xd8  CState* push-down stack (0x14 bytes; EH state 1)
    CString m_strEC;                // +0xec  (EH state 2)
    CString m_strMoviePath;         // +0xf0  resolved movie path (EH state 3)
    i32 m_f4;                       // +0xf4  (=1 in ctor)
    i32 m_f8, m_fc;                 // +0xf8, +0xfc
    i32 m_isVoiceEnabled;           // +0x100  "Voice"      enable (=1 in ctor)
    i32 m_isAmbientEnabled;         // +0x104  "Ambient"    enable (=1 in ctor)
    i32 m_isInterlaced;             // +0x108  "Interlaced" flag
    i32 m_isHighDetail;             // +0x10c  "High_Detail" flag (=1 in ctor)
    i32 m_isEffectsEnabled;         // +0x110  "Effects"    enable (=1 in ctor)
    i32 m_114;                      // +0x114
    i32 m_isEasyMode;               // +0x118  "Easy_Mode" flag
    i32 m_inputFlag;                // +0x11c  StoreInputFlag target
    i32 m_inputStateVal;            // +0x120  StoreInputState target
    i32 m_scrollSpeed;              // +0x124  "Scroll_Speed"
    i32 m_128, m_12c, m_130, m_134; // +0x128..+0x134  (m_134==3 -> "won"; FillSaveInfo)
    i32 m_optionsCount;             // +0x138  options-cycle high index (=3 in ctor -> 4 slots)
    i32 m_viewOriginL, m_viewOriginT, m_viewOriginR,
        m_viewOriginB;              // +0x13c..+0x148  view-edge origins
    char m_pad14c[0x150 - 0x14c];   // +0x14c..+0x150 gap
    CGruntzMgrOptions m_options[4]; // +0x150 (4x0x238 options array; EH state 4) -> 0xa30
};

#endif // GRUNTZ_GRUNTZ_GRUNTZMGR_H
