// OrphanMethods.cpp - moderate orphan-COMDAT methods with no recoverable owning
// class. Each is modeled from its disassembly with PLACEHOLDER class/field names;
// only OFFSETS + code bytes are load-bearing. Engine callees are external/no-body.
#include <Ints.h>
#include <Gruntz/Effect6b.h>
#include <Gruntz/Grunt.h>            // CGrunt (the m_player pointee is the canonical CGameObject)
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
// 0x6b2e0: an animation effect apply - cache the owner's active descriptor into
// this->m_prevDesc, run
// the owner's embedded anim sub-object (+0x1a0) advance, and (when the flag arg is
// set) re-target its draw-delta.
// @early-stop
// 76%: every instruction (lea anim, descriptor read, m_prevDesc store, arg push, both calls) is
// byte-faithful; the residual is pure register coloring + a 2-instr scheduling flip
// in this 0x39-byte leaf - retail keeps m_1b4 in edx and hoists the `a` load into
// eax before the m_prevDesc store; cl colors the descriptor in eax and stores m_prevDesc first. Not
// source-steerable (every operand/declaration reorder reproduced the same coloring).
extern "C" u32 g_engineFrameDelta;

// CGameObject (<Gruntz/UserLogic.h>) - the +0x1a0 embedded cursor and its m_14
// active descriptor; see <Gruntz/Effect6b.h>.)

// @interleaver CEffect6b::Apply emitted-in <boundary: unreconstructed>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are ddrawsubmgrleaf
// @0x6b2a0 (before) and gamelevel PointInBounds @0x6b330 (after), NOT a single
// reconstructed host on both sides. True obj is the unreconstructed 0x6b2xx run.)
RVA(0x0006b2e0, 0x39)
void CEffect6b::Apply(i32 a, i32 b) {
    CAniAdvanceCursor* anim = &m_player->m_1a0;
    m_prevDesc = m_player->m_1a0.m_14;
    anim->Setup_15c2d0((CAniElement*)a);
    if (b != 0) {
        anim->Advance(static_cast<i32>(g_engineFrameDelta));
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
// 0xb4350: CRainCloud::Tick (vtable slot 16, origin CPathHazard). IDENTITY
// RESOLVED (2026-07-16, ex the `CStrikeEffect` placeholder): its ILT thunk
// 0x36a2 is referenced ONLY from ??_7CRainCloud@@6B@+0x40 (slot 16), and every
// viewed field is the canonical CPathHazard strike state - m_118 IS
// m_strikeArmed, +0x120/+0x128 ARE m_strikeDeadline/m_strikeWindow, +0x10 IS
// CUserLogic::m_object (the bound CGameObject, whose +0x4c/+0x50/+0x58 are the
// draw-fill triple). The trailing "helper" thunk 0x2914 IS the base
// ?Tick@CPathHazard@@ @0xb4020 - the same base chain CUFO::Tick makes.
// The lightning-strike flash: while armed, pick flash frame 5 (or 0 once the
// g_timer200 threshold passes) unless the strike window elapsed (disarm), seed
// the bound object's draw-fill, then run the base Tick; returns 0.
extern "C" u32 g_frameTime; // tick
// g_timer200 (0x245598 countdown timer, compared to 0x64) comes from <Rez/FrameClock.h>.
extern "C" CGameRegistry* g_gameReg; // 0x64556c
#include <Gruntz/RainCloud.h>        // the real owner (CPathHazard strike state + m_object)
#include <Gruntz/UserLogic.h>        // CGameObject (the bound object's draw-fill triple)

// @early-stop
// 98.94%: every opcode/offset/branch is byte-identical. The lone residual is a
// load-order coin-flip in the sprite-write tail - retail reads g_gameReg->m_78
// (edx) before m_object (reusing eax for the sprite ptr); cl loads m_object first
// (into ecx) and pins the sprite there. A pure allocator choice on the
// [this+0x10] load; no source reorder flips it.
// @interleaver CRainCloud::Tick emitted-in <boundary: unreconstructed>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are ufo CUFO::Tick
// @0xb4330 (before) and pathhazard CLightningHazard::SiblingTick @0xb43f0 (after);
// the raincloud unit's own block is at 0xb49b0, so this out-of-line member stays
// in this holding TU pending the 0xb43xx leaf-tick pool reconstruction.)
RVA(0x000b4350, 0x7e)
i32 CRainCloud::Tick() {
    if (m_strikeArmed != 0) {
        i32 idx = 5;
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_strikeDeadline < m_strikeWindow) {
            if (static_cast<u32>(g_timer200) >= 0x64) {
                idx = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        i32 frame = (i32)g_gameReg->m_logicPump->m_tables[idx];
        CGameObject* spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillArg = frame;
        spr->m_drawFillCmd = 7;
    }
    CPathHazard::Tick(); // the base chain (thunk 0x2914 -> 0xb4020), result unused
    return 0;
}
SIZE_UNKNOWN(CGameRegistry);
