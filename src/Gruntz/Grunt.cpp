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
// edx/ecx coin-flip on the `m_activeAnimDesc = m_38->m_1b4` store that no source lever flips,
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
//     arg c (m_health/m_stamina/m_toyTime/m_wingzTime) vs arg b (m_tileOwnerLo): the push ORDER and VALUES
//     are byte-identical, only the temp register-field differs (a pure allocator
//     coin-flip; explicit `int c=..; int b=..;` temps did NOT flip it).
//   * 2-arg Add (Toy/Selected): with ONLY two args MSVC hoists arg a (m_tileOwnerHi) into
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
#include <stdlib.h>
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
// (Moving m_movingGeoSrc, Death m_deathGeoSrc, generic m_joyGeoSrc, Idle m_idleGeoSrc[idx], Battlecry m_battlecryGeoSrc[idx]);
// Idle/Battlecry pick idx via the engine LCG rand (Idle %3+1, Battlecry %3).
static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s__LOSEITEM[] = "_LOSEITEM";
static const char s_SingleAnimation[] = "SingleAnimation";
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
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

// The death/freeze finalize key string (reloc-masked .rodata, 0x60e0f0).
static const char s_GRUNTZ_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";

// LoadFreezeSpellAssets (@0x69d60) finalize keys + bute lookup tag/key (reloc-masked).
static const char s_GRUNTZ_DEATHZ_SPARKLE[] = "GRUNTZ_DEATHZ_SPARKLE";  // 0x60ee48
static const char s_GRUNTZ_DEATHZ_UNFREEZE[] = "GRUNTZ_DEATHZ_UNFREEZE"; // 0x60ee1c
static char s_Spellz[] = "Spellz";                                       // 0x60cca8
static char s_FreezeDelay[] = "FreezeDelay";                             // 0x60ee38

// StartBombGruntRun (@0x68520) bute tag/key (reloc-masked).
static char s_BOMBGRUNT[] = "BOMBGRUNT";                   // 0x60dbd0
static char s_RunningTimePerTile[] = "RunningTimePerTile"; // 0x60e264

// A global enable flag the neighbor-combat gate reads when the candidate IS self
// (DAT_006455b0, reloc-masked).
i32 g_6455b0;

// The single-char anim-set keys the entrance reads/looks-up (reloc-masked
// .rodata; DAT_0060a454 = "A" = the idle anim key, DAT_0060d7f8 = "K" =
// BuildEntranceAnimation's latch key).
static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";

// The global running game clock (DAT_00645588) snapshotted into m_entranceClockLo.
extern "C" u32 g_645588;

// The global default geometry source the entrance geometry-state setter consumes
// (g_defaultGeo @0x6bf3bc; defined in SpriteResource.cpp, reloc-masked here).
extern i32 g_defaultGeo;

// The scratch CString teardown the GetNameRecords reject paths run (defined with the
// dispatch-machine cluster below); forward-declared for the two entrance-step
// methods (StepEntranceReinit / RunEntranceMove) defined earlier in RVA order.
static void GruntScratchTeardown();

// ---------------------------------------------------------------------------
// CGrunt::GetTilePos(out)  @0x31c70 - write the grunt's current tile coordinates
// (its HUD pixel position m_10->m_5c/m_60 each >> 5) into the caller's {x,y} out
// slot, and return that pointer. __thiscall, ret 4.
//
// @early-stop
// leaf-accessor regalloc/schedule coin-flip (the tiny-accessor family of
// docs/patterns/pin-local-for-callee-saved-reg.md): instruction multiset is
// byte-identical (same 10 ops, same mem operands, same `sar 5`), logic/offsets
// exact. Residue = retail pins `out` in edx (so a trailing `mov eax,edx`
// materializes the return) and field-loads m_5c before m_60; cl keeps `out` in
// eax (no closing mov) and loads m_60 before m_5c. Source-invariant on a 29-byte
// leaf: direct-store / explicit-temps / aliased-ptr all normalize to one of the
// two valid schedules; no lever flips the register choice. ~85%.
RVA(0x00031c70, 0x1d)
GruntTilePos* CGrunt::GetTilePos(GruntTilePos* out) {
    CGruntHud* h = m_10;
    i32 x = h->m_5c >> 5;
    i32 y = h->m_60 >> 5;
    out->m_x = x;
    out->m_y = y;
    return out;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveMovingAnimation()
// Gate: m_animResolved == 0 (else return 0). Feed key "GRUNTZ_<type>_MOVING" + geometry
// m_movingGeoSrc into the player; look up tree key "B"; then randomize the move-start time
// (m_moveStartTime = (rand()%0x5dc1 + 0x1770)*10) and seed m_moveSeed/m_moveTimeHi/m_moveSeedHi.
RVA(0x00045100, 0x112)
i32 CGrunt::ResolveMovingAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__MOVING);

    m_activeAnimDesc = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_movingGeoSrc);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyB);

    m_moveStartTime = (GruntRand() % 0x5dc1 + 0x1770) * 10;
    m_moveSeedHi = 0;
    m_moveSeed = g_movingSeed;
    m_moveTimeHi = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveDeathAnimation()
// Gate: m_animResolved == 0 (else return 0); then latch m_animResolved = 1. Fire the on-screen cue
// (arg2 = m_deathCueArg), feed geometry m_deathGeoSrc then key "GRUNTZ_<type>_DEATH", look up "C".
RVA(0x000455f0, 0x15b)
i32 CGrunt::ResolveDeathAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }
    m_animResolved = 1;

    CGameRegistry* g = g_pGameRegistry;
    if (g->m_134 == 1) {
        CGruntHud* h = m_10;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        if (x < g->m_144 && x >= g->m_13c && y < g->m_148 && y >= g->m_140) {
            g->m_60->Cue(h->m_188, m_deathCueArg, -1, -1, -1);
        }
    } else {
        g->m_60->Cue(m_10->m_188, m_deathCueArg, -1, -1, -1);
    }

    m_activeAnimDesc = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_deathGeoSrc);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__DEATH);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyC);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveAnimation()  (generic "_JOY")
// Gate: m_animResolved == 0 (else return 0). The cue arg2 is a fixed constant (0x435 when
// on-screen / 0x43f otherwise). Geometry m_joyGeoSrc; key "GRUNTZ_<type>_JOY"; look "E".
RVA(0x000457b0, 0x14c)
i32 CGrunt::ResolveAnimation() {
    if (m_animResolved != 0) {
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

    m_activeAnimDesc = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_joyGeoSrc);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__JOY);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyE);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveIdleAnimation()
// Gate: m_animResolved == 0 (else return 0). Pick idx = rand()%3 + 1 (1..3); cue arg2 =
// idx+0x431 / idx+0x43b; geometry m_idleGeoSrc[idx]; then read the active-anim
// descriptor's first element's m_14 as a 2nd lookup arg (SetAnimEx); key
// "GRUNTZ_<type>_IDLE"; look up "A".
RVA(0x00045960, 0x181)
i32 CGrunt::ResolveIdleAnimation() {
    if (m_animResolved != 0) {
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

    m_activeAnimDesc = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_idleGeoSrc[idx]);

    CAnimDescColl* desc = m_38->m_1b4;
    CAnimElem* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = elem->m_14;

    m_38->SetAnimEx(s_GRUNTZ_ + m_typeName + s__IDLE, frame);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyA);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveBattlecryAnimation()
// Gate: m_animResolved == 0 (else return 0). Pick idx = rand()%3 (0..2); cue arg2 =
// idx+0x42e / idx+0x438; geometry m_battlecryGeoSrc[idx]; key "GRUNTZ_<type>_BATTLECRY";
// look up "F".
RVA(0x00045b60, 0x161)
i32 CGrunt::ResolveBattlecryAnimation() {
    if (m_animResolved != 0) {
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

    m_activeAnimDesc = (i32)m_38->m_1b4;
    m_38->m_1a0.SetGeometry(m_battlecryGeoSrc[idx]);

    m_38->SetAnim(s_GRUNTZ_ + m_typeName + s__BATTLECRY);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_animLookupTree.Find(s_keyF);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateHealthSprite()
// Gate: m_healthSprite unset AND m_health > 0. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddA(m_tileOwnerHi, m_tileOwnerLo, m_health).
RVA(0x0004d130, 0xb5)
i32 CGrunt::CreateHealthSprite() {
    if (m_healthSprite || m_health <= 0) {
        return 0;
    }

    m_healthSprite =
        g_pGameRegistry->m_30->m_8
            ->CreateSprite(0, m_10->m_5c, m_10->m_60 - 0x19, 0xdbba0, s_GruntHealthSprite, 0x40003);
    m_healthSprite->m_7c->m_init(m_healthSprite);

    CSpriteInner* inner = m_healthSprite->m_7c;
    CSpriteRegistrar* reg = inner->m_18;
    if (!reg->AddA(m_tileOwnerHi, m_tileOwnerLo, m_health)) {
        reg->m_38->m_8 |= 0x10000;
        m_healthSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateToySprite()
// Gate: m_toySprite unset. geoB = m_60 - 0x19; hint 0xdbba0.
// Register via AddB(m_tileOwnerHi, m_tileOwnerLo).
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
    if (!reg->AddB(m_tileOwnerHi, m_tileOwnerLo)) {
        reg->m_38->m_8 |= 0x10000;
        m_toySprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateStaminaSprite()
// Gate: m_staminaSprite unset AND m_stamina != 0x64. geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_tileOwnerHi, m_tileOwnerLo, m_stamina).
RVA(0x0004d2f0, 0xb4)
i32 CGrunt::CreateStaminaSprite() {
    if (m_staminaSprite || m_stamina == 0x64) {
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
    if (!reg->AddA(m_tileOwnerHi, m_tileOwnerLo, m_stamina)) {
        reg->m_38->m_8 |= 0x10000;
        m_staminaSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateToyTimeSprite()
// Gate: m_toyTimeSprite unset AND m_toyTime != 0. First clears the stamina sprite
// (m_staminaSprite) and wingz-time sprite (m_wingzTimeSprite) if set (OR 0x10000 into their record's
// +0x8, null the slot). geoB = m_60 - 0x20; hint 0xdbba0.
// Register via AddA(m_tileOwnerHi, m_tileOwnerLo, m_toyTime).
RVA(0x0004d3e0, 0xf5)
i32 CGrunt::CreateToyTimeSprite() {
    if (m_toyTimeSprite || m_toyTime == 0) {
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
    if (!reg->AddA(m_tileOwnerHi, m_tileOwnerLo, m_toyTime)) {
        reg->m_38->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateWingzTimeSprite()
// Gate: m_wingzTimeSprite unset AND m_wingzEnabled != 0 AND m_wingzTime != 0. Clears the
// toy-time sprite (m_toyTimeSprite) if set. geoB = m_60 - 0x26; hint 0xdbba0.
// Register via AddA(m_tileOwnerHi, m_tileOwnerLo, m_wingzTime).
RVA(0x0004d520, 0xe3)
i32 CGrunt::CreateWingzTimeSprite() {
    if (m_wingzTimeSprite || m_wingzEnabled == 0 || m_wingzTime == 0) {
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
    if (!reg->AddA(m_tileOwnerHi, m_tileOwnerLo, m_wingzTime)) {
        reg->m_38->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreatePowerupSprite(int a)
// Gate: m_powerupSprite unset. geoB = m_60 (no offset); hint 0x15.
// Register via AddC(m_tileOwnerHi, m_tileOwnerLo, a).
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
    if (!reg->AddC(m_tileOwnerHi, m_tileOwnerLo, a)) {
        reg->m_38->m_8 |= 0x10000;
        m_powerupSprite = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CreateSelectedSprite()
// Gate: m_selectedSprite unset. geoB = m_60 (no offset); hint 0x14.
// Register via AddD(m_tileOwnerHi, m_tileOwnerLo).
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
    if (!reg->AddD(m_tileOwnerHi, m_tileOwnerLo)) {
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
// Fills the per-pose animation-name index table (m_poseWalk..m_poseItem2) by looking up
// "GRUNTZ_" + this->m_animSetName + "_<POSE>" in the entrance player's
// name->animset hash (m_154->m_c->m_2c->m_10map). __thiscall, ret 8 (/GX - the
// two operator+ CString temporaries per block carry a C++ EH frame).
//
//   * kind==0  : load the full grunt pose set (WALK/ATTACK*/STRUCK*/IDLE1..5/
//                ITEM*/DEATH; 14 poses).
//   * kind!=0, toyOnly!=0 : reload only WALK + TOY-BREAK.
//   * kind!=0, toyOnly==0 : reload the toy poses (TOY1, TOY2, TOY-BREAK) and
//                derive the toy-swap blend percent m_toyBlendPct from the relative
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
        LOAD_POSE(m_poseWalk, s_pose_WALK);
        LOAD_POSE(m_poseAttack1, s_pose_ATTACK1);
        LOAD_POSE(m_poseAttack2, s_pose_ATTACK2);
        LOAD_POSE(m_poseAttackIdle, s_pose_ATTACKIDLE);
        LOAD_POSE(m_poseStruck1, s_pose_STRUCK1);
        LOAD_POSE(m_poseStruck2, s_pose_STRUCK2);
        LOAD_POSE(m_poseIdle[0], s_pose_IDLE1);
        LOAD_POSE(m_poseIdle[1], s_pose_IDLE2);
        LOAD_POSE(m_poseIdle[2], s_pose_IDLE3);
        LOAD_POSE(m_poseIdle4, s_pose_IDLE4);
        LOAD_POSE(m_poseIdle5, s_pose_IDLE5);
        LOAD_POSE(m_poseItem, s_pose_ITEM);
        LOAD_POSE(m_poseItem2, s_pose_ITEM2);
        LOAD_POSE(m_poseDeath, s_pose_DEATH);
        return;
    }

    if (toyOnly != 0) {
        LOAD_POSE(m_poseWalk, s_pose_WALK);
    } else {
        LOAD_POSE(m_poseToy1, s_pose_TOY1);
        i32 x = ((CAnimSetNode*)m_poseToy1)->m_10;
        LOAD_POSE(m_poseToy2, s_pose_TOY2);
        i32 y = ((CAnimSetNode*)m_poseToy2)->m_10;
        if (x >= y) {
            m_toyBlendPct = (i32)(100.0 / ((double)x / y - -1.0) - -0.5);
        } else {
            m_toyBlendPct = 100 - (i32)(100.0 / ((double)y / x - -1.0) - -0.5);
        }
    }

    LOAD_POSE(m_poseToyBreak, s_pose_TOYBREAK);
}

#undef LOAD_POSE

// ---------------------------------------------------------------------------
// CGrunt::ResetEntranceAnimation(int apply, int cycle, int cue)   @0x62e10
// The shared entrance/idle-anim reset the entrance state machine (and its two
// callers BuildEntranceAnimation + LoadEntranceConfig) run to (re)select and
// arm the grunt's entrance animation. __thiscall, ret 0xc (/GX - the per-cell
// key CString temp carries a C++ EH frame).
//
//   * clears the "applied" flag (m_resetApplied = 0), then reverse-looks-up the current
//     active-anim-set node's NAME and tests it against the idle key "A".
//   * if the grunt is NOT already idle and `cycle`==0: re-anchor the idle timer
//     (m_idleAnchorLo.. bookkeeping) off the IdleDelay config (rand window).
//   * else dispatches on the geometry-source array m_poseIdle[0..2]: a single source
//     (m_3b0==0) re-arms it; the 2-arg branch (cycle!=0) randomly cycles among
//     the available sources, firing a focused-grunt on-screen "cue" (consts
//     4/5/6 by index) when `cue`!=0 and the grunt is visible.
//   * latches a fresh active-anim-set node into m_14->m_1c (saving the old into
//     m_prevAnimSetNode), and finally - when something was applied or `apply`!=0 - rebuilds
//     the per-cell entrance-position key string (CString from the m_474 cell
//     table, indexed 3*col+row by either the per-grunt triple m_entranceCell or a preset
//     by reason m_444) and stamps the first frame.
static i32 s_entrancePreset0[3]; // DAT_00644aa0
static i32 s_entrancePreset1[3]; // DAT_00644ac0
static i32 s_entrancePreset2[3]; // DAT_00644ad0

RVA(0x00062e10, 0x47e)
void CGrunt::Stub_062e10(i32 apply, i32 cycle, i32 cue) {
    m_resetApplied = 0;

    i32 notIdle = strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), s_animKeyA) != 0;
    i32 applied = 0;

    if (notIdle && cycle == 0) {
        // Re-anchor the idle timer to a randomized IdleDelay window.
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseIdle[0]);
        m_idleWindowLo = 0x3a98;
        m_idleWindowHi = 0;
        m_idleTimerLo = (i32)g_645588;
        m_idleTimerHi = 0;
        i32 n = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530) + 1;
        m_idleDelayLo = GruntRand() % n + 0x7530;
        m_idleDelayHi = 0;
        m_idleAnchorLo = (i32)g_645588;
        m_idleAnchorHi = 0;
        applied = 1;
    } else if (m_poseIdle[1] == 0) {
        // Single geometry source: re-arm it (no flag set).
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseIdle[0]);
    } else if (cycle == 0) {
        // Already on this source? nothing to do.
        if ((void*)m_154->m_1b4 == (void*)m_poseIdle[0]) {
            goto latch;
        }
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseIdle[0]);
        {
            i32 d = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530);
            applied = 1;
            m_idleDelayLo = GruntRand() % (d - 0x4e1f) + 0x4e20;
            m_idleDelayHi = 0;
            m_idleAnchorLo = (i32)g_645588;
            m_idleAnchorHi = 0;
        }
    } else {
        // Cycle among the available sources, with the focused-grunt cue.
        i32 count = (m_poseIdle[2] == 0) ? 1 : 2;
        i32 idx = GruntRand() % count + 1;
        if (cue != 0) {
            CGameRegistry* g = g_pGameRegistry;
            g->CuePrep();
            i32 focused = (m_tileOwnerHi == g_focusedGruntSentinel);
            if (focused && idx > 0x5a) {
                if (CueVisible(g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
                    g->m_60->Cue((i32)this, 4, -1, -1, -1);
                }
            } else if (focused || m_entranceReason != 0) {
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
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseIdle[idx]);
        m_resetApplied = 1;
        applied = 1;
    }

latch:
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyA);

    if (!applied && apply == 0) {
        return;
    }

    // Rebuild the per-cell entrance key string + first frame. The cell is the
    // per-grunt triple {col,row,reason}, unless a non-default entrance reason
    // selects a preset triple.
    i32 col = m_entranceCell[0];
    i32 row = m_entranceCell[1];
    i32 reason = m_entranceCell[2];
    if ((void*)m_154->m_1b4 != (void*)m_poseIdle[0]) {
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
//   * if the entrance is active (m_entranceActive) and the grunt has not moved off its
//     last tile (m_5c==m_lastTilePxX, m_60==m_lastTilePxY) and that tile's high occupancy bit is
//     clear, clear m_entranceActive.
//   * arm the entrance geometry source (m_154->m_1a0.SetGeoSourceR); gate the
//     arrival on the elapsed-time window (clock - m_830_64 >= m_838_64) and the
//     grunt being off-screen (registry m_134 != 1) + its focus slot live.
//   * the arrival commit: notify the tile manager, latch m_tileClaimed, seed the
//     defender block (m_defenderX..m_314, m_arrivalState/2d4/2dc/2f0/2f4, m_arrivalFlags |= 0x18040402),
//     clear m_10's view-cull rect, run the arrival hook.
//   * tail: if the entrance anim is done (m_154->m_1b4 != m_poseIdle[0]) run
//     Stub_062e10(0,0,0) on the lookup-miss flags; else, when the idle window has
//     elapsed and the geometry source is ready, run Stub_062e10(0,1,1).
RVA(0x000633e0, 0x2ca)
void CGrunt::Stub_0633e0() {
    if (m_entranceActive != 0 && m_10->m_5c == m_lastTilePxX && m_10->m_60 == m_lastTilePxY) {
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
            m_entranceActive = 0;
        }
    }

    i32 ready = m_154->m_1a0.SetGeoSourceR(g_defaultGeo);

    if ((i64)(u32)g_645588 - *(i64*)&m_idleTimerLo >= *(i64*)&m_idleWindowLo) {
        CGameRegistry* g = g_pGameRegistry;
        i32 mode = g->m_134;
        if (mode != 1) {
            CFocusSlot* slot = (CFocusSlot*)((char*)g + 0x150 + m_tileOwnerHi * 0x238);
            if (slot != 0 && slot->m_14 != 0) {
                if (m_tileClaimed == 0 && m_arrivalNotified == 0 && mode == 2
                    && g_focusedGruntSentinel == m_tileOwnerHi && m_arrived == 0) {
                    m_tileMgr->NotifyArrival(m_tileOwnerHi, m_tileOwnerLo);
                    m_arrivalNotified = 1;
                    goto tail;
                }
                if (mode != 2 && g_focusedGruntSentinel == m_tileOwnerHi && m_arrived == 0
                    && m_tileClaimed != 1) {
                    m_308 = 0;
                    m_310 = 0;
                    m_30c = 0;
                    m_314 = 0;
                    m_defenderX = m_lastTilePxX;
                    m_defenderY = m_lastTilePxY;
                    m_tileClaimed = 1;
                    i32 kind = m_entranceReason;
                    switch (kind) {
                        case 2:
                            m_defenderRadius = 1;
                            break;
                        default:
                            m_defenderRadius =
                                g_buteMgr.GetIntDef(s_Grunt, s_PlayerDefenderRadius, 3) + 1;
                            break;
                    }
                    m_2f0 = -1;
                    m_2f4 = -1;
                    m_arrivalState = 4;
                    m_2d4 = 0;
                    m_230 = 0;
                    m_arrivalFlags |= 0x18040402;
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
    if ((void*)m_154->m_1b4 != (void*)m_poseIdle[0]) {
        if (m_154->m_1c8 == 0 && m_154->m_1c0 != 0) {
            Stub_062e10(0, 0, 0);
        }
        return;
    }
    if ((i64)(u32)g_645588 - *(i64*)&m_idleAnchorLo >= *(i64*)&m_idleDelayLo && ready == 1) {
        Stub_062e10(0, 1, 1);
    }
}

// ---------------------------------------------------------------------------
// CGrunt::StepEntranceReinit()   @0x637a0   (ret 0)
// @early-stop
// large-state-machine + grid-regalloc plateau: the D/L early rejects, the +0x8c0
// struck-timer reset, the "I"-code tile-mgr re-notify + idle reseed, and the two
// visibility-gated anim-set re-latch + entrance-cell frame re-stamp arms are
// reconstructed in shape/order. Residue is the strcmp-eq setcc sentinel pinning
// (docs/patterns/strcmp-eq-bool-local-setcc + zero-register-pinning), the deep
// g_gameReg->m_70 board chains modeled by raw offset, and the cross-arm regalloc on
// the shared cell-frame tail. Deferred to the final sweep.
RVA(0x000637a0, 0x2f8)
i32 CGrunt::StepEntranceReinit() {
    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeL) == 0);
    if (eq) {
        return 0;
    }
    // Reset the struck-cooldown timer window (m_8c0..m_8cc) to a fresh 0x7530 from
    // the running game clock.
    *(i32*)((char*)this + 0x8c8) = 0x7530;
    *(i32*)((char*)this + 0x8cc) = 0;
    *(i32*)((char*)this + 0x8c0) = (i32)g_645588;
    *(i32*)((char*)this + 0x8c4) = 0;
    *(i32*)((char*)this + 0x358) = 0;

    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        // code "I": re-notify the tile mgr of the arrival.
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        *(i32*)((char*)this + 0x218) = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ReseedIdleReset(1, 0, 0);
    }
    *(i32*)((char*)this + 0x35c) = 0;
    if (*(i32*)((char*)this + 0x328) == 0) {
        return 0;
    }

    // The head occupied-coord's tile is clear of the spawn-block bit -> re-latch a
    // fresh "D" anim set and re-stamp the first entrance-cell frame.
    GruntCoord* co = (*(GruntCoordNode**)((char*)this + 0x320))->m_coord;
    GruntBoard* b = g_gameReg->m_70;
    i32 flag;
    if ((u32)co->m_x >= (u32)b->m_c || (u32)co->m_y >= (u32)b->m_10) {
        flag = 1;
    } else {
        flag = ((i32*)b->m_8[co->m_y])[co->m_x * 7];
    }
    i32* cell;
    if (!(flag & 0x20000000)) {
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        cell = m_entranceCell;
    } else {
        // The grunt's own HUD point is unobstructed (the 0x80 walkable bit).
        i32 tx = m_10->m_5c >> 5;
        i32 ty = m_10->m_60 >> 5;
        i32 flag2;
        if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
            flag2 = 1;
        } else {
            flag2 = ((i32*)b->m_8[ty])[tx * 7];
        }
        if (!(flag2 & 0x80)) {
            return 0;
        }
        m_entranceActive = 1;
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        cell = m_entranceCell;
    }
    i32 col = cell[1] + cell[0] * 2;
    i32 base = cell[0] + col;
    i32 row = base * 3;
    i32 idx = base + row * 4;
    char* nm = GruntStrGetBuffer((char*)this + idx * 8 + 0x470, 0);
    m_154->SetAnimName(nm);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RunEntranceMove()   @0x67850   (ret 0)
// @early-stop
// large-state-machine plateau: the armed-but-not-running sub-player gate, the
// scratch-resolver "D" re-latch (GetNameRecords + the scratch CString teardown),
// the on-arrival HUD-stat-sprite creation, the entrance-cell frame re-stamp, and the
// +0x1a0 move-mode dispatch are reconstructed in shape/order. Residue is the
// scratch loop-strength-reduction (shared, no source spelling), the short-circuit
// gate branch ordering, and the cross-arm regalloc. Deferred to the final sweep.
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);
    // The geometry sub-player @m_154+0x1a0: m_20/m_28 live past its own m_1b4, so
    // read via raw offsets off &player->m_1a0 (keeps cl on one base).
    i32* sub = (i32*)((char*)m_154 + 0x1a0);
    if (!((sub[0xa] != 0 && sub[8] == 0) || *(i32*)((char*)this + 0x1a0) == 0)) {
        return 0;
    }

    m_entranceActive = 0;
    char* nm0 = g_animNameResolver.GetNameRecords((void*)m_prevAnimSetNode)->m_name;
    GruntScratchTeardown();
    bool eq;
    eq = (strcmp(nm0, g_codeD) == 0);
    if (eq) {
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            *(i32*)((char*)this + 0x218) = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        *(i32*)((char*)this + 0x35c) = 0;
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        i32* cell = m_entranceCell;
        i32 col = cell[1] + cell[0] * 2;
        i32 base = cell[0] + col;
        i32 row = base * 3;
        i32 idx = base + row * 4;
        char* nm = GruntStrGetBuffer((char*)this + idx * 8 + 0x470, 0);
        m_154->SetAnimName(nm);
    } else {
        ReseedIdleReset(1, 0, 0);
    }

    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    i32 mode = *(i32*)((char*)this + 0x1a0);
    if (mode == -1) {
        return 0;
    }
    if (mode >= 0x32) {
        return ApplyMoveMode(mode);
    }
    if (mode >= 0x22) {
        *(i32*)((char*)this + 0x194) = mode;
        *(i32*)((char*)this + 0x1a0) = -1;
        return 1;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        return 0;
    }
    return ApplyMoveMode(mode);
}

// ---------------------------------------------------------------------------
// CGrunt::BuildEntranceAnimation(int mode)   @0x67bd0
// Selects + loads the grunt's entrance animation (the "drop in / resurrect /
// random arrival" sequence). Latches a fresh active-anim-set node into m_14->m_1c
// (saving the old into m_prevAnimSetNode), seeds the entrance bookkeeping (m_entranceArmed=1, m_entranceCommitted=0,
// m_entranceActive=1), marks the HUD geometry dirty (m_10->m_74 = 0xcf850; m_10->m_8 |=
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
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyK);

    m_entranceArmed = 1;
    m_entranceCommitted = 0;
    m_entranceActive = 1;
    if (m_10->m_74 != 0xcf850) {
        m_10->m_74 = 0xcf850;
        m_10->m_8 |= 0x20000;
    }

    EntrancePrepare(); // thunk_FUN_0044b240 (a void this-method)

    CString key;

    // The on-screen / focused-grunt gate: fire the cue when the grunt is inside
    // the visible view rect, or when it is the registry's focused grunt and its
    // m_tileOwnerHi matches the focus sentinel.
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
            if (this == (CGrunt*)focus && m_tileOwnerHi == g_focusedGruntSentinel) {
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
        m_prevEntranceDesc = (i32)m_154->m_1b4;
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
// clears the OLD tile (m_lastTilePxX/m_lastTilePxY pixel coords, -1 = none) and stamps the NEW
// one (set occupancy bit 0x20<<24, write packed (m_tileOwnerHi<<8)|m_tileOwnerLo owner), then
// posts the wire call and records the new tile pixel coords. Marks the HUD anim
// id dirty (m_10->m_74 = m_60 + 0x186a0; m_8 |= 0x20000), looks the DROP entrance
// sprite-set up in the entrance player's table, and either (set found ==
// m_154->m_1b4) fires the focused-grunt entrance cue + claims the tile + reads the
// EntranceSafeTime config + seeds the safe-time bookkeeping, or (miss) releases the
// tile and runs the on-released hook. Finally clears m_entranceActive, reloads the per-grunt
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
            if (m_tileOwnerHi != b || m_tileOwnerLo != a) {
                m_tileMgr->SetTile(b, a, 2, m_tileOwnerHi);
            }
        }

        // Re-stamp the occupancy grid: clear old tile, set new tile.
        h = m_10;
        i32 oldX = m_lastTilePxX;
        m_entranceArmed = 0;
        i32 newPxX = h->m_5c;
        i32 newPxY = h->m_60;
        i32 oldTileX = oldX >> 5;
        i32 oldTileY = m_lastTilePxY >> 5;
        i32 newTileX = newPxX >> 5;
        i32 newTileY = newPxY >> 5;

        if (oldX != -1 && m_lastTilePxY != -1) {
            CTileGrid* og = g_pGameRegistry->m_70;
            ((char*)&og->m_8[oldTileY][oldTileX * 7])[3] &= ~0x20;
            og->m_8[oldTileY][oldTileX * 7 + 1] = -1;
        }
        {
            CTileGrid* ng = g_pGameRegistry->m_70;
            ((char*)&ng->m_8[newTileY][newTileX * 7])[3] |= 0x20;
            ng->m_8[newTileY][newTileX * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        }
        m_lastTilePxX = newPxX;
        m_lastTilePxY = newPxY;
        m_tileMgr->PostWire();

        h = m_10;
        m_entranceCommitted = 1;
        if (h->m_74 != h->m_60 + 0x186a0) {
            h->m_74 = h->m_60 + 0x186a0;
            h->m_8 |= 0x20000;
        }

        CEntranceAnimPlayer* p = m_154;
        CSprite* found = 0;
        void* cached = p->m_1b4;
        p->m_c->m_2c->m_10map.Lookup(s_GRUNTZ_ENTRANCEZ_DROP, &found);
        if ((void*)found == cached) {
            if (m_tileOwnerHi == g_focusedGruntSentinel) {
                g_pGameRegistry->m_60->CueA(this, 0x33f, -1, 0, -1, -1);
            }
            m_tileMgr->ClaimTile(m_tileOwnerHi, m_tileOwnerLo, 0, 0);
            m_364 = 1;
            m_entranceSafeTimeLo = g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_645588;
            m_entranceClockHi = 0;
            m_858 = 0;
            m_85c = 0;
        } else {
            if (m_tileMgr->ReleaseTile(m_tileOwnerHi, m_tileOwnerLo)) {
                EntranceOnReleased();
            }
        }
        m_entranceActive = 0;
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

    m_timePerTile = g_buteMgr.GetDwordDef((char*)(const char*)m_animSetName, s_TimePerTile, 1000);

    if (m_gruntKind == 0x37) {
        m_timePerTile >>= 1;
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

// The grunt movement / anim-name dispatch state machines' reloc-masked data.
// All TU-local definitions (reloc-masked against the retail symbols); the grunt
// freelist aliases the same g_freePoolHead/Base pool (0x645544 / 0x64554c).
WwdGameReg* g_gameReg;             // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
GruntCoordPool g_coordPool;        // DAT_00645540
CAnimScratchString* g_animScratch; // DAT_006bf66c
i32 g_animScratchCount;            // DAT_006bf670
void* g_gruntFreeList;             // DAT_00645544 (same pool as g_freePoolHead)
i32 g_gruntFreeListBias;           // DAT_0064554c (same as g_freePoolBase)

// The single-letter anim type-code literals (1-char .rodata, reloc-masked).
const char g_codeA[] = "A";
const char g_codeD[] = "D";
const char g_codeI[] = "I";
const char g_codeG[] = "G";
const char g_codeL[] = "L";
const char g_codeP[] = "P";
const char g_codeO[] = "O";
const char g_codeQ[] = "Q";
const char g_codeJ[] = "J";
const char g_codeN[] = "N";
const char g_codeM[] = "M";
const char g_codeK[] = "K";
const char g_codeF[] = "F";
const char g_codeE[] = "E";

// CGrunt::IsSameType(a, b) @0x3c7f0 - a free __cdecl comparator returning
// whether two grunts share the same type record (their +0x8 sub-object ptr).
RVA(0x0003c7f0, 0x18)
bool CGrunt_IsSameType(CGrunt* a, CGrunt* b) {
    return *(void**)((char*)a + 8) == *(void**)((char*)b + 8);
}

// The teardown helpers and the three retail vtables (reloc-masked).
void* g_cgruntVtbl;    // 0x5e8754
void* g_userLogicVtbl; // 0x5e705c
void* g_userBaseVtbl;  // 0x5e70b4

// ---------------------------------------------------------------------------
// CGrunt::~CGrunt   @0xf2f0   (__thiscall, /GX leaf dtor)
// Stamp the most-derived vtable (0x5e8754), drain the per-grunt name caches
// (FreeNameList), then tear down the six owned sub-objects in /GX trylevel order
// (the +0x468 cell array via the vector-dtor iterator, the two +0x44c/+0x448
// CStrings, the two +0x338/+0x31c CObLists, m_animSetName @+0x1c0), and finally
// fold the bare CUserLogic base teardown (CUserLogic vptr -> ~EngStr on the +0x18
// link -> CUserBase vptr). Every callee is external/reloc-masked.
//
// @early-stop
// /GX EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md +
// eh-dtor-model-members-as-destructible.md): the destructible-member model
// recovers the full /GX frame (push -1/fs:0), the __thiscall member-dtor calls
// (lea ecx,[this+off]; call - no stack push/cleanup), the correct member offsets
// (+0x468/+0x44c/+0x448/+0x338/+0x31c/+0x1c0) in retail order, the vector-dtor
// iterator args, the three vptr restamps, and the base ~EngStr fold - all
// byte-faithful in shape/order (42%->55.5%). The residue is the per-member
// descending trylevel chain ([esp+0x14] = 7,6,5,4,3,2,1 then 8): retail numbers
// each member's own EH state because CGrunt's members are true value subobjects of
// a CUserLogic-derived class, whereas the flat CGrunt model destructs them via
// explicit method calls (one trylevel). Emitting the per-member states needs CGrunt
// re-rooted on CUserLogic with the six members as real value fields - a TU-wide
// restructure that would shift the ~40 other matched methods. Deferred to the
// final sweep.
RVA(0x0000f2f0, 0xc8)
CGrunt::~CGrunt() {
    *(void**)this = &g_cgruntVtbl;
    FreeNameList();
    GruntVecDtor((char*)this + 0x468, 0x68, 9, &GruntCellDtor);
    ((GruntStrSub*)((char*)this + 0x44c))->Dtor();
    ((GruntStrSub*)((char*)this + 0x448))->Dtor();
    ((GruntListSub*)((char*)this + 0x338))->Dtor();
    ((GruntListSub*)((char*)this + 0x31c))->Dtor();
    ((GruntStrSub*)((char*)this + 0x1c0))->Dtor();
    *(void**)this = &g_userLogicVtbl;
    ((GruntLinkSub*)((char*)this + 0x18))->Dtor();
    *(void**)this = &g_userBaseVtbl;
}

// ---------------------------------------------------------------------------
// CGrunt::PlaySound(range, rec)   @0x4ac10   (__thiscall, ret 0x10)
// The directional grunt-voice entrance handler PlayMoveSound fires. `rec` is the
// 3-DWORD compass voice record passed by value {col, row, flag}; the latched cell
// record is m_entranceCell (this+0x43c). It bails if the new record matches the
// latched one (same +8 flag), else dispatches on the grunt's current anim-name
// single-letter type code (F/D/A/K/E/I/M) to one of four geometry arms, re-stamps
// the entrance player's geometry + per-cell frame from the m_474 cell tables, and
// latches the record into m_entranceCell. `range` (1000) is unused here.
// @early-stop
// regalloc/frame plateau (~62%): the full dispatch CFG, all 7 type-code arms (the
// bool-eq inline-strcmp setcc form), the 4 geometry arms, the 3 cell tables
// (0x468/0x474/0x470, (3*col+row)*0x68), and the record latch are byte-exact in
// shape/order. Residue = retail reserves a 0xc scratch frame + spills rec fields
// where mine keeps them in regs (cellrec pinned ebp vs ebx), plus the merged
// E/IDLE GetName tail and per-arm esi/edx placement - pure register/spill
// scheduling, no source lever flips it. Closing this brought PlayMoveSound to 100%.
// Deferred to the final sweep.
RVA(0x0004ac10, 0x402)
void CGrunt::PlaySound(i32 range, CGruntVoiceRec rec) {
    (void)range;
    if (CGrunt_IsSameType((CGrunt*)m_entranceCell, (CGrunt*)&rec)) {
        return;
    }

    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeF) == 0);
    if (eq) {
        return;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (eq) {
        goto walk;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeK) == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeE) == 0);
    if (eq) {
        // code "E": drive the ATTACK-IDLE geometry, stamp the cell frame from the
        // latched m_entranceCell triple (cell table base 0x468).
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(*(i32*)((char*)this + 0x3a0));
        {
            CEntranceAnimDescColl* desc = m_154->m_1b4;
            i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
            i32 frame = elem[0x14 / 4];
            i32 col = m_entranceCell[0];
            i32 row = m_entranceCell[1];
            i32 index = 3 * col + row;
            const char* nm = ((CGruntCell*)((char*)this + 0x468 + index * 0x68))->GetName(0);
            m_154->SetAnimFrame(nm, frame);
        }
        goto store;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        goto codeI;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeM) == 0);
    if (eq) {
        goto walk;
    }

codeI:
    // code "I": latch the record first, drive the IDLE2 geometry, reseed the idle
    // timer. Returns directly (no cell-frame stamp).
    m_entranceCell[0] = rec.m_0;
    m_entranceCell[1] = rec.m_4;
    m_entranceCell[2] = rec.m_8;
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(*(i32*)((char*)this + 0x3b0));
    ReseedIdleReset(1, 0, 0);
    return;

idle:
    // codes "A"/"K": drive the IDLE1 geometry (the forwarding setter), stamp the
    // cell frame from the incoming record (cell table base 0x474).
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->SetGeometryEx(*(i32*)((char*)this + 0x3ac), 0);
    {
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];
        i32 col = rec.m_0;
        i32 row = rec.m_4;
        i32 index = 3 * col + row;
        const char* nm = ((CGruntCell*)((char*)this + 0x474 + index * 0x68))->GetName(0);
        m_154->SetAnimFrame(nm, frame);
    }
    goto store;

walk:
    // codes "D"/"M" (and the default): drive the WALK geometry, stamp the cell name
    // from the incoming record (cell table base 0x470), set it by name only.
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(*(i32*)((char*)this + 0x394));
    {
        i32 col = rec.m_0;
        i32 row = rec.m_4;
        i32 index = 3 * col + row;
        const char* nm = ((CGruntCell*)((char*)this + 0x470 + index * 0x68))->GetName(0);
        m_154->SetAnimName(nm);
    }

store:
    m_entranceCell[0] = rec.m_0;
    m_entranceCell[1] = rec.m_4;
    m_entranceCell[2] = rec.m_8;
}

// CGrunt::ClearAllSprites() @0x4b240 - on death/teardown, flag each live HUD
// sprite record (+0x8 |= 0x10000) and null its slot. The stamina/toy-time/
// wingz-time trio is gated on m_entranceCommitted==0 (entrance not yet committed). Clears the
// arrival gate m_arrived last.
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
    if (m_entranceCommitted == 0) {
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
    m_arrived = 0;
}

// The 8 compass grunt-voice records (3 DWORDs each, runtime-filled .data) +
// PlaySound (the @0x4ac10 entrance handler, external/reloc-masked). TU-local
// definitions so each `mov ds:addr` reloc-masks against retail.
i32 g_voiceN[3];
i32 g_voiceS[3];
i32 g_voiceE[3];
i32 g_voiceW[3];
i32 g_voiceSE[3];
i32 g_voiceNW[3];
i32 g_voiceNE[3];
i32 g_voiceSW[3];

// CGrunt::PlayMoveSound(x, y) @0x511b0 - directional grunt-voice dispatcher.
// Bucketize the screen vector (x,y) - (m_10->m_5c, m_10->m_60) into one of 8
// compass directions by the slope dy/dx vs {+-2.0f, +-0.5} and fire the matching
// 3-DWORD voice record through PlaySound(1000, rec). __thiscall, ret 8, frameless.
RVA(0x000511b0, 0x246)
void CGrunt::PlayMoveSound(i32 x, i32 y) {
    CGruntHud* h = m_10;
    i32 dy = y - h->m_60;
    i32 dx = x - h->m_5c;
    i32 cx = h->m_5c;

    if (dx == 0) {
        if (y > h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceN);
        } else if (y < h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceS);
        }
        return;
    }

    float ratio = (float)dy / dx;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (y > h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceN);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceS);
        }
        return;
    }
    if (ratio <= 0.5 && ratio >= -0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceW);
        }
        return;
    }
    if (ratio > 0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceSE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceNW);
        }
        return;
    }
    if (ratio < -0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceNE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceSW);
        }
    }
}

// CGrunt::CanShowStamina() @0x514a0 - the stamina-bar visibility gate: shown
// only if not powered-up (m_218==0), stamina below full (m_stamina < 0x64), and not
// mid-entrance (m_entranceActive==0).
RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (*(i32*)((char*)this + 0x218) == 0 && m_stamina >= 0x64 && m_entranceActive == 0) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::PlayMoveSoundAtTile(tx, ty)   @0x514e0   (__thiscall, ret 8)
// Scale the tile coords to HUD pixel centers (tile*32 + 16) and forward to the
// directional grunt-voice dispatcher. `this` flows straight through to
// PlayMoveSound (both __thiscall).
RVA(0x000514e0, 0x1e)
void CGrunt::PlayMoveSoundAtTile(i32 tx, i32 ty) {
    PlayMoveSound(tx * 0x20 + 0x10, ty * 0x20 + 0x10);
}

// ---------------------------------------------------------------------------
// CGrunt::SnapToLastTile(a)   @0x517b0   (__thiscall, ret 4)
// Snap the grunt's HUD position to its last occupied tile (m_10->m_5c/m_60 =
// m_lastTilePxX/Y), bump the entrance latched-anim id (m_10->m_74 += 0x186a0 ->
// marks the HUD geometry dirty), commit the entrance position (SetEntrancePos(a,1)),
// and - if an arrival is pending (m_arrivalPending) - notify the tile mgr of the
// settled move and clear the pending latch.
RVA(0x000517b0, 0x7d)
void CGrunt::SnapToLastTile(i32 a) {
    m_10->m_5c = m_lastTilePxX;
    m_10->m_60 = m_lastTilePxY;
    CGruntHud* h = m_10;
    if (h->m_74 != h->m_60 + 0x186a0) {
        h->m_74 = h->m_60 + 0x186a0;
        h->m_8 |= 0x20000;
    }
    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {
        m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
        m_arrivalPending = 0;
    }
}

// ---------------------------------------------------------------------------
// CGrunt::ClaimSwitchTile()   @0x52c70   (__thiscall, ret 0)
// Pick a neighbour tile by the entrance-cell direction code (m_entranceCell[2],
// 1..8 -> the 8 compass deltas; anything else keeps the current tile), test the
// level board's occupancy flags there; if the tile is clear of the blocking bits
// (0x20000939 / 0x80), apply the tile switch (tile mgr), move the occupancy record
// from the old tile to the new one (clear bit 5 of the old tile's flag byte, stamp
// the new tile's owner = (ownerHi<<8)|ownerLo + set bit 5), re-anchor the grunt to
// the new pixel pos, recompute the facing (ComputeFacing(1.0)), latch
// m_arrivalPending=1, and return 1. On an obstructed tile return 0.
//
// @early-stop
// switch jump-table + grid-regalloc wall (docs/patterns: switch-cases-source-order,
// align-down-byte-and-encoding, the regalloc family): logic/CFG/offsets/flag bits +
// both engine calls byte-exact; residue = (1) the 8-way direction switch tail-merges
// the overlapping +-0x20 delta arms in a .text layout no source case-order pins, and
// (2) the x/y move-coords held across both calls + the level-board double-deref
// (g_gameReg->m_70->m_8[ty]) land in a different callee-saved-reg/stack-spill
// assignment than retail (retail spills the pre-switch x/y to [esp+0x18/0x1c] for the
// default arm and keeps x=ebx/y=edi). The memory-RMW byte twiddle is matched
// (55.8%->61.6%); the rest is the documented regalloc/scheduling plateau. Final sweep.
RVA(0x00052c70, 0x1b1)
i32 CGrunt::ClaimSwitchTile() {
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    switch (m_entranceCell[2] - 1) {
        case 0:
            y -= 0x20;
            break;
        case 1:
            x += 0x20;
            y -= 0x20;
            break;
        case 2:
            x += 0x20;
            break;
        case 3:
            x += 0x20;
            y += 0x20;
            break;
        case 4:
            y += 0x20;
            break;
        case 5:
            x -= 0x20;
            y += 0x20;
            break;
        case 6:
            x -= 0x20;
            break;
        case 7:
            x -= 0x20;
            y -= 0x20;
            break;
        default:
            break;
    }

    GruntBoard* b = g_gameReg->m_70;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 flags;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)b->m_8[ty])[tx * 7];
    }
    if ((flags & 0x20000939) || (flags & 0x80)) {
        return 0;
    }

    m_tileMgr->ApplyTileSwitch(this, m_lastTilePxX, m_lastTilePxY);

    // Release the grunt's old tile: clear bit 5 of the old tile's flag byte, set
    // its owner record to -1.
    m_184 = m_lastTilePxX;
    m_188_tilePxY = m_lastTilePxY;
    GruntBoard* gb = g_gameReg->m_70;
    i32 oldTx = m_lastTilePxX >> 5;
    i32 oldTy = m_lastTilePxY >> 5;
    ((char*)gb->m_8[oldTy])[oldTx * 7 * 4 + 3] &= 0xdf;
    *(i32*)&((char*)gb->m_8[oldTy])[oldTx * 7 * 4 + 4] = -1;

    // Claim the new tile: set bit 5 of its flag byte, stamp the owner id.
    i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
    ((char*)gb->m_8[ty])[tx * 7 * 4 + 3] |= 0x20;
    *(i32*)&((char*)gb->m_8[ty])[tx * 7 * 4 + 4] = owner;

    m_lastTilePxX = x;
    m_lastTilePxY = y;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::SetArrivalTarget(a, b, c, d)   @0x52ed0   (__thiscall, ret 0x10)
// Seed the arrival/defender block: m_2f0=a, m_2f4=b, m_230=1, and the two defender
// pixel coords m_defenderX/Y = (c/d aligned down to the tile grid) + 0x10.
//
// @early-stop
// leaf regalloc / align-down-encoding coin-flip (docs/patterns/align-down-byte-and-
// encoding + the regalloc family): all 5 member stores + values + `ret 0x10` exact.
// Residue = retail keeps `c` in edx (dword `and edx,~0x1f`) and materializes the
// constant 1 in eax (`mov eax,1; mov [m_230],eax`) interleaved into the c-block;
// our cl loads c into eax (byte `and al,0xe0`) + stores m_230 as an immediate. No
// source spelling pins which value owns edx vs eax on a 66-byte leaf. Final sweep.
RVA(0x00052ed0, 0x42)
void CGrunt::SetArrivalTarget(i32 a, i32 b, i32 c, i32 d) {
    m_2f0 = a;
    m_2f4 = b;
    m_230 = 1;
    m_defenderX = (c & ~0x1f) + 0x10;
    m_defenderY = (d & ~0x1f) + 0x10;
}

// ---------------------------------------------------------------------------
// CGrunt::ConsiderArrival(a)   @0x52f40   (__thiscall, ret 4)
// If the grunt's HUD point (aligned to the tile grid + 0x10) does not already sit
// on its last occupied tile, ask the drop-ready predicate whether to defer; when it
// is NOT ready, snap to the last tile (SnapToLastTile(a)). When the HUD point IS on
// the last tile, snap unconditionally. Modeled void: retail never sets eax on the
// tail path (no `xor eax,eax`), so the slot is morally void.
//
// @early-stop
// leaf regalloc/schedule coin-flip (the same tiny-accessor family as GetTilePos):
// logic/CFG/offsets exact + the tail (no eax write) byte-matched. Residue = retail
// pins the arg `a` in ebx (3 callee-saved: ebx/esi/edi, arg spilled up-front) and
// loads m_5c->eax/m_60->ecx (m_5c aligns first); our cl uses 2 callee-saved and
// reverses the eax/ecx axis assignment. Source-invariant on a 75-byte leaf. ~84%.
RVA(0x00052f40, 0x4b)
void CGrunt::ConsiderArrival(i32 a) {
    CGruntHud* h = m_10;
    i32 px = (h->m_5c & ~0x1f) + 0x10;
    i32 py = (h->m_60 & ~0x1f) + 0x10;
    if (px != m_lastTilePxX || py != m_lastTilePxY) {
        if (IsDropReady(a)) {
            return;
        }
    }
    SnapToLastTile(a);
}

// ---------------------------------------------------------------------------
// CGrunt::SelectMoveIcon(a)   @0x57800   (__thiscall, ret 4)
// Update the move-cursor icon index (m_1f4_moveIcon). No-op if unchanged; else
// clamp to [0, 0x11), resolve the matching sprite from the registry sprite-ref
// table (g_gameReg->m_74->GetSel(icon, reason>=0x17)) and stamp it onto the HUD
// (m_10->m_4c = sprite, m_50 = 0xa, m_58 = 1).
RVA(0x00057800, 0x64)
void CGrunt::SelectMoveIcon(i32 a) {
    if (m_1f4_moveIcon == a) {
        return;
    }
    m_1f4_moveIcon = a;
    if (a < 0 || a >= 0x11) {
        m_1f4_moveIcon = 0;
    }
    i32 sel = g_gameReg->m_74->GetSel(m_1f4_moveIcon, m_entranceReason >= 0x17);
    CGruntHud* h = m_10;
    h->m_58 = 1;
    h->m_50 = 0xa;
    h->m_4c = sel;
}

// ---------------------------------------------------------------------------
// CGrunt::BuildGruntLoseItemAnimation()   @0x57890   (__thiscall, ret 0, /GX)
// Step the anim dispatch, then if the entrance reason is a lose-item pose
// (0x12/0x16/0xe) spawn the one-shot "SingleAnimation" sprite, set its
// GRUNTZ_<animSet>_LOSEITEM key (both ApplyName + ApplyLookupGeometry forms),
// fire the on-screen spawn cue when the grunt's HUD point is in the cue rect,
// re-run the type-table step, and clear m_entranceActive.
//
// 99.9% (reloc-mask tail): code bytes match incl. the /GX frame + the two
// `s_GRUNTZ_ + m_animSetName + s__LOSEITEM` concat chains; the residual is the
// differently-named reloc operands (ApplyName/ApplyLookupGeometry/LoadGruntTypeTable
// resolve to the engine's own CGameObject/CUserLogic symbols) - the entropy tail.
RVA(0x00057890, 0x19c)
i32 CGrunt::BuildGruntLoseItemAnimation() {
    StepAnimDispatchB();
    i32 reason = m_entranceReason;
    if (reason != 0x12 && reason != 0x16 && reason != 0xe) {
        return 0;
    }

    CHudSprite* spr = g_pGameRegistry->m_30->m_8
                          ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0xcf850, s_SingleAnimation, 0x40003);
    spr->ApplyName(s_GRUNTZ_ + m_animSetName + s__LOSEITEM);
    spr->ApplyLookupGeometry(s_GRUNTZ_ + m_animSetName + s__LOSEITEM, 0);

    CGameRegistry* g = g_pGameRegistry;
    i32 x = m_10->m_5c;
    i32 y = m_10->m_60;
    CCueRect* rc = (CCueRect*)(g->m_30->m_24->m_5c + 0x40);
    if (x < rc->right && x >= rc->left && y < rc->bottom && y >= rc->top) {
        g->m_60->CueSpawn(this, 0xe, -1, -1, -1);
    }

    LoadGruntTypeTable(0, 1, 0, 1);
    m_entranceActive = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::TryPowerupAtTile()   @0x57aa0   (__thiscall, ret 0)
// Gated on a live entrance reason (0 < m_entranceReason < 0x17): read the level
// board's occupancy at the grunt's HUD tile; if it is clear of the blocking bits
// (0x939 / 0x2), probe a move-tile placement via the tile mgr and return 1; else 0.
//
// @early-stop
// single-instruction scheduling coin-flip: logic/CFG/offsets/board-deref/both returns
// byte-exact. Residue = cl loads board->m_c (`mov ecx,[ebx+0xc]`) one slot earlier
// than retail (retail defers it past `add eax,0x10`) and reads g_gameReg before m_10
// vs retail's m_10-first; the rest is identical. ~93%. Final sweep.
RVA(0x00057aa0, 0x9b)
i32 CGrunt::TryPowerupAtTile() {
    i32 reason = m_entranceReason;
    if (reason <= 0 || reason >= 0x17) {
        return 0;
    }
    CGruntHud* h = m_10;
    i32 mx = h->m_5c;
    i32 my = h->m_60;
    GruntBoard* b = g_gameReg->m_70;
    i32 px = (mx & ~0x1f) + 0x10;
    i32 py = (my & ~0x1f) + 0x10;
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 flags;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)b->m_8[ty])[tx * 7];
    }
    if ((flags & 0x939) || (flags & 2)) {
        return 0;
    }
    m_tileMgr->ProbeMoveTile(reason, px, py, 0, 1, 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::RectContains(x, y)   @0x51850   (__thiscall, ret 8)
// @early-stop
// register-relative rect-walk plateau: the logic is exact - builds two tile-space
// rects from the grunt's stored bounds (this+0x290 / this+0x2a0) shifted by the
// committed pixel position (m_17c/m_180)>>5 and inflated by +1 on the high edges,
// tests the query point (x>>5, y>>5) against the live rect(s) via IsRectEmpty, and
// returns whether it is contained. Residue: cl interleaves the two CRect builds
// and reuses the [esp+N] temp slots in an order the source can't pin (it folds the
// member loads + the >>5 shifts across both rects); the IAT-hoisted `IsRectEmpty`
// call shape matches. Pure stack-slot/regalloc scheduling. Deferred to final sweep.
RVA(0x00051850, 0x165)
i32 CGrunt::RectContains(i32 x, i32 y) {
    i32 dx = *(i32*)((char*)this + 0x17c) >> 5;
    i32 dy = *(i32*)((char*)this + 0x180) >> 5;
    i32 px = x >> 5;
    i32 py = y >> 5;

    i32* ra = (i32*)((char*)this + 0x290);
    i32* rb = (i32*)((char*)this + 0x2a0);

    RECT r1;
    r1.left = ra[0] + dx;
    r1.top = ra[1] + dy;
    r1.right = ra[2] + dx + 1;
    r1.bottom = ra[3] + dy + 1;

    RECT r2;
    r2.left = rb[0] + dx;
    r2.top = rb[1] + dy;
    r2.right = rb[2] + dx;
    r2.bottom = rb[3] + dy;

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            // rect2 degenerate: test the point against rect1 only.
            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    // both rects live: the point must sit inside rect1 and (left/top of) rect2.
    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
        if (px >= r2.right || px < r2.left || py >= r2.bottom || py >= r2.top) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RectContainsGated(x, y)   @0x51a20   (__thiscall, ret 8)
// @early-stop
// register-relative rect-walk plateau (sibling of RectContains, same wall): gated
// on the m_198 enable flag, then builds two tile-space rects from this+0x2b0 /
// this+0x2c0 shifted by (m_17c/m_180)>>5 (rect1's high edges +1), and tests the
// query point (x>>5, y>>5) against them via IsRectEmpty + the 4-way bounds compare.
// Residue: identical to RectContains - cl interleaves the two CRect builds and the
// [esp+N] temp-slot reuse in an unpinnable order. Deferred to the final sweep.
RVA(0x00051a20, 0x17d)
i32 CGrunt::RectContainsGated(i32 x, i32 y) {
    i32 px = x >> 5;
    i32 py = y >> 5;
    i32 dx = *(i32*)((char*)this + 0x17c) >> 5;
    i32 dy = *(i32*)((char*)this + 0x180) >> 5;

    i32* ra = (i32*)((char*)this + 0x2b0);
    i32* rb = (i32*)((char*)this + 0x2c0);

    RECT r1;
    r1.left = ra[0] + dx;
    r1.top = ra[1] + dy;
    r1.right = ra[2] + dx + 1;
    r1.bottom = ra[3] + dy + 1;

    RECT r2;
    r2.left = rb[0] + dx;
    r2.top = rb[1] + dy;
    r2.right = rb[2] + dx;
    r2.bottom = rb[3] + dy;

    if (*(i32*)((char*)this + 0x198) == 0) {
        return 0;
    }

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
        if (px >= r2.right || px < r2.left || py >= r2.bottom || py >= r2.top) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepCompassMove()   @0x51c00   (ret 0, /GX)
// The per-tick compass-move driver. The vehicle/move-command field on the current
// tile (or the entrance-cell direction) selects one of 8 compass moves + a grunt-
// voice record; the target tile is validated against board occupancy + corner-cut
// walls; on success the move is committed (release old tile, claim new tile, fire
// the voice cue, re-face) and the move counter advances.
//
// The two grunt-vehicle name -> "ToyTiles" config + the random toy-tile bag drive
// the multi-step toy path. The destructible CString temp + the CToyTileBag local
// force the /GX EH frame.
static char s_ToyTiles[] = "ToyTiles";                     // s_ToyTiles_0060dbf8
static const char s_BABYWALKERGRUNT[] = "BABYWALKERGRUNT"; // s_..._0060da6c
static const char s_BIGWHEELGRUNT[] = "BIGWHEELGRUNT";     // s_..._0060da48
static const char s_GOKARTGRUNT[] = "GOKARTGRUNT";         // s_..._0060da38
static const char s_POGOSTICKGRUNT[] = "POGOSTICKGRUNT";   // s_..._0060d9fc

// Read the tile-flag word at board cell (tx, ty); out-of-bounds -> 1 (blocking).
static __inline i32 s_TileFlags(GruntBoard* b, i32 tx, i32 ty) {
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        return 1;
    }
    return ((i32*)b->m_8[ty])[tx * 7];
}

// True if a move from the grunt's current tile to (moveX, moveY) is committable:
// the target is in-bounds + unoccupied (the arrival/owner flag-word test) AND, for a
// diagonal step, neither cardinal corner tile carries the wall bit (0x2000).
static __inline i32 s_CanCommitMove(CGrunt* g, i32 moveX, i32 moveY) {
    GruntBoard* board = g_gameReg->m_70;
    i32 tx = g->m_lastTilePxX >> 5;
    i32 ty = g->m_lastTilePxY >> 5;
    i32 mtx = moveX >> 5;
    i32 mty = moveY >> 5;
    i32 arr = *(i32*)((char*)g + 0x248) | 0x20000000; // m_arrivalFlags
    if (tx == mtx && ty == mty) {
        return 1;
    }
    if ((u32)mtx >= (u32)board->m_c || (u32)mty >= (u32)board->m_10) {
        return 0;
    }
    i32* tgt = &((i32*)board->m_8[mty])[mtx * 7];
    i32 tflags = *tgt;
    i32 hit = arr & tflags;
    if (hit & 0x20000000) {
        return 0;
    }
    if (hit != 0) {
        i32 mask = *(i32*)((char*)g + 0x24c) | 0x18000482;
        if ((tflags & mask) == 0) {
            return 0;
        }
    }
    i32 dx = mtx - tx;
    i32 dy = mty - ty;
    if (dx == 0 || dy == 0) {
        return 1;
    }
    char* cur = (char*)&((i32*)board->m_8[ty])[tx * 7];
    char* tg = (char*)tgt;
    i32 stride = board->m_c * 7 * 4; // bytes per board row
    if (dx > 0) {
        if (dy > 0) {
            if ((cur[0x1d] & 0x20) || (cur[stride + 1] & 0x20) || (*(i32*)(tg - 0x1c) & 0x2000) ||
                (*(i32*)(tg - stride) & 0x2000))
                return 0;
        } else {
            if ((cur[0x1d] & 0x20) || (*(i32*)(cur - stride) & 0x2000) || (*(i32*)(tg - 0x1c) & 0x2000) ||
                (*(i32*)(tg + stride) & 0x2000))
                return 0;
        }
    } else {
        if (dy > 0) {
            if ((cur[-0x1b] & 0x20) || (cur[stride + 1] & 0x20) || (*(i32*)(tg + 0x1c) & 0x2000) ||
                (*(i32*)(tg - stride) & 0x2000))
                return 0;
        } else {
            if ((cur[-0x1b] & 0x20) || (*(i32*)(cur - stride) & 0x2000) || (*(i32*)(tg + 0x1c) & 0x2000) ||
                (*(i32*)(tg + stride) & 0x2000))
                return 0;
        }
    }
    return 1;
}

// @early-stop
// dual switch jump-table + grid-regalloc + /GX-trylevel wall (the same family as
// ClaimSwitchTile in this TU). Logic/CFG/offsets/flag bits + the compass voice
// records + the board release/claim + both engine calls are reconstructed in
// shape/order. Residue: (1) the move-command + direction switches tail-merge their
// overlapping +-0x20 compass arms in a .text layout no source case-order pins; (2)
// the x/y move coords held across the validity/wall test + the level-board double-
// deref land in a different callee-saved-reg/stack-spill assignment than retail;
// (3) the /GX trylevel transitions for the CString + CToyTileBag temps. Final sweep.
RVA(0x00051c00, 0xc7b)
i32 CGrunt::StepCompassMove() {
    GruntBoard* board = g_gameReg->m_70;
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 result = 0;
    i32 moveX = x;
    i32 moveY = y;
    CGruntVoiceRec voice;

    if (s_TileFlags(board, tx, ty) & 0x80) {
        // The current tile carries a move command at field +0x10 (4th dword).
        i32 cmd = ((i32*)board->m_8[ty])[tx * 7 + 4];
        switch (cmd - 0xb) {
            case 0:
            case 4:
                moveY = y - 0x20;
                voice = *(CGruntVoiceRec*)g_voiceS;
                break;
            case 1:
            case 5:
                moveY = y + 0x20;
                voice = *(CGruntVoiceRec*)g_voiceN;
                break;
            case 2:
            case 6:
                moveX = x - 0x20;
                voice = *(CGruntVoiceRec*)g_voiceW;
                break;
            case 3:
            case 7:
                moveX = x + 0x20;
                voice = *(CGruntVoiceRec*)g_voiceE;
                break;
            case 8:
                switch (m_entranceCell[2] - 1) {
                    case 0:
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceS;
                        break;
                    case 1:
                        moveX = x + 0x20;
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceNE;
                        break;
                    case 2:
                        moveX = x + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceE;
                        break;
                    case 3:
                        moveX = x + 0x20;
                        moveY = y + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceSE;
                        break;
                    case 4:
                        moveY = y + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceN;
                        break;
                    case 5:
                        moveX = x - 0x20;
                        moveY = y + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceSW;
                        break;
                    case 6:
                        moveX = x - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceW;
                        break;
                    case 7:
                        moveX = x - 0x20;
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceNW;
                        break;
                }
                break;
        }
        i32 mtx = moveX >> 5;
        i32 mty = moveY >> 5;
        i32 tflags = s_TileFlags(board, mtx, mty);
        if ((tflags & 0x20000000) && !(tflags & 0x80)) {
            // The target is occupied by another owner: notify the tile mgr (the tile's
            // +0x4 owner id is split into its low two bytes).
            i32 owner;
            if ((u32)mtx >= (u32)board->m_c || (u32)mty >= (u32)board->m_10) {
                owner = -1;
            } else {
                owner = ((i32*)board->m_8[mty])[mtx * 7 + 1];
            }
            m_tileMgr->SetTile((owner >> 8) & 0xff, owner & 0xff, 2, m_tileOwnerHi);
        }
        goto commit;
    }

    // The current tile is a plain walkable tile.
    if (*(i32*)((char*)this + 0x388) != 0) {
        CString str;
        switch (m_entranceReason - 0x17) {
            case 0:
                str = s_BABYWALKERGRUNT;
                break;
            case 2:
                str = s_BIGWHEELGRUNT;
                break;
            case 3:
                str = s_GOKARTGRUNT;
                break;
            case 6:
                str = s_POGOSTICKGRUNT;
                break;
            default:
                break;
        }
        i32 toyCount = g_buteMgr.GetIntDef((char*)(LPCTSTR)str, s_ToyTiles, 1);
        if (*(i32*)((char*)this + 0x388) < toyCount) {
            switch (m_entranceCell[2] - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceS;
                    break;
                case 1:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNE;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceE;
                    break;
                case 3:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSE;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceN;
                    break;
                case 5:
                    moveX = x - 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSW;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceW;
                    break;
                case 7:
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNW;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            if (result == 0) {
                *(i32*)((char*)this + 0x388) = 0;
            }
        } else {
            *(i32*)((char*)this + 0x388) = 0;
        }
    }
    if (result != 0) {
        goto commit;
    }

    // The toy-tile bag: random-pick each of the 8 compass directions in turn and
    // commit the first that validates.
    {
        CToyTileBag bag;
        bag.SetAtGrow(bag.m_count, 1);
        bag.SetAtGrow(bag.m_count, 2);
        bag.SetAtGrow(bag.m_count, 3);
        bag.SetAtGrow(bag.m_count, 4);
        bag.SetAtGrow(bag.m_count, 5);
        bag.SetAtGrow(bag.m_count, 6);
        bag.SetAtGrow(bag.m_count, 7);
        bag.SetAtGrow(bag.m_count, 8);
        while (bag.m_count > 0) {
            i32 idx = GruntRand() % bag.m_count;
            i32 dir = bag.m_data[idx];
            moveX = x;
            moveY = y;
            switch (dir - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceS;
                    break;
                case 1:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNE;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceE;
                    break;
                case 3:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSE;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceN;
                    break;
                case 5:
                    moveX = x - 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSW;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceW;
                    break;
                case 7:
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNW;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            bag.RemoveAt(idx, 1);
            if (result != 0) {
                break;
            }
        }
    }
    if (result == 0) {
        return 0;
    }

commit:
    m_tileMgr->ApplyTileSwitch(this, m_lastTilePxX, m_lastTilePxY);
    PlaySound(0x3e8, voice);
    m_184 = m_lastTilePxX;
    m_188_tilePxY = m_lastTilePxY;
    {
        GruntBoard* b = g_gameReg->m_70;
        i32 ox = m_lastTilePxX >> 5;
        i32 oy = m_lastTilePxY >> 5;
        ((char*)b->m_8[oy])[ox * 7 * 4 + 3] &= 0xdf;
        *(i32*)&((char*)b->m_8[oy])[ox * 7 * 4 + 4] = -1;
    }
    {
        GruntBoard* b = g_gameReg->m_70;
        i32 nx = moveX >> 5;
        i32 ny = moveY >> 5;
        i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        ((char*)b->m_8[ny])[nx * 7 * 4 + 3] |= 0x20;
        *(i32*)&((char*)b->m_8[ny])[nx * 7 * 4 + 4] = owner;
    }
    m_lastTilePxX = moveX;
    m_lastTilePxY = moveY;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    *(i32*)((char*)this + 0x388) += 1;
    return 1;
}

// CGrunt::OnStruck(wasHit) @0x588f0 - the struck/damage reaction step. Re-arm the
// struck cooldown (m_270=0xfa0 window, m_268=game clock now), bump the struck
// counter (m_struckCount), and - if the grunt is on-screen (the registry
// visible-bounds rect at g->m_30->m_24->m_5c+0x40) - fire an escalating struck
// grunt-voice cue (CueA) keyed by whether it was a real hit and the running count.
// __thiscall, ret 4, frameless.
// @early-stop
// regalloc wall: logic/CFG/member-offsets/cue-ids byte-exact (the on-screen push
// blocks match verbatim); residue = retail pins `this` in esi (push esi; mov
// esi,ecx) for the whole 4-branch body, mine keeps it in ecx and reloads the cue
// sink per branch in a different slot - pure register placement, no source lever
// flips it. ~76%, deferred to the final sweep.
RVA(0x000588f0, 0x1ea)
void CGrunt::OnStruck(i32 wasHit) {
    m_struckTimerLo = 0xfa0;
    m_struckTimerHi = 0;
    m_struckClockLo = (i32)g_645588;
    m_struckClockHi = 0;
    i32 c = ++m_struckCount;

    if (wasHit == 0) {
        if (m_gruntKind == 0x36) {
            return;
        }
        i32 x = m_10->m_5c;
        i32 y = m_10->m_60;
        if (c < 5) {
            CGameRegistry* g = g_pGameRegistry;
            i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
            if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
                g->m_60->CueA(this, 0x370, -1, 0, -1, -1);
            }
            return;
        }
        CGameRegistry* g = g_pGameRegistry;
        i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_60->CueA(this, 0x371, -1, 0, -1, -1);
        } else {
            m_struckCount = 0;
        }
        return;
    }

    if (c < 5) {
        i32 x = m_10->m_5c;
        i32 y = m_10->m_60;
        CGameRegistry* g = g_pGameRegistry;
        i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_60->CueA(this, 0x320, -1, 0, -1, -1);
        }
        return;
    }
    if (c < 0xa) {
        i32 x = m_10->m_5c;
        i32 y = m_10->m_60;
        CGameRegistry* g = g_pGameRegistry;
        i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_60->CueA(this, 0x321, -1, 0, -1, -1);
        }
        return;
    }
    {
        i32 x = m_10->m_5c;
        i32 y = m_10->m_60;
        m_struckCount = 0;
        CGameRegistry* g = g_pGameRegistry;
        i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_60->CueA(this, 0x322, -1, 0, -1, -1);
        }
    }
}

// CGrunt::EnsureStruckSlot(key) @0x57b70 - the +0x424-slot sibling of
// EnsureStruckVoice (+0x428): lazily build + play a grunt sound sample for `key`.
// Bails if the +0x424 slot is already filled, or if the registry's m_10 gate is
// unset. Otherwise looks `key` up in the global sound table
// (g_gameReg->m_30->m_28->m_10), clones a sample from the entry's factory (GetItem),
// stores it into +0x424, and plays it on the registry sound channel (m_11c).
// __thiscall, ret 4. Same lookup shape as EnsureStruckVoice / CProjectile::LaunchSound.
//
// @early-stop
// reloc-naming scoring artifact (objdiff-reloc-scoring memory): the three engine
// callees - the sound-map Lookup (0x1b8438), the sample factory GetItem (0x135d70)
// and the sample Play (0x136300) - are not yet RVA-annotated, so their REL32
// operands pair to the target's FUN_ names and stay fuzzy until those engine fns
// get stubs (the SAME referent set EnsureStruckVoice waits on). g_gameReg IS named.
// Logic complete; flips to exact once that shared referent set is named.
RVA(0x00057b70, 0x77)
void CGrunt::EnsureStruckSlot(const char* key) {
    GruntSoundSample*& sample = *(GruntSoundSample**)((char*)this + 0x424);
    if (sample != 0) {
        return;
    }
    if (*(i32*)((char*)g_gameReg + 0x10) == 0) {
        return;
    }
    GruntSoundEntry* entry = 0;
    g_gameReg->m_30->m_28->m_10.Lookup(key, &entry);
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = entry->m_10->GetItem();
    if (sample == 0) {
        return;
    }
    sample->Play(g_gameReg->m_11c, 0, 0, 1);
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

// CGrunt::EnsureStruckVoice(key) @0x57c40 - lazily build + play the grunt's
// struck-voice sound sample. Bails if already created (the +0x428 slot ClearSubB
// frees). Looks `key` up in the global sound table (g_gameReg->m_30->m_28->m_10),
// clones a sample from the entry's factory (GetItem), stores it into +0x428, and
// plays it on the sound channel (g_gameReg->m_11c). __thiscall, ret 4. Same
// sound-lookup shape as CProjectile::LaunchSound (0xe2190).
//
// @early-stop
// reloc-naming scoring artifact (docs/patterns, objdiff-reloc-scoring memory):
// instruction stream byte-identical vs retail (verified llvm-objdump), 100% fuzzy.
// g_gameReg IS named (the two ds: relocs pair); the three engine callees - the
// sound-map Lookup (0x1b8438), the sample factory GetItem (0x135d70) and the
// sample Play (0x136300) - are not yet RVA-annotated, so their REL32 operands pair
// to the target's FUN_ names and stay fuzzy. Flips to exact once those engine
// functions get stubs (the SAME referent set LaunchSound waits on). Logic complete.
RVA(0x00057c40, 0x71)
void CGrunt::EnsureStruckVoice(const char* key) {
    GruntSoundSample*& sample = *(GruntSoundSample**)((char*)this + 0x428);
    if (sample != 0) {
        return;
    }
    GruntSoundEntry* entry = 0;
    g_gameReg->m_30->m_28->m_10.Lookup(key, &entry);
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = entry->m_10->GetItem();
    if (sample == 0) {
        return;
    }
    sample->Play(g_gameReg->m_11c, 0, 0, 1);
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
//   m_400 = (sqrt(dx*dx + dy*dy) / m_41c) * dt   (dx=m_lastTilePxX-m_5c, dy=m_lastTilePxY-m_60)
//   m_408 = (double)m_10->m_5c;  m_410 = (double)m_10->m_60
// dt is the per-tile time step; m_41c is the configured TimePerTile.
RVA(0x00057060, 0x6f)
void CGrunt::ComputeFacing(double dt) {
    CGruntHud* h = m_10;
    double dx = (double)m_lastTilePxX - (double)h->m_5c;
    double dy = (double)m_lastTilePxY - (double)h->m_60;
    *(double*)((char*)this + 0x400) = (sqrt(dx * dx + dy * dy) / (double)m_timePerTile) * dt;
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
// If already arrived (m_arrived) returns 1 immediately. Otherwise, if not yet
// claimed (m_tileClaimed==0): in alt-mode (registry m_134==2) it just notifies the tile
// owner; else it seeds the arrival defender block (m_308/m_310/.., m_tileClaimed, m_arrivalState,
// m_arrivalFlags &= mask) and claims the tile. Then runs the six per-arrival hooks and
// latches m_arrived=1.
RVA(0x0004b130, 0xc8)
i32 CGrunt::CommitArrival() {
    if (m_arrived != 0) {
        return 1;
    }
    if (m_tileClaimed != 0) {
        if (g_pGameRegistry->m_134 == 2) {
            m_tileMgr->NotifyArrival(m_tileOwnerHi, m_tileOwnerLo);
        } else if (m_tileClaimed != 0) {
            m_308 = 0;
            m_310 = 0;
            m_30c = 0;
            m_314 = 0;
            i32 flags = m_arrivalFlags & 0xe7fbfbfd;
            m_tileClaimed = 0;
            m_arrivalState = 0;
            m_arrivalFlags = flags;
            ArrivalClaim(1, 1);
        }
    }
    ArrivalHook0();
    ArrivalHook1();
    ArrivalHook2();
    ArrivalHook3();
    ArrivalHook4();
    ArrivalHook5();
    m_arrived = 1;
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
// its committed entrance position (m_174/m_178 = m_lastTilePxX/m_lastTilePxY), clears the
// arrival timers (m_210); if `a`, also clears m_450/m_230; and if `b` and the
// grunt is not a special kind (m_arrivalState!=0x11) it drains the name list at +0x320
// into the global free pool and resets the collection at +0x31c.
RVA(0x0004d060, 0x98)
void CGrunt::SetEntrancePos(i32 a, i32 b) {
    m_entrancePxX = m_lastTilePxX;
    m_entrancePxY = m_lastTilePxY;
    *(i32*)((char*)this + 0x210) = 0;
    if (a) {
        *(i32*)((char*)this + 0x450) = 0;
        m_230 = 0;
    }
    if (b && m_arrivalState != 0x11 && *(i32*)((char*)this + 0x328) != 0) {
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

// ---------------------------------------------------------------------------
// CGrunt::SerializeMove(ar, mode, a3, a4)   @0x53b80   (ret 0x10)
// The grunt move/idle-timer record (de)serializer. mode 4 = save (ar->Write,
// vtable slot +0x30), mode 7 = load (ar->Read, slot +0x2c). It chains the head
// anim-state serializer (0x16e7f0, external) and the +0x150/+0x43c sub-records,
// runs a mode-specific pre-step, then streams the eight 16-byte (double-pair)
// timer records at +0x810..+0x880 directly, and finishes through the six
// tail sub-record serializers (+0x890/+0x8a0/+0x8b0/+0x8c0/+0x308/+0x278). The
// per-record `if (mode==4) Write,Write; else if (mode==7) Read,Read;` inlines
// once per record (each half is 8 bytes: p, then p+8).
static __inline void SerRecord(CGruntArchive* ar, i32 mode, char* p) {
    switch (mode) {
        case 4:
            ar->Write(p, 8);
            ar->Write(p + 8, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
            break;
    }
}

RVA(0x00053b80, 0x340)
i32 CGrunt::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    if (SerializeAnimState(ar, mode, a3, a4) == 0) {
        return 0;
    }
    if (((CGruntSubSer*)((char*)this + 0x150))->Serialize(ar, mode, a3, a4) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (SerPreStep4(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (SerPreStep7(ar) == 0) {
                return 0;
            }
            break;
        case 8:
            m_tileMgr = (CGruntTileMgr*)g_gameReg->m_68;
            break;
    }
    ((CGruntSubSer*)((char*)this + 0x43c))->Serialize(ar, mode, a3, a4);
    SerRecord(ar, mode, (char*)this + 0x810);
    SerRecord(ar, mode, (char*)this + 0x820);
    SerRecord(ar, mode, (char*)this + 0x830);
    SerRecord(ar, mode, (char*)this + 0x840);
    SerRecord(ar, mode, (char*)this + 0x850);
    SerRecord(ar, mode, (char*)this + 0x860);
    SerRecord(ar, mode, (char*)this + 0x870);
    SerRecord(ar, mode, (char*)this + 0x880);
    ((CGruntSubSer*)((char*)this + 0x890))->Serialize(ar, mode, a3, a4);
    ((CGruntSubSer*)((char*)this + 0x8a0))->Serialize(ar, mode, a3, a4);
    ((CGruntSubSer*)((char*)this + 0x8b0))->Serialize(ar, mode, a3, a4);
    ((CGruntSubSer*)((char*)this + 0x8c0))->Serialize(ar, mode, a3, a4);
    ((CGruntSubSer*)((char*)this + 0x308))->Serialize(ar, mode, a3, a4);
    ((CGruntSubSer*)((char*)this + 0x278))->Serialize(ar, mode, a3, a4);
    return 1;
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
    strcpy(buf, (const char*)m_animSetName);
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
// the m_poseAttackIdle source, re-stamps the active anim-set node, and re-applies the
// per-cell entrance frame. Reads the active-anim descriptor's first element's
// frame (+0x14), looks the per-cell name up by the m_entranceCell {col,row} triple (cell
// stride 0x68 at +0x468), applies the frame, then latches a fresh idle anim-set
// node (g_entranceAnimSrc.LookupAnimSet) into m_14->m_1c. __thiscall, ret 0.
RVA(0x000616e0, 0xa8)
i32 CGrunt::ResetGeometry() {
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(*(i32*)((char*)this + 0x3a0));

    CEntranceAnimDescColl* desc = m_154->m_1b4;
    i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = elem[0x14 / 4];

    i32 col = m_entranceCell[0];
    i32 row = m_entranceCell[1];
    i32 index = 3 * col + row;
    const char* name = ((CGruntCell*)((char*)this + 0x468 + index * 0x68))->GetName(0);
    m_154->SetAnimFrame(name, frame);

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_animKeyA);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::CommitNeighbor(a, b, c, d)   @0x5b050   (__thiscall, ret 0x10)
// React to the grid-neighbour grunt at cell (a,b): gate on a not being self (or a
// global flag), the entrance-reason not being 0x13/0x14, and the committed tile
// (m_17c/m_180) being clear of the spawn-block bit; build the HUD health sprite,
// re-arm the combat-timer block (CombatTimeout config), then resolve the neighbour
// grunt from the tile-mgr's 15-wide cell grid (m_260 + (15a+b)*4 + 0x1c), gate it
// (live, both committed, not anim "F"); dispatch on m_170/m_19c (==1 -> a move
// config) else on the current anim type code ("I" -> arrival re-notify; "N" -> the
// align-down/drop-ready/snap re-latch); finally run the shared combat finalize:
// commit the in-flight move, latch m_220, build the neighbour's HUD health sprite +
// combat timer, recycle both arrival blocks, and (when not low-stamina/active)
// re-arm the attack anim. __thiscall, ret 0x10; returns 1 on success, 0 on bail.
//
// @early-stop
// large-state-machine + grid-regalloc plateau: the dispatch CFG, all three strcmp
// type-code arms (the inline-strcmp setcc form), the combat-timer + 15-wide cell grid
// index, the align-down/IsDropReady block, and the dual ArrivalRecycle finalize are
// reconstructed in shape/order. Residue = the deep g_gameReg->m_70 board chain modeled
// by raw offset, the cross-arm regalloc (the neighbour `ebx` pinned across the tail),
// and the strcmp-eq sentinel pinning. Deferred to the final sweep.
RVA(0x0005b050, 0x40b)
i32 CGrunt::CommitNeighbor(i32 a, i32 b, i32 c, i32 d) {
    if (a == m_tileOwnerHi && g_6455b0 == 0) {
        return 0;
    }
    i32 reason = m_entranceReason;
    if (reason == 0x14 || reason == 0x13) {
        return 0;
    }
    {
        GruntBoard* bd = g_gameReg->m_70;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if ((u32)tx >= (u32)bd->m_c || (u32)ty >= (u32)bd->m_10) {
            flags = 1;
        } else {
            flags = ((i32*)bd->m_8[ty])[tx * 7];
        }
        if (flags & 0x80) {
            return 0;
        }
    }

    CreateHealthSprite();
    *(i32*)((char*)this + 0x878) = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    *(i32*)((char*)this + 0x87c) = 0;
    *(i32*)((char*)this + 0x870) = (i32)g_645588;
    *(i32*)((char*)this + 0x874) = 0;
    *(i32*)((char*)this + 0x358) = 1;

    CGrunt* nb = (CGrunt*)(*(i32*)((char*)m_tileMgr + (15 * a + b) * 4 + 0x1c));
    if (nb == 0 || nb->m_entranceCommitted == 0 || m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeF) == 0);
    if (eq) {
        return 0;
    }
    i32 v = m_entranceReason;
    if (v > 0x16) {
        v = *(i32*)((char*)this + 0x19c);
    }
    if (v == 1) {
        RunMoveConfig(c >> 5, d >> 5);
        return 1;
    }

    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
    } else {
        eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeN) == 0);
        if (eq) {
            i32 px = (m_10->m_5c & ~0x1f) + 0x10;
            i32 py = (m_10->m_60 & ~0x1f) + 0x10;
            i32 redo = 1;
            if (px != m_lastTilePxX || py != m_lastTilePxY) {
                if (IsDropReady(1)) {
                    *(i32*)((char*)this + 0x234) = (*(i32*)((char*)this + 0x234) == 0);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                m_prevAnimSetNode = (i32)m_14->m_1c;
                m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
                OnCoordCommit(*(i32*)((char*)this + 0x234));
            }
        }
    }

    // The shared combat finalize.
    if (m_arrivalPending != 0) {
        m_tileMgr->CommitArrivalMove(this, m_10->m_5c, m_10->m_60);
        m_arrivalPending = 0;
    }
    m_poweredUp = 1;
    nb->CreateHealthSprite();
    *(i32*)((char*)nb + 0x878) = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    *(i32*)((char*)nb + 0x87c) = 0;
    *(i32*)((char*)nb + 0x870) = (i32)g_645588;
    *(i32*)((char*)nb + 0x874) = 0;
    ArrivalRecycle(c, d, 1, a, b);
    *(i32*)((char*)this + 0x200) = a;
    *(i32*)((char*)this + 0x204) = b;
    *(i32*)((char*)this + 0x208) = c;
    *(i32*)((char*)this + 0x20c) = d;
    if (m_stamina < 0x64 || m_entranceActive != 0) {
        *(i32*)((char*)this + 0x21c) = 1;
        return 1;
    }
    *(i32*)((char*)this + 0x21c) = 0;
    nb->ArrivalRecycle(m_10->m_5c, m_10->m_60, 0, m_tileOwnerHi, m_tileOwnerLo);
    RearmAttackAnim(a, b);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::FindGridNeighbor(int validate)   @0x5b6f0
// Resolves the grunt's stored grid-cell neighbour (the CGrunt* at
// m_tileMgr's 15-wide cell grid indexed by the latched coords m_neighborCol/m_neighborRow), validates
// it occupies the expected tile, runs the tile-rect predicate + commit, and
// returns it. __thiscall, ret 4. Frameless. On any miss it clears m_neighborValid and
// returns 0; a validate-mismatch returns 0 WITHOUT touching m_neighborValid.
RVA(0x0005b6f0, 0xb5)
CGrunt* CGrunt::FindGridNeighbor(i32 validate) {
    if (m_neighborCol == -1) {
        return 0;
    }
    if (m_neighborRow == -1) {
        return 0;
    }

    CGrunt* n = *(CGrunt**)((char*)m_tileMgr + (m_neighborRow + 15 * m_neighborCol) * 4 + 0x1c);
    if (n != 0 && n->m_entranceCommitted != 0) {
        if (validate != 0) {
            if (n->m_10->m_5c != n->m_lastTilePxX) {
                return 0;
            }
            if (n->m_10->m_60 != n->m_lastTilePxY) {
                return 0;
            }
        }
        if (RectContains(n->m_10->m_5c, n->m_10->m_60)) {
            CommitNeighbor(m_neighborCol, m_neighborRow, n->m_10->m_5c, n->m_10->m_60);
            return n;
        }
    }

    m_neighborValid = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RearmEntranceDrop() @0x68370 - re-arms the entrance "drop" geometry.
// Re-points the entrance player's geometry sub-player at the default source, and
// when the sub-player just became ready (m_1a0.m_28 set, m_1a0.m_20 clear) it
// re-inits geometry to the ITEM2 pose, re-applies the per-cell frame name, and
// arms the drop gate. Then, if the drop hasn't been latched (m_22c==0), it looks
// up the tile under the grunt's HUD point and either claims it (SetTile drop +
// owner) or marks the entrance committed (m_1fc=1). __thiscall, ret 0.
// @early-stop
// scheduling tail: logic/CFG/member-offsets/calls exact (same entrance cell-math
// `(3*col+row+0xb)*0x68`, SetGeoSourceR/SetGeometry/GetName/SetAnimFrame/LookupTile/
// SetTile all match). Residue = cl hoists the GetName(0) `push 0` + reuses the
// `m_154+0x1a0` address in a reg earlier than mine (same entropy-class scheduling
// as ResetGeometry @0x616e0) - no source lever flips it. ~88.5%.
RVA(0x00068370, 0x14c)
void CGrunt::RearmEntranceDrop() {
    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);

    if (*(i32*)((char*)m_154 + 0x1a0 + 0x28) != 0 && *(i32*)((char*)m_154 + 0x1a0 + 0x20) == 0) {
        m_22c = 0;
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseItem2);

        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];

        i32 col = m_entranceCell[0];
        i32 row = m_entranceCell[1];
        const char* name = ((CGruntCell*)((char*)this + (3 * col + row + 0xb) * 0x68))->GetName(0);
        m_154->SetAnimFrame(name, frame);
    }

    if (m_22c == 0) {
        i32 a;
        i32 b;
        m_entranceCommitted = 0;
        if (m_tileMgr->LookupTile(m_10->m_5c, m_10->m_60, &a, &b, 0) != 0) {
            m_tileMgr->SetTile(a, b, 0xb, -1);
            m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        } else {
            m_entranceCommitted = 1;
        }
    }
}

void CGrunt::ApplyMoveKind(i32 v) {} // thunk_0x3c29 (0x57100); external/reloc-masked
void CGrunt::RunMoveConfig(i32 a, i32 b) {} // thunk_0x2a40 (0x65630); external/reloc-masked

// ---------------------------------------------------------------------------
// CGrunt::UpdateEntranceAnim()   @0x690a0   (__thiscall, ret 0)
// The per-frame entrance-anim / arrival update step. Re-seeds the entrance
// player's geometry sub-player (m_154->m_1a0) and bails unless it is armed-but-not-
// running (m_28!=0 && m_20==0). On the FIRST pass (m_228==0) it stamps the
// TOY-BREAK pose geometry, applies the active descriptor's frame to the +0x448
// name CString, latches m_228=1, and kicks the move-kind apply (m_380 ? : m_37c).
// On a later pass (m_228!=0) it (when arrived) builds the three HUD stat sprites,
// re-latches the "A"(idle) anim-set node into m_14->m_1c, drives the move state
// (SetMoveStateA(m_19c,1,0,0)), clears m_entranceActive, then either - when the
// grunt's last tile carries the 0x80 attribute - commits the arrival move
// (SetEntrancePos(1,1); tileMgr->CommitArrivalMove(this, lastX, lastY)) or else
// bumps the HUD z-clamp (m_10->m_74 = m_60 + 0x186a0; m_8 |= 0x20000).
//
// @early-stop
// reloc-masked-extern plateau: CFG, every member offset/gate, the board index math
// (stride 7, attr bit 0x80), the z-clamp constant 0x186a0, and all call shapes are
// byte-faithful. Residue = the engine callees reached through incremental-link
// thunks (SetGeoSourceR/SetGeometry/GetBuffer/SetAnimFrame/LookupAnimSet, the
// CreateHealthSprite/Stamina/Toy creators, SetMoveStateA/SetEntrancePos/the apply
// + CommitArrivalMove thunks) are unnamed externals, so their `call rel32`
// displacements pair to differently-named retail thunks and score fuzzy. Naming
// that whole referent set is a final-sweep task.
RVA(0x000690a0, 0x1c5)
i32 CGrunt::UpdateEntranceAnim() {
    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);
    char* sub = (char*)m_154 + 0x1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return 0;
    }

    if (*(i32*)((char*)this + 0x228) == 0) {
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseToyBreak);

        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];

        char* buf = GruntStrGetBuffer((char*)this + 0x448, 0);
        m_154->SetAnimFrame(buf, frame);

        *(i32*)((char*)this + 0x228) = 1;
        i32 v = *(i32*)((char*)this + 0x380);
        if (v != 0) {
            ApplyMoveKind(v);
        } else {
            ApplyMoveKind(*(i32*)((char*)this + 0x37c));
        }
        return 0;
    }

    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeA);
    SetMoveStateA(*(i32*)((char*)this + 0x19c), 1, 0, 0);
    m_entranceActive = 0;

    WwdGameReg* g = g_gameReg;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    GruntBoard* board = g->m_70;
    i32 flags;
    if ((u32)tx >= (u32)board->m_c || (u32)ty >= (u32)board->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)board->m_8[ty])[tx * 7];
    }

    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
        return 0;
    }

    CGruntHud* h = m_10;
    i32 z = h->m_60 + 0x186a0;
    if (h->m_74 != z) {
        h->m_74 = z;
        h->m_8 |= 0x20000;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommit()   @0x692f0   (__thiscall, ret 0; always returns 0)
// The per-tick death/struck reaction + arrival-commit dispatch. Gated on the
// entrance being committed (m_1fc), it resolves the grunt's current anim-set name
// and dispatches on its single-letter type code (A/D -> finalize; I -> arrival
// re-notify; G/L/P -> idle reseed; O -> commit-move; J -> re-latch "D" + drive the
// move mode; N -> align-down/drop-ready snap; M -> tile set), then runs the shared
// finalize tail (arrival consider, clear the HUD stat sprites, latch the entrance
// reset, commit the tile, re-latch the "Q" anim set, and apply the DEATHZ_FREEZE
// geometry + first frame). __thiscall, ret 0.
//
// @early-stop
// big-state-machine fuzzy-desync wall (docs/patterns/big-seh-fuzzy-desync +
// zero-register-pinning): the 10-way type-code cascade (the inline-strcmp `bool eq`
// setcc form, both the GetNameRecord and scratch-teardown GetNameRecords variants),
// every dispatch arm, the m_1a0 mode sub-dispatch, the align-down/drop-ready block,
// and the DEATHZ_FREEZE finalize are reconstructed in shape/order; instruction
// multiset is close (mov 238/256, push 75/74, call 40/40 exact). Residue collapses
// the fuzzy% to ~2%: retail reserves a `sub esp,0xc` scratch frame (one dead cell[2]
// spill in the J-block) my frameless body omits, shifting the epilogue + every block;
// and retail tests the strcmp results with `test eax,eax` while my m_1fc==0 gate pins
// `xor ebx,ebx` at entry so all 10 arms emit `cmp eax,ebx`. Pure regalloc/frame
// (no source lever); the alignment desync rolls the faithful 0x850 carcass up to ~2%.
// Deferred to the final sweep.
RVA(0x000692f0, 0x850)
i32 CGrunt::StepArrivalCommit() {
    if (m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0);
    if (eq) {
        goto finalize;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (eq) {
        goto finalize;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_60->Cue1(m_10->m_188);
        }
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
        if (m_entranceReason != 1) {
            goto finalize;
        }
        m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        return 0;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeG) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeL) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeP) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeO) == 0);
    if (eq) {
        SnapToLastTile(1);
        m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
        goto finalize;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeJ) == 0);
    if (eq) {
        // code "J": clear the entrance gate; if the PREVIOUS anim set was "D",
        // re-latch a fresh "D" set + drive the WALK geometry + stamp the cell frame.
        m_entranceActive = 0;
        eq = (strcmp(*g_animNameResolver.GetNameRecord((void*)m_prevAnimSetNode), g_codeD) == 0);
        if (eq) {
            if (m_poweredUp == 0 && *(i32*)((char*)this + 0x21c) == 0) {
                m_entranceActive = 0;
                *(i32*)((char*)this + 0x218) = 0;
                *(i32*)((char*)this + 0x21c) = 0;
                m_poweredUp = 0;
                Stub_062e10(1, 0, 0);
            }
            *(i32*)((char*)this + 0x35c) = 0;
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseWalk);
            i32* cell = m_entranceCell;
            i32 base = 3 * cell[0] + cell[1];
            char* nm = GruntStrGetBuffer((char*)this + base * 0x68 + 0x470, 0);
            m_154->SetAnimName(nm);
        } else {
            Stub_062e10(1, 0, 0);
        }
        goto modeDispatch;
    }

    // default: the M / N reject codes.
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeN) == 0);
    if (eq) {
        i32 px = (m_10->m_5c & ~0x1f) + 0x10;
        i32 py = (m_10->m_60 & ~0x1f) + 0x10;
        i32 redo = 1;
        if (px != m_lastTilePxX || py != m_lastTilePxY) {
            if (IsDropReady(1)) {
                *(i32*)((char*)this + 0x234) = (*(i32*)((char*)this + 0x234) == 0);
                redo = 0;
            }
        }
        SnapToLastTile(1);
        if (redo) {
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
            OnCoordCommit(*(i32*)((char*)this + 0x234));
        }
        goto finalize;
    }
    {
        char* prev = g_animNameResolver.GetNameRecords(m_14->m_1c)->m_name;
        GruntScratchTeardown();
        eq = (strcmp(prev, g_codeM) == 0);
        if (eq) {
            m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
            return 0;
        }
        goto finalize;
    }

idleReseed:
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_60->Cue1(m_10->m_188);
    }
    SetMoveStateA(*(i32*)((char*)this + 0x19c), 1, 0, 0);
    {
        i32 z = m_10->m_60 + 0x186a0;
        if (m_10->m_74 != z) {
            m_10->m_74 = z;
            m_10->m_8 |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        ((CSpriteRegRecord*)m_toyTimeSprite)->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    ClearSubA();
    goto finalize;

modeDispatch: {
    i32 mode = *(i32*)((char*)this + 0x1a0);
    if (mode >= 0x32) {
        SetMoveStateA(mode, 1, 0, 1);
        *(i32*)((char*)this + 0x1a0) = -1;
        *(i32*)((char*)this + 0x1a4) = 0;
        goto finalize;
    }
    if (mode >= 0x22) {
        *(i32*)((char*)this + 0x194) = mode;
        *(i32*)((char*)this + 0x1a0) = -1;
        goto finalize;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        goto finalize;
    }
    SetMoveStateA(mode, 1, 0, 1);
    *(i32*)((char*)this + 0x1a0) = -1;
    goto finalize;
}

finalize:
    ConsiderArrival(1);
    if (m_healthSprite != 0) {
        m_healthSprite->m_8 |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        m_staminaSprite->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        m_toySprite->m_8 |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        m_wingzTimeSprite->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_poweredUp == 0 && *(i32*)((char*)this + 0x21c) == 0) {
        m_entranceActive = 0;
        *(i32*)((char*)this + 0x218) = 0;
        *(i32*)((char*)this + 0x21c) = 0;
        m_poweredUp = 0;
        Stub_062e10(1, 0, 0);
    }
    m_entranceActive = 1;
    m_tileMgr->CommitStruckTile(m_tileOwnerHi, m_tileOwnerLo, 1);
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeQ);
    {
        i32 z = m_10->m_60 + 0x186a0;
        if (m_10->m_74 != z) {
            m_10->m_74 = z;
            m_10->m_8 |= 0x20000;
        }
    }
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_FREEZE, 0);
    {
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];
        m_154->SetAnimFrame(s_GRUNTZ_DEATHZ_FREEZE, frame);
    }
    *(i32*)((char*)this + 0x23c) = 1;
    *(i32*)((char*)this + 0x240) = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveArrivalNeighbor() @0xf26f0 - the per-frame arrival follow-up,
// active only while the grunt is mid-arrival (m_2d4==2). When powered-up
// (m_poweredUp) it re-resolves the stored grid neighbour once stamina is full;
// otherwise it clears the arrival latch, looks up the grunt currently occupying
// its tile (m_tileMgr->GetOccupant), and - if that occupant is settled on its own
// tile and on-screen (RectContains) - commits a neighbour link to it. __thiscall,
// ret 0, always returns 1.
// @early-stop
// regalloc wall: logic/CFG/cue-paths exact, the m_2d4 dispatch is the retail
// switch subtract-chain (sub 0/sub 2, see switch-subtract-chain-vs-ifelse). Residue
// = retail pins `this` in edi + occupant in esi (mine flips them) and pre-stages
// the CommitNeighbor stack args (sub esp,8; redundant [esp+8]/[esp+1c] copies) -
// pure register/scheduling placement, no source lever flips it. ~75%.
RVA(0x000f26f0, 0x106)
i32 CGrunt::ResolveArrivalNeighbor() {
    switch (m_2d4) {
        case 0:
            return 1;
        case 2:
            break;
        default:
            return 1;
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            return 1;
        }
        if (*(i32*)((char*)this + 0x218) != 0) {
            return 1;
        }
        if (m_stamina < 0x64) {
            return 1;
        }
        FindGridNeighbor(1);
        return 1;
    }

    m_2d4 = 0;
    CGrunt* occ = m_tileMgr->GetOccupant(this);
    if (occ == 0) {
        return 1;
    }
    if (m_poweredUp != 0) {
        return 1;
    }
    if (m_stamina < 0x64) {
        return 1;
    }
    if (RectContains(occ->m_10->m_5c, occ->m_10->m_60) == 0) {
        return 1;
    }
    if (m_10->m_5c != occ->m_lastTilePxX) {
        return 1;
    }
    if (m_10->m_60 != occ->m_lastTilePxY) {
        return 1;
    }
    CommitNeighbor(occ->m_tileOwnerHi, occ->m_tileOwnerLo, occ->m_lastTilePxX, occ->m_lastTilePxY);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::UpdateGruntStatus()   @0x617c0
// The per-frame grunt status step. If not powered-up (m_poweredUp==0) it just runs the
// entrance reset and bails. Otherwise it re-arms the entrance geometry source and,
// gated on the grunt's stamina (m_stamina): when full (>=100) it resolves + commits its
// grid-cell neighbour (same 15-wide m_tileMgr grid as FindGridNeighbor); when low
// (0x33..0x63, latched once via m_lowStaminaCued) and on-screen, it fires a spawn cue.
// __thiscall, ret 0, frameless.
// @early-stop
// lazy callee-saved-reg save: instruction MULTISET byte-identical vs retail
// (verified), logic/CFG/offsets exact; residue = retail defers `push edi` until
// AFTER the m_poweredUp==0 early-bail (the cold path uses only esi) where cl saves edi
// in the prolog. Pure regalloc placement, not steerable from source. ~94.5%.
RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_poweredUp == 0) {
        Stub_062e10(1, 0, 0);
        return 0;
    }

    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);

    if (m_stamina >= 0x64) {
        if (m_neighborValid == 0) {
            return 0;
        }
        m_neighborValid = 0;
        CGrunt* n = *(CGrunt**)((char*)m_tileMgr + (m_neighborRow + 15 * m_neighborCol) * 4 + 0x1c);
        if (n == 0 || n->m_entranceCommitted == 0) {
            return 0;
        }
        if (RectContains(n->m_10->m_5c, n->m_10->m_60)) {
            CommitNeighbor(m_neighborCol, m_neighborRow, n->m_10->m_5c, n->m_10->m_60);
        }
        return 0;
    }

    if (m_stamina <= 0x32) {
        return 0;
    }
    if (m_lowStaminaCued != 0) {
        return 0;
    }

    CGameRegistry* g = g_pGameRegistry;
    i32 x = m_10->m_5c;
    i32 y = m_10->m_60;
    i32* vr = (i32*)(g->m_30->m_24->m_5c + 0x40);
    if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
        g->m_60->CueSpawn(this, 2, -1, -1, -1);
    }
    m_lowStaminaCued = 1;
    return 0;
}

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_animNameResolver.GetNameRecord(m_14->m_1c), or the scratch-teardown
// GetNameRecords form) and dispatches on its single-letter type code
// (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's movement/arrival state, recycling
// its occupied-coord nodes onto the shared freelist, and re-latching m_14->m_1c to
// a new anim set via g_entranceAnimSrc.LookupAnimSet. The inline-strcmp `== bool` setcc
// reject form is per docs/patterns/strcmp-eq-bool-local-setcc.md.
//
// These are the CGrunt analogues of UnknownClassArrays::Method_025d90 /
// Method_02f620 (the documented large-state-machine + grid-regalloc walls). Each is
// reconstructed complete in shape/order; all carry @early-stop on those walls.
// Raw-offset member access (the campaign style used by the cluster above) keeps the
// giant ~0x46c layout tractable.

// A grunt board-tile flag fetch (g_gameReg->m_70 board, tile = row[y][x*7]); the
// out-of-bounds path returns 1 (so any flag test passes). Shared by all five.
static i32 GruntTileFlags(i32 tx, i32 ty) {
    GruntBoard* b = g_gameReg->m_70;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        return 1;
    }
    return ((i32*)b->m_8[ty])[tx * 7];
}

// Recycle a grunt's occupied-coord list onto the shared freelist, then empty the
// CObList in place. Head = unit+0x320, count gate = unit+0x328.
static void GruntRecycleCoords(CGrunt* g) {
    GruntCoordNode* n = *(GruntCoordNode**)((char*)g + 0x320);
    while (n != 0) {
        GruntCoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != 0) {
            void** node = (void**)((char*)cur->m_coord - g_gruntFreeListBias);
            *node = g_gruntFreeList;
            g_gruntFreeList = node;
        }
    }
    ((CObList*)((char*)g + 0x31c))->RemoveAll();
}

// The scratch CString teardown the GetNameRecords reject paths run (Release each
// non-null slot, g_animScratchCount times). The shared loop-strength-reduction
// wall (docs/patterns; cl `mov edi,count` vs retail `lea edi,[eax+1]`).
static void GruntScratchTeardown() {
    CAnimScratchString* slot = g_animScratch;
    i32 cnt = g_animScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalDrop(a,b,c,d,e,f)   @0x4b370   (ret 0x18, /GX EH frame)
// @early-stop
// large-state-machine + /GX EH-state plateau: the dispatch cascade + the coord-node
// freelist recycle + the grid re-stamp are reconstructed in shape; residue is the
// EH try-level numbering across the cell CString temp, the grid/board chains modeled
// by raw offset, and the cross-scan regalloc. Deferred to the final sweep.
RVA(0x0004b370, 0xafd)
void CGrunt::StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    m_arrivalNotified = 0; // m_464 cleared on entry
    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (!eq && a == m_entrancePxX && b == m_entrancePxY) {
        goto reachedTarget;
    }
    // Recycle the occupied-coord nodes onto the CoordPool, empty the list, then
    // probe the destination tile via the engine pathfinder (0x20f4) and either
    // re-anchor (within range) or fall through to the big arrival commit.
    if (*(i32*)((char*)this + 0x328) != 0) {
        GruntCoordNode* n = *(GruntCoordNode**)((char*)this + 0x320);
        while (n != 0) {
            GruntCoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Recycle(cur->m_coord);
            }
        }
        ((CObList*)((char*)this + 0x31c))->RemoveAll();
    }
    StepDropApply();
    return;

reachedTarget:
    m_tileMgr->SetTileState4(a, b, c, d);
}

// ---------------------------------------------------------------------------
// CGrunt::StepGruntMovement()   @0x4c170   (ret 0)
// @early-stop
// large-state-machine + grid-regalloc plateau: the target-tile gate, the compass
// move-voice record selection (the 8 g_voice* records reused from PlayMoveSound),
// the freelist recycle, and the move-cue dispatch are reconstructed in shape/order;
// residue is the deep grid/board chains (g_gameReg->m_70->...) modeled by raw offset
// + the cross-scan regalloc across the three coord fetches. Final sweep.
RVA(0x0004c170, 0xbe7)
i32 CGrunt::StepGruntMovement() {
    // Reached the target tile? (last committed tile == cached tile).
    if (m_entrancePxX == m_lastTilePxX && m_entrancePxY == m_lastTilePxY) {
        SetMoveStateB(1, 1, 0, 0, 0);
        return 0;
    }
    // Drop-ready (state 0x11)? recycle the coord nodes if so.
    if (m_arrivalState == 0x11) {
        char* rec = (char*)g_gameReg + m_tileOwnerHi * 0x238 + 0x188;
        if (rec != 0 && IsDropReady() == 0) {
            SetMoveStateA(1, 1, 0, 0);
            return 0;
        }
    }
    if (*(i32*)((char*)this + 0x328) != 0) {
        GruntRecycleCoords(this);
    }
    CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
    SetMoveStateB(1, 1, 0, 0, 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepAnimDispatchA(x, y, c, d)   @0x52fb0   (ret 0x10)
// @early-stop
// large-state-machine plateau: the 12-way single-letter type-code cascade (the
// inline-strcmp `bool eq` setcc form, both the GetNameRecord and scratch-teardown
// GetNameRecords variants), every dispatch arm, the m_1a0 mode sub-dispatch, the
// coord recycle, and the LookupAnimSet re-latch are reconstructed in shape/order.
// Residue: the scratch loop-strength-reduction (shared with Method_02f620, no source
// spelling) + the deep grid/board chains by raw offset + cross-arm regalloc.
// Deferred to the final sweep.
RVA(0x00052fb0, 0x96e)
i32 CGrunt::StepAnimDispatchA(i32 x, i32 y, i32 c, i32 d) {
    if (m_entranceCommitted == 0) {
        return 1;
    }
    i32 flags = GruntTileFlags(x, y);
    if ((flags & 0xd39) || (flags & 0x82)) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        // code "I": arrival cue (m_170==0x13) then re-notify the tile mgr.
        if (m_entranceReason == 0x13) {
            EmitMoveCueShort(m_10->m_188, 0, 0);
        }
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
        if (m_entranceReason != 1) {
            goto applyTail;
        }
        m_tileMgr->SetTileState4(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        goto applyTail;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeG) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeL) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeP) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeO) == 0);
    if (eq) {
        // code "O": commit the move directly.
        ApplySetState1(1);
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        goto applyTail;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeQ) == 0);
    if (eq) {
        return 1;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeJ) == 0);
    if (eq) {
        // code "J": clear the entrance gate, re-latch a fresh anim set, drive the
        // geometry sub-player.
        m_entranceActive = 0;
        if (m_poweredUp == 0 && m_neighborValid == 0) {
            m_entranceCommitted = 0;
            ReseedIdleReset(1, 0, 0);
        }
        *(i32*)((char*)this + 0x35c) = 0;
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        // Stamp the first entrance-cell frame from the m_474 cell table.
        i32* cell = m_entranceCell;
        i32 col = cell[1] + cell[0] * 2;
        i32 row = (cell[0] + col) * 3;
        i32 idx = (cell[0] + col) + row * 4;
        const char* nm = ((CGruntCell*)((char*)this + idx * 8 + 0x470))->GetName(cell[2]);
        m_154->SetAnimFrame(nm, 0);
        goto modeDispatch;
    } else {
        ApplySetState1(1);
        goto modeDispatch;
    }

idleReseed:
    // codes G/L/P: drive the move state by m_19c and (m_170==0x1e) fire the cue.
    if (m_entranceReason == 0x1e) {
        EmitMoveCueShort(m_10->m_188, 0, 0);
    }
    SetMoveStateA(*(i32*)((char*)this + 0x19c), 1, 0, 1);
    {
        i32 px = m_10->m_60 + 0x186a0;
        if (m_10->m_74 != px) {
            m_10->m_74 = px;
            m_10->m_8 |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        ((CSpriteRegRecord*)m_toyTimeSprite)->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StepCoordTick();

applyTail:
    // The shared movement-apply tail: re-set the geometry, recycle coords.
    if (m_wingzEnabled != 0) {
        OnMoveFinishA(0);
    }
    if (m_poweredUp == 0 && m_neighborValid == 0) {
        m_entranceCommitted = 0;
        ReseedIdleReset(1, 0, 0);
    }
    StepDropApply();
    return 1;

modeDispatch: {
    i32 mode = *(i32*)((char*)this + 0x1a0);
    if (mode >= 0x32) {
        SetMoveStateA(mode, 1, 0, 1);
        *(i32*)((char*)this + 0x1a0) = -1;
        *(i32*)((char*)this + 0x1a4) = 0;
        return 1;
    }
    if (mode >= 0x22) {
        *(i32*)((char*)this + 0x194) = mode;
        *(i32*)((char*)this + 0x1a0) = -1;
        return 1;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        return 1;
    }
    SetMoveStateA(mode, 1, 0, 1);
    *(i32*)((char*)this + 0x1a0) = -1;
    return 1;
}
}

// ---------------------------------------------------------------------------
// CGrunt::StepCoordResolve()   @0x5f310   (ret 0)
// @early-stop
// large-state-machine plateau: the coord-probe head (claim the head coord's tile if
// free, else retry within the m_384 budget) and the scratch-resolver D-code reject
// cascade (the inline-strcmp `bool eq` setcc + the scratch CString teardown) are
// reconstructed in shape. Residue is the scratch loop-strength-reduction (shared, no
// source spelling), the deep grid/board chains by raw offset, and cross-arm
// regalloc. Deferred to the final sweep.
RVA(0x0005f310, 0xb5e)
void CGrunt::StepCoordResolve() {
    if (m_arrivalState != 0x11) {
        bool eq;
        eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0);
        if (eq && *(i32*)((char*)this + 0x328) != 0) {
            GruntCoordNode* head = *(GruntCoordNode**)((char*)this + 0x320);
            GruntCoord* co = head->m_coord;
            i32 fl = ((i32*)g_gameReg->m_70->m_8[co->m_y])[co->m_x * 7];
            i32 mask = m_arrivalFlags & fl;
            if (!(fl & 0x20000000) && !(mask & 0x20000000)
                && (mask == 0 || (m_arrivalNotified & fl) != 0)) {
                m_entrancePxX = (co->m_x << 5) + 0x10;
                m_entrancePxY = (co->m_y << 5) + 0x10;
                *(i32*)((char*)this + 0x384) = 0;
                NotifyDrop();
            } else if (*(i32*)((char*)this + 0x384) <= 5) {
                if (ProbeRetry() != 0) {
                    GruntCoord* h2 = (*(GruntCoordNode**)((char*)this + 0x320))->m_coord;
                    m_entrancePxX = (h2->m_x << 5) + 0x10;
                    m_entrancePxY = (h2->m_y << 5) + 0x10;
                    if (*(i32*)((char*)this + 0x328) != 0) {
                        GruntCoord* h3 = (*(GruntCoordNode**)((char*)this + 0x320))->m_coord;
                        i32 fl2 = ((i32*)g_gameReg->m_70->m_8[h3->m_y])[h3->m_x * 7];
                        if (!(fl2 & 0x20000000)) {
                            *(i32*)((char*)this + 0x384) = 0;
                            NotifyDrop();
                        }
                    }
                } else {
                    (*(i32*)((char*)this + 0x384))++;
                }
            }
        }
    }
    // The scratch-resolver D-code reject cascade (each via GetNameRecords + the
    // scratch CString teardown).
    GruntScratchTeardown();
    bool eq2;
    eq2 = (strcmp(g_animNameResolver.GetNameRecords(m_14->m_1c)->m_name, g_codeD) == 0);
    (void)eq2;
    GruntScratchTeardown();
    OnMoveFinishA(0);
}

// ---------------------------------------------------------------------------
// CGrunt::StepAnimDispatchB()   @0x6a6d0   (ret 0)
// @early-stop
// large-state-machine + zero-register-pinning plateau: the 12-way type-code cascade,
// the m_1a0 mode sub-dispatch, the K arrival arm, the coord recycle, and the
// LookupAnimSet re-latch are reconstructed in shape/order. Residue: retail pins the
// strcmp sentinels 0/-1 in callee-saved ebx/ebp
// (docs/patterns/zero-register-pinning.md), the scratch loop-strength-reduction, the
// grid/board raw-offset chains, and cross-arm regalloc. Deferred to the final sweep.
RVA(0x0006a6d0, 0x936)
i32 CGrunt::StepAnimDispatchB() {
    bool eq;
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0);
    if (eq) {
        goto kArm;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0);
    if (eq) {
        goto kArm;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            EmitMoveCueShort(m_10->m_188, 0, 0);
        }
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
        return 1;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeG) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeL) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeP) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeO) == 0);
    if (eq) {
        ApplySetState1(1);
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        return 1;
    }
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeJ) == 0);
    if (eq) {
        m_entranceActive = 0;
        if (m_poweredUp == 0 && m_neighborValid == 0) {
            m_entranceCommitted = 0;
            ReseedIdleReset(1, 0, 0);
        }
        *(i32*)((char*)this + 0x35c) = 0;
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        i32* cell = m_entranceCell;
        i32 col = cell[1] + cell[0] * 2;
        i32 row = (cell[0] + col) * 3;
        i32 idx = (cell[0] + col) + row * 4;
        const char* nm = ((CGruntCell*)((char*)this + idx * 8 + 0x470))->GetName(cell[2]);
        m_154->SetAnimFrame(nm, 0);
        goto modeDispatch;
    } else {
        ApplySetState1(1);
        goto modeDispatch;
    }

idleReseed:
    SetMoveStateA(*(i32*)((char*)this + 0x19c), 1, 0, 1);
    goto modeDispatch;

modeDispatch: {
    i32 mode = *(i32*)((char*)this + 0x1a0);
    if (mode >= 0x32) {
        SetMoveStateA(mode, 1, 0, 1);
        *(i32*)((char*)this + 0x1a0) = -1;
        *(i32*)((char*)this + 0x1a4) = 0;
        return 1;
    }
    if (mode >= 0x22) {
        *(i32*)((char*)this + 0x194) = mode;
        *(i32*)((char*)this + 0x1a0) = -1;
        return 1;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        return 1;
    }
    SetMoveStateA(mode, 1, 0, 1);
    *(i32*)((char*)this + 0x1a0) = -1;
    return 1;
}

kArm:
    // code "K": the arrival arm - re-anchor + re-stamp the grid cell.
    eq = (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeK) == 0);
    if (eq && m_entranceArmed != 0) {
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        StepDropApply();
    }
    return 1;
}

// ===========================================================================
// The arrival/update dispatch trio (ex-CUserLogic_* stubs @0x59230 / 0x5caa0 /
// 0x62110). RTTI/this-layout IDENTIFY them as CGrunt methods, NOT CUserLogic:
// every member offset they touch (m_arrivalState 0x2d0, m_2f0/m_2f4, m_arrivalPhase
// 0x450, m_tileMgr 0x260, m_tileOwnerHi/Lo 0x1ec/0x1f0, m_14, m_154, the +0x470
// entrance-cell record table, the +0x4b0 dir-vector table, m_health 0x3ec, ...) is
// in the CGrunt layout above, and they call the same CGrunt this-method thunks the
// rest of this TU does. The "CUserLogic" stub attribution was a mislabel (CUserLogic
// is a small vptr+link base; it has no 0x2d0/0x450/0x4b0 fields - the same mislabel a
// prior matcher found for other CGrunt stubs, RTTI vtable 0x5e8754 = .?AVCGrunt@@).
// ===========================================================================

// The "ToyTime" bute key the update step reads (reloc-masked .rodata @0x60e194).
static char s_ToyTime[] = "ToyTime";

// ---------------------------------------------------------------------------
// CGrunt::ArrivalRecycle(a, b, mode, d, e)   @0x59230   (__thiscall, ret 0x14)
// mode==0: latch the pending arrival target (a switch on m_arrivalState seeds
// m_2f0/m_2f4 from {d,e} and, for the in-flight states, marks m_2d4=2), then - when
// committing (m_arrivalPhase 2/3, m_230 set) - commit the occupied tile slot to its
// settled HUD position (RectContains[Gated]). mode!=0: drive the move-sound then run
// the occupied-coord recycle: for each resolver reject code "H"/"F"/"O", resolve the
// cell record (the resolver's coord->index map) and bail; final miss -> ResetGeometry().
//
// @early-stop
// large-state-machine + custom-resolver-internals plateau (sibling of RunEntranceMove
// 0x67850 / StepEntranceReinit 0x637a0): CFG, the switch jump-table mapping, every
// member offset/gate, the 15-stride tile index, the RectContains-gated commit, and the
// three sequential strcmp-reject cell-resolves are reconstructed in shape/order. Residue:
// the resolver coord-range field reads + the two distinct cell-record fallback helpers
// reloc-mask to differently-named externals, the inline-strcmp setcc sentinel pinning,
// and the cross-block regalloc on the shared resolve tail. Deferred to the final sweep.
RVA(0x00059230, 0x40d)
i32 CGrunt::ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e) {
    if (mode == 0) {
        switch (m_arrivalState) {
        case 2:
            m_2f0 = d;
            m_2f4 = e;
            break;
        case 1:
        case 4:
            m_2f0 = d;
            m_2f4 = e;
            m_2d4 = 2;
            break;
        case 5:
            m_2f0 = d;
            m_2f4 = e;
            m_2d4 = 2;
            break;
        case 3:
        case 6:
            m_2f0 = d;
            m_2f4 = e;
            m_2d4 = 2;
            break;
        case 0x11:
            m_2f0 = d;
            m_2f4 = e;
            break;
        default:
            break;
        }

        i32 phase = m_arrivalPhase;
        if ((phase == 3 || phase == 2) && m_230 != 0) {
            i32 idx = 15 * m_2f0 + m_2f4;
            void* obj = *(void**)((char*)m_tileMgr + idx * 4 + 0x1c);
            if (obj != 0) {
                CGruntHud* inner = *(CGruntHud**)((char*)obj + 0x10);
                i32 yMasked = (inner->m_60 & ~0x1f) + 0x10;
                i32 xMasked = (inner->m_5c & ~0x1f) + 0x10;
                i32 hit;
                if (phase == 3) {
                    hit = RectContains(xMasked, yMasked);
                } else {
                    hit = RectContainsGated(xMasked, yMasked);
                }
                if (hit != 0) {
                    OnReanchor(0);
                }
                if (phase == 3) {
                    m_tileMgr->CommitTileSlot(m_tileOwnerHi, m_tileOwnerLo, inner->m_5c, inner->m_60);
                } else {
                    m_tileMgr->CommitTileSlot2(m_tileOwnerHi, m_tileOwnerLo, inner->m_5c, inner->m_60);
                }
            }
        }
        return 1;
    }

    PlayMoveSound(a, b);

    // Occupied-coord recycle: three sequential resolver reject codes. Each block
    // resolves the current anim-set node's cell record (the resolver's coord-range
    // map; the bounds hit is the fast path, the two fallbacks are engine helpers).
    char* nm0 = *g_animNameResolver.GetNameRecord(m_14->m_1c);
    if (strcmp(nm0, g_codeH) == 0) {
        return 1;
    }
    {
        i32 coord = (i32)m_14->m_1c;
        g_animScratchCount = 0;
        i32 rec;
        if (coord < g_cellLo || coord > g_cellHi) {
            if (g_animNameResolver.MapCellIndex(coord, 0) != 0) {
                rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
            } else {
                g_animNameResolver.MapCellRecord(g_cellRecordBase, 0xc);
                rec = g_cellRet;
            }
        } else {
            rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
        }
        GruntScratchTeardown();
        (void)rec;
    }
    char* nm1 = *g_animNameResolver.GetNameRecord(m_14->m_1c);
    if (strcmp(nm1, g_codeF) == 0) {
        return 1;
    }
    {
        i32 coord = (i32)m_14->m_1c;
        g_animScratchCount = 0;
        i32 rec;
        if (coord < g_cellLo || coord > g_cellHi) {
            if (g_animNameResolver.MapCellIndex(coord, 0) != 0) {
                rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
            } else {
                i32 pin = g_animNameResolver.PinCellIndex();
                g_cellRecordRet = g_animNameResolver.MapCellRecord2(g_cellRecordBase, 0xc);
                rec = g_cellRet;
                (void)pin;
            }
        } else {
            rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
        }
        GruntScratchTeardown();
        (void)rec;
    }
    char* nm2 = *g_animNameResolver.GetNameRecord(m_14->m_1c);
    if (strcmp(nm2, g_codeO) == 0) {
        return 1;
    }
    ResetGeometry();
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::InitDirVectors()   @0x5caa0   (__thiscall, ret 0)
// The grunt reset/spawn-init step. Fills the per-direction velocity-vector table at
// this+0x4b0 (9 directions, each a 0x78-stride record, 4 doubles/record) from the 9
// runtime direction-index globals (0x644aa0..0x644b48; index = 3*dir[0] + dir[1]) with
// the unit/diagonal direction vectors (0, +-1.0, +-0.5, +-sqrt(2)/2), then resets the
// grunt's spawn state: HUD anchor, health/stamina (100), the entrance flags, the latches.
//
// @early-stop
// x87 FP instruction-scheduling wall (same family as ComputeFacing 0x57060): the integer
// stores, the 9 unrolled direction records, the 3*lo+hi index math, and the trailing
// stat/flag reset are reconstructed faithfully, but MSVC's fld/fst/fstp/fdivr/fdiv stack
// juggling for sqrt(2.0) and the 1/sqrt2 diagonals (the `fld st(1)` scheduling) rarely
// matches from C source. Deferred to the final sweep.
RVA(0x0005caa0, 0x5e4)
void CGrunt::InitDirVectors() {
    double diag = sqrt(g_dirConst2);              // sqrt(2.0)
    double* tbl = (double*)((char*)this + 0x4b0); // 0x78-stride records (15 doubles each)
    const i32 W = 0x78 / 8;                       // 15 doubles per record

    double s = g_dirConst1 / diag;  // 1 / sqrt2
    double n = g_dirConstN1 / s;    // -1 / (1/sqrt2)

    // Each record: 4 doubles at the cell's +0/8/0x10/0x18. The 9 globals are processed
    // in this fixed order (ab0,ae0,aa0,b28,ac0,b48,ad0,b18,b38).
    {
        i32 i = W * (3 * g_dirAb0[0] + g_dirAb0[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = -1.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirAe0[0] + g_dirAe0[1]);
        tbl[i + 0] = s;
        tbl[i + 1] = s;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirAa0[0] + g_dirAa0[1]);
        tbl[i + 0] = 1.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = 0.0;
    }
    {
        i32 i = W * (3 * g_dirB28[0] + g_dirB28[1]);
        tbl[i + 0] = s;
        tbl[i + 1] = s;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirAc0[0] + g_dirAc0[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = 1.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirB48[0] + g_dirB48[1]);
        tbl[i + 0] = n;
        tbl[i + 1] = s;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirAd0[0] + g_dirAd0[1]);
        tbl[i + 0] = -1.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = 0.0;
    }
    {
        i32 i = W * (3 * g_dirB18[0] + g_dirB18[1]);
        tbl[i + 0] = n;
        tbl[i + 1] = n;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirB38[0] + g_dirB38[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = 0.0;
    }

    // --- spawn-state reset tail (integer field stores) ---
    CGruntHud* h = m_10;
    i32 px = h->m_5c;
    *(i32*)((char*)this + 0x184) = px;
    *(i32*)((char*)this + 0x17c) = px;
    *(i32*)((char*)this + 0x174) = px;
    i32 py = h->m_60;
    *(i32*)((char*)this + 0x188) = py;
    *(i32*)((char*)this + 0x180) = py;
    *(i32*)((char*)this + 0x178) = py;
    *(i32*)((char*)this + 0x1dc) = 0;
    *(i32*)((char*)this + 0x1e0) = 0;
    m_health = 0x64;
    m_stamina = 0x64;
    m_toyTime = 0;
    m_wingzTime = 0;
    m_entranceActive = 0;
    *(i32*)((char*)this + 0x1e8) = 0;
    m_arrivalState = 0;
    m_poweredUp = 0;
    m_resetApplied = 0;
    m_arrivalFlags = 0x4000901;
    *(i32*)((char*)this + 0x24c) = 0;
    *(i32*)((char*)this + 0x368) = 0;
    m_tileClaimed = 0;
}

// ---------------------------------------------------------------------------
// CGrunt::UpdateArrival(a1, a2)   @0x62110   (__thiscall, ret 0x8)
// The per-frame arrival/entrance update step (sibling of UpdateEntranceAnim 0x690a0 and
// RunEntranceMove 0x67850). a2!=0 -> the commit pass (clear the coord sub, commit the
// in-flight occupied tile slot, reset the entrance latches, recycle the occupied-coord
// list onto the free pool + RemoveAll, OR 0x10000 into the toy/health sprite flag words,
// then either re-latch a "P" anim set + roll a rand toy pose + fire the cue, or load the
// ToyTime config + snapshot the clock). a1!=0 -> the "L" re-latch + walk geometry + cell
// SetAnimName + halved-ToyTime timer. a1==0 -> the "G" re-latch + HUD z-clamp + toy-timer
// pose select + the visible-bounds CueSpawn.
//
// @early-stop
// large-state-machine + reloc-masked-extern plateau (sibling of 0x690a0/0x637a0): CFG, the
// two-flag dispatch, every member offset/gate, the 15-stride tile index, the board attr
// chains, the rand()%N pose rolls, the 64-bit toy-timer compare/select, the +0x810/0x820
// timer snapshots, and all cue/anim call shapes are byte-faithful. Residue: the engine
// callees reached via incremental-link thunks reloc-mask to differently-named retail thunks,
// plus the cross-arm regalloc / zero-register pinning. Deferred to the final sweep.
RVA(0x00062110, 0x5bc)
i32 CGrunt::UpdateArrival(i32 a1, i32 a2) {
    if (a2 != 0) {
        ClearSubA();
        if (m_arrivalPhase == 3 && m_230 != 0) {
            i32 idx = 15 * m_2f0 + m_2f4;
            void* obj = *(void**)((char*)m_tileMgr + idx * 4 + 0x1c);
            if (obj != 0) {
                CGruntHud* inner = *(CGruntHud**)((char*)obj + 0x10);
                i32 yMasked = (inner->m_60 & ~0x1f) + 0x10;
                i32 xMasked = (inner->m_5c & ~0x1f) + 0x10;
                if (RectContainsGated(xMasked, yMasked) != 0) {
                    m_tileMgr->CommitTileSlot(m_tileOwnerHi, m_tileOwnerLo, inner->m_5c, inner->m_60);
                }
            }
        }

        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            *(i32*)((char*)this + 0x218) = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_entranceActive = 1;
        SetEntrancePos(1, 1);

        // Recycle the occupied-coord list (+0x320) onto the free pool, then RemoveAll.
        if (*(i32*)((char*)this + 0x328) != 0) {
            void** node = *(void***)((char*)this + 0x320);
            while (node != 0) {
                void* next = node[0];
                void* buf = node[2];
                if (buf != 0) {
                    void** sp = (void**)((char*)buf - g_freePoolBase);
                    *sp = g_freePoolHead;
                    g_freePoolHead = sp;
                }
                node = (void**)next;
            }
            ((CGruntColl*)((char*)this + 0x31c))->Reset();
        }

        *(i32*)((char*)this + 0x228) = 0;
        if (m_healthSprite != 0) {
            m_healthSprite->m_8 |= 0x10000;
            m_healthSprite = 0;
        }
        if (m_toySprite != 0) {
            m_toySprite->m_8 |= 0x10000;
            m_toySprite = 0;
        }

        if (m_entranceReason == 0x1e) {
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeP);
            i32 toyIdx = rand() % 2;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(*(i32*)((char*)this + toyIdx * 4 + 0x3c4), 0);

            CEntranceAnimDescColl* desc = m_154->m_1b4;
            i32* el = desc->m_10 > 0 ? *desc->m_c : 0;
            i32 frame = el[0x14 / 4];
            char* buf = GruntStrGetBuffer((char*)this + 0x448, 0);
            m_154->SetAnimFrame(buf, frame);

            i32 cueTier = ((toyIdx != 0) ? 0xa : 0) + 0x406;
            WwdGameReg* g = g_gameReg;
            i32 m380 = *(i32*)((char*)this + 0x380);
            if (m380 != 0) {
                i32 tier = cueTier + m380 - 1;
                i32 anchor = *(i32*)(*(char**)((char*)g->m_30 + 0x24) + 0x5c) + 0x40;
                if (GruntPointVisible(m_10->m_60, m_10->m_5c, anchor) != 0) {
                    g->m_60->CueA(this, tier, 0, -1, -1, -1);
                }
            } else {
                if (*(i32*)((char*)this + 0x37c) == 0) {
                    i32 md = (g->m_134 == 1) ? 3 : 6;
                    *(i32*)((char*)this + 0x37c) = rand() % md + 1;
                }
                i32 tier = cueTier + *(i32*)((char*)this + 0x37c) - 1;
                i32 anchor = *(i32*)(*(char**)((char*)g->m_30 + 0x24) + 0x5c) + 0x40;
                if (GruntPointVisible(m_10->m_60, m_10->m_5c, anchor) != 0) {
                    g->m_60->CueA(this, tier, 0, -1, -1, -1);
                }
            }
            return 0;
        } else {
            DWORD tt = g_buteMgr.GetDword(*(char**)((char*)this + 0x1c0), s_ToyTime);
            *(i32*)((char*)this + 0x818) = (i32)tt;
            *(i32*)((char*)this + 0x81c) = 0;
            *(i32*)((char*)this + 0x810) = (i32)g_645588;
            *(i32*)((char*)this + 0x814) = 0;
            m_toyTime = 0x64;
            CreateToyTimeSprite();
        }
    }

    if (a1 != 0) {
        // a1 != 0: the "L" re-latch + walk geometry + cell SetAnimName + halved-ToyTime timer.
        *(i32*)((char*)this + 0x388) = 0;
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            *(i32*)((char*)this + 0x218) = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_prevAnimSetNode = (i32)m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeL);
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        i32* cell = m_entranceCell;
        i32 colv = cell[1] + cell[0] * 2;
        i32 basev = cell[0] + colv;
        i32 idxv = basev + basev * 12;
        char* nm = GruntStrGetBuffer((char*)this + idxv * 8 + 0x470, 0);
        m_154->SetAnimName(nm);

        DWORD tt = g_buteMgr.GetDword(*(char**)((char*)this + 0x1c0), s_ToyTime);
        *(i32*)((char*)this + 0x828) = (i32)(tt >> 1);
        *(i32*)((char*)this + 0x82c) = 0;
        *(i32*)((char*)this + 0x820) = (i32)g_645588;
        *(i32*)((char*)this + 0x824) = 0;
        return 0;
    }

    // a1 == 0: the "G" re-latch + HUD z-clamp + toy-timer pose select + visible-bounds cue.
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeG);

    CGruntHud* h = m_10;
    i32 z = h->m_60 + 0xc3500;
    if (h->m_74 != z) {
        h->m_74 = z;
        h->m_8 |= 0x20000;
    }

    // Pick the active toy pose by comparing the two toy-pose timers (m_3c4/m_3c8 ->+0x24)
    // against the elapsed toy timer (m_810/m_814 - clock), then re-stamp on change.
    i32 t0 = *(i32*)(*(char**)((char*)this + 0x3c4) + 0x24);
    i32 t1 = *(i32*)(*(char**)((char*)this + 0x3c8) + 0x24);
    i64 elapsed = *(i64*)((char*)this + 0x810) - (i64)(u32)g_645588;
    i32 cap = (i32)elapsed;
    if (elapsed < 0) {
        cap = 0;
    }
    i32 d0 = (t0 > cap) ? (t0 - cap) : 0;
    i32 d1 = (t1 > cap) ? (t1 - cap) : 0;
    i32 sel;
    if (d0 != 0) {
        sel = (d1 != 0) ? ((d0 < d1) ? 0 : 1) : 0;
    } else if (d1 != 0) {
        sel = 1;
    } else {
        i32 r = rand() % 0x64 + 1;
        sel = (r >= m_toyBlendPct) ? 1 : 0;
    }

    CEntranceAnimDescColl* cur = m_154->m_1b4;
    i32 want = *(i32*)((char*)this + sel * 4 + 0x3c4);
    if ((i32)cur != want) {
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(want);
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* el = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = el[0x14 / 4];
        char* buf = GruntStrGetBuffer((char*)this + 0x448, 0);
        m_154->SetAnimFrame(buf, frame);
    }

    // The visible-bounds cue: probe the grunt's HUD point against the live view rect,
    // fire CueSpawn(this, 0xa|0xb, -1,-1,-1) when inside.
    CGruntHud* hud = m_10;
    WwdGameReg* g = g_gameReg;
    i32 yy = hud->m_60;
    i32 xx = hud->m_5c;
    i32* rectBase = (i32*)(*(char**)((char*)g->m_30 + 0x24) + 0x5c);
    i32 lim = rectBase[0x48 / 4];
    i32* rect = (i32*)((char*)rectBase + 0x40);
    if (sel != 0) {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_60->CueSpawn(this, 0xb, -1, -1, -1);
        }
    } else {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_60->CueSpawn(this, 0xa, -1, -1, -1);
        }
    }
    return 0;
}

// ===========================================================================
// Chunk-2 attributed targets (RearmAttack family + entrance-move tail). Same
// raw-offset campaign style; reconstructed in ascending retail-RVA order.
// ===========================================================================
// (s_CombatTimeout is defined near the top of this TU; shared with CommitNeighbor.)

// ---------------------------------------------------------------------------
// CGrunt::BeginAttack(a, b)  @0x5b570  (__thiscall, ret 8)
// Gated on the entrance being committed (m_1fc != 0), the current anim NOT being
// the "F"/struck code, and m_stamina >= 0x64. Fires the directional move-sound to
// (a, b), latches the powered-up / +0x218 combat state, builds the HUD health
// sprite, latches the combat-timer block (CombatTimeout config + game clock), and
// re-arms the ATTACK2 anim (RearmAttackAnim2). Returns 1 on commit, else 0.
//
// @early-stop
// inline-strcmp result-register coin-flip (docs/patterns/return-bool-via-local-setcc):
// CFG, every member offset/gate, the GetNameRecords+scratch-teardown, the inline
// CRT strcmp, the PlayMoveSound/CreateHealthSprite/GetDwordDef call shapes, the
// combat-timer block, and both returns are byte-faithful. Residue = retail lands the
// inline-strcmp result + the sete bool in eax/cl where cl picks ecx/al, cascading the
// register pairing through the bool-eval block; source-invariant (named `eq` local
// already in place). Deferred to the final sweep.
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 a, i32 b) {
    if (*(i32*)((char*)this + 0x1fc) == 0) {
        return 0;
    }
    char* nm = g_animNameResolver.GetNameRecords(m_14->m_1c)->m_name;
    GruntScratchTeardown();
    bool eq = (strcmp(nm, g_codeF) == 0);
    if (eq) {
        return 0;
    }
    if (m_stamina < 0x64) {
        return 0;
    }

    PlayMoveSound(a, b);
    m_poweredUp = 1;
    *(i32*)((char*)this + 0x218) = 1;
    CreateHealthSprite();

    *(i32*)((char*)this + 0x878) = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    *(i32*)((char*)this + 0x87c) = 0;
    *(i32*)((char*)this + 0x870) = (i32)g_645588;
    *(i32*)((char*)this + 0x874) = 0;
    *(i32*)((char*)this + 0x358) = 1;
    *(i32*)((char*)this + 0x208) = a;
    *(i32*)((char*)this + 0x20c) = b;
    RearmAttackAnim2();
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::RearmAttackAnim(col, row)  @0x61940  (__thiscall, ret 8)
// Gated on m_entranceReason < 0x17. Latches the neighbor col/row (+0x200/+0x204),
// re-latches the "F" anim set, then switches on (m_entranceReason - 2) to pick the
// branch: reason 2 sets m_entranceActive when m_arrivalState is live; reasons
// 9/10/11/17/20/21/22 force index 1; else a rand-bit picks index 0/1. Sets the
// +0x218 combat latch, latches the combat-timer block, fires the focused-grunt drop
// cue when the grunt is on-screen, marks the HUD anim id dirty, drives the
// ATTACK1/ATTACK2 geometry by index, and re-stamps the entrance-cell frame.
//
// @early-stop
// switch/regalloc + cell-frame scratch-spill plateau: CFG, the (m_entranceReason-2)
// switch (the byte+jump table reloc-masks), every member offset/gate, the
// CreateHealthSprite/GetDwordDef/CueSpawn/SetGeometry/SetAnimFrame call shapes, the
// combat-timer block, the on-screen cue gate, and the HUD-dirty stamp are
// byte-faithful. Residue = the switch index register (ecx vs edx), the rand()%2 mask
// (and eax,1 vs the CSE'd ebx=1), the dead cell[2] read retail spills into a scratch
// frame, and the cross-arm regalloc; source-invariant. Deferred to the final sweep.
RVA(0x00061940, 0x1cf)
i32 CGrunt::RearmAttackAnim(i32 col, i32 row) {
    if (m_entranceReason >= 0x17) {
        return 0;
    }

    m_neighborCol = col;
    m_neighborRow = row;
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeF);

    *(i32*)((char*)this + 0x218) = 1;

    i32 idx;
    switch (m_entranceReason - 2) {
        case 0:
            if (m_arrivalState != 0) {
                m_entranceActive = 1;
            }
            idx = 1;
            break;
        case 7:
        case 8:
        case 9:
        case 15:
        case 18:
        case 19:
        case 20:
            idx = 1;
            break;
        default:
            idx = GruntRand() % 2;
            break;
    }

    CreateHealthSprite();

    *(i32*)((char*)this + 0x878) = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    *(i32*)((char*)this + 0x87c) = 0;
    *(i32*)((char*)this + 0x870) = (i32)g_645588;
    *(i32*)((char*)this + 0x874) = 0;

    {
        CGruntHud* h = m_10;
        WwdGameReg* g = g_gameReg;
        i32 yy = h->m_60;
        i32 xx = h->m_5c;
        i32* rect = (i32*)(*(i32*)(*(char**)((char*)g->m_30 + 0x24) + 0x5c) + 0x40);
        if (xx < rect[2] && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_60->CueSpawn(this, 1, -1, -1, -1);
        }
    }

    {
        CGruntHud* h = m_10;
        i32 z = h->m_60 + 0x186c1;
        if (h->m_74 != z) {
            h->m_74 = z;
            h->m_8 |= 0x20000;
        }
    }

    CEntranceAnimPlayer* p = m_154;
    m_prevEntranceDesc = (i32)p->m_1b4;
    p->m_1a0.SetGeometry(*(i32*)((char*)this + idx * 4 + 0x398));

    CEntranceAnimDescColl* desc = m_154->m_1b4;
    i32* el = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = el[0x14 / 4];

    i32* cell = m_entranceCell;
    i32 cc = cell[0];
    i32 cr = cell[1];
    i32 base = cc + (cr + 2 * cc);
    i32 idx2 = base + base * 12;
    char* buf = GruntStrGetBuffer((char*)this + idx2 * 8 + 0x468, 0);
    m_154->SetAnimFrame(buf, frame);
    m_neighborValid = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RearmAttackAnim2()  @0x61bc0  (__thiscall, ret 0)
// The simple ATTACK2 re-arm: re-latch the "F" anim set, drive the m_poseAttack2
// geometry, re-stamp the entrance-cell frame, set the +0x214 latch. Returns 0.
//
// @early-stop
// cell-frame scratch-spill plateau: CFG, every member offset, the EntranceLookupAnimSet
// re-latch, the m_poseAttack2 geometry drive, the first-elem frame read, the
// (3*col+row)*0x68 cell-frame index, and the SetAnimFrame call are byte-faithful.
// Residue = retail loads the cell as a 3-int read and spills the dead cell[2] to a
// 0xc scratch frame (sub esp,0xc) where cl strips the unused read (no frame), plus
// the cell-base register; source-invariant. Deferred to the final sweep.
RVA(0x00061bc0, 0xb2)
i32 CGrunt::RearmAttackAnim2() {
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeF);

    CEntranceAnimPlayer* p = m_154;
    m_prevEntranceDesc = (i32)p->m_1b4;
    p->m_1a0.SetGeometry(m_poseAttack2);

    CEntranceAnimDescColl* desc = m_154->m_1b4;
    i32* el = desc->m_10 > 0 ? *desc->m_c : 0;
    i32 frame = el[0x14 / 4];

    i32* cell = m_entranceCell;
    i32 col = cell[0];
    i32 row = cell[1];
    i32 base = col + (row + 2 * col);
    i32 idx2 = base + base * 12;
    char* buf = GruntStrGetBuffer((char*)this + idx2 * 8 + 0x468, 0);
    m_154->SetAnimFrame(buf, frame);
    m_neighborValid = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt_SegBoxOverlap(p, e1, e2)  @0x62b70  (__stdcall, ret 0xc)
// Does the directed segment e1->e2 cross into the axis-aligned box `p`
// {x0,y0,x1,y1}? Tests the segment against each box edge (top/bottom horizontals
// at y0/y1, left/right verticals at x0/x1): when the two endpoints straddle the
// edge line, interpolate the crossing coordinate in float and test it falls within
// the opposite span. Returns 1 on the first crossing, else 0. Pure stack args.
//
// @early-stop
// x87 FP instruction-scheduling wall (same family as ComputeFacing 0x57060 /
// docs/patterns): CFG, the 4 edge tests, every straddle setl/setg sign test, the
// int subtractions feeding fild/fidiv/fimul/fiadd, and the fcompp comparison
// structure are byte-faithful in shape/order. Residue = the x87 register-stack
// scheduling + which spill slot holds each interpolation operand (source-invariant
// on this leaf). Deferred to the final sweep.
RVA(0x00062b70, 0x205)
i32 __stdcall CGrunt_SegBoxOverlap(GruntBox* p, GruntSegEnd* e1, GruntSegEnd* e2) {
    i32 e1y = e1->m_4;
    i32 e2y = e2->m_4;

    // Top edge (y = p->m_4): straddle in y, interpolate x.
    i32 py = p->m_4;
    if ((e1y < py) != (e2y < py)) {
        float t = (float)(py - e1y) / (float)(e2y - e1y);
        float ix = (float)e1->m_0 + t * (float)(e2->m_0 - e1->m_0);
        if ((float)p->m_0 <= ix && ix <= (float)p->m_8) {
            return 1;
        }
    }

    // Bottom edge (y = p->m_c).
    i32 pyc = p->m_c;
    if ((e1y < pyc) != (e2y < pyc)) {
        float t = (float)(pyc - e1y) / (float)(e2y - e1y);
        float ix = (float)e1->m_0 + t * (float)(e2->m_0 - e1->m_0);
        if ((float)p->m_0 <= ix && ix <= (float)p->m_8) {
            return 1;
        }
    }

    // Left edge (x = p->m_0): straddle in x, interpolate y.
    i32 e1x = e1->m_0;
    i32 e2x = e2->m_0;
    i32 px = p->m_0;
    if ((e1x > px) != (e2x > px)) {
        float t = (float)(e2x - px) / (float)(e2x - e1x);
        float iy = (float)e1y + t * (float)(e2y - e1y);
        if ((float)p->m_4 <= iy && iy <= (float)p->m_c) {
            return 1;
        }
    }

    // Right edge (x = p->m_8).
    i32 pxr = p->m_8;
    if ((e1x > pxr) != (e2x > pxr)) {
        float t = (float)(e2x - pxr) / (float)(e2x - e1x);
        float iy = (float)e1y + t * (float)(e2y - e1y);
        if ((float)p->m_4 <= iy && iy <= (float)p->m_c) {
            return 1;
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::GruntInRadius(col, row)  @0x67b00  (__thiscall, ret 8)
// Resolve the grunt occupying cell (col, row) via the tile-mgr's 15-wide cell grid
// (m_tileMgr + (15*col + row)*4 + 0x1c), gate it (live, entrance committed m_1fc, not
// state 0x36), then test whether the squared tile-distance from this grunt's HUD
// tile to it is within the (this->m_298 + that->m_2dc)^2 radius-sum threshold.
//
// @early-stop
// load-result register coin-flip (docs/patterns/select-zero-mask-dest-register family):
// the base disasm is byte-identical to retail (CFG, all 3 gates, the 15*col+row index,
// the >>5 tile math, the dx*dx+dy*dy, the abs() cdq/xor/sub idiom, the radius-sum
// compare/setl) EXCEPT the resolved `other` pointer lands in edx where retail reuses
// the index register eax, cascading the edx/eax pairing through every following
// operand. Source-invariant (reorder / typed-grid / inline all keep edx). Deferred.
RVA(0x00067b00, 0x92)
i32 CGrunt::GruntInRadius(i32 col, i32 row) {
    CGrunt* other =
        *(CGrunt**)(*(char**)((char*)this + 0x260) + (15 * col + row) * 4 + 0x1c);
    if (other == 0) {
        return 0;
    }
    if (*(i32*)((char*)other + 0x1fc) == 0) {
        return 0;
    }
    if (*(i32*)((char*)other + 0x258) == 0x36) {
        return 0;
    }

    i32 ox = *(i32*)((char*)other + 0x17c) >> 5;
    i32 oy = *(i32*)((char*)other + 0x180) >> 5;
    i32 tx = *(i32*)((char*)this + 0x300) >> 5;
    i32 ty = *(i32*)((char*)this + 0x304) >> 5;
    i32 dx = oy - ty;
    i32 dy = ox - tx;
    i32 sum = *(i32*)((char*)this + 0x2dc) + *(i32*)((char*)this + 0x298);
    i32 dist2 = abs(dx * dx + dy * dy);
    return dist2 < sum * sum ? 1 : 0;
}

// ---------------------------------------------------------------------------
// CGrunt::LoadFreezeSpellAssets()  @0x69d60  (__thiscall, ret 0)
// The freeze-spell entrance-anim finalize step. Arm the geometry source; when the
// sub-player is armed-but-not-running (sub+0x28 != 0 && sub+0x20 == 0):
//   * if the +0x240 "finalized" latch is set, clear the entrance, re-init the anim
//     name table, reseed the idle reset, and (if the last tile carries the high
//     occupancy bit) commit the arrival move - then return.
//   * else (+0x240 clear) stamp the DEATHZ_SPARKLE finalize geometry, seed the
//     freeze-delay idle window (Spellz/FreezeDelay bute, default 0x2710) anchored
//     at the game clock, and clear the +0x23c latch.
// Then, when +0x23c is clear and the idle-delay window has elapsed, stamp the
// DEATHZ_UNFREEZE geometry, fire the on-screen 6-arg entrance cue (0x35c) when the
// grunt's HUD point is in view, and set the +0x240/+0x23c latches. Returns 0.
// @early-stop
// callee-save rematerialization wall (docs/patterns/shrink-wrapped-callee-save-push):
// retail reloads m_lastTilePxX/Y for CommitArrivalMove, so the tile-read uses 4 regs
// (no ebp push); cl caches them in a callee-saved reg, pushing ebp and shifting the
// 64-bit idle-timer compare's regalloc. Body byte-exact apart from that one-register
// cascade (~87.4%). Logic complete; deferred to the final sweep.
RVA(0x00069d60, 0x1e1)
i32 CGrunt::LoadFreezeSpellAssets() {
    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) != 0 && *(i32*)(sub + 0x20) == 0) {
        if (*(i32*)((char*)this + 0x240) != 0) {
            m_entranceActive = 0;
            Stub_048400();
            Stub_048470(0, 0);
            LoadAnimNameTable(0, 0);
            Stub_062e10(1, 0, 0);
            if (s_TileFlags(g_gameReg->m_70, m_lastTilePxX >> 5, m_lastTilePxY >> 5) & 0x80) {
                m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
            }
            return 0;
        }
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->GameApplyLookupGeometry(s_GRUNTZ_DEATHZ_SPARKLE, 0);
        m_idleDelayLo = g_buteMgr.GetIntDef(s_Spellz, s_FreezeDelay, 0x2710);
        m_idleDelayHi = 0;
        m_idleAnchorLo = (i32)g_645588;
        m_idleAnchorHi = 0;
        *(i32*)((char*)this + 0x23c) = 0;
    }
    if (*(i32*)((char*)this + 0x23c) == 0) {
        if ((i64)(u32)g_645588 - *(i64*)&m_idleAnchorLo >= *(i64*)&m_idleDelayLo) {
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->GameApplyLookupGeometry(s_GRUNTZ_DEATHZ_UNFREEZE, 0);
            CGruntHud* h = m_10;
            i32 vx = h->m_5c;
            i32 vy = h->m_60;
            char* sc = *(char**)((char*)g_gameReg->m_30 + 0x24);
            i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
            if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                g_gameReg->m_60->CueA(this, 0x35c, -1, 0, -1, -1);
            }
            *(i32*)((char*)this + 0x240) = 1;
            *(i32*)((char*)this + 0x23c) = 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::FinishEntranceMove()  @0x69fd0  (__thiscall, ret 0)
// Arm the entrance geometry source (m_154->m_1a0.SetGeoSourceR(g_defaultGeo)); when
// the geometry sub-player is armed-but-not-running (sub+0x28 != 0 && sub+0x20 == 0):
// unless m_36c is already set, notify the tile-mgr of the drop, then retire the
// entrance player (m_154->m_8 |= 0x10000). Returns 0.
RVA(0x00069fd0, 0x69)
i32 CGrunt::FinishEntranceMove() {
    m_154->m_1a0.SetGeoSourceR(g_defaultGeo);
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return 0;
    }
    if (*(i32*)((char*)this + 0x36c) == 0) {
        m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    *(i32*)((char*)m_154 + 8) |= 0x10000;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepCombatReaction(...)   @0x646b0   (ret 0x20, 8 stack args)
// The combat-reaction anim-dispatch state machine (proximity-MISATTRIBUTED to
// CUserLogic; really CGrunt - see the header note). Gated on the entrance being
// committed (m_1fc) and idle (m_364==0), it clamps the HUD scroll, resolves the
// grunt's current anim name via g_animNameResolver and dispatches on its single-
// letter type code (A/D/I/G/L/P/O/Q/J/N), driving the grunt's combat/arrival
// bookkeeping; then the shared tail re-arms the combat-timeout window, forwards the
// 8 args down, resolves the "F"/"O" scratch codes, drives the attack-pose geometry +
// entrance-cell frame, and fires the focused-grunt spawn cue when on-screen.
//
// @early-stop
// large anim-dispatch state-machine plateau (the same family as StepEntranceReinit /
// RunEntranceMove in this TU): the +0x1fc/+0x364 gate, the HUD-scroll clamp, the 10
// inline-strcmp dispatch arms + their state transitions, the m_1a0 move-mode switch,
// the combat-timeout re-arm, the 8-arg forward, the two scratch-resolver (GetNameRecords
// + scratch CString teardown) re-latches, the cell-frame restamp and the on-screen
// spawn-cue gate are all reconstructed in shape/order. Residue is the shared
// strcmp-eq setcc/zero-register pinning (no source spelling), the scratch loop-
// strength-reduction, and the deep cross-arm regalloc. Final sweep.
RVA(0x000646b0, 0x9c8)
i32 CGrunt::StepCombatReaction(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    if (m_entranceCommitted == 0 || m_364 != 0) {
        return 0;
    }
    {
        CGruntHud* h = m_10;
        i32 v = h->m_60 + 0x186a0;
        if (h->m_74 != v) {
            h->m_74 = v;
            h->m_8 |= 0x20000;
        }
    }

    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeA) == 0) {
        goto tail;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeD) == 0) {
        goto tail;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeI) == 0) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_60->Cue1(m_10->m_188);
        }
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            *(i32*)((char*)this + 0x3e4),
            *(i32*)((char*)this + 0x3e8),
            m_entranceReason,
            -1
        );
        goto tail;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeG) == 0) {
        goto reject;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeL) == 0) {
        goto reject;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeP) == 0) {
        goto reject;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeO) == 0) {
        ApplySetState1(1);
        m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
        goto tail;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeQ) == 0) {
        m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 6, a2);
        return 0;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeJ) == 0) {
        m_entranceActive = 0;
        if (strcmp(*g_animNameResolver.GetNameRecord((void*)m_prevAnimSetNode), g_codeD) == 0) {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                *(i32*)((char*)this + 0x218) = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ReseedIdleReset(1, 0, 0);
            }
            *(i32*)((char*)this + 0x35c) = 0;
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseWalk);
            i32* cell = m_entranceCell;
            i32 col = cell[1] + cell[0] * 2;
            i32 base = cell[0] + col;
            i32 row = base * 3;
            i32 idx = base + row * 4;
            char* cn = GruntStrGetBuffer((char*)this + idx * 8 + 0x470, 0);
            m_154->GameApplyName(cn);
        } else {
            ReseedIdleReset(1, 0, 0);
        }
        i32 mode = *(i32*)((char*)this + 0x1a0);
        if (mode >= 0x32) {
            SetMoveStateA(mode, 1, 0, 1);
            *(i32*)((char*)this + 0x1a0) = -1;
            *(i32*)((char*)this + 0x1a4) = 0;
        } else if (mode >= 0x22) {
            *(i32*)((char*)this + 0x194) = mode;
            *(i32*)((char*)this + 0x1a0) = -1;
        } else if (mode >= 0x17) {
            EmitMoveCueQ(mode);
        } else {
            SetMoveStateA(mode, 1, 0, 1);
            *(i32*)((char*)this + 0x1a0) = -1;
        }
        goto tail;
    }
    if (strcmp(*g_animNameResolver.GetNameRecord(m_14->m_1c), g_codeN) == 0) {
        CGruntHud* h = m_10;
        i32 hx = (h->m_5c & ~0x1f) + 0x10;
        i32 hy = (h->m_60 & ~0x1f) + 0x10;
        i32 flag = 1;
        if (hx != m_lastTilePxX || hy != m_lastTilePxY) {
            if (IsDropReady(1)) {
                *(i32*)((char*)this + 0x234) = (*(i32*)((char*)this + 0x234) == 0) ? 1 : 0;
                flag = 0;
            }
        }
        ApplySetState1(1);
        if (flag != 0) {
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
            OnCoordCommit(*(i32*)((char*)this + 0x234));
        }
    }
    goto tail;

reject:
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_60->Cue1(m_10->m_188);
    }
    SetMoveStateA(*(i32*)((char*)this + 0x19c), 1, 0, 1);
    {
        CGruntHud* h = m_10;
        i32 v = h->m_60 + 0x186a0;
        if (h->m_74 != v) {
            h->m_74 = v;
            h->m_8 |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StepCoordTick();

tail:
    UpdateCombatTimer();
    *(i32*)((char*)this + 0x878) = g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
    *(i32*)((char*)this + 0x87c) = 0;
    *(i32*)((char*)this + 0x870) = (i32)g_645588;
    *(i32*)((char*)this + 0x874) = 0;
    if (m_10->m_5c != m_lastTilePxX || m_10->m_60 != m_lastTilePxY) {
        OnTileMismatch(1);
    }
    if (ForwardCombatStep(a0, a1, a2, a3, a4, a5, a6, a7) == 0) {
        return 0;
    }

    {
        CAnimNameRecord* rec = g_animNameResolver.GetNameRecords(m_14->m_1c);
        GruntScratchTeardown();
        if (strcmp(rec->m_name, g_codeF) == 0) {
            if (m_entranceCommitted != 0) {
                return 0;
            }
        }
    }
    m_entranceActive = 1;
    {
        CAnimNameRecord* rec = g_animNameResolver.GetNameRecords(m_14->m_1c);
        GruntScratchTeardown();
        if (strcmp(rec->m_name, g_codeO) != 0) {
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeH);
            void* cellObj = ((void**)((char*)m_tileMgr + 0x1c))[a3 + a2 * 15];
            if (cellObj != 0) {
                CGruntHud* oh = ((CGrunt*)cellObj)->m_10;
                i32 cx = oh->m_5c;
                i32 cy = oh->m_60;
                if (*(i32*)((char*)this + 0x358) != 0 && m_entranceCommitted != 0 &&
                    IsInCombatRange(cx, cy)) {
                    if (!(s_TileFlags(g_gameReg->m_70, m_lastTilePxX >> 5, m_lastTilePxY >> 5) & 0x80)) {
                        CommitCombatMove(a2, a3, cx, cy);
                    }
                }
            }
        }
    }

    *(i32*)((char*)this + 0x218) = 0;
    i32 pose = *(i32*)((char*)this + a1 * 4 + 0x3a4);
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(pose);
    i32 frame;
    {
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem;
        if (desc->m_10 > 0) {
            elem = desc->m_c[0];
        } else {
            elem = 0;
        }
        frame = *(i32*)((char*)elem + 0x14);
    }
    {
        i32* cell = m_entranceCell;
        i32 col = cell[1] + cell[0] * 2;
        i32 base = cell[0] + col;
        i32 row = base * 3;
        i32 idx = base + row * 4;
        char* cn = GruntStrGetBuffer((char*)this + idx * 8 + 0x46c, 0);
        m_154->GameApplyLookupSprite(cn, frame);
    }
    {
        CGruntHud* h = m_10;
        i32 vx = h->m_5c;
        i32 vy = h->m_60;
        char* sc = *(char**)((char*)g_gameReg->m_30 + 0x24);
        i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
            g_gameReg->m_60->CueSpawn(this, 7, -1, -1, -1);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveArrivalReposition()   @0xec670   (__thiscall, ret 0 -> 1)
// The per-tick arrival-reposition step. Latch the defender position to the last
// tile; if the tile occupant is in radius and the +0x2ec dwell exceeds 0xfa, try to
// tile-switch onto it and commit its slot - on a -1 commit (slot not yet free) fire
// the on-screen entrance cue (0x366) when in view. With no in-radius occupant, once
// the +0x2ec dwell window passes the thresholds and the (m_308..m_314) idle timer has
// elapsed, reset that timer to a fresh rand%0x7530; otherwise re-roll a random target
// inside the HUD scroll region (m_134..m_140) and tile-switch onto it, escalating to
// SetEntrancePos when the spread exceeds m_328. Returns 1.
// @early-stop
// idiv/rand + abs + shared-tail-zero-reg + 64-bit-sbb-timer plateau: the occupant
// resolve, the in-radius/dwell gates, the random-region re-roll (two rand()%span via
// abs spans), the max-spread SetEntrancePos escalation, the idle-timer reset, and the
// structure-1 on-screen cue are all reconstructed in shape/order. Residue is the
// MSVC /O2 idiv scheduling + the ebx zero-register tail sharing. Final sweep.
RVA(0x000ec670, 0x298)
i32 CGrunt::ResolveArrivalReposition() {
    CGrunt* occ = m_tileMgr->GetOccupant(this);
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    if (occ != 0 && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
        if ((u32) * (u32*)((char*)this + 0x2ec) > 0xfa) {
            CGruntHud* oh = occ->m_10;
            if (TileSwitch6(oh->m_5c >> 5, oh->m_60 >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
                CGruntHud* oh2 = occ->m_10;
                if (m_tileMgr->CommitTileSlot2(m_tileOwnerHi, m_tileOwnerLo, oh2->m_5c, oh2->m_60) ==
                    -1) {
                    *(i32*)((char*)this + 0x2ec) = 0;
                    if (*(i32*)((char*)this + 0x390) != 0) {
                        CGruntHud* h = m_10;
                        i32 vx = h->m_5c;
                        i32 vy = h->m_60;
                        char* sc = *(char**)((char*)g_gameReg->m_30 + 0x24);
                        i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
                        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                            g_gameReg->m_60->CueA(this, 0x366, -1, 0, -1, -1);
                        }
                        *(i32*)((char*)this + 0x390) = 0;
                        *(i32*)((char*)this + 0x2ec) = 0;
                        return 1;
                    }
                }
            }
            goto L8a2;
        }
        return 1;
    }

    {
        u32 dwell = *(u32*)((char*)this + 0x2ec);
        if (dwell > 0x3e8 && m_resetApplied == 0 && *(i32*)((char*)this + 0x318) != 0 &&
            dwell > 0xbb8) {
            if ((i64)(u32)g_645588 - *(i64*)((char*)this + 0x308) >=
                *(i64*)((char*)this + 0x310)) {
                goto L8b5;
            }
            CGruntHud* h = m_10;
            i32 baseX = h->m_134;
            i32 spanX = abs(h->m_13c - baseX);
            i32 baseY = h->m_138;
            i32 spanY = abs(h->m_140 - baseY);
            i32 outX = baseX;
            if (spanX != 0) {
                outX += GruntRand() % spanX;
            }
            i32 outY = baseY;
            if (spanY != 0) {
                outY += GruntRand() % spanY;
            }
            TileSwitch6(outX, outY, 0, m_arrivalFlags, 1, 0);
            i32 m328 = *(i32*)((char*)this + 0x328);
            if (m328 != 0) {
                i32 mx = spanX > spanY ? spanX : spanY;
                if (m328 > mx) {
                    SetEntrancePos(1, 1);
                }
            }
            *(i32*)((char*)this + 0x390) = 1;
            goto L8a2;
        }
    }
    return 1;

L8a2:
    *(i32*)((char*)this + 0x2ec) = 0;
    return 1;

L8b5:
    Stub_062e10(1, 1, 0);
    *(i32*)((char*)this + 0x308) = 0;
    *(i32*)((char*)this + 0x310) = 0;
    *(i32*)((char*)this + 0x30c) = 0;
    *(i32*)((char*)this + 0x314) = 0;
    *(i32*)((char*)this + 0x310) = GruntRand() % 0x7530 + 0x7530;
    *(i32*)((char*)this + 0x314) = 0;
    *(i32*)((char*)this + 0x308) = (i32)g_645588;
    *(i32*)((char*)this + 0x30c) = 0;
    *(i32*)((char*)this + 0x390) = 1;
    goto L8a2;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalDefense()   @0xf2b20   (__thiscall, ret 0 -> 1)
// The multi-state arrival-defender step (the big sibling of ResolveArrival-
// Reposition / ResolveArrivalNeighbor). Latch the defender position to the last
// tile, then dispatch on m_2d4:
//   state 2: resolve the stored grid occupant; if it is in radius + committed +
//            settled on its own tile + on-screen, commit the tile slot (m_198==0x1e)
//            or neighbour-link onto it; on a gate miss mark m_2d4=1 and cue.
//   state 1: re-resolve the grid occupant + GetOccupant agreement; gated on dwell
//            run a StepArrivalDrop, then commit/neighbour-link and advance to state 2.
//   state 0: GetOccupant; if settled + on-screen commit/neighbour onto it, else
//            (dwell elapsed) tile-switch to the occupant + advance to state 1 + cue,
//            or (no occupant) reset the idle timer / re-roll a random in-region target.
// @early-stop
// shared ResolveArrivalReposition wall: the m_2d4 switch subtract-chain, the grid
// index math, the in-radius/settled/on-screen gates, the CommitTileSlot/CommitNeighbor
// paths, the 64-bit sbb idle-timer, the idiv/rand re-roll + abs spans and the
// structure-1 cue are all reconstructed in shape/order. Residue is the MSVC /O2
// idiv/rand scheduling, the shared-tail zero-register sharing and the redundant
// CommitNeighbor stack spills - pure regalloc/scheduling placement. Final sweep.
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepArrivalDefense() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    CGrunt* occ;
    switch (m_2d4) {
        case 2:
            if (m_poweredUp == 0) {
                m_2d4 = 1;
                return 1;
            }
            occ = *(CGrunt**)((char*)m_tileMgr + (m_2f4 + 15 * m_2f0) * 4 + 0x1c);
            if (occ == 0) {
                m_2d4 = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto c2_occcheck;
            }
            if (occ->m_entranceCommitted == 0) {
                goto c2_occcheck;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            if (*(i32*)((char*)this + 0x218) != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_10->m_5c, occ->m_10->m_60) == 0) {
                goto c2_miss;
            }
            if (occ->m_10->m_5c != occ->m_lastTilePxX) {
                goto c2_miss;
            }
            if (occ->m_10->m_60 != occ->m_lastTilePxY) {
                goto c2_miss;
            }
            if (*(i32*)((char*)this + 0x198) == 0x1e) {
                ((CGruntTileMgr*)g_gameReg->m_68)
                    ->CommitTileSlot(m_tileOwnerHi, m_tileOwnerLo, occ->m_10->m_5c, occ->m_10->m_60);
                return 1;
            }
            CommitNeighbor(occ->m_tileOwnerHi, occ->m_tileOwnerLo, occ->m_lastTilePxX,
                           occ->m_lastTilePxY);
            return 1;
        c2_occcheck:
            if (occ == 0) {
                m_2d4 = 0;
                return 1;
            }
        c2_miss:
            m_2d4 = 1;
            {
                CGruntHud* h = m_10;
                i32 vx = h->m_5c;
                i32 vy = h->m_60;
                char* m24 = *(char**)((char*)g_gameReg->m_30 + 0x24);
                i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
                if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                    g_gameReg->m_60->CueA(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case 1: {
            occ = *(CGrunt**)((char*)m_tileMgr + (m_2f4 + 15 * m_2f0) * 4 + 0x1c);
            CGrunt* g = m_tileMgr->GetOccupant(this);
            if (g != 0 && g != occ) {
                m_2f0 = -1;
                m_2d4 = 0;
                m_2f4 = -1;
                return 1;
            }
            if (occ == 0) {
                m_2d4 = 0;
                return 1;
            }
            if (occ->m_entranceCommitted == 0) {
                m_2d4 = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                m_2d4 = 0;
                return 1;
            }
            if ((u32) * (u32*)((char*)this + 0x2ec) > 0x1f4) {
                StepArrivalDrop(occ->m_lastTilePxX, occ->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                *(i32*)((char*)this + 0x2ec) = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_10->m_5c, occ->m_10->m_60) == 0) {
                return 1;
            }
            if (*(i32*)((char*)this + 0x198) == 0x1e) {
                ((CGruntTileMgr*)g_gameReg->m_68)
                    ->CommitTileSlot(m_tileOwnerHi, m_tileOwnerLo, occ->m_10->m_5c, occ->m_10->m_60);
                m_2d4 = 2;
                return 1;
            }
            if (occ->m_10->m_5c == occ->m_lastTilePxX && occ->m_10->m_60 == occ->m_lastTilePxY) {
                CommitNeighbor(occ->m_tileOwnerHi, occ->m_tileOwnerLo, occ->m_lastTilePxX,
                               occ->m_lastTilePxY);
            }
            m_2d4 = 2;
            return 1;
        }

        case 0:
            occ = m_tileMgr->GetOccupant(this);
            if (occ == 0) {
                goto L_f308a;
            }
            if (m_poweredUp == 0 && m_stamina >= 0x64 && occ->m_10->m_5c == occ->m_lastTilePxX &&
                occ->m_10->m_60 == occ->m_lastTilePxY &&
                RectContains(occ->m_10->m_5c, occ->m_10->m_60) != 0) {
                if (*(i32*)((char*)this + 0x198) == 0x1e) {
                    ((CGruntTileMgr*)g_gameReg->m_68)
                        ->CommitTileSlot(m_tileOwnerHi, m_tileOwnerLo, occ->m_10->m_5c,
                                         occ->m_10->m_60);
                    return 1;
                }
                if (occ->m_10->m_5c != occ->m_lastTilePxX) {
                    return 1;
                }
                if (occ->m_10->m_60 != occ->m_lastTilePxY) {
                    return 1;
                }
                CommitNeighbor(occ->m_tileOwnerHi, occ->m_tileOwnerLo, occ->m_lastTilePxX,
                               occ->m_lastTilePxY);
                return 1;
            }
            if (occ == 0) {
                goto L_f308a;
            }
            if ((u32) * (u32*)((char*)this + 0x2ec) <= 0x3e8) {
                goto L_f308a;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto L_f318a;
            }
            {
                GruntTilePos sp;
                occ->GetScreenPos(&sp);
                if (TileSwitch6(sp.m_x >> 5, sp.m_y >> 5, 0, m_arrivalFlags, 1, 0) == 0) {
                    goto L_f318a;
                }
                SetEntrancePos(1, 1);
                m_2f0 = occ->m_tileOwnerHi;
                m_2f4 = occ->m_tileOwnerLo;
                m_2d4 = 1;
                CGruntHud* h = m_10;
                char* m24 = *(char**)((char*)g_gameReg->m_30 + 0x24);
                i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
                if (CueVisible((i32)rect, h->m_5c, h->m_60) == 0) {
                    goto L_f318a;
                }
                g_gameReg->m_60->CueA(this, 0x366, -1, 0, -1, -1);
            }
        L_f318a:
            *(i32*)((char*)this + 0x2ec) = 0;
            return 1;
        L_f308a:
            if (m_resetApplied != 0) {
                return 1;
            }
            if (*(i32*)((char*)this + 0x318) == 0) {
                return 1;
            }
            if ((u32) * (u32*)((char*)this + 0x2ec) <= 0xbb8) {
                return 1;
            }
            if ((i64)(u32)g_645588 - *(i64*)((char*)this + 0x308) >=
                *(i64*)((char*)this + 0x310)) {
                Stub_062e10(1, 1, 0);
                *(i32*)((char*)this + 0x308) = 0;
                *(i32*)((char*)this + 0x310) = 0;
                *(i32*)((char*)this + 0x30c) = 0;
                *(i32*)((char*)this + 0x314) = 0;
                *(i32*)((char*)this + 0x310) = GruntRand() % 0x7530 + 0x7530;
                *(i32*)((char*)this + 0x314) = 0;
                *(i32*)((char*)this + 0x308) = (i32)g_645588;
                *(i32*)((char*)this + 0x30c) = 0;
                *(i32*)((char*)this + 0x2ec) = 0;
                return 1;
            }
            {
                CGruntHud* h = m_10;
                i32 baseX = h->m_134;
                i32 spanX = abs(h->m_13c - baseX);
                i32 baseY = h->m_138;
                i32 spanY = abs(h->m_140 - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += GruntRand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += GruntRand() % spanY;
                }
                if (outX < g_gameReg->m_70->m_c && outY < g_gameReg->m_70->m_10) {
                    TileSwitch6(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = *(i32*)((char*)this + 0x328);
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
            *(i32*)((char*)this + 0x2ec) = 0;
            return 1;

        default:
            return 1;
    }
}

// ---------------------------------------------------------------------------
// CGrunt::StartBombGruntRun()   @0x68520   (__thiscall, ret 0)
// Begin the bomb-grunt run reaction: run the anim-dispatch step, retire all seven
// HUD stat sprites, clear the grunt-kind, and (when powered-up with no live
// neighbor) reset the entrance + idle state. Latch the entrance/struck state, apply
// the move-state; if the move-state driver declines (returns 0) just re-notify the
// move at the current HUD pos and return. Otherwise pick a random adjacent tile
// (rand%3-1 in each axis, forced non-zero), play the directional move sound, latch
// the resolved tile + the "M" run anim-set, load RunningTimePerTile, fire the
// on-screen spawn cue when in view, drive the _ITEM geometry, and re-stamp the
// entrance-cell frame name. Returns 0.
// @early-stop
// frame-slot wall: the instruction stream is byte-identical to retail EXCEPT the
// 0xc-byte stack frame retail reserves for a DEAD m_entranceCell[2] spill (a missed
// MSVC5 optimization not reproducible from clean source - the dead local is DCE'd
// here). Missing `sub esp,0xc`/`add esp,0xc`+spill (~93.4%, body exact). Final sweep.
RVA(0x00068520, 0x2a2)
i32 CGrunt::StartBombGruntRun() {
    StepAnimDispatchB();
    if (m_healthSprite != 0) {
        ((CSpriteRegRecord*)m_healthSprite)->m_8 |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        ((CSpriteRegRecord*)m_staminaSprite)->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        ((CSpriteRegRecord*)m_toySprite)->m_8 |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        ((CSpriteRegRecord*)m_toyTimeSprite)->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        ((CSpriteRegRecord*)m_wingzTimeSprite)->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite != 0) {
        ((CSpriteRegRecord*)m_powerupSprite)->m_8 |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite != 0) {
        ((CSpriteRegRecord*)m_selectedSprite)->m_8 |= 0x10000;
        m_selectedSprite = 0;
    }
    m_gruntKind = 0;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        *(i32*)((char*)this + 0x218) = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ReseedIdleReset(1, 0, 0);
    }
    m_entranceActive = 1;
    m_tileMgr->CommitStruckTile(m_tileOwnerHi, m_tileOwnerLo, 1);
    ApplySetState1(1);
    SetEntrancePos(1, 1);
    if (SetMoveStateA(1, 1, 0, 1) == 0) {
        CGruntHud* h = m_10;
        m_tileMgr->NotifyMoveAt(h->m_5c, h->m_60, -1, 0);
        return 0;
    }
    i32 dx = GruntRand() % 3 - 1;
    i32 dy = GruntRand() % 3 - 1;
    if (dx == 0 && dy == 0) {
        dx = 1;
    }
    {
        CGruntHud* h = m_10;
        dy += h->m_60 >> 5;
        dx += h->m_5c >> 5;
    }
    PlayMoveSoundAtTile(dx, dy);
    *(i32*)((char*)this + 0x3e4) = dx;
    *(i32*)((char*)this + 0x3e8) = dy;
    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeM);
    m_timePerTile = g_buteMgr.GetIntDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
    m_22c = 1;
    {
        CGruntHud* h = m_10;
        i32 vx = h->m_5c;
        i32 vy = h->m_60;
        char* sc = *(char**)((char*)g_gameReg->m_30 + 0x24);
        i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
            g_gameReg->m_60->CueSpawn(this, 8, -1, -1, -1);
        }
    }
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(m_poseItem);
    i32 col = m_entranceCell[1] + m_entranceCell[0] * 2;
    i32 base = m_entranceCell[0] + col + 0xb;
    i32 idx = base + base * 3 * 4;
    char* cn = GruntStrGetBuffer((char*)this + idx * 8, 0);
    m_154->GameApplyName(cn);
    return 0;
}
