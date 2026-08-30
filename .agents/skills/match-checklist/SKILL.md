---
name: match-checklist
description: Build and execute an evidence-backed hypothesis plan for every Gruntz function-matching attempt. Use before editing any function below historical MAX, revisiting a hard wall, recovering lost headroom, or declaring a residue bounded; it mines exact-match history and requires every applicable source-shape family—including inline helpers—to be considered and recorded.
---

# Gruntz per-function match checklist

Use this skill together with `matcher`; use `wall-identifier` to classify the
current pair. This skill governs **what must be considered before and during one
function campaign**. It does not replace the repository's lowest-MAX queue,
source-model rules, or verification gates.

## Non-negotiable deliverable

Before the first source edit, publish a concise **Function Match Plan** using
the template in [references/function-plan.md](references/function-plan.md).
Read and copy the applicable rows from
[references/attempt-matrix.md](references/attempt-matrix.md) into that plan.
The matrix is executable: every row must end as `tested`, `proved`,
`checked — no evidence`, or `proved inapplicable`; merely naming a family is
not completion.
The plan is specific to the selected function, not a generic list. It must:

1. state the current/bank/historical score, size, owner TU, source fingerprint,
   wall class, first real divergence, call/branch/return/relocation counts, and
   ordered-referent verdict;
2. include the function's own history and the closest exact historical
   precedents;
3. give a prioritized queue of concrete source A/Bs, each with the evidence
   that licenses it and the machine-code feature it is expected to move;
4. mark every checklist family `candidate`, `checked — no evidence`, `tested`,
   or `proved inapplicable`, with a short reason. No family may be silently
   skipped; and
5. enumerate the concrete source forms to compile within every candidate
   family. In particular, “checked inlining” is invalid unless the plan records
   the distinct helper/macro/visibility forms from the attempt matrix and their
   individual verdicts.

Keep the live plan in commentary/task notes. Do not create a hand-maintained wall
ledger. Persist only reusable compiler findings in `docs/patterns/` and durable
lineage decisions in their designated ledger.

## Required evidence pass

Do this before planning edits:

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
`docs/patterns/INDEX.md` by the observed instruction, CFG, C++ construct, and
wall-class tags. Read the selected pattern files completely. Use the historical
catalog in [references/exact-match-levers.md](references/exact-match-levers.md)
to ensure the search is not limited to the first familiar explanation.

## Plan and execute

Prioritize hypotheses by evidence, not convenience:

1. surviving/source-oracle body and complete family;
2. identity, ABI, ownership, type, layout, referent, or missing-body defects;
3. authentic inline/helper/macro/operator/constructor boundaries;
4. local census, scope, lifetime, initialization, parameter reuse, and
   statement/control-flow shape;
5. expression, loop, standard-library/MFC idiom, and evaluation order;
6. classified compiler-state experiments only after semantics, call set, CFG,
   constants, and referents are credible.

For each candidate, record the exact source change and predicted emitted delta.
Compile the real TU, compare from the first real divergence, and update the plan
with score, size/frame, call/branch/return/relocation counts, semantic diff, and
verdict. A single dip does not reject a sourced or structurally convergent base:
apply exploratory descent and compose the next independently evidenced lever.
Before composing, confirm the desired feature was absent from the original
baseline.

Do not run a Cartesian/permutation campaign until the plan shows that all
applicable structural families were checked and the current wall is genuinely
register/schedule or TU-state. Do not retain probes, unused declarations,
artificial locals, or distorted source.

“Checked” means one of two things: a real-TU A/B was compiled, or retail/source
evidence proves that exact form inapplicable. A score from an older source hash
may serve as the A/B only when the historical body and relevant TU context are
identified. Do not replace the attempt matrix with one broad experiment that
mixes several independent levers.

## Exit criteria

A function campaign may end only when one of these is true:

- it is exact and a full `gruntz build` banks the intended fingerprint; or
- the complete checklist has evidence-backed dispositions, the residue is
  reproducibly bounded, and the report identifies the remaining wall class and
  the tested negative controls.

Before commit, re-audit raw constants and ordered referents, run the full build
and MAX gate, and stage only the focused source, reusable pattern documentation,
and focused baseline rows.
