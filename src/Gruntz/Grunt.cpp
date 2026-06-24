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
#include <math.h>
#include <string.h>

// The sprite class-name string the factory is asked to build, per creator. These
// are literal .rodata strings in the binary (the reloc-masked DIR32 operand).
static const char s_GruntHealthSprite[] = "GruntHealthSprite";
static const char s_GruntToySprite[] = "GruntToySprite";
static const char s_GruntStaminaSprite[] = "GruntStaminaSprite";
static const char s_GruntToyTimeSprite[] = "GruntToyTimeSprite";
static const char s_GruntWingzTimeSprite[] = "GruntWingzTimeSprite";
static const char s_GruntPowerupSprite[] = "GruntPowerupSprite";
static const char s_GruntSelectedSprite[] = "GruntSelectedSprite";

// The global manager pointer (reloc-masked).
CGameRegistry* g_pGameRegistry;

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
static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

// The global animation lookup tree (a CButeTree) + the rand seed
// default (reloc-masked).
CAnimLookupTree g_animLookupTree;
i32 g_movingSeed;

// Entrance-animation globals (reloc-masked; see Grunt.h).
CEntranceAnimSrc g_entranceAnimSrc;   // DAT_006bf620
CAnimNameResolver g_animNameResolver; // DAT_006bf650
i32 g_focusedGruntSentinel;           // DAT_00644c54

// The global CButeMgr config singleton + the tuning keys this TU reads. Minimal
// local decl (the full ButeMgr.h redefines CString, already pulled in by this
// TU), with only the typed getter the functions call.
#include <Bute/ButeMgr.h>
extern CButeMgr g_buteMgr;
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac

// The single-char anim-set keys the entrance reads/looks-up (reloc-masked
// .rodata; DAT_0060a454 = "A" = the idle anim key, DAT_0060d7f8 = "K" =
// BuildEntranceAnimation's latch key).
static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";

// The global running game clock (DAT_00645588) snapshotted into m_840.
extern "C" u32 g_645588;

// The global default geometry source the entrance geometry-state setter consumes
// (g_defaultGeo @0x6bf3bc; defined in SpriteResource.cpp, reloc-masked here).
extern i32 g_defaultGeo;

// ---------------------------------------------------------------------------
// CGrunt::ResolveMovingAnimation()
// Gate: m_a8 == 0 (else return 0). Feed key "GRUNTZ_<type>_MOVING" + geometry
// m_7c into the player; look up tree key "B"; then randomize the move-start time
// (m_90 = (rand()%0x5dc1 + 0x1770)*10) and seed m_88/m_8c/m_94.
RVA(0x00045100, 0x112)
i32 CGrunt::ResolveMovingAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__MOVING);

    m_40 = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_7c);

    m_30 = (i32)m_14->m_1c;
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
RVA(0x000455f0, 0x15b)
i32 CGrunt::ResolveDeathAnimation() {
    if (m_a8 != 0) {
        return 0;
    }
    m_a8 = 1;

    CGameRegistry* g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud* h = m_10;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            g->m_60->Cue(h->m_188, m_ac, -1, -1, -1);
        }
    } else {
        g->m_60->Cue(m_10->m_188, m_ac, -1, -1, -1);
    }

    m_40 = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_78);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__DEATH);

    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyC);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveAnimation()  (generic "_JOY")
// Gate: m_a8 == 0 (else return 0). The cue arg2 is a fixed constant (0x435 when
// on-screen / 0x43f otherwise). Geometry m_74; key "GRUNTZ_<type>_JOY"; look "E".
RVA(0x000457b0, 0x14c)
i32 CGrunt::ResolveAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    CGameRegistry* g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud* h = m_10;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            g->m_60->Cue(h->m_188, 0x435, -1, -1, -1);
        }
    } else {
        g->m_60->Cue(m_10->m_188, 0x43f, -1, -1, -1);
    }

    m_40 = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_74);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__JOY);

    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyE);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveIdleAnimation()
// Gate: m_a8 == 0 (else return 0). Pick idx = rand()%3 + 1 (1..3); cue arg2 =
// idx+0x431 / idx+0x43b; geometry m_58[idx]; then read the active-anim
// descriptor's first element's m_14 as a 2nd lookup arg (SetAnimEx); key
// "GRUNTZ_<type>_IDLE"; look up "A".
RVA(0x00045960, 0x181)
i32 CGrunt::ResolveIdleAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3 + 1;

    CGameRegistry* g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud* h = m_10;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            g->m_60->Cue(h->m_188, idx + 0x431, -1, -1, -1);
        }
    } else {
        g->m_60->Cue(m_10->m_188, idx + 0x43b, -1, -1, -1);
    }

    m_40 = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_58[idx]);

    CAnimDescColl* desc = m_38->m_1b4;
    CAnimElem* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = elem->m_14;

    m_38->SetAnimEx(s_GRUNTZ_ + m_typeName + s__IDLE, frame);

    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyA);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveBattlecryAnimation()
// Gate: m_a8 == 0 (else return 0). Pick idx = rand()%3 (0..2); cue arg2 =
// idx+0x42e / idx+0x438; geometry m_68[idx]; key "GRUNTZ_<type>_BATTLECRY";
// look up "F".
RVA(0x00045b60, 0x161)
i32 CGrunt::ResolveBattlecryAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3;

    CGameRegistry* g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud* h = m_10;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            g->m_60->Cue(h->m_188, idx + 0x42e, -1, -1, -1);
        }
    } else {
        g->m_60->Cue(m_10->m_188, idx + 0x438, -1, -1, -1);
    }

    m_40 = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_68[idx]);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__BATTLECRY);

    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyF);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateHealthSprite()
// Gate: m_healthSprite unset AND m_3ec > 0. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddA(m_1ec, m_1f0, m_3ec).
RVA(0x0004d130, 0xb5)
i32 CGrunt::CreateHealthSprite() {
    if (m_healthSprite || m_3ec <= 0) {
        return 0;
    }

    m_healthSprite =
        g_pGameRegistry->m_30->m_8
            ->CreateSprite(0, m_10->m_5c, m_10->m_60 - 0x19, 0xdbba0, s_GruntHealthSprite, 0x40003);
    m_healthSprite->m_7c->m_init(m_healthSprite);

    CSpriteInner* inner = m_healthSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
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
RVA(0x0004d220, 0x9c)
i32 CGrunt::CreateToySprite() {
    if (m_toySprite) {
        return 0;
    }

    m_toySprite =
        g_pGameRegistry->m_30->m_8
            ->CreateSprite(0, m_10->m_5c, m_10->m_60 - 0x19, 0xdbba0, s_GruntToySprite, 0x40003);
    m_toySprite->m_7c->m_init(m_toySprite);

    CSpriteRegistrar* reg = m_toySprite->m_7c->m_18;
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
RVA(0x0004d2f0, 0xb4)
i32 CGrunt::CreateStaminaSprite() {
    if (m_staminaSprite || m_3f0 == 0x64) {
        return 0;
    }

    m_staminaSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0,
        m_10->m_5c,
        m_10->m_60 - 0x20,
        0xdbba0,
        s_GruntStaminaSprite,
        0x40003
    );
    m_staminaSprite->m_7c->m_init(m_staminaSprite);

    CSpriteInner* inner = m_staminaSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
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
RVA(0x0004d3e0, 0xf5)
i32 CGrunt::CreateToyTimeSprite() {
    if (m_toyTimeSprite || m_3f4 == 0) {
        return 0;
    }

    if (m_staminaSprite) {
        ((CSpriteRegRecord*)m_staminaSprite)->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_wingzTimeSprite) {
        ((CSpriteRegRecord*)m_wingzTimeSprite)->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }

    m_toyTimeSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0,
        m_10->m_5c,
        m_10->m_60 - 0x20,
        0xdbba0,
        s_GruntToyTimeSprite,
        0x40003
    );
    m_toyTimeSprite->m_7c->m_init(m_toyTimeSprite);

    CSpriteInner* inner = m_toyTimeSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
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
RVA(0x0004d520, 0xe3)
i32 CGrunt::CreateWingzTimeSprite() {
    if (m_wingzTimeSprite || m_238 == 0 || m_3f8 == 0) {
        return 0;
    }

    if (m_toyTimeSprite) {
        ((CSpriteRegRecord*)m_toyTimeSprite)->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }

    m_wingzTimeSprite = g_pGameRegistry->m_30->m_8->CreateSprite(
        0,
        m_10->m_5c,
        m_10->m_60 - 0x26,
        0xdbba0,
        s_GruntWingzTimeSprite,
        0x40003
    );
    m_wingzTimeSprite->m_7c->m_init(m_wingzTimeSprite);

    CSpriteInner* inner = m_wingzTimeSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
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
RVA(0x0004d650, 0xa1)
i32 CGrunt::CreatePowerupSprite(i32 a) {
    if (m_powerupSprite) {
        return 0;
    }

    m_powerupSprite =
        g_pGameRegistry->m_30->m_8
            ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0x15, s_GruntPowerupSprite, 0x40003);
    m_powerupSprite->m_7c->m_init(m_powerupSprite);

    CSpriteInner* inner = m_powerupSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
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
RVA(0x0004d730, 0x96)
i32 CGrunt::CreateSelectedSprite() {
    if (m_selectedSprite) {
        return 0;
    }

    m_selectedSprite =
        g_pGameRegistry->m_30->m_8
            ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0x14, s_GruntSelectedSprite, 0x40003);
    m_selectedSprite->m_7c->m_init(m_selectedSprite);

    CSpriteRegistrar* reg = m_selectedSprite->m_7c->m_18;
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
RVA(0x00047a10, 0x770)
void CGrunt::Stub_047a10() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00048470, 0x131b)
void CGrunt::Stub_048470(i32, i32) {}

// ---------------------------------------------------------------------------
// CGrunt::LoadAnimNameTable(int kind, int toyOnly)   @0x49c60
// Fills the per-pose animation-name index table (m_394..m_3d4) by looking up
// "GRUNTZ_" + this->m_animSetName + "_<POSE>" in the entrance player's
// name->animset hash (m_154->m_c->m_2c->m_10map). __thiscall, ret 8 (/GX - the
// two operator+ CString temporaries per block carry a C++ EH frame).
//
//   * kind==0  : load the full grunt pose set (WALK/ATTACK*/STRUCK*/IDLE1..5/
//                ITEM*/DEATH; 14 poses).
//   * kind!=0, toyOnly!=0 : reload only WALK + TOY-BREAK.
//   * kind!=0, toyOnly==0 : reload the toy poses (TOY1, TOY2, TOY-BREAK) and
//                derive the toy-swap blend percent m_190 from the relative
//                animation lengths (node->m_10) of TOY1 vs TOY2.
//
// The pose-suffix strings + "GRUNTZ_" prefix are literal .rodata (reloc-masked
// DIR32 operands). Each `LOAD_POSE` builds the key via the two AFXAPI (__stdcall)
// operator+ overloads -> a pair of stack CString temps -> CEntranceHashTable::
// Lookup(key, &out) (writes node->m_c on a hit; out defaults 0).
static const char s_pose_WALK[] = "_WALK";
static const char s_pose_ATTACK1[] = "_ATTACK1";
static const char s_pose_ATTACK2[] = "_ATTACK2";
static const char s_pose_ATTACKIDLE[] = "_ATTACK-IDLE";
static const char s_pose_STRUCK1[] = "_STRUCK1";
static const char s_pose_STRUCK2[] = "_STRUCK2";
static const char s_pose_IDLE1[] = "_IDLE1";
static const char s_pose_IDLE2[] = "_IDLE2";
static const char s_pose_IDLE3[] = "_IDLE3";
static const char s_pose_IDLE4[] = "_IDLE4";
static const char s_pose_IDLE5[] = "_IDLE5";
static const char s_pose_ITEM[] = "_ITEM";
static const char s_pose_ITEM2[] = "_ITEM2";
static const char s_pose_DEATH[] = "_DEATH";
static const char s_pose_TOY1[] = "_TOY1";
static const char s_pose_TOY2[] = "_TOY2";
static const char s_pose_TOYBREAK[] = "_TOY-BREAK";

// The looked-up animation-set node: Lookup stores node->m_c (an int) into the
// out slot; the blend math reads node->m_10 (the animation length).
struct CAnimSetNode {
    char m_pad0[0xc];
    i32 m_c;  // +0x0c  the value Lookup returns into the table
    i32 m_10; // +0x10  animation length (toy-swap blend uses this)
};

#define LOAD_POSE(dst, sfx)                                                                        \
    do {                                                                                           \
        CSprite* _out = 0;                                                                         \
        m_154->m_c->m_2c->m_10map.Lookup("GRUNTZ_" + m_animSetName + (sfx), &_out);                \
        (dst) = (int)_out;                                                                         \
    } while (0)

// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// instruction MULTISET byte-identical vs retail (verified), logic/CFG/offsets exact;
// residue = retail SINKS the `out=0` store past the Lookup arg pushes + stores the
// table member before the temp-dtors where cl hoists/reorders, permuted per block
// across 22 near-identical Lookup blocks. Source-invariant. ~76%.
RVA(0x00049c60, 0x8d1)
void CGrunt::LoadAnimNameTable(i32 kind, i32 toyOnly) {
    if (kind == 0) {
        LOAD_POSE(m_394, s_pose_WALK);
        LOAD_POSE(m_398, s_pose_ATTACK1);
        LOAD_POSE(m_39c, s_pose_ATTACK2);
        LOAD_POSE(m_3a0, s_pose_ATTACKIDLE);
        LOAD_POSE(m_3a4, s_pose_STRUCK1);
        LOAD_POSE(m_3a8, s_pose_STRUCK2);
        LOAD_POSE(m_3ac[0], s_pose_IDLE1);
        LOAD_POSE(m_3ac[1], s_pose_IDLE2);
        LOAD_POSE(m_3ac[2], s_pose_IDLE3);
        LOAD_POSE(m_3b8, s_pose_IDLE4);
        LOAD_POSE(m_3bc, s_pose_IDLE5);
        LOAD_POSE(m_3d0, s_pose_ITEM);
        LOAD_POSE(m_3d4, s_pose_ITEM2);
        LOAD_POSE(m_3c0, s_pose_DEATH);
        return;
    }

    if (toyOnly != 0) {
        LOAD_POSE(m_394, s_pose_WALK);
    } else {
        LOAD_POSE(m_3c4, s_pose_TOY1);
        i32 x = ((CAnimSetNode*)m_3c4)->m_10;
        LOAD_POSE(m_3c8, s_pose_TOY2);
        i32 y = ((CAnimSetNode*)m_3c8)->m_10;
        if (x >= y) {
            m_190 = (i32)(100.0 / ((double)x / y - -1.0) - -0.5);
        } else {
            m_190 = 100 - (i32)(100.0 / ((double)y / x - -1.0) - -0.5);
        }
    }

    LOAD_POSE(m_3cc, s_pose_TOYBREAK);
}

#undef LOAD_POSE

// ---------------------------------------------------------------------------
// CGrunt::ResetEntranceAnimation(int apply, int cycle, int cue)   @0x62e10
// The shared entrance/idle-anim reset the entrance state machine (and its two
// callers BuildEntranceAnimation + LoadEntranceConfig) run to (re)select and
// arm the grunt's entrance animation. __thiscall, ret 0xc (/GX - the per-cell
// key CString temp carries a C++ EH frame).
//
//   * clears the "applied" flag (m_244 = 0), then reverse-looks-up the current
//     active-anim-set node's NAME and tests it against the idle key "A".
//   * if the grunt is NOT already idle and `cycle`==0: re-anchor the idle timer
//     (m_820.. bookkeeping) off the IdleDelay config (rand window).
//   * else dispatches on the geometry-source array m_3ac[0..2]: a single source
//     (m_3b0==0) re-arms it; the 2-arg branch (cycle!=0) randomly cycles among
//     the available sources, firing a focused-grunt on-screen "cue" (consts
//     4/5/6 by index) when `cue`!=0 and the grunt is visible.
//   * latches a fresh active-anim-set node into m_14->m_1c (saving the old into
//     m_30), and finally - when something was applied or `apply`!=0 - rebuilds
//     the per-cell entrance-position key string (CString from the m_474 cell
//     table, indexed 3*col+row by either the per-grunt triple m_43c or a preset
//     by reason m_444) and stamps the first frame.
static i32 s_entrancePreset0[3]; // DAT_00644aa0
static i32 s_entrancePreset1[3]; // DAT_00644ac0
static i32 s_entrancePreset2[3]; // DAT_00644ad0

RVA(0x00062e10, 0x47e)
void CGrunt::Stub_062e10(i32 apply, i32 cycle, i32 cue) {
    m_244 = 0;

    i32 notIdle = strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), s_animKeyA) != 0;
    i32 applied = 0;

    if (notIdle && cycle == 0) {
        // Re-anchor the idle timer to a randomized IdleDelay window.
        m_15c = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_3ac[0]);
        m_838 = 0x3a98;
        m_83c = 0;
        m_830 = (i32)g_645588;
        m_834 = 0;
        i32 n = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530) + 1;
        m_828 = GruntRand() % n + 0x7530;
        m_82c = 0;
        m_820 = (i32)g_645588;
        m_824 = 0;
        applied = 1;
    } else if (m_3ac[1] == 0) {
        // Single geometry source: re-arm it (no flag set).
        m_15c = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_3ac[0]);
    } else if (cycle == 0) {
        // Already on this source? nothing to do.
        if ((void*)m_154->m_1b4 == (void*)m_3ac[0]) {
            goto latch;
        }
        m_15c = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_3ac[0]);
        {
            i32 d = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530);
            applied = 1;
            m_828 = GruntRand() % (d - 0x4e1f) + 0x4e20;
            m_82c = 0;
            m_820 = (i32)g_645588;
            m_824 = 0;
        }
    } else {
        // Cycle among the available sources, with the focused-grunt cue.
        i32 count = (m_3ac[2] == 0) ? 1 : 2;
        i32 idx = GruntRand() % count + 1;
        if (cue != 0) {
            CGameRegistry* g = g_pGameRegistry;
            g->CuePrep();
            i32 focused = (m_1ec == g_focusedGruntSentinel);
            if (focused && idx > 0x5a) {
                if (CueVisible(g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
                    g->m_60->Cue((i32)this, 4, -1, -1, -1);
                }
            } else if (focused || m_170 != 0) {
                if (idx == 1) {
                    if (CueVisible(g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
                        g->m_60->Cue((i32)this, 5, -1, -1, -1);
                    }
                } else if (idx == 2) {
                    if (CueVisible(g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
                        g->m_60->Cue((i32)this, 6, -1, -1, -1);
                    }
                }
            }
        }
        m_15c = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_3ac[idx]);
        m_244 = 1;
        applied = 1;
    }

latch:
    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyA);

    if (!applied && apply == 0) {
        return;
    }

    // Rebuild the per-cell entrance key string + first frame. The cell is the
    // per-grunt triple {col,row,reason}, unless a non-default entrance reason
    // selects a preset triple.
    i32 col = m_43c[0];
    i32 row = m_43c[1];
    i32 reason = m_43c[2];
    if ((void*)m_154->m_1b4 != (void*)m_3ac[0]) {
        switch (reason) {
            case 2:
            case 3:
                col = s_entrancePreset0[0];
                row = s_entrancePreset0[1];
                break;
            case 4:
            case 5:
                col = s_entrancePreset1[0];
                row = s_entrancePreset1[1];
                break;
            case 6:
            case 7:
            case 8:
                col = s_entrancePreset2[0];
                row = s_entrancePreset2[1];
                break;
            default:
                break;
        }
    }

    CString key = (const char*)&m_474[(3 * col + row) * 0x68];

    CEntranceAnimDescColl* desc = m_154->m_1b4;
    i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    EntranceApplyFrame(key, elem[0x14 / 4]);
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveEntranceArrival()   @0x633e0
// The per-tick entrance-arrival resolver: once the grunt has settled on its
// destination tile it commits the "arrival" (claims the tile, seeds the
// per-grunt defender bookkeeping, clears the view-cull state on m_10) and, when
// the entrance window has elapsed + the grunt is off-screen/unfocused, runs the
// entrance reset (Stub_062e10). __thiscall, ret 0.
//
//   * if the entrance is active (m_1e4) and the grunt has not moved off its
//     last tile (m_5c==m_17c, m_60==m_180) and that tile's high occupancy bit is
//     clear, clear m_1e4.
//   * arm the entrance geometry source (m_154->m_1a0.SetGeoSourceR); gate the
//     arrival on the elapsed-time window (clock - m_830_64 >= m_838_64) and the
//     grunt being off-screen (registry m_134 != 1) + its focus slot live.
//   * the arrival commit: notify the tile manager, latch m_420, seed the
//     defender block (m_300..m_314, m_2d0/2d4/2dc/2f0/2f4, m_248 |= 0x18040402),
//     clear m_10's view-cull rect, run the arrival hook.
//   * tail: if the entrance anim is done (m_154->m_1b4 != m_3ac[0]) run
//     Stub_062e10(0,0,0) on the lookup-miss flags; else, when the idle window has
//     elapsed and the geometry source is ready, run Stub_062e10(0,1,1).
RVA(0x000633e0, 0x2ca)
void CGrunt::Stub_0633e0() {
    if (m_1e4 != 0 && m_10->m_5c == m_17c && m_10->m_60 == m_180) {
        CGameRegistry* g = g_pGameRegistry;
        CTileGrid* grid = g->m_70;
        i32 tx = m_10->m_5c >> 5;
        i32 ty = m_10->m_60 >> 5;
        i32 flags;
        if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }
        if (!(flags & 0x80)) {
            m_1e4 = 0;
        }
    }

    i32 ready = m_154->m_1a0.SetGeoSourceR(g_defaultGeo);

    if ((i64)(u32)g_645588 - *(i64*)&m_830 >= *(i64*)&m_838) {
        CGameRegistry* g = g_pGameRegistry;
        i32 mode = g->m_134;
        if (mode != 1) {
            CFocusSlot* slot = (CFocusSlot*)((char*)g + 0x150 + m_1ec * 0x238);
            if (slot != 0 && slot->m_14 != 0) {
                if (m_420 == 0 && m_464 == 0 && mode == 2 && g_focusedGruntSentinel == m_1ec
                    && m_1d8 == 0) {
                    m_260->NotifyArrival(m_1ec, m_1f0);
                    m_464 = 1;
                    goto tail;
                }
                if (mode != 2 && g_focusedGruntSentinel == m_1ec && m_1d8 == 0 && m_420 != 1) {
                    m_308 = 0;
                    m_310 = 0;
                    m_30c = 0;
                    m_314 = 0;
                    m_300 = m_17c;
                    m_304 = m_180;
                    m_420 = 1;
                    i32 kind = m_170;
                    switch (kind) {
                        case 2:
                            m_2dc = 1;
                            break;
                        default:
                            m_2dc = g_buteMgr.GetIntDef(s_Grunt, s_PlayerDefenderRadius, 3) + 1;
                            break;
                    }
                    m_2f0 = -1;
                    m_2f4 = -1;
                    m_2d0 = 4;
                    m_2d4 = 0;
                    m_230 = 0;
                    m_248 |= 0x18040402;
                    m_10->m_134 = 0;
                    m_10->m_13c = 0;
                    m_10->m_138 = 0;
                    m_10->m_140 = 0;
                    EntranceArrivalHook(0, 0);
                }
            }
        }
    }

tail:
    if ((void*)m_154->m_1b4 != (void*)m_3ac[0]) {
        if (m_154->m_1c8 == 0 && m_154->m_1c0 != 0) {
            Stub_062e10(0, 0, 0);
        }
        return;
    }
    if ((i64)(u32)g_645588 - *(i64*)&m_820 >= *(i64*)&m_828 && ready == 1) {
        Stub_062e10(0, 1, 1);
    }
}

// ---------------------------------------------------------------------------
// CGrunt::BuildEntranceAnimation(int mode)   @0x67bd0
// Selects + loads the grunt's entrance animation (the "drop in / resurrect /
// random arrival" sequence). Latches a fresh active-anim-set node into m_14->m_1c
// (saving the old into m_30), seeds the entrance bookkeeping (m_25c=1, m_1fc=0,
// m_1e4=1), marks the HUD geometry dirty (m_10->m_74 = 0xcf850; m_10->m_8 |=
// 0x20000), then picks an entrance-key string by `mode`:
//   mode==1 : a rand()%0x1e1-bucketed arrival (ENTRANCEZ_ONE / _TWO / _THREE)
//   mode==2 : ENTRANCEZ_DROP
//   else    : ENTRANCEZ_RESSURECT (key) / DEATHZ_MELT (base)
// looks that sprite-set up in the entrance player's table (m_154->m_c->m_2c map),
// fires a 6-arg on-screen "cue" when the grunt is visible/focused, copies the base
// "GRUNTZ_ENTRANCEZ"/"GRUNTZ_DEATHZ_MELT" key into a CString, and finally either
// runs the entrance reset (Stub_062e10(1,0,0)) on a lookup miss, or applies the
// resolved geometry to the player's sub-player (+0x1a0) plus the first frame.
// /GX (the CString temp carries a C++ EH frame). __thiscall, ret 4.
//
// `mode`-string table (reloc-masked .rodata literals):
static const char s_GRUNTZ_ENTRANCEZ[] = "GRUNTZ_ENTRANCEZ";
static const char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
static const char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
static const char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
static const char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
static const char s_GRUNTZ_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";

RVA(0x00067bd0, 0x2ef)
void CGrunt::BuildEntranceAnimation(i32 mode) {
    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyK);

    m_25c = 1;
    m_1fc = 0;
    m_1e4 = 1;
    if (m_10->m_74 != 0xcf850) {
        m_10->m_74 = 0xcf850;
        m_10->m_8 |= 0x20000;
    }

    EntrancePrepare(); // thunk_FUN_0044b240 (a void this-method)

    CString key;

    // The on-screen / focused-grunt gate: fire the cue when the grunt is inside
    // the visible view rect, or when it is the registry's focused grunt and its
    // m_1ec matches the focus sentinel.
    i32 onScreen = 0;
    CGameRegistry* g = g_pGameRegistry;
    {
        i32 x = m_10->m_5c;
        i32 y = m_10->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            onScreen = 1;
        } else {
            CEntranceAnimPlayer* focus = 0;
            i32* cell = (i32*)((char*)g + 0x68);
            CEntranceAnimPlayer** slot = (CEntranceAnimPlayer**)(*cell);
            if (((i32*)slot)[0x24c / 4] == 1) {
                i32* idxObj = ((i32**)slot)[0x244 / 4];
                i32* vec = (i32*)idxObj[2];
                i32 a = vec[0];
                i32 b = vec[1];
                i32 off = a * 15 + b;
                focus = ((CEntranceAnimPlayer**)slot)[off + 0x1c / 4];
            }
            if (this == (CGrunt*)focus && m_1ec == g_focusedGruntSentinel) {
                onScreen = 1;
            }
        }
    }

    CSprite* found = 0;
    const char* base;

    if (mode == 1) {
        i32 r = GruntRand() % 0x1e1;
        if (r > 0x140) {
            m_154->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_ONE, &found);
            if (onScreen) {
                g->m_60->CueA(this, 0x37a, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else if (r > 0xa0) {
            m_154->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_TWO, &found);
            if (onScreen) {
                g->m_60->CueA(this, 0x37b, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else {
            m_154->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_THREE, &found);
            if (onScreen) {
                g->m_60->CueA(this, 0x37c, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        }
    } else if (mode == 2) {
        m_154->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_DROP, &found);
        base = s_GRUNTZ_ENTRANCEZ_DROP;
    } else {
        m_154->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_RESSURECT, &found);
        base = s_GRUNTZ_DEATHZ_MELT;
    }

    key = base;

    if (!found) {
        Stub_062e10(1, 0, 0);
    } else {
        m_15c = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry((i32)found);
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        EntranceApplyFrame(key, elem[0x14 / 4]);
    }
}

// ---------------------------------------------------------------------------
// CGrunt::LoadEntranceConfig()  @0x67f80
// Commits the grunt to its newly resolved entrance position: arms the entrance
// player (m_154->m_1a0 geometry-state setter), then re-stamps the grunt's
// footprint into the global tile-occupancy grid (g_pGameRegistry->m_70): on the
// NEW tile (m_10->m_5c>>5, m_10->m_60>>5) it reads the occupying owner word and,
// if a *different* grunt holds it, fires the path sub-manager's contention notify;
// clears the OLD tile (m_17c/m_180 pixel coords, -1 = none) and stamps the NEW
// one (set occupancy bit 0x20<<24, write packed (m_1ec<<8)|m_1f0 owner), then
// posts the wire call and records the new tile pixel coords. Marks the HUD anim
// id dirty (m_10->m_74 = m_60 + 0x186a0; m_8 |= 0x20000), looks the DROP entrance
// sprite-set up in the entrance player's table, and either (set found ==
// m_154->m_1b4) fires the focused-grunt entrance cue + claims the tile + reads the
// EntranceSafeTime config + seeds the safe-time bookkeeping, or (miss) releases the
// tile and runs the on-released hook. Finally clears m_1e4, reloads the per-grunt
// tuning (ReadConfigFromButeMgr), runs the two tail stubs, and when the entrance
// sub-player is armed-but-not-running (m_28!=0 && m_20==0) runs the entrance
// reset (Stub_062e10(1,0,0)). __thiscall, ret 0.
//
// ~78.8% fuzzy: CFG, every member offset, all constants, the tile-grid index math
// (cell stride 0x1c, occupancy bit 0x20<<24, packed owner word), the config read,
// and all six call shapes are byte-exact. Residue = the callee-saved zero-register
// coin-flip (the original pins `0` in ebx at the if-body head and re-reads
// grid->m_c from memory under the resulting register pressure; our build keeps the
// zero deferred and caches the width in a free reg, cascading a 1-instruction phase
// shift through the two grid-write blocks). Same entropy class as the 5 resolvers'
// edx/ecx coin-flip; no source lever flips it (an explicit `int z=0;` did not pin).
RVA(0x00067f80, 0x313)
void CGrunt::LoadEntranceConfig() {
    if (m_154->m_1a0.SetGeoSourceR(g_defaultGeo) == 1) {
        CGameRegistry* g = g_pGameRegistry;
        CGruntHud* h = m_10;
        CTileGrid* grid = g->m_70;
        i32 tx = h->m_5c >> 5;
        i32 ty = h->m_60 >> 5;

        i32 flags;
        if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }

        if (flags & 0x20000000) {
            i32 owner;
            if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
                owner = -1;
            } else {
                owner = ((i32*)grid->m_8[ty])[tx * 7 + 1];
            }
            i32 b = (owner >> 8) & 0xff;
            i32 a = owner & 0xff;
            if (m_1ec != b || m_1f0 != a) {
                m_260->SetTile(b, a, 2, m_1ec);
            }
        }

        // Re-stamp the occupancy grid: clear old tile, set new tile.
        h = m_10;
        i32 oldX = m_17c;
        m_25c = 0;
        i32 newPxX = h->m_5c;
        i32 newPxY = h->m_60;
        i32 oldTileX = oldX >> 5;
        i32 oldTileY = m_180 >> 5;
        i32 newTileX = newPxX >> 5;
        i32 newTileY = newPxY >> 5;

        if (oldX != -1 && m_180 != -1) {
            CTileGrid* og = g_pGameRegistry->m_70;
            ((char*)&og->m_8[oldTileY][oldTileX * 7])[3] &= ~0x20;
            og->m_8[oldTileY][oldTileX * 7 + 1] = -1;
        }
        {
            CTileGrid* ng = g_pGameRegistry->m_70;
            ((char*)&ng->m_8[newTileY][newTileX * 7])[3] |= 0x20;
            ng->m_8[newTileY][newTileX * 7 + 1] = (m_1ec << 8) | m_1f0;
        }
        m_17c = newPxX;
        m_180 = newPxY;
        m_260->PostWire();

        h = m_10;
        m_1fc = 1;
        if (h->m_74 != h->m_60 + 0x186a0) {
            h->m_74 = h->m_60 + 0x186a0;
            h->m_8 |= 0x20000;
        }

        CEntranceAnimPlayer* p = m_154;
        CSprite* found = 0;
        void* cached = p->m_1b4;
        p->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_DROP, &found);
        if ((void*)found == cached) {
            if (m_1ec == g_focusedGruntSentinel) {
                g_pGameRegistry->m_60->CueA(this, 0x33f, -1, 0, -1, -1);
            }
            m_260->ClaimTile(m_1ec, m_1f0, 0, 0);
            m_364 = 1;
            m_848 = g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 5000);
            m_84c = 0;
            m_840 = g_645588;
            m_844 = 0;
            m_858 = 0;
            m_85c = 0;
        } else {
            if (m_260->ReleaseTile(m_1ec, m_1f0)) {
                EntranceOnReleased();
            }
        }
        m_1e4 = 0;
        ReadConfigFromButeMgr();
        Stub_048470(0, 0);
        EntranceFinishWire(0, 0);
    }

    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return;
    }
    Stub_062e10(1, 0, 0);
}

// ---------------------------------------------------------------------------
// CGrunt::ReadConfigFromButeMgr
// Reads the TimePerTile tuning config from CButeMgr for this grunt's type.
// Applies a special-case halving for grunt kind 55 (0x37).
// ---------------------------------------------------------------------------
RVA(0x00048400, 0x47)
void CGrunt::ReadConfigFromButeMgr() {
    *(i32*)((char*)this + 0x18c) = 0;
    *(i32*)((char*)this + 0x418) = 0;

    *(i32*)((char*)this + 0x41c) =
        g_buteMgr.GetDwordDef(*(char**)((char*)this + 0x1c0), s_TimePerTile, 1000);

    if (*(i32*)((char*)this + 0x258) == 0x37) {
        *(i32*)((char*)this + 0x41c) >>= 1;
    }
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0006a060, 0x43d)
void CGrunt::LoadGruntMovingDeathConfig() {}

// ===========================================================================
// Migrated CGrunt cluster (formerly the CUserLogic_* stubs in
// src/Stub/Discovered.cpp). A prior matcher proved this whole block is CGrunt:
// the dtor @0xf2f0 stamps vtable 0x5e8754 over CUserLogic/CUserBase bases, and
// the anim loader @0x49c60 builds "GRUNTZ_<type>_<POSE>" keys. Reconstructed in
// ascending retail-RVA order. Raw-offset member access (the campaign style used
// by ReadConfigFromButeMgr above) keeps the giant ~0x46c layout tractable.
// ===========================================================================

// The global free-list pool the name caches recycle into (head @0x645544, base
// subtrahend @0x64554c). Defined TU-local (reloc-masked); shared in retail.
void** g_freePoolHead; // DAT_00645544
i32 g_freePoolBase;    // DAT_0064554c (raw subtrahend)
i32 g_serialCounter;   // DAT_00629ad0 (Save's per-record counter)

// CGrunt::IsSameType(a, b) @0x3c7f0 - a free __cdecl comparator returning
// whether two grunts share the same type record (their +0x8 sub-object ptr).
RVA(0x0003c7f0, 0x18)
i32 CGrunt_IsSameType(CGrunt* a, CGrunt* b) {
    return *(void**)((char*)a + 8) == *(void**)((char*)b + 8);
}

// CGrunt::ClearAllSprites() @0x4b240 - on death/teardown, flag each live HUD
// sprite record (+0x8 |= 0x10000) and null its slot. The stamina/toy-time/
// wingz-time trio is gated on m_1fc==0 (entrance not yet committed). Clears the
// arrival gate m_1d8 last.
RVA(0x0004b240, 0xaa)
void CGrunt::ClearAllSprites() {
    if (m_selectedSprite) {
        ((CSpriteRegRecord*)m_selectedSprite)->m_8 |= 0x10000;
        m_selectedSprite = 0;
    }
    if (m_healthSprite) {
        ((CSpriteRegRecord*)m_healthSprite)->m_8 |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_toySprite) {
        ((CSpriteRegRecord*)m_toySprite)->m_8 |= 0x10000;
        m_toySprite = 0;
    }
    if (m_1fc == 0) {
        if (m_staminaSprite) {
            ((CSpriteRegRecord*)m_staminaSprite)->m_8 |= 0x10000;
            m_staminaSprite = 0;
        }
        if (m_toyTimeSprite) {
            ((CSpriteRegRecord*)m_toyTimeSprite)->m_8 |= 0x10000;
            m_toyTimeSprite = 0;
        }
        if (m_wingzTimeSprite) {
            ((CSpriteRegRecord*)m_wingzTimeSprite)->m_8 |= 0x10000;
            m_wingzTimeSprite = 0;
        }
    }
    m_1d8 = 0;
}

// CGrunt::CanShowStamina() @0x514a0 - the stamina-bar visibility gate: shown
// only if not powered-up (m_218==0), stamina below full (m_3f0 < 0x64), and not
// mid-entrance (m_1e4==0).
RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (*(i32*)((char*)this + 0x218) == 0 && m_3f0 >= 0x64 && m_1e4 == 0) {
        return 1;
    }
    return 0;
}

// CGrunt::ClearSubA() @0x57c10 - destroy the optional sub-object at +0x424.
RVA(0x00057c10, 0x1e)
void CGrunt::ClearSubA() {
    CGruntSub* p = *(CGruntSub**)((char*)this + 0x424);
    if (p) {
        p->Free();
        *(CGruntSub**)((char*)this + 0x424) = 0;
    }
}

// CGrunt::ClearSubB() @0x57ce0 - destroy the optional sub-object at +0x428.
RVA(0x00057ce0, 0x1e)
void CGrunt::ClearSubB() {
    CGruntSub* p = *(CGruntSub**)((char*)this + 0x428);
    if (p) {
        p->Free();
        *(CGruntSub**)((char*)this + 0x428) = 0;
    }
}

// CGrunt::DestroyAnims() @0x57d80 - the two-step anim teardown (both this-calls
// reach engine cleanup; reloc-masked).
RVA(0x00057d80, 0x11)
void CGrunt::DestroyAnims() {
    AnimTeardownA();
    AnimTeardownB();
}

// CGrunt::DispatchVtbl24() @0x6b260 - a one-instruction virtual tail-call thunk
// (mov eax,[ecx]; jmp [eax+0x24] = vtable slot 9). Modeled by reinterpreting
// this as a 10-virtual interface and calling its 10th slot in tail position.
RVA(0x0006b260, 0x5)
void CGrunt::DispatchVtbl24() {
    ((CVtblSlot9*)this)->Slot9();
}

// @early-stop
// reloc-masked-symbol plateau: instruction stream byte-exact vs retail (verified
// llvm-objdump), but the two free-pool globals (g_freePoolHead/Base) and the
// three engine calls (Coll::Reset, List::RemoveHead, node deleter) are unnamed,
// so their DIR32/REL32 operands pair to differently named retail symbols and
// score fuzzy. Naming the whole referent set is a final-sweep task.
// CGrunt::FreeNameList() @0x48360 - tears down the per-grunt name/animation
// caches: walks a small list at +0x320 returning each node's +0x8 buffer to a
// global free pool (head/base at 0x645544/0x64554c), empties the collection at
// +0x31c, then drains the name CObList at +0x338 (count = m_33c->m_8; each node
// freed via the engine deleter).
RVA(0x00048360, 0x7e)
void CGrunt::FreeNameList() {
    if (*(i32*)((char*)this + 0x328) != 0) {
        void** node = *(void***)((char*)this + 0x320);
        if (node) {
            do {
                void* next = node[0];
                void* buf = node[2];
                if (buf) {
                    void** slot = (void**)((char*)buf - g_freePoolBase);
                    *slot = g_freePoolHead;
                    g_freePoolHead = slot;
                }
                node = (void**)next;
            } while (node);
        }
        ((CGruntColl*)((char*)this + 0x31c))->Reset();
    }

    while (1) {
        void* list = *(void**)((char*)this + 0x344);
        i32 count = list ? *(i32*)((char*)(*(void**)((char*)this + 0x33c)) + 8) : 0;
        if (count == 0) {
            return;
        }
        if (list == 0) {
            continue;
        }
        void* p = ((CGruntList*)((char*)this + 0x338))->RemoveHead();
        GruntNode_Delete(p);
    }
}

// @early-stop
// FP instruction-scheduling wall: logic + offsets exact, but MSVC's x87 stack
// scheduling for the magnitude/normalize rarely matches from C source (an x87
// fxch/fsubp ordering the compiler picks); see docs/patterns (FP scheduling).
// CGrunt::ComputeFacing(double dt) @0x57060 - sets the grunt's facing/velocity:
//   m_400 = (sqrt(dx*dx + dy*dy) / m_41c) * dt   (dx=m_17c-m_5c, dy=m_180-m_60)
//   m_408 = (double)m_10->m_5c;  m_410 = (double)m_10->m_60
// dt is the per-tile time step; m_41c is the configured TimePerTile.
RVA(0x00057060, 0x6f)
void CGrunt::ComputeFacing(double dt) {
    CGruntHud* h = m_10;
    double dx = (double)m_17c - (double)h->m_5c;
    double dy = (double)m_180 - (double)h->m_60;
    *(double*)((char*)this + 0x400) =
        (sqrt(dx * dx + dy * dy) / (double)*(i32*)((char*)this + 0x41c)) * dt;
    *(double*)((char*)this + 0x408) = (double)h->m_5c;
    *(double*)((char*)this + 0x410) = (double)h->m_60;
}

// @early-stop
// reloc-masked-extern plateau: logic + CFG + every member store byte-exact, but
// the 8 engine this-calls (NotifyArrival, ArrivalClaim, 6 ArrivalHooks) are
// unnamed externals, so their `call rel32` displacements pair to differently
// named retail thunks and score fuzzy. Resolving each thunk to its real fn is a
// final-sweep task (the whole referent set must be named for exact).
// CGrunt::CommitArrival() @0x4b130 - finalizes the grunt's arrival on its tile.
// If already arrived (m_1d8) returns 1 immediately. Otherwise, if not yet
// claimed (m_420==0): in alt-mode (registry m_134==2) it just notifies the tile
// owner; else it seeds the arrival defender block (m_308/m_310/.., m_420, m_2d0,
// m_248 &= mask) and claims the tile. Then runs the six per-arrival hooks and
// latches m_1d8=1.
RVA(0x0004b130, 0xc8)
i32 CGrunt::CommitArrival() {
    if (m_1d8 != 0) {
        return 1;
    }
    if (m_420 != 0) {
        if (g_pGameRegistry->m_134 == 2) {
            m_260->NotifyArrival(m_1ec, m_1f0);
        } else if (m_420 != 0) {
            m_308 = 0;
            m_310 = 0;
            m_30c = 0;
            m_314 = 0;
            i32 flags = m_248 & 0xe7fbfbfd;
            m_420 = 0;
            m_2d0 = 0;
            m_248 = flags;
            ArrivalClaim(1, 1);
        }
    }
    ArrivalHook0();
    ArrivalHook1();
    ArrivalHook2();
    ArrivalHook3();
    ArrivalHook4();
    ArrivalHook5();
    m_1d8 = 1;
    return 1;
}

// @early-stop
// shuttle-register regalloc wall: logic exact; the target threads the four
// passthrough args (c..f) through one saved esi (push esi; mov esi,[..]; push
// esi x4) while MSVC here pre-loads them into eax/ecx/edx. Pure arg-marshalling
// schedule coin-flip; no source lever flips it (entropy-class).
// CGrunt::TileSwitch(a, b, c, d, e, f) @0x4b320 - a __stdcall passthrough that
// scales the first two grid coords to tile-pixel centers (*0x20 + 0x10) and
// forwards all six args to the engine tile-switch helper.
RVA(0x0004b320, 0x34)
void __stdcall CGrunt_TileSwitch(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    GruntTileSwitchImpl(a * 0x20 + 0x10, b * 0x20 + 0x10, c, d, e, f);
}

// @early-stop
// reloc-masked-symbol plateau: instruction stream byte-exact vs retail (verified
// llvm-objdump), residual is the two unnamed free-pool globals (g_freePoolHead/
// Base) + the Coll::Reset call pairing to differently named retail symbols.
// CGrunt::SetEntrancePos(a, b) @0x4d060 - records the grunt's current tile as
// its committed entrance position (m_174/m_178 = m_17c/m_180), clears the
// arrival timers (m_210); if `a`, also clears m_450/m_230; and if `b` and the
// grunt is not a special kind (m_2d0!=0x11) it drains the name list at +0x320
// into the global free pool and resets the collection at +0x31c.
RVA(0x0004d060, 0x98)
void CGrunt::SetEntrancePos(i32 a, i32 b) {
    *(i32*)((char*)this + 0x174) = m_17c;
    *(i32*)((char*)this + 0x178) = m_180;
    *(i32*)((char*)this + 0x210) = 0;
    if (a) {
        *(i32*)((char*)this + 0x450) = 0;
        m_230 = 0;
    }
    if (b && m_2d0 != 0x11 && *(i32*)((char*)this + 0x328) != 0) {
        void** node = *(void***)((char*)this + 0x320);
        if (node) {
            do {
                void* next = node[0];
                void* buf = node[2];
                if (buf) {
                    void** slot = (void**)((char*)buf - g_freePoolBase);
                    *slot = g_freePoolHead;
                    g_freePoolHead = slot;
                }
                node = (void**)next;
            } while (node);
        }
        ((CGruntColl*)((char*)this + 0x31c))->Reset();
    }
}

// @early-stop
// reloc-masked-extern tail (94%+): the 4560-byte instruction stream is
// byte-exact vs retail (no EH frame, same sprite/string/name-id/field/tail
// blocks, verified llvm-objdump) - the residual is the ~35 unnamed call operands
// in the 18 name-id + 3 string blocks (catalog LookupName + ~CString) pairing to
// differently named retail symbols. Naming the whole referent set -> exact is a
// final-sweep task.
// ---------------------------------------------------------------------------
// CGrunt::Save(ar) @0x53f90 - serializes the whole grunt state into a custom
// archive (each member -> ar->Write(&field, size) via vtable slot 0x30). Bails
// (return 0) if the archive is null or the type catalog (m_158->m_c) is unset.
// The 4560-byte body is, in order: 7 sprite-id blocks (each bumps the global
// serialize counter and writes the sprite's m_188, or 0 if the slot is empty);
// 3 name strings (a 0x80-byte buffer copy); 18 anim-name-id blocks (look the id
// up in the catalog's name map and copy the resolved name into the buffer); then
// ~100 plain field writes; finally a linked-list tail (m_33c) writing each
// node's +0x8 (size 0x2c). The serialize counter is the global DAT_00629ad0.
RVA(0x00053f90, 0x11d0)
i32 CGrunt::Save(CGruntArchive* ar) {
    if (!ar) {
        return 0;
    }
    CGruntNameMap* catalog = ((CGruntTypeCatalog*)*(void**)((char*)this + 0x158))->m_c;
    if (!catalog) {
        return 0;
    }
    i32 tmp;
    char buf[0x80];
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1b8);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1bc);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1c4);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1c8);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1cc);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1d0);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CHudSprite* sp = *(CHudSprite**)((char*)this + 0x1d4);
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, *(const char**)((char*)this + 0x1c0));
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, *(const char**)((char*)this + 0x448));
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, *(const char**)((char*)this + 0x44c));
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x394);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x398);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x39c);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3a0);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3a4);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3a8);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3ac);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3b0);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3b4);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3b8);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3bc);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3c0);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3c4);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3c8);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3cc);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3d0);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3d4);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = *(i32*)((char*)this + 0x3d8);
        if (id) {
            CString nm = catalog->LookupName(id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    ar->Write((char*)this + 0x18c, 4);
    ar->Write((char*)this + 0x190, 4);
    ar->Write((char*)this + 0x194, 4);
    ar->Write((char*)this + 0x170, 4);
    ar->Write((char*)this + 0x198, 4);
    ar->Write((char*)this + 0x19c, 4);
    ar->Write((char*)this + 0x1a0, 4);
    ar->Write((char*)this + 0x1a4, 4);
    ar->Write((char*)this + 0x1a8, 4);
    ar->Write((char*)this + 0x1ac, 4);
    ar->Write((char*)this + 0x1b0, 4);
    ar->Write((char*)this + 0x1b4, 4);
    ar->Write((char*)this + 0x1d8, 4);
    ar->Write((char*)this + 0x174, 8);
    ar->Write((char*)this + 0x17c, 8);
    ar->Write((char*)this + 0x184, 8);
    ar->Write((char*)this + 0x1dc, 8);
    ar->Write((char*)this + 0x1e4, 4);
    ar->Write((char*)this + 0x1e8, 4);
    ar->Write((char*)this + 0x1ec, 4);
    ar->Write((char*)this + 0x1f0, 4);
    ar->Write((char*)this + 0x1f4, 4);
    ar->Write((char*)this + 0x1f8, 4);
    ar->Write((char*)this + 0x1fc, 4);
    ar->Write((char*)this + 0x200, 8);
    ar->Write((char*)this + 0x208, 8);
    ar->Write((char*)this + 0x210, 4);
    ar->Write((char*)this + 0x214, 4);
    ar->Write((char*)this + 0x218, 4);
    ar->Write((char*)this + 0x21c, 4);
    ar->Write((char*)this + 0x220, 4);
    ar->Write((char*)this + 0x224, 4);
    ar->Write((char*)this + 0x228, 4);
    ar->Write((char*)this + 0x22c, 4);
    ar->Write((char*)this + 0x230, 4);
    ar->Write((char*)this + 0x290, 16);
    ar->Write((char*)this + 0x2a0, 16);
    ar->Write((char*)this + 0x2b0, 16);
    ar->Write((char*)this + 0x2c0, 16);
    ar->Write((char*)this + 0x3ec, 4);
    ar->Write((char*)this + 0x3f0, 4);
    ar->Write((char*)this + 0x3f4, 4);
    ar->Write((char*)this + 0x3f8, 4);
    ar->Write((char*)this + 0x400, 8);
    ar->Write((char*)this + 0x418, 4);
    ar->Write((char*)this + 0x42c, 4);
    ar->Write((char*)this + 0x430, 4);
    ar->Write((char*)this + 0x434, 4);
    ar->Write((char*)this + 0x438, 4);
    ar->Write((char*)this + 0x2d0, 4);
    ar->Write((char*)this + 0x2d4, 4);
    ar->Write((char*)this + 0x2d8, 4);
    ar->Write((char*)this + 0x2dc, 4);
    ar->Write((char*)this + 0x2e0, 4);
    ar->Write((char*)this + 0x2e4, 4);
    ar->Write((char*)this + 0x2ec, 4);
    ar->Write((char*)this + 0x2f0, 8);
    ar->Write((char*)this + 0x300, 8);
    ar->Write((char*)this + 0x354, 4);
    ar->Write((char*)this + 0x358, 4);
    ar->Write((char*)this + 0x35c, 4);
    ar->Write((char*)this + 0x3dc, 8);
    ar->Write((char*)this + 0x3e4, 8);
    ar->Write((char*)this + 0x450, 4);
    ar->Write((char*)this + 0x41c, 4);
    ar->Write((char*)this + 0x408, 8);
    ar->Write((char*)this + 0x410, 8);
    ar->Write((char*)this + 0x8d0, 4);
    ar->Write((char*)this + 0x234, 4);
    ar->Write((char*)this + 0x238, 4);
    ar->Write((char*)this + 0x23c, 4);
    ar->Write((char*)this + 0x240, 4);
    ar->Write((char*)this + 0x244, 4);
    ar->Write((char*)this + 0x248, 4);
    ar->Write((char*)this + 0x24c, 4);
    ar->Write((char*)this + 0x258, 4);
    ar->Write((char*)this + 0x25c, 4);
    ar->Write((char*)this + 0x360, 4);
    ar->Write((char*)this + 0x364, 4);
    ar->Write((char*)this + 0x318, 4);
    ar->Write((char*)this + 0x2f8, 8);
    ar->Write((char*)this + 0x36c, 4);
    ar->Write((char*)this + 0x454, 4);
    ar->Write((char*)this + 0x370, 4);
    ar->Write((char*)this + 0x420, 4);
    ar->Write((char*)this + 0x368, 4);
    ar->Write((char*)this + 0x458, 8);
    ar->Write((char*)this + 0x250, 4);
    ar->Write((char*)this + 0x254, 4);
    ar->Write((char*)this + 0x374, 4);
    ar->Write((char*)this + 0x37c, 4);
    ar->Write((char*)this + 0x380, 4);
    ar->Write((char*)this + 0x384, 4);
    ar->Write((char*)this + 0x388, 4);
    ar->Write((char*)this + 0x390, 4);
    ar->Write((char*)this + 0x378, 4);
    ar->Write((char*)this + 0x38c, 4);
    ar->Write((char*)this + 0x460, 4);
    ar->Write((char*)this + 0x2e8, 4);
    ar->Write((char*)this + 0x288, 8);

    for (CGruntListNode* node = *(CGruntListNode**)((char*)this + 0x33c); node;
         node = node->m_next) {
        ar->Write(node->m_data, 0x2c);
    }
    return 1;
}

// @early-stop
// reloc-masked-extern plateau: instruction stream byte-exact (verified
// llvm-objdump - prologue, geometry call, desc/frame read, cell-index math
// `0x468 + (3*col+row)*0x68`, GetName/SetAnimFrame, anim-set latch all match),
// residual is the 4 unnamed engine calls (SetGeometry/GetName/SetAnimFrame/
// LookupAnimSet) pairing to differently named retail symbols.
// ---------------------------------------------------------------------------
// CGrunt::ResetGeometry() @0x616e0 - re-arms the entrance player's geometry to
// the m_3a0 source, re-stamps the active anim-set node, and re-applies the
// per-cell entrance frame. Reads the active-anim descriptor's first element's
// frame (+0x14), looks the per-cell name up by the m_43c {col,row} triple (cell
// stride 0x68 at +0x468), applies the frame, then latches a fresh idle anim-set
// node (g_entranceAnimSrc.LookupAnimSet) into m_14->m_1c. __thiscall, ret 0.
RVA(0x000616e0, 0xa8)
i32 CGrunt::ResetGeometry() {
    m_15c = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(*(i32*)((char*)this + 0x3a0));

    CEntranceAnimDescColl* desc = m_154->m_1b4;
    i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = elem[0x14 / 4];

    i32 col = m_43c[0];
    i32 row = m_43c[1];
    i32 index = 3 * col + row;
    const char* name = ((CGruntCell*)((char*)this + 0x468 + index * 0x68))->GetName(0);
    m_154->SetAnimFrame(name, frame);

    m_30 = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyA);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::FindGridNeighbor(int validate)   @0x5b6f0
// Resolves the grunt's stored grid-cell neighbour (the CGrunt* at
// m_260's 15-wide cell grid indexed by the latched coords m_200/m_204), validates
// it occupies the expected tile, runs the tile-rect predicate + commit, and
// returns it. __thiscall, ret 4. Frameless. On any miss it clears m_21c and
// returns 0; a validate-mismatch returns 0 WITHOUT touching m_21c.
RVA(0x0005b6f0, 0xb5)
CGrunt* CGrunt::FindGridNeighbor(i32 validate) {
    if (m_200 == -1) {
        return 0;
    }
    if (m_204 == -1) {
        return 0;
    }

    CGrunt* n = *(CGrunt**)((char*)m_260 + (m_204 + 15 * m_200) * 4 + 0x1c);
    if (n != 0 && n->m_1fc != 0) {
        if (validate != 0) {
            if (n->m_10->m_5c != n->m_17c) {
                return 0;
            }
            if (n->m_10->m_60 != n->m_180) {
                return 0;
            }
        }
        if (RectContains(n->m_10->m_5c, n->m_10->m_60)) {
            CommitNeighbor(m_200, m_204, n->m_10->m_5c, n->m_10->m_60);
            return n;
        }
    }

    m_21c = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::UpdateGruntStatus()   @0x617c0
// The per-frame grunt status step. If not powered-up (m_220==0) it just runs the
// entrance reset and bails. Otherwise it re-arms the entrance geometry source and,
// gated on the grunt's stamina (m_3f0): when full (>=100) it resolves + commits its
// grid-cell neighbour (same 15-wide m_260 grid as FindGridNeighbor); when low
// (0x33..0x63, latched once via m_460) and on-screen, it fires a spawn cue.
// __thiscall, ret 0, frameless.
// @early-stop
// lazy callee-saved-reg save: instruction MULTISET byte-identical vs retail
// (verified), logic/CFG/offsets exact; residue = retail defers `push edi` until
// AFTER the m_220==0 early-bail (the cold path uses only esi) where cl saves edi
// in the prolog. Pure regalloc placement, not steerable from source. ~94.5%.
RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_220 == 0) {
        Stub_062e10(1, 0, 0);
        return 0;
    }

    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);

    if (m_3f0 >= 0x64) {
        if (m_21c == 0) {
            return 0;
        }
        m_21c = 0;
        CGrunt* n = *(CGrunt**)((char*)m_260 + (m_204 + 15 * m_200) * 4 + 0x1c);
        if (n == 0 || n->m_1fc == 0) {
            return 0;
        }
        if (RectContains(n->m_10->m_5c, n->m_10->m_60)) {
            CommitNeighbor(m_200, m_204, n->m_10->m_5c, n->m_10->m_60);
        }
        return 0;
    }

    if (m_3f0 <= 0x32) {
        return 0;
    }
    if (m_460 != 0) {
        return 0;
    }

    CGameRegistry* g = g_pGameRegistry;
    i32 x = m_10->m_5c;
    i32 y = m_10->m_60;
    i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
    if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
        g->m_60->CueSpawn(this, 2, -1, -1, -1);
    }
    m_460 = 1;
    return 0;
}
