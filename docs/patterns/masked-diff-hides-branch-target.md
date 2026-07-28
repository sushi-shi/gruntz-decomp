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

## Related

- `--blocks --diff --lite` gives the basic-block topology, where a divergent
  in-edge shows up directly; use it as the first look on any function whose
  flat `--diff` looks clean but does not score 100.
