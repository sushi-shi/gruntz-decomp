# The frame-size sieve: `sub esp,N` deltas, and the four things that are NOT scope overlay

tags: cpp:local cpp:scope cpp:switch | asm:sub-esp | topic:wall topic:regalloc topic:tooling
symptoms: a reconstruction whose `sub esp,N` differs from retail's, so every
`[esp+K]` and every intra-function branch displacement shifts and a nearly
byte-identical body scores in the 80s or low 90s
confidence: 9/10 (tree-wide sweep of the 580-row todo queue, 2026-08-23)
variants: switch-arm-locals-overlay-only-when-scoped.md, temp-slot-overlay-reveals-scope-structure.md

[switch-arm-locals-overlay-only-when-scoped](switch-arm-locals-overlay-only-when-scoped.md)
says a function-scope temp used by one `switch` arm keeps its own dword where
retail's arm-scoped one overlays. That is real and it is worth a whole score
(CSBI_StatzTabGruntBar::SerializeFields 94.03 -> EXACT). This entry is the
tree-wide follow-up: how to FIND those rows, and — more useful — the four
other reasons a frame differs, so the signature is not over-read.

## The instrument

    gruntz walls framescan --todo

For every paired row it reads the normalized pair objdiff scored and prints

* the **frame delta** — our `sub esp,N` (plus a `__chkstk` reservation) minus
  retail's; positive means we carry slots retail does not;
* the **residual** — diff lines left after masking BOTH the `[esp+N]`
  displacements and the intra-function branch targets.

Masking both is what makes it a sieve rather than a listing: everything a frame
shift perturbs disappears, so a pure frame-inflation hit reads residual ~0 and
sorts to the top. Rank by residual, not by delta.

## The census (580-row todo queue, 2026-08-23)

| frame vs retail | rows | of which residual <= 4 |
|---|---|---|
| LARGER  | 25 | **2** |
| SMALLER | 50 | 0 |
| EQUAL   | 505 | 101 |

The vein is nearly drained: two rows in the whole queue are a frame delta and
nothing else. Do not spend a sweep on this signature again without checking the
residual column first — 23 of the 25 larger-frame rows carry an unrelated
reconstruction difference that the frame delta is merely riding on.

The 101 equal-frame residual<=4 rows are a *different* and much larger vein,
and they are mostly instruction-SCHEDULING coins (see below), not source bugs.

## The false positives, i.e. what a positive delta usually means instead

**1. A carried variable retail recomputes.** `CGameLevel::ResolveFloorCollision`
0x15ede0 spends its extra dword on a loop-carried `hi` that retail
rematerialises as `y + 1` at the exit (`lea eax,[esi+0x1]; sub eax,edx; dec
eax`). Removing the variable removes the slot — and loses, because cl 5.0
folds `(y + 1) - b - 1` to `y - b` in *every* spelling (measured: inline,
named temp, split statements, `++y`, `(b + 1)` on the right, six-variant probe)
and then cross-jumps the two now-identical exits. See
[cl5-always-reassociates-plus-one-minus-one](cl5-always-reassociates-plus-one-minus-one.md).

**2. A spilled variable retail enregistered.** `CGrunt::RunMoveConfig` 0x65630
homes its `poseIdx` in `[esp+0x10]`; retail keeps it in EBX and reuses the same
register as its zero constant. The frame delta is the *consequence* of a
register-pressure difference, and the real divergence is elsewhere in the
function (there, a cold arm retail places at the tail).

**3. A cached address temp.** `CAniAdvanceCursor::Deserialize` 0x15ca70 caches
four `&m_field` addresses across a run of `Read` calls; retail keeps one of
them in EBP and slots the other three, we slot all four. The first divergence
is one register pick at the head of the block — a cursor position, not a scope.

**4. A named local where retail has a compiler temp.** `CSBI_GruntMachine::
SerializeFields` 0x0e8e00 (residual 0, the purest hit in the queue) has
`out`@0x10, `idx`@0x14, `reg`@0x18, `buf`@0x1c; retail has `out`@0x10,
`reg`+`idx` **sharing** 0x14, `buf`@0x18. `reg` (`g_gameReg->m_world`, checked
for NULL before the `switch`) is scope-allocated for us and live-range
allocated for retail, so retail can put the LOAD arm's `idx` on top of it once
the value is in ESI. A named function-scope local cannot express that, and
moving the declaration into the arms only makes the arm scope wider.

Rule of thumb: the frame delta tells you a slot count differs, never *why*.
Read the slot MAP (align the two sides on the masked stream and pair up the
`[esp+N]` displacements) before touching source — the pairing is what names
the variable.

## Reading the negative delta

A SMALLER frame is the inverse defect: retail keeps a local we folded into an
expression or scalarised out of an aggregate. Fifty rows carry it and none is
pure, but it is the direction that finds a MISSING variable, which is a
stronger structural claim than an extra one.
