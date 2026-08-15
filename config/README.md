# config/ — tracked configuration and recovered evidence

Configuration is grouped by the thing it describes. NEVER resolve a merge
conflict here with a blanket `--ours`/`--theirs` (that broke main twice) — use
the owning tool's merge/update rule.

## Root build, matching, and audit contracts

- **`units.toml`** — THE per-TU manifest: unit → source path + compile-flags
  profile (`[flags]`: `c` vendor C /O2 /MT, `cpp` /O2 /MT /GX,
  `cpp-rtti` +/GR (the Gruntz project), `cpp-rtti-noeh` /GR without /GX —
  recovered from the retail bytes, not chosen). Read by everything via
  `gruntz.core.manifest`. Add a TU = add an `[[unit]]` block.
- **`match_baseline.tsv`** — per-function best-fuzzy% regression baseline
  (`gruntz.match.status`). Bless reviewed dips/losses via
  `status update --accept-regressions`.

## `cleanliness/` tracked metrics

These files belong to source-quality audits. Baselines are tool-rolled and
merged per row; remeasure the merged tree rather than taking one side wholesale.

- **`cleanliness-text-baseline.tsv`** — fast source-text scoreboard floors
  (`gruntz.cleanliness.board`), measured on normal builds.
- **`cleanliness-semantic-baseline.tsv`** — build/IR-derived scoreboard floors,
  measured only by the periodic full build. Ratcheted rows in either file are
  DOWN-ONLY; bless a lower floor via `board --update` (`--semantic` includes
  the semantic collectors).
- **`bare-constants-baseline.tsv`**, **`data-tu-order-baseline.tsv`**,
  **`single-view-baseline.tsv`**, and **`tu-order-baseline.tsv`** — focused
  audit ratchets and frozen backlogs.
- **`kept-comdat-exiles.tsv`** — the tu-order gate's reviewed exemptions:
  interleaves retail genuinely produced (a COMDAT kept inside a foreign
  unit's contribution). Gate policy, not retail labeling.
- **`data-integrity-ratchet.tsv`** — maxima for the data-integrity audit.

## `retail/` executable labels — the channel model

One BASE census per address space, structural only (starts + kinds, no sizes,
no names), and PROVIDER tables layered on it. A row's extent is DERIVED to the
next base row; the exact CODE/data size of a byte-matched body lives on its
CLAIM (`RVA(rva, size)` in src/, `functions_zlib.tsv`, clang `sizeof` for
`DATA()` globals). `gruntz.audit.channels` gates the invariant: every provider
rva is an admitted base row of the matching kind.

Bases:

- **`functions.tsv`** — the admitted `.text` partition: one row per function
  start, kind ∈ (empty=body | thunk | eh | helper | pad). Hand-owned;
  boundary corrections are edited here, the build never regenerates it.
- **`data.tsv`** — the admitted data census: one row per datum start in
  `.rdata`/`.data`/`.bss`, kind ∈ (empty=datum | string | fppool | vtable |
  rtti | ehtable | guard | common | copy | pad).
  `gruntz.audit.data_denominator --check` re-proves kinds and tiling against
  the image + the current enrolment every build.
- **`link_bands.tsv`** — the image's coarse link-layout bands ([lo, hi) over
  `.text` and the data sections) for reporting and the hard band↔kind
  invariants. DERIVED like link_order (bytes + link_order transitions), so
  tracked-but-never-hand-edited; the rebuild grows its regenerator/check.

Function providers:

- **`functions_static_libs.tsv`** — CRT/MFC/library carve-outs (rva, name,
  lib, confidence, source): RVAs proven NOT reconstructable game C++, excluded
  from the match denominator. LOW rows are diagnostic leads, never claims. An
  rva may carry several alias/provenance rows. HAND-OWNED (the FID
  regeneration pipeline is retired); merge per ROW.
- **`functions_zlib.tsv`** — the vendored zlib-1.0.4 function labels
  (rva, name, unit, size). The vendored sources are pristine, so this table is
  their claim channel; `size` is the exact matched extent.

Data providers:

- **`data_vtables.tsv`** — game/engine `??_7` tables (rva, size, name, kind,
  note); kind separates primary, MI-secondary, and template tables.
  `data_manifest.vtable_rows()` enrolls each once per emitting base obj.
- **`data_static_libs.tsv`** — MFC/CRT/iostream vtables, vtable-like tables,
  SDK GUIDs, and library constants game code names (rva, size, name, unit,
  note). A unit + size ENROL the row; `library_data` is the deliberate
  holding unit objdiff never opens.
- **`data_zlib.tsv`** — the vendored zlib data labels (rva, name, unit, size).
- **`data_compgen.tsv`** — compiler-generated data with no reachable source
  spelling (rva, size, name, owner, class): `class=common` COFF COMMONs from
  header-inline local statics + their `??_B` guards
  (`gruntz.audit.compgen_data` re-proves them against the base objs), and
  `class=copy` per-TU copies of header statics (the GruntDirStatics device).

Retail-derived evidence:

- **`link_order.tsv`** — DERIVED retail link order: one row per unit's
  contiguous contribution band (ascending band start = arrival order).
- **`reloc_referents.tsv`** — per-site retail relocation referents whose exact
  spelling (symbol + addend) containment inference cannot reach.
- **`rsrc/`** — the retail resources (extracted payloads + manifest).

Everything under retail/ states a fact about the retail image. Gate policy
lives in cleanliness/; loss detection (the old gruntz_functions.tsv census
floor) is match_baseline.tsv's LOST reporting.

Retired configs (caller-audit ledgers, match-queue.md, `library_labels.csv` +
`zlib_labels.csv` + `vtables_*.csv` + `compiler-generated-data.tsv` +
`static_data_copies.tsv` + `compiler-helper-functions.tsv` +
`data-coverage-partition.tsv`, all dissolved into the channel model above) are
gone — see `docs/data-attribution.md` and `gruntz.audit.channels`.
