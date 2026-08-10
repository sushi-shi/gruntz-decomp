# objdiff compares a relocation by its target's SECTION NAME, so `.text$x` never matched `.text`
tags: cpp:eh data:objdiff | asm:push asm:mov | topic:scoring-artifact topic:tooling
symptoms: `DIFF_ARG_MISMATCH` on `push OFFSET <label>` or `mov eax,<FuncInfo>` while both sides print the SAME symbol name; a /GX function stuck one instruction short of 100 with its whole body identical; every `__ehreg$` registration stub pinned at exactly 97.5%
confidence: 10/10

## What it is

This project runs objdiff with `functionRelocDiffs = data_value`. Read
`objdiff-core/src/diff/code.rs::reloc_eq` with that setting substituted in and what
is left is:

```rust
section_name_eq(left_obj, right_obj, left_sym.section, right_sym.section)
    && (true)                                  // the NAME/ADDRESS clause is skipped
    && (kind != Object || size == 0 || <the referenced bytes are equal>)
```

The relocation target's **name is deliberately not consulted**. Two things decide
the comparison: the target symbol's **section NAME** must be equal on both sides,
and (for a data target) the bytes it points at must be equal.

That makes a whole class of mismatch invisible to every other view. `--diff`,
`--blocks --diff` and `--branches --diff` all mask address operands, and a byte
compare of the two objects shows nothing either, because the operand is a
relocation placeholder on both sides. The only surface it appears on is the
per-instruction `diff_kind` in `objdiff-cli diff --format json`.

## Where it bit

cl 5.0 puts a `/GX` function's unwind funclets in a `.text$x` COMDAT and its EH
state tables in `.xdata$x`. Retail's linker folded those into `.text` and `.rdata`,
and that is what the delinker rebuilds - so for every one of GRUNTZ.EXE's 750 EH
groups:

* the OWNER's prologue `push OFFSET <registration stub>` relocated into `.text$x`
  on our side and `.text` on retail's, and
* the stub's own `mov eax,<FuncInfo>` relocated into `.xdata$x` against `.rdata`.

Both compared FALSE. 750 /GX functions each carried one guaranteed mismatching
instruction that had nothing to do with their code (`CActionArea::~CActionArea` sat
at 99.71% with that single `push` as its only diff), and all 750 registration stubs
sat at exactly 97.5% - 10 bytes, two instructions, one of them permanently wrong.

## The fix

`gruntz.build.canonicalize_data_symbols._canonicalize_eh_section_names` renames the
BASE's grouped sections to the section the linker put them in - `.text$x` -> `.text`,
`.xdata$x` -> `.rdata` - in the normalized comparison copy only. `$x` is an ordering
key, not part of a section's identity (the same reading the delinker's own
`vostok-grouped-section-names.patch` already applies to `.rdata$r`), and retail's
image proves the destination: it has no `.xdata` section at all and every `FuncInfo`
sits inside `.rdata`. Nothing but the 8-byte name field changes.

Only the base can move: the delinked target genuinely has one `.text` and one
`.rdata`.

Measured: EH band 1758/3034 -> 2508/3034 exact records (all 750 stubs), data
fidelity 77.27% -> 97.00%.

## The general rule

Before concluding that a relocation operand is "the same on both sides", check the
target symbol's SECTION NAME, not its symbol name. Any section cl spells with a `$`
group suffix that the delinker rebuilds under the group name has this defect, and it
will never show up in a masked diff.
