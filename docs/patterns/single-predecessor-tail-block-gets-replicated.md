# A far `goto`-tail with ONE predecessor is replicated into it — give it a second
tags: cpp:branch cpp:goto | asm:jcc asm:ret | topic:codegen-idiom
symptoms: retail's `jcc <far>` reads `jcc <near>` + an inline copy of the target block in the base; block skeleton diverges at the FIRST such branch and every block index after it is shifted; base has one FEWER `ret` than the target; the base's compiled length is short by roughly one epilogue
confidence: 8/10

A `goto` to a small `return`-terminated label placed at the end of the function is
emitted FAR by cl5 only when the label has **two or more predecessors**. With exactly
one, cl replicates the whole block into that predecessor's fall-through and drops the
far copy — which shifts every downstream block and can also let an unrelated arm
fall into the function's terminal `return`, merging two epilogues retail keeps apart.

## Symptom

`gruntz walls diagnose <rva> --asm` reports the first true divergence at
the branch itself, with the *kind* mismatched, and everything after it renumbered:

```
  B6     3i [jcc B8   | fall B7]   ~=   3i [jcc B185 | fall B7]
  B7    11i [ret]                  !!   2i [jcc B9   | fall B8]     <- the replicated copy
  ...
  B185   3i [jcc B187 | fall B186] !!  11i [ret]                    <- where retail keeps it
```

Two cheap corroborations before you touch the source:

- `--lite | grep -c ret` on both sides — the base is short by one.
- `gruntz walls diagnose <rva>` — the base is short by about one
  epilogue's worth of bytes (13 B for `mov eax,N` + four `pop` + `add esp,N` + `ret`).

## Cause

The block is `SetEntrancePos(1, 1); return 0;` — 11 instructions including a call —
so it is not "tiny"; size is not the gate. The gate is the predecessor count.
Retail's `CGrunt::StepGruntMovement` 0x0004c170 reaches `label_dropRet0` (0x4cd35)
from **one** site (`0x4c1f0: je 0x4cd35`) and still keeps it far, because a *second*
site reaches it by falling into it from the block above.

## The fix

Find the other place in the function that spells the SAME statements and make it a
`goto` at the shared tail instead of a second inline copy:

```cpp
label_4cb2a:
    SetFacing(0x3e8, rec);
    goto label_dropRet0;        // was: SetEntrancePos(1, 1); return 0;
...
label_dropRet0:
    SetEntrancePos(1, 1);
    return 0;
```

cl then replicates the body into the *fall-through* predecessor (which is what retail
has at that site anyway) and leaves the far copy standing for the conditional
predecessor. `StepGruntMovement` 64.91 -> 66.15, first true skeleton divergence
B7 -> B32 (25 blocks became byte-exact), base length -82 B -> -65 B.

Do NOT reach for this when the far label genuinely has one predecessor in retail too
and the block is still inline in your build — see the sibling below, where the same
symptom comes from a *cross-jump* instead and has no known lever.

## Related

- [`allocate-check-then-body-is-the-then-block`](allocate-check-then-body-is-the-then-block.md)
  — which arm falls through names the source's if/else shape.
- [`negated-condition-far-block`](negated-condition-far-block.md) — the other
  block-placement lever (negate the outer test).
- [`tail-block-placement-cross-jump-wall`](tail-block-placement-cross-jump-wall.md) —
  the same symptom when the cause is a cross-jump, which is a wall.
