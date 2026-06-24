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
#include <Gruntz/CState.h>    // CState (m_2c game-state; Update() at slot 4)

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
    int IsBusy(); // FUN_00538f60 (this) -> ret (busy state-id 4/0x10)
};

// A typed VIEW of the state-stack array at CGruntzMgr +0xd8. The member itself
// stays a destructible CByteArray (so the ctor/dtor's EH-state numbering is
// unchanged); the accessor methods reinterpret &m_arrD8 as this CObArray-shaped
// view (CObject vptr at +0x00, m_pData at +0x04, m_nSize at +0x08, ...). The
// stored elements are CState* (the pushed game states) whose Update() (slot 4,
// +0x10) reports the state id; SetSize/SetAtGrow/RemoveAt are the out-of-line
// NAFXCW helpers (reloc-masked).
struct CStateStackZ {
    void* m_vptr;                        // +0x00 CObject vptr
    CState** m_pData;                    // +0x04 element store
    int m_nSize;                         // +0x08 live count
    int m_nMaxSize;                      // +0x0c
    int m_nGrowBy;                       // +0x10
    void SetSize(int n, int growBy);     // @0x5b4f75  (clear via SetSize(0, -1))
    void SetAtGrow(int i, CState* elem); // @0x5b5144
    void RemoveAt(int i, int n);         // @0x5b5200
};
struct CGruntzSoundZ {
    void StopBank(int flag);  // FUN_00538900 (this, 1)  -> ret 4
    void StopAll();           // FUN_005388f0 (this)
    void StopBank2();         // FUN_00538920 (this)     -> ret 4 (busy-driven stop)
    char m_pad0[0x1c];        // +0x00..+0x1c
    CGruntzSoundInnerZ* m_1c; // +0x1c  inner object IsBusy/StopAll deref
};

// The level/world object held at CGruntzMgr +0x30 (the loaded map + its active
// CWorld view). Reached as `m_30->...`; every method is reloc-masked. +0x24 is
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
    int m_14; // +0x14
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
        long(__stdcall* Release)(IDirectPlayLobbyZ*); // +0x08
        void *slot3, *slot4, *slot5, *slot6, *slot7;
        long(__stdcall* GetConnectionSettings)(
            IDirectPlayLobbyZ*,
            unsigned long appId,
            void* lpData,
            unsigned long* lpdwSize
        ); // +0x20
    }* vtbl;
};

class CGruntzMgr : public WAP32::CGameMgr {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE; // vtbl slot 0 (own vftable 0x5e9b64)

    // Manager-owned methods reconstructed in GruntzMgr.cpp.
    void UnknownClose() OVERRIDE;                   // @0x0855e0 (member teardown; stubbed)
    void ReportError(WPARAM wParam, LPARAM lParam); // @0x08dc60  -> m_8->vtbl[0x1c]
    char GetGruntzDriveLetter();                    // @0x08fa70  (memoised CD letter)
    int InitializeLobbyConnectionSettings();        // @0x08eca0 (DirectPlay lobby connect)
    CString BuildMoviePath(int movie);              // @0x08ff30 (per-movie path on the CD)
    void PerFrameTick();                            // @0x08f620 (per-frame draw-clock tick)
    void AdvanceFrame(int doDraw, int unused);      // @0x08f6a0 (the per-frame advance gate)
    int CheckPlayState();                           // @0x08ec50 (m_2c->Update()==3||==0x11)
    int RestoreVideoMode(int save);                 // @0x08ddd0 (re-assert 640x480; save on hit)
    int SetVideoMode(int w, int h, int flag);       // @0x08df00 (mode-switch the display; stubbed)

    // Per-state notification forwarders: dispatch (a,b[,c]) into the live state's
    // vtable slot 11..20, returning 0 with no state (0x8d9d0..0x8dbe0).
    int NotifyState0b(int a, int b);        // @0x08d9d0 -> m_2c slot 11 (+0x2c)
    int NotifyState0c(int a, int b);        // @0x08da00 -> m_2c slot 12 (+0x30)
    int NotifyState0d(int a, int b);        // @0x08da30 -> m_2c slot 13 (+0x34)
    int NotifyState0e(int a, int b, int c); // @0x08da60 -> m_2c slot 14 (+0x38)
    int NotifyState0f(int a, int b, int c); // @0x08daa0 -> m_2c slot 15 (+0x3c)
    int NotifyState10(int a, int b, int c); // @0x08dae0 -> m_2c slot 16 (+0x40)
    int NotifyState11(int a, int b, int c); // @0x08db20 -> m_2c slot 17 (+0x44)
    int NotifyState12(int a, int b, int c); // @0x08db60 -> m_2c slot 18 (+0x48)
    int NotifyState13(int a, int b, int c); // @0x08dba0 -> m_2c slot 19 (+0x4c)
    int NotifyState14(int a, int b, int c); // @0x08dbe0 -> m_2c slot 20 (+0x50)

    // State-stack accessors (the CState* array at +0xd8).
    CState* TopState();             // @0x090980 (last pushed state)
    void PushState(CState* s);      // @0x0909b0 (SetAtGrow append)
    int PopTopIfMatches(CState* s); // @0x0909e0 (RemoveAt last; old top == s)
    void ClearStateStack();         // @0x090a50 (delete all, SetSize(0,-1))
    int CheckMovieFileExists();     // @0x090aa0 (FileExists(m_strF0))
    CState* FindStateById(int id);  // @0x092900 (live + stack search by Update id)

    int StoreInputState(int v);              // @0x091a10 (store +0x120, forward to m_60)
    void StoreInputFlag(int v);              // @0x0919d0 (store +0x11c, mirror to g_61ab24 + m_54)
    void UnloadSoundChain();                 // @0x08f740 (m_30->m_28->m_2c teardown + StopBank2)
    void ClearOptionsSlots();                // @0x092ec0 (zero the 4 options slots' +0x20/+0x24)
    CString GetWorldFileName();              // @0x0928c0 (return a copy of m_strC8)
    int AdvanceOptionsCycle();               // @0x0933e0 (round-robin tick of the options slots)
    void SetCellHeight(int r, int c, int v); // @0x111ec0 (write the world height grid)
    int PassClickToPlayState(int a0, int a1, int a2);       // @0x08d780
    int SwitchToNextState();                                // @0x08d6a0
    void EnterModalUI(int arg);                             // @0x08ef10
    int ExitModalUI(class CModalDialog* dlg, int notify);   // @0x0903f0
    int FinishLevel(int full, int stopBank);                // @0x08e980
    int FillSaveInfo(struct SaveInfo* dst, void* snapshot); // @0x0927b0
    int SaveState(class CSerializerZ* ar);                  // @0x093620
    void UpdateScoreHud();                                  // @0x0860b0
    int BroadcastCmd(int a0, int cmd, int a2, int a3);      // @0x093460
    void RecomputeViewScale();                              // @0x08f7f0
    int PrepCmd4(int a0);                                   // reloc-masked sibling (cmd-4 arm gate)
    int PrepCmd7(int a0);                                   // reloc-masked sibling (cmd-7 arm gate)
    struct CActiveObj* GetActiveObj(); // reloc-masked sibling (active game object)
    void RunWinHook();                 // reloc-masked sibling (win/level-complete hook)
    int CheckLevelActive();            // reloc-masked sibling (level-active predicate)
    void* GetSaveSource();             // reloc-masked sibling (live source-state ptr)
    CString GetLevelName();            // reloc-masked sibling (current level name)
    // A sibling state-transition pusher reached by PassClickToPlayState's reloc-
    // masked 4-arg call (deferred body / matched elsewhere).
    int ChangeToPlayState(int a, int b, int c, int d);
    // Reloc-masked CGruntzMgr siblings reached from SwitchToNextState.
    CState* MakeNextState();       // build/find the next state to switch to
    void ActivateState(CState* s); // install + activate the new live state
    void PostSwitchHook();         // post-switch app hook

    // Clock / global helpers.
    void SetGameClock(int now, int delta, int abs); // @0x08f7b0 (mirror the 5 clock globals)
    void ResetClockGlobals();                       // @0x08f4f0 (zero the 7 clock globals)
    int TickStateMgrs();                            // @0x0920b0 (drive two engine singletons)
    int CheckSavedMode();                           // @0x08de70 (saved==live mode test)
    int IsLobbyHostReady();                         // @0x091500 (m_2c/m_8/m_ac null-chain)
    int RunFromState();                // @0x090200 (thin wrapper -> ChangeState_8fab0(1))
    CState* PickPlayOrPausedState();   // @0x092990 (FindStateById(3))
    CState* PickPausedThenPlayState(); // @0x0929b0 (FindStateById(0x11)|| (3))

    // Larger sibling reached by RunFromState's reloc-masked call; body still a
    // @stub in GruntzMgr.cpp (migrated from src/Stub/Discovered.cpp) so the call
    // binds to a CGruntzMgr symbol at 0x08fab0.
    int ChangeState_8fab0(int arg); // @0x08fab0 (deferred / stubbed)

    // --- members (offsets relative to `this`; base CGameMgr occupies 0x00..0x2c) ---
    CState* m_2c;                     // +0x2c  current game-state (Update() -> state id)
    CWorldZ* m_30;                    // +0x30  loaded world/map object (also a draw gate)
    int m_34, m_38, m_3c, m_40, m_44; // +0x34..+0x44
    CGruntzSoundZ* m_48;              // +0x48  sound/bank object (StopBank/StopAll)
    int m_4c, m_50;                   // +0x4c, +0x50
    int m_54, m_58, m_5c, m_60, m_64, m_68, m_6c, m_70, m_74, m_78; // +0x54..+0x78
    int m_7c;                                                       // +0x7c
    int m_80, m_84;                                                 // +0x80, +0x84
    int m_88;                                                       // +0x88  (=0x10 in ctor)
    int m_8c, m_90;                               // +0x8c, +0x90  live video mode (w, h)
    int m_94, m_98;                               // +0x94, +0x98  saved/last-good mode (w, h)
    int m_9c, m_a0, m_a4, m_a8, m_ac, m_b0, m_b4; // +0x9c..+0xb4
    int m_b8;                                     // +0xb8  (=1 in ctor)
    int m_bc;                                     // +0xbc
    IDirectPlayLobbyZ* m_c0;                      // +0xc0  the lobby interface (Released/recreated)
    void* m_c4;                                   // +0xc4  the IDirectPlay interface from Connect
    CString m_strC8;                              // +0xc8  (EH state 0)
    int m_cc;                                     // +0xcc  (=0x1e in ctor)
    char m_d0;                                    // +0xd0  cached CD drive letter
    char m_padD1[3];                              // +0xd1
    int m_d4;                                     // +0xd4  drive-letter probed flag
    CByteArray m_arrD8;                           // +0xd8  (0x14 bytes; EH state 1)
    CString m_strEC;                              // +0xec  (EH state 2)
    CString m_strF0;                              // +0xf0  (EH state 3)
    int m_f4;                                     // +0xf4  (=1 in ctor)
    int m_f8, m_fc;                               // +0xf8, +0xfc
    int m_100, m_104, m_108;                      // +0x100..+0x108  (m_100/m_104 =1 in ctor)
    int m_10c, m_110;                             // +0x10c, +0x110  (=1 in ctor)
    int m_114;                                    // +0x114
    int m_118;                                    // +0x118
    int m_11c;                                    // +0x11c
    int m_120;                                    // +0x120  StoreInputState target
    int m_124;                                    // +0x124
    int m_128, m_12c, m_130, m_134;               // +0x128..+0x134
    int m_138;                                    // +0x138  (=3 in ctor)
    int m_13c, m_140, m_144, m_148;               // +0x13c..+0x148  view-edge origins
    char m_pad14c[0x150 - 0x14c];                 // +0x14c..+0x150 gap
    CGruntzMgrOptions m_options150;               // +0x150 (0x238 bytes; EH state 4)
    char m_pad388[0xa30 - 0x388];                 // +0x388..0xa30  remaining game state
};

#endif // GRUNTZ_GRUNTZ_GRUNTZMGR_H
