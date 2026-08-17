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
| [cl5-callcrossing-ebx-first-by-use-schedule.md](cl5-callcrossing-ebx-first-by-use-schedule.md) | the first call-crossing value **used** after a call takes EBX — a use-order lever | a whole-body ESI/EDI/EBX role swap with no source cause |
| [cl5-inline-budget-is-arithmetic-you-can-compute.md](cl5-inline-budget-is-arithmetic-you-can-compute.md) | the inline budget is **computable arithmetic** — every constant is a literal in `c2.exe`, and a decline deficit measures MISSING CALLER STRUCTURE in cb units | a per-function inlining mood to grind against |
| [cl5-crossjump-merges-suffixes-not-blocks.md](cl5-crossjump-merges-suffixes-not-blocks.md) | a duplicated tail is a **CFG fact** — cl cross-jumps every common SUFFIX unless a join, per-arm EH state, or an IL-empty epilogue blocks it; and `c2.exe` names its own passes | an unexplained block-placement coin |
| [cl5-c2-register-picker-is-a-rotating-cursor.md](cl5-c2-register-picker-is-a-rotating-cursor.md) | `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` is a **rotation order with a persistent cursor**, and a two-pass "already-used first" filter | a preference ranking, and a scalar pair whose ECX/EDX order a source spelling should reach |

Plus one index, not a revelation:
[**data-matching-checklist.md**](data-matching-checklist.md) — the terse whole-checklist for
starting data matching in a new project, with pointers into the long form.
