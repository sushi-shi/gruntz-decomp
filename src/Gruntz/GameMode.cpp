// GameMode.cpp - the game-state ("mode") hierarchy the per-frame tick drives.
// See GameMode.h for the hierarchy + the headline finding. Names are placeholders;
// only offsets + code bytes are load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE) - 8/8 BYTE-EXACT (reloc-masked):
//   CState::CState()        @0x8c750  (169 B, thiscall)         99.86% - ctor (scalar zero/seed)
//   CState::~CState() (??_G)@0x8c710  (36  B, thiscall ret 4)   98.75% - slot-0 scalar-deleting dtor
//   CState::Update()        @0x8c4b0  (6   B, thiscall)        100.00% - slot 4  return 1;
//   CState::Render()        @0x8c4d0  (6   B, thiscall)        100.00% - slot 5  return 1;
//   CPlay::Update()         @0x8c910  (6   B, thiscall)        100.00% - return 3;
//   CMenuState::Update()    @0x8ce10  (6   B, thiscall)        100.00% - return 5;
//   CCreditsState::Update() @0x8d590  (6   B, thiscall)        100.00% - return 8;
//   CBootyState::Update()   @0x8d3f0  (6   B, thiscall)        100.00% - return 0xa;
// The <100% are reloc-masked operands only: the ctor's lone diff is the vtable
// DIR32 store; the dtor's three are vtable DIR32 + base-cleanup REL32 + op-delete
// REL32 (instruction sequences are byte-identical vs dump_target.py).
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

// CState::~CState()  @0x8c710 (36 B, ret 4): the slot-0 scalar-deleting dtor
// `??_G`. Restore the vftable, chain the WAP32 base cleanup (@0xfa150, thiscall),
// then (if the low bit of the hidden flags arg is set) `operator delete(this)`.
// The dtor body is defined INLINE in the header so MSVC folds it into the synth
// `??_G` thunk (the target inlines it; see GameMode.h).

// CState::Update()  @0x8c4b0 (6 B, slot 4 / +0x10): the base default = return 1.
int CState::Update()
{
    return 1;
}

// CState::Render()  @0x8c4d0 (6 B, slot 5 / +0x14): the base default = return 1.
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

// CPlay::Update()  @0x8c910 (6 B): the PLAY state's ID = 3.
int CPlay::Update()
{
    return 3;
}

// CMenuState::Update()  @0x8ce10 (6 B): the MENU state's ID = 5.
int CMenuState::Update()
{
    return 5;
}

// CCreditsState::Update()  @0x8d590 (6 B): the CREDITS state's ID = 8.
int CCreditsState::Update()
{
    return 8;
}

// CBootyState::Update()  @0x8d3f0 (6 B): the BOOTY state's ID = 0xa.
int CBootyState::Update()
{
    return 0xa;
}

// ===========================================================================
// CARCASS / MAP of the per-frame Render step (NOT byte-matched here; documented
// for the next matcher). The concrete Render overrides (slot +0x14) ARE the real
// per-frame game-state step+draw. CCreditsState::Render @0x391d0 (380 B) is the
// smallest and shows the canonical shape (this == ebp; large member offsets):
//
//   int CCreditsState::Render() {                 // @0x391d0
//     // 1. POLL/UPDATE INPUT: m_c->m_4->[+0x10]->m_2c->m_8; if set, vptr[+0x60]()
//     //    -> if it returns nonzero, fall through to the input-virtual block.
//     // 2. INPUT VIRTUAL: this->vtbl[+0x20]() (slot 8); if it returns nonzero ->
//     //    PostMessageA(hwnd, 0xfa0 /*WM_USER+...*/, 0x8006, 0); return 0;
//     // 3. CURSOR/ANIM: m_c->m_28->m_2c; if set, SimpleAnim(-1) @0x136e20.
//     // 4. ENTITY UPDATE LOOP A: walk g_entityList @0x645574 (count @+0x4,
//     //    elems @+0x8): for each, e->vtbl[+0x10]()  (Update each entity).
//     // 5. ENTITY SCAN LOOP B: walk the same list; for the first entity whose
//     //    (e->m_2ac & 0xffffff) != 0 -> PostMessageA(hwnd, 0x111 /*WM_COMMAND*/,
//     //    wParam=0x8023 (if m_24==5) else 0x8027); m_4->m_8->m_244 = 0.
//     // 6. TWO this-methods: this->@0x1352(), this->@0x141a() (sub-steps).
//     // 7. DRAW: m_c->m_4->[+0x10]->m_2c (@0x13e850, push 0), then
//     //    m_4->m_14->(m_4->m_18)  (@0x1564) - the blit/present.
//     // 8. ONE-SHOT FX: if (!m_1b4 && m_4->m_14) { PlaySomething("..."@0x60ce90,1)
//     //    @0x138840 on m_4->m_48; m_1b4 = 1; }   (latch at +0x1b4)
//     // 9. CONDITIONAL FX: if (m_1c4) { r = Find("..."@0x60ce74) @0x138730 on
//     //    m_4->m_48; if (r) { s = @0x138f60(r); if (!s) this->@0x3d41(); } }
//     //    return 1;
//   }
//
// CMenuState::Render @0xa0750 (464 B), CBootyState::Render @0x1c210 (1205 B) and
// the in-game CPlay::Render @0xc8cf0 (3092 B) follow the same spine (input poll ->
// per-entity Update loop -> network/message post -> draw -> latched FX), scaled up
// with the level/grunt simulation. CPlay::Render is the high-value next target.
//
// CState member offsets the Render path pins (beyond the ctor's): +0x4 (a
// CGruntzMgr/owner back-ptr -> +0x4->+0x4 = HWND), +0xc (an input/animation
// sub-object holder), +0x24 (a state-discriminator: ==5 selects WM 0x8023 vs
// 0x8027), +0x1b4 (a one-shot FX latch), +0x1c4 (a conditional-FX gate), and the
// per-frame counters at +0x150..+0x1a4 the ctor seeds. Global g_entityList
// @0x645574 (count@+4, elem array@+8) is the per-frame entity set every state
// Render walks.
