# Place an if-body FAR: negate the outer condition (`if(x!=A){if(x==B)}else{..A..}`)
tags: cpp:branch | asm:cmp asm:je asm:jne asm:jmp | topic:codegen-idiom
symptoms: `cmp reg,A; je FAR; cmp reg,B; jne END; <B body>; jmp END; FAR: <A body>` in retail vs your `cmp reg,A; jne L; <A body>; jmp END; L: cmp reg,B; jne END; <B body>`; the two if-bodies are byte-identical but SWAPPED in layout; je↔jne on the first compare
confidence: 8/10

## Symptom

A two-way dispatch `if (x == A) { bodyA } else if (x == B) { bodyB }` where the
recompile lays **bodyA inline** (fall-through) but retail places **bodyA FAR**
(after bodyB). The instruction selection inside each body already matches; only
the block ORDER + the first compare's branch sense differ:

```
; retail                              ; natural if/else-if recompile
cmp  ebx, 4                           cmp  ebx, 4
je   FAR            ; A far            jne  L1            ; A inline
cmp  ebx, 7                           <bodyA>
jne  END                              jmp  END
<bodyB>                               L1:
jmp  END                              cmp  ebx, 7
FAR: <bodyA>                          jne  END
END:                                  <bodyB>
                                      END:
```

Objdiff shows the two bodies mutually mis-aligned (a run of `+`/`-` where bodyA and
bodyB trade places) plus a lone `je`↔`jne` on the first `cmp`. Everything else is
byte-identical, so the function plateaus high-80s/low-90s with the diff clustered at
the dispatch heads.

## Cause

MSVC5 /O2 fixes the physical block order at the point it emits the branch. For the
NATURAL `if (x==A) {A} else if (x==B) {B}` it emits A inline (the first-tested body
falls through). Retail's source tested `x==A` first but placed the A-body last —
which is exactly what MSVC emits when the source NEGATES the first test and demotes
the A-body into the trailing `else`:

```cpp
if (x != A) {          // cmp A; je <else = A body>
    if (x == B) {      // cmp B; jne END
        bodyB;
    }
} else {               // (outer-true end) jmp END
    bodyA;             // FAR: placed after bodyB
}
```

`if (x != A) … else` reorders the physical blocks *without changing behaviour*: the
`!= A` test jumps FORWARD to the else (bodyA), so bodyA lands after the fall-through
bodyB. When a body ends in `return`/a terminal jump, MSVC folds the inner
`jne skip; ret; skip: jmp END` into a single `jne END`, so no extra bytes appear.

## Fix

Rewrite each such dispatch as the negated-outer form. It is behaviourally identical
to `if(x==A)else if(x==B)` but matches retail's block layout. Applied to all four
kind-4/kind-7 dispatches in `CTriggerMgr::Serialize` (0x7a5e0): 88% → **100%**
(after also fixing the 4-arg prototype, the probe polarity, and the virtual
GetA/GetB dispatch). The same negation works whether a body falls through (pose
blocks) or returns (the probe guards + the final pose block's `return 1`).

## Related

- [redundant-sibling-guard-retest](redundant-sibling-guard-retest.md) — nested vs
  sibling guard blocks (a different branch-shape lever).
- [switch-subtract-chain-vs-ifelse](switch-subtract-chain-vs-ifelse.md) — when the
  dispatch is 3+ near-consecutive values, prefer `switch` over the if-chain.
