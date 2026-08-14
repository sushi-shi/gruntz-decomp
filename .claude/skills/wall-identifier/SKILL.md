---
name: wall-identifier
description: Classify a Gruntz matching WALL before spending effort on it. When a reconstruction plateaus below 100% and no spelling obviously closes it, name WHICH cl 5.0 decision diverged - inline/call-set, control flow, register/schedule, or masked/referent - and route to the lever for that class. Start with `gruntz sema diagnose <rva>`. Use when a function is stuck, when triaging plateaus, when asked "why won't this match" or "what kind of wall is this". Complements `matcher` (reconstructs); this one DIAGNOSES - register/schedule walls are parked with `@early-stop`, not ground (the permute machinery is retired).
---

# wall-identifier — classify the wall before fighting it

The pinned cl 5.0 SP3 is a deterministic function: `bytes = f(preprocessed TU,
flags)`. Retail came out of the same `f` from real source, so every function is
reducible in principle — the job is finding which *input* differs. The unit of
reproduction is the **whole TU**, not the function: some residue is TU-global
front-end state that no local body edit can reach, but matching the TU context
can. Classify first; each class has a different lever and two of the four are
not codegen-residue problems at all.

## Start here

`gruntz sema diagnose <rva>` — classifies the residual from the same base/target
pair objdiff scores (no recompile) and routes it in the order below. The manual
equivalents, when you need the evidence itself:

- `gruntz sema match <unit|rva>` — current % vs best-ever (is there proven headroom?)
- `gruntz sema disasm <rva> --diff` — masked asm diff; rc 0 = byte-shape agrees
- `gruntz sema disasm <rva> --branches --diff` — branch/ret counts + symbolic sequence
- `gruntz sema disasm <rva> --blocks --diff` — block-aligned CFG view
- `python -m gruntz.audit.assert_relocs <rva>` — the actual referent set, unmasked

## The four classes, in routing order

Do not call a wall class N while class N-1 still diverges.

| class | deciding signal | lever |
|---|---|---|
| **inline / call-set** | out-of-line CALL multiset differs | body completeness / inline budget — see below |
| **control flow** | block, branch, or ret COUNTS differ | source construct — structural matcher work, never ordering experiments |
| **register / schedule** | counts and branch sequence agree; operand order, spills, coloring differ | park with `@early-stop` + byte-level reason (permute machinery retired) |
| **masked / referent** | masked diff rc 0 but score < 100 | referent identity — labeling work, not codegen |

### inline / call-set

`/O2` on cl 5.0 is `/Ob1`: an unmarked function is NEVER auto-inlined, and an
`inline`-marked one is expanded under a per-CALLER budget that is measured and
modeled — `docs/patterns/inline-budget-emits-ool-comdat.md` re-validated the
sibling HoMM3 formula (`budget = clamp(2*cb(caller), 1000, 35000)`, free below
cb 0x28, nested expansions split the remainder) on our compiler, including where
the two compilers diverge (cl 5.0 has no VC6 S=14 cliff). Consequences:

- A missing inline expansion usually means the CALLER's body is incomplete —
  budget follows statement mass. Finish the caller before touching the callee.
- `llvm-nm build/objdiff/base/*.obj | grep <mangled>` screens which TUs emit a
  COMDAT; compare against retail's placement.
- Depth-2 declines, the one live `inline_depth` pragma, and a genuine cl 5.0
  wrong-code drop are catalogued: `inline-depth-two-declines-in-the-largest-caller.md`,
  `msvc5-inline-depth-zero-is-the-only-live-lever.md`,
  `ob1-budget-drops-the-inlined-dtor-and-the-return.md`.
- Never land a forcing device (PMF ref, dllexport, artificial caller) to
  materialize a COMDAT — `ob1-inline-budget-divergence.md`.

### control flow

A count mismatch is a reconstruction problem. cl 5.0's exit-merging has exactly
three source-selected regimes (separate returns / `goto fail` / `||`-collapse):
`goto-fail-shares-one-exit-block.md` is the master entry;
`dup-exit-means-a-shared-goto-label.md`, `while-not-do-while-keeps-the-inline-return.md`,
`do-while-duplicates-the-leading-call.md`, `void-vs-bool-return-epilogue-split.md`,
`backward-goto-sinks-its-target-region.md` cover the common shapes.
(The tree-wide exit_merge_sieve screen is retired; check the shape per function.)

### register / schedule

Reached by elimination only. The permute machinery is RETIRED: park the wall
with `@early-stop` and the byte-level reason; a genuinely bounded wall is
recorded in `wall-break.md`. Caution: one misplaced register op can mean the
TYPE is wrong (a member array modeled as scalars, a lost aggregate) — re-check
the model before parking. Reading rule: `zero-register-compare-is-against-zero.md`.

### masked / referent

Objdiff reloc scoring is strict (target name/address, pointed-to data, DIR32
addends all participate), and the masked diff by construction cannot show a
wrong callee. If `--diff` returns rc 0 while the score sits below 100, the
divergence is referent identity: audit with `assert_relocs`, fix the labeling /
identity model - this class is labeling work, not codegen.

## What does NOT transfer from HoMM3

The HoMM3 `wall-identifier` doctrine (this file's ancestor) is VC6-specific in
its mechanics. Do not use here without re-proving on cl 5.0:

- every `homm3 vc6 *` command (different repo, different compiler);
- the VC6 register-allocator model (preference order, first-fit by creation
  order) — nothing in this tree validates it for cl 5.0; hypothesis only;
- `il-diff` / C1XX IL capture — the cl 5.0 recipe is PROVEN
  (`/d1il<prefix>` capture, `/d2il<prefix>` feed; normalization rules and the
  probe-kind handle-stride table in `build/il-probe/REPORT.md` and the
  quantified section of `tu-state-probe-family-decides-reachability.md`), but
  the production `gruntz sema il-diff` verb is not built yet;
- `/Ob2` semantics and the S=14 save-gate cliff (cl 5.0 is `/Ob1`, no cliff).

A lever proven here goes in `docs/patterns/` + `INDEX.md` with the A/B evidence.
A bounded wall goes in `wall-break.md`: before/after historical MAX, retail
evidence, retained lever, negative controls, remaining mismatch class. Walls get
BROKEN, not documented — the ledger entry comes after the search stalls, not
instead of it.
