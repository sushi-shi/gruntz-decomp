// State.cpp - CState, the WAP32 base game-state ("mode") class (C:\Proj\Gruntz).
//
// CState's constructor (0x0008c750) + its slot-0 scalar-deleting dtor `??_G` (0x0008c710)
// form one retail .obj: a 2-function block that the linker pooled into the CState/CPlay
// vtable-emission region (bracketed by the per-class leaf slots in menustate/multi/
// gruntzmgr). Carved out of GameMode.cpp (REHOME package D7) so the CState BASE
// implementation - the ctor that stamps ??_7CState@@6B@ @0x005ea21c plus the slot-1/2
// vtable-order anchors - is its own TU, matching <Gruntz/State.h>. GameMode.cpp keeps
// only the free menu/HUD helpers + CState's NON-virtual out-of-line effect-loader
// methods (LoadGruntEffectSprites / LevelMsgHudDriver), which do not emit the vtable.
#include <Gruntz/State.h>
#include <rva.h>

// The scalar-deleting dtor's `operator delete` (reached by the synthesized `??_G`);
// declare it so /GX tracks the EH state.
void operator delete(void*);

// CState::CState(): store the vftable, then zero a flat list of scalar members in
// source-declaration order, seeding four time/budget fields to 0x40. NO embedded
// sub-object ctors and NO EH frame (plain /O2). This ctor (with the leaf dtors in the
// per-class TUs) anchors the CState vtable + inline-virtual emission.
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

// CState::~CState() - the slot-0 scalar-deleting dtor `??_G` (0x8c710). Its body is
// defined INLINE in <Gruntz/State.h> so MSVC folds it into the synth `??_G` thunk; the
// thunk has no source body, so pin its symbol by mangled name here.
// @rva-symbol: ??_GCState@@UAEPAXI@Z 0x0008c710 0x24

// CState::Update (0x0008c4b0) / Render (0x0008c4d0) are inline members in the header.

// The intervening vtable slots (1,2) - out-of-line stubs that anchor the vftable order
// so Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
i32 CState::Vfunc1(i32, i32, i32) {
    return 0;
}
void CState::ReleaseResources() {}
