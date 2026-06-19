// Grunt.cpp - the engine's CGrunt entity. Two byte-matched clusters: the 7
// contiguous HUD sprite creators and the 5 animation
// resolvers. Names are placeholders; only offsets + code
// bytes are load-bearing. Built /O2 /MT /GX (the resolvers' CString temporaries
// carry a C++ EH frame; /GX does NOT add an EH frame to the 7 creators - they
// stay byte-exact at their original percentages).
//
// HUD sprite-creator cluster (7/7 logic byte-exact; 5 at 99.3%+ = reloc-masked +
// a 2-instr edx/ecx register coin-flip, 2 at ~91.6% = the 2-arg Add* register-
// alloc/scheduling plateau - see below):
//   CGrunt::CreateHealthSprite()      99.31%
//   CGrunt::CreateToySprite()         91.70%  (2-arg plateau)
//   CGrunt::CreateStaminaSprite()     99.30%
//   CGrunt::CreateToyTimeSprite()     99.44%
//   CGrunt::CreateWingzTimeSprite()   99.41%
//   CGrunt::CreatePowerupSprite(int)  99.26%
//   CGrunt::CreateSelectedSprite()    91.54%  (2-arg plateau)
//
// Animation-resolver cluster (5/5 logic + CFG + member-offsets byte-exact; equal
// instruction counts; residue = reloc-masked EH/CString/call operands + an
// edx/ecx coin-flip on the `m_40 = m_38->m_1b4` store that no source lever flips,
// plus, for Idle/Battlecry, the compiler hoisting `idx+0xNNN` into a callee-saved
// reg early - all entropy-class):
//   CGrunt::ResolveMovingAnimation()    91.97%
//   CGrunt::ResolveDeathAnimation()     97.08%
//   CGrunt::ResolveAnimation()          94.71%
//   CGrunt::ResolveIdleAnimation()      91.38%
//   CGrunt::ResolveBattlecryAnimation() 89.58%
//
// Resolver levers (the load-bearing source forms):
//   * operator+ is AFXAPI = __stdcall (ret 0xc, callee-pops the hidden return
//     slot + both args) - declaring it plain __cdecl emits a bogus `add esp,0xc`
//     at each call site (caller-cleanup). +~4% across all 5.
//   * CString needs a user-declared ~CString() so
//     the two key-string temporaries get destruction calls under the C++ EH frame
//     - without it MSVC emits NO EH frame at all (the temps look trivially
//     destructible). This is what produced the fs:0 prolog/epilog. ~57%->89%.
//   * The visible-bounds gate caches `int x = h->m_5c; int y = h->m_60;` into
//     explicit locals so MSVC allocates the extra callee-saved regs the target
//     uses (edx/edi/ebp) instead of re-reading m_5c/m_60 per compare - +~6% on
//     Death. (Same "pin a local to force the 4th callee-saved reg" family as
//     ButeMgr::ParseTagLine.)
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
// sprite factory (the global registry ptr -> +0x30 -> +0x8, __thiscall) to build a
// named sprite from its class-name string + two HUD-geometry values derived from
// this->m_10 (+0x5c, and +0x60 optionally minus a per-sprite constant); store
// the sprite into the slot; run its slot-0x10 init virtual; register it into the
// grunt's sprite collection (sprite->m_7c->m_18 . Add*(args)). On a failed
// register: OR 0x10000 into the registrar's m_38->m_8 flag word, null the slot,
// return 0; else return 1.
#include <Gruntz/Grunt.h>
#include <rva.h>

// The sprite class-name string the factory is asked to build, per creator. These
// are literal .rodata strings in the binary (the reloc-masked DIR32 operand).
static const char s_GruntHealthSprite[]     = "GruntHealthSprite";
static const char s_GruntToySprite[]        = "GruntToySprite";
static const char s_GruntStaminaSprite[]    = "GruntStaminaSprite";
static const char s_GruntToyTimeSprite[]    = "GruntToyTimeSprite";
static const char s_GruntWingzTimeSprite[]  = "GruntWingzTimeSprite";
static const char s_GruntPowerupSprite[]    = "GruntPowerupSprite";
static const char s_GruntSelectedSprite[]   = "GruntSelectedSprite";

// The global manager pointer (reloc-masked).
CGameRegistry *g_pGameRegistry;

// ---------------------------------------------------------------------------
// Animation-resolver cluster (the 5 CGrunt::Resolve*Animation methods, the
// SECOND wave on this CGrunt TU). Each builds an animation-key string
//   "GRUNTZ_" + this->m_typeName + "_<CATEGORY>"
// (via the two engine global operator+ overloads -> a pair of stack CString
// temporaries with dtors -> the unit needs /GX for the C++ EH frame), feeds the
// resolved geometry source into the grunt's animation player (m_38), then looks
// the key up in the global animation tree (CButeTree::Find) and
// caches the result into m_14->m_1c. Several also fire a 5-arg on-screen "cue"
// (via g_pGameRegistry->m_60) gated on the grunt being inside the visible view
// rect (registry m_134==1 -> a 4-way bounds test).
//
//   CGrunt::ResolveMovingAnimation()    ("_MOVING", key "B")
//   CGrunt::ResolveDeathAnimation()     ("_DEATH",  key "C")
//   CGrunt::ResolveAnimation()          ("_JOY",    key "E")
//   CGrunt::ResolveIdleAnimation()      ("_IDLE",   key "A")
//   CGrunt::ResolveBattlecryAnimation() ("_BATTLECRY", key "F")
//
// The category suffix strings + the single-char tree keys are literal .rodata
// (reloc-masked DIR32 operands). The geometry source member differs per category
// (Moving m_7c, Death m_78, generic m_74, Idle m_58[idx], Battlecry m_68[idx]);
// Idle/Battlecry pick idx via the engine LCG rand (Idle %3+1, Battlecry %3).
static const char s_GRUNTZ_[]    = "GRUNTZ_";
static const char s__MOVING[]    = "_MOVING";
static const char s__DEATH[]     = "_DEATH";
static const char s__JOY[]       = "_JOY";
static const char s__IDLE[]      = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s_keyB[]       = "B";
static const char s_keyC[]       = "C";
static const char s_keyE[]       = "E";
static const char s_keyA[]       = "A";
static const char s_keyF[]       = "F";

// The global animation lookup tree (a CButeTree) + the rand seed
// default (reloc-masked).
CAnimLookupTree g_animLookupTree;
int  g_movingSeed;

// ---------------------------------------------------------------------------
// CGrunt::ResolveMovingAnimation()
// Gate: m_a8 == 0 (else return 0). Feed key "GRUNTZ_<type>_MOVING" + geometry
// m_7c into the player; look up tree key "B"; then randomize the move-start time
// (m_90 = (rand()%0x5dc1 + 0x1770)*10) and seed m_88/m_8c/m_94.
RVA(0x045100, 0x112)
int CGrunt::ResolveMovingAnimation()
{
    if (m_a8 != 0)
        return 0;

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__MOVING);

    m_40 = (int)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_7c);

    m_30 = (int)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyB);

    m_90 = (GruntRand() % 0x5dc1 + 0x1770) * 10;
    m_94 = 0;
    m_88 = g_movingSeed;
    m_8c = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveDeathAnimation()
// Gate: m_a8 == 0 (else return 0); then latch m_a8 = 1. Fire the on-screen cue
// (arg2 = m_ac), feed geometry m_78 then key "GRUNTZ_<type>_DEATH", look up "C".
RVA(0x0455f0, 0x15b)
int CGrunt::ResolveDeathAnimation()
{
    if (m_a8 != 0)
        return 0;
    m_a8 = 1;

    CGameRegistry *g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud *h = m_10;
        int x = h->m_5c;
        int y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140)
            g->m_60->Cue(h->m_188, m_ac, -1, -1, -1);
    } else {
        g->m_60->Cue(m_10->m_188, m_ac, -1, -1, -1);
    }

    m_40 = (int)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_78);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__DEATH);

    m_30 = (int)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyC);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveAnimation()  (generic "_JOY")
// Gate: m_a8 == 0 (else return 0). The cue arg2 is a fixed constant (0x435 when
// on-screen / 0x43f otherwise). Geometry m_74; key "GRUNTZ_<type>_JOY"; look "E".
RVA(0x0457b0, 0x14c)
int CGrunt::ResolveAnimation()
{
    if (m_a8 != 0)
        return 0;

    CGameRegistry *g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud *h = m_10;
        int x = h->m_5c;
        int y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140)
            g->m_60->Cue(h->m_188, 0x435, -1, -1, -1);
    } else {
        g->m_60->Cue(m_10->m_188, 0x43f, -1, -1, -1);
    }

    m_40 = (int)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_74);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__JOY);

    m_30 = (int)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyE);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveIdleAnimation()
// Gate: m_a8 == 0 (else return 0). Pick idx = rand()%3 + 1 (1..3); cue arg2 =
// idx+0x431 / idx+0x43b; geometry m_58[idx]; then read the active-anim
// descriptor's first element's m_14 as a 2nd lookup arg (SetAnimEx); key
// "GRUNTZ_<type>_IDLE"; look up "A".
RVA(0x045960, 0x181)
int CGrunt::ResolveIdleAnimation()
{
    if (m_a8 != 0)
        return 0;

    int idx = GruntRand() % 3 + 1;

    CGameRegistry *g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud *h = m_10;
        int x = h->m_5c;
        int y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140)
            g->m_60->Cue(h->m_188, idx + 0x431, -1, -1, -1);
    } else {
        g->m_60->Cue(m_10->m_188, idx + 0x43b, -1, -1, -1);
    }

    m_40 = (int)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_58[idx]);

    CAnimDescColl *desc = m_38->m_1b4;
    CAnimElem *elem = desc->m_10 > 0 ? *desc->m_c : 0;
    int frame = elem->m_14;

    m_38->SetAnimEx(s_GRUNTZ_ + m_typeName + s__IDLE, frame);

    m_30 = (int)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyA);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveBattlecryAnimation()
// Gate: m_a8 == 0 (else return 0). Pick idx = rand()%3 (0..2); cue arg2 =
// idx+0x42e / idx+0x438; geometry m_68[idx]; key "GRUNTZ_<type>_BATTLECRY";
// look up "F".
RVA(0x045b60, 0x161)
int CGrunt::ResolveBattlecryAnimation()
{
    if (m_a8 != 0)
        return 0;

    int idx = GruntRand() % 3;

    CGameRegistry *g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud *h = m_10;
        int x = h->m_5c;
        int y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140)
            g->m_60->Cue(h->m_188, idx + 0x42e, -1, -1, -1);
    } else {
        g->m_60->Cue(m_10->m_188, idx + 0x438, -1, -1, -1);
    }

    m_40 = (int)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_68[idx]);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__BATTLECRY);

    m_30 = (int)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyF);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateHealthSprite()
// Gate: m_healthSprite unset AND m_3ec > 0. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3ec).
RVA(0x04d130, 0xb5)
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
// CGrunt::CreateToySprite()
// Gate: m_toySprite unset. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddB(m_1ec, m_1f0).
RVA(0x04d220, 0x9c)
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
// CGrunt::CreateStaminaSprite()
// Gate: m_staminaSprite unset AND m_3f0 != 0x64. geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f0).
RVA(0x04d2f0, 0xb4)
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
// CGrunt::CreateToyTimeSprite()
// Gate: m_toyTimeSprite unset AND m_3f4 != 0. First clears the stamina sprite
// (m_1c8) and wingz-time sprite (m_1d0) if set (OR 0x10000 into their record's
// +0x8, null the slot). geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f4).
RVA(0x04d3e0, 0xf5)
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
// CGrunt::CreateWingzTimeSprite()
// Gate: m_wingzTimeSprite unset AND m_238 != 0 AND m_3f8 != 0. Clears the
// toy-time sprite (m_1cc) if set. geoB = m_60 - 0x26; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3f8).
RVA(0x04d520, 0xe3)
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
// CGrunt::CreatePowerupSprite(int a)
// Gate: m_powerupSprite unset. geoB = m_60 (no offset); hint 0x15.
// Register via AddC(m_1ec, m_1f0, a).
RVA(0x04d650, 0xa1)
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
// CGrunt::CreateSelectedSprite()
// Gate: m_selectedSprite unset. geoB = m_60 (no offset); hint 0x14.
// Register via AddD(m_1ec, m_1f0).
RVA(0x04d730, 0x96)
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

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x047a10, 0x770)
void CGrunt::Stub_047a10() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x048470, 0x131b)
void CGrunt::Stub_048470() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x062e10, 0x47e)
void CGrunt::Stub_062e10() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0633e0, 0x2ca)
void CGrunt::Stub_0633e0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x067bd0, 0x2ef)
void CGrunt::BuildEntranceAnimation() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x067f80, 0x313)
void CGrunt::LoadEntranceConfig() {}

// The global CButeMgr config singleton + the tuning key ReadConfigFromButeMgr
// reads. Minimal local decl (the full ButeMgr.h redefines CString, already
// pulled in by this TU), with only the typed getter the function calls.
#include <Bute/ButeMgr.h>
extern CButeMgr g_buteMgr;
static char s_TimePerTile[] = "TimePerTile";

// ---------------------------------------------------------------------------
// CGrunt::ReadConfigFromButeMgr
// Reads the TimePerTile tuning config from CButeMgr for this grunt's type.
// Applies a special-case halving for grunt kind 55 (0x37).
// ---------------------------------------------------------------------------
RVA(0x48400, 0x47)
void CGrunt::ReadConfigFromButeMgr()
{
    *(int *)((char *)this + 0x18c) = 0;
    *(int *)((char *)this + 0x418) = 0;

    *(int *)((char *)this + 0x41c) = g_buteMgr.GetDwordDef(
        *(char **)((char *)this + 0x1c0), s_TimePerTile, 1000);

    if (*(int *)((char *)this + 0x258) == 0x37)
        *(int *)((char *)this + 0x41c) >>= 1;
}
