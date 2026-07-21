# DATA-section attribution + the data-match loop

Adopted from the sibling **homm2-decomp** engine (its `docs/data-symbol-normalization`,
`reviewed-data-objdiff`, `static-storage-link-audit`). objdiff scores `.text` well but
Gruntz historically had **no DATA loop**: nothing verified `.rdata`/`.data`/`.bss` BYTES
against retail, and the delinked target's data symbols didn't pair with the base's. This
doc covers the two pieces now in-tree and the staged path to a full data-byte loop.

## 1. Compiler-private data-symbol normalization (matching-neutral, wired)

MSVC 5.0 names compiler-private data with unstable counters — `$SG30360` (string),
`$T42489` (temp/const), `name$S31565` (named static) — and spells in-`.text` jump/switch
tables with local `$L<n>` labels. The base obj and the delinked target obj therefore
carry *different spellings of identical data*, so objdiff can't pair them.

`scripts/gruntz/build/canonicalize_data_symbols.py` (ported from homm2) rewrites those
names in **disposable COFF copies** under `build/objdiff/normalized/{base,target}/` into
content-addressed identities — `$anon_str_<sha>`, `$anon_f32_<val>`, `$anon_f64_<val>`,
`$anon_data_<sha>` — and rewrites same-function `$L` jump-table `DIR32` relocs to
*containing-function + owner-relative addend* (how the delinked target already spells
them). A fail-closed reparse proves ONLY symbol names + authorized jump-table reloc fields
changed and every resolved section offset is identical, so it can only sharpen objdiff,
never inflate a false match. The real `base/` and `delink/` objs are untouched.

`scripts/gruntz/build/normalize_objs.py` drives it from one ninja edge (keyed on the base
objs + the delink stamp; mtime-skips unchanged objs); `objdiff.json` pairs the normalized
copies (`configure.py:emit_objdiff`). It is matching-neutral: over all base+target objs the
exact-match count is unchanged (247136 code / 2366 fns), with a small fuzzy gain from the
jump-table alignment. Its full string/const **pairing** payoff unlocks once the delinker
emits per-symbol MSVC-private data names on the target side (§3) — today the stock delinker
emits almost none, so `matched_data` stays ~4/69184.

Audit any object: `python -m gruntz.build.canonicalize_data_symbols --input X.obj
--output /tmp/X.obj --sidecar /tmp/X.tsv` (the `.symbols.tsv` sidecar lists every rename +
its proof). Corpus census: `--summary-root build/objdiff/base --summary-root
build/objdiff/target`.

## 2. Retail data attribution + fingerprint — `gruntz data-audit` (wired)

`scripts/gruntz/core/data_audit.py` ports homm2's pure-PE evidence core
(`link_exe.py: read_pe / classify_pe_storage / read_pe_payload_evidence`). Reading ONLY the
retail `GRUNTZ.EXE` (no delinker/PDB/wine), for every `kind=data` symbol in
`symbol_names.csv` it:

- classifies PE storage (`.rdata` ro / `.data` initialized / `.data` **unprovable tail** /
  `.data` loader-zero tail / other / outside);
- resolves an EXTENT: reviewed `symbol_names.csv` size, else the next-data-symbol gap
  (flagged `next-symbol-gap`);
- reads that span, zeroes every HIGHLOW base-relocation field, and records
  `sha256`, `normalized_sha256`, and the HIGHLOW offset set — a base-independent
  content+relocation fingerprint.

Output `build/gen/data_attribution.tsv` is the reviewable attribution ledger; `--json`
emits full per-symbol evidence, `--rva 0x..` probes one symbol. Current census: 929 data
symbols (rdata=305, data-init=112, bss-tail=414, unprovable-tail=3, other=95); 414
rdata/data spans fingerprinted; 395/929 extents still come from the next-symbol gap — the
"DATA() has no size" gap (§3). Identical vtables surface immediately:
`CActionArea`/`CUserLogic`/`CGuardPoint` share `normalized_sha256 10eef285…`.

**`data-unprovable-tail` — the rawsize-edge artifact (fixed).** `data-initialized` vs
`data-loader-zero-tail` splits on `raw_size`, but MSVC merges `.bss` INTO `.data` and
`SizeOfRawData` is `round_up(E, FileAlignment)` for the true end `E` of emitted content.
So `E ∈ (raw_size − FileAlignment, raw_size]` and the last `<FileAlignment` file-backed
bytes may be alignment padding that the first `.bss` symbols already occupy — an all-zero
run there is byte-identical to a zero-valued `.data` global and the PE cannot tell them
apart. Measured on GRUNTZ.EXE: `.data` rva `0x208000`, `raw_size 0x21400`,
`FileAlignment 0x200` → the unprovable window is exactly **rva [0x229200, 0x229400)**.
Three symbols sit in it — `?g_projReg@@3UCCoordColl@@A`, `?g_projRegColl2@@3PAUCVariantSlot@@A`
(`actionarea`) and `_g_emptyString` (`netmgrerror`, 12 bytes from the edge). They are now
reported `data-unprovable-tail`; `data_manifest.STORAGE` does not map that class, so they
are withheld rather than asserted.

Note the trailing all-zero run is `0x3ae0` bytes — far bigger than FileAlignment — but the
zeros *below* `0x229200` are **proven** emitted content (`round_up` could not have produced
`0x21400` from a smaller `E`), so they stay `data-initialized`. Only the last `0x200` is
unprovable, and a nonzero byte anywhere in `[offset, raw_size)` resolves it back to
`data-initialized`.

**Withholding those two enrolled rows RAISED `matched_data` 26.63% → 27.66% (+3000
bytes)** — asserting `.data` for them had been breaking their containers; `netmgrerror`'s
2920-byte `.data` went to **100%**. The correctness fix and the metric agreed.

This is the retail oracle a real data-byte loop gates against: once source data
initializers relink (or the delinked target carries typed data), compare candidate bytes
(relocs normalized) to these digests.

## 2b. Link-side DATA static-storage audit — `gruntz audit exe-diff` §E (wired)

`gruntz link` (Phase 2, VC5 link.exe 5.10.7303, `/FORCE`) links the base objs into a
candidate `GRUNTZ.EXE` + `.map`. `gruntz audit exe-diff` already audits `.text` layout/bytes; it
now also runs a **DATA static-storage** audit (§E), ported from homm2's
`link_exe.py: static_symbol_diagnostics` + `classify_pe_storage`. It joins each retail data
symbol to its candidate `.map` entry, classifies `.rdata` / initialized `.data` /
loader-zero storage on both sides, and reports the absolute RVA delta **and** the
**section-relative delta** (whole-section drift subtracted, so a section shift is not
mistaken for a contribution error), listing the first divergence per section — the earliest
credible contribution cause to fix, exactly as homm2's method prescribes (fix the earliest,
relink; later rows are cumulative consequences).

This is the DATA analog of the `.text` layout levers: it verifies each global lands at its
retail offset *within its section after a real link* — something no per-object diff can see.
Retail data owners come from Ghidra + `DATA()` (`symbol_names.csv`), substituting for
homm2's CodeView `sstModule` inventory (GRUNTZ.EXE has no debug stream).

Measured on the first real candidate link (392 objs, 4886 unresolved externs under
`/FORCE`):

```
retail data symbols (DATA()/Ghidra) : 925   rdata=306 data-init=115 bss-tail=409 other=95
DEFINED + placed by candidate link  : 771/925 (83.4%)   (rest extern/unresolved)
at CORRECT absolute retail RVA      : 0/771             (needs link order + full coverage)
at CORRECT section-relative offset  : 0/771             (the real contribution signal)
storage-class (.rdata/.data) matches: 715/771
first divergence: ??_7CActionArea@@6B@ rdata ret 0x1e7004 cand 0x101000 Doff -0x4
```

## 3. Why `matched_data` is ~0, and what it actually costs to fix (MEASURED)

Historical (kept for the mechanism; the numbers are superseded by §3b): `matched_data`
was **4 / 69184 bytes (0.006%)** vs homm2's **305328/305328 = 100%**. It is now
**80902/292476 = 27.66%** — and §3c explains why the `.bss` share of the remainder is
not reachable at all.

**Root cause (measured, not naming).** The delinked target objs already carry REAL data
names — `??_7CActionArea@@6B@` in `.rdata`, `_g_gameReg`, `?g_buteMgr@@3VCButeMgr@@A`. The
problem is that **objdiff compares data at SECTION granularity**: our base `.rdata` has the
MSVC pool layout (vtable + `$T` float pool + literals) while the delinked target `.rdata`
holds only the pieces vostok extracted. The two sections do not align, so almost nothing
matches regardless of symbol names. Content-addressing names (§1) therefore cannot move
`matched_data` on its own. homm2 reaches 100% only because its delinker REBUILDS
**candidate-shaped** target sections (`--data-section-manifest`) and places each definition
at the candidate's own `section_offset` (`--data-manifest`).

**The bump is NOT just a flake-input change.** Measured by running homm2's data-topology
`vostok-delinker` (`/nix/store/5pc398fq…`, all four manifest flags) directly on Gruntz's
existing synth PDB + retail EXE:

1. Default (canonical) mode → `Error: no candidate writable identity can represent retail
   RVA 0x229328`, 0 objs. It is fail-closed: every writable RVA must be covered by a
   data/section manifest.
2. With the bootstrap escape hatch `--recover-data-relocs-from-pdb` (± `--coalesce-common-functions`)
   → `Error: IAT relocation target 0x2c44ac has no exact PDB symbol`, 0 objs.

So there are **two hard prerequisites** before the new delinker emits anything:

- **(a) IAT symbols in the synth PDB.** `.idata` is retail section 4 (RVA `0x2c3000`,
  vsize `0x3b41`); `synth_pdb.py` maps only `.text`/`.rdata`/`.data` → segments 1/2/3 and
  emits no `.idata` symbols. The new vostok reconstructs `__imp__…` COFF relocations from
  CodeView-backed `.idata` symbols and errors without them. Fix: add `.idata` as segment 4
  and emit an `__imp__` data symbol per IAT slot. Exact decorations are available as
  evidence — the base objs already carry **150** real MSVC5 spellings
  (`__imp__AIL_init_sequence@12`, …); join them to the retail import table by undecorated
  name, and take the remainder from the MSVC import libs (`$MSVC_DIR/lib/*.LIB`), which
  carry the authoritative `@N`. Do NOT invent `@N`.
- **(b) Candidate data + section manifests.** The delinker needs each definition's owning
  object, storage, alignment, **`section_offset`**, and scope, plus a candidate section
  table — homm2 derives these from the candidate COFFs (`candidate_data_manifest.py`) and
  constrains owner resolution with a contribution manifest. Gruntz has no NB09, so the
  contribution ranges must come from the candidate `.map` (`gruntz link` / `link_order.py`)
  or Ghidra, not `sstModule`.

**Extents are now in place (step 1 of that work is done).** `labels.py` resolves each
`DATA()` global's exact `sizeof` from its declared type (§2) — **532/925** data rows carry a
real extent, so the `--data-manifest` has bounded sizes to enroll instead of next-symbol
gaps.

### 3a. Status: (a) DONE, the bump is IN, (b) proven but coverage-blocked

- **(a) DONE** — `synth_pdb.py` emits all **456/456** `.idata` IAT symbols (0 guessed);
  the delinker's IAT hard-error is gone (0 objs → 406).
- **Bump DONE** — `flake.nix` pins the reviewed-data-topology rev and `delink.py`
  passes `--recover-data-relocs-from-pdb`. **exact 2366 → 2385 (+19)**: the branch
  retains real PDB identities instead of coalescing byte-identical function groups.
- **(b) mechanism PROVEN, coverage incomplete.** `gruntz.build.data_manifest`
  generates the manifest from the type-derived extents (519 enrolled). With it the
  delinker runs in STRICT mode (`RVA 0x229328` cleared, 407 objs) and the metric
  finally moves:

  | | no manifest | + data-manifest (strict) |
  |---|---|---|
  | `matched_data` | 8/69184 = **0.012%** | **38275/246684 = 15.52%** |
  | `exact` | 2385 | 2382 (**-3**) |

  **The -3, and how it was closed:** a data manifest is the topology AUTHORITY for the
  objects it names — data it does not enroll stops being materialized into those target
  objects. Enrolling only the `DATA()` globals dropped each unit's compiler-emitted data
  (string literals `??_C@…`, the unsized globals, `$T` pools), and three functions
  referenced exactly that (`soundfontpath BuildSoundFontPath`, `gametext _$E1`/`_$E4`).
  Enrolling each unit's `??_C@` literals alongside the `DATA()` globals fixed it;
  `delink.py` now passes `--data-manifest` **and** `--data-section-manifest` (§3b), plus
  `--recover-data-relocs-from-pdb` as a safety net for anything left uncovered.

- **Bonus: the sizeof extents are a contradiction check.** A reviewed extent must fit
  the span to its neighbour. Six overlaps fell out, each proving one of a pair is
  mis-modelled (neither is enrolled — we do not know which is wrong):
  `g_singleCmdList`/`g_multiCmdList`/`g_pool` (`CPtrList` 0x1c swallows the adjacent
  `…Count` 0xc in), `g_smallFont` (`Font` 0x18 swallows `g_loadedFlag`), `g_panTable`
  (mangles `PAHA` = `int*` = 4 but the declared type sized 0x20), `g_imageCache`
  (`CPtrArray` 0x14 swallows `g_imageCacheIndex`). A real defect worklist:
  `python -m gruntz.build.data_manifest --report`.

### 3b. `--data-section-manifest` is IN — the container artifact is dead (DONE)

Placing definitions at candidate `section_offset`s turned out **not** to need
contribution ranges (see the correction below). `data_manifest.section_rows()` now
emits the candidate section manifest and `delink.py` passes it.

**The defect it kills.** cl.exe emits every `??_C@` literal as its OWN COMDAT section
holding one symbol at offset 0; the delinked target PACKED a unit's literals into one
blob (`soundfontpath`: base `0x15|0x16|0x0e|0x0f` vs target one `0x49`). `objdiff-cli
report generate` hard-codes `combine_data_sections=true`, so it diffed the packed blob
against the base's COMBINED-COMDAT layout — every payload present, all at shifted
offsets → ~99.3%, never the **exact 100.0** that `report.rs` demands before it credits
a section. *`matched_data` is all-or-nothing PER SECTION; naming/enrolling alone could
never move it.*

| | before | + `--data-section-manifest` | + blowfish storage fix |
|---|---|---|---|
| `matched_data` | 41258/274106 = **15.05%** | 58744/275462 = **21.33%** | 67080/279630 = **23.99%** |
| `.data` | 32268/61476 = 52.5% | 49754/62832 = 79.2% | — |
| `exact` | 2384 | 2384 | 2384 |

Nothing is invented: `rva`/`size` stay the PROVEN retail extent, while
name/alignment/characteristics/COMDAT-selection are read out of the candidate COFF.
Ordinals are manifest-local, **contiguous from one**, and follow the candidate's
section order (objdiff stable-sorts same-named sections when combining, so order
decides the combined layout).

### 3b-i. The folded-COMDAT copies are IN — the `duplicate data RVA` limit is gone (DONE)

**The diagnosis, twice corrected.** These 296 payloads were first framed as "retail
owner unprovable, needs `--contribution-manifest`" — the wrong question. A COMDAT is *by
definition* emitted into **every** TU that uses the literal, and the linker folds them to
one surviving rva, so **all** owners are correct and each target object gets its own copy
(our base objs already do: `actionoptionsmenubar.obj` and `statusbarmgr.obj` both define
`??_C@…GAME_INGAMEICONZ_GRE…`, folded to `0x20a544`). There is no owner to attribute. That
left a pure **delinker constraint**: `Error: <manifest>:1801: duplicate data RVA`.

**It was an upstream asymmetry, now patched.** `data_section_manifest.rs` *already*
permitted exactly this aliasing — `compatible_folded_comdat_alias()` admits two sections
at one rva when they agree on name/size/alignment/characteristics/storage/COMDAT-selection
and the selection is one that permits duplicates (`2|3|4|6|7`, i.e. every
`IMAGE_COMDAT_SELECT_*` except `1=NODUPLICATES` and `5=ASSOCIATIVE`) — and
`object_files.rs` already materializes per-object
(`topology_replays_relocations_for_each_folded_comdat_section`). Only `data_manifest.rs`
never got the same treatment. `nix/patches/vostok-data-manifest-folded-comdat.patch`
(pinned via `flake.nix`, **upstream-pending**) mirrors the predicate there.

**Why admitting them cannot make owner resolution ambiguous** (the reason the constraint
was over-strict, not load-bearing): every consumer of a resolved owner —
`relocs.rs:315/450` via `owner_and_addend_for_rva` — reads only `owner.name`,
`owner.storage` and the addend. **`owner.object` is never read.** So for a group agreeing
on all of those, *which* copy is returned is unobservable.

The relaxation is narrow and stays fail-closed: a copy is admitted only when it agrees
with its group on (name, rva, size, storage, alignment) and comes from a **distinct**
object, and only for `external` scope. Two *different* names at one rva, a same-name
disagreement on extent/storage/alignment, one object defining a name twice, and two
`local` statics sharing an rva all still bail (tests
`rejects_duplicate_rvas`, `rejects_folded_comdat_copies_that_disagree`,
`rejects_a_duplicate_definition_within_one_object`, `rejects_local_definitions_sharing_an_rva`).

Measured (A/B on one base, delinker patch held constant, generator toggled):

| | folds withheld | folds enrolled |
|---|---|---|
| `matched_data` | 67080/279630 = **23.99%** | 77902/292484 = **26.63%** |
| `.data` | 58090/67000 = 86.70% | 68912/79854 = 86.30% |
| `exact` | 2386 | 2386 |

**+10822 matched bytes.** `.data`'s *percentage* dips 0.4pt because the fold materializes
+12854 previously-absent real bytes and matches 10822 (84%) of them — absolute matched
bytes, which is what `matched_data` sums, rises. Owners-per-payload histogram:
`{2:224, 3:31, 4:20, 5:6, 6:6, 7:3, 14:1, 16:1, 35:1, 43:3}`; 517 copies enrol (only the
payloads that content-match a retail data symbol reach the manifest).

**Build-graph fix shipped with it:** `delink.py` regenerates both manifests in-process on
every run, but the ninja edge did not depend on `data_manifest.py` — so editing the
generator left objdiff scoring the *previous* manifest until `--force-delink`.
`configure.py` now declares it an implicit dep.

**Contribution ranges are still BLOCKED (measured)** — but only the `$T` pools and
absolute-RVA layout now depend on them. GRUNTZ.EXE has no NB09, so they must come from
our TU partition, which does not hold: only **8 of 86** per-(unit,storage) bands
overlap no other band (rdata 2/15, data 5/17, bss 1/54; aggregates excluded). Measure
overlap **all-pairs**, never adjacent-only — that error reports ~70–80% clean where the
truth is ~9%. See **`docs/tu-partition-brief.md`**.

### 3b-ii. `.rdata` was 419 bytes because the VTABLES were withheld (fixed; vein now visible)

`.rdata` read **68/419 = 16.23%** and looked like nothing was there. It was measuring an
almost-empty container. Of the **305** `kind=data` rows that classify `rdata`, only **68**
carried a proven extent; **237** were withheld `no proven extent` and **220 of those are
`??_7` vtables** — `labels.py` derives an extent by `sizeof()` on a DECLARED C++ type, and a
compiler-emitted vtable has no such type. So the delinker never materialized them and
objdiff compared ~nothing.

**A vtable is emitted exactly like a `??_C@` literal**, so it enrolls through the same fold
path (`data_manifest.vtable_rows()`): measured over the base objs, **235 distinct `??_7`
symbols across 457 definitions**, every one a *lone member at offset 0 of its own `.rdata`
COMDAT* (`comdat=2` PICK_ANY, align 8) — `??_7CUserLogic@@6B@` alone is emitted by **47**
objects and folded by the linker onto one rva. All owners are correct; each target object
gets its own copy.

**The extent is never fabricated — two INDEPENDENT sources must agree:** the retail RTTI
slot map (`vtable_hierarchy`'s registry, read out of the shipped image's COL/base-class
arrays) and the candidate COMDAT cl.exe emitted. `slot_count * 4 == candidate section size`
or the row is withheld — the same contradiction check `section_rows()` applies to a literal.
It immediately caught one real defect (`candidate section 0xc != RTTI 2 slots`) plus 17
secondary/MI vtables (`??_7X@@6B<base>@@@`, left to a later pass).

| | vtables withheld | vtables enrolled |
|---|---|---|
| `.rdata` | 68/**419** = 16.23% | 20/**21319** = 0.09% |
| `matched_data` | 81691/293441 = 27.84% | 81643/314341 = 25.97% |
| `exact` | 2385 | 2385 |

**Read that table carefully — the old number was the lie.** Enrolling materializes **+20900
bytes of real, previously-invisible `.rdata`**; absolute matched bytes are flat (−48, one
container that had been scoring 100% on incomplete content). The percentage falls only
because the denominator finally tells the truth.

**The vein is REAL and the blocker is now named.** The 147 `.rdata` sections land at
**84–94%, not 100** (rezfile 93.9, movinglogic 86.7, donothing 85.0, …; only 3 tiny
float-pool sections reach 100). `matched_data` credits a section **only at exactly 100.0**,
so ~21 KB sits one step away. The residual ~15% is the **slot pointers**: a vtable's slots
are DIR32 relocs to its virtuals, and a section pairs only when EVERY slot's function is
named — the data-side analog of "a fn flips exact only when its WHOLE referent set is
named". ⇒ **`.rdata` unlocks as vtable-slot function naming completes**, and it pays ~21 KB
when it does. Do not re-derive this; the enrolment is already wired.

### 3c. `.bss` is capped by an objdiff INFERENCE artifact — do not budget against it

**`.bss` is 212211 of 279630 `total_data` (~76%), and `ddsurface` alone is 197144 of it
— stuck at 99.998985% on ONE symbol.** COFF carries no symbol sizes, so objdiff infers
`size = next symbol's offset` (`obj/read.rs: infer_symbol_sizes`), and `diff_bss_symbol`
scores 100 iff the two sizes are equal. MSVC5's `.bss` hole-filling allocator always
parks a 4-byte int in the pad before the first 8-aligned array, so that int measures
**8** on the base and **4** on the target. Both sides are correct; only the measurement
differs. Three `cl /O2` probes prove the layout is **declaration-order invariant**, so
it is not steerable from `src/`.

Do NOT candidate-shape `.bss` to "fix" it: `.bss` has no bytes, so mirroring the
candidate's offsets makes every inferred size agree **by construction, for any set of
globals** — a vacuous 100%. (Candidate-shaping is legitimate for `.data`/`.rdata`
precisely because the delinker still fills the container with retail bytes from each
definition's proven rva, so the byte comparison stays real.) Full mechanism +
the rejected fabrications: **`docs/patterns/bss-symbol-size-inference-hole.md`**.

⇒ **Read `matched_data` as a `.data`/`.rdata` measure.** Its `.bss` share is gated on
tooling, not on reconstruction quality.

**Ordering + gate.** (a) → re-delink → gate `code exact >= 2385`; then (b) incrementally,
enrolling reviewed extents in batches and re-gating each time. Also available, already in
`/nix/store`: homm2's objdiff-cli 3.7.1 + `objdiff-data-symbol-details.patch` (per-symbol
`section` + `data_relocations` JSON rows) which unlocks the project-neutral
`strict_allocation_diff.py`; and `--reloc-alias-manifest` for array-index/negative-addend
spellings (pairs with `assert_relocs`).

What does NOT port: homm2's NB09/`sstModule`-sourced ordering, contribution ranges, and
`cv-public-data` inventory (no debug stream in GRUNTZ.EXE — use Ghidra + candidate `.map` +
`DATA()`); and homm2's VC4.0 LINK 3.00 `/Od` flags (Gruntz is VC5 `/O2` LINK 5.10).
