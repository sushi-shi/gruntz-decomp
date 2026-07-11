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
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_cmdGrid)

// The rock-break particle spawner (canonical body in RockBreakParticles.cpp); reached
// through the registry's +0x68 command-grid slot in this effect context.
class CRockBreakMgr {
public:
    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4); // 0x7b440
};

extern "C" u32 g_6bf3bc;             // 0x6bf3bc  per-frame draw delta (advance ctx)
extern "C" CGameRegistry* g_gameReg; // *0x64556c the game-registry singleton

// The target game object (this->m_10): world position + the +0x114 state union.
SIZE_UNKNOWN(RbTarget);
struct RbTarget {
    char p0[0x5c];
    i32 m_5c, m_60; // +0x5c  world position (particle spawn center)
    char p64[0x114 - 0x64];
    i32 m_114; // +0x114  state union (==1 -> gates the particle spawn)
    char p118[0x124 - 0x118];
    i32 m_124; // +0x124  spawn param
};

// The effect sprite (this->m_38): a game object with a CAniAdvanceCursor at +0x1a0
// plus the +0x8 goal-flag word and the +0x1c0/+0x1c8 arm/consume gates.
SIZE_UNKNOWN(RbSprite);
struct RbSprite {
    char p0[0x8];
    i32 m_8; // +0x8  flags (|= 0x10000 goal mark)
    char pc[0x1c0 - 0xc];
    i32 m_1c0; // +0x1c0  consumed gate
    char p1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8  armed gate
    CAniAdvanceCursor* cursor() {
        return (CAniAdvanceCursor*)((char*)this + 0x1a0);
    }
};

SIZE_UNKNOWN(RbEffect);
struct RbEffect {
    char p0[0x10];
    RbTarget* m_10; // +0x10  target object
    char p14[0x38 - 0x14];
    RbSprite* m_38; // +0x38  effect sprite
    i32 Update();   // 0x476b0
};

// @early-stop
// Regalloc wall on the tail (~90%): `this` is dead after the spawn, so retail reuses
// esi for the re-read `this->m_38` (mov esi,[esi+0x38]) and reads the arm/consume gates
// via esi; cl keeps this in esi and caches m_38 in eax (mov eax,[esi+0x38]) + reads via
// eax/ecx. Same instructions, only the ModRM register field differs; not source-
// steerable (a cached-pointer local didn't flip it). Logic + all relocs exact.
RVA(0x000476b0, 0x69)
i32 RbEffect::Update() {
    if (m_38->cursor()->Advance_15c360(g_6bf3bc) == 1) {
        RbTarget* t = m_10;
        if (t->m_114 == 1) {
            ((CRockBreakMgr*)g_gameReg->m_cmdGrid)
                ->BuildRockBreakParticles(t->m_5c, t->m_60, 1, t->m_124);
        }
    }
    if (m_38->m_1c8 != 0 && m_38->m_1c0 == 0) {
        m_38->m_8 |= 0x10000;
    }
    return 0;
}
