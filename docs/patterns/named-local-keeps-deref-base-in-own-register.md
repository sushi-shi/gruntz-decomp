# A chained deref COLLAPSES into one register - a short-lived named local keeps the base separate

**Tags:** `cpp:local` `cpp:member` | `asm:mov` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 8/10

## Symptom

A two-step member deref is one register in your build and two in retail:

```
base:    mov ecx,[esi+0x48]      ; m_midi
         mov ecx,[ecx+0x1c]      ; ->m_currentSequence   (same reg reused)
target:  mov eax,[esi+0x48]      ; m_midi        (base kept separate)
         mov ecx,[eax+0x1c]      ; ->m_currentSequence
```

Everything else matches. The identical source expression in a SIBLING function may
already match retail's two-register form (it did here: `CGruntzMgr::StopMusic`
@0x8f6a0 with the same `m_midi->m_currentSequence ? ... : 0` ternary is byte-exact), which
is what makes this look like an unsteerable context-dependent coin flip.

## Cause

When the base subexpression is written inline (`m_midi->m_currentSequence`), cl5 treats it
as a dead-on-arrival temp and is free to reuse its register for the loaded field.
Whether it does depends on local pressure. Reading the base through a NAMED LOCAL
gives it its own live range, so the base lands in its own register.

## Fix

Introduce the local for the expression that needs the split - and *only* for it. The
local must die where retail's register dies:

```cpp
// before - cl collapses ecx <- ecx
if ((m_midi->m_currentSequence ? m_midi->m_currentSequence->IsPlaying() : 0) && pauseMusic) {
    m_midi->PauseCurrent();
}

// after - matches retail (base in eax, field in ecx)
MidiManager* midi = m_midi;
if ((midi->m_currentSequence ? midi->m_currentSequence->IsPlaying() : 0) && pauseMusic) {
    m_midi->PauseCurrent();          // retail RELOADS here - do NOT use snd
}
```

**Do not widen the local.** Using `snd` in the body too keeps it live across the call
and forces a callee-saved register / reload shuffle: that spelling scored 96.06% where
the narrow one is 100%.

## Evidence

`CGruntzMgr::FinishLevel` @0x08e980, 99.90% -> 100.00 EXACT with exactly this change
(`src/Gruntz/GruntzMgr.cpp`).
