// GameMode.h - the game-state ("mode") hierarchy that the per-frame tick drives.
//
// CGruntzMgr::PerFrameTick (0x8b740, matched in the `rezmgr` unit) holds
// the active game-state object at CGruntzMgr+0x2c (m_curState) and each frame calls:
//     int  m_curState->vtbl[+0x10]()   (slot 4) = Update()  -> a state-ID/status int
//     int  m_curState->vtbl[+0x14]()   (slot 5) = Render()  -> the per-frame step+draw
// (PerFrameTick gates timing on `Update() != 0x11` and gates Render on
// m_renderGate.) This file reconstructs that state hierarchy.
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
// CRgn (the CCreditsState +0x1e8 embed) - RTTI .?AVCRgn@@ @0x1ea2a4. Skip the
// afxwin*.inl bodies for the CLANG LABEL STEP only (implicit-int CMenu::op==);
// wine cl keeps the inlines. docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

// A per-frame entity (the g_actorList element) and the list itself. These WERE defined
// here as CGMEntity/CGMEntityList - a second, byte-identical model of the classes
// <Gruntz/AttractActor.h> already carries as AttractActor/AttractActorList (same vtable
// slots, same +0x2ac flag word, same {pad,count,ptr-array} list at the same global). One
// class, two names, and the global they hang off had no definition under EITHER name.
// Unified: the shape lives in AttractActor.h and the old names are aliases of it.
#include <Gruntz/AttractActor.h> // AttractActor / AttractActorList / g_actorList
typedef AttractActor CGMEntity;
typedef AttractActorList CGMEntityList;

// (The list's element array is m_data[] on AttractActorList; the Render loops load the
// global first: `mov reg,[0x645574]; mov cnt,[reg+4]; elems = reg+8`. The old spelling
// `g_645574` is gone - the one symbol is g_actorList, declared in AttractActor.h.)

// (The former CGMInputObj "input/anim sub-object" view - a 24-filler fake vtable
// with a __stdcall "Poll" at slot 24 (+0x60) - is GONE. The object at
// m_c->m_drawTarget->m_10->m_2c->m_8 is the game's real IDirectDrawSurface and the
// "poll" is IDirectDrawSurface::IsLost (COM slot 24, +0x60, __stdcall) - exactly how
// CreditsState.cpp / SplashState.cpp already dispatch the same path via <ddraw.h>.
// Nothing referenced the view anymore.)

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
// shared CSpriteFactoryHolder (<Gruntz/View.h>): m_c->m_4->m_10->m_2c->m_8 (input obj), the draw
// block m_c->m_4->{m_10->m_2c (Draw), m_14 (Blit), m_18 (blit arg)}, m_28->m_2c
// (cursor gate). Reached through m_c directly (no cast).

// CMenuState's m_1b4 menu-UI object IS a CChatBox (the entity-flag scans fire its
// OnFlag*/Step/Pre/Post/HitTest* front-end drive; the delete path runs ~CChatBox).
// The former CGMMenuUI view is folded onto the canonical class here.
#include <Gruntz/ChatBox.h>

// The version-string RECT source globals (4 ints copied to a stack RECT by value).
SIZE_UNKNOWN(CGMVerRect);
struct CGMVerRect {
    i32 a, b, c, d;
};
extern "C" CGMVerRect g_versionRect; // (the 4-int source @c8/cc/d0/d4)
extern "C" u32 g_frameDelta;         // (last-frame delta, fed to Step)

// A {y,x} onscreen-coordinate pair (the booty idle-sprite geometry table @0x5e8fe4;
// DATA home BootyMessages.cpp). Homed here from that TU (per-TU view -> owner header).
SIZE_UNKNOWN(BzGeomPair);
struct BzGeomPair {
    i32 m_y; // +0x00  onscreen y
    i32 m_x; // +0x04  onscreen x
};

// The SecretColor -> handle color table (the CBootyState/wormhole-tint local table;
// DATA home GameMode.cpp). Homed here from that TU.
SIZE_UNKNOWN(CGlitterColorTable);
struct CGlitterColorTable {
    char m_pad00[0x14];
    i32 m_arr14[1]; // +0x14  SecretColor -> handle table
};
// The 8 booty-message layout RECTs (0x60b8f8; DATA home BootyMessages.cpp). Shared
// by CBootyState/CMultiBootyState layout code here (declared, not per-TU extern).
extern RECT g_levelMsgRectsB[8];

// (g_60ce90 / g_60ce74 were NOT globals: they are the .rdata STRING LITERALS
//  "CREDITZ" and "MONOLITH" - the credits cue/sound names - re-declared by a previous
//  pass as extern char[] symbols that nothing defines. The literals are written at
//  their use sites in CreditsState.cpp; cl emits the same reloc-masked $SG entries.)

// ---------------------------------------------------------------------------
// CState - the base game-state class. One canonical definition, shared via
// <Gruntz/State.h> (full 41-slot vftable + ctor-pinned layout). The leaf states
// below derive from it; the gamemode TU casts the owner member (CGruntzMgr* m_4)
// to its own CGMOwner reconstruction and reaches the +0x0c CSpriteFactoryHolder resource facet
// (m_c, the shared <Gruntz/View.h> class) directly.
#include <Gruntz/State.h>
#include <Gruntz/View.h> // the CState::m_c render sub-object facets (CRenderer/CDrawSurface)
#include <Gruntz/GameRegistry.h> // CSpriteFactoryHolder (the CState::m_c holder itself)
#include <Gruntz/ResMgr.h> // its real sub-object classes (CDrawTarget/CImageRegistry/CSoundRegistry)

// Single-type leaf-state sub-object views, defined in GameMode.cpp; forward-
// declared so the leaf members below are typed to their real class (no per-site
// cast). Each is a pointer slot, so the typing is codegen-neutral.
struct LeafCue;     // CMenuState::m_1bc - the menu-music sound cue (Gruntz/LeafCue.h)
class CMoviePlayer; // CCreditsState::m_videoHandle - real Smacker video player
// (CBootyBonusState is GONE - there was never a "bonus state object": +0x2f8 holds the
// BOOTY_PERFECT CGameObject sprite, and its "phase" is that sprite's own m_screenX.)
struct CGameObject; // CMultiBootyState::m_cursorLetter + the +0x1ec/+0x204 letter-sprite arrays
                    // (the created "SimpleAnimation" sprite - the shared CGameObject; the booty
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
    // Constructed by CGruntzMgr::TransitionState (`new CMenuState`, state id 5,
    // `push 0x1c0`); cl inlines this body at the new-site exactly as retail does.
    CMenuState() {
        m_1b4 = 0;
    }
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual i32 Vslot07() OVERRIDE;              // slot 7
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
    virtual i32 Vslot10(i32, i32, i32) OVERRIDE; // slot 16
    virtual i32 Vslot14(i32, i32, i32) OVERRIDE; // slot 20
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
    // (the version-banner draw at 0xa0d80 IS BuildVersionString below - the former
    //  fake DrawVersion alias was folded away)

    // Non-virtual menu helpers (called from FrameSlot28 / its siblings).
    void StartMusic();     // 0xa05a0 - music start gate
    void StopMusicChain(); // 0xa0640 - stop + cue chain

    // ReadyGate (0xa0d40): the &&-chained ready/transition probe -
    // Vfunc3() (slot 3) && CommitState() (the 0x1136 thunk) ? Vslot06() (slot 6).
    i32 ReadyGate();
    // CommitState (reached via the 0x1136 ILT thunk; external no-body ->
    // reloc-masked). Returns nonzero when the pending state commit succeeds.
    i32 CommitState();

    // (FormatHudText @0x1af70 is GONE from here - it is a CBootyState method; see the
    // proof on CBootyState below. It never belonged to CMenuState: it reads [this+0x1d0]
    // off its own `this`, and this class is 0x1c0.)

    // MENU asset loader (0x9fe50, MenuStateAssets.cpp): registers the MENU
    // IMAGEZ/SOUNDZ namespaces through the m_c (CSpriteFactoryHolder) resource facet, primes the
    // state core, then builds the menu HUD object + wires its keys/sound cues.
    i32 LoadAssets(i32 a1, i32 a2, i32 a3);
    // Base namespace loader (0xf9ea0) inherited from CState (called cast-free).

    char m_pad1a8[0x1b4 - 0x1a8];
    CChatBox* m_1b4; // +0x1b4 the menu UI object the scans drive (the real CChatBox)
    i32 m_1b8;       // +0x1b8 fade/poll duration
    LeafCue*
        m_1bc; // +0x1bc menu-music sound cue (LeafCue: DSoundCloneInst m_10 + m_14/m_18 clock gate)
    // ENDS AT 0x1c0 - the allocation-proven size (TransitionState @0x8be11:
    // `push 0x1c0; call ??2@YAPAXI@Z`, then the ??_7CMenuState (0x5e9e84) stamp).
    // The out-of-bounds `m_liveGame` @+0x1d0 (and its 0x1c0..0x1d0 pad) that used to sit
    // here is GONE: it was never CMenuState's. It is CBootyState::m_initOnce @+0x1d0, read
    // by FormatHudText - a CBootyState method, now re-homed. This class is whole again.

    void BuildVersionString(CGMVerRect r); // 0xa0d80 (RECT by value; Render's tail draw)
};
VTBL(CMenuState, 0x1e9e84);

// The +0x1e8 embed is a REAL MFC CRgn (afxwin.h) - RTTI-proven: the vtable its
// out-of-line ctor COMDAT (??0CRgn @0x8c3b0, called by CGruntzMgr::TransitionState
// when it builds the credits state) stamps is 0x1ea2a4, whose COL names .?AVCRgn@@.
// ~CCreditsState's inlined member teardown is the inlined ~CRgn chain with the
// CRgn own-stamp dead-store-elided: stamp ??_7CGdiObject (0x1e8cd4), call
// CGdiObject::DeleteObject (0x1c6a5c - the former "CImageList::DeleteImageList"
// misname; its Detach calls afxMapHGDIOBJ and the indirect call goes through the
// GDI32 DeleteObject IAT slot), restamp ??_7CObject (0x1e8cb4). The former fake

// CCreditsState - the credits state. Render
// is the canonical Render spine: input poll -> input-virtual bail -> cursor anim
// -> per-entity Update loop -> message scan -> two sub-steps -> draw -> two
// latched one-shot FX.
SIZE_UNKNOWN(CCreditsState);
// The 4-arg `Set` the credits ctor calls on each of its two +0x1c8/+0x1d8 rect
// sub-objects (0x08c380, __thiscall on the sub-object; reloc-masked, no body here).
void CreditsRectSet(void* rect, i32 l, i32 t, i32 r, i32 b); // 0x08c380

class CCreditsState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CCreditsState`, state id 8,
    // `push 0x218`); cl inlines this body at the new-site exactly as retail does (the
    // CRgn + CString members are constructed first, then the ??_7CCreditsState stamp,
    // then these field seeds).
    CCreditsState() {
        m_1b8 = 0;
        m_1bc = 0;
        m_1c0 = 0;
        m_1c4 = 0;
        m_1f4 = 0;
        m_scrollAccum = 0;
        m_scrollStep = 0;
        CreditsRectSet(&m_scrollRect, 0, 0, 0x280, 0x1e0);
        CreditsRectSet(&m_drawRect, 0, 0, 0x280, 0x1e0);
        m_20c = 1;
        m_videoHandle = 0;
        m_videoPlaying = 0;
        m_1b4 = 0;
    }
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE; // slot 1
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
    // LoadCreditzAssets (0x39dc0, CreditzAssets.cpp): the credits music-bank swap
    // tick - toggles m_1c4, (re)starts CREDITZ / hands MONOLITH to the mixer via
    // the canonical CGruntzSoundZ transport slots.
    void LoadCreditzAssets();

    // --- CCreditsState members the Render path pins (placeholders) ---
    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_1b4; // +0x1b4 one-shot FX latch
    i32 m_1b8; // +0x1b8 packed random RGB flash color
    i32 m_1bc; // +0x1bc flash re-roll timer
    i32 m_1c0; // +0x1c0 fade countdown ms (LoadCreditzAssets arms 3000 on the rising edge)
    i32 m_1c4; // +0x1c4 conditional-FX gate / credits-music toggle
    // The two 0x10-byte rect sub-objects the ctor Set-initialises to the full 640x480
    // screen (Set @0x08c380, 4 args). They are plain RECTs: SetupTitle SetRect()s the
    // master scroll rect and DrawTextA-measures into the working one; the per-frame
    // DrawScrollingCredits copies master -> working and scrolls it up.
    RECT m_scrollRect; // +0x1c8  master caption rect (Set(0,0,0x280,0x1e0); SetupTitle SetRect)
    RECT m_drawRect;   // +0x1d8  working/scrolled caption rect (DrawTextA target)
    CRgn m_1e8;        // +0x1e8 embedded GDI region (RTTI .?AVCRgn@@; freed by ~CCreditsState)
    CString m_caption; // +0x1f0 credits caption CString (freed by ~CCreditsState)
    i32 m_1f4;         // +0x1f4  scroll reseed timer (counts the frame delta down)
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

    i32 LoadCreditzStateAssets(i32 a1, i32 a2, i32 a3); // 0x38d20 (slot 1, called non-virtually)
    i32 InitAttractTitle();
    // SetupTitle (0x39a60): pull the "CREDITZ" TXT section into m_caption, build the
    // clip region, measure the text to seed m_scrollRect / m_scrollStep. (Was hosted on
    // the CreditsState.cpp-local `CCreditzOwner` this-view; it is a CCreditsState method.)
    i32 SetupTitle();
    // ShowAttractTitle (0x393b0) is the slot-8 InputVirtual override (declared above).

    // Own attract-title tail helpers reached via ILT thunks (reloc-masked
    // self-calls; formerly the CAttractSelf `this`-alias view).
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x1fa1f0
    i32 BuildMenuPage(i32 x, i32 w, i32 h, i32 flag);               // 0x1fa8f0
};
VTBL(CCreditsState, 0x001e9c64);

SIZE_UNKNOWN(CBootyState);
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
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE; // slot 1
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
    RVA(0x0008d3f0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_BOOTY;
    }
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
    void StateOnEnter();

    // Booty-title tail helpers reached via ILT thunks (reloc-masked; the byte match is
    // name-independent). These are SHARED bodies owned by other classes but dispatched
    // with a CBootyState `this` (same functions its sibling CMultiBootyState calls):
    //   FadeInTitle @0xfa1f0 is a CState base method (inherited - no local shadow;
    //   its callers reach it cast-free), BuildPage == CSoundFxEmitter's 0xfa8f0
    //   (the booty idle-anim tick reaches the same 0xfa8f0 through BuildPage too).
    i32 BuildPage(i32 a, i32 b, i32 c, i32 d); // 0xfa8f0
    // The booty HUD/idle overlays + the per-frame walking-grunt tick (BootyMessages.cpp,
    // BootyWalkAnim.cpp) - the former BzState view folded onto this canonical class. The
    // Show* toasts are popped by the slot-8 activator (StateImages.cpp::InputVirtual),
    // which now binds to these real CBootyState symbols (was an @rva SYMBOL override).
    i32
    BuildBootyGruntIdleAnimation(); // 0x1ce60  the shared booty idle-anim builder Vslot0c tail-calls
    i32 ShowSecretBonusMessage();   // 0x18f00  the secret-bonus toast (ret int)
    void ShowLevelCompleteMessage(); // 0x1c9d0  the level-complete toast
    i32 BuildBootyWalkingGruntz();   // 0x1b450  one-time per-player idle/walk sprite setup
    i32 UpdateBootyWalkingGruntz();  // 0x1b690  per-frame walking-grunt state machine

    // --- the level-message HUD / effect-sprite trio, RE-HOMED here from CState ---
    // All three were CState-homed (and FormatHudText CMenuState-homed) behind a
    // `(CEffLoaderSelf*)this` view-cast; all three are binary-proven CBootyState methods:
    //   * 0x18830 (this class's vtable SLOT 1, Vfunc1) is data-referenced at
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
    // Vfunc1 (slot 1) IS 0x18830 - data-referenced at ??_7CBootyState@@6B@+0x4. It was
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

    // --- CBootyState members (offsets are the ctor ground truth; folded from the
    // former BzState view). m_activation@+0x1bc doubles as the overlay/animation state
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
    i32 m_1e8;                      // +0x1e8  computed scratch Y
    CGameObject* m_trailSprites[4]; // +0x1ec  the 4 warp-letter glitter / trailing idle sprites
                                    //         (the `(char*)this + 0x1ec` array of 0x19540)
    CGameObject* m_cursorLetter;    // +0x1fc  the trailing/cursor letter sprite
    i32 m_levelCompleteGate;        // +0x200  level-complete gate
    // +0x204..+0x224: the 8 directional sprint sprites BuildGruntSprintAnimation builds -
    // exactly filling what was padding (8 pointers = 0x20 bytes).
    CGameObject* m_sprintSprites[8]; // +0x204
    // The level-message HUD sprite banks (folded in from the CEffLoaderSelf view; each
    // lands in what was pure padding here, so no declared field moved).
    CGameObject* m_bomb[8];   // +0x224  bomb-grunt sprites (slide left)
    CGameObject* m_gokart[8]; // +0x244  go-kart sprites (slide right)
    CGameObject* m_expl[8];   // +0x264  explosion sprites (latched active once landed)
    // +0x284 / +0x2a4: the view called these m_shownA / m_shownB - the SAME two latches,
    // at the same offsets, that this class already named. Canonical names kept; the roles
    // LevelMsgHudDriver proves are recorded here.
    i32 m_readyFlags[8];           // +0x284  per-slot "stat line (rectsB) shown" latch
    i32 m_templateFlags[8];        // +0x2a4  per-slot "level message (rectsA) shown" latch
    i32 m_slot;                    // +0x2c4  active reveal slot / phase counter (0..8)
    CGameObject* m_visSprites[4];  // +0x2c8  per-player idle sprites (visibility)
    CGameObject* m_animSprites[4]; // +0x2d8  per-player idle sprites (animation)
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
    CGameObject* m_bootyPerfectSprite;
    // +0x2fc  the eight in-game effect icons LoadGruntEffectSprites populates
    // ([0]=stopwatch [1]=exit [2]=deathtwitch [3]=gauntletz [4]=beachballz [5]=roidz
    //  [6]=coin [7]=wormhole/teleporter); LevelMsgHudDriver indexes them by m_slot.
    // Ends at 0x31c - inside the allocation-proven 0x320, which is what pins these
    // methods to THIS class rather than any smaller state.
    CGameObject* m_icons[8]; // +0x2fc
    // Tail padding to the TRUE retail size: TransitionState `push 0x320; call ??2` @0x8bebc,
    // then the inline `mov [esi],??_7CBootyState@@6B@` (0x5e9cec) stamp.
    char m_pad31c[0x320 - 0x31c];
};
VTBL(CBootyState, 0x001e9cec);

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
    // Constructed by CGruntzMgr::TransitionState (`new CMultiBootyState`, state id 18,
    // `push 0x244`); cl inlines this body at the new-site exactly as retail does.
    CMultiBootyState() {
        m_1b4 = 0;
        m_1b8 = 0x64;
    }
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE; // slot 1
    // Own vtable slots (RTTI vtbl@0x5e9bdc, 26 slots; slot order anchored by CState).
    // The EH-framed `??1` (slot 0, @0x8d510) re-stamps the CMultiBootyState vtable,
    // runs the slot-2 release (statically bound), re-stamps CState, chains BaseCleanup.
    // Slots whose bodies live in another TU (slot 8 == OnActivate2 in the booty-activate
    // TU; slot 1 == 0x1d440, shared with CBootyState::StateOnEnter) or are deferred are
    // declared-only (the vtable references them reloc-masked; the vtable isn't diffed).
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
    // 0x1ed30 - the BATTLE-STATZ scoreboard draw (was the fake class CBattleStatsView,
    // whose lone field `m_c @+0x0c` is CState::m_c at the same offset). Both call sites
    // (Render 0x1f480 / InputVirtual 0x1f6f0) invoke it with `mov ecx,this` on their own
    // CMultiBootyState `this`, and no other caller exists (sema xref: only its ILT thunk).
    void DrawBattleStats();
    // (CheckPerfectBonus @0x1c0f0 is GONE from here - it is CBootyState::, proven by its
    // [this+0x2f8] reads and its sole caller CBootyState::Render.)
    i32 QueryGruntSlots(); // 0x1ecf0 - scan 4 reg records for an empty slot

    // Own booty-title tail helpers reached via ILT thunks (reloc-masked self-calls;
    // formerly the CBootyAnimSelf `this`-alias view). FadeInTitle @0xfa1f0 is a CState
    // base method (inherited - no local shadow; callers reach it cast-free).
    void BuildPage(i32 x, i32 w, i32 h, i32 flag); // FUN_004fa8f0
    // Slot-8 activator (OnActivate2 @0x1f6f0, booty-activate TU) tail helper:
    // OnActivated (0x1ed30) runs after the namespaces install. (The shared image-load
    // gate at 0xface0 is CState's slot-8 base virtual - reached via CState::InputVirtual(),
    // not a CMultiBootyState-local BaseOnActivate alias; see StateImages / Attract.cpp.)
    void OnActivated(); // 0x1ed30

    // Ready-gate + paint (0x1ce30): if the active/ready virtual (CState slot 3) fires,
    // run the per-frame paint and return its normalized result, else 0.
    i32 ReadyAndPaint(); // 0x1ce30
    // 0x1d420 is CBootyState::Vslot0c (vtable slot 12), homed out-of-line (matcher-5);
    // this non-virtual CMultiBootyState alias is kept decl-only (no RVA) for callers.
    i32 ForwardIdleAnim(i32 a, i32 b);
    i32 Paint();                        // 0xfac70 (reloc-masked engine paint)
    i32 BuildBootyGruntIdleAnimation(); // 0x1ce60 (reloc-masked, own method;
                                        // shares the forwarder's arg frame)
    // 0x1f8a0: post WM_COMMAND 0x8023 when the m_1b8 latch reads 0xc7 (the folded
    // PendingCmdKeyHost view). The slot-12/14/17 forwarders tail-call it on `this`.
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
    i32 m_1e8;       // +0x1e8 computed scratch Y (cos(ang)*r + tableY)
    char m_pad1ec[0x1fc - 0x1ec];
    CGameObject* m_cursorLetter; // +0x1fc the trailing/cursor letter sprite
    // ENDS AT 0x244 - the allocation-proven size (TransitionState @0x8c056:
    // `push 0x244; call ??2@YAPAXI@Z`, then the ??_7CMultiBootyState (0x5e9bdc) stamp).
    // The out-of-bounds `m_bonusState` @+0x2f8 that used to sit here is GONE, and so is its
    // reader (CheckPerfectBonus @0x1c0f0): both were CBootyState's. That body reads
    // [this+0x2f8] off its own `this`, 0xb4 bytes past this class's end, and its only caller
    // is CBootyState::Render. This class is whole again - the fourth and last of the
    // out-of-bounds @identity-TODOs to close by the allocation-site bound.
    char m_pad200[0x244 - 0x200];
};
VTBL(CMultiBootyState, 0x001e9bdc);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SRC_GRUNTZ_GAMEMODE_H
