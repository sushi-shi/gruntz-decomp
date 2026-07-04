// GameMode.h - the game-state ("mode") hierarchy that the per-frame tick drives.
//
// CGruntzMgr/RezMgr::PerFrameTick (matched in the `rezmgr` unit) holds
// the active game-state object at RezMgr+0x2c (m_mode) and each frame calls:
//     int  m_mode->vtbl[+0x10]()   (slot 4) = Update()  -> a state-ID/status int
//     int  m_mode->vtbl[+0x14]()   (slot 5) = Render()  -> the per-frame step+draw
// (PerFrameTick gates timing on `Update() != 0x11` and gates Render on a render
// flag.) This file reconstructs that state hierarchy.
//
// THE HIERARCHY (recovered from RTTI + the vtables, ImageBase 0x400000):
//   CState           base game-state class.
//                    (CState itself derives from a WAP32 base - the dtor
//                     chains to it; modeled here as an external no-body fn.)
//   CPlay            the in-game PLAY state.
//   CMenuState       the front-end menu state.
//   CCreditsState    the credits state.
//   CBootyState      the bonus/"booty" state.
//
// THE KEY FINDING: each state's Update() (slot +0x10) is a 6-byte stub that just
// returns the state's own ID tag - it is NOT the per-frame logic:
//   CState::Update      ->  return 1;
//   CPlay::Update       ->  return 3;
//   CMenuState::Update  ->  return 5;
//   CCreditsState::Update -> return 8;
//   CBootyState::Update ->  return 0xa;
// The REAL per-frame step+draw lives in Render() (slot +0x14): CState::Render is a
// trivial `return 1;` default, but the concrete states override it with
// the heavy per-frame work (input poll -> entity update loop -> network post ->
// draw): CCreditsState::Render, CMenuState::Render,
// CBootyState::Render, CPlay::Render (the in-game per-frame heart).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code bytes
// are load-bearing (campaign doctrine). The CState layout below is confirmed from
// the ctor and dtor.
#ifndef SRC_GRUNTZ_GAMEMODE_H
#define SRC_GRUNTZ_GAMEMODE_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// The WAP32 base cleanup CState's dtor chains to (reached via the
// incremental-link thunk). It is a __thiscall (this in ecx, callee-
// cleaned, NO `add esp,4` at the call site), so it is modeled as a method on a
// tiny helper whose `this` is the CState object - that emits `mov ecx,this; call
// rel32` (reloc-masked) with no stack cleanup, matching the target.
#include <Gruntz/GameModeBase.h>
// (The scalar-deleting dtor's `operator delete` is reached via MSVC's
// auto-synthesized `??3@YAXPAX@Z` in the `??_G` thunk - no explicit decl needed.)

// ---------------------------------------------------------------------------
// Sub-object layouts the concrete Render overrides (CCreditsState::Render,
// CMenuState::Render) walk - only the offsets they read are
// modeled; field names are placeholders. Unmatched engine callees are external
// no-body fns (reloc-masked); the global per-frame entity set + the cached
// USER32/engine globals (the frame clock, the version-RECT) are file-scope.
// ---------------------------------------------------------------------------
// <Mfc.h> brings <windows.h> USER32: PostMessageA (the per-frame message scan posts
// WM_COMMAND to the owner HWND; the owner's HWND member is typed HWND below).
#include <Mfc.h>

// A per-frame entity (g_entityList element). Render iterates it (slot +0x10 =
// Update) and the message scans test its flag word m_2ac.
SIZE_UNKNOWN(CGMEntity);
struct CGMEntity {
    virtual void Gv0();
    virtual void Gv1();
    virtual void Gv2();
    virtual void Gv3();
    virtual void Update(); // slot 4 (+0x10) - per-entity per-frame step
    char m_pad4[0x2ac - 0x4];
    i32 m_2ac; // +0x2ac flag word (scanned with a bit mask)
};

// The per-frame entity set: count @+0x4, element-ptr array @+0x8. The global
// The global is a POINTER to this structure (the Render loops load it first:
// `mov reg,[0x645574]; mov cnt,[reg+4]; elems = reg+8`).
SIZE_UNKNOWN(CGMEntityList);
struct CGMEntityList {
    void* m_0;             // +0x00
    i32 m_count;           // +0x04
    CGMEntity* m_elems[1]; // +0x08 (the entity-ptr array)
};
extern "C" CGMEntityList* g_645574; // (a pointer to the list)

// The input/anim sub-object the credits poll reaches (m_c->m_4->m_10->m_2c->m_8).
// Its slot +0x60 is a fn-ptr the object is passed to as the explicit STACK arg
// (NOT in ecx) and the CALLEE cleans the stack (no `add esp,4` at the call site)
// -> __stdcall: `mov ecx,[obj]; push obj; call [ecx+0x60]`.
struct CGMInputVtbl;
SIZE_UNKNOWN(CGMInputObj);
struct CGMInputObj {
    CGMInputVtbl* vtbl;
}; // +0x00 vtable ptr
SIZE_UNKNOWN(CGMInputVtbl);
struct CGMInputVtbl {
    char m_pad0[0x60];
    i32(__stdcall* Poll)(CGMInputObj* self); // +0x60
};

// The owner back-ptr (CState+0x4) the Render path dereferences. +0x4->+0x4 = the
// OS HWND (PostMessageA target); +0x8 a sub-object (m_244 cleared); +0x14 a view
// gate; +0x48 the sound manager; the credits "post & bail" is m_4->Post(...).
SIZE_UNKNOWN(CGMSound);
struct CGMSound {
    void Play(const char* name, i32 z); // (thiscall, 2 args)
    i32 Find(const char* name);         // (thiscall, 1 arg -> ptr)
};
SIZE_UNKNOWN(CGMSoundEntry);
struct CGMSoundEntry {
    i32 Query();
}; // (thiscall, no arg -> int)
SIZE_UNKNOWN(CGMOwner);
struct CGMOwner {
    void Post(u32 a, u32 b); // (thiscall, 2 args)
    char p0[0x4];            // +0x00
    struct M4 {
        char p0[0x4];
        HWND m_4;
    }* m_4; // +0x04 -> +0x04 = HWND
    struct M8 {
        char p0[0x244];
        i32 m_244;
    }* m_8; // +0x08 -> +0x244 latch
    char p0c[0x14 - 0x0c];
    void* m_14; // +0x14 view gate (0 -> skip FX)
    char p18[0x48 - 0x18];
    CGMSound* m_48; // +0x48 sound manager
};

// The cursor/anim object reached via m_c->m_28->m_2c (credits only). Callee-
// cleaned (no `add esp,4` at the call site) -> __stdcall.
extern "C" void __stdcall GM_SimpleAnim(i32 z); // (stdcall, 1 arg)

// The view/draw holder (CState+0xc) render facet the credits poll walks is the same
// shared CView (<Gruntz/View.h>): m_c->m_4->m_10->m_2c->m_8 (input obj), the draw
// block m_c->m_4->{m_10->m_2c (Draw), m_14 (Blit), m_18 (blit arg)}, m_28->m_2c
// (cursor gate). Reached through m_c directly (no cast).

// The CMenuState UI object (m_1b4): each entity-flag scan fires a distinct no-arg
// method on it; the tail steps it (one arg = g_645584) + draws the version RECT.
SIZE_UNKNOWN(CGMMenuUI);
struct CGMMenuUI {
    void OnFlag80000000();
    void OnFlag40000000();
    void OnFlag20000000();
    void OnFlag10000000();
    void OnFlag00000003();
    i32 OnFlag00000100(); // (-> int)
    void Step(u32 dt);    // (1 arg)
    void Pre();           // (no arg)
    void Post();          // (no arg)
    void PreDelete();     // FUN_004a0360 - pre-delete release (ReleaseResources teardown)
};
// The version-string RECT source globals (4 ints copied to a stack RECT by value).
SIZE_UNKNOWN(CGMVerRect);
struct CGMVerRect {
    i32 a, b, c, d;
};
extern "C" CGMVerRect g_645cc8; // (the 4-int source @c8/cc/d0/d4)
extern "C" u32 g_645584;        // (last-frame delta, fed to Step)

// The two cue/sound-name string constants the credits one-shot FX reference.
extern "C" char g_60ce90[]; // "CREDITZ" (PlaySound name)
extern "C" char g_60ce74[]; // "MONOLITH" (FindSound name)

// ---------------------------------------------------------------------------
// CState - the base game-state class. One canonical definition, shared via
// <Gruntz/State.h> (full 41-slot vftable + ctor-pinned layout). The leaf states
// below derive from it; the gamemode TU casts the owner member (CGruntzMgr* m_4)
// to its own CGMOwner reconstruction and reaches the +0x0c CView resource facet
// (m_c, the shared <Gruntz/View.h> class) directly.
#include <Gruntz/State.h>
#include <Gruntz/View.h> // the shared CState::m_c view/render/resource class

// Single-type leaf-state sub-object views, defined in GameMode.cpp; forward-
// declared so the leaf members below are typed to their real class (no per-site
// cast). Each is a pointer slot, so the typing is codegen-neutral.
struct CMenuMusic;       // CMenuState::m_1bc       - menu music controller
struct CCreditsVideo;    // CCreditsState::m_videoHandle - Smacker video handle
struct CBootyBonusState; // CMultiBootyState::m_bonusState - bonus scroll/flags object
struct CGlitterAnim; // CMultiBootyState::m_cursorLetter + the +0x1ec/+0x204 letter-sprite arrays
                     // (the "SimpleAnimation" sprite the factory builds; the booty
                     //  draw walks the same objects as position/flag records)

// ---------------------------------------------------------------------------
// The concrete leaf states. Each overrides Update() to return its own state-ID
// tag (the 6-byte stub) - the only override modeled here for the leaf match
// (their own vtables carry the heavy Render overrides, matched/carcassed
// separately). The in-game PLAY state CPlay lives in its own <Gruntz/Play.h>.
// ---------------------------------------------------------------------------

// CMenuState - the front-end menu state. Render
// (464 B) drives the per-frame menu: a per-entity Update pass, then six
// entity-flag scans each firing a distinct method on the menu UI object m_1b4,
// then the UI step + the on-screen version-string RECT draw.
SIZE_UNKNOWN(CMenuState);
class CMenuState : public CState {
public:
    // The ~CMenuState() destructor (EH-framed `??1` under /GX): it re-stamps the
    // CMenuState vtable, runs the slot-2 resource release (ReleaseResources,
    // statically bound in the dtor), then re-stamps the CState vtable and chains
    // the base cleanup. Defined out-of-line (GameMode.cpp) so MSVC emits a
    // distinct `??1` the `??_G` deleting dtor dispatches to.
    virtual ~CMenuState() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;     // GAMESTATE_MENU (5);  (slot 4)
    virtual i32 Render() OVERRIDE;             // the per-frame menu draw (this TU)
    virtual void ReleaseResources() OVERRIDE;  // slot 2 (+0x8) - menu teardown
    virtual i32 FrameSlot28(i32 arg) OVERRIDE; // slot 10 (+0x28) - per-frame poll

    // CMenuState's own methods (the rel32 thunks Render dispatches to with
    // `mov ecx,this`). External no-body -> reloc-masked.
    void DrawVersion(CGMVerRect r); // (this, RECT by value)

    // Non-virtual menu helpers (called from FrameSlot28 / its siblings).
    void StartMusic();     // 0xa05a0 - music start gate
    void StopMusicChain(); // 0xa0640 - stop + cue chain

    // ReadyGate (0xa0d40): the &&-chained ready/transition probe -
    // Vfunc3() (slot 3) && CommitState() (the 0x1136 thunk) ? Vslot06() (slot 6).
    i32 ReadyGate();
    // CommitState (reached via the 0x1136 ILT thunk; external no-body ->
    // reloc-masked). Returns nonzero when the pending state commit succeeds.
    i32 CommitState();

    // 0x1af70 - the 960-B HUD-text formatter switch: 8 cases of sprintf over the
    // game-stats object (g_mgrSettings->m_7c), each stat read via the live-getter /
    // cached-field pair gated by m_liveGame && stats->m_c (the sibling-guard idiom).
    void FormatHudText(struct CHudBuf* buf, i32 sel);

    // MENU asset loader (0x9fe50, MenuStateAssets.cpp): registers the MENU
    // IMAGEZ/SOUNDZ namespaces through the m_c (CView) resource facet, primes the
    // state core, then builds the menu HUD object + wires its keys/sound cues.
    i32 LoadAssets(i32 a1, i32 a2, i32 a3);
    // Base namespace loader chained first (reloc-masked near call).
    i32 LoadGameAssetNamespaces(i32, i32, i32);

    char m_pad1a8[0x1b4 - 0x1a8];
    CGMMenuUI* m_1b4;  // +0x1b4 the menu UI object the scans drive
    i32 m_1b8;         // +0x1b8 fade/poll duration
    CMenuMusic* m_1bc; // +0x1bc menu music controller (player + draw-clock gate)
    char m_pad1c0[0x1d0 - 0x1c0];
    i32 m_liveGame; // +0x1d0  live-game flag (FormatHudText getter-path gate)

    void BuildVersionString(i32, i32, i32, i32);
};

// CImageList-owning sub-object embedded at CCreditsState+0x1e8 (an MFC CImageList
// the credits screen builds). Modeled as the twin of Dialogs' CImgHolder: a
// CObject-ish grand-base (vtable 0x5e8cb4) + the derived holder (vtable 0x5e8cd4)
// whose virtual dtor frees the image list (CImageList::DeleteImageList @0x1c6a5c).
// Real-virtual model - cl emits the implicit ??_7 stamps (reloc-masked); the base
// dtor folds into ~CCreditsState as the final base-vptr restore.
SIZE_UNKNOWN(CCreditsImgBase);
struct CCreditsImgBase {
    virtual ~CCreditsImgBase() {}
};
SIZE_UNKNOWN(CCreditsImageList);
struct CCreditsImageList : CCreditsImgBase {
    void DeleteImageList(); // 0x1c6a5c (NAFXCW, reloc-masked)
    // Inline so ~CCreditsState folds the stamp/DeleteImageList/base-restore teardown
    // (retail inlines it; an out-of-line ??1 would emit a `call` and shrink the frame).
    virtual ~CCreditsImageList() OVERRIDE {
        DeleteImageList();
    }
    void* m_hImageList; // +0x04
};

// CCreditsState - the credits state. Render
// is the canonical Render spine: input poll -> input-virtual bail -> cursor anim
// -> per-entity Update loop -> message scan -> two sub-steps -> draw -> two
// latched one-shot FX.
SIZE_UNKNOWN(CCreditsState);
class CCreditsState : public CState {
public:
    // Own vtable slots (RTTI vtbl@0x5e9c64, 26 slots; slot order anchored by
    // CState). Out-of-line dtor (0x8d5e0, GameMode.cpp): runs ReleaseResources then
    // cl auto-destroys the m_caption CString + the m_1e8 image list before chaining
    // ~CState. Slots whose bodies live in another TU (0x39160 shared with
    // CAttract::RefreshTitle; 0x39440/0x394b0 in ApiCallers) or are deferred are
    // declared-only (the vtable references them reloc-masked; the vtable isn't diffed).
    virtual ~CCreditsState() OVERRIDE;        // slot 0  0x08d5e0 (??1) / 0x08d5b0 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x038f00 credits teardown
    virtual GameStateId Update() OVERRIDE;    // slot 4  (+0x10) 0x08d590 GAMESTATE_CREDITS (8)
    virtual i32 Render() OVERRIDE;            // slot 5  (+0x14) 0x0391d0 per-frame credits draw
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x039400 (declared-only: Vfunc3-gated roll)
    virtual i32 InputVirtual()
        OVERRIDE;                      // slot 8  (+0x20) 0x0393b0 per-frame input poll (title gate)
    virtual i32 Vslot09(i32) OVERRIDE; // slot 9  (+0x24) 0x039120 (declared-only)
    virtual i32 FrameSlot28(i32) OVERRIDE; // slot 10 (+0x28) 0x039160 (declared-only; shared body)
    virtual i32 Vslot0c(i32, i32)
        OVERRIDE; // slot 12 (+0x30) 0x039440 (declared-only: ESC/SPC/ENTER cmd)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x0394b0 (declared-only)

    // CCreditsState's own sub-steps (the rel32 thunks Render dispatches to with
    // `mov ecx,this`). External no-body -> reloc-masked.
    void Sub1();
    void Sub2();
    void Sub3();

    i32 DrawScrollingCredits(); // 0x396f0 per-frame credits scroll-text renderer

    // FinishState (0x39c40): clear the playing gate, ret 1. StepVideo (0x39c60):
    // advance the Smacker movie frame + blit it. FlashColor (0x39d00): re-roll a
    // random RGB flash latch when its timer expires.
    i32 FinishState();
    i32 StepVideo();
    i32 FlashColor();

    // --- CCreditsState members the Render path pins (placeholders) ---
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_1b4; // +0x1b4 one-shot FX latch
    i32 m_1b8; // +0x1b8 packed random RGB flash color
    i32 m_1bc; // +0x1bc flash re-roll timer
    char m_pad1c0[0x1c4 - 0x1c0];
    i32 m_1c4; // +0x1c4 conditional-FX gate
    char m_pad1c8[0x1e8 - 0x1c8];
    CCreditsImageList m_1e8; // +0x1e8 embedded image list (freed by ~CCreditsState)
    CString m_caption;       // +0x1f0 credits caption CString (freed by ~CCreditsState)
    char m_pad1f4[0x208 - 0x1f4];
    i32 m_videoPlaying; // +0x208 video playing gate
    char m_pad20c[0x210 - 0x20c];
    CCreditsVideo* m_videoHandle; // +0x210 Smacker video handle

    i32 LoadCreditzStateAssets(i32 a1, i32 a2, i32 a3); // 0x38d20 (slot 1, called non-virtually)
    i32 InitAttractTitle();
    // ShowAttractTitle (0x393b0) is the slot-8 InputVirtual override (declared above).

    // Own attract-title tail helpers reached via ILT thunks (reloc-masked
    // self-calls; formerly the CAttractSelf `this`-alias view).
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x1fa1f0
    i32 BuildMenuPage(i32 x, i32 w, i32 h, i32 flag);               // 0x1fa8f0
};

SIZE_UNKNOWN(CBootyState);
class CBootyState : public CState {
public:
    // Own vtable slots (RTTI vtbl@0x5e9cec, 26 slots; slot order anchored by CState).
    // CBootyState shares many slot bodies with its siblings CMultiBootyState /
    // CBootyCheatState (0x1ce30/0x1d420 own CMultiBootyState methods, 0x18830 is
    // CBootyCheatState::LoadAssets, 0x18d30/0x1c8a0 live in the booty-activate/
    // state-image TUs); those and the deferred slots are declared-only here (the
    // vtable references them reloc-masked - the vtable itself is not diffed). The
    // EH-framed `??1` (slot 0) re-stamps the CBootyState vtable, runs the slot-2
    // release (statically bound), re-stamps CState, chains base cleanup.
    virtual ~CBootyState() OVERRIDE;          // slot 0  0x08d440 (??1) / 0x08d410 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x018c90 booty teardown
    virtual GameStateId Update() OVERRIDE;    // slot 4  (+0x10) 0x08d3f0 GAMESTATE_BOOTY (0xa)
    virtual i32 Render() OVERRIDE;  // slot 5  (+0x14) 0x01c210 per-frame booty draw (stub)
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x01ce10 (declared-only)
    virtual i32 Vslot07() OVERRIDE; // slot 7  (+0x1c) 0x01ce30 (declared-only; sib ReadyAndPaint)
    virtual i32 InputVirtual() OVERRIDE;   // slot 8  (+0x20) 0x01c8a0 (declared-only; StateImages)
    virtual i32 Vslot09(i32) OVERRIDE;     // slot 9  (+0x24) 0x018d30 (declared-only; vfunc_9)
    virtual i32 FrameSlot28(i32) OVERRIDE; // slot 10 (+0x28) 0x018e40 (declared-only)
    virtual i32 Vslot0c(i32, i32)
        OVERRIDE; // slot 12 (+0x30) 0x01d420 (declared-only; sib ForwardIdleAnim)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x01d3e0 (declared-only)
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17 (+0x44) 0x01d400 (declared-only)

    // Slot 1 (0x18830, CBootyCheatState::LoadAssets, (int,int,int)) is reached
    // non-virtually and its shape differs from CState's slot-1 placeholder, so it is
    // not modeled as a CBootyState virtual.

    // Non-virtual engine-label backlog stub (0x1d440; vtable-neutral).
    void vfunc_1();

    // Booty-title tail helpers reached via ILT thunks (reloc-masked; the byte match is
    // name-independent). These are SHARED bodies owned by other classes but dispatched
    // with a CBootyState `this` (same functions its sibling CMultiBootyState calls):
    //   FadeInTitle == CAttract::FadeInTitle (0xfa1f0), BuildPage == CSoundFxEmitter's
    //   0xfa8f0. ShowSecretBonusMessage/ShowLevelCompleteMessage are the booty-toast
    //   helpers the slot-8 activator (StateImages.cpp) pops. Homed here so StateImages'
    //   and BootyStateActivate's local CBootyState views fold onto this canonical class.
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0xfa1f0
    i32 BuildPage(i32 a, i32 b, i32 c, i32 d);                      // 0xfa8f0
    void ShowSecretBonusMessage();                                 // 0xfa540
    void ShowLevelCompleteMessage();                               // 0xfa120

    // --- CBootyState members (placeholders, beyond the CState layout) ---
    char m_pad1a8[0x1bc - 0x1a8];
    i32 m_activation; // +0x1bc  activation discriminator (==200 -> secret-bonus toast)
};

// CMultiBootyState - the MULTIPLAYER booty/bonus state (RTTI .?AVCMultiBootyState@@,
// vtable @0x5e9bdc; a SIBLING of CBootyState (.?AVCBootyState@@, vtable @0x5e9cec),
// NOT the same class - the trace conflated the two under "CBootyState"). Drives the
// "multi"/"BOOTY_LOOP"/"BOOTY_PERFECT" cue set. Its slots line up with CState's
// (ReleaseResources slot 2, the dtor slot 0); on top of the CState layout it owns a
// glitter/spawn animation block at +0x1b4..+0x208 (a count, a phase, two scratch
// coords, two ptr arrays @+0x1ec/+0x204, a target @+0x1fc) and a state object @+0x2f8.
SIZE_UNKNOWN(CMultiBootyState);
class CMultiBootyState : public CState {
public:
    // Own vtable slots (RTTI vtbl@0x5e9bdc, 26 slots; slot order anchored by CState).
    // The EH-framed `??1` (slot 0, @0x8d510) re-stamps the CMultiBootyState vtable,
    // runs the slot-2 release (statically bound), re-stamps CState, chains BaseCleanup.
    // Slots whose bodies live in another TU (slot 8 == OnActivate2 in the booty-activate
    // TU; slot 1 == 0x1d440, shared with CBootyState::vfunc_1) or are deferred are
    // declared-only (the vtable references them reloc-masked; the vtable isn't diffed).
    virtual ~CMultiBootyState() OVERRIDE;     // slot 0  0x08d510 (??1) / 0x08d4e0 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x01e520 booty teardown
    virtual GameStateId Update() OVERRIDE; // slot 4  (+0x10) 0x08d4c0 GAMESTATE_MULTIBOOTY (0x12)
    virtual i32 Render() OVERRIDE;       // slot 5  (+0x14) 0x01f480 (declared-only; per-frame draw)
    virtual i32 Vslot06() OVERRIDE;      // slot 6  (+0x18) 0x01f850 (declared-only)
    virtual i32 Vslot07() OVERRIDE;      // slot 7  (+0x1c) 0x01f870 (declared-only)
    virtual i32 InputVirtual() OVERRIDE; // slot 8  (+0x20) 0x01f6f0 (declared-only; OnActivate2)
    virtual i32 Vslot09(i32) OVERRIDE;   // slot 9  (+0x24) 0x01e570 per-frame cue poll (ret 4)
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10 (+0x28) 0x01e660 (declared-only)
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12 (+0x30) 0x01f920 (declared-only)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x01f8e0 (declared-only)
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17 (+0x44) 0x01f900 (declared-only)

    // Non-virtual behavioral methods (the rel32 thunks dispatched with mov ecx,this).
    i32 BuildWarpStoneGlitterAnimation(); // 0x19540 - build the 4 warp-letter glitter anims
    void StepGlitterAnim();               // 0x196c0 - the trig glitter/spawn positioner
    void MoveLettersByDir();              // 0x19b90 - the 8-direction letter walk (jump-table)
    i32 CheckPerfectBonus();              // 0x1c0f0 - "BOOTY_PERFECT" cue + scroll advance
    i32 QueryGruntSlots();                // 0x1ecf0 - scan 4 reg records for an empty slot

    // Own booty-title tail helpers reached via ILT thunks (reloc-masked self-calls;
    // formerly the CBootyAnimSelf `this`-alias view).
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // FUN_004fa1f0
    void BuildPage(i32 x, i32 w, i32 h, i32 flag);                  // FUN_004fa8f0
    // Slot-8 activator (OnActivate2 @0x1f6f0, booty-activate TU) tail helpers:
    // BaseOnActivate is the shared image-load gate (0xface0, == CImageState's
    // Unmatched_0face0 / CMgrPersistObj::Init), dispatched here with the state `this`;
    // OnActivated (0x1ed30) runs after the namespaces install. Reloc-masked externals.
    i32 BaseOnActivate(); // 0xface0
    void OnActivated();   // 0x1ed30

    // Ready-gate + paint (0x1ce30): if the active/ready virtual (CState slot 3) fires,
    // run the per-frame paint and return its normalized result, else 0.
    i32 ReadyAndPaint();                // 0x1ce30
    i32 ForwardIdleAnim(i32 a, i32 b);  // 0x1d420 -> BuildBootyGruntIdleAnimation
    i32 Paint();                        // 0xfac70 (reloc-masked engine paint)
    i32 BuildBootyGruntIdleAnimation(); // 0x1ce60 (reloc-masked, own method;
                                        // shares the forwarder's arg frame)

    // --- CMultiBootyState members (placeholders, beyond the CState layout) ---
    // The +0x1ec and +0x204 sprite-ptr arrays overlap (the two animators index the
    // same letter set from different bases) - accessed by offset in the bodies, not
    // declared as fields here. Only the directly-stored scalars are named.
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_1b4; // +0x1b4 anim-mode gate (0 = trig path, !=0 = table path)
    char m_pad1b8[0x1d8 - 0x1b8];
    i32 m_letterIdx; // +0x1d8 active letter count / index
    i32 m_radius;    // +0x1dc sine-spiral radius (loaded (float) for sin/cos; shrinks to 0)
    i32 m_angleStep; // +0x1e0 spiral angle/step counter (advances by 5)
    i32 m_scratchX;  // +0x1e4 computed scratch X (sin(ang)*r + tableX)
    i32 m_1e8;       // +0x1e8 computed scratch Y (cos(ang)*r + tableY)
    char m_pad1ec[0x1fc - 0x1ec];
    CGlitterAnim* m_cursorLetter; // +0x1fc the trailing/cursor letter sprite
    char m_pad200[0x2f8 - 0x200];
    CBootyBonusState* m_bonusState; // +0x2f8 the bonus state object (m_5c phase / m_8 flags)
};

#endif // SRC_GRUNTZ_GAMEMODE_H
