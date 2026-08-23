# Relevations

Findings that changed **how we look**, not just what we fixed.

A pattern doc in `docs/patterns/` answers *"the disassembly looks like X — what do I write?"*.
A file here answers a different question: *"what is this artefact actually telling me, that I
had been reading as noise?"* Each one names a signal the project was masking or ignoring,
shows the evidence that turned it into an oracle, and states what it costs to ignore.

Entry requirements — a revelation is not a tip:

* **A concrete worked example** with real assembly, real C++, and the real diff. No sketches.
* **Measured before/after.** If it did not move a number, it is a hypothesis, not a revelation.
* **The counter-example** where available: what looks like this signal and *is not* a defect.
  Half the value is stopping the next reader chasing a false positive.

| file | the signal | what it was being read as |
| :-- | :-- | :-- |
| [funclet-is-a-type-oracle.md](funclet-is-a-type-oracle.md) | a `/GX` unwind funclet's jump target names a member's or base's **type** | compiler noise to be masked |
| [call-count-is-a-defect-oracle.md](call-count-is-a-defect-oracle.md) | a callee retail has and we don't — **absence**, which no percentage measures | a low score to grind later |
| [byte-exact-can-still-crash.md](byte-exact-can-still-crash.md) | a 100%-exact function fed garbage by its callers' **argument protocol** | proof the function was correct |
| [eh-band-is-where-a-declined-inline-shows.md](eh-band-is-where-a-declined-inline-shows.md) | an unwind-state **count** short — a body retail expanded and our compiler declined | a mid-% constructor to park |
| [when-the-compiler-owns-the-name-use-content.md](when-the-compiler-owns-the-name-use-content.md) | a toolchain-minted symbol is a **coordinate, not a name** — match its bytes | data that looked unreachable, or churn that looked like regression |
| [cl5-callcrossing-ebx-first-by-use-schedule.md](cl5-callcrossing-ebx-first-by-use-schedule.md) | cl 5.0 keeps FOUR callee-saved GPRs (EBX ESI EDI EBP) in play before it spills — but the use-order lever it claims is **falsified**, see `wall-reasons-allocation.md` R2a | a whole-body ESI/EDI/EBX role swap with no source cause |
| [cl5-inline-budget-is-arithmetic-you-can-compute.md](cl5-inline-budget-is-arithmetic-you-can-compute.md) | the inline budget is **computable arithmetic** — every constant is a literal in `c2.exe`, and a decline deficit measures MISSING CALLER STRUCTURE in cb units | a per-function inlining mood to grind against |
| [cl5-crossjump-merges-suffixes-not-blocks.md](cl5-crossjump-merges-suffixes-not-blocks.md) | a duplicated tail is a **CFG fact** — cl cross-jumps every common SUFFIX unless a join, per-arm EH state, or an IL-empty epilogue blocks it; and `c2.exe` names its own passes | an unexplained block-placement coin |
| [cl5-c2-register-picker-is-a-rotating-cursor.md](cl5-c2-register-picker-is-a-rotating-cursor.md) | `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` is a **rotation order with a persistent cursor**, and a two-pass "already-used first" filter | a preference ranking, and a scalar pair whose ECX/EDX order a source spelling should reach |
| [cl5-globalopt-has-a-511-handle-phase.md](cl5-globalopt-has-a-511-handle-phase.md) | an unchanged function can alternate on an exact **511-symbol-handle `/Og` phase**, while a flat function has no phase-sensitive choice under that campaign | random compiler instability, or permission to keep state probes |
| [wall-reasons-globalopt.md](wall-reasons-globalopt.md) | the alias model is **one byte in the C1→C2 IL**, set to the most pessimistic value — every reload, pinned store order and un-hoisted loop invariant follows from it; and IVs coalesce on equal **byte stride**, never on pointer type | a pile of one-off source tricks, and "ours hoists where retail re-derives" as a compiler mood |
| [wall-reasons-layout.md](wall-reasons-layout.md) | pre-layout block order is **topological** (after the last predecessor), while later value factoring can create backward edges in the final graph; the unconditional tail cross-jump is **`/Os`-gated and off in our build**, switch xlat obeys `3·range > 4·targets + 12`, frame slots are declaration+first-use order, and an EH state index is a lexical **count** | one "block-placement coin" covering four unrelated decisions |
| [wall-reasons-allocation.md](wall-reasons-allocation.md) | the allocation/selection wall catalogue: a value's register is **the rotation cursor at the tuple that requests it**, call-crossing values bind ESI/EDI/EBX/EBP in **definition** order, and `test r,r` vs `cmp r,<reg>` counts the function's **zero stores** | one undifferentiated "regalloc/scheduling" verdict to park |

| [a-control-that-cannot-fail-is-not-a-calibration.md](a-control-that-cannot-fail-is-not-a-calibration.md) | every paired sieve's *"returns zero over the exact rows"* is **vacuous for byte-keyed comparisons** — the two inputs are identical there, so `f(x)==f(x)` cannot fail; the one live cell is the two objects' differing **relocation tables** | proof a sieve measures what it claims |

Reference indexes, not revelations:

* [**data-matching-checklist.md**](data-matching-checklist.md) — the terse whole-checklist
  for starting data matching in a new project, with pointers into the long form.
* [**cl5-c2-function-map.md**](cl5-c2-function-map.md) — Ghidra addresses, working semantic
  names, confidence, and causal controls for the external VC5 SP3 back-end routines
  already reverse-engineered by this project.
