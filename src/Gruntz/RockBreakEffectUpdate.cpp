// RockBreakEffectUpdate.cpp - CExplosion::Update @0x476b0, a Ghidra-missed
// per-frame effect driver (__thiscall; a leaf between LogicActReg's registrar
// 0x474b0 and CGrunt 0x47a10).
//
// IDENTITY SETTLED 2026-07-29 (was @identity-TODO "RbEffect"): FortressFlag's
// RegisterXLogic_6447f8 @0x474b0 stores ILT 0x4041ec -> 0x476b0 into
// CActRegPool<CExplosion>::s_table. An act table only ever holds its OWN class's
// CUserLogic member fn, so this body is CExplosion::Update - and the ex-placeholder's
// two modelled fields land exactly on CUserLogic::m_object (+0x10) and m_38 (+0x38),
// both CWwdGameObjectA*. 0x476b0 also sits inside the fortressflag obj's own band
// (registrar 0x474b0 + 0x18d = 0x4763d), which is where the partition note below
// already suspected it belonged.
//
// It drives an effect sprite (m_38, with a CAniAdvanceCursor embedded at +0x1a0) over
// a target game object (m_object). When the anim advance completes AND the target's
// +0x114 state == 1, spawn rock-break particles at the target's world position; then
// mark the effect's goal flag when it is armed (+0x1c8 set) and not yet consumed
// (+0x1c0 clear).
// FOLLOW-UP: the body should be re-homed into FortressFlag.cpp (its RVA is contiguous
// with that obj's block); left in its own unit here so the fold stays byte-local.
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/UserLogic.h>    // CGameObject (the target + effect sprite are both one)
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_cmdGrid)
#include <Gruntz/TriggerMgr.h> // BuildRockBreakParticles (ex CRockBreakMgr - dissolved onto CTriggerMgr)
#include <Gruntz/Explosion.h> // CExplosion - the owning leaf (ex-`RbEffect`; see Explosion.h)

// Its bound target (m_object, +0x114 state gates the spawn) and its effect sprite
// (m_38, CAniAdvanceCursor @+0x1a0, +0x1c0/+0x1c8 gates) are BOTH real CGameObjects
// (world pos @+0x5c/+0x60, flags @+0x08).

// @early-stop
RVA(0x000476b0, 0x69)
i32 CExplosion::Update() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) == 1) {
        CWwdGameObjectA* t = m_object;
        if (t->m_114 == 1) {
            g_gameReg->m_cmdGrid->BuildRockBreakParticles(t->m_screenX, t->m_screenY, 1, t->m_124);
        }
    }
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}
