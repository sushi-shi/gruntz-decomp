#ifndef SRC_GRUNTZ_GAMEMODE_H
#define SRC_GRUNTZ_GAMEMODE_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <DDrawMgr/DDrawSurfaceMgr.h>

#include <Mfc.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

#include <Gruntz/AttractActor.h> // AttractActor / AttractActorList / g_actorList
typedef AttractActor CGMEntity;
typedef AttractActorList CGMEntityList;

// (CGMSound / CGMSoundEntry / CGMOwner DISSOLVED 2026-07-20: they were phantom views of
// the CState::m_4 CGruntzMgr. The credits/menu code now reaches the real classes cast-free:
// CGMOwner IS the CGruntzMgr (m_4 -> CGameMgr::m_gameWnd, m_8 -> CGameMgr::m_owner CGameApp,
// m_musicEnabled, m_48 -> CGruntzMgr::m_sound; Post IS CGruntzMgr::ReportError @0x8dc60);
// CGMSound IS CGruntzSoundZ (Play/Find -> PlayByName/FindBank); CGMSoundEntry IS the
// CGruntzSoundInnerZ bank FindBank returns (Query -> IsStarted @0x138a10).)

extern "C" void __stdcall GM_SimpleAnim(i32 z); // (stdcall, 1 arg)

#include <Gruntz/ChatBox.h>

struct CGMVerRect {
    i32 a, b, c, d;
};
SIZE_UNKNOWN();
extern "C" CGMVerRect g_versionRect; // (the 4-int source @c8/cc/d0/d4)
extern "C" i32 g_frameDelta;         // (last-frame delta, fed to Step)

struct BzGeomPair {
    i32 m_y; // +0x00  onscreen y
    i32 m_x; // +0x04  onscreen x
};
SIZE_UNKNOWN();

extern RECT g_levelMsgRectsB[8];

#include <Gruntz/State.h>
#include <Gruntz/View.h> // the CState::m_c render sub-object facets (CRenderer/CDrawSurface)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (the CState::m_c holder itself)
#include <DDrawMgr/DDrawSurfaceMgr.h> // its real sub-object classes (CDDrawSubMgrPages/CImageRegistry/CDDrawSubMgrLeafScan)

struct LeafCue;     // CMenuState::m_1bc - the menu-music sound cue (Gruntz/LeafCue.h)
class CMoviePlayer; // CCreditsState::m_videoHandle - real Smacker video player
struct CGameObject; // CMultiBootyState::m_cursorLetter + the +0x1ec/+0x204 letter-sprite arrays
class CWwdGameObjectA; // the created-sprite kind (sprite fields below hold it)

class CMenuState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CMenuState`, state id 5,
    // `push 0x1c0`); cl inlines this body at the new-site exactly as retail does.
    CMenuState() {
        m_1b4 = 0;
    }
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x0a0a30 (ex CAttract::Activate; defined in Attract.cpp)
    // slot 1  0x09fe50 (MenuStateAssets.cpp; retail ??_7CMenuState slot 1 = ILT
    // 0x32ec -> 0x9fe50, ex "LoadAssets") - the MENU asset loader: registers the
    // MENU IMAGEZ/SOUNDZ namespaces through the m_c (CDDrawSurfaceMgr) resource
    // facet, primes the state core, then builds the menu HUD object + wires its
    // keys/sound cues.
    virtual i32 LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) OVERRIDE;
    virtual i32 Vslot07() OVERRIDE; // slot 7  (+0x1c) 0x0a0d40 (ex ReadyGate: the &&-chained ready/transition probe - IsActive() && CommitState() ? Vslot06())
    virtual i32 InputVirtual() OVERRIDE; // slot 8  (+0x20) 0x0a09a0 (ex CImageState::LoadStateImages; defined in StateImages.cpp)
    virtual i32 Vslot09(i32) OVERRIDE; // slot 9  (+0x24) 0x0a03f0 (ex CAttract::LoadTitleConfig; defined in Attract.cpp)
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
    virtual i32 Vslot10(i32, i32, i32) OVERRIDE; // slot 16
    virtual i32 SetBeginClearParams(i32, i32, i32) OVERRIDE; // slot 20
    // The ~CMenuState() destructor (EH-framed `??1` under /GX): it re-stamps the
    // CMenuState vtable, runs the slot-2 resource release (ReleaseResources,
    // statically bound in the dtor), then re-stamps the CState vtable and chains
    // the base cleanup. Defined out-of-line (GameMode.cpp) so MSVC emits a
    // distinct `??1` the `??_G` deleting dtor dispatches to.
    virtual ~CMenuState() OVERRIDE;
    RVA(0x0008ce10, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MENU;
    }
    virtual i32 Render() OVERRIDE;             // the per-frame menu draw (this TU)
    virtual void ReleaseResources() OVERRIDE;  // slot 2 (+0x8) - menu teardown
    virtual i32 FrameSlot28(i32 arg) OVERRIDE; // slot 10 (+0x28) - per-frame poll

    // CMenuState's own methods (the rel32 thunks Render dispatches to with
    // `mov ecx,this`). External no-body -> reloc-masked.
    // (the version-banner draw at 0xa0d80 IS BuildVersionString below)

    // Non-virtual menu helpers (called from FrameSlot28 / its siblings).
    void StartMusic();     // 0xa05a0 - music start gate
    void StopMusicChain(); // 0xa0640 - stop + cue chain

    // CommitState (reached via the 0x1136 ILT thunk; external no-body ->
    // reloc-masked). Returns nonzero when the pending state commit succeeds.
    i32 CommitState();

    // (FormatHudText @0x1af70 is GONE from here - it is a CBootyState method; see the
    // proof on CBootyState below. It never belonged to CMenuState: it reads [this+0x1d0]
    // off its own `this`, and this class is 0x1c0.)

    // (The ex "LoadAssets" decl is GONE - it IS the slot-1 override above; its body
    // chains the base default via the qualified CState::LoadGameAssetNamespaces().)

    char m_pad1a8[0x1b4 - 0x1a8];
    CChatBox* m_1b4; // +0x1b4 the menu UI object the scans drive (the real CChatBox)
    i32 m_1b8;       // +0x1b8 fade/poll duration
    LeafCue*
        m_1bc; // +0x1bc menu-music sound cue (LeafCue: DSoundCloneInst m_10 + m_14/m_18 clock gate)
    // ENDS AT 0x1c0 - the allocation-proven size (TransitionState @0x8be11:
    // `push 0x1c0; call ??2@YAPAXI@Z`, then the ??_7CMenuState (0x5e9e84) stamp).
    // The out-of-bounds `m_liveGame` @+0x1d0 (and its 0x1c0..0x1d0 pad) that used to sit
    // here is GONE: it was never CMenuState's. It is CBootyState::m_initOnce @+0x1d0, read
    // by FormatHudText - a CBootyState method.

    void BuildVersionString(CGMVerRect r); // 0xa0d80 (RECT by value; Render's tail draw)
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

void CreditsRectSet(void* rect, i32 l, i32 t, i32 r, i32 b); // 0x08c380

class CCreditsState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CCreditsState`, state id 8,
    // `push 0x218`); cl inlines this body at the new-site exactly as retail does (the
    // CRgn + CString members are constructed first, then the ??_7CCreditsState stamp,
    // then these field seeds).
    CCreditsState() {
        m_flashColor = 0;
        m_flashTimer = 0;
        m_fadeCountdown = 0;
        m_fxEnabled = 0;
        m_scrollReseedTimer = 0;
        m_scrollAccum = 0;
        m_scrollStep = 0;
        CreditsRectSet(&m_scrollRect, 0, 0, 0x280, 0x1e0);
        CreditsRectSet(&m_drawRect, 0, 0, 0x280, 0x1e0);
        m_20c = 1;
        m_videoHandle = 0;
        m_videoPlaying = 0;
        m_musicStarted = 0;
    }
    // slot 1  0x038d20 (CreditsState.cpp; retail ??_7CCreditsState slot 1 = ILT
    // 0x3954 -> 0x38d20, ex "LoadCreditzStateAssets") - the credits asset loader.
    virtual i32 LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) OVERRIDE;
    // Own vtable slots (RTTI vtbl@0x5e9c64, 26 slots; slot order anchored by
    // CState). Out-of-line dtor (0x8d5e0, GameMode.cpp): runs ReleaseResources then
    // cl auto-destroys the m_caption CString + the m_1e8 image list before chaining
    // ~CState. Slots whose bodies live in another TU (0x39160 shared with
    // CAttract::RefreshTitle; 0x39440/0x394b0 in ApiCallers) or are deferred are
    // declared-only (the vtable references them reloc-masked; the vtable isn't diffed).
    virtual ~CCreditsState() OVERRIDE;        // slot 0  0x08d5e0 (??1) / 0x08d5b0 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x038f00 credits teardown
    RVA(0x0008d590, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_CREDITS;
    }
    virtual i32 Render() OVERRIDE;  // slot 5  (+0x14) 0x0391d0 per-frame credits draw
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x039400 (declared-only: IsActive-gated roll)
    virtual i32 InputVirtual()
        OVERRIDE;                      // slot 8  (+0x20) 0x0393b0 per-frame input poll (title gate)
    virtual i32 Vslot09(i32) OVERRIDE; // slot 9  (+0x24) 0x039120 (declared-only)
    virtual i32 FrameSlot28(i32) OVERRIDE; // slot 10 (+0x28) 0x039160 (ex CAttract::RefreshTitle; defined in Attract.cpp)
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
    // LoadCreditzAssets (0x39dc0, CreditzAssets.cpp): the credits music-bank swap
    // tick - toggles m_1c4, (re)starts CREDITZ / hands MONOLITH to the mixer via
    // the canonical CGruntzSoundZ transport slots.
    void LoadCreditzAssets();

    // --- CCreditsState members the Render path pins (placeholders) ---
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_musicStarted; // +0x1b4 CREDITZ music one-shot latch
    i32 m_flashColor; // +0x1b8 packed random RGB flash color
    i32 m_flashTimer; // +0x1bc flash re-roll timer
    i32 m_fadeCountdown; // +0x1c0 fade countdown ms (LoadCreditzAssets arms 3000 on the rising edge)
    i32 m_fxEnabled; // +0x1c4 conditional-FX gate / credits-music toggle
    // The two 0x10-byte rect sub-objects the ctor Set-initialises to the full 640x480
    // screen (Set @0x08c380, 4 args). They are plain RECTs: SetupTitle SetRect()s the
    // master scroll rect and DrawTextA-measures into the working one; the per-frame
    // DrawScrollingCredits copies master -> working and scrolls it up.
    RECT m_scrollRect; // +0x1c8  master caption rect (Set(0,0,0x280,0x1e0); SetupTitle SetRect)
    RECT m_drawRect;   // +0x1d8  working/scrolled caption rect (DrawTextA target)
    CRgn m_1e8;        // +0x1e8 embedded GDI region (RTTI .?AVCRgn@@; freed by ~CCreditsState)
    CString m_caption; // +0x1f0 credits caption CString (freed by ~CCreditsState)
    i32 m_scrollReseedTimer;         // +0x1f4  scroll reseed timer (counts the frame delta down)
    // +0x1f8 / +0x200 are DOUBLES, not four ints: DrawScrollingCredits does
    // `fmul QWORD PTR [esi+0x200]` / `fadd|fstp QWORD PTR [esi+0x1f8]` and SetupTitle
    // seeds both through the x87 pipe. (The ctor's four zero-dword stores are how MSVC5
    // writes a 0.0 double when a zero register is live - not four int fields.)
    double m_scrollAccum;        // +0x1f8  scroll accumulator (pixels scrolled, fractional)
    double m_scrollStep;         // +0x200  scroll speed (reseeded from the measured text height)
    i32 m_videoPlaying;          // +0x208 video playing gate
    i32 m_20c;                   // +0x20c  = 1 in the ctor (video-enabled gate)
    CMoviePlayer* m_videoHandle; // +0x210 Smacker video player (real CMoviePlayer)
    // Tail padding to the TRUE retail object size. Ground truth is the operator-new size
    // in CGruntzMgr::TransitionState (0x8b960): `push 0x218; call ??2@YAPAXI@Z` @0x8bf7f,
    // then the inline `mov [esi],??_7CCreditsState@@6B@` (0x5e9c64) stamp. Without this
    // the class is 0x214 and cannot host the `new` that GruntzMgr.cpp performs.
    char m_pad214[0x218 - 0x214];

    // (The ex "LoadCreditzStateAssets" decl is GONE - it IS the slot-1 override
    // above; its body chains the base via CState::LoadGameAssetNamespaces().)
    i32 InitAttractTitle();
    // SetupTitle (0x39a60): pull the "CREDITZ" TXT section into m_caption, build the
    // clip region, measure the text to seed m_scrollRect / m_scrollStep. (Was hosted on
    // the CreditsState.cpp-local `CCreditzOwner` this-view; it is a CCreditsState method.)
    i32 SetupTitle();
    // ShowAttractTitle (0x393b0) is the slot-8 InputVirtual override (declared above).

    // (The ex "FadeInTitle"/"BuildMenuPage" local shadows @0xfa1f0/0xfa8f0 are
    // GONE - they ARE the inherited CState::FadeInTitle / CState::RetireScene
    // title-roll/transition helpers, reached cast-free.)
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CBootyState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CBootyState`, state id 10,
    // `push 0x320`); cl inlines this body at the new-site exactly as retail does.
    CBootyState() {
        m_1c0 = 0;
        m_1c8 = 0;
        m_1c4 = 0;
        m_1cc = 0;
        m_1b8 = 0;
        m_activation = 0x64;
        m_slot = 0;
        m_stepIndex = 0;
        m_walkStarted = 0;
        m_soundStarted = 0;
        m_initGate = 0;
        m_secretGate = 0;
        m_levelCompleteGate = 0;
        m_initOnce = 0;
        m_secretBannerOnce = 0;
        for (i32 t = 0; t < 4; t++) {
            m_trailSprites[t] = 0;
        }
        for (i32 i = 0; i < 8; i++) {
            m_readyFlags[i] = 0;
            m_templateFlags[i] = 0;
        }
    }
    // slot 1  0x018830 (BootyCheatState.cpp; retail ??_7CBootyState slot 1 = ILT
    // 0x3111 -> 0x18830, ex "Vfunc1"/"CBootyCheatState::LoadAssets") - the booty
    // asset/cheat-table loader. Chains the base via CState::LoadGameAssetNamespaces().
    virtual i32 LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) OVERRIDE;
    // Own vtable slots (RTTI vtbl@0x5e9cec, 26 slots; slot order anchored by CState).
    // CBootyState shares many slot bodies with its siblings (0x1ce30/0x1d420 own
    // CMultiBootyState methods, 0x18d30/0x1c8a0 live in the booty-activate/
    // state-image TUs); those and the deferred slots are declared-only here (the
    // vtable references them reloc-masked - the vtable itself is not diffed). The
    // EH-framed `??1` (slot 0) re-stamps the CBootyState vtable, runs the slot-2
    // release (statically bound), re-stamps CState, chains base cleanup.
    virtual ~CBootyState() OVERRIDE;          // slot 0  0x08d440 (??1) / 0x08d410 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x018c90 booty teardown
    RVA(0x0008d3f0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_BOOTY;
    }
    virtual i32 Render() OVERRIDE;  // slot 5  (+0x14) 0x01c210 per-frame booty draw (stub)
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x01ce10 (declared-only)
    virtual i32 Vslot07() OVERRIDE; // slot 7  (+0x1c) 0x01ce30 (ex ReadyAndPaint: ready-gate + per-frame paint; defined in BootyStateActivate.cpp)
    virtual i32 InputVirtual() OVERRIDE;   // slot 8  (+0x20) 0x01c8a0 (declared-only; StateImages)
    virtual i32 Vslot09(i32) OVERRIDE;     // slot 9  (+0x24) 0x018d30 (declared-only; vfunc_9)
    virtual i32 FrameSlot28(i32) OVERRIDE; // slot 10 (+0x28) 0x018e40 (declared-only)
    virtual i32 Vslot0c(i32, i32)
        OVERRIDE; // slot 12 (+0x30) 0x01d420 (declared-only; sib ForwardIdleAnim)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x01d3e0 (declared-only)
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17 (+0x44) 0x01d400 (declared-only)

    // (The ex "StateOnEnter" @0x1d440 decl is GONE from here - retail's ONLY
    // reference to 0x1d440 is ??_7CMultiBootyState slot 1 (ILT 0x2900), so it is
    // CMultiBootyState's LoadGameAssetNamespaces override, not a CBootyState
    // method. The ex "BuildPage" @0xfa8f0 alias is GONE too - it IS the inherited
    // CState::RetireScene, reached cast-free like FadeInTitle @0xfa1f0.)
    // The booty HUD/idle overlays + the per-frame walking-grunt tick (BootyMessages.cpp,
    // BootyWalkAnim.cpp). The Show* toasts are popped by the slot-8 activator
    // (StateImages.cpp::InputVirtual), which binds to these real CBootyState symbols.
    i32
    BuildBootyGruntIdleAnimation(); // 0x1ce60  the shared booty idle-anim builder Vslot0c tail-calls
    i32 ShowSecretBonusMessage();   // 0x18f00  the secret-bonus toast (ret int)
    void ShowLevelCompleteMessage(); // 0x1c9d0  the level-complete toast
    i32 BuildBootyWalkingGruntz();   // 0x1b450  one-time per-player idle/walk sprite setup
    i32 UpdateBootyWalkingGruntz();  // 0x1b690  per-frame walking-grunt state machine

    // --- the level-message HUD / effect-sprite trio, RE-HOMED here from CState ---
    // All three were CState-homed (and FormatHudText CMenuState-homed) behind a
    // `(CEffLoaderSelf*)this` view-cast; all three are binary-proven CBootyState methods:
    //   * 0x18830 (this class's vtable SLOT 1, LoadGameAssetNamespaces) is data-referenced at
    //     ??_7CBootyState@@6B@+0x4 - so its `this` IS a CBootyState. It calls
    //     LoadGruntEffectSprites via thunk 0x3b8e with `mov ecx,esi` (its own `this`).
    //   * LoadGruntEffectSprites WRITES m_icons at +0x2fc..+0x31c, and LevelMsgHudDriver
    //     reads [this+0x2c4] / [this+0x264] - all far beyond CState, which is the base of
    //     the 0x1c0 CMenuState and therefore <= 0x1c0. CState can host NONE of them.
    //   * LevelMsgHudDriver's only caller is Render (slot 5, 0x1c210) via thunk 0x24b4 with
    //     `mov ecx,esi`; FormatHudText's only two callers are LevelMsgHudDriver and
    //     ShowLevelCompleteMessage (0x1c9d0), both via thunk 0x238d with `mov ecx,esi`.
    //   * Every field the view modeled lands in one of this class's existing PADS at the
    //     same offset, and the three it already named agree exactly (view m_hudPhase@+0x1b4
    //     == m_initGate, m_shownA@+0x284 == m_readyFlags, m_shownB@+0x2a4 == m_templateFlags),
    //     with the view's top field ending at 0x31c - just inside the allocation-proven 0x320.
    i32 LoadGruntEffectSprites();              // 0x1a040  build the 8 effect icons + sprites
    i32 LevelMsgHudDriver();                   // 0x1a700  per-frame level-message HUD driver
    void FormatHudText(CString* buf, i32 sel); // 0x1af70  the 960-B stat-line formatter

    // --- the slot-1 asset loader's four-stage build chain, RE-HOMED here ---
    // LoadGameAssetNamespaces (slot 1) IS 0x18830 - data-referenced at ??_7CBootyState@@6B@+0x4. It was
    // defined as `CBootyCheatState::LoadAssets` on a .cpp-local view class that does not
    // exist as a type, and its build chain was five DECLARED-ONLY aliases (Init1..Init5) of
    // real, already-defined functions - fabricated symbols nothing could link. Every one is
    // called on this `this` (`mov ecx,esi`), and each of the three below has 0x18830 as its
    // ONLY caller, so all three are CBootyState methods. Two of the five needed no move
    // (LoadGruntEffectSprites, BuildBootyWalkingGruntz were already here) and the base
    // loader is CState::LoadGameAssetNamespaces (inherited).
    //   Init1 -> 0x19540 - was ?BuildWarpStoneGlitterAnimation@CMultiBootyState@@, a SIBLING
    //            class: a CBootyState method cannot call a sibling's method on its own this.
    //   Init2 -> 0x19920 - was ?BuildGruntSprintAnimation@CGruntSprintAnim@@ (a view class).
    //   Init5 -> 0x1c070 - was ?BuildBootyPerfectAnimation@EngineLabelBacklog@@ (a stub class).
    i32 BuildWarpStoneGlitterAnimation(); // 0x19540  the 4 warp-letter glitter anims
    i32 BuildGruntSprintAnimation();      // 0x19920  the 8 directional sprint sprites
    i32 BuildBootyPerfectAnimation();     // 0x1c070  the BOOTY_PERFECT celebration sprite
    // 0x1c0f0 - scroll that same sprite in from off-screen left (x = -0x82, the very value
    // BuildBootyPerfectAnimation spawns it at) by 0xa a frame, cueing BOOTY_PERFECT on the
    // frame it appears and latching the done bit past x >= 0x302. RE-HOMED from
    // CMultiBootyState (0x244), which cannot hold the [this+0x2f8] it reads off its own
    // `this`; its only caller is this class's Render (slot 5).
    i32 CheckPerfectBonus();

    // 0x19cd0 - the per-selector (1..8) random edge spawn position BuildGruntSprintAnimation
    // uses for each compass direction. It is a MEMBER of this class, not the free __stdcall
    // it was modeled as: its body never touches ecx, but its ONLY caller (0x19920) sets
    // `mov ecx,ebp` (its own `this`) right before the call - that `mov` only exists if the
    // original source called it as a member. A member whose body ignores `this` compiles
    // byte-identically to __stdcall (same [esp+4..0xc] args, same `ret 0xc`), so the callee
    // cannot tell you - only the CALL SITE can, and it says member.
    void GenMenuRandPos(i32 sel, i32* outX, i32* outY);

    // --- CBootyState members (offsets are the ctor ground truth). m_activation@+0x1bc
    // doubles as the overlay/animation state
    // id (0xc7/0xc8/-2 in the idle-anim tick, ==200 -> secret-bonus toast in slot 8).
    // The +0xc HUD sink the booty toasts read IS the inherited CState::m_c holder.
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_initGate;   // +0x1b4  init/step gate (armed flag)
    i32 m_1b8;        // +0x1b8  cleared by the slot-1 loader before the build chain
    i32 m_activation; // +0x1bc  activation / overlay-animation state id
    // +0x1c0..+0x1d0: the four mode words the slot-1 loader stamps last
    // (m_1c8 = 0x21, m_1cc = 0, m_1c0 = g_frameTime, m_1c4 = 0).
    i32 m_1c0;
    i32 m_1c4;
    i32 m_1c8;
    i32 m_1cc;
    i32 m_initOnce;         // +0x1d0  init-once gate
    i32 m_secretBannerOnce; // +0x1d4  secret-banner once gate
    // +0x1d8..+0x1ec: the warp-stone glitter block BuildWarpStoneGlitterAnimation drives
    // (it landed in what was pure padding here; the same names its old CMultiBootyState
    // attribution used, which declares a parallel block at these very offsets).
    i32 m_letterIdx;                // +0x1d8  active letter count / index
    i32 m_radius;                   // +0x1dc  sine-spiral radius (loaded (float) for sin/cos)
    i32 m_angleStep;                // +0x1e0  spiral angle/step counter (advances by 5)
    i32 m_scratchX;                 // +0x1e4  computed scratch X
    i32 m_scratchY;                      // +0x1e8  computed scratch Y
    CWwdGameObjectA* m_trailSprites[4]; // +0x1ec  the 4 warp-letter glitter / trailing idle sprites
                                    //         (the `(char*)this + 0x1ec` array of 0x19540)
    CWwdGameObjectA* m_cursorLetter;    // +0x1fc  the trailing/cursor letter sprite
    i32 m_levelCompleteGate;        // +0x200  level-complete gate
    // +0x204..+0x224: the 8 directional sprint sprites BuildGruntSprintAnimation builds -
    // exactly filling what was padding (8 pointers = 0x20 bytes).
    CWwdGameObjectA* m_sprintSprites[8]; // +0x204
    // The level-message HUD sprite banks (each lands in what was pure padding here, so
    // no declared field moved).
    CWwdGameObjectA* m_bomb[8];   // +0x224  bomb-grunt sprites (slide left)
    CWwdGameObjectA* m_gokart[8]; // +0x244  go-kart sprites (slide right)
    CWwdGameObjectA* m_expl[8];   // +0x264  explosion sprites (latched active once landed)
    // +0x284 / +0x2a4: the view called these m_shownA / m_shownB - the SAME two latches,
    // at the same offsets, that this class already named. Canonical names kept; the roles
    // LevelMsgHudDriver proves are recorded here.
    i32 m_readyFlags[8];           // +0x284  per-slot "stat line (rectsB) shown" latch
    i32 m_templateFlags[8];        // +0x2a4  per-slot "level message (rectsA) shown" latch
    i32 m_slot;                    // +0x2c4  active reveal slot / phase counter (0..8)
    CWwdGameObjectA* m_visSprites[4];  // +0x2c8  per-player idle sprites (visibility)
    CWwdGameObjectA* m_animSprites[4]; // +0x2d8  per-player idle sprites (A-kind)
    i32 m_stepIndex;               // +0x2e8  active-player step index
    i32 m_walkStarted;             // +0x2ec  walk-animation-started gate
    i32 m_soundStarted;            // +0x2f0  sound-started gate
    i32 m_secretGate;              // +0x2f4  secret-message gate
    // +0x2f8  the BOOTY_PERFECT celebration sprite BuildBootyPerfectAnimation creates.
    // This is the LAST hole in the class, and it closes the third out-of-bounds
    // @identity-TODO: CMultiBootyState also declares a `m_bonusState` here, but that class
    // is allocation-proven 0x244 - +0x2f8 is 0xb4 bytes past its end. The "bonus state
    // object (m_5c phase / m_8 flags)" IS this sprite: CGameObject's m_flags (+0x08) and
    // m_screenX (+0x5c) - the scroll "phase" is literally the sprite's screen x, wrapping
    // off-screen left at -0x82. Same object, two names.
    CWwdGameObjectA* m_bootyPerfectSprite;
    // +0x2fc  the eight in-game effect icons LoadGruntEffectSprites populates
    // ([0]=stopwatch [1]=exit [2]=deathtwitch [3]=gauntletz [4]=beachballz [5]=roidz
    //  [6]=coin [7]=wormhole/teleporter); LevelMsgHudDriver indexes them by m_slot.
    // Ends at 0x31c - inside the allocation-proven 0x320, which is what pins these
    // methods to THIS class rather than any smaller state.
    CWwdGameObjectA* m_icons[8]; // +0x2fc
    // Tail padding to the TRUE retail size: TransitionState `push 0x320; call ??2` @0x8bebc,
    // then the inline `mov [esi],??_7CBootyState@@6B@` (0x5e9cec) stamp.
    char m_pad31c[0x320 - 0x31c];
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CMultiBootyState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CMultiBootyState`, state id 18,
    // `push 0x244`); cl inlines this body at the new-site exactly as retail does.
    CMultiBootyState() {
        m_1b4 = 0;
        m_1b8 = 0x64;
    }
    // slot 1  0x01d440 (BootyStateActivate.cpp; retail ??_7CMultiBootyState slot 1
    // = ILT 0x2900 -> 0x1d440, ex "CBootyState::StateOnEnter" - a mis-attribution:
    // 0x1d440 appears in NO other vtable and has no direct caller, so it is this
    // class's own slot-1 loader). Body is a @stub (0xd7d B, unreconstructed).
    virtual i32 LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) OVERRIDE;
    // Own vtable slots (RTTI vtbl@0x5e9bdc, 26 slots; slot order anchored by CState).
    // The EH-framed `??1` (slot 0, @0x8d510) re-stamps the CMultiBootyState vtable,
    // runs the slot-2 release (statically bound), re-stamps CState, chains
    // CState::ReleaseResources.
    // Slots whose bodies live in another TU (slot 8 == OnActivate2 in the
    // booty-activate TU) or are deferred are declared-only (the vtable references
    // them reloc-masked; the vtable isn't diffed).
    virtual ~CMultiBootyState() OVERRIDE;     // slot 0  0x08d510 (??1) / 0x08d4e0 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x01e520 booty teardown
    RVA(0x0008d4c0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MULTIBOOTY;
    }
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
    // (BuildWarpStoneGlitterAnimation @0x19540 is GONE from here - it is a CBootyState
    // method. Its ONLY caller is 0x18830, CBootyState's own vtable slot 1, calling it with
    // `mov ecx,esi` - its own `this`. CMultiBootyState is a SIBLING of CBootyState, not a
    // base, so a CBootyState method could never have called it on itself.)
    void StepGlitterAnim();  // 0x196c0 - the trig glitter/spawn positioner
    void MoveLettersByDir(); // 0x19b90 - the 8-direction letter walk (jump-table)
    // 0x1ed30 - the BATTLE-STATZ scoreboard draw (its m_c @+0x0c is CState::m_c at the
    // same offset). Both call sites
    // (Render 0x1f480 / InputVirtual 0x1f6f0) invoke it with `mov ecx,this` on their own
    // CMultiBootyState `this`, and no other caller exists (sema xref: only its ILT thunk).
    void DrawBattleStats();
    // (CheckPerfectBonus @0x1c0f0 is GONE from here - it is CBootyState::, proven by its
    // [this+0x2f8] reads and its sole caller CBootyState::Render.)
    i32 QueryGruntSlots(); // 0x1ecf0 - scan 4 reg records for an empty slot

    // (The ex "BuildPage" @0xfa8f0 alias is GONE - it IS the inherited
    // CState::RetireScene; FadeInTitle @0xfa1f0 likewise inherited, cast-free.)
    // Slot-8 activator (OnActivate2 @0x1f6f0, booty-activate TU) tail helper:
    // OnActivated (0x1ed30) runs after the namespaces install. (The shared image-load
    // gate at 0xface0 is CState's slot-8 base virtual - reached via CState::InputVirtual(),
    // not a CMultiBootyState-local BaseOnActivate alias; see StateImages / Attract.cpp.)
    void OnActivated(); // 0x1ed30

    // 0x1d420 is CBootyState::Vslot0c (vtable slot 12), homed out-of-line (matcher-5);
    // this non-virtual CMultiBootyState alias is kept decl-only (no RVA) for callers.
    i32 ForwardIdleAnim(i32 a, i32 b);
    i32 Paint();                        // 0xfac70 (reloc-masked engine paint)
    i32 BuildBootyGruntIdleAnimation(); // 0x1ce60 (reloc-masked, own method;
                                        // shares the forwarder's arg frame)
    // 0x1f8a0: post WM_COMMAND 0x8023 when the m_1b8 latch reads 0xc7. The slot-12/14/17
    // forwarders tail-call it on `this`.
    i32 PostCommandIfKey();

    // --- CMultiBootyState members (placeholders, beyond the CState layout) ---
    // The +0x1ec and +0x204 sprite-ptr arrays overlap (the two animators index the
    // same letter set from different bases) - accessed by offset in the bodies, not
    // declared as fields here. Only the directly-stored scalars are named.
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_1b4; // +0x1b4 anim-mode gate (0 = trig path, !=0 = table path)
    i32 m_1b8; // +0x1b8 Render's one-shot battle-stats latch (0x64 -> draws once -> 0xc7)
    char m_pad1bc[0x1d8 - 0x1bc];
    i32 m_letterIdx; // +0x1d8 active letter count / index
    i32 m_radius;    // +0x1dc sine-spiral radius (loaded (float) for sin/cos; shrinks to 0)
    i32 m_angleStep; // +0x1e0 spiral angle/step counter (advances by 5)
    i32 m_scratchX;  // +0x1e4 computed scratch X (sin(ang)*r + tableX)
    i32 m_scratchY;       // +0x1e8 computed scratch Y (cos(ang)*r + tableY)
    CWwdGameObjectA* m_trailSprites[4]; // +0x1ec  the 4 warp-letter glitter / trailing idle
                                    //         sprites (walked 0..m_letterIdx, %4-bounded)
    CWwdGameObjectA* m_cursorLetter;    // +0x1fc the trailing/cursor letter sprite
    // ENDS AT 0x244 - the allocation-proven size (TransitionState @0x8c056:
    // `push 0x244; call ??2@YAPAXI@Z`, then the ??_7CMultiBootyState (0x5e9bdc) stamp).
    // The out-of-bounds `m_bonusState` @+0x2f8 that used to sit here is GONE, and so is its
    // reader (CheckPerfectBonus @0x1c0f0): both were CBootyState's. That body reads
    // [this+0x2f8] off its own `this`, 0xb4 bytes past this class's end, and its only caller
    // is CBootyState::Render. This class is whole again - the fourth and last of the
    // out-of-bounds @identity-TODOs to close by the allocation-site bound.
    i32 m_levelCompleteGate;         // +0x200  level-complete gate (mirrors CBootyState)
    CWwdGameObjectA* m_sprintSprites[8]; // +0x204..+0x223  the 8 directional sprint sprites
    char m_pad224[0x244 - 0x224];
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_GAMEMODE_H
