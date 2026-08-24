# An `inline` helper supplies the extra IL tuple a pure-colour row needs

tags: cpp:inline cpp:call cpp:local | asm:mov asm:imul | topic:codegen-idiom topic:regalloc
symptoms: a short function is instruction-for-instruction identical to retail
except that one computed value lives in a different register from its first
definition to the `ret`, retail pays a redundant closing `mov eax,<reg>` that no
straight-line spelling asks for, and every in-function A/B (named result local,
reused input local, constant first, explicit parenthesisation, `else if` chain,
declaration-count probes) is byte-identical
confidence: 9/10
variants: register-colour-is-cursor-phase-not-a-work-item.md

## Mechanism

cl 5.0 picks a destination register per IL tuple from a rotating cursor, so to
move the colour of tuple N you must change how many tuples precede it in the same
basic block. Within one expression there is nothing left to respell - which is
what made these rows look terminal. **An inlined call is a tuple the source can
add.** Binding the argument and substituting the body advances the cursor before
the body's own tuples are allocated, so the whole tail re-colours.

The prior points the same way: the era devs DID write inlines, and a computation
that appears verbatim in more than one function is a helper, not a coincidence.
Recurrence is the evidence that licenses the change.

```cpp
// helper in the domain's own header, beside the constants it clamps against
inline i32 MidiVolumeToPercent(i32 v) {
    if (v <= 0) { return 0; }
    if (v >= MIDI_VOLUME_MAX) { return VOLUME_PCT_MAX; }
    return v * VOLUME_PCT_MAX / MIDI_VOLUME_MAX;
}

i32 MidiManager::GetMasterVolume() {
    if (g_ailMidiDriver == NULL) { return VOLUME_PCT_MAX; }
    return MidiVolumeToPercent(AIL_XMIDI_master_volume(g_ailMidiDriver));
}
```
```asm
; retail: the quotient stays in EDX and is copied out at the return
add  edx,ecx
sar  edx,0x6
mov  eax,edx
shr  eax,0x1f
add  edx,eax
mov  eax,edx        ; the "redundant" move the hand-expanded form never emits
; base before: the same six instructions with EAX and EDX/ECX transposed
```

Steerable. `MidiManager::GetMasterVolume` 0x1389c0 91.30 -> **100.00 EXACT** in
one build, after five in-function A/Bs and eighteen declaration-count probes were
byte-flat. The inverse conversion (percent -> MIDI) occurs verbatim twice in the
same TU and is what licensed the helper; both of those sites are already 100.00 so
they were left as written - a candidate spelling only overrules a transcription
when the transcription is NOT already exact.

Screen for it: a sub-100 row whose `walls diagnose` says REGALLOC with equal
instruction counts, AND whose arithmetic appears in another function of the same
module. Do not invent a helper for a computation that occurs once.
