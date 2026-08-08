# A guard epilogue that runs BEFORE the callee-save pushes means the body was an inlined helper

tags: cpp:inline cpp:call cpp:return cpp:branch | asm:push asm:pop asm:ret | topic:codegen-idiom
symptoms: `jcc_sieve` POLARITY row with `rets N -> N+1` (retail has MORE epilogues); retail's
FIRST `ret` sits before any `push esi`/`push edi`, and those pushes appear *inside* the body block
in first-use order (`push edi; push esi`, not the canonical prologue order); the recompile pushes
both up front and tail-merges the guard into the shared bottom exit; everything after the guard is
otherwise byte-identical
confidence: 9/10

## The shape

`CChatBox::PlayFocusSound` @0x183030, retail:

```asm
    push  ecx                 ; the one stack local
    mov   edx,[ecx+0x44]
    mov   eax,[edx-0x8]       ; m_row0Key.GetLength()
    test  eax,eax
    jne   0x18303f
    xor   eax,eax
    pop   ecx
    ret                       ; <- epilogue A: restores NOTHING
0x18303f:
    mov   eax,[ecx]
    push  edi                 ; <- the saves live HERE, in first-USE order
    push  esi
    ...
```

The recompile, from the *same* early-return source, emits `push esi; push edi` in the prologue
(canonical order) and turns the guard into `je <shared bottom exit>`. Both gate spellings —
`if (len == 0) return 0;` and `if (len != 0) { body } return 0;` — are **byte-identical**, so the
gate was never the variable (`positive-gate-enables-shrink-wrap.md` calls this direction
"diagnosed, not actionable"; for this sub-shape it is actionable).

## The lever

The body after the guard was an **inlined helper**, and cl 5.0 emits the callee-save pushes at the
top of the inlined REGION rather than in the prologue. Extract it:

```cpp
static __inline i32 PlayChatCue(CDDrawSubMgrLeafScan* roster, const char* key) {
    /* ...the whole body... */
    return 0;
}

i32 CChatBox::PlayFocusSound() {
    if (m_row0Key.GetLength() == 0) {
        return 0;
    }
    return PlayChatCue(m_page->m_soundRegistry, m_row0Key);
}
```

| function | before -> after |
|---|---|
| `CChatBox::PlayFocusSound` @0x183030 | 84.71 -> **100.00 EXACT** |
| `CChatBox::PlayActivationSound` @0x1830b0 | 84.71 -> **100.00 EXACT** |
| `CPlay::StepGridWalk` @0xd0a60 | 66.67 -> **100.00 EXACT** |

## How to find the candidates

Two of the three came free: `PlayFocusSound`/`PlayActivationSound` are the SAME body over
`m_row0Key`/`m_row1Key`, which is the [inlined-ENTITY tell](shared-inline-transcribed-once-per-call-site.md).
`StepGridWalk` had no twin — the disassembly alone said it, via the mechanical screen:

    retail: index of the first `ret` < index of the first `push esi|edi|ebx|ebp`
    base:   the reverse

A tree-wide sweep of the 62-row POLARITY bucket (2026-08-08) found exactly **three** rows with
that signature, and all three closed. It is a narrow but decisive screen — it is not the bucket's
shared mechanism (there isn't one).

## Trap

Put the `RVA()` pin on the METHOD, not on the extracted helper. The helper is inlined and emits no
COMDAT, so a pin above it silently drops the label and `labels.py` fails the build with
`unit '<u>': labelled functions DROPPED N -> N-1`.

related: shared-inline-transcribed-once-per-call-site.md, positive-gate-enables-shrink-wrap.md,
shrink-wrapped-callee-save-push.md, inline-boundary-is-readable-off-the-callsite.md
