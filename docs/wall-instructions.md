# Wall instructions — the matcher doctrine, in reverse

`.claude/agents/matcher.md` is the FORWARD doctrine: how to push a function to
100%. This is the REVERSE: how to recognize, characterize, and **document a wall**
so the campaign `@early-stop`s *efficiently* — on recognition — instead of
re-grinding the unsteerable.

## The rule

Whenever a function's match % moves — **improves OR decreases** — because of a
change that is NOT to that function's own body (a neighbor added to the same
aggregate TU, a shared base/grand-base edit, a calling-convention or type-model
change elsewhere, a reloc-name change, a /GR or /GX flag flip), DOCUMENT BOTH:

1. **The unrelated trigger** — exactly what changed. e.g. *"added `vfunc_16`'s
   866 B body to the `engine_label_stubs` aggregate (`src/Stub/All.cpp`)"*;
   *"made `AlbusMapBase` polymorphic"*; *"renamed extern `g_x`"*.
2. **The assembly-level effect** — what diverged in THIS function's codegen,
   verified with `llvm-objdump -dr` (base obj vs target obj). Name the mechanism:
   regalloc / spill-slot recoloring, frame-size shift (every `[esp+N]` +k),
   prologue shrink-wrap push set, vptr-stamp position, loop rotation / exit-block
   ordering, tail-merge of identical epilogues, demangled-vs-mangled reloc-name
   mismatch, etc.

## Where to record it

- In the function's `// @early-stop` comment — the byte-level wall reason (so it is
  never mistaken for an unfinished match, and the next worker sees the diagnosis).
- If the mechanism recurs or generalizes, add/extend a `docs/patterns/<wall>.md`
  entry (the corpus) and cite it. Reuse the existing ones first:
  `shrink-wrapped-callee-save-push.md`, `identical-return-epilogue-tailmerge.md`,
  `loop-preheader-vs-exit-block-order.md`, `reread-member-view-pointer.md`,
  `reloc-typing-vptr-global.md`, `big-seh-fuzzy-desync` family, …

## Why (the payoff)

- **Faster early-stops:** the next matcher recognizes the shape from the docs and
  `@early-stop`s immediately — no budget burned grinding a known-unsteerable wall.
- **Recoverability map for the final sweep:** a wall doc says whether a regression
  is recoverable and how. The canonical example: a body added to a shared aggregate
  TU that knocks a neighbor off 100% is recovered by **re-homing it into its own
  unit** (see the *always-integrate* rule in `.claude/commands/match.md`).
- **best-% / Fuzzy Max** already retains each function's prior high; the wall doc
  explains the gap so the dip is not mistaken for lost work.

## Distinction from matcher.md

matcher.md governs *"make it 100%"*. Wall instructions govern the **STOP decision
and the knowledge capture** — early-stopping becomes evidence-based (a named,
asm-verified mechanism), not a guess or a give-up.
