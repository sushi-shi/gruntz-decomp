# The `if` BODY owns the fall-through — that is what fixes a lone jcc polarity flip at the tail
tags: cpp:branch cpp:return cpp:goto | asm:jcc asm:je asm:jne | topic:codegen-idiom
symptoms: a function is 96-99% with EXACTLY ONE branch inverted (`je`↔`jne`) and it is the LAST
conditional; the two exit blocks are byte-identical but SWAPPED (whichever is emitted first sets
eax *before* the pops, the second sets it *between* the pops); `gruntz walls diagnose --asm` shows only the
one flip; ret counts are EQUAL on both sides
confidence: 9/10

## Symptom

`jcc_sieve` reports `POLARITY … rets N->N: #last je->jne`. The function's whole body matches; the
tail has two exit blocks whose *contents* match and whose *order* does not:

```asm
; retail                                    ; recompile
  cmp  eax,0x17                               cmp  eax,0x17
  je   RET1                                   jne  RETNEG          ; <- the lone flip
  or   eax,-1                                 mov  eax,0x1
  pop  edi ; pop esi ; ret 4                  pop  edi ; pop esi ; ret 4
RET1:                                       RETNEG:
  pop  edi ; mov eax,1 ; pop esi ; ret 4      pop edi ; or eax,-1 ; pop esi ; ret 4
```

Note the tell that they really are the same two blocks: on **both** sides the block emitted
FIRST sets `eax` before the pops, and the block emitted SECOND sets it in the middle of them.

## Cause

cl5 gives the fall-through to the **body of the `if`** and branches, on the negated condition,
*over* it. It does that even when the body is a bare `return` and even when a `goto` label
follows — so the source shape that decides the layout is which arm you wrote as the body:

| source | emitted |
|---|---|
| `if (x == A) { goto out; } return -1; out: return 1;` | `jne <return -1>`, **`return 1` first** |
| `if (x != A) { return -1; } out: return 1;`           | `je <return 1>`, **`return -1` first** ← retail |

Both are behaviourally identical. Only the second matches retail, because in retail the `-1`
exit is the fall-through of the last compare.

The `--diff` view **hides this**: it masks branch displacements, so a flip whose only other
consequence is two swapped tail blocks reads as a couple of moved lines, and the polarity looks
like a coin toss. It is not — it is a source shape. See
[masked-diff-hides-branch-target](masked-diff-hides-branch-target.md).

## Fix

Rewrite the last conditional so that the arm retail **falls through to** is the `if` BODY, and
the arm retail **branches to** is what follows. For a chain of same-exit guards, nest the
negations instead of `goto`-ing a shared label:

```cpp
// BEFORE - each probe `goto yes`, `return 0` before the label: cl put `yes` FIRST
if (r1 == kTileSoft)  goto yes;
if (r2 == kTileSoft2) goto yes;
return 0;
yes:
return 1;

// AFTER - the next probe (finally `return 0`) is the BODY: every guard is `je <return 1>`
if (r1 != kTileSoft) {
    ...second probe...
    if (r2 != kTileSoft2) {
        return 0;
    }
}
return 1;
```

Measured (2026-07-28), all three had been parked as regalloc/tail-duplication walls:

| function | before → after |
|---|---|
| `CGameLevel::ProbeFootSoft` @0x160080 | 98.72 → **99.99** |
| `CGameLevel::ProbeFootBlocked` @0x160210 | 99.07 → **99.99** |
| `CTileTriggerLogic::Classify` @0x112970 | 96.03 → **98.85** |

## The polarity flip can also be a REAL BUG — check the branch TARGET first

In `Classify` the flip was a *symptom*. Retail's `cmp [esi+0x38],1 / jne` went to the shared
`or eax,-1` block, i.e. the guard only gates a `Tick()` call and the arm returns **−1** either
way; our source returned **+1** there (reporting "still active" on an expired duty cycle). Fixing
the semantics is what let the exit blocks fall into retail's order at all. So:

**read where each side's branch GOES, not just its mnemonic.** A polarity hit whose two sides
jump to *differently-valued* exits is a behaviour difference, not a layout choice.

## Not this pattern

- **ret counts differ** (`rets 1->2` / `3->4`): the exits are duplicated/merged differently, which
  is [positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md)'s lever, not this
  one. `EngStr_DrawText` @0x115440 (1→2 rets) is the documented *inverse* wall — retail emits the
  guard's `ret` inline where cl tail-merges it, and no gate spelling splits a bare `void` ret
  ([identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md)).
- **the flip is not the LAST conditional** and the bodies are not two exits: see
  [negated-condition-far-block](negated-condition-far-block.md) (two if-BODIES swapped) or
  [positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md).
- **a constant fold rides along**: `CGruntzMgr::ToggleObjectLayer` @0x8efe0's flip is coupled to
  cl folding `count-1` to `mov eax,3`; rewriting the conditional fixes the polarity and re-colours
  two registers for a net loss. Retested 2026-07-28, kept as an `@early-stop`.

## Related

- [masked-diff-hides-branch-target](masked-diff-hides-branch-target.md) — why `--diff` hides this,
  and `gruntz walls diagnose <fn>`, which names the divergence class per function.
- [positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md) — the ret-count lever.
- [negated-condition-far-block](negated-condition-far-block.md) — the two-if-bodies variant.
