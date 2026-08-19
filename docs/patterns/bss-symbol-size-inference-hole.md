# `.bss` sections cap below 100% — objdiff infers COFF symbol sizes from the next symbol's offset

**Tags:** `data:bss` `data:objdiff` | `topic:scoring-artifact` `topic:tooling`
**Confidence:** c9 (measured; 3 independent cl 5.0 probes + the objdiff source)

> **RESOLVED 2026-08-09** — `objdiff-cli` is built from source with
> `nix/patches/objdiff-bss-inferred-extent.patch`, which compares a BSS symbol's size
> only when at least one side actually STATES one. `matched_data` 16.83% → **90.87%**,
> function scoring bit-identical. Everything below is the mechanism (still true) and the
> refutation of the two fabrications (still binding). Jump to **The fix, and why the
> other two routes cannot work**.

## Symptom

A `.bss` section sits at 99.9…% and `matched_data` credits it **nothing** — objdiff
scores data all-or-nothing per section (`objdiff-cli/src/cmd/report.rs`: a section's
size counts only `if section_match_percent == 100.0`). The residual traces to ONE
symbol, always a small int, always the one cl placed at **section offset 0**.

Measured on `ddsurface` (the worst case in the tree — **197144 B, 70% of
`total_data`**, stuck at **99.998985%**):

| symbol | base off | base size | target off | target size |
|---|---|---|---|---|
| `?g_clut@@3PAGA` | 0x8 | 0x30000 | 0x0 | 0x30000 |
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

## The fix, and why the other two routes cannot work

**The two sides use different allocators.** The base `.bss` layout is cl's. The target
`.bss` layout is the *delinker's own* sequential append with per-definition alignment —
it is not retail's. So the inferred span compares two allocators' padding, and there is
no retail referent in it at all: the delinker even takes each `.bss` size from OUR
source (the Model → `data_manifest`, provenance `src-DATA-sizeof`).

**Census, whole tree** — `gruntz verify data-coverage` (the claim-side gap census, exact-name
pairing, which is what objdiff does for these names): **363 paired `.bss` symbols, 50
extent disagreements, |delta| histogram `{3: 1, 4: 49}`** — the widest disagreement in
the whole program is FOUR bytes. Every one is sub-alignment padding; not one is a size.
The same tool lists the 91 unpaired target symbols, which are the real worklist.

Three routes were considered; only the third is possible.

1. **Have the delinker STATE the size.** Impossible. COFF has no symbol-size field; the
   `object` crate reads a size only for a **COMMON** symbol, where `Value` *is* the size.
   The delinker already sets `size:` on the `object::write::Symbol` it emits and the
   COFF writer discards it. Emitting `.bss` data as COMMON instead would take the symbol
   out of the section entirely.
2. **Have the delinker pack `.bss` tight**, so the inferred span equals the declared
   size. It makes the *symbol* extent true and the *section* extent false: the target
   `.bss` size currently reproduces retail's contribution span exactly (verified —
   imagepolyclip `0x2856f0..0x2becfe` = 0x3960e; ddsurface `0x253c88..0x283eb8` =
   0x30230), which is the one genuinely retail-derived number a `.bss` section carries,
   and it feeds `total_data`. It also cannot fix the biggest case: **ddsurface's 8-byte
   reading is on the BASE side**, inside cl's output, where nothing we do reaches.
3. **Compare sizes only when at least one side states one** —
   `nix/patches/objdiff-bss-inferred-extent.patch`, in `diff_bss_symbol`. Declines to
   compare a quantity neither object states. Unpaired symbols are still a mismatch, so
   a missing/extra global is still caught, and COMMON symbols (which DO carry a size)
   are still compared. Same ruling as the sibling project's
   `docs/strict-data-allocations.md`: *"The checker never accepts that span as a
   reviewed extent."*

The extent check that actually bites lives in `gruntz.delink.data_manifest.candidates()`:
a reviewed extent must fit the span to its retail neighbour, and an overlap withholds
BOTH rows and reports them as a reconstruction-defect worklist.

## Consequence for the metric

Measured on unchanged objects (only the `objdiff` ninja edge re-ran):

| | before | after |
|---|---|---|
| `matched_data` | 118,484/704,148 = **16.83%** | 639,859/704,148 = **90.87%** |
| data sections at exactly 100.0 | 338 | 357 (+19, **all `.bss`**) |
| data-bearing units fully exact | 176 | 187 |
| size-weighted data | 99.6055% | 99.6501% |
| `matched_functions` / `matched_code` / fuzzy | 3498 / 474,819 / 91.99114 | **identical** |

4325 functions, **zero** changed in size, percent or address; every section's identity
and size identical. The control that the change is not just inflation: the sections
whose defect is a real *missing symbol* did **not** flip — `videoconfig` 40%, `butemgr`
8.7%, `battlezmapconfig` 12.9%, `brickzload` 0%, `netcmdslot`, `dialogs`, `gruntzapp`,
`worldsoundset`, `wwdfactoryobject`, `fadereffects` all unchanged, and
`checkpointdlg`/`fonts`/`play`/`savegame`/`multi`/`customworlddialog` rose only as far
as their extent-only symbols allowed.

**The naming gap is CLOSED (2026-08-09).** The normalizer now (a) rewrites the
delinker's rva-suffixed enrolments (`?s_gruntDirEast_22bd28@@3U...`) onto cl's
`_s_gruntDirEast$S` family and gives a BSS static a span/payload-independent
identity (`DELINKED_STATIC_COPY`, proof `bss-no-content` - the same
two-allocators argument as this patch), and (b) materializes base-side COFF
COMMONs (header-inline local statics + `??_B` guards) into `.bss` as the
linker would. Library data no game TU can re-emit (CDialog's messageMap,
type_info's vtable, filebuf::openprot) moved to the non-compared `library_data`
holding unit; and every one of the 71 `GruntDirectionCell` blocks is enrolled
rva-suffixed and pairs against its TU's own emission.

**`butemgr` was the last hole, and it was the same naming defect one level
deeper (fixed 2026-08-09, `docs/data-attribution.md` §3d-ii).** Its band stayed
at 8.70% because `config/retail/data_compgen.tsv (class=copy)` enrolled it under invented C
names (`_s_default_rect_butemgr`) while the base obj spells it the way cl does
—`_?s_default@?1??GetRect@CButeMgr@@QAEPAUButeIntRect@@PBD0@Z@4U3@A$S20265`, a
NON-inline function's local static (`docs/compiler-data-layout.md`, the six
cases). The rows now carry cl's name, and `STATIC_ORDINAL` masks EVERY `$S<n>`
counter in it — c1xx spells the guard's own unnamed object `?$S<n>@…`, and that
inner counter renumbers exactly as c2's trailing `outdname` suffix does. With
that, **all 515 data sections sit at exactly 100.0 and `matched_data` is
100.00%.**

## See also

- `docs/data-attribution.md` §3 — the data loop and the manifests.
- `scripts/gruntz/build/data_manifest.py: section_rows()` — the legitimate
  candidate-shaping, and why it is limited to the string COMDATs.
