// GruntzMgr.h - CGruntzMgr, the Gruntz game manager (C:\Proj\Gruntz). It is the
// REAL derived game manager: `CGruntzMgr : public WAP32::CGameMgr`, 0xa30 bytes,
// with its own vftable (??_7CGruntzMgr@@6B@ @0x5e9b64). CGruntzApp::Initialize-
// GameManager (@0x080a20) does `new CGruntzMgr` => operator new(0xa30) + the
// CGruntzMgr ctor.  The base WAP32::CGameMgr is the genuine 0x2c engine class;
// all the 0xa30 of per-game state lives HERE.
//
// SAME OBJECT AS THE g_gameReg SINGLETON (*0x24556c): CGruntzMgr is the RTTI-true,
// fully-typed MFC VIEW of the very object that <Gruntz/GameRegistry.h>'s
// CGameRegistry models as a plain (MFC-free) struct for the ~60 engine/Win32 TUs.
// Proof: CGruntzMgr::ReportError == CGameRegistry::Ack (both @0x08dc60); m_curState
// ==CGameRegistry::m_2c, m_sound==m_48, m_modeW==m_8c, etc. The two headers are the
// ONE canonical layout expressed twice: CGameRegistry.h is the field-offset source
// of truth (its comments carry these descriptive names). They CANNOT be a single
// header - CGameRegistry.h is included by pure-Win32 TUs (`<Win32.h>`->windows.h)
// and this MFC class pulls afx (C1189 "MFC apps must not #include <windows.h>").
// See docs/vtable-conversion-log.md ("0x24556c dual-view: MFC/Win32 wall").
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
#include <Gruntz/String.h>
#include <Gruntz/GameLevel.h>     // CByteArray
#include <Gruntz/State.h>         // CState (m_curState game-state; Update() at slot 4)
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ / CGruntzSoundInnerZ (m_sound @ +0x48)
// The +0x54/+0x74 sub-objects are typed with their REAL classes (below); pull their
// (lightweight, Ints.h-only) definitions instead of forward-declaring them. Including
// the definition rather than a file-scope `struct X;`/`class X;` fwd-decl keeps the
// SBI_MenuItem TU's transitive fwd-decl count under the DecCounter regalloc-butterfly
// threshold (see docs/patterns/header-fwd-decl-count-regalloc-butterfly.md).
#include <Gruntz/InputState.h>     // CInput54 (+0x54)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (+0x74)
#include <Gruntz/SaveInfo.h> // SaveInfo (m_saveInfoRec) + SaveSink58 (+0x58) + HudGuard44 (+0x44)

// The 0x238-byte options/registry-backed sub-object embedded at +0x150. Its
// ctor (FUN_0051f5a0) takes (this, 0x238, 4, &thunk_FUN_00431250, &LoadOptions)
// and its dtor (FUN_0051f640) takes (this, 0x238, 4, &LoadOptions); both are
// out-of-line so the calls reloc-mask. Only the size (0x238) + that it is a
// destructible member (EH state 4) are load-bearing.
SIZE_UNKNOWN(CGruntzMgrOptions);
struct CGruntzMgrOptions {
    CGruntzMgrOptions();
    ~CGruntzMgrOptions();
    char m_pad0[0x228];
    i32 m_comboSel; // +0x228  battlez-dialog dropdown selection (+1) for this slot
    char m_pad22c[0x238 - 0x22c];
};

// A typed VIEW of the state-stack array at CGruntzMgr +0xd8. The member itself
// stays a destructible CByteArray (so the ctor/dtor's EH-state numbering is
// unchanged); the accessor methods reinterpret &m_stateStack as this CObArray-shaped
// view (CObject vptr at +0x00, m_pData at +0x04, m_nSize at +0x08, ...). The
// stored elements are CState* (the pushed game states) whose Update() (slot 4,
// +0x10) reports the state id; SetSize/SetAtGrow/RemoveAt are the out-of-line
// NAFXCW helpers (reloc-masked).
SIZE_UNKNOWN(CStateStackZ);
struct CStateStackZ {
    char _vft0[4];    // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CState** m_pData; // +0x04 element store
    i32 m_nSize;      // +0x08 live count
    i32 m_nMaxSize;   // +0x0c
    i32 m_nGrowBy;    // +0x10
    void SetSize(i32 n, i32 growBy);     // @0x5b4f75  (clear via SetSize(0, -1))
    void SetAtGrow(i32 i, CState* elem); // @0x5b5144
    void RemoveAt(i32 i, i32 n);         // @0x5b5200
};
// The level/world object held at CGruntzMgr +0x30 (the loaded map + its active
// CWorld view). Reached as `m_world->...`; every method is reloc-masked. +0x24 is
// the active world view (the scroll/camera holder the FP scaler reads); +0x28 the
// sound/anim cue host (the CSndHost of <Gruntz/SoundCue.h>, included above):
// the former CWorldSub28/CWorldSub2c "sub-controller + teardown" views were IT -
// FUN_00537a80 == RVA 0x137a80 == SoundStream::Stop, and the +0x10 keyed map ==
// CSndFinder (Lookup 0x1b8438). +0x520 is a 4-slot status array the paused-state
// poll walks (status id at each slot's +0x20).
struct CWorldView;
// The world's +0x10 recolor lookup holder (its own +0x10 is an embedded CColorLookup);
// CWorldZ::m_10 is a pointer, so a forward declaration suffices - the holder's full
// layout lives in GruntzMgr.cpp, the only TU that walks it.
struct CWorldLookupHolder;
// A polymorphic sub-object held in the world at +0x1c, dispatched through a
// pointer-to-pointer (`*m_1c`) then virtual slot 10 (+0x28). The full COM-interface
// definition (real virtuals) lives in GruntzMgr.cpp - the only TU that dispatches on
// it; here CWorldZ::m_1c is a pointer, so a forward declaration suffices (keeps this
// widely-included class header free of the COM/STDMETHOD (objbase.h) footprint).
struct CWorldDispatch;
// The world's +0x4 sub-object exposes a map-index field at +0x14 and, at +0x10, the
// command-context object the world-present toolbar forwarder (Fwd114f00) drills to.
SIZE_UNKNOWN(CWorldSub4);
struct CWorldSub4 {
    char m_pad0[0x10];
    void* m_10; // +0x10  command-context object (its +0x2c is forwarded to 0x267b)
    i32 m_14;   // +0x14
};
// The real loaded-world/map object (CGruntzMgr::m_world). One object; each manager
// method that touches it reads a different facet: the +0x4 map sub-object, the +0x10
// recolor lookup holder, the +0x1c polymorphic dispatch, the +0x24 active view, the
// +0x28 sound/state sub-controller and the +0x38 load-status code.
SIZE_UNKNOWN(CWorldZ);
struct CWorldZ {
    char m_pad0[0x4];
    CWorldSub4* m_4; // +0x04
    // +0x08 the sprite/object factory (== CSpriteFactoryHolder::m_8 in the
    // GameRegistry.h view of this same object; its +0x48 embedded map hosts the
    // 0x8106 death lookup). Fwd-declared; Grunt.h carries the full def.
    struct CSpriteFactory* m_8; // +0x08
    char m_padc[0x10 - 0xc];
    CWorldLookupHolder* m_10; // +0x10  recolor lookup holder
    char m_pad14[0x1c - 0x14];
    CWorldDispatch** m_1c; // +0x1c
    char m_pad20[0x24 - 0x20];
    CGameLevel* m_24; // +0x24  active world view
    CSndHost* m_28;   // +0x28  sound/anim cue host (finder m_10 / stream m_2c / gate m_30)
    char m_pad2c[0x38 - 0x2c];
    u32 m_38; // +0x38  load-status code (ReportWorldStatus maps it to a message id)
};

// Minimal IDirectPlayLobby-shaped COM surface used by
// InitializeLobbyConnectionSettings (Release slot 2 / GetConnectionSettings slot 8,
// called twice in the size-probe / fill idiom). The full COM-interface definition
// (real virtuals) lives in GruntzMgr.cpp - the only TU that dispatches on it; the
// CGruntzMgr::m_lobby member is a pointer to the real DirectPlay lobby interface
// (IDirectPlayLobby, from <dplobby.h>); a forward declaration suffices here, so this
// ~60-TU header stays free of the DirectPlay/windows.h chain (only GruntzMgr.cpp,
// which dispatches on it, includes <dplobby.h>).
struct IDirectPlayLobby;

// CInput54 (+0x54 active-level input/spatial-sound object; Flush/Arm/Disarm/InitInput/
// StoreFlag; the ambient TU reads its +0x24 armed==playable gate + its
// +0x08 spatial voice CObList) is defined by the <Gruntz/InputState.h> include above.

// The manager's owned engine sub-objects, each a real class reached only through a
// reloc-masked thiscall / vtable slot from GruntzMgr.cpp; the members are pointers,
// so forward declarations suffice here. Each unifies what were previously several
// per-method facet views of the SAME object into its one real class (defined in
// GruntzMgr.cpp): the teardown-only slots share EngObj (Teardown()), and the
// multi-facet slots carry all their facets' fields + methods.
struct EngObj; // teardown-only sub-object (Teardown())
// CSpriteRefTable (+0x74 sprite/animation ref table; Reset teardown @0xe2290) is
// defined by the <Gruntz/SpriteRefTable.h> include above.
class CGruntSpawnConfig;
class CGameLevel;
class CLightFxMgr;      // +0x78 light-FX/shade-table pump (Reset teardown @0x9dc80)
class CWorldDelete;     // +0x3c world sub-object torn down via vtable slot 1
struct CRezSurface94;   // +0x34 recolor surface (Build/Apply/Teardown)
struct CSettingsWriter; // +0x38 settings/registry writer (WriteInt/Teardown)
// HudGuard44 (+0x44 HUD first-frame guard) and SaveSink58 (+0x58 save-record
// sink) are defined by the <Gruntz/SaveInfo.h> include above.
struct CChatLog; // +0x5c chat/message log (Insert)
struct TimerObj; // +0x60 per-frame timer/poll (m_inputMirror/Stop/Tick)
// +0x68: the world command/trigger grid is the ONE CTriggerMgr (TriggerMgr.h) -
// the former CCmdGrid view is dtor-proven the same object (Close's teardown thunk
// 0x3b1b IS ~CTriggerMgr; the +0x20c/+0x21c delta tables == m_rowStateB/C, the
// +0x288 scored flag == m_288).
class CTriggerMgr;
class CPlay;        // PickPlayOrPausedState's concrete return (the PLAY state; Play.h)
struct CmdSink;     // +0x6c command sub-manager sink (Command)
class CmdSinkV;     // +0x70 polymorphic command sink (slot 1) + cell-height notify
class CBattlezData; // +0x7c HUD/score accumulator + command sink (BattlezData.h)

SIZE(CGruntzMgr, 0xa30);
VTBL(CGruntzMgr, 0x001e9b64); // vtable_names -> code (RTTI game class)
class CGruntzMgr : public WAP32::CGameMgr {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE;             // vtbl slot 0 (own vftable 0x5e9b64)
    virtual i32 Run(CGameWnd*, char*) OVERRIDE; // slot 1 (declared-only)
    virtual i32 Wap32GameMgrVfunc3() OVERRIDE;  // slot 3
    // The ??_G scalar-deleting destructor (vtable slot 0 entry the retail vtable
    // holds): run the dtor body, then operator delete when the low flag bit is set;
    // returns this. Modeled by hand (MSVC's own ??_G mangling differs from the retail
    // label the delinker emits, so this carries the RVA).
    void* ScalarDeletingDtor(u32 flags); // @0x083330

    // Manager-owned methods reconstructed in GruntzMgr.cpp.
    virtual void Close() OVERRIDE; // @0x0855e0 (member teardown)
    void AccrueScoreTime();        // @0x0861e0 (per-state HUD time/score accrual + state push)
    void OnCheckpointReached();    // @0x08e6c0 (checkpoint modal -> WM_COMMAND 0x80cf)
    void DelayedQuit();            // @0x08f530 (menu-activate delay spin -> WM_CLOSE)
    // @0x0907c0 (ret 4) - resolve the installed "Portal" companion exe path
    // (LaunchPortalExe), spawn it in place (LaunchProcessInDir), and when `quitAfter`
    // is set schedule the delayed shutdown. Returns 1 on a successful launch, else 0.
    i32 LaunchPortal(i32 quitAfter);
    // Clamp a command index into (0,0x29] and PostMessageA WM_COMMAND 0x807f to the
    // game window (wParam = index, or 1 when index==0x29). CPlay reaches it as
    // m_4->Post via the CState owner back-ptr (called by CPlay::Vslot15/Dispatch).
    void Post(i32 code); // @0x090220
    i32 SaveGameAs();    // @0x092f00 (save-as name dialog -> WM_COMMAND 0x80e3)
    void ReportError(WPARAM wParam, LPARAM lParam); // @0x08dc60  -> m_8->vtbl[0x1c]
    // @0x08dc20 (ret 4) - XOR the flag bits (m_stateFlags) on every live world
    // sprite-factory object (m_world->m_8->m_liveObjects).
    void XorLiveObjectFlags(i32 mask);
    // @0x08dc90 (ret 0) - (re)register the level asset-namespace keys ("GRUNTZ" /
    // "LEVEL" / "ACTION") into the world's lookup holder (m_10) and sound host (m_28),
    // resetting the world coord dispatch (m_1c) in between. No-op with no world.
    void RegisterLevelAssetKeys();
    char GetGruntzDriveLetter();                    // @0x08fa70  (memoised CD letter)
    RVA(0x0008fa40, 0x16)
    i32 IsInPlayState() {
        if (m_curState == 0) {
            return 0;
        }
        return CheckPlayState() != 0;
    }
    // @0x08f340 (/GX): when the live state is playable (Update() in {5,2,3,7}),
    // capture the current world-file name from the game window into m_strWorldFile,
    // clear the score/state slots (m_128/m_12c), and PostMessageA WM_COMMAND 0x8005.
    i32 CaptureWorldFile();
    i32 InitializeLobbyConnectionSettings();   // @0x08eca0 (DirectPlay lobby connect)
    CString BuildMoviePath(i32 movie);         // @0x08ff30 (per-movie path on the CD)
    virtual void PerFrameTick() OVERRIDE;      // @0x08f620 slot 4 (per-frame draw-clock tick)
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
    i32 GoToNextLevel();     // @0x08d850 (PLAY: advance current level index, wrap)
    i32 GoToPrevLevel();     // @0x08d910 (PLAY: retreat current level index, wrap)
    i32 ToggleObjectLayer(); // @0x08efe0 (toggle the "current" object layer visible bit)
    i32
    ToggleHeightLayer(); // @0x08f060 (toggle the height sub-layer (m_world->m_24->m_5c) visible bit)
    i32 ToggleBaseLayer();            // @0x08f0b0 (toggle layer[0]'s visible bit)
    i32 PollUnlessIdle();             // @0x08f2f0 (CheckPlayState() unless state idle (5); ret 0)
    i32 AppendChatMessage(char* msg); // @0x08f9c0 (-> m_chatLog insert)
    i32 ShowToggleMessage(char* itemName, i32 on); // @0x08f9f0 ("<item> is ON/OFF")

    i32 IsMoviePathValid();        // @0x0901d0 (bool-normalized FileExists(m_strMoviePath))
    void ReportWorldStatus(i32 a); // @0x090ac0 (map m_world->m_38 status to ReportError)
    i32 LoadMonologoSprite();      // @0x090d10 (PLAY-only: find/toggle/create the MONOLITH logo)
    i32 CheatRevealTreasures();    // @0x090f10 (PLAY-only: color all treasure/collectible rows)
    i32 SetGruntColor(i32 sink, i32 key, i32 idx); // @0x0910d0 (recolor a sink cell)
    void CheatSkeletonToggle(); // @0x091250 (toggle the grunt "skeleton" image type + cue)
    void CheatEclipseToggle();  // @0x091390 (toggle the grunt "eclipse" image type + cue)
    i32 WarpCheat();            // @0x08eaf0 (per-level warp X/Y registry set/apply)
    // World-object scans: walk m_world's live game-object list, invoking a __cdecl
    // callback(obj, user) for each object matching `mask` whose screen position falls
    // inside a radius / rect; return the hit count. The cb/rect pointers are passed
    // as i32 (the consumers cast) to keep this widely-included header dependency-free.
    i32 ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, i32 cb, i32 user);   // @0x092180
    i32 ScanObjectsInRect(i32 offX, i32 offY, i32 rect, i32 mask, i32 cb, i32 user); // @0x092250
    i32 SetColorDepth(i32 depth);    // @0x091170 (set the packed g_surfaceColorKey color by depth)
    i32 LoadWorldMode(i32 mode);     // @0x091a40 (switch the world video/color mode + reload)
    i32 ResetWorldState(i32 notify); // @0x091e20 (idle/exit-prep the world, reset cursor)
    void StopBankIfActive();         // @0x092000 (if m_sound && m_14: m_sound->StopAll())
    void StopBank0IfActive();        // @0x092030 (if m_sound && m_14: m_sound->StopBank(0))
    // @0x092060: set the global asset-root path CString (g_assetRoot @0x64e25c) and
    // post WM_COMMAND 0x80ab to the game window; ret 1 (0 when path is null).
    i32 SetAssetRoot(char* path);
    // Music-volume guards on the same m_sound/m_14 gate (BoundaryLowerMethods.cpp;
    // the former C915d0 view): fade the CURRENT bank to 0 / back to 100 over `ms`.
    void MuteMusicIfActive(i32 ms);          // @0x0915d0 (m_pCurrent->SetVolume(0, ms))
    void RestoreMusicVolumeIfActive(i32 ms); // @0x091620 (m_pCurrent->SetVolume(100, ms))
    // Debug-command hook (the former C8e880 view): when the current state is
    // GAMESTATE_PLAY, register the DEBUG_SETSKILL command (via ILT thunk 0x2bb7).
    i32 RegisterSetSkillDebugCmd();                               // @0x08e880
    void RegisterDebugCommand(const char* name, void* fn, i32 n); // ILT 0x2bb7 (reloc-masked)

    i32 StoreInputState(i32 v); // @0x091a10 (store m_inputStateVal, forward to m_timer)
    void StoreInputFlag(i32 v); // @0x0919d0 (store m_inputFlag, mirror to g_61ab24 + m_inputState)
    void UnloadSoundChain();    // @0x08f740 (m_world->m_28->m_2c teardown + StopBank2)
    void ClearOptionsSlots();   // @0x092ec0 (zero the 4 options slots' +0x20/+0x24)
    RVA(0x000928c0, 0x23)
    CString GetWorldFileName() {
        return m_strWorldFile;
    }
    i32 AdvanceOptionsCycle(); // @0x0933e0 (round-robin tick of the options slots)
    i32 SyncOptionsState();    // @0x093170 (reload each options slot's config; dual-slot)
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
    i32 ExitModalUI(class CModalDialog* dlg, i32 notify); // @0x0903f0
    i32 FinishLevel(i32 full, i32 stopBank);              // @0x08e980
    i32 FillSaveInfo(SaveInfo* dst, void* snapshot);      // @0x0927b0
    i32 SaveState(struct CSerialArchive* ar);             // @0x093620 (shared CSerialArchive)
    i32 LoadState(struct CSerialArchive* ar);             // @0x093920 (deserialize counterpart)
    void UpdateScoreHud();                                // @0x0860b0
    i32 BroadcastCmd(i32 a0, i32 cmd, i32 a2, i32 a3);    // @0x093460
    void RecomputeViewScale();                            // @0x08f7f0
    i32 PrepCmd4(i32 a0);                                 // reloc-masked sibling (cmd-4 arm gate)
    i32 PrepCmd7(i32 a0);                                 // reloc-masked sibling (cmd-7 arm gate)
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
    void SetRunState(i32 v); // @0x092340 (set base m_10 run-state + side-effects)
    i32 CheckSavedMode();    // @0x08de70 (saved==live mode test)
    i32 IsLobbyHostReady();  // @0x091500 (m_curState/m_8/m_modalBusy null-chain)
    RVA(0x00090200, 0x8)
    i32 RunFromState() {
        return ChangeState_8fab0(1);
    }
    RVA(0x00092990, 0x8)
    // Returns the concrete PLAY/paused state (FindStateById(3) is always a CPlay);
    // typed CPlay* so the ~12 cheat-dispatch callers drop their (CPlay*) downcast.
    // CState is CPlay's offset-0 base -> the cast is a no-op reinterpret (byte-neutral).
    CPlay* PickPlayOrPausedState() {
        return (CPlay*)FindStateById(3);
    }
    CState* PickPausedThenPlayState(); // @0x0929b0 (FindStateById(0x11)|| (3))

    // The modal-dialog runner sibling (a __thiscall (template, dlgProc, flag) ->
    // i32; reloc-masked). The leading Ghidra label winapi_090260_DialogBoxParamA
    // is the engine's DialogBoxParamA wrapper.
    i32 RunModalDialog(const char* tmpl, void* dlgProc, i32 flag); // @0x090260

    // The WM_COMMAND / accelerator + cheat-code dispatcher (the binary's single
    // largest function; body in GruntzMgrCmd.cpp).
    virtual i32 HandleCommand(i32 p1, i32 nID, i32 p3) OVERRIDE; // @0x0862f0 slot 5
    // Shell-launch the given URL in the default browser (0x8038 handler sibling).
    void LaunchWebBrowser(const char* url); // @0x08f120 (thunk 0x235b)

    // Sound/level-loaded sync (@0x0923b0): set the base level-loaded flag (m_14)
    // and, when it changes and a sound bank is bound, drive the bank.
    void SetSoundLevelState(i32 loaded);
    i32 RunLoadGameDialog(); // @0x092500 (GAME_LOAD dialog; ret 1)  == Quickload's Fallback
    i32 Quicksave();         // @0x092530 (quicksave the game; /GX)
    // @0x092710 - the quickload counterpart: if a save sink + a valid quicksave
    // record (m_saveInfoRec bit 0) exist, flush the timer, load via the sink
    // (SaveSink58::Check), notify the window (WM_COMMAND 0x807e) and log to the
    // chat log; else fall back to RunLoadGameDialog (0x092500).
    i32 Quickload();
    i32 RunDebugGruntTypeDialog(); // @0x0929e0 (DEBUG_GRUNTTYPE dialog when PLAY)
    // @0x092d50 - if in play state and the options slot is not yet loaded, assign
    // its name CString. 7 raw args (only the slot index + the value string used).
    i32 LoadOptionsSlotName(i32 slot, i32 a2, i32 a3, i32 a4, i32 a5, const char* val, i32 a7);
    i32 CountReadyOptionsSlots(i32 anyState);   // @0x092e30 (count loaded/armed slots)
    struct OptionsSlot* FindOptionsSlot(i32 x); // @0x092e80 (slot whose m_18 == x)
    i32 ResetOptionsSlot(i32 idx);              // @0x092da0 (reset slot idx if loaded)
    void ResetAllOptionsSlots();                // @0x092df0 (reset all 4 slots)
    i32 IsStandardMode();                       // @0x08f980 (mode == 640x480)
    i32 DebugJumpLevel();                       // @0x08e780 (DEBUG_JUMPLEVEL dialog)
    i32 PostSlotCommandB1(i32 slot);            // @0x0920e0 (post WM_COMMAND 0x80b1, slot)
    i32 PostSlotCommandB6(i32 slot);            // @0x092130 (post WM_COMMAND 0x80b6, slot)
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
    CState* m_curState;              // +0x2c  current game-state (Update() -> state id)
    CWorldZ* m_world;                // +0x30  loaded world/map object (also a draw gate)
    CRezSurface94* m_recolorSurface; // +0x34  color-depth recolor surface
    CSettingsWriter* m_settings;     // +0x38  settings/registry writer (WriteInt)
    CWorldDelete* m_3c;              // +0x3c  engine sub-object (vtable-slot-1 teardown)
    EngObj* m_40;                    // +0x40  engine sub-object (teardown-only)
    HudGuard44* m_hudGuard;          // +0x44  HUD first-frame seed guard (+0x124)
    CGruntzSoundZ* m_sound;          // +0x48  sound/bank object (StopBank/StopAll)
    i32 m_4c;                        // +0x4c
    EngObj* m_50;                    // +0x50  engine sub-object (teardown-only)
    // +0x54..+0x78 sub-controllers (real engine sub-object pointers reached through
    // reloc-masked thiscalls / vtable slots from GruntzMgr.cpp):
    CInput54* m_inputState; // +0x54  input/state object (Flush/Arm/Disarm/StoreFlag)
    SaveSink58* m_saveSink; // +0x58  save-record sink (SaveSink58::Store)
    CChatLog* m_chatLog;    // +0x5c  chat/message log (Insert)
    CGruntSpawnConfig*
        m_timer; // +0x60  per-frame timer/poll (== the spawn-config obj; Stop/Tick; m_2c mirror)
    i32 m_64;    // +0x64
    CTriggerMgr* m_cmdGrid;           // +0x68  world command/trigger grid (CTriggerMgr)
    CmdSink* m_cmdSubMgr;             // +0x6c  command sub-manager sink
    CmdSinkV* m_cmdNotify;            // +0x70  command sink (vtbl slot 1) + cell-height notify
    CSpriteRefTable* m_spriteFactory; // +0x74  sprite/animation ref table (LoadSprite/GetSel/
    //         GetByIndex consumers; Close tears it down via Reset)
    CLightFxMgr* m_logicPump; // +0x78  light-FX/shade-table pump (Push + m_tables[10]@+0x14
                              //         consumers; Close tears it down via Reset @0x9dc80)
    CBattlezData* m_scoreHud; // +0x7c  HUD/score accumulator + command sink (real CBattlezData)
    i32 m_numRuns;            // +0x80  "Num_Runs"   (launch counter; Close WriteInt)
    i32 m_numMovies;          // +0x84  "Num_Movies" (movie-playback counter)
    i32 m_colorDepth;         // +0x88  live color depth (bpp): 8/16(=HiColor)/24 (=0x10 in ctor)
    i32 m_modeW, m_modeH;     // +0x8c, +0x90  live video mode (w, h)
    i32 m_savedModeW, m_savedModeH;   // +0x94, +0x98  saved/last-good mode (w, h)
    i32 m_lobbyResult;                // +0x9c  lobby-connect success flag (1/0)
    i32 m_lobbyProbed;                // +0xa0  one-shot lobby-connect guard
    i32 m_a4, m_a8;                   // +0xa4, +0xa8
    i32 m_modalBusy;                  // +0xac  modal-UI/cursor-busy gate
    i32 m_b0, m_b4;                   // +0xb0, +0xb4
    i32 m_isCheckpointPrompts;        // +0xb8  "Checkpoint_Prompts" enable (=1 in ctor)
    SaveInfo* m_saveInfoRec;          // +0xbc  last FillSaveInfo dst record
    struct IDirectPlayLobby* m_lobby; // +0xc0  the DirectPlay lobby interface (Released/recreated)
    void* m_connSettings;             // +0xc4  the launch connection-settings buffer
    CString m_strWorldFile;           // +0xc8  world file name (EH state 0)
    i32 m_cc;                         // +0xcc  (=0x1e in ctor)
    char m_driveLetter;               // +0xd0  cached CD drive letter
    char m_padD1[3];                  // +0xd1
    i32 m_driveLetterProbed;          // +0xd4  drive-letter probed flag
    CByteArray m_stateStack;          // +0xd8  CState* push-down stack (0x14 bytes; EH state 1)
    CString m_strEC;                  // +0xec  (EH state 2)
    CString m_strMoviePath;           // +0xf0  resolved movie path (EH state 3)
    i32 m_f4;                         // +0xf4  (=1 in ctor)
    i32 m_f8, m_fc;                   // +0xf8, +0xfc
    i32 m_isVoiceEnabled;             // +0x100  "Voice"      enable (=1 in ctor)
    i32 m_isAmbientEnabled;           // +0x104  "Ambient"    enable (=1 in ctor)
    i32 m_isInterlaced;               // +0x108  "Interlaced" flag
    i32 m_isHighDetail;               // +0x10c  "High_Detail" flag (=1 in ctor)
    i32 m_isEffectsEnabled;           // +0x110  "Effects"    enable (=1 in ctor)
    i32 m_114;                        // +0x114
    i32 m_isEasyMode;                 // +0x118  "Easy_Mode" flag
    i32 m_inputFlag;                  // +0x11c  StoreInputFlag target
    i32 m_inputStateVal;              // +0x120  StoreInputState target
    i32 m_scrollSpeed;                // +0x124  "Scroll_Speed"
    i32 m_128, m_12c, m_130, m_134;   // +0x128..+0x134  (m_134==3 -> "won"; FillSaveInfo)
    i32 m_optionsCount;               // +0x138  options-cycle high index (=3 in ctor -> 4 slots)
    i32 m_viewOriginL, m_viewOriginT, m_viewOriginR,
        m_viewOriginB;              // +0x13c..+0x148  view-edge origins
    char m_pad14c[0x150 - 0x14c];   // +0x14c..+0x150 gap
    CGruntzMgrOptions m_options[4]; // +0x150 (4x0x238 options array; EH state 4) -> 0xa30
};

#endif // GRUNTZ_GRUNTZ_GRUNTZMGR_H
