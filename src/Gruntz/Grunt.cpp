// Grunt.cpp - the engine's CGrunt HUD sprite-creator cluster (7 contiguous
// __thiscall methods @0x04d130..0x04d730). Names are placeholders; only offsets
// + code bytes are load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):  7/7 logic byte-exact;
// 5 at 99.3%+ (reloc-masked + a 2-instr edx/ecx register coin-flip), 2 at ~91.6%
// (the 2-arg Add* register-alloc/scheduling plateau - see below).
//   CGrunt::CreateHealthSprite()      @ 0x04d130 (181 B, thiscall ret)   99.31%
//   CGrunt::CreateToySprite()         @ 0x04d220 (156 B, thiscall ret)   91.70%  (2-arg plateau)
//   CGrunt::CreateStaminaSprite()     @ 0x04d2f0 (180 B, thiscall ret)   99.30%
//   CGrunt::CreateToyTimeSprite()     @ 0x04d3e0 (245 B, thiscall ret)   99.44%
//   CGrunt::CreateWingzTimeSprite()   @ 0x04d520 (227 B, thiscall ret)   99.41%
//   CGrunt::CreatePowerupSprite(int)  @ 0x04d650 (161 B, thiscall ret 4) 99.26%
//   CGrunt::CreateSelectedSprite()    @ 0x04d730 (150 B, thiscall ret)   91.54%  (2-arg plateau)
//
// Every member offset, gate, the factory call, the init virtual, the failure-path
// flag write (registrar->m_38->m_8 |= 0x10000), the slot-null, and both returns
// are byte-exact across all 7 (equal instruction counts). The residue:
//   * 3-arg Add (Health/Stamina/ToyTime/WingzTime/Powerup): the registrar `this`
//     is computed via an explicit `CSpriteInner *inner = sprite->m_7c;` temp so
//     MSVC keeps m_7c in eax (in-place reuse of the reloaded-sprite reg) and
//     interleaves `mov edi,[eax+0x18]` between the arg pushes - exactly the target
//     schedule (95.8% -> 99.3%). Lone residue = which of edx/ecx temporarily holds
//     arg c (m_3ec/m_3f0/m_3f4/m_3f8) vs arg b (m_1f0): the push ORDER and VALUES
//     are byte-identical, only the temp register-field differs (a pure allocator
//     coin-flip; explicit `int c=..; int b=..;` temps did NOT flip it).
//   * 2-arg Add (Toy/Selected): with ONLY two args MSVC hoists arg a (m_1ec) into
//     a reg BEFORE computing the registrar (target loads it AFTER), and puts m_7c
//     in ecx not eax. The `inner` temp makes it WORSE here (90.7%); the plain
//     `reg = sprite->m_7c->m_18;` form is best (91.6%). Six source forms (inner
//     temp, sprite-local, preload-b, explicit a/b temps, full-inline, plain) all
//     normalize to one of two valid schedules; no lever flips the 2-arg ordering.
//     Entropy-class (the 2-arg variant has one fewer arg to pin the schedule).
//     Logic/offsets/CFG byte-exact; left per the campaign doctrine.
//
// Shared shape: bail (return 0) if the target sprite slot is already populated
// (and, for several, if a stat/flags gate is unset); else ask the global HUD
// sprite factory (*0x64556c -> +0x30 -> +0x8, __thiscall @0x1597b0) to build a
// named sprite from its class-name string + two HUD-geometry values derived from
// this->m_10 (+0x5c, and +0x60 optionally minus a per-sprite constant); store
// the sprite into the slot; run its slot-0x10 init virtual; register it into the
// grunt's sprite collection (sprite->m_7c->m_18 . Add*(args)). On a failed
// register: OR 0x10000 into the registrar's m_38->m_8 flag word, null the slot,
// return 0; else return 1.
#include "Grunt.h"

// The sprite class-name string the factory is asked to build, per creator. These
// are literal .rodata strings in the binary (the reloc-masked DIR32 operand).
static const char s_GruntHealthSprite[]     = "GruntHealthSprite";
static const char s_GruntToySprite[]        = "GruntToySprite";
static const char s_GruntStaminaSprite[]    = "GruntStaminaSprite";
static const char s_GruntToyTimeSprite[]    = "GruntToyTimeSprite";
static const char s_GruntWingzTimeSprite[]  = "GruntWingzTimeSprite";
static const char s_GruntPowerupSprite[]    = "GruntPowerupSprite";
static const char s_GruntSelectedSprite[]   = "GruntSelectedSprite";

// The global manager pointer at binary 0x64556c (reloc-masked).
CGameRegistry *g_pGameRegistry;

// ---------------------------------------------------------------------------
// CGrunt::CreateHealthSprite()  @ 0x04d130
// Gate: m_healthSprite unset AND m_3ec > 0. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3ec).
// ---------------------------------------------------------------------------
int CGrunt::CreateHealthSprite()
{
    if (m_healthSprite || m_3ec <= 0)
        return 0;

    m_healthSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60 - 0x19, 0xdbba0, s_GruntHealthSprite, 0x40003);
    m_healthSprite->m_7c->m_init(m_healthSprite);

    CSpriteInner *inner = m_healthSprite->m_7c;
    CSpriteRegistrar *reg = inner->m_18;
    if (!reg->AddA(m_1ec, m_1f0, m_3ec)) {
        reg->m_38->m_8 |= 0x10000;
        m_healthSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateToySprite()  @ 0x04d220
// Gate: m_toySprite unset. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddB(m_1ec, m_1f0).
// ---------------------------------------------------------------------------
int CGrunt::CreateToySprite()
{
    if (m_toySprite)
        return 0;

    m_toySprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60 - 0x19, 0xdbba0, s_GruntToySprite, 0x40003);
    m_toySprite->m_7c->m_init(m_toySprite);

    CSpriteRegistrar *reg = m_toySprite->m_7c->m_18;
    if (!reg->AddB(m_1ec, m_1f0)) {
        reg->m_38->m_8 |= 0x10000;
        m_toySprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateStaminaSprite()  @ 0x04d2f0
// Gate: m_staminaSprite unset AND m_3f0 != 0x64. geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f0).
// ---------------------------------------------------------------------------
int CGrunt::CreateStaminaSprite()
{
    if (m_staminaSprite || m_3f0 == 0x64)
        return 0;

    m_staminaSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60 - 0x20, 0xdbba0, s_GruntStaminaSprite, 0x40003);
    m_staminaSprite->m_7c->m_init(m_staminaSprite);

    CSpriteInner *inner = m_staminaSprite->m_7c;
    CSpriteRegistrar *reg = inner->m_18;
    if (!reg->AddA(m_1ec, m_1f0, m_3f0)) {
        reg->m_38->m_8 |= 0x10000;
        m_staminaSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateToyTimeSprite()  @ 0x04d3e0
// Gate: m_toyTimeSprite unset AND m_3f4 != 0. First clears the stamina sprite
// (m_1c8) and wingz-time sprite (m_1d0) if set (OR 0x10000 into their record's
// +0x8, null the slot). geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f4).
// ---------------------------------------------------------------------------
int CGrunt::CreateToyTimeSprite()
{
    if (m_toyTimeSprite || m_3f4 == 0)
        return 0;

    if (m_staminaSprite) {
        ((CSpriteRegRecord *)m_staminaSprite)->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_wingzTimeSprite) {
        ((CSpriteRegRecord *)m_wingzTimeSprite)->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }

    m_toyTimeSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60 - 0x20, 0xdbba0, s_GruntToyTimeSprite, 0x40003);
    m_toyTimeSprite->m_7c->m_init(m_toyTimeSprite);

    CSpriteInner *inner = m_toyTimeSprite->m_7c;
    CSpriteRegistrar *reg = inner->m_18;
    if (!reg->AddA(m_1ec, m_1f0, m_3f4)) {
        reg->m_38->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateWingzTimeSprite()  @ 0x04d520
// Gate: m_wingzTimeSprite unset AND m_238 != 0 AND m_3f8 != 0. Clears the
// toy-time sprite (m_1cc) if set. geoB = m_60 - 0x26; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f8).
// ---------------------------------------------------------------------------
int CGrunt::CreateWingzTimeSprite()
{
    if (m_wingzTimeSprite || m_238 == 0 || m_3f8 == 0)
        return 0;

    if (m_toyTimeSprite) {
        ((CSpriteRegRecord *)m_toyTimeSprite)->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }

    m_wingzTimeSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60 - 0x26, 0xdbba0, s_GruntWingzTimeSprite, 0x40003);
    m_wingzTimeSprite->m_7c->m_init(m_wingzTimeSprite);

    CSpriteInner *inner = m_wingzTimeSprite->m_7c;
    CSpriteRegistrar *reg = inner->m_18;
    if (!reg->AddA(m_1ec, m_1f0, m_3f8)) {
        reg->m_38->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreatePowerupSprite(int a)  @ 0x04d650  (ret 4)
// Gate: m_powerupSprite unset. geoB = m_60 (no offset); hint 0x15.
// Register via AddC(m_1ec, m_1f0, a).
// ---------------------------------------------------------------------------
int CGrunt::CreatePowerupSprite(int a)
{
    if (m_powerupSprite)
        return 0;

    m_powerupSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60, 0x15, s_GruntPowerupSprite, 0x40003);
    m_powerupSprite->m_7c->m_init(m_powerupSprite);

    CSpriteInner *inner = m_powerupSprite->m_7c;
    CSpriteRegistrar *reg = inner->m_18;
    if (!reg->AddC(m_1ec, m_1f0, a)) {
        reg->m_38->m_8 |= 0x10000;
        m_powerupSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateSelectedSprite()  @ 0x04d730
// Gate: m_selectedSprite unset. geoB = m_60 (no offset); hint 0x14.
// Register via AddD(m_1ec, m_1f0).
// ---------------------------------------------------------------------------
int CGrunt::CreateSelectedSprite()
{
    if (m_selectedSprite)
        return 0;

    m_selectedSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0, m_10->m_5c, m_10->m_60, 0x14, s_GruntSelectedSprite, 0x40003);
    m_selectedSprite->m_7c->m_init(m_selectedSprite);

    CSpriteRegistrar *reg = m_selectedSprite->m_7c->m_18;
    if (!reg->AddD(m_1ec, m_1f0)) {
        reg->m_38->m_8 |= 0x10000;
        m_selectedSprite = 0;
        return 0;
    }
    return 1;
}
