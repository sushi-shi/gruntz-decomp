# Data matching: the whole checklist

Terse index for starting data matching in a **new** decompilation project. Each line is a
thing that must be true, and where in this repo the long form lives. Ordered by the cost of
getting it wrong.

## Before any number means anything

1. **You generate BOTH sides.** Unclaimed data is never compared — objdiff cannot surface
   what you did not model. → `data-matching-problems.md` §1
2. **Coverage is an ADDRESS property.** Union of retail ranges, each byte once. Summing
   per-object section sizes double-counts folded COMDATs. → §4
3. **Report three numbers, never one.** gross coverage · reconstructable coverage (÷ eligible
   after a proven reachability partition) · fidelity (of enrolled, share byte-equal).
4. **Uncertainty must never flatter.** Unclassified bytes stay eligible; a "not measurable"
   column keeps every percentage a floor.
5. **A wrong model that scores 100% is worse than a low score.** A low score is a worklist.

## The seven ways a datum lies

| # | failure | how it hides | where |
| :-- | :-- | :-- | :-- |
| 1 | **wrong pin** | code compensates with a bias; both sides agree | `data-matching-problems.md` §2 |
| 2 | **wrong extent** | slack is zero-filled and looks like padding | §2 |
| 3 | **wrong referent** | placeholder bytes match; the pointer aims elsewhere | §3b |
| 4 | **wrong addend** | `sym+K` — stored INLINE in the masked displacement | §3c |
| 5 | **wrong field read/written** | same instruction, different offset | `store_offsets` |
| 6 | **wrong TYPE** | destruction path only | `funclet-is-a-type-oracle.md` |
| 7 | **absent entirely** | a body retail expanded and we call; no percentage measures absence | `eh-band-is-where-a-declined-inline-shows.md` |

**A compensated defect is not corrected until every compensation is gone.** Grep for the
bias, do not trust the pin. (`g_clut` shipped a rendering bug for months this way.)

## Tools, in build order

```
gruntz audit data_access_map --symbol <name|rva>   # is it even compared? --findings unclaimed
gruntz audit data_denominator                      # the reachability partition (regenerate, review)
gruntz audit data_relocs / assert_relocs           # is the referent right?
python -m gruntz.audit.reloc_addends               # is the ADDEND right? (multisets, not positions)
python -m gruntz.audit.store_offsets               # is it WRITTEN where retail writes it?
python -m gruntz.audit.immediates --strong         # is a bare constant wrong?
python -m gruntz.audit.eh_band --census / --check  # is the TYPE right?
python -m gruntz.audit.link_sections --undersized  # is the extent right?
```

## Scoring config that must be deliberate

- `functionRelocDiffs=all` — target name/address and pointed-to data both score.
- Absolute DIR32 addends score too; the pinned patch preserves this if the mode is
  relaxed during an experiment. → §3c
- A name-only mismatch is evidence to reconcile, not automatically a source bug:
  prove whether it is an alias or an interior-address naming defect.
- A relocated word cannot be byte-compared at all — the linker writes it. → §3
- Section-name grouping (`.text$x`, `.xdata$x`) makes a reloc compare FALSE even when the
  target is right; cost lands on the *owner*, not the target.

## Traps that cost real time here

- **Extent by alignment is not proof.** c2 aligns within a contribution; the linker places
  it. Absolute-RVA alignment arguments reject 131 known-good controls.
- **`header + 1` overshoots** when the trailing array is not at `sizeof` (0x20 vs 0x24).
- **The delinker's nearest-symbol guess** must *contain* the RVA, or it beats an exact PDB
  symbol (1,020 relocs decomposed past their symbol's end).
- **Naming ≠ enrollment.** A datum can be named and still unenrolled, so it pairs as a bare
  `DAT_<va>` with a guessed extent and forfeits its whole section.
- **A fidelity drop can be the metric getting honest.** Enrolling more, or resolving
  referents correctly, lowers it. Check *why* before treating it as a regression.

## The one-line summary

Percentage measures **similarity of what is present**. Every "what did retail have that we
don't" question — a datum, a call, a member, a type — needs its **own census**.
