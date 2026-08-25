# A commutative `memberA + memberB` of the SAME object: which one is loaded first is canonicalized
tags: cpp:member cpp:int | asm:mov asm:add asm:lea | topic:wall topic:regalloc
symptoms: two `mov r,[obj+N]` whose DISPLACEMENTS are swapped, the `add`/`lea` that consumes them byte-identical, ~99.98%
confidence: 9/10
variants: commutative-imul-operand-in-eax.md

Two members of the same object summed (`t->m_screenY + t->m_extent.top + dy`) lower to two
loads and one `add`/`lea`. The `add`/`lea` bytes are the same either way, so the ONLY diff is
which member's displacement is in the first `mov`. cl5 picks it in the back end; the source
expression cannot reach it.

```cpp
// all of these emit the SAME load order - the pick is not in the source:
t->m_screenY + t->m_extent.top + dy        t->m_extent.top + t->m_screenY + dy
t->m_screenY + (t->m_extent.top + dy)      dy + t->m_screenY + t->m_extent.top
i32 sy = t->m_screenY; sy + t->m_extent.top + dy   // and the m_extent.top-first twin
i32 py = t->m_screenY; py += t->m_extent.top; py += dy;   // compound form
```
```asm
base:   mov eax,[edx+0x138]  /  mov ebx,[edx+0x60]   ; then  add eax,ebx
retail: mov eax,[edx+0x60]   /  mov ebx,[edx+0x138]  ; then  add eax,ebx   (same bytes)
```
WALL. Evidence: `CGameLevel::ProbeHeadSoft` 0x160450 / `ProbeFootSoft` 0x160080 /
`ProbeFootBlocked` 0x160210 / `HoldMove` 0x15ff20 all sit at 99.98-99.99% on exactly this
4-byte pair. **It is context-, not source-, determined**: a standalone `struct AB : CGameLevel`
replica compiled from the IDENTICAL source picks retail's order for the foot pair and ours for
the head pair, so it is TU-cumulative compiler state, not a spelling. ~35 expression AND
statement forms tried across two harnesses; none moved it.
Distinct from [[commutative-imul-operand-in-eax]] (an `imul`'s reg-vs-mem operand).

## Mixed-kind island closure (2026-08-18)

The modern classified campaign made the context result reproducible on
`ProbeHeadSoft`. Baseline source hash `613cedf2f37f` emitted 99.977010%; four
deterministic mixed declaration-forest islands all emitted the one other normalized
target state, and that state was 100.000000% with exact 0xd6 extent and zero
relocations on both sides. Trial 1 replayed exact and banked MAX for the unchanged
source fingerprint; the generated declarations were discarded.

The source-only negative control covered both expression and declaration structure:
reversed operands, incremental `+=`, named member operands, `py` before `px`, a
partially initialized `py` before `px`, and member temporaries on either side of `px`.
All nine controls emitted the baseline object byte-for-byte. The exact island differs
from baseline only in the two instructions starting at +0x8: retail/island load
`m_screenY` then `m_extent.top`, while baseline loads those two members in the reverse
order; 206 of the 214 bytes agree.

This corrects the older vague “back-end state” wording above. C1XX supplies the changing
global symbol handles, but the target's own IL stays byte-identical and C2's `/Og` pass
selects the alternate order on an exact 511-handle phase, as proven in
[`../relevations/cl5-globalopt-has-a-511-handle-phase.md`](../relevations/cl5-globalopt-has-a-511-handle-phase.md).
The declaration-kind strides remain those proven by
[`tu-state-probe-family-decides-reachability.md`](tu-state-probe-family-decides-reachability.md):
the body has no honest source spelling that reaches the alternate order, but mixed
declaration kinds move the unchanged body to it. Treat the frontier as evidence of
missing authentic TU declarations, never as permission to retain the probes.

## The rule cl actually applies (standalone cl A/B, 2026-07-29)

Compiled OUTSIDE the tree with the real `cl /O2`, over 20 spellings of
`sink = t->m_extent.top + t->m_screenY` (member offsets 0x138 / 0x60):

* **cl loads the LOWER member displacement first, into the accumulator** - always.
  Source operand order, parenthesisation, `+=` decomposition, one-or-two named locals,
  RECT-field vs flat-int members, two *different* pointer variables, a `const RECT*`
  hop, a wrapper `static int add(int,int)`, `- (-b)`, use inside a loop, an extra use of
  one operand, an `unsigned` cast, and passing the sum as a call argument all emit the
  identical `mov acc,[p+0x60] / mov r,[p+0x138] / add acc,r`.
* **The ONLY thing that defeats it is an intervening potentially-aliasing STORE.**
  `sink = t->m_extent.top; sink += t->m_screenY;` (a global between the two loads)
  emits `mov acc,[p+0x138] ... mov r,[p+0x60] / add acc,r` - retail's order. A local
  between them does not (locals do not alias).

So retail's higher-offset-first sites are NOT reachable by re-spelling the expression:
either retail's source had a store between the two reads that our reconstruction has
dropped (nothing in these bodies is missing - the sizes match), or the pick is the
TU-cumulative back-end state this file already documents. Same wall in
`CCheckpointTrigger::CCheckpointTrigger` @0x10ee20 (99.87 %, `m_frameImage->m_anchorY +
m_screenY`) and `CDDrawChildGroup::SumWeighted` @0x15aaf0 (99.93 %, a 4-member sum
whose 1st and 3rd loads swap) - both measured against real objdiff %, not diff lines.
