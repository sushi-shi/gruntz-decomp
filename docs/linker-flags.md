# Linker & build flags — for the DEFERRED whole-binary LINK-reproduction phase

This is the flag reference for **reproducing the whole-binary link** of
`GRUNTZ.EXE` (matching `.text` function order, section/header layout, image base)
— a **DEFERRED** phase, distinct from the per-function matching loop that the rest
of these docs are about. **You do not need any of this to byte-match an individual
function.** Per-function matching is governed by the compile flags only (locked
`/O2 /MT /Gd`); the link phase is what eventually lines up *addresses* and the PE
layout once enough functions exist to relink.

Toolchain: MSVC 5.0 SP3, **LINK 5.10.7303** (PE `MajorLinker 5 / MinorLinker 10`;
see `docs/libraries-and-funcid.md` § Toolchain and `docs/toolchain-vc50-sp3.md`).

Each entry is tagged **[VERIFIED]** (measured/confirmed in this repo) or
**[HEURISTIC]** (reference from isledecomp/reccmp practice for this MSVC era, not
yet confirmed for VC5/Gruntz). See `docs/matching-patterns.md` for the codegen
(compile-side) idioms and `docs/zlib-matching.md` for how the *compile* flags were
calibrated.

---

## Compile flags (the per-function lock — reconciled, here for completeness)

These are settled and govern the per-function loop; repeated here only so the link
phase doesn't second-guess them. Authority: `docs/zlib-matching.md`.

- **`/O2`** [VERIFIED] — optimization level (`== /Ox`); proven by the zlib
  byte-match (frameless prologue, register allocation). Not `/O1`/`/Os`/`/Od`.
- **`/MT`** [VERIFIED] — **static multithreaded** CRT (`LIBCMT.LIB`), **NOT**
  `/MD` (there is no `msvcrt.dll` import; static MFC `…42s` requires the static MT
  CRT — `docs/libraries-and-funcid.md` § 1.1).
- **`/Gd` = `__cdecl`** [VERIFIED] — the default calling convention for free
  functions (zlib `_name` cdecl); members are `__thiscall` (the 5 matched ctors
  are `??0…@@QAE@XZ`). Do not globally override the convention.
- `/Zp` = default (`/Zp8`) [VERIFIED, pinned by deflate.c], `/Gy` forced on by
  `/O2` [VERIFIED], `/GF` unconstrained / no effect [VERIFIED]. See
  `zlib-matching.md` for the evidence.
- `/Zi` or `/Z7` (PDB / debug info) [HEURISTIC] — **no codegen change**; affects
  only the debug stream, so it neither helps nor hurts byte-matching. (We synth our
  own PDB; see `synth_pdb`.)
- `/DNDEBUG` [HEURISTIC, VERIFY] — strips `assert()`. **Do not assume** retail
  built with it: this is a release build but ships leftover debug/profiler
  overlay strings, so check whether `assert` `__FILE__`/line strings are actually
  present (`docs/strings-analysis.md`) **before** deciding to define `NDEBUG`.
  See the "assertions" item in `matching-patterns.md` § "Common mismatch
  checklist".

---

## Linker flags that affect the bytes / layout

Mostly relevant to the link-reproduction phase, **not** per-function matching.

### Optimization / folding

- **`/OPT:ICF` (identical COMDAT folding) — [VERIFIED OFF for this binary].**
  This resolves the reference's "confirm `/OPT:ICF` default for VC5" caveat: for
  **retail Gruntz v0.76 we MEASURED ICF did not fold** — **574 byte-identical
  functions live at distinct addresses** (including 47 that are ≥32 bytes, well
  past any minimum-size threshold). So:
  - **Do NOT force `/OPT:ICF`** and **do NOT model COMDAT folding** when
    reproducing the link — duplicated identical bodies are expected to remain
    separate.
  - There is no "FOLDED" concept to track here (and `reccmp`'s `FOLDED` flag is
    moot — `reccmp` is not used in this project; we use the delink→objdiff loop —
    see `orchestration.md`).
- **`/OPT:REF` (dead-strip unreferenced COMDATs/data) — [HEURISTIC, UNTESTED].**
  Separate from ICF and **not yet measured** for this binary. It removes
  unreferenced functions/data from the image; if our relinked output carries
  bodies the retail image dropped (or vice-versa), revisit this. Don't assume a
  setting until measured.

### Layout / addresses

- **`/ORDER:@<file>` — [HEURISTIC]. The biggest lever for the link phase.**
  Controls `.text` function order directly from a response file, so you can line
  up function **addresses** with retail **without reshuffling source**. (Recall
  COMDAT layout is link/COMDAT order, not source-definition order — see the
  trees.c note in `zlib-matching.md`.) This is the primary tool once enough of the
  binary exists to relink.
- **`/BASE:0x400000` — [VERIFIED] image base.** Confirmed `0x400000`
  (`docs/libraries-and-funcid.md` § section map; `.text` VMA `0x00401000`).
- **`/INCREMENTAL:NO` — [HEURISTIC]. Must be OFF.** Incremental linking inserts
  thunks and padding and reorders functions, which would never match a retail
  (full) link. Retail is a non-incremental link by nature; force `:NO` when
  relinking.
- **`/ALIGN`, `/FILEALIGN` — [HEURISTIC].** Section virtual/file alignment; wrong
  values shift every section and break the PE header/layout match.
- **`/SUBSYSTEM` — [HEURISTIC].** `WINDOWS` here (PE32 GUI —
  `docs/libraries-and-funcid.md`); sets the subsystem field + entry-point
  convention.
- **Linker version — [VERIFIED target].** LINK 5.10.7303; the linker version is
  stamped in the PE header and influences default layout, so reproduce with the
  matching LINK.

---

## Relationship to the rest of the pipeline

- This is **deferred**: we are matching functions first (delink → `cl` →
  objdiff). The link-reproduction phase only becomes relevant when we want the
  *whole image* (correct addresses + PE layout), at which point `/ORDER:@file`
  + `/BASE` + `/INCREMENTAL:NO` + `/OPT` are the levers.
- `reccmp` is **not** used here (see `orchestration.md` § Concurrency / Definition
  of done); its address-reconciliation and `FOLDED` machinery don't apply.
- Cross-links: compile-flag calibration → `docs/zlib-matching.md`; library/CRT/MFC
  linkage evidence → `docs/libraries-and-funcid.md`; codegen idioms →
  `docs/matching-patterns.md`; toolchain identity → `docs/toolchain-vc50-sp3.md`.
