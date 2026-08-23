# The inverted branch over a wedged block is a C2 layout coin, not a source question
tags: asm:jcc asm:jmp cpp:branch | topic:wall topic:scoring-artifact
symptoms: ours `jcc <near>` skipping a short block that ends `jmp <far>`, retail a single `jcc <far>` falling through (or the reverse); `walls jccscan --flips` reports the row as "neither clears a RET: block placement"; a foreign block sits between the branch and its fall-through successor
confidence: 9/10
variants: tail-block-placement-cross-jump-wall.md, switch-arm-tail-crossjump-vs-duplicate.md, first-function-epilogue-merge-oracle.md

A conditional branch's fall-through successor has to be the block physically
next, or cl inverts the branch and reaches the real successor through a
trailing transfer:

```asm
; ours - the fall-through successor is NOT next, so the branch is inverted
0573: jne 0x582          ; skip
0575: jmp 0x887          ; the real successor, reached the long way
057a: mov ebp,[esp+0x30] ; a FOREIGN block cl placed here
057e: mov edi,[esp+0x34]
0582: ...                ; the fall-through successor, finally

; retail - the fall-through successor IS next, so one branch does it
055c: je  0x933
0562: mov ecx,[esp+0x10]
```

Both sides hold the same CFG. The only difference is which block the layout
pass put next. `CStatusBarMgr::Sync` 0x1084d0, `CGrunt::ArrivalReticleScan`
0xee800 and `CTriggerMgr::WireTileSwitchLogic` 0x6c130 all show it with
instruction-identical context on both sides at the site.

## It is a coin - the census, run in BOTH directions

Counted per function on the normalized pair: a conditional branch whose target
lands just past a short (<= 8 instruction) inline tail ending in `jmp` or
`ret`, i.e. the shape above. Functions carrying an in-`.text` switch table are
excluded, because the table bytes decode as instructions
(`CGruntzMgr::HandleCommand` 0x862f0 alone reads 47/28 from table payload).

| | base | target |
|---|---|---|
| all 4250 functions | 4044 | 4049 |
| 3677 byte-exact rows | 2789 | 2789 |
| **573 sub-100 rows** | **1255** | **1260** |

The exact-row line is degenerate and proves nothing - byte-identical functions
must agree. The load-bearing line is the last one, and on it the shape occurs
**1255 times in our build against 1260 in retail, a 0.4% difference that goes
the WRONG way for a "cl over-produces it" story.** Per row: 19 rows are
base-heavy (+27 sites), 22 are target-heavy (+32 sites). The inline-tail-length
distribution over those same sub-100 rows is flat between the two sides:

```
tail insns   1     2     3     4     5     6     7     8
base         1   342   157   124   167   195   210    59
target       1   352   160   123   158   205   208    53
```

Same shape, same sizes, same frequency, no direction. That is a coin, and it is
the coin already characterized for cl 5.0's C2 layout pass (region-intrinsic,
decided inside c2 with no IL representation, six construct families refuted -
`docs/patterns/tail-block-placement-cross-jump-wall.md` and the ChargeStep
reproducer, which is itself in the base-heavy list).

## What this retires

`gruntz walls jccscan --flips` prints a `-` bucket for rows where neither side's
near jump clears a `ret`, described as "block placement, not an exit count".
**That bucket is not a worklist.** It selects rows by where the first condition
code flips, not by any placement surplus, and the census above says the
population it is pointing at has no bias to correct. Work the `ours` and
`retail` buckets, which are about exit COUNTS, and read `-` as "this row's first
flip carries no information".

## The scope, stated narrowly

Block placement is of course reachable from source in general - any CFG edit
moves it. Measured the same day on `CProjectile::SerializeMove` 0xe0d40:
rewriting `if (s == NULL) return 0;` as an `else` arm of the main body did move
the block, but the wrong way (cl then merged all three `return 0` blocks
including the one at the function head, 95.07 -> 92.74). The claim here is the
narrow one: **given a CFG that already matches, which of two successors cl puts
next is not steerable by a spelling**, and a `--flips` row whose only signal is
that choice is not worth a build.
