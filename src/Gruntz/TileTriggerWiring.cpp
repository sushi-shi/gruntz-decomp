// TileTriggerWiring.cpp - CTileTriggerWiring::AddLogicDefaults (0x1163b0): forward
// to the full AddLogic factory, supplying six zeroed 16-byte parameter blocks.
#include <Gruntz/TileTriggerWiring.h>

// ===========================================================================
// CTileTriggerWiring::AddLogicDefaults  (0x1163b0)
// ===========================================================================
// @early-stop
// Register-allocation wall (topic:regalloc). The forwarder structure is faithful -
// push the four trailing ids, build six in-place zeroed 16-byte param blocks, push
// the five leading ids, tail into AddLogic - and the zeroing-ctor temps reproduce
// retail's shared-zero-register stores (MSVC5 will NOT value-init `CTrigParam()`,
// so the explicit ctor is required: a no-ctor POD copied garbage, 27%->76%). The
// residual is purely which registers cl picks: retail loads the forwarded ids
// THROUGH ebx (reused as the block pointer) and keeps exactly FOUR zero regs
// (eax/edx/esi/edi) live across all six blocks; cl loads through ebp and spends a
// FIFTH zero reg (ebx), so every arg-load/zero-store operand shifts. A pure /O2
// regalloc coin-flip with no source lever. ~75.9%, logic complete; deferred to the
// final sweep.
RVA(0x001163b0, 0xb2)
void CTileTriggerWiring::
    AddLogicDefaults(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9) {
    AddLogic(
        type,
        a2,
        a3,
        a4,
        a5,
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        a6,
        a7,
        a8,
        a9
    );
}

// ===========================================================================
// CTileTriggerWiring::AddLogicFromRecord  (0x1164a0)
// ===========================================================================
// Same forward as AddLogicDefaults, but the five ids (a3/a4/a5) and the six
// CTrigParam blocks come from a source tile record instead of being zeroed:
// a3/a4/a5 = rec->m_164/m_168/m_4; p1..p6 = rec->m_134/m_144/m_154/m_64 and the
// +0x7c sub-object's m_f0/m_100. __thiscall, ret 0xc.
RVA(0x001164a0, 0x116)
void CTileTriggerWiring::AddLogicFromRecord(i32 type, i32 a2, CTrigSourceRecord* rec) {
    AddLogic(
        type,
        a2,
        rec->m_164,
        rec->m_168,
        rec->m_4,
        rec->m_134,
        rec->m_144,
        rec->m_154,
        rec->m_64,
        rec->m_7c->m_f0,
        rec->m_7c->m_100,
        rec->m_124,
        rec->m_120,
        rec->m_118,
        rec->m_128
    );
}
