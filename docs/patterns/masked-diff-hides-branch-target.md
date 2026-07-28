# "identical asm" from `sema disasm --diff` can still be a CONTROL-FLOW BUG — branch displacements are masked

**Tags:** cpp:branch | asm:jcc | topic:tooling topic:scoring-artifact

## Symptom

`gruntz sema disasm <rva> --diff --lite` prints

```
identical asm (40 instruction(s); addresses/relocs masked)
```

yet `report.json` says the function is **not** 100% (e.g. 99.875%), and
`llvm-objdump -dr` of the base and target objects shows byte-identical
instruction bytes *and* identical relocation symbol names.

## Cause

`--diff` masks every address operand as `<tgt>` / `<addr>` so that reloc-bound
call/jump targets do not show up as spurious diffs. That masking also hides
**intra-function conditional-branch displacements** — a `je` that goes to a
*different basic block* on the two sides is printed as `je <tgt>` on both and
compares equal.

So a genuine divergence in control flow can render as "identical asm".

## Detection

When a `--diff` says identical but the score is below 100, re-read both sides
**with addresses** and compare the jump displacement bytes:

```
gruntz sema disasm <rva> --target        # e.g.  155f7b: 74 2e  je 0x155fab
gruntz sema disasm <rva> --base          # e.g.      1b: 74 16  je 0x33
```

`74 2e` vs `74 16` is the whole bug: retail jumps past the second `if`, ours
falls into it.

## Evidence

`CDDrawSurfaceMgr::SetDimensions` 0x155f60 (2026-07-28), parked at 99.88% with
"identical asm". Retail's early-out on an unchanged size jumps straight to
`return 1`; the reconstruction ran the `m_level` rebuild unconditionally:

```cpp
// wrong - the level rebuild is outside the resize guard
if (child->m_width != x || child->m_height != y) {
    if (m_drawTarget->ResizePages(x, y, flags) == 0) { return 0; }
}
if (m_level != 0) { ... }

// right - retail's je (0x155f7b -> 0x155fab) skips it entirely
if (child->m_width != x || child->m_height != y) {
    if (m_drawTarget->ResizePages(x, y, flags) == 0) { return 0; }
    if (m_level != 0) { ... }
}
```

99.88% -> **100% EXACT**, and a real behavioural bug (a redundant full level
rebuild on every no-op SetDimensions) removed.

## Scan it in bulk — the ordered jcc sequence is a cheap whole-campaign sieve

Because the masking hides both the displacement AND (implicitly) the block
layout, a batch scan pays. For each sub-100% function, disassemble both sides
WITH addresses, extract the ordered list of `(offset, mnemonic, target)` for
every in-range branch, and compare:

- **branch present on one side only** -> extra/missing basic block;
- **same count, 1-4 differing MNEMONICS at matching positions** -> the gold:
  - `jg/ja`, `jle/jbe`, `jl/jb` pairs = a **SIGNEDNESS** bug (retype the member
    or the loop counter unsigned - never the function PARAMETER, that rewrites
    the mangled name);
  - `je/jne` = **block-layout polarity** (see positive-gate-enables-shrink-wrap
    and negated-condition-far-block);
- **same mnemonics, displacements shifted by a constant** -> noise, a size
  difference upstream. Ignore.

**This is a first-class disasm mode now — do not re-derive it by hand:**

    gruntz sema disasm <rva> --branches --diff    # ONE function: what actually differs
    gruntz sema disasm <rva> --branches           # one side, the raw sequence
    python -m gruntz.audit.jcc_sieve              # the whole tree, SIGNEDNESS first
    python -m gruntz.audit.jcc_sieve --summary    # counts only
    python -m gruntz.audit.jcc_sieve --class SIGNEDNESS --unit gamelevel

`--branches` names every target by **branch index**, so a uniform displacement shift
compares equal and a genuine retarget does not; it prints both sides' `ret` counts (the
[positive-gate](positive-gate-enables-shrink-wrap.md) lever) and, when llvm-objdump's
linear decode hits jump-table data in `.text`, it says
`[stream truncated at +0xNNN ... branch list is partial]` instead of silently returning a
short list. Both it and the sweep share `gruntz.core.branches`, so there is no logic to
drift. **`--diff` and `--blocks --diff` now print a one-line pointer to `--branches`**
when they have nothing to show and the function is not 100% — that is the loop this
pattern kept costing people.

It reads `report.json` for the scores and disassembles the `build/objdiff/{base,target}`
object pairs directly (nothing masked), classifies each hit SIGNEDNESS / POLARITY / OTHER /
TOPOLOGY, and prints **each side's `ret` count** on the row so the
[positive-gate](positive-gate-enables-shrink-wrap.md) lever can be picked without a second
pass. Guards and known false positives are in its docstring.

First run (2026-07-28) over 1309 sub-100% functions: 105 same-count 1-4-flip hits, of which
6 SIGNEDNESS and 53 pure je/jne polarity. Four signedness fixes and one polarity fix landed
straight away (`CRezDirNode::Load` 98.97 -> 100 EXACT, `DirectSoundMgr::Play` 95.52 -> 100
EXACT). The tool's own first run found the remaining 2 SIGNEDNESS (both real: `g_frameDelta`
is u32; `CDDSurface::Blit168`'s LUT loop is a signed counted loop) and the POLARITY half
resolved into two new families — [if-body-owns-the-fallthrough](if-body-owns-the-fallthrough.md)
and [literal-comparison-form-survives-o2](literal-comparison-form-survives-o2.md).

## Related

- `--blocks --diff --lite` gives the basic-block topology, where a divergent
  in-edge shows up directly; use it as the first look on any function whose
  flat `--diff` looks clean but does not score 100.
- [positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md) —
  what to do with a je/jne polarity hit (count the `ret`s first).
