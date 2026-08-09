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
  the span to its neighbour. Six overlaps originally fell out. Three are now
  resolved: `g_singleCmdList`, `g_multiCmdList`, and `g_pool` were fake names for
  real `CPtrList` template-static specializations, while each adjacent `…Count`
  was the list's inherited `m_nCount` member at `+0xc`, not independent storage.
  The remaining overlap worklist is `g_smallFont` (`Font` 0x18 swallows
  `g_loadedFlag`), `g_panTable` (mangles `PAHA` = `int*` = 4 but the declared type
  sized 0x20), and `g_imageCache` (`CPtrArray` 0x14 swallows
  `g_imageCacheIndex`). Audit it with
  `python -m gruntz.build.data_manifest --report`.

### 3b. `--data-section-manifest` is IN — the container artifact is dead (DONE)

Placing definitions at candidate `section_offset`s turned out **not** to need
contribution ranges (see the correction below). `data_manifest.section_rows()` now
emits the candidate section manifest and `delink.py` passes it.

**The defect it kills.** cl.exe emits every `??_C@` literal as its OWN COMDAT section
holding one symbol at offset 0; the delinked target PACKED a unit's literals into one
blob (`soundfontpath`: base `0x15|0x16|0x0e|0x0f` vs target one `0x49`). `objdiff-cli
report generate` defaults to `combine_data_sections=true`, so it diffed the packed blob
against the base's COMBINED-COMDAT layout — every payload present, all at shifted
offsets → ~99.3%, never the **exact 100.0** that `report.rs` demands before it credits
a section. *`matched_data` is all-or-nothing PER SECTION; naming/enrolling alone could
never move it.*

**`combine_data_sections` IS steerable, and it is not the fix** (measured 2026-08-09,
objdiff-cli 3.7.3). `report generate` takes `-c key=value` and validates the key
(`-c bogus_key=1` → `Failed: Invalid configuration property`), so
`-c combine_data_sections=false` runs: 344 units become 2714 `.data` + 2009 `.rdata`
rows, `total_data` 704148 → 694617 and `matched_data` 16.40% → **17.74%**, with
`fuzzy_match_percent` (91.9774) and `matched_code` (475702) **bit-identical** — it
cannot touch function scoring. But the all-or-nothing credit lives in `report.rs` and
has no config key, so splitting the sections just makes more of them miss 100.0.
⇒ **report both numbers instead of choosing one.** `gruntz.core.report.data_measures()`
computes the size-weighted figure from the report's own per-section rows; the build
scoreboard and the README block print it beside `matched_data`. Today: **99.16%
size-weighted over 704,148 B** (`.bss` 99.76, `.data` 95.89, `.rdata` 99.28) against
`matched_data`'s 16.40%. The weighted number tracks reconstruction; the
all-or-nothing one tracks how many sections are FINISHED, and `.bss`'s 1.54% there is
the symbol-size inference hole, not reconstruction debt
(`docs/patterns/bss-symbol-size-inference-hole.md`).

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

### 3b-ii-b. The `offset == 0` test silently skipped the `/GR` majority (fixed)

§3b-ii's enrolment demanded a COMDAT with **exactly one member, at offset 0**. That is the
shape of a *non-`/GR`* vtable. Under `/GR` cl puts the **`??_R4` complete-object-locator
POINTER at offset 0** (an unnamed word — the COMDAT's only defined symbol is still the
vtable) and `??_7<class>@@6B@` at **offset 4**, which is exactly why the selection is
`SELECT_LARGEST` (6): the four-bytes-bigger `/GR` copy beats a non-`/GR` TU's bare slot
array. Measured over every base obj: **101 distinct `??_7` at offset 0 versus 140 at offset
4 (369 definitions)** — so the common case failed the test and was skipped **without a
withheld row**. Those vtables were never materialized into a target object, so objdiff
could not compare them and **no defect in them was scorable at all**.

Three independent things had to change:

1. **`vtable_rows()` accepts either offset**; the enrolled section spans the whole COMDAT
   (`rva = vtable_rva - offset`, so the COL word is carried and relocated with it) and the
   contradiction check becomes `offset + slots*4 == candidate section size`.
2. **`retail_col_head()` reads the SHIPPED IMAGE, not our compile.** Whether retail's copy
   has the COL word is an independent fact: the word at `vtable-4` must carry a base
   relocation AND point at an `.rdata` COL (signature 0, `pTypeDescriptor` at +12 naming a
   `.?A…` type descriptor). The reloc alone is not enough — for **45** of the 233 names the
   word at `vtable-4` is the PREVIOUS vtable's last slot, which relocates too.
3. **The delinker's COMDAT leader may sit at a non-zero offset**
   (`nix/patches/vostok-comdat-leader-nonzero-offset.patch`): `finish_data_comdats` had
   demanded an external definition at offset 0, which is neither what COFF requires nor
   what cl emits. It now takes the section's lowest-offset external definition.

Outcome, per distinct `??_7` name (233 in the base objs): **125** agree with retail (COL,
symbol at 4) and enroll placed; **86** have neither and enroll as before; **6** are emitted
BOTH ways (one `cpp` TU and one `cpp-rtti` TU compiling the same class — `CObject`,
`CImage`, `CGameApp`, `CGameMgr`, `CGameWnd`, `CGruntzMapMgr`), where the `/GR` emitters take
the placed section and the others enroll UNPLACED so two section extents never claim one
retail range; **5** are withheld — `CFaderFlat/Light/Radial/Shape/Sine`, where **we compile
`/GR` and retail did not**, so the section would claim four retail bytes that are actually
the `g_faderHalfPi` / `g_faderOne` float constants beside them; **11** are secondary/MI
vtables with no primary slot map.

|  | before | after |
|---|---|---|
| enrolled vtable definitions | 158 | **514** (+356, none lost) |
| distinct vtable names | 92 | **217** |
| `??_7` symbols in the target objs | 163 (97 distinct) | **516 (219 distinct)** |
| `total_data` | 637720 | 660086 |
| `matched_data` | 55214 (8.66%) | 55326 (**8.38%**) |
| `matched_code` / `exact` | 471777 / 3471 | unchanged |

The percentage falls for the §3b-ii reason: `matched_data` credits a combined section only
at exactly 100.0, and the units gaining vtables had `.rdata` below 100 already, so +22366
bytes of newly-visible content lands entirely in the denominator. **That is the point** —
what was invisible is now diffable. Comparing every base vtable COMDAT against its target
counterpart byte-for-byte and reloc-for-reloc gives **354 of 444 present (name, object)
pairs clean but for two systematic naming families**, and **90 pairs / 46 names with a real
residual defect**. The three families, each its own follow-on:

* **COL word (336 sites, 125 vtables).** The target's offset-0 reloc names
  `??_7CMenuItem2@@6B@+addend`: no manifest row covers the COL, so
  `relocs.rs` falls through to `hypothesis_owner_and_addend_for_rva`, which returns the
  CLOSEST `.rdata` definition. Fixing it means enrolling the RTTI graph itself
  (`??_R4` → `??_R3` → `??_R2` → `??_R1` → `??_R0`; the base objs define **717** of these,
  and their names are readable straight off cl's own relocations, the `apply_string_names`
  oracle applied to RTTI).
* **`??_E` vs `??_G` (416 sites, 175 vtables) — a SCORING ARTIFACT, not a defect.** Our base
  obj's vtable slot names `??_E<C>@@UAEPAXI@Z`, which is a COFF **WEAK EXTERNAL**
  (`IMAGE_SYM_CLASS_WEAK_EXTERNAL`, storage class 105) whose default is the `??_G<C>` the
  same object DEFINES. The linker resolves it, so the delinked target correctly names
  `??_G`. Resolving base-side weak externals to their default in `normalize_objs.py` is the
  faithful fix.
* **MFC base slots mislabeled on the TARGET side (314 sites, 40 vtables).** e.g. base
  `?GetRuntimeClass@CObject@@UBEPAUCRuntimeClass@@XZ` versus target
  `?AfxExtractSubString@@YGHAAVCString@@PBDHD@Z` — a `library_labels.csv` / FLIRT naming
  defect, now visible.

The genuine per-class residuals this exposed are the deliverable: `??_7CGruntzCommand@@6B@`
has real `Serialize`/`Save`/`Load`/`GetTag`/`Parse` bodies where **retail has `__purecall`**;
`??_7CGameWnd@@6B@` / `??_7CGruntzWnd@@6B@` disagree on slots 1–3
(`PreDispatchMessage`/`HandleWindowCommand`/`OnCreate` versus retail's `FUN_00494c40`,
`FUN_00494c60`, `GameWindowProc`); `??_7CGameApp@@6B@` disagrees at +0x34.

### 3b-ii-c. All three families closed; `.rdata` 1.3% → 66.3% (fixed)

Each family had a different mechanism and a different home. **None of them was a source
defect**, and the function side did not move by a byte (`matched_code` 472319,
`matched_functions` 3469, `fuzzy` 90.907135, identical before and after).

**1. The RTTI graph was not in the manifest at all** (357 comparable COL sites over 132
vtables). `data_manifest.rtti_rows()` enrolls `??_R4`/`??_R3`/`??_R2`/`??_R1`/`??_R0`.

> **Both graphs are walked IN PARALLEL from one anchor per class.** The retail image gives
> the ADDRESSES (`vtable-4` → COL → hierarchy descriptor → base-class array → base-class
> descriptors → type descriptors) and the base object that emitted the same vtable gives the
> NAMES at the identical offsets. Nothing is mangled by hand: a `??_R1`'s spelling encodes
> its PMD (`??_R1BFA@?0A@A@CWapX@@8` = mdisp 0x150, pdisp −1, vdisp 0) and a `??_R0`'s
> encodes the decorated type name, so re-deriving either would be a guess where cl already
> wrote the answer. This is the `apply_string_names` oracle applied to RTTI.

Every node is then **re-proven byte-for-byte** against the shipped image with the relocated
dwords masked (the type-descriptor name string, the PMD displacements, the base counts). A
node the walk reaches at two addresses, or whose bytes contradict the candidate record, is
withheld. Measured: **131 classes walked, 666 distinct nodes located, 0 name/rva conflicts,
0 payload mismatches**; 1940 rows (one per owning unit — RTTI folds exactly like a string
literal or a vtable) enroll with **0 withheld**, and no other withheld class in the manifest
moved. Shapes, read off cl's output: `??_R4` 20 B and `??_R3` 16 B and `??_R1` 24 B in
`.rdata$r`; `??_R2` is `n*4 + 1` (cl NUL-terminates the base-class array); `??_R0` is
`8 + strlen(name) + 1` in **`.data`**, because the runtime writes its `spare` field.

`nix/patches/vostok-grouped-section-names.patch` was needed: the section manifest's storage
check demanded an exact `.rdata`/`.data`/`.bss`, with one hand-rolled `.CRT$` exception. A
`$` suffix is COFF's **grouped-section** form (a linker ordering key, stripped at link time)
and cl puts every RTTI record in `.rdata$r`. It compares the group prefix now.

**2. `??_E` → `??_G` weak externals** (477 sites over 190 vtables) — a pure scoring artefact,
fixed in `canonicalize_data_symbols.canonicalize_coff` through the existing `dup_retargets`
path, so the fail-closed postcondition already covers it. **No `src/` change.** The
whole-link precondition (a weak external resolves to its default only while nothing defines
it strongly) is not assumed: `normalize_objs` re-proves it over the whole processed set every
build and fails loudly otherwise. Measured 508 weak `??_E<C>@@UAEPAXI@Z` references and 6
strong `??_E` definitions, and the two name sets are disjoint — the strong ones are
non-virtual `QAEPAXI@Z` bodies and `W7AEPAXI@Z` thunks, a different mangling.

**3. MFC base slots** (155 sites over 52 vtables). `gruntz.audit.vtable_slot_labels` reads
`(name → retail rva)` out of the vtable slots themselves: the base object names slot *i*, the
retail vtable holds its address. Accepted only when the map is 1:1 in BOTH directions, only
for rvas no src claim (address or extent) and no active library row already covers, and with
the retail bytes compared against our own COMDAT wherever we emit one. **45 accepted, 0 model
contradictions, 3 ambiguous.** 32 are MFC bodies and become `HIGH` rows in
`library_labels.csv` (the `mfc-4.2-header-inline` precedent: `CObject::Serialize` and friends
are `_AFX_INLINE` in `AFX.INL`, so cl materialises them as COMDATs inside a game TU's
contribution). The wrong `LOW` FID row `0x014be0,__fpclear` is pruned — that address is
`CObject::AssertValid`.

**Why an unlabelled MFC virtual did not merely show up blank.** `relocs.rs` names a `.text`
address by the closest PRECEDING function, and a vtable slot holds an `/INCREMENTAL` **jmp
thunk**, so the "closest preceding function" is *the previous 5-byte thunk*, which forwards
somewhere unrelated: `CObject::Serialize` printed as `?ToggleRegionA@CTriggerMgr@@QAEHXZ+5`,
once per vtable. Off the thunk band it is the enclosing admitted boundary instead —
`?AfxExtractSubString@@YGHAAVCString@@PBDHD@Z+0x78` for `CObject::GetRuntimeClass`.

`synth_pdb` also had to synthesize a record for the 32: a library label carries no extent
(the CSV has no size column) and the inventory never carved these bodies — the linker packed
them **unaligned INSIDE a neighbouring admitted boundary** (`0x1bef01` sits inside
`0x1bee89+0x120`). The record binds a NAME to an ADDRESS; its extent is the distance to the
next boundary we do know, which is a bound and not a claim about where the body ends. It is
restricted to library rows — src claims with no `@size` keep the old WARN-and-skip, or 11
game functions absent from `functions.tsv` would start being carved (measured: `total_functions`
4301 → 4312, `fuzzy` 90.907 → 90.834).

| | before | after |
|---|---|---|
| `??_7` (name, object) pairs present | 515 | 515 |
| byte- and reloc-clean | **13** | **498** |
| residual pairs / distinct names | 142 / 60 | **16 / 8** |
| COL sites wrong | 357 | **1** |
| `??_E`/`??_G` sites | 477 | **0** |
| MFC-label sites | 155 | **0** |
| `.rdata` bytes / at 100% | 31566 / 416 (1.3%) | **62598 / 41496 (66.3%)** |
| `.data` bytes / at 100% | 88384 / 46609 (52.7%) | **101368 / 58805 (58.0%)** |
| `total_data` | 660102 | 704118 |
| `matched_data` | 55326 (8.38%) | **108602 (15.42%)** |
| `matched_code` / `matched_functions` / `fuzzy` | 472319 / 3469 / 90.907135 | unchanged |

**The eight names still residual are real signal.** Thirteen `CGameWnd`/`CGameApp` addresses
and four `??_G` scalar-deleting destructors (`CResolveNode`, `CRgn`, `CSBI_SideTab`,
`CSBI_StatzTabArrow`) are **byte-identical compiler-generated COMDATs of OUR classes** with
no claim on their retail rva — so `CGameWnd` slots 1–3 and `CGameApp` **+0x30** (not +0x34)
are NOT model defects, they are missing `RVA_COMPGEN` pins; `vtable_slot_labels` prints them
under `home = src-RVA_COMPGEN`. The one real model defect left is `CDDrawWorkerA` slots
5/7/8, which point at `0x157060`/`0x157130`/`0x1570a0` while our model inherits
`CDDrawWorkerBase::IsLoaded`/`Unload`/`GetClassId` (claimed at `0x157200`/`0x157310`/
`0x157210`) — three unreconstructed `CDDrawWorkerA` overrides. `??_7CGruntzCommand@@6B@` is
now clean, confirming its `__purecall` fix. The last COL site is
`??_7?$zDArray@P8CUserLogic@@AEHXZ@@6B@`, which has no primary-vtable slot map.

### 3b-iii. `DATA_COMPGEN(rva, name, value)` — reviewed compiler-generated data (wired)

Adopted from homm2-decomp's contract (its `docs/candidate-data-topology.md`): automatic
string inference cannot establish retail identity when a payload content-matches several
retail RVAs (the "identical payload at N retail RVAs" withheld class - short strings whose
bytes also occur inside other data), when Ghidra never carved the literal, or for FP pool
entries (no content-derived name exists at all). The macro wraps the value AT ITS USE SITE
and expands to it under both compilers:

    g_pathStr += DATA_COMPGEN(0x0020cfbc, wwdExtension, ".WWD");
    health * DATA_COMPGEN(0x001e9a98, healthSlotScale, 0.2)

`labels.py` parses the invocations (balanced-paren - expression position wraps), then
authority-checks each claim against the claiming TU's base obj: a string payload must
equal a `??_C@` COMDAT there (cl's own spelling for those bytes IS the emitted name), a
float's bits must sit in the TU's `$T` FP pool (emitted spelling `$T<rva>`, which only has
to satisfy canonicalize's VOLATILE_T - both sides content-address to `$anon_f64_<bits>`,
so the volatile counter never matters). Claims land in `build/gen/data_compgen.csv` (one
row per claiming unit) + one representative `symbol_names.csv` row per rva; the data
manifest enrolls them via `compgen_rows()` (strings take the candidate-COFF section
shape, floats the legacy packed form).

**Gates (labels.py, FATAL):** semantic name unique per TU; one compiler-generated
identity per RVA - EXCEPT byte-identical string payloads, which /Gf pooling (implied by
/O2) legitimately folds from N TUs onto ONE retail RVA (`docs/string-pooling.md`); those
claims coalesce onto the one `??_C@` name and enroll once per owner (the §3b-i alias
form). This per-RVA relaxation is the deliberate divergence from homm2's stricter
"different names at one RVA are rejected" rule: VC4.2 there, VC5 pooling here. FP pools
never fold, so a numeric RVA claimed by two TUs is always a mis-pin. The semantic name is
per-TU documentation only - it never reaches the delinker.

First proven claims: the `".WWD"` disambiguation (0x20cfbc vs the `"*.WWD"` tail at
0x20cf95 that inference withheld), a 2-TU `"Wormhole"` fold (gameobjectfactory +
wormhole at 0x20a7ac), and grunthealthsprite's `0.2`/`0.5` FP pool entries
(0x1e9a98/0x1e9aa0) - the first FP data to pair at all.

### 3b-iv. `config/retail/compiler-generated-data.tsv` — COMMON pins (wired)

The class §3b-iii cannot express: a datum with **no source site to attach to at all**.
`DATA_COMPGEN` needs a value expression; a `??_B` dynamic-init guard byte has none (cl
assigns it a counter). And `DATA()` needs an AST VarDecl in the MAIN file; a function-local
static inside a **header** inline is not there.

cl emits both as a COFF **COMMON** — a tentative definition — into every TU that
instantiates the inline, and the linker merges them into one bss slot. That is also why
the pin is a manifest rather than a source macro: there is **no owning TU** for a source
position to encode (contrast `RVA_COMPGEN`, whose position IS its TU-ownership proof and
is ratcheted by `compgen_order`).

Columns `rva`/`size`/`symbol`/`emitter`; only the retail ADDRESS is stated. Per TU,
`labels.compgen_data_tu` emits a `symbol_names.csv` row only when that unit's base obj has
the symbol as a COMMON of exactly the pinned size, so every emitting unit re-proves the
pin (`write_symbol_names` then dedups the copies to one representative row per rva, and
`candidates()` enrolls it in the data manifest as ordinary `bss`).
`gruntz.audit.compgen_data` (normal tier, FATAL) adds spelling, the `symbol_names` binding,
and a **coverage ratchet**: every COMMON in any base obj must be pinned.

Coverage is the point. Nothing else in the pipeline can see this class: objdiff masks
relocations so an unnamed COMMON costs 0%, and it links cleanly (`gruntz link` resolves
each as `<common>`), so `link_defects` is silent too. `assert_relocs` was the only reporter
and it mis-read COMMON as an unresolved external — see its `defined_syms`, now fixed.

First (and currently only) claims: the three `GetRandomNumber` guard/seed pairs —
0x2c127d/0x2c1288 (the free function in `<Gruntz/GameRand.h>`), 0x2c278c/0x2c2798
(`CAniRecordView`), 0x2c279c/0x2c27a8 (`CFaderSine`). 26 `assert_relocs` FAKE → 0,
byte-neutral (3322/4290 exact, 89.08% fuzzy unchanged).

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

## The access map (`gruntz.audit.data_access`)

The mis-typed-globals audit: `python -m gruntz.audit.data_access` decodes EVERY
`.text` reloc site into `.rdata`/`.data` (objdump over the whole section, one
pass) into an access event — width, direct/indexed/imm/lea/indcall mode,
read/write, movzx/movsx signedness, FPU f32/f64 witness — plus every data-side
reloc cell (fn-ptr vs data-ptr content). Attribution charges a symbol only up
to its DECLARED size (reviewed `symbol_names.csv`, else `globals.json` sizeof);
everything past coverage clusters into candidate missing-global RUNS, with the
unpinnable string pool down-ranked by a byte peek. A `TypeOracle` over
`structs.json` keeps the verdict flags honest (a struct whose fields contain
pointers/floats never flags; unresolvable typedefs are conservative). Output:
`build/gen/data_access.tsv` + `--flagged` / `--unclaimed` / `--rva 0x...`
views. Runs with the same site-count/width fingerprint are instances of the
SAME unmodelled struct type — that is the shape a dynamic trace would have
given us, recovered statically and exhaustively (the deferred debugger trace is
a second producer into the same event schema, for pointer-mediated interior
accesses only code-flow can reveal).

### Access-map campaign state (2026-08-06, lane/r2)

The 260-item mis-typed-globals worklist (8 flagged + 252 unclaimed runs) is
resolved to: **0 flagged symbols**, and unclaimed runs reclassified to
31 data / 105 library / 836 string-pool / 1 alias. Landed devices: the
GruntDirStatics per-TU static-copy family (66 full + 14 split copies,
`config/static_data_copies.tsv` folded in at merge_labels), 45 FP pool
constants as `DATA_COMPGEN` at their use sites, the CButeMgr getter-static
band, message maps for CBattlezDlgColors/CMultiStartDlg (11 handler stubs
claimed from pfn evidence), the DirectInput GUID triple, and ~40 per-slot
zero-init globals in owner TUs. The 31 residual runs are ALL accessed only
by unreconstructed (<gap>) code - their owner TUs are unprovable until the
accessor fns are matched, so they stay on the queue rather than taking
fabricated homes (`python -m gruntz.audit.data_access --unclaimed`).
Library runs (CRT/MFC/zlib data) are policy-excluded like library code;
string-pool runs are the unpinnable pooled literals.
