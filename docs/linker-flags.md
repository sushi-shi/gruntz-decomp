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
  `/O2` [VERIFIED]. `/Gf` (string pooling) implied by `/O2` — passing it is a
  byte-neutral no-op [VERIFIED]; `/GF` would move the pooled literal COMDATs from
  writable `.data` to `.rdata` — retail's literals are in writable `.data`, so
  `/GF` is OFF [VERIFIED]. See `zlib-matching.md` and `string-pooling.md`.
- **`/G<n>` (processor target) — unconstrained except `/G6` is EXCLUDED** [VERIFIED
  on `directinputmgr2`, 2026-08-05]. `/G3`, `/G4`, `/G5`, `/GB` and the default all
  emit that unit **byte-identically** (115/140 fns exact, mean 99.28); only `/G6`
  (Pentium Pro) differs and it *regresses* to 97/140, so retail was not built with
  it. Useful corollary: because the 386 target (no dual pipeline) matches the
  Pentium target byte-for-byte here, MSVC 5.0's instruction ORDER in these blocks is
  not produced by a processor-targeted pairing scheduler — do not attribute an
  instruction-transposition residue to one. Same sweep: `/Ob1`, `/Ot`, `/Gy-`, `/Gf`
  are codegen-identical; `/Ob0`, `/Oy-`, `/Oi-`, `/Os`, `/Og-`, `/O1` all regress.
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
  past any minimum-size threshold).

  **The flag EXISTS — do not restate this as "MSVC5 has no `/OPT:ICF`".** Our own
  `LINK.EXE`, *Microsoft (R) 32-Bit Incremental Linker Version 5.10.7303*, prints

      /OPT:{ICF[,iterations]|NOICF|NOREF|REF}

  in its usage text, and our link line already passes `/OPT:NOICF`. The measured
  conclusion below is about how RETAIL was linked, not about what the toolchain
  can do; nothing downstream changes, because every use of it only needs "retail
  did not fold", which the 574 duplicates establish directly. So:
  - **Do NOT force `/OPT:ICF`** and **do NOT model COMDAT folding** when
    reproducing the link — duplicated identical bodies are expected to remain
    separate.
  - There is no "FOLDED" concept to track here (and `reccmp`'s `FOLDED` flag is
    moot — `reccmp` is not used in this project; we use the delink→objdiff loop).
- **`/OPT:REF` (dead-strip unreferenced COMDATs/data) — [HEURISTIC, UNTESTED].**
  Separate from ICF and **not yet measured** for this binary. It removes
  unreferenced functions/data from the image; if our relinked output carries
  bodies the retail image dropped (or vice-versa), revisit this. Don't assume a
  setting until measured.

### Libraries — [VERIFIED]

Recovered from the objs' own `.drectve` directives cross-checked against retail's
import table; wired into `link.py` (details + the synthesis story:
`docs/build-system.md` § "The library set"). Summary:

- **`/NODEFAULTLIB` — OFF.** `cl /MT` writes `-defaultlib:LIBCMT` +
  `-defaultlib:OLDNAMES`, and the MFC headers add `nafxcw kernel32 user32 gdi32
  comdlg32 winspool advapi32 shell32 comctl32` (+ `uuid`, and `libcpmt`/`libcimt`
  from the ANSI-C++ / old-iostream headers). Letting those fire reproduces the devs'
  link line for free.
- **Explicit extras:** `version winmm` (imported by the game, declared by nothing) and
  DX6 `ddraw dsound dinput dplayx dxguid` (the DX SDK ships no `#pragma comment(lib)`).
- **Synthesised:** `mss32 smackw32` — the RAD SDKs we lack; `gruntz.build.import_lib`
  rebuilds their import libs from retail's import table.
- **`/ENTRY` — `WinMainCRTStartup`** (LIBCMT), which calls `_WinMain@16` (NAFXCW).
  The old `_x` placeholder only existed because no library was on the line.

With that set the punch list drops from **481** unresolved externals to **1**.

**Library ORDER is load-bearing — [VERIFIED], and `LINK_LIBS` reproduces retail's
import-descriptor order EXACTLY (0/120 inversions, all 16 DLLs).** Two rules:

1. link.exe emits a DLL's `__IMPORT_DESCRIPTOR_*` when a library search **first
   satisfies an undefined symbol**, so lib order = descriptor order.
2. …but only for symbols already undefined when that lib is searched. **`nafxcw`/
   `libcmt` must therefore be named FIRST.** Two thirds of the import table (306 of
   456 names, including *all* of comctl32/winspool/comdlg32/shell32) is referenced by
   **nothing in our objs** — it arrives through MFC/CRT library members. Search Win32
   before MFC and those four DLLs have no pending undefines yet, so they resolve only
   on a later pass and their descriptors sink to the end (17/120 inversions). Naming
   MFC/CRT ahead of them fixes it — and says the retail link line did the same.

Measured side effects of the ordering: **zero of our own symbols move** (only the
MFC/CRT block shifts, ~2959 library symbols); the imported-name *sets* are unchanged.

Residuals, both real but neither a flag:

- **Per-DLL name order does NOT follow our object order.** Relinking with the objects
  fed in retail-RVA order (`--order`) moved it 63.2% → 62.1% pairwise-misordered —
  nothing. It is dominated by the order MFC/CRT members get pulled from `NAFXCW.LIB`,
  which is a function of *which* members our code drags in, not of TU sequence. It is
  also not hint-sorted in either binary (~50% ascending in both), so there is no cheap
  structural rule to copy.
- **26 wrong hint values, all in the synthesised libs.** 423 of 449 named imports
  carry the *same* `.idata$6` hint as retail — our Win32/DX import libs are the right
  vintage. The 26 that differ are exactly mss32(16) + smackw32(10): a hint is the
  export-table index, and our stub DLL exports only the 26 names retail imports, not
  the real DLL's full export list. Fixable by stubbing every export of the real DLLs
  (needs `$GRUNTZ_RUNTIME`) or by patching the hints from retail. Functionally inert —
  the loader falls back to a name search — but it blocks a byte-exact `.idata`.


### Static-library split — [VERIFIED]

Retail linked the **engine projects as static libraries**, not as objects on the link
line, and the incremental thunk band proves it: MSVC thunks every cross-object call
between OBJECTS but never a `.lib` member. Retail's thunk targets stop dead at
`0x11c860` — below that line there are **2** cross-unit direct calls, above it **4664**
and zero thunks. Sorting our units by that boundary puts 227 of 237 `src/Gruntz` units
below it and DDrawMgr/Image/Bute/Crypto/Rez/Wwd/Dsndmgr/zlib above: the leaked
`C:\Proj\{...}` project boundary.

`link.py --engine-lib` archives those modules with the real VC5 `LIB.EXE` and links the
archive instead:

| | `E9` thunks | vs retail |
|---|---|---|
| retail | 2695 | — |
| default (all objs) | 4559 | 1.69x |
| **`--engine-lib`** | **2976** | **1.10x** |

Opt-in: it changes the map's object attribution wholesale. Import table stays exact and
`link_order` still reads the map. Partition by MODULE, not by measured `min(RVA)` —
the latter mis-assigns units that straddle the line (3242 thunks, worse).

### Layout / addresses

- **`/ORDER:@<file>` — [HEURISTIC]. The biggest lever for the link phase.**
  Controls `.text` function order directly from a response file, so you can line
  up function **addresses** with retail **without reshuffling source**. (Recall
  COMDAT layout is link/COMDAT order, not source-definition order — see the
  trees.c note in `zlib-matching.md`.) This is the primary tool once enough of the
  binary exists to relink.
- **`/BASE:0x400000` — [VERIFIED] image base.** Confirmed `0x400000`
  (`docs/libraries-and-funcid.md` § section map; `.text` VMA `0x00401000`).
- **`/INCREMENTAL` — [VERIFIED ON; the old "must be OFF" here was WRONG].** The
  reasoning used to be "incremental linking inserts thunks and padding, which would
  never match a retail link." Retail *has* those thunks and that padding:
  * the `E9 rel32` band at the top of `.text` (`pe.ILT_LO..ILT_HI` = 0x1000..0x7c20,
    which this repo already models) is the incremental linker's thunk table —
    **399 of the first 400 5-byte slots are `E9`**;
  * retail's IAT carries **zeroed slack after every DLL's terminator** (705 slots
    allocated for 472 used) — incremental growth room, not a packed array;
  * retail keeps a separate writable **`.idata`** section rather than folding the
    import data into read-only `.rdata`, which is what our non-incremental link does.

  So retail is an **incremental** link, and `/INCREMENTAL:YES` is now **the default**
  (2026-08-03). It became reachable only once the tree linked `/FORCE`-free — any
  `/FORCE`, including `/FORCE:MULTIPLE`, makes link silently ignore it (LNK4075).

  **The decision was measured, not assumed.** It costs nothing: per-object
  fragmentation is *identical* under `:YES` and `:NO` (median 1 fragment, mean 1.07,
  94.7% of 646 objects perfectly contiguous, **zero** objects more fragmented), and
  `gruntz.audit.link_order` still reads the map. It buys three retail shapes at once:

  | | `:NO` | `:YES` | retail |
  |---|---|---|---|
  | `E9` thunks in the first 0x8000 of `.text` | 14 | **4559** | **2704** |
  | sections | `.text .rdata .data .reloc` | + **`.idata`** | + `.idata .rsrc` |
  | image | 1,725,952 | **2,258,432** | 2,511,872 |

  Our thunk band is *larger* than retail's (4559 vs 2704) because we thunk library
  code retail did not — the shape is right, the extent is not yet. `--no-incremental`
  restores the flat layout for isolating that variable.
- **`/FIXED:NO` — [VERIFIED ON].** Retail **has a `.reloc`** (it is why the EXE is
  delinkable at all), so base relocations were kept. `link.py` passes it by default;
  measured **purely additive** — `.text`/`.rdata`/`.data` are byte-identical with and
  without it, only the trailing `.reloc` section appears.
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
- `reccmp` is **not** used here (delink → cl → objdiff, not reccmp); its
  address-reconciliation and `FOLDED` machinery don't apply.
- Cross-links: compile-flag calibration → `docs/zlib-matching.md`; library/CRT/MFC
  linkage evidence → `docs/libraries-and-funcid.md`; codegen idioms →
  `docs/matching-patterns.md`; toolchain identity → `docs/toolchain-vc50-sp3.md`.
