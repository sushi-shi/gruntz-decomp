// OrphanMethods.cpp - moderate orphan-COMDAT methods with no recoverable owning
// class. Each is modeled from its disassembly with PLACEHOLDER class/field names;
// only OFFSETS + code bytes are load-bearing. Engine callees are external/no-body.
#include <Ints.h>
#include <Gruntz/Effect6b.h>
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Setup_15c2d0 (0x15c2d0) - +0x1a0 geo setter
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (0x15c360) - +0x1a0 advance
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Rez/FrameClock.h> // g_timer200 (strike-effect threshold)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Globals.h>

// ---------------------------------------------------------------------------
// 0x6b2e0: an animation effect apply - cache the owner's m_1b4 into this->m_c, run
// the owner's embedded anim sub-object (+0x1a0) advance, and (when the flag arg is
// set) re-target its draw-delta.
// @early-stop
// 76%: every instruction (lea anim, m_1b4 read, m_c store, arg push, both calls) is
// byte-faithful; the residual is pure register coloring + a 2-instr scheduling flip
// in this 0x39-byte leaf - retail keeps m_1b4 in edx and hoists the `a` load into
// eax before the m_c store; cl colors m_1b4 in eax and stores m_c first. Not
// source-steerable (every operand/declaration reorder reproduced the same coloring).
extern "C" u32 g_engineFrameDelta;

// The anim sub-player at owner+0x1a0 IS a CDDrawBlitParam (geo setter Setup_15c2d0
// @0x15c2d0) / CAniAdvanceCursor (advance @0x15c360) - the two canonical engine
// classes for the +0x1a0 sub-object; reached cast-at-use like the rest of the tree.
struct CAnimOwner6b {
    char _00[0x1b4];
    i32 m_1b4; // +0x1b4
};

// @interleaver CEffect6b::Apply emitted-in <boundary: unreconstructed>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are ddrawsubmgrleaf
// @0x6b2a0 (before) and gamelevel PointInBounds @0x6b330 (after), NOT a single
// reconstructed host on both sides. True obj is the unreconstructed 0x6b2xx run.)
RVA(0x0006b2e0, 0x39)
void CEffect6b::Apply(i32 a, i32 b) {
    char* anim = (char*)m_4 + 0x1a0;
    m_c = m_4->m_1b4;
    ((CAniAdvanceCursor*)anim)->Setup_15c2d0((CAniElement*)a);
    if (b != 0) {
        ((CAniAdvanceCursor*)anim)->Advance((i32)g_engineFrameDelta);
    }
}

// ---------------------------------------------------------------------------
// (CGridLookup::Lookup 0x75a40 is merged into src/Gruntz/TriggerMgrHitTest.cpp -
// called only by that TU's megafn FUN_6f2f0; interval verdict.)

// ---------------------------------------------------------------------------
// 0x95140 (was the CState95 placeholder view) IS CHelpState::Vslot09 (slot 9) - HOMED
// to src/Gruntz/HelpState.cpp (2026-07-14). Identity proven by the ??_7CHelpState@@6B@
// +0x24 slot-9 data-ref (via thunk 0x3b43): the retail vtable's slot-9 entry points at
// 0x95140, so it is CHelpState's own slot-9 body, emitted in the helpstate obj (its
// RVA sits between LoadAssets 0x95090 and Render 0x951f0, both helpstate). The former
// CState95/CMenuHolder95/CWorkerObj95 views are gone.

// ---------------------------------------------------------------------------
// 0xb4350: a strike/flash effect tick - while the latch m_118 is set, pick the
// frame index (5, or 0 once a global threshold is reached) unless the strike timer
// has elapsed (which clears the latch), then seed the bound sprite's anim state
// (m_4c frame / m_50 = 7 / m_58 = 1). Always runs the trailing helper, returns 0.
extern "C" u32 g_frameTime; // tick
// g_timer200 (0x245598 countdown timer, compared to 0x64) comes from <Rez/FrameClock.h>.
extern "C" CGameRegistry* g_gameReg; // 0x64556c
extern "C" void Helper2914();        // 0x2914 (ILT thunk)

struct CStrikeSprite {
    char _00[0x4c];
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    char _54[0x58 - 0x54];
    i32 m_58; // +0x58
};
struct CStrikeEffect {
    char _00[0x10];
    CStrikeSprite* m_10; // +0x10
    char _14[0x118 - 0x14];
    i32 m_118; // +0x118 latch
    char _11c[0x120 - 0x11c];
    i64 m_120;  // +0x120 timestamp
    i64 m_128;  // +0x128 duration
    i32 Tick(); // 0xb4350
};

// @early-stop
// 98.94%: every opcode/offset/branch is byte-identical. The lone residual is a
// load-order coin-flip in the sprite-write tail - retail reads g_gameReg->m_78
// (edx) before m_10 (reusing eax for the sprite ptr); cl loads m_10 first (into ecx)
// and pins the sprite there. A pure allocator choice on the [this+0x10] load; no
// source reorder flips it.
// @interleaver CStrikeEffect::Tick emitted-in <boundary: unreconstructed>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are ufo CUFO::Tick
// @0xb4330 (before) and pathhazard CLightningHazard::SiblingTick @0xb43f0 (after),
// NOT a single reconstructed host on both sides. True obj is the unreconstructed
// leaf-tick pool at 0xb43xx; CStrikeEffect identity is a placeholder view.)
RVA(0x000b4350, 0x7e)
i32 CStrikeEffect::Tick() {
    if (m_118 != 0) {
        i32 idx = 5;
        if ((i64)(u32)g_frameTime - m_120 < m_128) {
            if ((u32)g_timer200 >= 0x64) {
                idx = 0;
            }
        } else {
            m_118 = 0;
        }
        i32 frame = (i32)g_gameReg->m_logicPump->m_tables[idx];
        CStrikeSprite* spr = m_10;
        spr->m_58 = 1;
        spr->m_4c = frame;
        spr->m_50 = 7;
    }
    Helper2914();
    return 0;
}
SIZE_UNKNOWN(CAnimOwner6b);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CStrikeEffect);
SIZE_UNKNOWN(CStrikeSprite);
