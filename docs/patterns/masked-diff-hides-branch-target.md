# A masked diff that reads "identical asm" can still be a CONTROL-FLOW BUG — branch displacements are masked

**Tags:** cpp:branch | asm:jcc | topic:tooling topic:scoring-artifact

## Symptom

The masked comparison of the two sides reports the instruction streams as
identical, yet `report.json` says the function is **not** 100% (e.g. 99.875%),
and `llvm-objdump -dr` of the base and target objects shows byte-identical
instruction bytes *and* identical relocation symbol names.

## Cause

Every masked view prints an address operand as `<tgt>` / `<addr>` so that
reloc-bound call/jump targets do not show up as spurious diffs. That masking
also hides **intra-function conditional-branch displacements** — a `je` that
goes to a *different basic block* on the two sides is printed as `je <tgt>` on
both and compares equal.

So a genuine divergence in control flow can render as "identical asm".

## Detection

`gruntz walls diagnose <rva>` never masks: it prints each side's branch and
`ret` counts, and classifies the wall. Equal call sets with unequal branch or
ret counts is the CFG class, and that is this bug. When the counts agree but the
bytes do not, read the pair **with addresses** and compare the jump displacement
bytes directly:

```
gruntz walls diagnose <rva> --asm    # both sides, addresses and bytes, unmasked
gruntz sema disasm <rva>             # retail alone, Model-annotated
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

## Reading the ordered jcc sequence

The durable method, whatever prints it: disassemble both sides WITH addresses,
extract the ordered list of `(offset, mnemonic, target)` for every in-range
branch, name each target by **branch index** (so a uniform displacement shift
compares equal while a genuine retarget does not), and compare.

- **branch present on one side only** -> extra/missing basic block;
- **same count, 1-4 differing MNEMONICS at matching positions** -> the gold:
  - `jg/ja`, `jle/jbe`, `jl/jb` pairs = a **SIGNEDNESS** bug (retype the member
    or the loop counter unsigned - never the function PARAMETER, that rewrites
    the mangled name);
  - `je/jne` = **block-layout polarity** (see positive-gate-enables-shrink-wrap
    and negated-condition-far-block);
- **same mnemonics, displacements shifted by a constant** -> noise, a size
  difference upstream. Ignore.

One trap when decoding by hand: llvm-objdump's linear decode runs into
jump-table data embedded in `.text`, and everything after that point is
garbage rather than a short list. Stop the decode at the last `ret`.

The tree-wide jcc sieve that once automated this is retired with the old
tooling; the classification above is what it encoded, and
`gruntz walls diagnose` gives the per-function verdict. Its first run
(2026-07-28) over 1309 sub-100% functions found 105 same-count 1-4-flip hits,
of which 6 SIGNEDNESS and 53 pure je/jne polarity. Four signedness fixes and
one polarity fix landed straight away (`CRezDirNode::Load` 98.97 -> 100 EXACT,
`SoundBuffer::Play` 95.52 -> 100 EXACT), and the POLARITY half resolved into
two new families — [if-body-owns-the-fallthrough](if-body-owns-the-fallthrough.md)
and [literal-comparison-form-survives-o2](literal-comparison-form-survives-o2.md).

## Related

- [positive-gate-enables-shrink-wrap](positive-gate-enables-shrink-wrap.md) —
  what to do with a je/jne polarity hit (count the `ret`s first).
