// GameMode.cpp - the game-state ("mode") hierarchy the per-frame tick drives.
// See GameMode.h for the hierarchy + the headline finding. Names are placeholders;
// only offsets + code bytes are load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CState::CState()         @0x8c750 (169 B, thiscall)        99.86% - ctor (scalar zero/seed)
//   CState::~CState() (??_G) @0x8c710 (36  B, thiscall ret 4)  98.75% - slot-0 scalar-deleting dtor
//   CState::Update()         @0x8c4b0 (6   B, thiscall)       100.00% - slot 4  return 1;
//   CState::Render()         @0x8c4d0 (6   B, thiscall)       100.00% - slot 5  return 1;
//   CPlay::Update()          @0x8c910 (6   B, thiscall)       100.00% - return 3;
//   CMenuState::Update()     @0x8ce10 (6   B, thiscall)       100.00% - return 5;
//   CCreditsState::Update()  @0x8d590 (6   B, thiscall)       100.00% - return 8;
//   CBootyState::Update()    @0x8d3f0 (6   B, thiscall)       100.00% - return 0xa;
//   CMenuState::Render()     @0xa0750 (464 B, thiscall) BYTE-EXACT 99.43% (all
//       residuals reloc-masked) - the front-end per-frame menu draw.
//   CCreditsState::Render()  @0x391d0 (380 B, thiscall) 97.17% plateau - the
//       per-frame credits draw (residuals: reloc-masked operands + one register
//       coin-flip in the cursor-anim chain + the wParam select's mov-vs-push
//       codegen form; see the body comments).
// The <100% leaves are reloc-masked operands only: the ctor's lone diff is the
// vtable DIR32 store; the dtor's three are vtable DIR32 + base-cleanup REL32 +
// op-delete REL32 (instruction sequences are byte-identical vs dump_target.py).
//
// TWO LEVERS: (1) the base cleanup (@0xfa150) is __thiscall - modeled as a method
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
#include "GameMode.h"

// ===========================================================================
// CState - the base game-state class.
// ===========================================================================

// CState::CState()  @0x8c750 (169 B): store the vftable, then zero a flat list of
// scalar members in source-declaration order, seeding four time/budget fields to
// 0x40. NO embedded sub-object ctors and NO EH frame (plain /O2 - the ctor uses
// eax=this, edx=0x40, ecx=0 held registers).
// @address: 0x08c750
// @size:    0xa9
CState::CState()
{
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_28 = 0;
    m_2c = 0;
    m_14 = 0;
    m_18 = 0;
    m_38 = 0;
    m_3c = 0;
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
    m_150 = 0;
    m_154 = 0;
}

// CState::~CState()  - the slot-0 scalar-deleting dtor `??_G`. Restore the
// vftable, chain the WAP32 base cleanup (@0xfa150, thiscall), then (if the low
// bit of the hidden flags arg is set) `operator delete(this)`. The dtor body is
// defined INLINE in the header so MSVC folds it into the synth `??_G` thunk (the
// target inlines it; see GameMode.h). clang's AST mangles the inline dtor as the
// plain `??1`, so pin the deleting-dtor symbol explicitly here.
//
// @address: 0x08c710
// @symbol:  ??_GCState@@UAEPAXI@Z
// @size:    0x24

// CState::Update()  @0x8c4b0 (6 B, slot 4 / +0x10): the base default = return 1.
// @address: 0x08c4b0
// @size:    0x6
int CState::Update()
{
    return 1;
}

// CState::Render()  @0x8c4d0 (6 B, slot 5 / +0x14): the base default = return 1.
// @address: 0x08c4d0
// @size:    0x6
int CState::Render()
{
    return 1;
}

// The intervening vtable slots (1..3) - out-of-line stubs that anchor the vftable
// order so Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
void CState::Vfunc1() {}
void CState::Vfunc2() {}
void CState::Vfunc3() {}

// ===========================================================================
// The concrete states - each overrides Update() to return its state-ID tag.
// ===========================================================================

// CMenuState::~CMenuState()  @0x8ce60 (85 B): EH-framed complete-object dtor.
// @address: 0x08ce60
// @size:    0x55
CMenuState::~CMenuState()
{
}

// CPlay::Update()  @0x8c910 (6 B): the PLAY state's ID = 3.
// @address: 0x08c910
// @size:    0x6
int CPlay::Update()
{
    return 3;
}

// CMenuState::Update()  @0x8ce10 (6 B): the MENU state's ID = 5.
// @address: 0x08ce10
// @size:    0x6
int CMenuState::Update()
{
    return 5;
}

// CCreditsState::Update()  @0x8d590 (6 B): the CREDITS state's ID = 8.
// @address: 0x08d590
// @size:    0x6
int CCreditsState::Update()
{
    return 8;
}

// CBootyState::Update()  @0x8d3f0 (6 B): the BOOTY state's ID = 0xa.
// @address: 0x08d3f0
// @size:    0x6
int CBootyState::Update()
{
    return 0xa;
}

// ===========================================================================
// The concrete Render overrides (vtable slot +0x14) - the real per-frame
// step+draw. Plain /O2 /MT: neither carries a stack C++ object / EH frame.
// ===========================================================================

// CCreditsState::Render()  @0x391d0 (380 B): the canonical Render spine.
//   1. INPUT POLL: i = m_c->m_4->m_10->m_2c->m_8; if (i && i->vtbl[+0x60](i))
//      skip the input-virtual; else
//   2. INPUT VIRTUAL: if (this->vtbl[+0x20]()) { m_4->Post(0x8006,0xfa0); return 0; }
//   3. CURSOR ANIM: if (m_c->m_28->m_2c) GM_SimpleAnim(-1);
//   4. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   5. MESSAGE SCAN: first e with (e->m_2ac & 0xffffff) -> PostMessageA(hwnd,
//      WM_COMMAND, m_24==5 ? 0x8023 : 0x8027, 0); m_4->m_8->m_244 = 0;
//   6. SUB-STEPS: Sub1(); Sub2();
//   7. DRAW: m_c->m_4->m_10->m_2c->Draw(0); m_c->m_4->m_14->Blit(m_c->m_4->m_18);
//   8. ONE-SHOT FX (+0x1b4 latch): if (!m_1b4 && m_4->m_14) {
//        m_4->m_48->Play("CREDITZ",1); m_1b4 = 1; }
//   9. CONDITIONAL FX (+0x1c4 gate): if (m_1c4) { s = m_4->m_48->Find("MONOLITH");
//        if (s && !s->Query()) Sub3(); }   return 1;
// @address: 0x0391d0
// @size:    0x17c
int CCreditsState::Render()
{
    CGMInputObj *in = ((CGMView *)m_c)->m_4->m_10->m_2c->m_8;
    if (!in || in->vtbl->Poll(in)) {
        if (!InputVirtual()) {
            ((CGMOwner *)m_4)->Post(0x8006, 0xfa0);
            return 0;
        }
    }

    if (((CGMView *)m_c)->m_28->m_2c)
        GM_SimpleAnim(-1);

    // per-entity Update pass
    {
        CGMEntityList *L = g_645574;
        for (int i = 0; i < L->m_count; i++)
            L->m_elems[i]->Update();
    }

    // message scan: first flagged entity posts a WM_COMMAND
    {
        CGMEntityList *L = g_645574;
        int n = L->m_count;
        for (int j = 0; j < n; j++) {
            if (L->m_elems[j]->m_2ac & 0xffffff) {
                // wParam = (m_24==5) ? 0x8023 : 0x8027. MSVC 5.0 /O2 branchless-
                // collapses an inline `?:` of these (sub/neg/sbb/and 4/add); the
                // init+conditional-override below keeps the cmp+jne branch (the
                // target's push-per-branch is the lazy `?:` form MSVC won't emit
                // when both arms fold to a 4-apart constant - irreducible).
                unsigned wp = 0x8027;
                if (m_24 == 5)
                    wp = 0x8023;
                PostMessageA(((CGMOwner *)m_4)->m_4->m_4, 0x111, wp, 0);
                ((CGMOwner *)m_4)->m_8->m_244 = 0;
                break;
            }
        }
    }

    Sub1();
    Sub2();

    // draw: cache m_c->m_4 (the target keeps it in esi for the three derefs).
    CGMView::M4 *v4 = ((CGMView *)m_c)->m_4;
    v4->m_10->m_2c->Draw(0);
    v4->m_14->Blit((int)v4->m_18);

    if (!m_1b4 && ((CGMOwner *)m_4)->m_14) {
        ((CGMOwner *)m_4)->m_48->Play(g_60ce90, 1);
        m_1b4 = 1;
    }

    if (m_1c4) {
        int s = ((CGMOwner *)m_4)->m_48->Find(g_60ce74);
        if (s && !((CGMSoundEntry *)s)->Query())
            Sub3();
    }
    return 1;
}

// CMenuState::Render()  @0xa0750 (464 B): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_645584); m_1b4->Pre(); DrawVersion({g_645cc8..d4});
//      m_1b4->Post();   return 1;
// @address: 0x0a0750
// @size:    0x1d0
int CMenuState::Render()
{
    CGMEntityList *L = g_645574;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (int i = 0; i < L->m_count; i++)
        L->m_elems[i]->Update();

    // six prioritized entity-flag scans, each firing a distinct UI handler
    int c;
    L = g_645574;
    int n = L->m_count;
    for (c = 0; c < n; c++)
        if ((unsigned)L->m_elems[c]->m_2ac & 0x80000000) { m_1b4->OnFlag80000000(); goto tail; }
    for (c = 0; c < n; c++)
        if ((unsigned)L->m_elems[c]->m_2ac & 0x40000000) { m_1b4->OnFlag40000000(); goto tail; }
    for (c = 0; c < n; c++)
        if ((unsigned)L->m_elems[c]->m_2ac & 0x20000000) { m_1b4->OnFlag20000000(); goto tail; }
    for (c = 0; c < n; c++)
        if ((unsigned)L->m_elems[c]->m_2ac & 0x10000000) { m_1b4->OnFlag10000000(); goto tail; }
    for (c = 0; c < n; c++)
        if (L->m_elems[c]->m_2ac & 0x3) { m_1b4->OnFlag00000003(); goto tail; }
    for (c = 0; c < n; c++)
        if (L->m_elems[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100())
                PostMessageA(((CGMOwner *)m_4)->m_4->m_4, 0x111, 0x8036, 0);
            goto tail;
        }
tail:
    m_1b4->Step(g_645584);
    m_1b4->Pre();
    DrawVersion(g_645cc8);
    m_1b4->Post();
    return 1;
}

// CCreditsState vtable anchors: slots 6,7 pad so the input virtual lands at slot
// 8 (+0x20); InputVirtual (slot 8) is the per-frame input poll the Render does an
// indirect `this->vtbl[+0x20]()` to (its body is irrelevant to the Render match -
// only the indirect call site is). These are out-of-line so the CCreditsState
// vtable @0x5e9c64 resolves; they are NOT byte-matched targets.
void CCreditsState::Cv6() {}
void CCreditsState::Cv7() {}
int  CCreditsState::InputVirtual() { return 0; }

// ===========================================================================
// REMAINING Render overrides (slot +0x14) NOT matched here (deferred targets):
//   CBootyState::Render @0x1c210 (1205 B) - the bonus-state per-frame draw.
//   CPlay::Render       @0xc8cf0 (3092 B) - the in-game per-frame heart (the
//       carcass is reconstructed in the `cplay` WIP unit, src/Gruntz/CPlay.cpp).
// Both follow the same spine the two matched Renders above show (per-entity
// Update loop over g_645574 -> entity-flag message scans -> draw/UI step),
// scaled up with the level/grunt simulation.
//
// CState member offsets the Render path pins (beyond the ctor's): +0x4 (the
// owner back-ptr -> +0x4->+0x4 = HWND), +0xc (the view/input sub-object holder),
// +0x24 (a state-discriminator: ==5 selects WM 0x8023 vs 0x8027), and the
// subclass FX state at +0x1b4 (CCreditsState one-shot-FX latch / CMenuState UI
// object) + +0x1c4 (CCreditsState conditional-FX gate). The global @0x645574 is a
// POINTER to the per-frame entity list (count@+4, elem-ptr array@+8) that every
// state Render iterates (e->vtbl[+0x10] = per-entity Update; e->m_2ac = flags).
