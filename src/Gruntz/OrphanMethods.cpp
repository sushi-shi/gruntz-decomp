// OrphanMethods.cpp - moderate orphan-COMDAT methods with no recoverable owning
// class. Each is modeled from its disassembly with PLACEHOLDER class/field names;
// only OFFSETS + code bytes are load-bearing. Engine callees are external/no-body.
#include <Ints.h>
#include <Gruntz/Effect6b.h>
#include <DDrawMgr/DDrawBlitParam.h>     // CDDrawBlitParam::Setup_15c2d0 (0x15c2d0) - +0x1a0 geo setter
#include <Gruntz/AniAdvanceCursor.h>     // CAniAdvanceCursor::Advance_15c360 (0x15c360) - +0x1a0 advance
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Gruntz/SoundFxEmitter.h> // CSoundFxEmitter::Method_fa8f0 (0xfa8f0) - shared body dispatched
// (FadeInTitle @0xfa1f0 is now a CState base method via <Gruntz/State.h>; no Attract.h.)
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
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

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
    ((CDDrawBlitParam*)anim)->Setup_15c2d0((CDDrawBlitParamSrc*)a);
    if (b != 0) {
        ((CAniAdvanceCursor*)anim)->Advance_15c360((i32)g_6bf3bc);
    }
}

// ---------------------------------------------------------------------------
// (CGridLookup::Lookup 0x75a40 is merged into src/Gruntz/TriggerMgrHitTest.cpp -
// called only by that TU's megafn FUN_6f2f0; interval verdict.)

// ---------------------------------------------------------------------------
// 0x95140: a state-machine step - poke the input sub-object, gate on the worker
// being busy or acquirable, then run the start sequence + report. Returns 1 on the
// full path, 0 on either early-out.
struct CMenuHolder95 {
    char _00[4];
    CDDrawSubMgrPages* m_4; // +0x04
};

struct CState95 {
    char _00[4];
    CGruntzMgr* m_4; // +0x04
    char _08[0xc - 8];
    CMenuHolder95* m_c; // +0x0c
    i32 Step(i32 arg);  // 0x95140
    // Start/Report are SHARED bodies: FadeInTitle @0xfa1f0 is a CState base method
    // (so `this` reaches it via its true base CState, not a sibling cross-cast), and
    // Method_fa8f0 @0xfa8f0 is CSoundFxEmitter's. CState95's own identity is still
    // unrecovered (@identity-TODO) so it stays a local placeholder whose object IS a
    // CState-base (the (CState*)this reinterpret is honest; the (CAttract*) sibling
    // cross-cast it replaced was a fakeness).
};

// @interleaver CState95::Step emitted-in helpstate - blocked: CState95 is a local
// placeholder view (this .cpp), identity unrecovered. Retail emits this COMDAT INSIDE
// helpstate's block (0x95090 CHelpState::LoadAssets .. 0x951f0 CHelpState::Render,
// both helpstate) - a rule-(c) interleaver surrounded by helpstate on both sides, so
// CState95 is very likely a sibling menu/help state. Homing to HelpState.cpp is
// blocked until CState95's identity is recovered into a shared header (@identity-TODO).
RVA(0x00095140, 0x6e)
i32 CState95::Step(i32 arg) {
    m_4->RestoreVideoMode(0);
    if (m_c->m_4->Method_158d20() == 0 && m_c->m_4->Method_158cb0(0, 0x30000) == 0) {
        return 0;
    }
    if (((CState*)this)->FadeInTitle((const char*)&g_6111b0, 0, 0, 0, 0, 1) == 0) {
        return 0;
    }
    ((CSoundFxEmitter*)this)->Method_fa8f0(0x50, 0x3e8, 0, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xb4350: a strike/flash effect tick - while the latch m_118 is set, pick the
// frame index (5, or 0 once a global threshold is reached) unless the strike timer
// has elapsed (which clears the latch), then seed the bound sprite's anim state
// (m_4c frame / m_50 = 7 / m_58 = 1). Always runs the trailing helper, returns 0.
DATA(0x00245588)
extern "C" u32 g_645588;   // tick
extern i32 g_strikeThresh; // 0x645598
DATA(0x0024556c)
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
        if ((i64)(u32)g_645588 - m_120 < m_128) {
            if ((u32)g_strikeThresh >= 0x64) {
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
SIZE_UNKNOWN(CGridLookup);
SIZE_UNKNOWN(CMenuHolder95);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CState95);
SIZE_UNKNOWN(CStrikeEffect);
SIZE_UNKNOWN(CStrikeSprite);
SIZE_UNKNOWN(CWorkerObj95);
