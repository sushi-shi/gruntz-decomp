// RockBreakEffectUpdate.cpp - 0x476b0, a Ghidra-missed per-frame effect Update
// (__thiscall, void; a leaf between LogicActReg's registrar 0x474b0 and CGrunt 0x47a10).
// @identity-TODO: the owning class is unrecovered (no xref/RTTI). It drives an effect
// sprite (this->m_38, a game object with a CAniAdvanceCursor embedded at +0x1a0) over a
// target game object (this->m_10). When the anim advance completes AND the target's
// +0x114 state == 1, spawn rock-break particles at the target's world position; then
// mark the effect's goal flag when it is armed (+0x1c8 set) and not yet consumed (+0x1c0
// clear). Field names are placeholders; only offsets + code bytes are load-bearing.
// wave3-I partition note: Update@RbEffect @0x476b0 sits at the exact boundary
// between the fortressflag obj (ff+particlez+explosion, ends ~0x4763d) and the
// grunt-main obj (frags @0x47740+, ctor @0x47a10). No private .data cells or
// frags pin it to either side; the TU_MIGRATION MOVE row's "-> grunt" was the
// dominant-unit heuristic only. Left in its own unit pending stronger evidence
// (@identity-TODO).
#include <Ints.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/UserLogic.h>    // CGameObject (the target + effect sprite are both one)
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_cmdGrid)
#include <Gruntz/TriggerMgr.h> // BuildRockBreakParticles (ex CRockBreakMgr - dissolved onto CTriggerMgr)

extern "C" u32 g_engineFrameDelta;   // 0x6bf3bc  per-frame draw delta (advance ctx)
extern "C" CGameRegistry* g_gameReg; // *0x64556c the game-registry singleton

// The effect leaf: its bound target (m_10, +0x114 state gates the spawn) and its
// effect sprite (m_38, CAniAdvanceCursor @+0x1a0, +0x1c0/+0x1c8 gates) are BOTH real
// CGameObjects (world pos @+0x5c/+0x60, flags @+0x08 - proven: the ex RbTarget/RbSprite
// .cpp-local views are the same class at the same offsets, now dissolved onto
// CGameObject <Gruntz/UserLogic.h>). @identity-TODO: this owning leaf's own class name
// is unrecovered (orphan COMDAT @0x476b0 - no caller/new-site/RTTI/vtable-dispatch; the
// applicable techniques - caller xref, callee sigs (CAniAdvanceCursor::Advance +
// CRockBreakMgr::BuildRockBreakParticles name the callees not the owner), new-site,
// COL/RTTI - all dead-end), so the leaf stays a flagged local placeholder.
SIZE_UNKNOWN(RbEffect);
struct RbEffect {
    char p0[0x10];
    CGameObject* m_10; // +0x10  target object
    char p14[0x38 - 0x14];
    CGameObject* m_38; // +0x38  effect sprite
    i32 Update();      // 0x476b0
};

// @early-stop
// Regalloc wall on the tail (~90%): `this` is dead after the spawn, so retail reuses
// esi for the re-read `this->m_38` (mov esi,[esi+0x38]) and reads the arm/consume gates
// via esi; cl keeps this in esi and caches m_38 in eax (mov eax,[esi+0x38]) + reads via
// eax/ecx. Same instructions, only the ModRM register field differs; not source-
// steerable (a cached-pointer local didn't flip it). Logic + all relocs exact.
RVA(0x000476b0, 0x69)
i32 RbEffect::Update() {
    if (((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance(g_engineFrameDelta) == 1) {
        CGameObject* t = m_10;
        if (t->m_114 == 1) {
            g_gameReg->m_cmdGrid->BuildRockBreakParticles(t->m_screenX, t->m_screenY, 1, t->m_124);
        }
    }
    if (m_38->m_1c8 != 0 && m_38->m_1c0 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}
