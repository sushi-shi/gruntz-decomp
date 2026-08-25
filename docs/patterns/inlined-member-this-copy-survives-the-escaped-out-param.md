# A spelled-out inline member body needs the call's `this` copy, or cl reloads the escaped out-param

**Tags:** cpp:local cpp:inline cpp:method | asm:mov | topic:codegen-idiom topic:regalloc

## Symptom

A body reconstructed by transcribing an inline member's statements over a pointer
that came out of an *escaping* out-parameter (`Lookup(key, out)`, `GetNextAssoc`,
any `T*&` sink) is ONE instruction longer than retail, and the extra instruction
is a redundant reload of that pointer immediately after a store through it:

```asm
; base                                   ; retail
mov  [eax+0x14],ecx                      mov  [eax+0x14],ecx
mov  eax,[esp+0x10]     ; <- EXTRA       mov  ecx,[eax+0x10]
push 0                                   push 0
push 0
mov  ecx,[eax+0x10]
```

`walls diagnose` calls it REGALLOC/SCHEDULING with a 1-instruction size delta and
no call-set or CFG difference; `walls semdiff` shows every key identical.

## Mechanism

The out-param's address escapes into the lookup, so the variable gets a stack home
and cl treats a store *through* it as possibly clobbering that home. In retail the
same statements were not written in the caller at all - they were the body of an
inline member (`cue->PlayIfElapsed(...)`), and inlining materialises the call's
`this` argument as a fresh IL temp. That temp does not alias anything, so it stays
in a register across the store and no reload is emitted.

## Fix

Give the transcribed body its own pointer local - the `this` copy the inline would
have made:

```cpp
// NO - the store through the escaped out-param forces a reload
LeafCue* cue = NULL;
MapLookup(host->m_cues, "GAME_MINORCHEAT", cue);
if (cue != NULL && g_sndEnabled) {
    if (g_killCueClock - cue->m_lastPlayTime >= cue->m_replayDelay) {
        cue->m_lastPlayTime = g_killCueClock;
        cue->m_sound->AcquireAndPlay(g_sndCueTag, 0, 0, 0);
    }
}

// YES - `cue` is the inlined member's `this`, and it is not the lookup's sink
LeafCue* found = NULL;
MapLookup(host->m_cues, "GAME_MINORCHEAT", found);
LeafCue* cue = found;
if (cue != NULL && g_sndEnabled) { ... }
```

A file-local `static inline LeafCue* LookupCue(CMapStringToPtr&, LPCTSTR)` that
returns the pointer is the same device (`src/Gruntz/InGameIcon.cpp` already uses
it) - the return value is the non-aliasing temp.

## Return the typed copy when the caller repeatedly reads one member

There is a stronger form when the caller does not modify the looked-up object but
repeatedly calls through one of its members. A sink plus a caller-side copy keeps
the object in a register, but cl can replace that object identity with the address
of the repeated member:

```asm
; sink + caller copy                    ; retail
mov  eax,[esp+8]                        mov  esi,[esp+8]
lea  edi,[eax+0x10]    ; &cue->m_sound  mov  ecx,[esi+0x10]
call IsPlaying                          call IsPlaying
mov  ecx,[edi]                          mov  ecx,[esi+0x10]
```

Put the typed sink inside a file-local inline helper and return it. The returned
value is the caller's non-aliasing object identity, so every use remains
`[cue+0x10]` and the extra `lea` disappears. Both
`CBootyState::LeaveState` (0x18e40) and
`CMultiBootyState::LeaveState` (0x1e660) moved **79.30 -> 89.75**, with retail's
exact 45 instructions, 6 calls, 5 branches, one return, 7 relocations, identical
mnemonics/constants/displacements, and zero ordered-referent divergence. The
remaining residue is only an ESI/EDI swap between `this` and the returned cue.

The boundary, not a spelling accident, is what matters. Taking the map by
reference versus taking its sound-registry owner by pointer was byte-identical;
using a `CObject*` sink and casting the returned value to `LeafCue*` was also
byte-identical. Passing the whole `CState*` into the helper was worse (85.32),
and a named sound-registry local was byte-flat. Moving the existing
`CDDrawSubMgrLeafScan::Lookup` body into the header was rejected even though it
gave the same caller shape: our compiler then emitted no standalone body, while
retail has the exact 0x5b7e0 body and five TUs with out-of-line references.
Preserve that real symbol until the missing inline-budget context is recovered.

STEERABLE. Measured 2026-08-20, all on the LeafCue play-cue transcription:
`CRainCloud::HitTest` 94.93 -> **100.00 EXACT**,
`CPreviewState::LoadLevelPreviewScreen` 94.74 -> **100.00 EXACT** (unit to 100%),
`CMulti::HandlePlayerCreated` 95.79 -> **100.00 EXACT**,
`CMulti::ShowMultiStartDlg` 97.67 -> **100.00 EXACT**,
`CGruntzMgr::CheatEclipseToggle` 96.70 -> **100.00 EXACT**,
`CGruntzMgr::CheatSkeletonToggle` 95.50 -> 99.25,
`ScrollDialog` 89.95 -> 94.26,
`CTriggerMgr::BuildRockBreakParticles` 90.01 -> 91.81,
`CSpotLight::Tick` 78.88 -> 79.33.

The same transcription in the switch/chat cue bodies gives:
`CMenuTree::PlayFocusSound` 90.20 -> **100.00 EXACT**,
`CMenuTree::PlayActivationSound` 90.20 -> **100.00 EXACT**,
`CGiantRockLogic::BuildRockBreakInGameText` 96.33 -> 99.46,
`CTileTriggerSwitchLogic::SwitchDown` 89.46 -> 93.60, and
`CTileTriggerSwitchLogic::SwitchUp` 90.15 -> 93.60.

## The second half of the same body: the gate and the tag are BOTH locals

The `this` copy fixes the pointer. The play-cue body has a second, independent
defect with the same cause — retail materialises **both** globals into registers
before the gate test, so the tag is live across the elapsed comparison:

```asm
; retail 0xb1af0+0x1c1                  ; ours, tag read at the call
mov  ecx,ds:g_sndEnabled                cmp  edi,ds:g_sndEnabled
mov  edx,ds:g_sndCueTag                 je   skip
cmp  edi,ecx                            ...
je   skip                               mov  [eax+0x14],ecx
...                                     mov  ecx,ds:g_sndCueTag   ; <- LATE
mov  [eax+0x14],ecx                     push edi
push edx           ; the tag            push ecx
```

One extra long-lived value shifts the whole callee-saved rotation (R2), and in
`CSpotLight::Tick` it also decided the exit shape. The null test has to be split
out of the gate so the loads sit between them:

```cpp
// NO
if (cue != NULL && g_sndEnabled != 0) { ... AcquireAndPlay(g_sndCueTag, 0, 0, 0); }

// YES
if (cue != NULL) {
    i32 gate = g_sndEnabled;
    i32 item = g_sndCueTag;
    if (gate != 0) { ... AcquireAndPlay(item, 0, 0, 0); }
}
```

STEERABLE. Measured 2026-08-21: `CSpotLight::Tick` 79.33 -> **84.21**,
`CGrunt::LoadGruntCombatAnimations` 74.45 -> 75.86,
`CBootyState::LevelMsgHudDriver` 89.13 -> 90.69,
`CTriggerMgr::BuildRockBreakParticles` 91.81 -> 92.52,
`CBootyState::UpdateBootyWalkingGruntz` 95.24 -> 95.48.

**Adjudicate per site.** `CGruntzMgr::CheatEclipseToggle` is EXACT with the
tag-before-gate spelling and its body-sharing sibling `CheatSkeletonToggle`
(99.25) must keep it; `CTriggerMgr::LoadFinishLevelSprite` is byte-flat under the
change. Read the target's two `mov <reg>,ds:g_snd*` loads before editing.

## Bounds

The copy only helps where the pointer is REASSIGNED-through, i.e. the body stores
into the pointed-to object before its next read, and the lookup needs its OWN
sink. `CTriggerMgr::LoadFinishLevelSprite` re-uses an outer `p` for two successive
lookups: copying out of that shared `p` costs it (86.06 -> 84.30), giving the
second lookup a fresh sink AND a fresh copy gains it (86.06 -> 88.36). Measure per
site rather than sweeping a file.
