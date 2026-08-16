# A found-it scan loop ends by NULLING THE CURSOR, not by `break`
tags: cpp:loop cpp:branch | asm:xor asm:jmp asm:jcc | topic:codegen-idiom
symptoms: a `while (cursor)` walk whose last match test is inverted vs retail, and retail has an
extra two-instruction block `xor <cursor-reg>,<cursor-reg>` + `jmp <the loop's bottom test>` that
the recompile does not emit at all; the recompile's match test jumps straight PAST the loop
confidence: 9/10

## Symptom

`jcc_sieve` flags one flipped branch inside a list/map walk. Reading both sides:

```asm
; retail                                    ; recompile (`break`)
  cmp  ecx,1                                  cmp  ecx,1
  je   NULL_CURSOR                            je   AFTER_LOOP
  cmp  ecx,2                                  cmp  ecx,2
  jne  CLEAR_FOUND                            je   AFTER_LOOP
NULL_CURSOR:                                CLEAR_FOUND:
  xor  eax,eax        ; cursor = 0            xor esi,esi     ; found = 0
  jmp  BOTTOM                                BOTTOM:
CLEAR_FOUND:                                  test eax,eax
  xor  esi,esi        ; found = 0             jne  TOP
BOTTOM:                                     AFTER_LOOP:
  test eax,eax        ; while (cursor)
  jne  TOP
AFTER_LOOP:
```

The recompile's version is two instructions SHORTER, so this does not look like a missing
construct — it looks like a branch-polarity coin flip. It is not: retail's source keeps the
single loop exit and stops the walk by clearing the cursor.

## Cause

`break` gives cl a second exit edge out of the loop, so it branches directly to the code after
the loop and never needs the cursor store. Clearing the cursor keeps ONE exit — the bottom test
— so cl must emit the `xor` and a `jmp` to that test.

Both are behaviourally identical here because the loop condition is the cursor.

## Fix

```cpp
while (node) {
    found = PrevItem(node);          // advances node, returns the item
    if (found) {
        i32 k = found->m_state;
        if (k == 1 || k == 2) {
            node = 0;                // NOT `break`
            continue;
        }
    }
    found = 0;
}
```

`CMenuPage::FocusNext` @0x183c50 and `CMenuPage::FocusPrev` @0x183d10, 97.18 → **100.00 EXACT**
each (2026-07-28). Both had also needed the positive-form wrap gate
([positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md)) to get to 97.18; this
is the last piece, and it is invisible to `gruntz walls diagnose --asm` because the whole difference is two
instructions plus a masked displacement.

## Related

- [retry-loop-bail-while-goto-no-peel](retry-loop-bail-while-goto-no-peel.md) — the other
  `break`-is-wrong loop family (there the answer is a `goto` past the success block, because the
  bail must skip code the bottom test would run).
- [masked-diff-hides-branch-target](masked-diff-hides-branch-target.md) — `gruntz walls diagnose <rva>` is what surfaces these.
