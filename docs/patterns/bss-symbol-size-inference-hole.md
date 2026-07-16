# `.bss` sections cap below 100% — objdiff infers COFF symbol sizes from the next symbol's offset

**Tags:** `data:bss` `data:objdiff` | `topic:wall` `topic:scoring-artifact` `topic:tooling`
**Confidence:** c9 (measured; 3 independent cl 5.0 probes + the objdiff source)

## Symptom

A `.bss` section sits at 99.9…% and `matched_data` credits it **nothing** — objdiff
scores data all-or-nothing per section (`objdiff-cli/src/cmd/report.rs`: a section's
size counts only `if section_match_percent == 100.0`). The residual traces to ONE
symbol, always a small int, always the one cl placed at **section offset 0**.

Measured on `ddsurface` (the worst case in the tree — **197144 B, 70% of
`total_data`**, stuck at **99.998985%**):

| symbol | base off | base size | target off | target size |
|---|---|---|---|---|
| `?g_clut@@3PAEA` | 0x8 | 0x30000 | 0x0 | 0x30000 |
| … | | | | |
| `?g_bDown@@3HA` | **0x0** | **0x8** | 0x30214 | **0x4** |

`g_bDown` is an `i32`. Both sides are *correct*; only the measurement differs.

## Mechanism

**COFF carries no symbol sizes.** objdiff synthesises them
(`objdiff-core/src/obj/read.rs: infer_symbol_sizes`): `size = next symbol's address`
(or the section end). For BSS that number is the *whole* score —
`diff_bss_symbol` is literally:

```rust
let percent = if left_symbol.size == right_symbol.size { 100.0 } else { 50.0 };
```

and `diff_generic_section` returns exactly `100.0` only when **every** symbol in the
section is at 100.

**MSVC 5.0's `.bss` allocator is a hole-filler.** It lays the ≥8-aligned objects down
first, then packs the 4-byte ints into the gaps — including the gap **before** the
first 8-aligned object. So one int always lands at offset 0 with `g_clut` at 8, and
objdiff measures that int as **8 bytes**, not 4.

## It is NOT steerable from the source

Three `cl /O2 /MT` probes, same globals, different declaration order / object set:

| probe | declaration order | result |
|---|---|---|
| A | `g_lut16, g_rUp…g_bDown, g_imageCacheIndex, g_clut` | `g_bDown@0 → 8` |
| B | `g_imageCacheIndex, g_clut, g_lut16, g_rUp…g_bDown` | `g_bDown@0 → 8` (layout **identical** to A) |
| C | as A but `g_imageCache` modelled 8 B instead of `CPtrArray` 0x14 | `g_bDown@0 → 8`, **and now `g_rUp` → 8 too** |

**MSVC5's `.bss` layout is declaration-order invariant** (A vs B are byte-identical) —
it depends only on the *set* of objects and their alignments. Reordering, retyping the
neighbours, and fixing the adjacent size contradiction all leave the offset-0 hole.

## Do NOT "fix" this

Two tempting moves, both fabrication — reject on sight:

1. **Give the target a candidate-shaped `.bss`** via `--data-section-manifest`
   (mirroring the candidate's offsets). It would score 100% instantly — and
   **vacuously**: `.bss` has no bytes, so shaping the container to the candidate makes
   every symbol's inferred size agree *by construction*, for **any** set of globals,
   right or wrong. It proves nothing and hides real defects.
   *(Contrast `.data`/`.rdata`, where candidate-shaping is legitimate: the container is
   neutralised but the delinker still fills it with **retail bytes read from each
   definition's proven RVA**, so the byte comparison stays real. That is the
   `--data-section-manifest` win in `data_manifest.section_rows()`.)*
2. **Add a filler global** to plug the offset-0 hole. Invents a symbol retail never had.

## Consequence for the metric

`.bss` is **212211 of 279630** `total_data` bytes (~76%), and `ddsurface` alone is
197144 of that. A large, permanent-looking slice of `matched_data` is gated on an
inference artifact, not on reconstruction quality. **Read `matched_data` as a
`.data`/`.rdata` measure**; do not budget work against the `.bss` share.

The real fix is upstream, in the tooling, not in `src/`: either teach the delinker to
emit COFF symbol sizes on the target side, or score BSS by (name → *declared type
size*) instead of by inferred gaps. Until then this is a documented wall.

## See also

- `docs/data-attribution.md` §3 — the data loop and the manifests.
- `scripts/gruntz/build/data_manifest.py: section_rows()` — the legitimate
  candidate-shaping, and why it is limited to the string COMDATs.
