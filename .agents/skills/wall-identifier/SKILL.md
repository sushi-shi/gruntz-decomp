---
name: wall-identifier
description: Classify a Gruntz matching WALL before spending effort on it. When a reconstruction plateaus below 100% and no spelling obviously closes it, name WHICH cl 5.0 decision diverged - inline/call-set, control flow, register/schedule, or masked/referent - and route to the lever for that class. Start with `gruntz walls diagnose <rva>`. Use when a function is stuck, when triaging plateaus, when asked "why won't this match" or "what kind of wall is this". Complements `matcher` (reconstructs) and `permute` (breaks proven codegen residue); this one DIAGNOSES.
---

# wall-identifier — classify the wall before fighting it

The pinned cl 5.0 SP3 is deterministic: `bytes = f(preprocessed TU, flags)`.
Retail came out of the same `f` from real source, so every function is
reducible in principle; the job is finding which *input* differs. The unit of
reproduction is the whole TU: some residue is front-end state no local body
edit can reach. Each class below has a different lever, and two of the four are
not permute problems at all.

## Start here

- `gruntz walls diagnose <rva> --asm` — classifies the residual from the same
  normalized base/target pair objdiff scores (no recompile): first divergence
  class, call/branch/return counts, and both sides' leading instructions.
- `gruntz walls semdiff <rva>` — operand, FP-opcode, constant, and ordered
  referent comparison over that pair.
- `gruntz sema match <unit|rva>` — current % vs best-ever (proven headroom?).
- `gruntz sema disasm <rva> --blocks` — retail-only basic-block view.
- `gruntz verify assert-relocs <rva>` — the actual referent set, unmasked.

## The four classes, in routing order

Do not call a wall class N while class N-1 still diverges.

| class | deciding signal | lever |
|---|---|---|
| **inline / call-set** | out-of-line CALL multiset differs | body completeness, inline boundary, or duplicated call tail |
| **control flow** | block, branch, or ret COUNTS differ | source construct — structural matcher work |
| **register / schedule** | counts and branch sequence agree; operand order, spills, coloring differ | source-shape checklist, then classified `gruntz permute state\|variants` |
| **masked / referent** | masked diff identical but score < 100 | referent identity — labeling work, not codegen |

### inline / call-set

`/O2` on cl 5.0 is `/Ob1`: unmarked ordinary non-template functions stay calls.
Instantiated template members can expand without `inline`
(`docs/patterns/vc5-template-members-inline-without-inline-keyword.md`).
Eligible bodies expand under a per-CALLER budget
(`docs/patterns/inline-budget-emits-ool-comdat.md`; cl 5.0 has no VC6 S=14
cliff).

- The class is **inline / call-set**, not "inline budget". A call-count delta
  can also be a duplicated or merged call-carrying exit tail, and an
  equal-count callee substitution can be a wrong identity. Name the differing
  sites before choosing a lever.
- `REPEATED-SITE DELTA` (a direct callee present on both sides with different
  counts) does not distinguish a per-site inline decision from a cross-jumped
  call tail. Locate the sites and check the retail jumps first.
- A missing expansion usually means the CALLER's body is incomplete — budget
  follows statement mass. Finish the caller before touching the callee.
- `gruntz walls inline-model --gap <rva>` reports candidacy evidence;
  `--measure-cb` titrates the budget with the real compiler.
  `llvm-nm build/objdiff/base/*.obj | grep <mangled>` screens which TUs emit a
  COMDAT.
- Never land a forcing device (PMF ref, dllexport, artificial caller) to
  materialize a COMDAT.

### control flow

A count mismatch is a reconstruction problem. cl 5.0's exit merging has three
source-selected regimes — separate returns, `goto fail` to one exit, and a
total `||`/`&&` collapse — plus loop-form effects (`while` versus
`do/while`, backward gotos). See the matcher lever catalog, sections 6-7.

One narrow exception is proven: when the first real divergence is an earlier
register rotation and every extra edge is confined to the returns of an
inlined value-only accessor, register availability can decide whether global
optimization factors the caller tail. That is a branch- or return-count delta
downstream of coloring with no authored CFG difference. Require the complete
signature — same guards, call set, constants, and ordered referents; only the
accessor-return tail differs; source-shaped result/receiver/scope controls are
byte-flat. It is not permission to relabel an ordinary branch mismatch from
counts alone: follow the first divergence.

### register / schedule

Reached by elimination only. First exhaust the source-shape checklist
(matcher `references/levers.md`): widths, cv/ref boundaries,
local census and lifetimes, helper boundaries, statement grouping. One
misplaced register op can mean the TYPE is wrong (a member array modeled as
scalars, a lost aggregate). Then use classified `gruntz permute state|variants`
(the `permute` skill). TU-global effects exist
(`docs/patterns/tu-state-probe-family-decides-reachability.md`): a flat probe
sweep is evidence about the probe, not the function. Probes are diagnostics:
bank the MAX, then delete them.

### masked / referent

Objdiff relocation scoring is strict (target name/address, pointed-to data,
DIR32 addends), and a masked diff by construction cannot show a wrong callee.
If the masked bodies agree while the score sits below 100, audit with
`gruntz verify assert-relocs`, fix the labeling/identity model, and do not
permute it.

## What does NOT transfer from HoMM3

The HoMM3 ancestor of this doctrine is VC6-specific. Do not use without
re-proving on cl 5.0: the VC6 register-allocator model (hypothesis only here),
`/Ob2` semantics and the S=14 save-gate cliff. The cl 5.0 IL tap is proven
(`/d1il<prefix>` capture, `/d2il<prefix>` feed), but there is no production
IL-diff verb.

A reproducibly bounded residue stays visible through the derived inventory,
the MAX ledger, and a valid `@early-stop` marker; never a hand-kept wall
ledger. Consolidate a reusable lever under `docs/patterns/README.md` only with
bounded A/B evidence.
