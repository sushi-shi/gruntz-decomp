---
name: match-checklist
description: Apply evidence-backed source-shape checks to Gruntz function matching without writing per-function plans. Use before editing below historical MAX, revisiting a hard wall, recovering lost headroom, or declaring a residue bounded; mine exact-match history and consider applicable helpers and source families.
---

# Gruntz per-function match checklist

Use this skill together with `matcher`; use `wall-identifier` to classify the
current pair. This skill governs **what must be considered before and during one
function campaign**. It does not replace the repository's lowest-MAX queue,
source-model rules, or verification gates.

Follow `matcher`'s validation cadence: run test suites only at authorized PR
squash merge, never during matching, ordinary commits, or pushes. Here,
`tested` means a measured source A/B compilation and retail comparison; it
does not require running a unit/regression or compiler-backed test suite.

## Evidence checks, not plan documents

Do not create per-function plan files, including arrival plans or temporary
`*-plan.md` files, or publish a formal Function Match Plan unless the user
explicitly asks for one. Matching work should produce source changes and measured
results, not planning paperwork. A brief commentary update naming the target,
evidence-backed hypothesis, and next compiler control is enough.

Use [references/attempt-matrix.md](references/attempt-matrix.md) as a reasoning
aid; do not copy it into a document or fill a ceremonial matrix before editing.
Still inspect the current/bank/historical scores, source fingerprint, complete
owner and history, first divergence, call/CFG structure, constants, and ordered
referents. Consider the applicable source-shape families, especially real inline
helpers, and distinguish measured controls from untried hypotheses.

Keep concise results in the handoff. Consolidate reusable compiler findings
under the admission rules in `docs/patterns/README.md`; do not add one entry per
closure. Keep durable adoption/rejection/defer decisions, their evidence,
and reopening conditions only in the canonical lineage ledger. Do not create a
hand-maintained wall ledger. The optional
[plan template](references/function-plan.md) is only for an explicitly requested
written plan.

## Required evidence pass

Inspect this evidence before source edits:

```sh
gruntz walls diagnose <rva> --asm
gruntz walls semdiff <rva>
gruntz sema disasm <rva>
gruntz sema xref <rva>
```

Also read the whole source function, declaration, callers, callees, adjacent
family members, class layout, and any source-lineage candidate. Audit raw
constants and ordered relocations. If identity or layout is uncertain, resolve
that before treating the row as codegen residue.

Mine history every time:

```sh
git log -S'<function-name>' --all -- src include config/match_baseline.tsv docs/patterns
git log -G'<mangled-name>.*100\.0000' -- config/match_baseline.tsv
git show <candidate-commit>
```

For `hist_pct > best_pct`, inspect the exact source-hash transition and
`gruntz walls priors` before inventing new forms. For other walls, search
`docs/patterns/INDEX.md` for a relevant mechanism; it is not a per-function
catalog or proof that a wall is bounded. Read selected entries completely. Use the historical
catalog in [references/exact-match-levers.md](references/exact-match-levers.md)
to ensure the search is not limited to the first familiar explanation.

## Match and compare

Prioritize hypotheses by evidence, not convenience:

1. surviving/source-oracle body and complete family;
2. identity, ABI, ownership, type, layout, referent, or missing-body defects;
3. authentic inline/helper/macro/operator/constructor boundaries;
4. local census, scope, lifetime, initialization, parameter reuse, and
   statement/control-flow shape;
5. expression, loop, standard-library/MFC idiom, and evaluation order;
6. classified compiler-state experiments only after semantics, call set, CFG,
   constants, and referents are credible.

For each candidate, identify the exact source change and expected emitted delta.
Compile the real TU and compare from the first real divergence. Report the
measured score and relevant size/frame, call/CFG, semantic, and referent changes
concisely; no separate plan file is required. A single dip does not reject a
sourced or structurally convergent base: apply exploratory descent and compose
the next independently evidenced lever.
Before composing, confirm the desired feature was absent from the original
baseline.

Do not run a Cartesian/permutation campaign until the evidence shows that all
applicable structural families were checked and the current wall is genuinely
register/schedule or TU-state. Do not retain probes, unused declarations,
artificial locals, or distorted source.

“Checked” means one of two things: a real-TU A/B was compiled, or retail/source
evidence proves that exact form inapplicable. A score from an older source hash
may serve as the A/B only when the historical body and relevant TU context are
identified. Do not replace the attempt matrix with one broad experiment that
mixes several independent levers.

## Exit criteria

Distinguish completion from a short batch:

- Claim exact closure only from actual compilation/comparison of the intended
  fingerprint; bank only when authorized.
- Claim a bounded residue only after applicable families have evidence-backed
  dispositions and the remaining wall and negative controls are identified.
- A user-directed short batch or target switch may stop sooner. Mark remaining
  hypotheses open, give a concise handoff, and move on without a plan document.

Before commit, re-audit raw constants and ordered referents, refresh compilation
and comparison with `gruntz build base compare`, inspect MAX results, and stage
only the focused source, reusable pattern documentation, and focused baseline
rows. Defer full gated builds and test suites until authorized squash merge;
record that deferral rather than claiming the final candidate is fully tested.
