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
struct CGMEntity {
    virtual void Gv0();
    virtual void Gv1();
    virtual void Gv2();
    virtual void Gv3();
    virtual void Update(); // slot 4 (+0x10) - per-entity per-frame step
    char m_pad4[0x2ac - 0x4];
    int m_2ac; // +0x2ac flag word (scanned with a bit mask)
};

// The per-frame entity set: count @+0x4, element-ptr array @+0x8. The global
// The global is a POINTER to this structure (the Render loops load it first:
// `mov reg,[0x645574]; mov cnt,[reg+4]; elems = reg+8`).
struct CGMEntityList {
    void* m_0;             // +0x00
    int m_count;           // +0x04
    CGMEntity* m_elems[1]; // +0x08 (the entity-ptr array)
};
extern "C" CGMEntityList* g_645574; // (a pointer to the list)

// The input/anim sub-object the credits poll reaches (m_c->m_4->m_10->m_2c->m_8).
// Its slot +0x60 is a fn-ptr the object is passed to as the explicit STACK arg
// (NOT in ecx) and the CALLEE cleans the stack (no `add esp,4` at the call site)
// -> __stdcall: `mov ecx,[obj]; push obj; call [ecx+0x60]`.
struct CGMInputVtbl;
struct CGMInputObj {
    CGMInputVtbl* vtbl;
}; // +0x00 vtable ptr
struct CGMInputVtbl {
    char m_pad0[0x60];
    int(__stdcall* Poll)(CGMInputObj* self); // +0x60
};

// The owner back-ptr (CState+0x4) the Render path dereferences. +0x4->+0x4 = the
// OS HWND (PostMessageA target); +0x8 a sub-object (m_244 cleared); +0x14 a view
// gate; +0x48 the sound manager; the credits "post & bail" is m_4->Post(...).
struct CGMSound {
    void Play(const char* name, int z); // (thiscall, 2 args)
    int Find(const char* name);         // (thiscall, 1 arg -> ptr)
};
struct CGMSoundEntry {
    int Query();
}; // (thiscall, no arg -> int)
struct CGMOwner {
    void Post(unsigned a, unsigned b); // (thiscall, 2 args)
    char p0[0x4];                      // +0x00
    struct M4 {
        char p0[0x4];
        HWND m_4;
    }* m_4; // +0x04 -> +0x04 = HWND
    struct M8 {
        char p0[0x244];
        int m_244;
    }* m_8; // +0x08 -> +0x244 latch
    char p0c[0x14 - 0x0c];
    void* m_14; // +0x14 view gate (0 -> skip FX)
    char p18[0x48 - 0x18];
    CGMSound* m_48; // +0x48 sound manager
};

// The cursor/anim object reached via m_c->m_28->m_2c (credits only). Callee-
// cleaned (no `add esp,4` at the call site) -> __stdcall.
extern "C" void __stdcall GM_SimpleAnim(int z); // (stdcall, 1 arg)

// The view/draw holder (CState+0xc). The credits input poll reaches
// m_c->m_4->m_10->m_2c->m_8 (the input obj); the draw block walks
// m_c->m_4->{m_10->m_2c (Draw this), m_14 (blit this), m_18 (blit arg)}.
struct CGMBlitTarget {
    void Blit(int arg);
}; // (thiscall)
struct CGMView {
    char p0[0x4]; // +0x00
    struct M4 {
        char p0[0x10];
        struct M10 {
            char p0[0x2c];
            struct M2c { // +0x2c the draw target (also holds the input obj)
                char p0[0x8];
                CGMInputObj* m_8; // +0x08 input obj (credits poll source)
                void Draw(int z); // (thiscall on this M2c)
            }* m_2c;
        }* m_10;             // +0x10
        CGMBlitTarget* m_14; // +0x14 blit this
        void* m_18;          // +0x18 blit arg
    }* m_4;                  // +0x04
    char p8[0x28 - 0x8];
    struct M28 {
        char p0[0x2c];
        int m_2c;
    }* m_28; // +0x28 cursor/anim gate
};

// The CMenuState UI object (m_1b4): each entity-flag scan fires a distinct no-arg
// method on it; the tail steps it (one arg = g_645584) + draws the version RECT.
struct CGMMenuUI {
    void OnFlag80000000();
    void OnFlag40000000();
    void OnFlag20000000();
    void OnFlag10000000();
    void OnFlag00000003();
    int OnFlag00000100();   // (-> int)
    void Step(unsigned dt); // (1 arg)
    void Pre();             // (no arg)
    void Post();            // (no arg)
};
// The version-string RECT source globals (4 ints copied to a stack RECT by value).
struct CGMVerRect {
    int a, b, c, d;
};
extern "C" CGMVerRect g_645cc8;   // (the 4-int source @c8/cc/d0/d4)
extern "C" unsigned int g_645584; // (last-frame delta, fed to Step)

// The two cue/sound-name string constants the credits one-shot FX reference.
extern "C" char g_60ce90[]; // "CREDITZ" (PlaySound name)
extern "C" char g_60ce74[]; // "MONOLITH" (FindSound name)

// ---------------------------------------------------------------------------
// CState - the base game-state class. One canonical definition, shared via
// <Gruntz/CState.h> (full 41-slot vftable + ctor-pinned layout). The leaf states
// below derive from it; the gamemode TU casts the owner/view members (void* m_4,
// CView* m_c) to its own CGMOwner/CGMView reconstructions.
#include <Gruntz/CState.h>

// ---------------------------------------------------------------------------
// The concrete leaf states. Each overrides Update() to return its own state-ID
// tag (the 6-byte stub) - the only override modeled here for the leaf match
// (their own vtables carry the heavy Render overrides, matched/carcassed
// separately). The in-game PLAY state CPlay lives in its own <Gruntz/CPlay.h>.
// ---------------------------------------------------------------------------

// CMenuState - the front-end menu state. Render
// (464 B) drives the per-frame menu: a per-entity Update pass, then six
// entity-flag scans each firing a distinct method on the menu UI object m_1b4,
// then the UI step + the on-screen version-string RECT draw.
class CMenuState : public CState {
public:
    virtual int Update() OVERRIDE; // return 5;  (slot 4)
    virtual int Render() OVERRIDE; // the per-frame menu draw (this TU)

    // CMenuState's own methods (the rel32 thunks Render dispatches to with
    // `mov ecx,this`). External no-body -> reloc-masked.
    void DrawVersion(CGMVerRect r); // (this, RECT by value)

    char m_pad1a8[0x1b4 - 0x1a8];
    CGMMenuUI* m_1b4; // +0x1b4 the menu UI object the scans drive

    void BuildVersionString(int, int, int, int);
};

// CCreditsState - the credits state. Render
// is the canonical Render spine: input poll -> input-virtual bail -> cursor anim
// -> per-entity Update loop -> message scan -> two sub-steps -> draw -> two
// latched one-shot FX.
class CCreditsState : public CState {
public:
    virtual int Update() OVERRIDE;       // return 8;  (slot 4)
    virtual int Render() OVERRIDE;       // the per-frame credits draw (this TU)
    virtual int InputVirtual() OVERRIDE; // slot 8 (+0x20) - polled each frame
                                         // (slots 6,7 inherited from CState)

    // CCreditsState's own sub-steps (the rel32 thunks Render dispatches to with
    // `mov ecx,this`). External no-body -> reloc-masked.
    void Sub1();
    void Sub2();
    void Sub3();

    // Engine-label backlog stub: the scalar-deleting dtor. A
    // non-virtual placeholder so the carefully-built vftable (slots 4..8) is
    // unchanged; the real ??1/??_G is not matched here.
    void Stub_08d5e0();
    int winapi_0396f0_SelectClipRgn_SetBkMode();

    // --- CCreditsState members the Render path pins (placeholders) ---
    char m_pad1a8[0x1b4 - 0x1a8];
    int m_1b4; // +0x1b4 one-shot FX latch
    char m_pad1b8[0x1c4 - 0x1b8];
    int m_1c4; // +0x1c4 conditional-FX gate

    int LoadCreditzStateAssets(int a1, int a2, int a3);
    void InitAttractTitle();
};

class CBootyState : public CState {
public:
    virtual int Update() OVERRIDE; // return 0xa; (slot 4)

    // Engine-label backlog stub (non-virtual placeholder; vtable-neutral).
    void vfunc_1(); // stub

    void CheckWarpLetterBonus();
};

#endif // SRC_GRUNTZ_GAMEMODE_H
