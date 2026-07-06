// GameMode.cpp - the game-state ("mode") hierarchy the per-frame tick drives.
// See GameMode.h for the hierarchy + the headline finding. Names are placeholders;
// only offsets + code bytes are load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CState::CState()          99.86% - ctor (scalar zero/seed)
//   CState::~CState() (??_G)  98.75% - slot-0 scalar-deleting dtor
//   CState::Update()         100.00% - slot 4  return 1;
//   CState::Render()         100.00% - slot 5  return 1;
//   CMenuState::Update()     100.00% - return 5;
//   CCreditsState::Update()  100.00% - return 8;
//   CBootyState::Update()    100.00% - return 0xa;
//   CMenuState::Render()     BYTE-EXACT 99.43% (all
//       residuals reloc-masked) - the front-end per-frame menu draw.
//   CCreditsState::Render()  97.17% plateau - the
//       per-frame credits draw (residuals: reloc-masked operands + one register
//       coin-flip in the cursor-anim chain + the wParam select's mov-vs-push
//       codegen form; see the body comments).
// The <100% leaves are reloc-masked operands only: the ctor's lone diff is the
// vtable DIR32 store; the dtor's three are vtable DIR32 + base-cleanup REL32 +
// op-delete REL32 (instruction sequences are byte-identical vs dump_target.py).
//
// TWO LEVERS: (1) the base cleanup is __thiscall - modeled as a method
// on a helper struct so the dtor tail-call emits NO `add esp,4`. (2) define the
// dtor body INLINE in the header so MSVC folds the vtable-restore + base cleanup
// directly into the synthesized `??_G` (the target inlines, not `call ??1`).
//
// THE STATE-ID FINDING: the per-frame tick (RezMgr::PerFrameTick) calls m_mode's
// slot +0x10 (Update) and gates timing on `!= 0x11`; each concrete state's Update
// returns a fixed small enum tag (1/3/5/8/0xa = base/play/menu/credits/booty),
// i.e. slot +0x10 is the "which state am I" query, NOT the simulation step. The
// real per-frame step+draw is slot +0x14 (Render), overridden by each concrete
// state (carcassed in the long comment at the bottom of this file).
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDSurface.h>
#include <Bute/SymTab.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <Rez/RezMgr.h>        // RezFree - the engine allocator the video-handle teardown uses
#include <Bute/ButeMgr.h>      // CButeMgr g_buteMgr (GetIntDef for the SecretColor wormhole tint)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created glitter/letter sprites)
#include <Win32.h>                // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                // real IDirectDrawSurface (credits-scroll DC: GetDC/ReleaseDC)
#include <math.h>
#include <rva.h>
#include <stdio.h> // sprintf - the ATTRACT/title stack-buffer string builders
// Real MFC CRgn/CGdiObject for the credits clip region (CreditsScrollSelf::m_1e8).
// GameMode.h pulled <Mfc.h>->afx.h (defines _AFX_ENABLE_INLINES); skip afxwin*.inl for
// the clang label step only (implicit-int CMenu::op==); wine cl keeps the inlines.
// See docs/patterns/afxwin-clang-label-step-skip-inl.md. (Game CView is now CSpriteFactoryHolder,
// so afxwin's real MFC CView no longer collides.)
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

// ===========================================================================
// CState - the base game-state class.
// ===========================================================================

// CState::CState(): store the vftable, then zero a flat list of
// scalar members in source-declaration order, seeding four time/budget fields to
// 0x40. NO embedded sub-object ctors and NO EH frame (plain /O2 - the ctor uses
// eax=this, edx=0x40, ecx=0 held registers).
RVA(0x0008c750, 0xa9)
CState::CState() {
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_levelBank = 0;
    m_2c = 0;
    m_14 = 0;
    m_18 = 0;
    m_38 = 0;
    m_ready = 0;
    m_4c = 0;
    m_24 = 0;
    m_160 = 0;
    m_164 = 0;
    m_168 = 0;
    m_170 = 0x40;
    m_16c = 0;
    m_174 = 0x40;
    m_178 = 0;
    m_180 = 0x40;
    m_17c = 0;
    m_184 = 0x40;
    m_188 = 0;
    m_190 = 0;
    m_18c = 0;
    m_194 = 0;
    m_198 = 0;
    m_1a0 = 0;
    m_19c = 0;
    m_1a4 = 0;
    m_cursorX = 0;
    m_cursorY = 0;
}

// CState::~CState()  - the slot-0 scalar-deleting dtor `??_G`. Restore the
// vftable, chain the WAP32 base cleanup, then (if the low
// bit of the hidden flags arg is set) `operator delete(this)`. The dtor body is
// defined INLINE in the header so MSVC folds it into the synth `??_G` thunk (the
// target inlines it; see GameMode.h). This thunk has no source body, so it cannot
// carry an RVA() attribute - pin the deleting-dtor symbol by mangled name here.
// @rva-symbol: ??_GCState@@UAEPAXI@Z 0x0008c710 0x24

// CState::Update()  (slot 4 / +0x10): the base default = return 1.
RVA(0x0008c4b0, 0x6)
GameStateId CState::Update() {
    return GAMESTATE_BASE;
}

// CState::Render()  (slot 5 / +0x14): the base default = return 1.
RVA(0x0008c4d0, 0x6)
i32 CState::Render() {
    return 1;
}

// The intervening vtable slots (1,2) - out-of-line stubs that anchor the vftable
// order so Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
void CState::Vfunc1() {}
void CState::ReleaseResources() {}

// CState::Vfunc3() (slot 3 / +0xc): the active/ready gate - returns m_ready.
RVA(0x0008c490, 0x4)
i32 CState::Vfunc3() {
    return m_ready;
}

// CState::Vslot11() (slot 17 / +0x44): a 3-arg base default returning 0 (the
// derived states that care override it; CAttract inherits this body unchanged).
RVA(0x0008c610, 0x5)
i32 CState::Vslot11(i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CState-derived leaf teardown / per-frame poll methods (trace-discovered).
// The cleanup virtuals (slot 2) release the named resource sets the state
// registered, then chain BaseCleanup; the destructors are the EH-framed `??1`
// the `??_G` deleting dtors dispatch to. The `m_c` view sub-object (CState+0xc)
// is re-modeled here for the resource-release facet (registry @+0x10, leaf
// registry @+0x28, worker list @+0x0c, ddraw @+0x04) - field names placeholder.
// ===========================================================================

// The CState +0x0c resource-release facet (registry @+0x10, sound registry +
// pooled resource @+0x28, third registry @+0x2c, worker list @+0x0c, render/flip
// view @+0x04) is the RESOURCE facet of the one shared CSpriteFactoryHolder (<Gruntz/View.h>);
// the leaf-state teardown paths reach it through m_c directly (no cast).

// CState::~CState() chains the WAP32 base cleanup; the leaf `??1`s re-stamp the
// base vtable and call it (compiler-emitted). `operator delete` is reached by
// the synthesized `??_G`; declare it so /GX tracks the EH state.
void operator delete(void*);

// GenMenuRandPos (0x19cd0): a free __stdcall helper (no `this`; the trace
// mis-attributed it to CState - it is reached via a thunk from a CState method
// but takes only stack args). Generates a random {x,y} spawn position by edge,
// selected by `sel` (1..8); the edge cases share rand-modulo tails (the goto
// labels mirror the retail jump-table fall-throughs). Both output pointers must
// be non-null. Rand() = signed game RNG; RandRange(0,N) = uniform [0,N).

// The global game registry (canonical <Gruntz/WwdGameReg.h>): these CState helpers
// read m_10 (presence gate) / m_11c (ConfigureItem item) + the Rand()/RandRange()
// __thiscall helpers (all reloc-masked).
extern WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A (reloc-masked)

// @early-stop
// regalloc coin-flip wall (~89%): all 8 cases + shared tails + idiv constants are
// byte-identical; the sole residual is outX/outY swapped between esi/edi (retail
// outX->edi/outY->esi, recompile outX->esi/outY->edi). A named-local pin
// (docs/patterns/pin-local-for-callee-saved-reg.md) did NOT flip it -> the pure
// allocator coin-flip that doc flags as the zero-register-pinning.md wall.
RVA(0x00019cd0, 0x1df)
void __stdcall GenMenuRandPos(i32 sel, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    switch (sel) {
        case 1:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0x1e0;
            return;
        case 5:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0;
            return;
        case 3:
            *outX = 0;
            goto y_1e1;
        case 7:
            *outX = 0x280;
            goto y_1e1;
        y_1e1:
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case 2:
            if (g_gameReg->Rand() % 2) {
                *outX = 0;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = 0x1e0;
            return;
        case 8:
            if (g_gameReg->Rand() % 2) {
                *outX = 0x280;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141 + 0x140;
            *outY = 0x1e0;
            return;
        y_f1:
            *outY = g_gameReg->Rand() % 0xf1 + 0xf0;
            return;
        case 4:
            if (g_gameReg->Rand() % 2) {
                *outX = g_gameReg->RandRange(0, 0x140);
                *outY = 0;
                return;
            }
            *outX = 0;
            goto y_f0;
        case 6:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, 0x140) + 0x140;
                *outY = 0;
                return;
            }
            *outX = 0x280;
            goto y_f0;
        y_f0:
            *outY = g_gameReg->RandRange(0, 0xf0);
            return;
    }
}

// CBootyState::ReleaseResources() (slot 2 / +0x8): release the BOOTY resource
// set, then chain BaseCleanup. The `m_c` view's leaf registry (m_28) holds a
// pooled resource (Free if set) and releases two named sound sets; the name
// registry (m_10) releases two named sprite sets. Also reached directly from
// ~CBootyState (statically bound) - the booty teardown.
RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    // The view (m_c) is re-read for every access (retail does not cache it).
    CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
    if (r) {
        r->Free();
    }
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("BOOTY", "_");
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("GRUNTZ_WANDGRUNT", "_");
    m_c->m_10->Release("BOOTY", "_");
    m_c->m_10->Release("GRUNTZ_GOKARTGRUNT", "_");
    ((CGameModeBase*)this)->BaseCleanup();
}

// ===========================================================================
// The concrete states - each overrides Update() to return its state-ID tag.
// ===========================================================================

// (CPlay::Update lives with the rest of CPlay in src/Gruntz/Play.cpp.)

// CMenuState::Update(): the MENU state's ID = 5.
RVA(0x0008ce10, 0x6)
GameStateId CMenuState::Update() {
    return GAMESTATE_MENU;
}

// CCreditsState::Update(): the CREDITS state's ID = 8.
RVA(0x0008d590, 0x6)
GameStateId CCreditsState::Update() {
    return GAMESTATE_CREDITS;
}

// CBootyState::Update(): the BOOTY state's ID = 0xa.
RVA(0x0008d3f0, 0x6)
GameStateId CBootyState::Update() {
    return GAMESTATE_BOOTY;
}

// ===========================================================================
// The concrete Render overrides (vtable slot +0x14) - the real per-frame
// step+draw. Plain /O2 /MT: neither carries a stack C++ object / EH frame.
// ===========================================================================

// CCreditsState::Render(): the canonical Render spine.
//   1. INPUT POLL: i = m_c->m_drawTarget->m_10->m_2c->m_8; if (i && i->vtbl[+0x60](i))
//      skip the input-virtual; else
//   2. INPUT VIRTUAL: if (this->vtbl[+0x20]()) { m_4->Post(0x8006,0xfa0); return 0; }
//   3. CURSOR ANIM: if (((CSoundRegistry*)m_c->m_28)->m_2c) GM_SimpleAnim(-1);
//   4. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   5. MESSAGE SCAN: first e with (e->m_2ac & 0xffffff) -> PostMessageA(hwnd,
//      WM_COMMAND, m_24==5 ? 0x8023 : 0x8027, 0); m_4->m_8->m_244 = 0;
//   6. SUB-STEPS: Sub1(); Sub2();
//   7. DRAW: m_c->m_drawTarget->m_10->m_2c->Draw(0); m_c->m_4->m_14->Blit(m_c->m_4->m_18);
//   8. ONE-SHOT FX (+0x1b4 latch): if (!m_1b4 && m_4->m_14) {
//        m_4->m_48->Play("CREDITZ",1); m_1b4 = 1; }
//   9. CONDITIONAL FX (+0x1c4 gate): if (m_1c4) { s = m_4->m_48->Find("MONOLITH");
//        if (s && !s->Query()) Sub3(); }   return 1;
RVA(0x000391d0, 0x17c)
i32 CCreditsState::Render() {
    CGMInputObj* in = m_c->m_drawTarget->m_10->m_2c->m_8;
    if (!in || in->vtbl->Poll(in)) {
        if (!InputVirtual()) {
            ((CGMOwner*)m_4)->Post(0x8006, 0xfa0);
            return 0;
        }
    }

    if (((CSoundRegistry*)m_c->m_28)->m_2c) {
        GM_SimpleAnim(-1);
    }

    // per-entity Update pass
    {
        CGMEntityList* L = g_645574;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_elems[i]->Update();
        }
    }

    // message scan: first flagged entity posts a WM_COMMAND
    {
        CGMEntityList* L = g_645574;
        i32 n = L->m_count;
        for (i32 j = 0; j < n; j++) {
            if (L->m_elems[j]->m_2ac & 0xffffff) {
                // wParam = (m_24==5) ? 0x8023 : 0x8027. MSVC 5.0 /O2 branchless-
                // collapses an inline `?:` of these (sub/neg/sbb/and 4/add); the
                // init+conditional-override below keeps the cmp+jne branch (the
                // target's push-per-branch is the lazy `?:` form MSVC won't emit
                // when both arms fold to a 4-apart constant - irreducible).
                u32 wp = 0x8027;
                if (m_24 == 5) {
                    wp = 0x8023;
                }
                PostMessageA(((CGMOwner*)m_4)->m_4->m_4, 0x111, wp, 0);
                ((CGMOwner*)m_4)->m_8->m_244 = 0;
                break;
            }
        }
    }

    Sub1();
    Sub2();

    // draw: cache m_c->m_4 (the target keeps it in esi for the three derefs).
    CDrawTarget* v4 = m_c->m_drawTarget;
    v4->m_10->m_2c->Draw(0);
    v4->m_14->Blit((i32)v4->m_18);

    if (!m_1b4 && ((CGMOwner*)m_4)->m_14) {
        ((CGMOwner*)m_4)->m_48->Play(g_60ce90, 1);
        m_1b4 = 1;
    }

    if (m_1c4) {
        i32 s = ((CGMOwner*)m_4)->m_48->Find(g_60ce74);
        if (s && !((CGMSoundEntry*)s)->Query()) {
            Sub3();
        }
    }
    return 1;
}

// CMenuState::Render(): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_645584); m_1b4->Pre(); DrawVersion({g_645cc8..d4});
//      m_1b4->Post();   return 1;
RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    CGMEntityList* L = g_645574;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (i32 i = 0; i < L->m_count; i++) {
        L->m_elems[i]->Update();
    }

    // six prioritized entity-flag scans, each firing a distinct UI handler
    i32 c;
    L = g_645574;
    i32 n = L->m_count;
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x80000000) {
            m_1b4->OnFlag80000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x40000000) {
            m_1b4->OnFlag40000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x20000000) {
            m_1b4->OnFlag20000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x10000000) {
            m_1b4->OnFlag10000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x3) {
            m_1b4->OnFlag00000003();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100()) {
                PostMessageA(((CGMOwner*)m_4)->m_4->m_4, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:
    m_1b4->Step(g_645584);
    m_1b4->Pre();
    DrawVersion(g_645cc8);
    m_1b4->Post();
    return 1;
}

// (CCreditsState::InputVirtual is slot 8 / +0x20 == ShowAttractTitle @0x393b0, the
// per-frame input poll the Render dispatches via `this->vtbl[+0x20]()`; its body is
// defined below at its real RVA. Slot 6 is CCreditsState's own Vslot06 override,
// slot 7 is inherited from CState.)

// ===========================================================================
// REMAINING Render overrides (slot +0x14) NOT matched here (deferred targets):
//   CBootyState::Render - the bonus-state per-frame draw.
//   CPlay::Render       - the in-game per-frame heart (the
//       carcass is reconstructed in the `cplay` WIP unit, src/Gruntz/Play.cpp).
// Both follow the same spine the two matched Renders above show (per-entity
// Update loop over g_645574 -> entity-flag message scans -> draw/UI step),
// scaled up with the level/grunt simulation.
//
// CState member offsets the Render path pins (beyond the ctor's): +0x4 (the
// owner back-ptr -> +0x4->+0x4 = HWND), +0xc (the view/input sub-object holder),
// +0x24 (a state-discriminator: ==5 selects WM 0x8023 vs 0x8027), and the
// subclass FX state at +0x1b4 (CCreditsState one-shot-FX latch / CMenuState UI
// object) + +0x1c4 (CCreditsState conditional-FX gate). The global is a
// POINTER to the per-frame entity list (count@+4, elem-ptr array@+8) that every
// state Render iterates (e->vtbl[+0x10] = per-entity Update; e->m_2ac = flags).

// -------------------------------------------------------------------------
// Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001d440, 0xd7d)
void CBootyState::vfunc_1() {}

// DrawScrollingCredits (0x396f0): the credits scroll-text renderer. Each frame it ticks the
// three overlay timers down by the frame delta, advances the scrolling caption RECT by
// `delta * speed * 0.001` (wrapping when the RECT scrolls off, reseeding m_1f4 to
// 480/0.025), then - if the IDirectDrawSurface hands back an HDC - paints the caption
// (self->m_1f0) transparently into the scrolled RECT, and (when both the m_1c4/m_1c0 gates
// are live) the static "Now is the time at Monolith when we dance" credit into a fixed
// 640x480 RECT. GDI (SetBkMode/SelectClipRgn/SetTextColor/DrawTextA) via the IAT; the
// surface GetDC/ReleaseDC are the DDraw COM slots (+0x44/+0x68). CString temp -> /GX frame.
extern "C" i32 g_62bf74; // clip-region enable gate
extern double g_5e96f8;  // 480.0 (screen height) - extern-loaded so the reseed division
extern double g_5e96f0;  // 0.025 (scroll rate)   - is fld/fdiv, not a folded immediate
// The credits scroll's DirectDraw surface (prov->m_8): the game's real
// IDirectDrawSurface (<ddraw.h>). Only GetDC (slot 17, +0x44) and ReleaseDC
// (slot 26, +0x68) are used; both __stdcall with the surface as the hidden `this`,
// so `surf->GetDC(&hdc)` lowers to `push &hdc; push surf; mov reg,[surf];
// call [reg+slot]` - pointer-only, no vtable emitted in this TU.
SIZE_UNKNOWN(CreditsHdcProv);
struct CreditsHdcProv { // m_c->m_4->m_14->m_2c
    char p0[0x8];
    IDirectDrawSurface* m_8; // +0x08 the DDraw surface
};
SIZE_UNKNOWN(CreditsView4M14);
struct CreditsView4M14 {
    char p0[0x2c];
    CreditsHdcProv* m_2c; // +0x2c
};
SIZE_UNKNOWN(CreditsView4);
struct CreditsView4 {
    char p0[0x14];
    CreditsView4M14* m_14; // +0x14
};
SIZE_UNKNOWN(CreditsScrollView);
struct CreditsScrollView {
    char p0[0x4];
    CreditsView4* m_4; // +0x04
};
// The credits clip region at CreditsScrollSelf+0x1e8 is the real MFC CRgn/CGdiObject
// (vptr @+0x0, m_hObject @+0x4): the `(&m_1e8 != 0) ? m_hObject : 0` ternary at the
// SelectClipRgn site is CGdiObject::operator HRGN's null-this check verbatim. Folded
// to the real <afxwin.h> CRgn (the former local CreditsGdiObj view is gone); the
// game CView->CSpriteFactoryHolder rename cleared the afxwin CView collision that blocked it.
SIZE_UNKNOWN(CreditsScrollSelf);
struct CreditsScrollSelf {
    char m_pad00[0xc];
    CreditsScrollView* m_c; // +0x0c
    char m_pad10[0x1bc - 0x10];
    u32 m_1bc;    // +0x1bc overlay timer B (unsigned countdown)
    u32 m_1c0;    // +0x1c0 overlay timer C (unsigned countdown)
    i32 m_1c4;    // +0x1c4 second-caption gate
    RECT m_src;   // +0x1c8 source caption RECT
    RECT m_dst;   // +0x1d8 scrolled caption RECT (top +0x1dc / bottom +0x1e4 scroll)
    CRgn m_1e8;   // +0x1e8 clip region (real MFC CRgn: vptr + m_hObject)
    char* m_1f0;  // +0x1f0 caption CString buffer
    u32 m_1f4;    // +0x1f4 reseed timer A (unsigned countdown)
    double m_1f8; // +0x1f8 scroll accumulator
    double m_200; // +0x200 scroll speed
};

// @confidence: med
// @source: winapi:SelectClipRgn;SetBkMode
// @early-stop
// ~75%: complete + correct (timer decrements with matched jb-branch polarity, the
// scroll accumulator now fadd - the extern reseed constants block the /O2 constant-fold
// so the reseed is fld/fdiv/ftol like retail - the DDraw GetDC/ReleaseDC COM slots, the
// CGdiObject::operator-HRGN null-check clip, both DrawTextA paths + the static credit
// CString). Residual walls: (1) the /GX EH-frame representation (Unwind@ vs $L +
// __except_list, docs/seh-eh.md); (2) MSVC keeps the accumulator in st0 (fst) and lets
// ftol reuse it where retail stores-and-reloads (fstp/fld) - a float-consistency (/Op)
// mode difference, not source-steerable; (3) FP/prov-chain scheduling around the RECT
// copy. All logic + externs/strings named; the 3 FP-constant relocs stay differently-named.
RVA(0x000396f0, 0x2b8)
i32 CCreditsState::DrawScrollingCredits() {
    CreditsScrollSelf* self = (CreditsScrollSelf*)this;
    if (self->m_c == 0) {
        return 0;
    }
    CreditsHdcProv* prov = self->m_c->m_4->m_14->m_2c;

    if (g_645584 >= self->m_1f4) {
        self->m_1f4 = 0;
    } else {
        self->m_1f4 -= g_645584;
    }
    if (self->m_1c4 != 0) {
        if (g_645584 >= self->m_1bc) {
            self->m_1bc = 0;
        } else {
            self->m_1bc -= g_645584;
        }
        if (g_645584 >= self->m_1c0) {
            self->m_1c0 = 0;
        } else {
            self->m_1c0 -= g_645584;
        }
    }

    self->m_dst = self->m_src;
    double contrib = (double)g_645584 * self->m_200 * 0.001;
    self->m_1f8 = self->m_1f8 + contrib;
    i32 scrolled = (i32)self->m_1f8;
    self->m_dst.top -= scrolled;
    self->m_dst.bottom -= scrolled;
    if (self->m_dst.bottom < 0) {
        self->m_1f8 = 0.0;
        self->m_dst = self->m_src;
        self->m_1f4 = (i32)(g_5e96f8 / g_5e96f0);
    }

    HDC hdc = 0;
    prov->m_8->GetDC(&hdc);
    if (hdc != 0) {
        i32 oldBk = SetBkMode(hdc, TRANSPARENT);
        if (g_62bf74 != 0) {
            SelectClipRgn(
                hdc,
                self->m_1e8
            ); // CRgn -> HRGN via CGdiObject::operator HRGN (null-this)
        }
        i32 oldColor = SetTextColor(hdc, ((CCreditsState*)self)->FlashColor());
        DrawTextA(hdc, self->m_1f0, -1, &self->m_dst, 0x50);
        SetTextColor(hdc, oldColor);
        if (self->m_1c4 != 0 && self->m_1c0 != 0) {
            CString s("Now is the time at Monolith when we dance");
            RECT r = {0, 0, 0x280, 0x1e0};
            i32 oldColor2 = SetTextColor(hdc, 0xffffff);
            DrawTextA(hdc, s, -1, &r, 0x75); // CString -> LPCTSTR (implicit)
            SetTextColor(hdc, oldColor2);
        }
        if (g_62bf74 != 0) {
            SelectClipRgn(hdc, 0);
        }
        SetBkMode(hdc, oldBk);
        prov->m_8->ReleaseDC(hdc);
    }
    return 1;
}

// -------------------------------------------------------------------------
// Re-homed __thiscall behavioral methods (relocated from src/Stub/).
// -------------------------------------------------------------------------

// BuildWarpStoneGlitterAnimation (0x19540): a CMultiBootyState (booty) method - the
// trace mis-homed it on CState (the `this` is really a CMultiBootyState, whose
// glitter block sits at +0x1d8..+0x1fc). Build 4 "DoNothing" warp-letter animations
// through the mgr-settings animation factory (g_mgrSettings->m_world->m_8, the
// canonical CSpriteFactory), stash them in the +0x1ec ptr array (naming-independent
// offset access), set/clear their active bit, then build the trailing
// "SimpleAnimation" glitter sprite.
// The created "SimpleAnimation"/"DoNothing" sprite is the shared CGameObject
// (ApplyLookupSprite @0x1504d0 / ApplyName @0x150540 / ApplyLookupGeometry
// @0x1505b0). The WARP booty-letter draw (StepGlitterAnim/MoveLettersByDir) walks
// the very same objects as position/flag records - its former CBootyLetter facet
// (m_flags latch bit, m_screenX/Y, m_stateFlags active/out-of-bounds bit,
// m_latchedAnimId one-shot latch) is the same one class.
SIZE_UNKNOWN(CGlitterMgrM30);
struct CGlitterMgrM30 {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08 the animation factory (CreateSprite @0x1597b0)
};
SIZE_UNKNOWN(CGlitterMgrSet);
struct CGlitterMgrSet {
    char m_pad00[0x4];
    i32 m_4; // +0x04 element count
};
// The selection source (m_74): GetSel resolves an active selection handle.
// The color->handle table (m_78): the SecretColor-indexed handle array at +0x14.
SIZE_UNKNOWN(CGlitterColorTable);
struct CGlitterColorTable {
    char m_pad00[0x14];
    i32 m_arr14[1]; // +0x14  color->handle table
};
SIZE_UNKNOWN(CGlitterMgr);
struct CGlitterMgr {
    char m_pad00[0x30];
    CGlitterMgrM30* m_world; // +0x30
    char m_pad34[0x74 - 0x34];
    CSpriteRefTable* m_74;    // +0x74  selection source
    CGlitterColorTable* m_78; // +0x78  color->handle table
    CGlitterMgrSet* m_7c;     // +0x7c
    i32 m_80;                 // +0x80  attract frame counter (title rotation source)
};
DATA(0x0024556c)
extern "C" CGlitterMgr* g_mgrSettings;
// @early-stop
// 88.1%: logic byte-faithful. Residual is the branchless-select codegen for the per-letter
// `(i != m_letterIdx) ? 1 : 3` kind (retail's neg/sbb/and/add form vs cl's) + the per-iteration
// g_mgrSettings reload scheduling. Not source-steerable.
RVA(0x00019540, 0x12a)
i32 CMultiBootyState::BuildWarpStoneGlitterAnimation() {
    // The +0x1ec and +0x204 arrays overlap; reach the letter-sprite array by offset
    // (naming-independent, campaign doctrine) - the rest are real CMultiBootyState members.
    CGameObject** slot = (CGameObject**)((char*)this + 0x1ec);
    m_radius = 0xc8;
    m_letterIdx = (g_mgrSettings->m_7c->m_4 - 1) % 4;
    m_angleStep = 0;
    m_scratchX = 0;
    m_1e8 = 0;
    for (i32 i = 0; i < 4; i++) {
        CGameObject* a = g_mgrSettings->m_world->m_8
                             ->CreateSprite(0, 0, 0, (i != m_letterIdx) ? 1 : 3, "DoNothing", 3);
        slot[i] = a;
        if (a == 0) {
            return 0;
        }
        a->ApplyLookupSprite("GAME_STATUSBAR_TABZ_GAMETAB_WARP", i + 2);
        a->m_stateFlags |= 1;
    }
    for (i32 k = 0; k <= m_letterIdx; k++) {
        slot[k]->m_stateFlags &= ~1;
    }
    CGameObject* g = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 4, "SimpleAnimation", 3);
    m_cursorLetter = g;
    if (g == 0) {
        return 0;
    }
    g->ApplyName("GAME_GLITTERGOLD");
    m_cursorLetter->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

// LoadGruntEffectSprites (0x1a040): preload the in-game effect/icon animation set.
// Really a CPlay-layout method (the trace homed it on the CState base); it walks the
// same g_mgrSettings->m_world->m_8 SimpleAnimation factory as BuildWarpStoneGlitterAnimation
// but stores ~15 named effect sprites into the big +0x2fc.. block plus three parallel
// 8-element sprite arrays (bomb/go-kart/explosion) at +0x224/+0x244/+0x264, positioned
// from the geometry table. Every factory/lookup/GDI callee is a reloc-masked engine
// extern; all texture/cycle strings are named. Fields beyond CState are reached by a
// typed self-view (naming-independent offset access, campaign doctrine).
extern CButeMgr g_buteMgr;        // ?g_buteMgr@@3VCButeMgr@@A
extern char* g_wormholeSpawnKey;  // ?g_wormholeSpawnKey@@3PADA ("Wormhole" bute tag @0x60a7ac)
extern unsigned char g_dat60b588; // ?g_dat60b588@@3EA  (go-kart install byte flag)

// The go-kart install target reached via m_c->m_10 is the shared CSpriteFactoryHolder image
// registry (Install at vtable slot 18 / +0x48; <Gruntz/View.h>).
// The namespace/image registry at this+0x30 (resolves a named image handle).
// The geometry table (0x60b8fc, 0x10-byte rows): the effect sprites' x-position is
// (row.a + row.c) / 2. The loop init/bound relocs land on &row[0].c / &row[8].c.
SIZE_UNKNOWN(CEffGeomRow);
struct CEffGeomRow {
    i32 a;     // +0x00
    i32 pad4;  // +0x04 (417, unused)
    i32 c;     // +0x08
    i32 pad12; // +0x0c (245, unused)
};
DATA(0x0020b8fc)
extern CEffGeomRow g_effGeom[8]; // 0x60b8fc

// Typed self-view for this in-game effect loader. The +0x0c view is the shared
// CSpriteFactoryHolder; but its effect-sprite members below (m_224/m_2fc.. arrays) sit at offsets
// (0x304..0x318) that CPlay's own reconstruction models as drag-clamp/HUD-rect
// scalars - a genuine unresolved offset-reuse ambiguity, so the sprite block stays a
// documented self-view (offset access) rather than being folded into CPlay's layout.
SIZE_UNKNOWN(CEffLoaderSelf);
struct CEffLoaderSelf {
    char m_pad00[0xc];
    CSpriteFactoryHolder* m_c; // +0x0c  the shared view/render/resource context
    char m_pad10[0x30 - 0x10];
    CSymTab* m_30; // +0x30  image namespace
    char m_pad34[0x224 - 0x34];
    CGameObject* m_bomb[8];   // +0x224  bomb-grunt sprites
    CGameObject* m_gokart[8]; // +0x244  go-kart sprites
    CGameObject* m_expl[8];   // +0x264  explosion sprites
    char m_pad284[0x2fc - 0x284];
    CGameObject* m_2fc; // +0x2fc  stopwatch
    CGameObject* m_300; // +0x300  exit
    CGameObject* m_304; // +0x304  death twitch
    CGameObject* m_308; // +0x308  gauntletz
    CGameObject* m_30c; // +0x30c  beachballz
    CGameObject* m_310; // +0x310  roidz
    CGameObject* m_314; // +0x314  coin
    CGameObject* m_318; // +0x318  wormhole/teleporter
};

// @confidence: med
// @source: string-xref
// @early-stop
// ~96.3%: complete + correct, dev-authentic shape (natural array indexing throughout -
// self->m_bomb[i]/m_gokart[i]/m_expl[i] and g_effGeom[i].a/.c - which MSVC fuses into the
// retail single-pointer inductions). Residual is two scheduling walls: (1) the SecretColor
// block schedules the g_mgrSettings->m_78 load AFTER the GetIntDef call (retail hoists it
// into ebp before the call and keeps it across) - an eval-order choice not source-steerable
// without regressing the frame; (2) the (a+c)/2 geom pair loads a/c in the opposite eax/edx
// order (commutative). All externs/strings named.
RVA(0x0001a040, 0x55e)
i32 CState::LoadGruntEffectSprites() {
    CEffLoaderSelf* self = (CEffLoaderSelf*)this;

    i32 handleA = g_mgrSettings->m_74->GetSel(0, 0);
    if (handleA == 0) {
        return 0;
    }
    i32 handleB = g_mgrSettings->m_74->GetSel(0, 1);

    void* img = self->m_30->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == 0) {
        return 0;
    }
    self->m_c->m_10->Install(img, "GRUNTZ_GOKARTGRUNT", (const char*)&g_dat60b588);

    CSpriteFactory* f = g_mgrSettings->m_world->m_8;

    CGameObject* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_2fc = sw;
    if (sw == 0) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    self->m_2fc->ApplyLookupGeometry("GAME_CYCLE100", 0);
    self->m_2fc->m_stateFlags |= 1;

    CGameObject* wh = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_318 = wh;
    if (wh == 0) {
        return 0;
    }
    i32 tint =
        g_mgrSettings->m_78->m_arr14[g_buteMgr.GetIntDef(g_wormholeSpawnKey, "SecretColor", 1)];
    self->m_318->ApplyName("GAME_WORMHOLE");
    self->m_318->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    CGameObject* p318 = self->m_318;
    p318->m_drawActive = 1;
    p318->m_drawFillCmd = 7;
    p318->m_drawFillArg = tint;

    CGameObject* ex = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_300 = ex;
    if (ex == 0) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    self->m_300->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CGameObject* p300 = self->m_300;
    p300->m_drawActive = 1;
    p300->m_drawFillCmd = 0xa;
    p300->m_drawFillArg = handleA;
    self->m_300->m_stateFlags |= 1;

    CGameObject* dt = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_304 = dt;
    if (dt == 0) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    self->m_304->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CGameObject* p304 = self->m_304;
    p304->m_drawActive = 1;
    p304->m_drawFillCmd = 0xa;
    p304->m_drawFillArg = handleA;
    self->m_304->m_stateFlags |= 1;

    CGameObject* gl = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_308 = gl;
    if (gl == 0) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    self->m_308->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p308 = self->m_308;
    p308->m_drawActive = 1;
    p308->m_drawFillCmd = 0xa;
    p308->m_drawFillArg = handleA;
    self->m_308->m_stateFlags |= 1;

    CGameObject* bb = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_30c = bb;
    if (bb == 0) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    self->m_30c->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p30c = self->m_30c;
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = 0xa;
    p30c->m_drawFillArg = handleA;
    self->m_30c->m_stateFlags |= 1;

    CGameObject* rz = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_310 = rz;
    if (rz == 0) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    self->m_310->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p310 = self->m_310;
    p310->m_drawActive = 1;
    p310->m_drawFillCmd = 0xa;
    p310->m_drawFillArg = handleA;
    self->m_310->m_stateFlags |= 1;

    CGameObject* cn = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_314 = cn;
    if (cn == 0) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    self->m_314->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p314 = self->m_314;
    p314->m_drawActive = 1;
    p314->m_drawFillCmd = 0xa;
    p314->m_drawFillArg = handleA;
    self->m_314->m_stateFlags |= 1;

    // The three per-direction sprite arrays sit contiguously (bomb/go-kart/explosion),
    // positioned from the geometry table row's {a,c} midpoint; MSVC fuses the three
    // parallel array walks + the geom walk into single induction pointers.
    for (i32 i = 0; i < 8; i++) {
        CGameObject* b =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_bomb[i] = b;
        if (b == 0) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        self->m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CGameObject* bp = self->m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = 0xa;
        bp->m_drawFillArg = handleA;
        self->m_bomb[i]->m_screenX = 0x2c6;
        self->m_bomb[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        self->m_bomb[i]->m_stateFlags |= 1;

        CGameObject* e =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_expl[i] = e;
        if (e == 0) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        self->m_expl[i]->m_stateFlags |= 1;

        CGameObject* g =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_gokart[i] = g;
        if (g == 0) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        self->m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CGameObject* gp = self->m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = 0xa;
        gp->m_drawFillArg = handleB;
        self->m_gokart[i]->m_screenX = -70;
        self->m_gokart[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        self->m_gokart[i]->m_stateFlags |= 1;
    }
    return 1;
}

// BuildBootyWalkingGruntz (0x1b450) is re-homed to its real class BzState in
// src/Gruntz/BootyWalkAnim.cpp (beside its per-frame Update sibling).

// CBootyState::Render (slot 5 / +0x14, 0x1c210): the per-frame bonus-state draw
// (1205B). Still a reconstruction target - stub body marks the slot.
// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c210, 0x4b5)
i32 CBootyState::Render() {
    return 0;
}

// CMenuState::BuildVersionString (0xa0d80): format the on-screen version banner
// into a transient CString, append " (SPAWN MODE)" when the CD prompt latched the
// spawn install, then hand it to the shared HUD-message sprite helper into the
// caller-supplied RECT (the 4 args form the RECT by value). The build/patch field
// g_65160c selects the two- vs three-number version format.
extern "C" i32 g_651608;         // 0x651608  version field A
extern "C" i32 g_65160c;         // 0x65160c  build/patch field (0 -> two-number format)
extern "C" i32 g_651610;         // 0x651610  version field B
extern "C" i32 g_cdPromptResult; // 0x6455ec  spawn-mode latch
// The shared HUD message-sprite helper (0x1154b0 via the 0x1f00 ILT thunk,
// __cdecl): push a transient text sprite carrying `text` into `rect`.
void ShowHudMessage(
    void* sink,
    CString* text,
    RECT* rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0
RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(i32 rectLeft, i32, i32, i32) {
    CString str;
    if (g_65160c == 0) {
        str.Format("Gruntz v%d.%d", g_651608, g_651610);
    } else {
        str.Format("Gruntz v%d.%d%d", g_651608, g_65160c, g_651610);
    }
    if (g_cdPromptResult) {
        str += " (SPAWN MODE)";
    }
    ShowHudMessage(m_c, &str, (RECT*)&rectLeft, 0x64, 1, 0xff, 0xff, 0, 0);
}

// The "STATEZ_CREDITZ" registered object (m_2c): same Register source as
// CHelpState (FUN_0053c030). FindSet/FindSubset/Resolve/IsLoaded below are the
// reloc-masked __thiscall helpers off it / its sub-entries.
struct CCreditzSubEntry { // a music sub-entry ("PLAY"/"MONOLITH")
    i32 IsLoaded();       // FUN_00539960 __thiscall, ret BOOL/value
    char m_pad00[0xc];
    void* m_c; // +0x0c
};
struct CCreditzMusicSet { // the looked-up "MIDIZ" set (m_2c->FindSet)
    // FUN_0053a000 __thiscall: resolve a named sub-entry under a packed tag.
    CCreditzSubEntry* Resolve(char* szName, i32 tag);
};
struct CCreditzRegObj {               // the registered STATEZ_CREDITZ object (m_2c)
    void* FindSoundSet(char* szName); // FUN_0053a230 __thiscall, ret set ptr
    void* FindMusicSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
struct CCreditzSoundRegistry { // this->m_c->+0x28 (the LoadLevelSounds registry)
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
struct CCreditzImageRegistry { // this->m_4->+0x48
    // FUN_00538670 __thiscall: install a resolved sub-entry under a name.
    void Install3(void* res, void* host, char* szName);
};
struct CCreditzStateCore {      // this->m_c->m_4 (the ready/init pump)
    i32 IsReady();              // FUN_00558d20 __thiscall, ret BOOL
    i32 Init(i32 a, i32 flags); // FUN_00558cb0 __thiscall, ret BOOL
    i32 IsLoaded();             // FUN_00558bc0 __thiscall, ret BOOL (ready-3 predicate)
};
struct CCreditzImageRoot { // this->m_4 points here; +0x48 is the registry
    char m_pad00[0x48];
    CCreditzImageRegistry* m_48; // +0x48
};
struct CCreditzSoundMgr { // this->m_c points here
    char m_pad00[0x4];
    CCreditzStateCore* m_4; // +0x04
    char m_pad08[0x28 - 0x8];
    CCreditzSoundRegistry* m_28; // +0x28
};
struct CCreditzRegSet {                   // this->m_8 points here
    CCreditzRegObj* Register(char* name); // FUN_0053c030 __thiscall (CHelpState idiom)
};
// Two owner methods reached at the tail, both __thiscall(this) no args:
// the title/cursor setup (RVA 0x39a60) and the state-finish (0x439c40).

// Typed view of `this`: m_4 the image-registry root, m_8 the namespace registry,
// m_c the sound/state manager, m_2c the registered STATEZ_CREDITZ object.
struct CCreditzOwner {
    char m_pad00[0x4];
    CCreditzImageRoot* m_4; // +0x04
    CCreditzRegSet* m_8;    // +0x08
    CCreditzSoundMgr* m_c;  // +0x0c
    char m_pad10[0x2c - 0x10];
    CCreditzRegObj* m_2c; // +0x2c
    char m_pad30[0x1b4 - 0x30];
    i32 m_1b4; // +0x1b4
    i32 m_1b8; // +0x1b8
    i32 m_1bc; // +0x1bc
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4
    char m_pad1c8[0x20c - 0x1c8];
    i32 m_20c;                                  // +0x20c
    void SetupTitle();                          // RVA 0x39a60 __thiscall
    i32 FinishState();                          // RVA 0x439c40 __thiscall
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked near call
};

// @confidence: high
// @source: decomp-xref
// Byte-exact (100%). int (BOOL) return like its loader siblings: the early
// guards `return 0` (each reusing the just-loaded/zeroed eax via `test eax,eax`),
// and the success tail returns FinishState()'s result unmodified - a `void`
// return would tail-merge the bare epilogues. The literal `return 0;` (not
// `return loaded;`) is load-bearing: it keeps the opening/Init guards as
// `test eax,eax` and lets cl defer `xor ebp,ebp` to where retail materializes it.
// The MONOLITH block is a SIBLING (not nested) `if(midiz)` so the second
// `cmp edi,ebp; je` survives (docs/patterns/redundant-sibling-guard-retest.md).
// The 'IMX' music tag (0x584d49) is a non-relocated immediate. The
// "STATEZ_CREDITZ" Register is the CHelpState::LoadAssets source (FUN_0053c030).
RVA(0x00038d20, 0x176)
i32 CCreditsState::LoadCreditzStateAssets(i32 a1, i32 a2, i32 a3) {
    CCreditzOwner* self = (CCreditzOwner*)this;

    if (!self->LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    self->m_1b8 = 0;
    self->m_1bc = 0;
    self->m_1c0 = 0;
    self->m_1c4 = 0;
    self->m_2c = self->m_8->Register("STATEZ_CREDITZ");
    if (!self->m_2c) {
        return 0;
    }

    void* sounds = self->m_2c->FindSoundSet("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    self->m_c->m_28->Install(sounds, "CREDITZ", "_");

    CCreditzMusicSet* midiz = (CCreditzMusicSet*)self->m_2c->FindMusicSet("MIDIZ");
    if (midiz) {
        CCreditzSubEntry* e = midiz->Resolve("PLAY", 0x584d49);
        if (e) {
            i32 val = e->IsLoaded();
            if (val) {
                self->m_4->m_48->Install3((void*)val, e->m_c, "CREDITZ");
            }
        }
    }
    // Sibling re-test (not nested): retail re-emits `cmp edi,ebp; je` for the
    // MONOLITH block, pinning midiz in edi across the PLAY calls
    // (docs/patterns/redundant-sibling-guard-retest.md).
    if (midiz) {
        CCreditzSubEntry* e2 = midiz->Resolve("MONOLITH", 0x584d49);
        if (e2) {
            i32 val = e2->IsLoaded();
            if (val) {
                self->m_4->m_48->Install3((void*)val, e2->m_c, "MONOLITH");
            }
        }
    }

    if (!self->m_c->m_4->IsReady()) {
        if (!self->m_c->m_4->Init(0, 0x30000)) {
            return 0;
        }
    }

    self->SetupTitle();
    self->m_20c = 2;
    i32 r = self->FinishState();
    self->m_1b4 = 0;
    return r;
}

// CCreditsState::InputVirtual (slot 8 / +0x20, @0x393b0, formerly ShowAttractTitle) -
// the per-frame input poll: gate on the state core (m_c->m_4->IsLoaded); if loaded,
// force the cursor hidden then prime the attract title.
// Returns 1 (0 when the gate is not yet loaded, reusing the already-zero eax).
RVA(0x000393b0, 0x3a)
i32 CCreditsState::InputVirtual() {
    CCreditzOwner* self = (CCreditzOwner*)this;
    if (self->m_c->m_4->IsLoaded() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    InitAttractTitle();
    return 1;
}

// InitAttractTitle (0x39570): the credits/attract title (re)init - the twin of
// CAttract::LoadTitleConfig (CAttract.cpp). If the title view is already live (m_videoPlaying),
// just run the menu-page frame sub-steps (0x158dc0 / TransTitle / 0x158d50 + the
// m_18 fill) and bail; otherwise pick a rotating TITLE index off the mgr frame
// counter, format the "STATEZ_ATTRACT"/"TITLE%d" keys, resolve the attract state
// (m_8->LookupState) into m_2c, fade the title in (FadeInTitle), apply the configured
// "Menu"/"BrightnessPercent" level, transition the page, and build the menu page.
// The state/menu/self sub-calls + the g_buteMgr GetIntDef are reloc-masked externs.
SIZE_UNKNOWN(CMenuBrightHolder);
struct CMenuBrightHolder {
    char m_pad00[0x2c];
    CDDSurface* m_2c; // +0x2c
};
SIZE_UNKNOWN(CMenuPageA);
struct CMenuPageA {
    char m_pad00[0x14];
    CMenuBrightHolder* m_14; // +0x14 title brightness holder
    CMenuBrightHolder* m_18; // +0x18 menu brightness holder
};
SIZE_UNKNOWN(CMenuRootA);
struct CMenuRootA {
    char m_pad00[0x4];
    CMenuPageA* m_04; // +0x04
};
// CCreditsState's own FadeInTitle/BuildMenuPage are declared on the class (GameMode.h);
// they are reached through its own ILT thunks (0x1e60/0x1843).
// The CButeMgr text-config singleton (same 0x6453d8 datum as g_buteMgr) + the
// attract-state count divisor. TU-local views; both reloc-mask.
SIZE_UNKNOWN(CButeCfg);
struct CButeCfg {};
DATA(0x002453d8)
extern CButeCfg g_buteCfg;
extern "C" i32 g_645534;
// @early-stop
// 81.2%: logic byte-faithful (the twin of CAttract::LoadTitleConfig). Residual is the
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md)
// on the FadeInTitle fail return-0 + the sprintf stack-buffer slot layout. Not steerable.
RVA(0x00039570, 0x122)
i32 CCreditsState::InitAttractTitle() {
    CMenuRootA* root = (CMenuRootA*)m_c;
    if (m_videoPlaying != 0) {
        ((CDDrawSubMgrPages*)root->m_04)->Method_158dc0();
        ((CDDrawSubMgrPages*)root->m_04)->Method_158e90();
        ((CDDrawSubMgrPages*)root->m_04)->Method_158d50(0);
        root->m_04->m_18->m_2c->Fill(0);
        return 1;
    }
    char stateName[0x20];
    char titleName[0x20];
    i32 idx = g_mgrSettings->m_80 % g_645534 + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);
    void* saved = (void*)m_2c;
    void* state = ((CSymParser*)m_8)->ResolvePath(stateName);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }
    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (CResSource*)saved;
    if (faded == 0) {
        return 0;
    }
    CDDSurface* tgt = root->m_04->m_14->m_2c;
    tgt->ShadeRect(((CButeMgr*)&g_buteCfg)->GetIntDef("Menu", "BrightnessPercent", 0x32), 0);
    ((CDDrawSubMgrPages*)root->m_04)->Method_158e90();
    BuildMenuPage(0x50, 0x3e8, 0, 1);
    return 1;
}

// ===========================================================================
// CCreditsState teardown / per-frame video + flash steps (trace-discovered).
// ===========================================================================

// The video-handle (m_videoHandle) sub-object: its EH-framed destructor (0x38fc0) is a
// __thiscall on the handle, reached by the cleanup before RezFree. Reloc-masked.
struct CCreditsVideo {
    void Teardown(); // FUN_00438fc0 __thiscall, no-arg (the /GX dtor)
    void Close();    // FUN_0057c9b0 __thiscall, no-arg (SmackClose wrapper)
};

// The Smacker frame-step wrapper (FUN_0057c8e0): __stdcall(handle, frame); ret
// nonzero while more frames remain (PTR__SmackGoto@8). Reloc-masked.
extern "C" i32 __stdcall Eng_SmackStep(void* handle, i32 frame);

// The credits draw view (m_c->m_4): m_14 the source surface holder, m_18 the dest
// surface holder; each holds a DD surface at +0x2c. The dest surface carries the
// Smacker frame buffer at +0x8 (the step arg) and a clip RECT at +0x1c the blit is
// clipped to. BltFast (FUN_0053ef90) is a __thiscall on the source surface taking
// (0, 0, destSurf, &destRect, 0x10). Reloc-masked.
struct CCreditsSurface {
    void BltFast(i32 x, i32 y, CCreditsSurface* dst, void* rect, i32 flags);
    char m_pad00[0x8];
    void* m_8; // +0x08  Smacker frame buffer (SmackStep arg)
};
struct CCreditsDrawHolder {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  clip RECT (address taken)
    char m_pad20[0x2c - 0x20];
    CCreditsSurface* m_2c; // +0x2c  the DD surface
};
struct CCreditsDrawView {
    char m_pad00[0x14];
    CCreditsDrawHolder* m_14; // +0x14  source surface holder
    CCreditsDrawHolder* m_18; // +0x18  dest surface holder
};
struct CCreditsDrawRoot {
    char m_pad00[0x4];
    CCreditsDrawView* m_4; // +0x04
};

// @confidence: high
// @source: decomp-xref
// CCreditsState::ReleaseResources() (0x38f00): if (m_c) free the pooled resource
// then release the three named registries ("CREDITZ"); then tear down + RezFree
// the video handle (m_videoHandle) and chain BaseCleanup. m_c is re-read for each access
// (retail never caches it); the pooled-Free sits INSIDE the m_c guard.
RVA(0x00038f00, 0x87)
void CCreditsState::ReleaseResources() {
    if (m_c) {
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            r->Free();
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("CREDITZ", "_");
        m_c->m_10->Release("CREDITZ", "_");
        m_c->m_animRegistry->Release("CREDITZ", "_");
    }
    // Cache the video handle in a local so it stays pinned in edi across the
    // Teardown call (retail reuses the same register for the RezFree push).
    CCreditsVideo* vh = m_videoHandle;
    if (vh) {
        vh->Teardown();
        RezFree(vh);
        m_videoHandle = 0;
    }
    ((CGameModeBase*)this)->BaseCleanup();
}

// CCreditsState::~CCreditsState (`??1`, 0x8d5e0): stamp the CCreditsState vptr, run
// ReleaseResources (the credits teardown), then cl auto-destroys the m_1f0 CString
// (~CString @0x1b9cde) and the m_1e8 image list (stamp/DeleteImageList/base-restore)
// in reverse-declaration order before chaining the ~CState base. /GX EH frame for the
// member unwind. ReleaseResources / the callees all reloc-mask.
RVA(0x0008d5e0, 0x8b)
CCreditsState::~CCreditsState() {
    ReleaseResources();
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::FinishState() (0x39c40): clear the playing gate, return 1.
RVA(0x00039c40, 0x10)
i32 CCreditsState::FinishState() {
    m_videoPlaying = 0;
    return 1;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::StepVideo() (0x39c60): if the credits aren't playing (m_videoPlaying==0)
// return 1. Else advance the Smacker movie one frame; when the last frame is
// reached, Close() the handle and FinishState(). Either way, if both surfaces are
// live, blit the current frame to the dest surface. Returns the FinishState result
// (0 unless the movie just ended).
// @early-stop
// scheduling coin-flip wall (~95%): 49/51 instructions byte-identical; the sole
// residual is the BltFast `this` (src->m_2c) load scheduled one push earlier in
// retail (between the &rect and dst->m_2c pushes) + scratch-reg rotation. Complete
// + correct body; not source-steerable (zero-register-pinning.md family).
RVA(0x00039c60, 0x7a)
i32 CCreditsState::StepVideo() {
    if (!m_videoPlaying) {
        return 1;
    }
    i32 ret = 0;
    if (m_videoHandle) {
        CCreditsDrawView* v = ((CCreditsDrawRoot*)m_c)->m_4;
        CCreditsDrawHolder* dst = v->m_18;
        CCreditsDrawHolder* src = v->m_14;
        if (!Eng_SmackStep(dst->m_2c->m_8, -1)) {
            m_videoHandle->Close();
            ret = FinishState();
        }
        if (dst && src) {
            src->m_2c->BltFast(0, 0, dst->m_2c, &dst->m_1c, 0x10);
        }
    }
    return ret;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::FlashColor() (0x39d00): if the flash gate (m_1c4) is set and the
// re-roll timer (m_1bc) has expired, roll a fresh random RGB color (rand()%256 per
// channel, packed (b<<16)|(g<<8)|r), reset the timer to 0x12c, latch it at m_1b8
// and return it. Otherwise return the held color (0xffffff if the gate is clear).
// @early-stop
// byte-insert RGB pack wall (~70%): the value-correct (b<<16)|(g<<8)|r packs via
// `mov ch,al; mov cl,bl; shl 8; or esi` in retail (g/b land in byte regs) vs two
// `shl 8; or` here, compounded by a shrink-wrapped callee-save push (retail defers
// push esi/edi past the two early-return guards). See docs/patterns/
// rgb-pack-byte-insert.md + shrink-wrapped-callee-save-push.md; not steerable.
RVA(0x00039d00, 0x8c)
i32 CCreditsState::FlashColor() {
    i32 color = 0xffffff;
    if (m_1c4) {
        if (m_1bc) {
            return m_1b8;
        }
        i32 r = rand() % 256;
        i32 g = rand() % 256;
        i32 b = rand() % 256;
        m_1bc = 0x12c;
        color = (b << 16) | ((g & 0xff) << 8) | (r & 0xff);
        m_1b8 = color;
    }
    return color;
}

// ===========================================================================
// CMenuState / CBootyState teardown (the `??1` destructors + the slot-2
// resource-release virtuals). Compiled under /GX (gamemode unit = "eh"): the
// destructors carry the C++ EH frame the retail `??1` does. The destructor
// re-stamps its own vtable, calls the slot-2 release (statically bound), then
// re-stamps the CState vtable and chains BaseCleanup (compiler-emitted).
// ===========================================================================

// CMenuState::ReleaseResources() (slot 2 / +0x8): release the MENU resource set
// (name registry + leaf registry), dispose the worker list, free the menu UI
// object, then chain BaseCleanup. Also reached directly from ~CMenuState.
RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {
    // m_c re-read for each access (retail does not cache it); the null-guarded
    // block tests m_c once and reuses it for both the Free and DisposeWorkers.
    m_c->m_10->Release("MENU", "_");
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("MENU", "_");
    if (m_c) {
        // The test value of m_c is reused for the leaf-registry access; the
        // worker-list dispose re-reads m_c fresh (retail does not cache it).
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            r->Free();
        }
        m_c->m_rendererB->DisposeWorkers();
    }
    // m_1b4 IS cached (retail holds it in edi across the pre-delete + delete).
    CGMMenuUI* ui = m_1b4;
    if (ui) {
        ui->PreDelete();
        operator delete(ui);
        m_1b4 = 0;
    }
    ((CGameModeBase*)this)->BaseCleanup();
}

// CMenuState::~CMenuState() (`??1`, 0x8ce60): run the menu teardown then chain
// the base. ReleaseResources/the base ~CState are statically bound in the dtor.
RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

// CBootyState::~CBootyState() (`??1`, 0x8d440): run the booty teardown then
// chain the base.
RVA(0x0008d440, 0x55)
CBootyState::~CBootyState() {
    ReleaseResources();
}

// ===========================================================================
// CMultiBootyState - the MULTIPLAYER booty/bonus state (RTTI .?AVCMultiBootyState@@,
// vtable @0x5e9bdc). A SIBLING of CBootyState (the trace conflated them); shares the
// CState spine + the g_gameReg / draw-clock music idiom (CMenuState::StartMusic), and
// drives the "multi"/"BOOTY_LOOP"/"BOOTY_PERFECT" cue set. Its glitter/spawn animator
// (StepGlitterAnim) lays eight letter sprites on a sine spiral that shrinks per frame.
// ===========================================================================

// (The booty letter sprites are CGameObject - the same created "SimpleAnimation"
// objects the factory builds, walked here as position/flag records. See CGameObject.)

// The packed {x,y} spawn-coordinate table the animator indexes by m_letterIdx (DAT_005e8fe8;
// the disasm reads x via [tbl] and y via [tbl+4], stride 8). The +0x1ec / +0x204 sprite
// arrays are reached by offset off `this`.
extern "C" i32 g_5e8fe8[]; // {472,101, 525,98, 474,146, 525,144, ...}

// The trig constants: deg->rad (0.017453292), a phase bias (-225.0f), and the
// shrink curve (350.0 - step*0.002*350.0). Modeled as named extern doubles/floats so
// the fld/fmul carry DIR32 relocs (reloc-masked).
extern "C" float g_5e93b4;  // -225.0f  (phase bias, fsub'd)
extern "C" double g_5e93b8; // 0.017453292  (pi/180)
extern "C" double g_5e93c0; // 0.002
extern "C" double g_5e93c8; // 350.0

// The bonus state object (CMultiBootyState+0x2f8): flags @+0x8, a scroll phase @+0x5c.
struct CBootyBonusState {
    char m_pad00[0x8];
    i32 m_8; // +0x08 flags
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c scroll phase
};

// g_gameReg facet these booty pollers add: a draw object @+0x7c (a frame-ready query),
// the music host @+0x30 (->m_28->m_30 gate, ->m_28+0x10 the Lookup map), the configured
// item @+0x11c. Read through CBootyGameReg below (an extended WwdGameReg view).
// The draw object (g_gameReg+0x7c) the frame-ready gate runs on (__thiscall, ret 4).
struct CBootyDrawObj {
    i32 FrameReady(i32 z); // FUN_004fcd70
};
struct CBootyMusicHost; // the g_gameReg+0x30 music host (defined below)
struct CBootyGameReg {
    char m_pad00[0x30];
    CBootyMusicHost* m_30; // +0x30 music host
    char m_pad34[0x7c - 0x34];
    CBootyDrawObj* m_7c; // +0x7c draw object
    char m_pad80[0x11c - 0x80];
    i32 m_11c; // +0x11c configured music item
};
// g_gameReg is the WwdGameReg* declared above; the booty pollers read it through this
// extended view (codegen-neutral: same pointer, same loads).
#define BOOTY_REG ((CBootyGameReg*)g_gameReg)

// The Lookup output ("BOOTY_LOOP"/"BOOTY_PERFECT" cue entry): a player @+0x10
// (the ConfigureItem `this`), and a draw-clock gate (last @+0x14, interval @+0x18).
struct CBootyPlayer; // the found-cue player (ConfigureItem target; defined below)
struct CBootyFound {
    char m_pad00[0x10];
    CBootyPlayer* m_10; // +0x10 player (ConfigureItem this)
    i32 m_14;           // +0x14 last draw-clock
    i32 m_18;           // +0x18 interval
};

// The embedded CMapStringToOb the cue lookup runs on (M28+0x10, reached by offset).
struct CBootyLookupMap {
    i32 Lookup(char* key, void** out); // FUN_005b8438 CMapStringToOb::Lookup, ret 8
};

// The music host chain g_gameReg->m_world->m_28->{m_30 gate, Lookup map @+0x10}.
struct CBootyMusicHost {
    char m_pad00[0x28];
    struct M28 {
        char m_pad00[0x30];
        void* m_30; // +0x30 gate (non-null => skip); the lookup map is at this+0x10
    }* m_28;
};

// The cue/player config call (FUN_005360d0 ConfigureItem, __thiscall on found+0x10).
struct CBootyPlayer {
    void ConfigureItem(i32 item, i32 a, i32 b, i32 c); // FUN_005360d0, ret 0x10
};
// CMultiBootyState's own FadeInTitle/BuildPage are declared on the class (GameMode.h);
// they are reached through its own ILT thunks.

// ReleaseResources teardown chain: m_4 (owner) -> m_60 sub-object -> Teardown (the
// __thiscall no-arg FUN_0051c7b0, reloc-masked).
struct CBootyM4Sub {
    void Teardown(); // FUN_0051c7b0
};
struct CBootyOwnerView {
    char m_pad00[0x60];
    CBootyM4Sub* m_60; // +0x60
};

// The draw-clock mirror + the reentrancy gate the booty music gate reads (declared
// again near the menu-music helpers below; same DATA symbols, reloc-masked).
extern "C" u32 g_6bf3c0; // draw-clock mirror
extern i32 g_61ab20;     // DAT_0061ab20 reentrancy gate

// CMultiBootyState::StepGlitterAnim() (0x196c0): the glitter/spawn positioner. With
// m_1b4 set it snaps the eight letter sprites to the static spawn table; otherwise it
// walks a sine spiral (radius m_radius, angle (m_angleStep+225)*pi/180), advances the step by 5,
// shrinks the radius along the 350.0-step*0.002*350.0 curve, then latches the trailing
// sprite's spawn flag when the radius reaches zero.
// @early-stop
// regalloc wall (~80%): the float branch is byte-exact (sin/cos/__ftol chain matches);
// the residual is the two integer letter-loops + the final latch block, a pure
// register-allocation coin-flip - retail pins the `1` latch constant in eax and the
// loop index in ebx; the recompile picks ebx/edi (docs/patterns/zero-register-pinning.md).
RVA(0x000196c0, 0x1d3)
void CMultiBootyState::StepGlitterAnim() {
    if (m_1b4) {
        if (m_letterIdx >= 0) {
            i32* tbl = g_5e8fe8 + 1; // walks: tbl[-1]=x, tbl[0]=y; advances by 2
            CGameObject** ap = (CGameObject**)((char*)this + 0x1ec); // walks arr1ec by 1
            for (i32 i = 0; i <= m_letterIdx; i++) {
                CGameObject* e = *ap;
                e->m_screenX = tbl[-1];
                e = *ap;
                e->m_screenY = tbl[0];
                e = *ap;
                if (e->m_latchedAnimId != 1) {
                    e->m_latchedAnimId = 1;
                    e->m_flags |= 0x20000;
                }
                ap++;
                tbl += 2;
            }
        }
        m_cursorLetter->m_screenX = g_5e8fe8[m_letterIdx * 2];
        m_cursorLetter->m_screenY = g_5e8fe8[m_letterIdx * 2 + 1];
        return;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r = (float)m_radius; // load (float)m_radius first; shared across sin/cos terms
    double ang = ((float)step - g_5e93b4) * g_5e93b8;
    m_scratchX = (i32)(sin(ang) * r + (float)g_5e8fe8[idx * 2]);
    m_1e8 = (i32)(cos(ang) * r + (float)g_5e8fe8[idx * 2 + 1]);
    m_angleStep = step + 5;
    m_radius = (i32)(g_5e93c8 - (float)(step + 5) * g_5e93c0 * g_5e93c8);

    // Snap the leading sprites (0..m_letterIdx-1) to their static table coords (pointer walk).
    i32 i = 0;
    CGameObject** arr1ec = (CGameObject**)((char*)this + 0x1ec);
    if (idx > 0) {
        i32* tbl = g_5e8fe8 + 1;   // ecx: tbl[-1]=x, tbl[0]=y
        CGameObject** ap = arr1ec; // eax
        do {
            CGameObject* e = *ap;
            i++;
            ap++;
            e->m_screenX = tbl[-1];
            e = ap[-1];
            e->m_screenY = tbl[0];
            tbl += 2;
        } while (i < m_letterIdx);
    }
    // The trailing sprite + the i'th (== m_letterIdx) sprite get the computed scratch coords.
    m_cursorLetter->m_screenX = m_scratchX;
    m_cursorLetter->m_screenY = m_1e8;
    arr1ec[i]->m_screenX = m_scratchX;
    arr1ec[i]->m_screenY = m_1e8;

    MoveLettersByDir();

    if (m_radius == 0) {
        CGameObject* e = arr1ec[i];
        if (e->m_latchedAnimId != 1) {
            e->m_latchedAnimId = 1;
            e->m_flags |= 0x20000;
        }
    }
}

// CMultiBootyState::MoveLettersByDir() (0x19b90): if the anim-mode latch (m_1b4) is set,
// OR the spawn bit into all eight letters' flags; otherwise step each of the eight
// letters one cell (+/-4 px) along its compass direction (an 8-way jump table), flagging
// any that leave the [0,0x280]x[0,0x1e0] play field.
// @early-stop
// regalloc wall (~60%): logic/offsets/control-flow/jump-table all match; the residual is
// pure register allocation - retail loads x->ecx,y->edx and walks the array with `lea
// edx,[ecx+0x204]` (preserving this), the recompile swaps x/y and uses `add ecx`
// (docs/patterns/zero-register-pinning.md). The 8-way switch body itself is byte-aligned.
RVA(0x00019b90, 0xd7)
void CMultiBootyState::MoveLettersByDir() {
    if (m_1b4) {
        CGameObject** p = (CGameObject**)((char*)this + 0x204);
        i32 n = 8;
        do {
            CGameObject* e = *p;
            p++;
            e->m_stateFlags |= 1;
        } while (--n);
        return;
    }
    CGameObject** p = (CGameObject**)((char*)this + 0x204);
    for (i32 i = 0; i < 8; i++, p++) {
        CGameObject* e = *p;
        i32 x = e->m_screenX;
        i32 y = e->m_screenY;
        if (x < 0 || x > 0x280 || y < 0 || y > 0x1e0) {
            e->m_stateFlags |= 1;
        } else {
            switch (i) {
                case 0:
                    e->m_screenX = x;
                    (*p)->m_screenY = y - 4;
                    break;
                case 1:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y - 4;
                    break;
                case 2:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y;
                    break;
                case 3:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y + 4;
                    break;
                case 4:
                    e->m_screenX = x;
                    (*p)->m_screenY = y + 4;
                    break;
                case 5:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y + 4;
                    break;
                case 6:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y;
                    break;
                case 7:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y - 4;
                    break;
            }
        }
    }
}

// CMultiBootyState::CheckPerfectBonus() (0x1c0f0): once the frame-ready gate fires,
// drive the bonus state's scroll phase (m_bonusState->m_5c): on the wrap value (-0x82) play
// the "BOOTY_PERFECT" cue on the draw-clock window; past 0x302 latch the done flag
// (m_8 |= 0x10000); otherwise advance the phase by 0xa. Returns 1.
RVA(0x0001c0f0, 0xd5)
i32 CMultiBootyState::CheckPerfectBonus() {
    if (!BOOTY_REG->m_7c->FrameReady(-1)) {
        return 1;
    }
    CBootyBonusState* st = m_bonusState;
    i32 phase = st->m_5c;
    if (phase == (i32)0xffffff7e) {
        CBootyMusicHost* host = BOOTY_REG->m_30;
        i32 item = BOOTY_REG->m_11c;
        CBootyMusicHost::M28* m28 = host->m_28;
        if (m28->m_30 == 0) {
            void* found = 0;
            CBootyLookupMap* map = (CBootyLookupMap*)((char*)m28 + 0x10);
            map->Lookup("BOOTY_PERFECT", &found);
            if (found && g_61ab20 != 0) {
                CBootyFound* p = (CBootyFound*)found;
                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                    p->m_14 = g_6bf3c0;
                    p->m_10->ConfigureItem(item, 0, 0, 0);
                }
            }
        }
    }
    if (phase >= 0x302) {
        m_bonusState->m_8 |= 0x10000;
        return 1;
    }
    m_bonusState->m_5c = phase + 0xa;
    return 1;
}

// CMultiBootyState::ReadyAndPaint() (0x1ce30): gate on the active/ready virtual (CState
// slot 3 / +0xc); when ready, run the per-frame Paint and return the normalized result;
// otherwise return the (zero) gate result. (Was conflated under "CBootyState" by the trace.)
RVA(0x0001ce30, 0x1d)
i32 CMultiBootyState::ReadyAndPaint() {
    if (Vfunc3() == 0) {
        return 0;
    }
    return Paint() != 0;
}

// CMultiBootyState::ForwardIdleAnim(a, b) (0x1d420): trivial forwarder to the own
// BuildBootyGruntIdleAnimation (0x1ce60), passing this/args straight through.
RVA(0x0001d420, 0x8)
i32 CMultiBootyState::ForwardIdleAnim(i32 a, i32 b) {
    return BuildBootyGruntIdleAnimation();
}

// CMultiBootyState::ReleaseResources() (slot 2 / +0x8, 0x1e520): free the leaf-registry
// pooled resource (if set), release the "BOOTY" set on the leaf registry, run a teardown
// on the owner's m_4->m_60 sub-object, then chain BaseCleanup.
// @early-stop
// near-exact (~98.5%): structure/offsets/calls all match; the sole non-reloc residual is
// the m_4 deref landing in eax vs retail's edx (single-register coin-flip).
RVA(0x0001e520, 0x3e)
void CMultiBootyState::ReleaseResources() {
    CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
    if (r) {
        r->Free();
    }
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("BOOTY", "_");
    ((CBootyOwnerView*)m_4)->m_60->Teardown();
    ((CGameModeBase*)this)->BaseCleanup();
}

// CMultiBootyState::Update() (slot 4 / +0x10, 0x08d4c0): the multi-booty state's ID = 0x12.
RVA(0x0008d4c0, 0x6)
GameStateId CMultiBootyState::Update() {
    return GAMESTATE_MULTIBOOTY;
}

// CMultiBootyState::Vslot09() (slot 9 / +0x24, 0x1e570): on entry build the "multi"
// title page (fade + page) then, if the menu is live, push the "BOOTY_LOOP" cue into the
// player on the draw-clock window. Returns 1.
RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::Vslot09(i32) {
    i32 ok = FadeInTitle("multi", 0, 0, 0, 0, 1);
    if (!ok) {
        return ok; // eax already 0 (the FadeInTitle result) - no xor/mov re-materialize
    }
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);

    CBootyMusicHost* host = BOOTY_REG->m_30;
    i32 item = BOOTY_REG->m_11c;
    CBootyMusicHost::M28* m28 = host->m_28;
    if (m28->m_30 == 0) {
        void* found = 0;
        CBootyLookupMap* map = (CBootyLookupMap*)((char*)m28 + 0x10);
        map->Lookup("BOOTY_LOOP", &found);
        if (found && g_61ab20 != 0) {
            CBootyFound* p = (CBootyFound*)found;
            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                p->m_14 = g_6bf3c0;
                p->m_10->ConfigureItem(item, 0, 0, 1);
            }
        }
    }
    return 1;
}

// CMultiBootyState::QueryGruntSlots() (0x1ecf0): scan the four per-player registry
// records (g_gameReg+0x174, stride 0x238); the first whose +0x4 is set but +0x0 is
// clear yields its +0x150 field; none -> 0. (`this`/ecx is unused - ecx is the loop
// counter; modeled as a member that ignores `this`.)
// @early-stop
// 1-instruction-order wall (~94%): the `je`/test/loop shape is byte-exact (incl. the
// `xor ecx,ecx` before the +0x174 setup); the sole residual is the loop tail's `inc ecx`
// vs `add eax,0x238` order (counter-bump vs pointer-advance) - a /O2 scheduling coin-flip
// (a do-while reorder reshaped the back-edge worse, so the natural for-loop form is kept).
RVA(0x0001ecf0, 0x2a)
i32 CMultiBootyState::QueryGruntSlots() {
    char* base = (char*)g_gameReg;
    i32 i = 0;
    char* rec = base + 0x174;
    for (; i < 4; i++) {
        if (*(i32*)(rec + 4) != 0 && *(i32*)rec == 0) {
            return *(i32*)(rec - 0x24);
        }
        rec += 0x238;
    }
    return 0;
}

// CMultiBootyState::~CMultiBootyState() (`??1`, 0x8d510): run the booty teardown then
// chain the base (EH-framed; the vtable re-stamps fold into the compiler-emitted body).
RVA(0x0008d510, 0x55)
CMultiBootyState::~CMultiBootyState() {
    ReleaseResources();
}

// ===========================================================================
// CMenuState per-frame music/poll helpers (0xa05a0/0xa0640) and the slot-10
// per-frame poll (0xa06d0). They share the menu music controller at +0x1bc and
// the global game registry / draw-clock idiom (see StatusBarUpdaters.cpp).
// ===========================================================================

// The menu music controller (CMenuState+0x1bc): a player @+0x10 with a draw-clock
// gate (last @+0x14, interval @+0x18). The player has IsPlaying / Stop /
// ConfigureItem __thiscall slots (reloc-masked externs).
struct CMenuMusicPlayer {                           // m_1bc->m_10
    i32 IsPlaying();                                // FUN_001353f0, ret value
    void Stop(i32 a, i32 b, i32 c);                 // FUN_00135660, ret 0xc
    void ConfigureItem(i32 a, i32 b, i32 c, i32 d); // FUN_001360d0, ret 0x10
};
struct CMenuMusic {
    char m_pad00[0x10];
    CMenuMusicPlayer* m_10; // +0x10  player
    i32 m_14;               // +0x14  last draw-clock
    i32 m_18;               // +0x18  interval
};

// CMenuState::FormatHudText(buf, sel) (0x1af70): the 960-byte HUD-text formatter - an
// 8-case switch that sprintf()s the game clock (MM:SS via the imul-by-0x10624dd3
// divide-by-1000 then /60), score, and "%d of %d" progress into `buf`. Every stat is
// read via STAT(getter, field) = (m_liveGame && stats->m_c) ? getter() : stats->field over
// the game-stats object g_mgrSettings->m_7c (the sibling-guard idiom). The default
// case writes "???".
//
// Local view: CHudStats = the live-value/score object (g_mgrSettings->m_7c). The
// output arg is a real CString*: each case is CString::Format @0x1b2cf5 (masked),
// the default is operator=(LPCSTR) @0x1b9e74 writing "???".
SIZE_UNKNOWN(CHudStats);
struct CHudStats { // g_mgrSettings->m_7c - the live-value getters are thiscall on THIS
    // The 13 reloc-masked live-value getters (thiscall on the stats object):
    i32 GetC10();
    i32 GetC1c();
    i32 GetC20();
    i32 GetC34();
    i32 GetC18();
    i32 GetC30();
    i32 GetC14();
    i32 GetC38();
    i32 GetC24();
    i32 GetC40();
    i32 GetC2c();
    i32 GetC3c();
    i32 GetC28();
    char p0[0xc];
    i32 m_c; // +0xc  live-game flag (getter gate)
    i32 m_10, m_14, m_18, m_1c, m_20, m_24, m_28, m_2c, m_30, m_34, m_38, m_3c, m_40;
};
#define STATS ((CHudStats*)g_mgrSettings->m_7c)
#define STAT(getter, field) ((m_liveGame != 0 && STATS->m_c != 0) ? STATS->getter() : STATS->field)
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// 960-byte switch body is CODE-BYTE-EXACT (verified llvm-objdump -dr base vs retail:
// every stat sibling-guard block, the MM:SS unsigned /1000-then-/60 divide magic, the
// "%d of %d" clamp, the 13 stats-thiscall getters, and the sprintf pushes all match;
// the ~24 g_mgrSettings loads are the retail A1 moffs32 form). Residual ~2.5% is the
// inline .rdata jump table (8 case addresses) + the reloc-typed format-string DIR32
// operands, neither source-steerable. ~97.5%.
RVA(0x0001af70, 0x3c0)
void CMenuState::FormatHudText(CString* buf, i32 sel) {
    switch (sel) {
        case 0: {
            u32 secs = (u32)(STAT(GetC10, m_10) / 1000);
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case 1:
            buf->Format("%d", STAT(GetC1c, m_1c));
            return;
        case 2:
            buf->Format("%d", STAT(GetC20, m_20));
            return;
        case 3: {
            i32 total = STAT(GetC34, m_34);
            i32 cap = STAT(GetC34, m_34);
            i32 cur = STAT(GetC18, m_18);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 4: {
            i32 total = STAT(GetC30, m_30);
            i32 cap = STAT(GetC30, m_30);
            i32 cur = STAT(GetC14, m_14);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 5: {
            i32 total = STAT(GetC38, m_38);
            i32 cap = STAT(GetC38, m_38);
            i32 cur = STAT(GetC24, m_24);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 6: {
            i32 total = STAT(GetC40, m_40);
            i32 cap = STAT(GetC40, m_40);
            i32 cur = STAT(GetC2c, m_2c);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 7: {
            i32 total = STAT(GetC3c, m_3c);
            i32 cap = STAT(GetC3c, m_3c);
            i32 cur = STAT(GetC28, m_28);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        default:
            *buf = "???";
            return;
    }
}

// The draw-clock mirror + the reentrancy gate the menu music poll save/restores.
extern "C" u32 g_6bf3c0; // draw-clock mirror
extern i32 g_61ab20;     // DAT_0061ab20 reentrancy gate

// CMenuState::StartMusic() (0xa05a0): if the menu music + the registry gate are
// live, push the configured item into the player on the draw-clock window, under
// a save/restored reentrancy gate.
RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_1bc == 0) {
        return;
    }
    if (g_gameReg->m_10 == 0) {
        return;
    }
    i32 saved = g_61ab20;
    i32 flag = saved;
    if (!saved) {
        flag = 1;
        g_61ab20 = 1;
    }
    i32 item = g_gameReg->m_11c;
    CMenuMusic* mus = m_1bc;
    if (flag) {
        u32 clk = g_6bf3c0;
        if (clk - mus->m_14 >= (u32)mus->m_18) {
            mus->m_14 = clk;
            mus->m_10->ConfigureItem(item, 0, 0, 1);
        }
    }
    if (!saved) {
        g_61ab20 = saved;
    }
}

// CMenuState::StopMusicChain() (0xa0640): if the menu music is playing, request
// a fade-out stop, then spin the cursor/anim tick until playback ends.
RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_1bc == 0) {
        return;
    }
    CMenuMusic* mus = m_1bc;
    if (!mus->m_10->IsPlaying()) {
        return;
    }
    m_1bc->m_10->Stop(0, 0x1f4, 1);
    if (!m_1bc->m_10->IsPlaying()) {
        return;
    }
    do {
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            r->TickAnim(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

// CMenuState::FrameSlot28(int) (slot 10 / +0x28, 0xa06d0): flush + flip the menu
// view, stamp the start clock, run the music-stop chain, then busy-wait m_1b8 ms.
RVA(0x000a06d0, 0x5f)
i32 CMenuState::FrameSlot28(i32) {
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    m_c->m_drawTarget->m_10->m_2c->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

SIZE_UNKNOWN(CCreditzSubEntry);
SIZE_UNKNOWN(CCreditzMusicSet);
SIZE_UNKNOWN(CCreditzRegObj);
SIZE_UNKNOWN(CCreditzSoundRegistry);
SIZE_UNKNOWN(CCreditzImageRegistry);
SIZE_UNKNOWN(CCreditzStateCore);
SIZE_UNKNOWN(CCreditzImageRoot);
SIZE_UNKNOWN(CCreditzSoundMgr);
SIZE_UNKNOWN(CCreditzRegSet);
SIZE_UNKNOWN(CCreditzOwner);
SIZE_UNKNOWN(CCreditsVideo);
SIZE_UNKNOWN(CCreditsSurface);
SIZE_UNKNOWN(CCreditsDrawHolder);
SIZE_UNKNOWN(CCreditsDrawView);
SIZE_UNKNOWN(CCreditsDrawRoot);
SIZE_UNKNOWN(CBootyBonusState);
SIZE_UNKNOWN(CBootyDrawObj);
SIZE_UNKNOWN(CBootyGameReg);
SIZE_UNKNOWN(CBootyFound);
SIZE_UNKNOWN(CBootyLookupMap);
SIZE_UNKNOWN(CBootyMusicHost);
SIZE_UNKNOWN(CBootyPlayer);
SIZE_UNKNOWN(CBootyM4Sub);
SIZE_UNKNOWN(CBootyOwnerView);
SIZE_UNKNOWN(CMenuMusicPlayer);
SIZE_UNKNOWN(CMenuMusic);
