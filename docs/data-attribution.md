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
copies (`gruntz.compare.project`). It is matching-neutral: over all base+target objs the
exact-match count is unchanged (247136 code / 2366 fns), with a small fuzzy gain from the
jump-table alignment. Its full string/const **pairing** payoff unlocks once the delinker
emits per-symbol MSVC-private data names on the target side (§3) — today the stock delinker
emits almost none, so `matched_data` stays ~4/69184.

Audit any object: `python3 -m gruntz.compare.canonicalize --input X.obj
--output /tmp/X.obj --sidecar /tmp/X.tsv` (the `.symbols.tsv` sidecar lists every rename +
its proof). Corpus census: `--summary-root build/objdiff/base --summary-root
build/objdiff/target`.

## 2. Retail data attribution + fingerprint — `gruntz verify data-access` (wired)

`scripts/gruntz/core/data_audit.py` ports homm2's pure-PE evidence core
(`link_exe.py: read_pe / classify_pe_storage / read_pe_payload_evidence`). Reading ONLY the
retail `GRUNTZ.EXE` (no delinker/PDB/wine), for every `kind=data` symbol in
the Model it:

- classifies PE storage (`.rdata` ro / `.data` initialized / `.data` **unprovable tail** /
  `.data` loader-zero tail / other / outside);
- resolves an EXTENT: reviewed the Model size, else the next-data-symbol gap
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

## 2b. Link-side DATA static-storage audit — `gruntz verify link-tier` §E (wired)

`gruntz link` (Phase 2, VC5 link.exe 5.10.7303, `/FORCE`) links the base objs into a
candidate `GRUNTZ.EXE` + `.map`. `gruntz verify link-tier` already audits `.text` layout/bytes; it
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
Retail data owners come from Ghidra + `DATA()` (the Model), substituting for
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
**639859/704148 = 90.87%**; §3c is why the `.bss` share, once written off as
unreachable, was a measurement artifact rather than a reconstruction gap.

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
- **(b) mechanism PROVEN, coverage incomplete.** `gruntz.delink.data_manifest`
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
  `gruntz.delink.data_manifest --report`.

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
⇒ **report both numbers instead of choosing one.** `gruntz.verify.scores`
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
`gruntz configure` now declares it an implicit dep.

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
It immediately caught one real defect (`candidate section 0xc != RTTI 2 slots`). Secondary/MI
vtables (`??_7X@@6B<base>@@@`) now take their reviewed retail identity and extent from
`data_vtables.tsv`, while the independently emitted candidate COMDAT still has to agree;
the former promised "later pass" was never implemented and silently withheld them.

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
the `g_faderHalfPi` / `g_faderOne` float constants beside them. The former secondary/MI
withheld set is enrolled through the catalog-plus-COMDAT cross-check described above.

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
  `?AfxExtractSubString@@YGHAAVCString@@PBDHD@Z` — a `functions_static_libs.tsv` / FLIRT naming
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

**3. MFC base slots** (155 sites over 52 vtables). the vtable-slot label oracle (retired) reads
`(name → retail rva)` out of the vtable slots themselves: the base object names slot *i*, the
retail vtable holds its address. Accepted only when the map is 1:1 in BOTH directions, only
for rvas no src claim (address or extent) and no active library row already covers, and with
the retail bytes compared against our own COMDAT wherever we emit one. **45 accepted, 0 model
contradictions, 3 ambiguous.** 32 are MFC bodies and become `HIGH` rows in
`functions_static_libs.tsv` (the `mfc-4.2-header-inline` precedent: `CObject::Serialize` and friends
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

**Template specializations bridged (2026-08-10).** `rtti_rows()` built its class list
from the registry alone, and a template vtable's registry key decodes the `??_R0` name
on `@` into a symbol no object defines (`?$CArray@PAU…` → `PAU1::…::?$CArray`), so a
template class's ENTIRE RTTI graph went unwalked — `vtable_rows()` had the
`data_vtables.tsv` name→rva bridge, the walk did not. Bridged (the rva must still be a
registry-located base-0 vtable), `CArray<PLAYLISTINFOSTRUCT*>`'s graph
(COL `0x1f4320` → R3 `0x1f4308` → R1 `0x1f42d8` → R0 `0x20ceb0`, previously unclaimed
`.data`) enrolls for its three emitters. Known cost: the walked
`??_R1A@?0A@A@zErrHandling@@8` makes gametext/chatboxowner's combined `.rdata$r`
comparable, and their base-side `??_R4zPTree@@6BzErrHandling@@@` — a secondary/MI
record formerly withheld by the missing MI pass — is now reached from the reviewed
secondary-vtable anchor. Its node names still come from cl's relocations and every
payload is independently re-proven against retail before enrollment.

### 3b-iii. `DATA_COMPGEN(rva, value)` — the LAST-RESORT reviewed pin (wired)

**The rule (one sentence): a `DATA_COMPGEN` claim exists only where the automatic
identity oracles cannot establish the retail identity of a compiler-generated datum;
everywhere else the literal is written bare.** The two oracles, both re-proven every
build:

* **strings** — `string_rows()` content-matches every retail-referenced address (the
  PE `.reloc`-derived data-symbol universe, `synth_pdb.read_data_symbols`) against
  the base objs' `??_C@` pools; `synth_pdb.apply_string_names` names the same
  addresses in the fake PDB, which is what keeps the *referencing functions*
  matching with no pin at all. Inference withholds only an ambiguous payload
  (identical bytes at several retail addresses — 1–2 byte literals, mostly).
* **FP pool slots** — `fp_pool_rows()` addresses cl's `$T` pool out of retail's own
  relocation table (positional pairing of DIR32 sites inside each corroborated
  referrer, self-proving, byte-re-proven). A slot is unreachable only while NO
  referrer function is reloc-corroborated ("stranded").

Only in those two failure classes does the macro wrap the value AT ITS USE SITE
(expanding to it under both compilers — byte-neutral by construction):

    t += DATA_COMPGEN(0x00212754, " ");                  // ambiguous payload
    frac * frac * DATA_COMPGEN(0x001e9a40, 750.0)        // stranded FP slot

`labels.py` parses the invocations (balanced-paren - expression position wraps), then
authority-checks each claim against the claiming TU's base obj (FATAL, per TU): a
string payload must equal a `??_C@` COMDAT there (cl's own spelling for those bytes
IS the emitted name), a float's bits must sit in the TU's `$T` FP pool (emitted
spelling `$T<rva>`, which only has to satisfy canonicalize's VOLATILE_T - both sides
content-address to `$anon_f64_<bits>`, so the volatile counter never matters). The
claim travels as an ordinary `kind=data` row in the TU's label fragment into
the Model; the data manifest enrolls it through `candidates()`
(`src-DATA-sizeof`), and a float claim additionally feeds the `fp_pool_rows()`
bridge for its stranded slot (`src-DATA_COMPGEN-fp-pool`). There is no separate
claims table: `build/gen/data_compgen.csv` and `compgen_rows()` are RETIRED — the
per-TU fragment build had left that channel dead (the table was clobbered per TU at
a path nothing read) while the label channel carried every claim, measured
by A/B removal (2026-08-13).

**Cross-TU discipline is enforced downstream, not in extraction** (each ninja
label invocation sees one TU): two names at one rva withhold BOTH in the manifest,
and any de-enrolled datum fails the FATAL data-census partition gate on the same
build. Byte-identical string payloads from N TUs legitimately coalesce onto one
`??_C@` name (/Gf pooling, `docs/string-pooling.md`), and two of our TUs may both
spill the FP literal that retail's TU boundary put in ONE pool slot (the
`kitchenslime`/`pathhazard` 0x1ea400 fold) — a same-value cross-TU claim is a fold,
not a mis-pin.

**Removal is self-verifying.** A pin whose slot an oracle covers is noise: removing
it moves nothing (the manifest row survives under the oracle's provenance, the
partition reproduces). A load-bearing pin's removal de-materializes the datum and
fails the partition gate on the next build — measured on `"2"`@0x0020b5bc: the
manifest row and its neighbouring gap row vanish, the 2 enrolled bytes become an
eligible-unenrolled run, `data_denominator --check` goes FATAL, while the
referencing function's score does NOT move (the PDB oracle still names the reloc).
The 2026-08-13 reconciliation removed every oracle-covered pin (18 string sites, 41
FP slots) and kept 13: 8 ambiguous-payload strings (`" "` `"!"` `"1"` `"2"` `"C"`
`"D"` `"F"` `"\\"`) and 5 stranded FP slots (all in `grunt`, whose referrers are
not yet reloc-corroborated). When a referrer later corroborates, its pin may be
dropped — the build proves it (no partition diff). The negative control: unwrapping
`750.0`@0x001e9a40 left its slot with no manifest row and the partition gained an
8-byte eligible-unenrolled run — the FATAL gate caught it on the same build.

First proven claims (historical): the `".WWD"` disambiguation (0x20cfbc vs the
`"*.WWD"` tail at 0x20cf95 that inference then withheld — the oracle reaches it
today), a 2-TU `"Wormhole"` fold at 0x20a7ac, and grunthealthsprite's `0.2`/`0.5`
FP pool entries (0x1e9a98/0x1e9aa0) - the first FP data to pair at all.

### 3b-iv. `config/retail/data_compgen.tsv` — COMMON pins (wired)

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
extraction emits a Model row only when that unit's base obj has the symbol as a
COMMON of exactly the pinned size, so every emitting unit re-proves the pin (the
Model then resolves the copies to one binding per rva, and `candidates()` enrolls
it in the data manifest as ordinary `bss`). `gruntz delink` adds the authority
half: a `class=common` row with no emitting base obj is an error, and the owner is
the earliest-arriving module in link order among the objs that do emit it.

Coverage is the point. Nothing else in the pipeline can see this class: objdiff masks
relocations so an unnamed COMMON costs 0%, and it links cleanly (`gruntz link` resolves
each as `<common>`), so `link_defects` is silent too. `assert_relocs` was the only reporter
and it mis-read COMMON as an unresolved external — see its `defined_syms`, now fixed.

First (and currently only) claims: the three `GetRandomNumber` guard/seed pairs —
0x2c127d/0x2c1288 (the free function in `<Gruntz/GameRand.h>`), 0x2c278c/0x2c2798
(`CAniRecordView`), 0x2c279c/0x2c27a8 (`CFaderSine`). 26 `assert_relocs` FAKE → 0,
byte-neutral (3322/4290 exact, 89.08% fuzzy unchanged).

### 3c. `.bss` was capped by an objdiff INFERENCE artifact — FIXED in the CLI

**Historical statement of the problem** (kept for the mechanism): `.bss` was 212211 of
279630 `total_data` (~76%), and `ddsurface` alone 197144 of it — stuck at 99.998985% on
ONE symbol. COFF carries no symbol sizes, so objdiff infers `size = next symbol's
offset` (`obj/read.rs: infer_symbol_sizes`), and `diff_bss_symbol` scored 100 iff the
two sizes were equal. MSVC5's `.bss` hole-filling allocator always parks a 4-byte int in
the pad before the first 8-aligned array, so that int measured **8** on one side and
**4** on the other. Both sides were correct; only the measurement differed. Three
`cl /O2` probes prove the layout is **declaration-order invariant**, so it was never
steerable from `src/`.

**Resolution (2026-08-09).** `objdiff-cli` is now built from source with
`nix/patches/objdiff-bss-inferred-extent.patch`: a BSS symbol's size is compared only
when at least one side actually STATES one. It is not a relaxation of a real check — a
census of the whole tree (`gruntz verify data-coverage`) found 363 paired `.bss`
symbols with 50 extent disagreements and **every delta 3 or 4 bytes**, i.e. sub-alignment
padding, none of them a size.
Unpaired symbols are still a mismatch, and the extent audit that does bite is
`data_manifest.candidates()` (a reviewed extent must fit the span to its retail
neighbour, or BOTH rows are withheld). Measured on unchanged objs: `matched_data`
118484/704148 (16.83%) → **639859/704148 (90.87%)**, 19 more sections at 100.0, all
`.bss`; `matched_functions` 3498 and `fuzzy_match_percent` 91.99114 bit-identical.

Do NOT candidate-shape `.bss`: `.bss` has no bytes, so mirroring the candidate's offsets
makes every inferred size agree **by construction, for any set of globals** — a vacuous
100%. (Candidate-shaping is legitimate for `.data`/`.rdata` precisely because the
delinker still fills the container with retail bytes from each definition's proven rva,
so the byte comparison stays real.) Full mechanism + the rejected fabrications:
**`docs/patterns/bss-symbol-size-inference-hole.md`**.

⇒ The residual `.bss` gap is now a **naming** gap, not a measurement one: 16 sections
(10476 B) still hold target symbols with no base counterpart — chiefly the nine
`GruntDirectionCell` header statics, which the delinker enrolls as
`?s_gruntDirEast_22bd28@@3UGruntDirectionCell@@A` while cl names them
`_s_gruntDirEast$S17426`, and the normalizer canonicalises only the `$S` side. Same
shape for the `CButeMgr` function-local statics and their `??_B` guards. That is a
tooling gap in the pairing, not a pin gap.

### 3d. A legacy-form row is APPENDED to the object's first section — use `rva = -`

**The defect.** A data-manifest row with `section_ordinal = -` is not free-floating: the
delinker appends it, 8-aligned, to the object's first manifest section of the same
storage. `object_files.rs::with_sections` picks that section as
`rdata_section_id`/`data_section_id` and `add_data_definition`'s legacy arm calls
`append_section_data` on it. So every unplaced row grew a real COMDAT a phantom tail and
shifted every extent behind it. `interfaceobject`'s `.rdata` came out as ONE 0x2c section
(`??_7InterfaceObject@@6B@`, four bytes of pad, `??_7CObject@@6B@`) where cl emits two
separate 0x14 COMDATs.

**The delinker already models the fix.** A *section*-manifest row whose `rva` column is
`-` is **non-affine**: it keeps the candidate COFF shape (name, size, alignment, COMDAT
selection) but claims no retail range, so `compatible_folded_comdat_alias`'s overlap check
skips it, `add_data_definition` copies the definition's own retail payload into the
zero-filled buffer, and `definition_uses_affine_topology` routes it to
`add_legacy_data_relocations`, which relocates it from the definition's own rva. Nothing
is fabricated: the shape comes from the candidate COFF and the bytes from the proven
retail extent. Two families now use it (`data_manifest.section_rows` /
`ordinary_sections`):

1. **an unplaceable folded-COMDAT copy** — a TU that emits a class WITHOUT the `??_R4`
   COL word retail's surviving copy carries cannot claim `[rva, rva+size)`, because the
   with-COL emitters' placed section already claims `[rva-4, rva+size)` and two
   overlapping placements bail the delink. 28 such sections, 23 of them `??_7CObject@@6B@`.
2. **cl's ordinary, non-COMDAT `.data`/`.rdata`** — a unit's plain globals own no COMDAT,
   so all 149 of them were packed in manifest (rva) order into whatever section came
   first. Each object's ordinary section is now published as one non-affine section with
   every global at ITS candidate offset — admitted only when the section is provably
   COMPLETE: every member (`_Coff.section_members`, class STATIC **and** EXTERNAL, because
   cl's `$S<id>` function-local statics are class STATIC and are most of the bytes) has an
   enrolled row of matching storage, no two overlap, none overruns, and every uncovered
   byte is ZERO in the candidate payload. 42 of 78 candidate sections clear that bar.

| | before | after |
|---|---|---|
| `matched_data` | 639627/704140 = **90.84%** | 648397/704305 = **92.06%** |
| sections at exactly 100.0 | 358 | **383** |
| size-weighted | 99.6533% | 99.6813% |

25 sections closed, none regressed; `matched_functions` 3498, `matched_code` 474819 and
`fuzzy_match_percent` 92.14783 are bit-identical, and `data_relocs` stays WRONG 0 with 0
new orphans. The control also held: every section whose defect is a genuinely missing
symbol is unmoved (`brickzload` 0.00, `wwdfactoryobject` 0.00, `butemgr` 8.70,
`battlezmapconfig` 12.90, `worldsoundset` 33.33, `videoconfig` 40.00, `dialogs` 52.00,
`fadereffects` 66.67, `gruntzapp` 81.01, `netcmdslot` 97.93).

### 3d-i. Both legacy-row families closed (2026-08-09) — 92.00% → 97.04%

275 rows over 23 objects were still appending, each an ordinary section rejected for ONE
unenrolled member, in two families. Both are now addressed, from the same oracle.

**RETAIL'S `.reloc` DIRECTORY IS THE ADDRESS ORACLE, and it proves itself.** The PE lists
every site the linker wrote an absolute address into, so retail's DIR32 sites inside
`[fn_rva, fn_rva+size)` are known exactly — and so are our base obj's, from its COFF
relocations. Equal counts pair the two lists positionally, and then *every base symbol
whose rva we already know must equal what retail wrote at its partner site* (plus the
addend in our own instruction bytes); one disagreement discards the whole function. Only
then is a site whose symbol we cannot name read off, and the address is re-proven against
the shipped bytes. This is strictly stronger than content matching, which for FP pools is
not merely ambiguous but SELF-CONFIRMING: we would copy the retail bytes from the address
we found *by* those bytes, so a wrong constant in our source would still score 100.

* **`$T<id>` FP-pool constants** (`data_manifest.fp_pool_rows`). cl's floating-point pool
  has no source pin — `DATA()` needs a VarDecl and `DATA_COMPGEN` needs a value expression,
  and a `$T<n>` entry is neither. 44 of 66 members resolve from the reloc oracle with zero
  ambiguity. The other half was ALREADY PINNED and still did not count: 43 `DATA_COMPGEN`
  float claims state pool slots, but labels.py spells them `$T<rva>` while
  `ordinary_sections` keys off the COFF member name `$T<counter>`, so the pin and the
  member never met. Rows now carry `member` (cl's per-object spelling) and the manifest
  name is `$T<decimal rva>` for both channels — which coalesces them and lets N objects own
  one slot, as they must: our TU partition is a reconstruction, so `kitchenslime $T35488`
  and `pathhazard $T35508` are the same 0x1ea400. 14 more bridge on an exact extent+bytes
  match unique in both directions.
* **an exact `(rva, size)` ALIAS of a pooled `??_C@` literal.** `_s_strLBrack$S19420` and
  `??_C@_01KHLB@?$FL?$AA@` are both 2 bytes at `0x213efc`; /Gf (implied by /O2) put one
  TU's `static char s[] = "["` and the literal other TUs emit at one address. Both claims
  are true and the tell is that the extents are EXACTLY equal — a real contradiction only
  INTERSECTS. The pooled literal keeps the authoritative claim (it is what every
  referencing object's relocation resolves to, and it has the owners) and the named static
  is re-provenanced **`provisional-`**, which in the delinker means precisely "carve this
  definition, but do not own the address": `address_authoritative` gates both `proved_rvas`
  and `owner_and_addend_for_rva`, while `object_files::add_data_definition` carves it
  either way. No `provisional-` row can re-spell a relocation, so nothing is asserted
  twice — the objection that blocked this before.
* **A FOLD NEEDS BOTH SIDES IN ONE STORAGE, and 30 of the 44 fail that.** VC5 pools
  literals but does not make them read-only (no `/GF`), so a bare literal lands in a
  `.data` COMDAT — while a static our source declares `const` compiles into `.rdata`,
  which the linker CANNOT have folded onto it. Those pairs are mis-modelled declarations
  (retail's code used the literal directly and had no array), and enrolling them appended a
  phantom tail to the object's first `.data`: measured, `warlord` 100 → 83.07,
  `gruntsteps` 100 → 76.06, plus `triggermgr`, `gruntassetloaders`, `directsoundmgr`,
  `ddrawsubmgrleaf`. The candidate obj's own section name refutes the PIN rather than the
  literal, and the 30 are listed by name by `data_manifest --report` as a source worklist.
  (The reloc oracle independently confirms their ADDRESSES: retail's code really does load
  `0x20a5dc` where our `triggermgr` loads its `.rdata` `s_LightFx`. It is the `const`, not
  the address, that is wrong.)

The last real extent contradiction is gone too: `?g_idleGeom@@3PAUBzGeomPair@@A` at
`0x1e8fe4` was a PHANTOM. `BuildBootyGruntIdleAnimation` (0x1ce60) carries exactly two data
relocations, `0x1e8fec` and `0x1e900c` — a `g_bootyLetterCoords + 1` cursor and its end
after four pairs, the same walk `StepGlitterAnim` already spells over the same table.
Nothing starts at `0x1e8fe4`. `BzGeomPair` was a type invented to give the phantom a shape
and went with it.

`data_tu_order` also stops **inventing** a storage class: it defaulted an unenrolled rva to
`data`, which dragged `BootyStateActivate.cpp`'s `.data` band down over its `.rdata`
globals at `0x1e8fe4`, and a band stretched that far swallows enough foreign defs to be
classified a POOL — which EXEMPTS it. It now asks the PE. That deleted one reported
crossing outright (an artifact) and, with the folds now correctly COMDAT and out of the
linear band model, unmasked three real ones, recorded with their evidence in
`config/cleanliness/data-tu-order-baseline.tsv`.

| | before | after |
|---|---|---|
| `matched_data` | 649673/706177 = **92.00%** | 686767/707695 = **97.04%** |
| data sections at exactly 100.0 | 419 | **432** |
| ordinary sections published | 42 | **57** |
| legacy `.data`/`.rdata` append rows | 275 / 23 objects | **323 / 19 objects** |

(The append count RISES because enrolling a member does not by itself publish its section:
157 rows that used to be withheld entirely are now carried, and until the LAST member of a
section is proven the whole section stays legacy. Objects, not rows, is the number that
tracks progress here — and the sections that did complete went from 42 to 57.)

`butemgr .data` 99.92 → 100.00 closes the largest single data item (29,480 B);
`shadetablecache .rdata` 92.31, `pathhazard .rdata` 99.71, `sbi_wellgoo .rdata` 99.81,
`projectile .rdata` 99.83 and `bootystateactivate .rdata` 99.85 all reach 100, and
`warpstonefly`, `cursorsnapactreg`, `ddrawworkerregistry`, `wormholeacts` get a section for
the first time. NOTHING regressed: 0 sections dropped below 100.0, `matched_functions`
3498 and `matched_code` 474819 are bit-identical, `data_relocs` stays WRONG 0 with 0
orphans, and every control section whose defect is a genuinely missing symbol is unmoved
(`brickzload` 0.00 … `netcmdslot` 97.93).

**What is left.** 31 sub-100 data sections holding 21,808 B, most of it `.bss` (the §3c
NAMING gap, not an extent one). The `.data`/`.rdata` remainder is ~11 KB and splits into:
the 30 `const`-vs-`.data` declarations above; members with a source declaration but no
`DATA()` pin at all (`s_HELP` is the ONLY thing left between `mainmenubuilder .data` and
100); 7 `$T` entries with no relocation-paired referrer; and `?g_menuSparkleLo@@3HA` /
`?s_cheatWaWaWide@@3PAGA`, both source-modelling questions rather than manifest ones.

**Ordering + gate.** (a) → re-delink → gate `code exact >= 2385`; then (b) incrementally,
enrolling reviewed extents in batches and re-gating each time. Also available, already in
`/nix/store`: homm2's objdiff-cli 3.7.1 + `objdiff-data-symbol-details.patch` (per-symbol
`section` + `data_relocations` JSON rows) which unlocks the project-neutral
`strict_allocation_diff.py`; and `--reloc-alias-manifest` for array-index/negative-addend
spellings (pairs with `assert_relocs`).

What does NOT port: homm2's NB09/`sstModule`-sourced ordering, contribution ranges, and
`cv-public-data` inventory (no debug stream in GRUNTZ.EXE — use Ghidra + candidate `.map` +
`DATA()`); and homm2's VC4.0 LINK 3.00 `/Od` flags (Gruntz is VC5 `/O2` LINK 5.10).

### 3d-ii. `matched_data` 100.00% (2026-08-09) — the c2 alignment rule, the COMDAT append, and butemgr's `.bss` names

Three defects, all of them about the CONTAINER rather than the bytes. After them every one
of the 515 data sections is exactly 100.0.

**(1) The manifest synthesised an alignment cl never uses.** `data_manifest._alignment` was
"the largest power of two ≤ 8 dividing BOTH the retail rva and the size" — a rule with no
counterpart in the compiler. It hands a `char` guard byte alignment 1, a `short` 2, and a
12-byte struct 4. `docs/compiler-data-layout.md` reverse-engineered what c2 actually does
(probe-validated 41/41 on a blind TU) and the MSVC5 data-layout oracle's `obj_align` implements
it; the manifest now CALLS that function instead of guessing. Its inputs:

* **size** — already proven (`labels.sizeof_qualtype`);
* **object kind** — the declared type, from two oracles that are both the compiler's own
  statement: `build/gen/globals.json` (clang's printed qualType for the `DATA()` pin,
  written by the same labels merge edge that writes the Model) and, for anything
  it does not cover, the MSVC mangled type inside a `?`-decorated symbol name. cl's `$T`
  pool is float/double by construction, so its extent names its type.
* **the per-section ratchet** — **NOT RECOVERABLE**, see below.

The kind question collapses almost entirely into the size: `size < 4` is 4 for every kind,
`size > 8` is 8 for every kind (no i386 scalar is wider than 8), and with the ratchet at
its un-latched 4 the whole 4..8 band is 4 unless the object is a `double`/`__int64`. So the
only thing the oracles have to decide is *"is this an 8-byte wide scalar?"* — and three
rows tree-wide reach that question with neither oracle answering (`AFX_MSGMAP`,
`ButeIntPoint`, `CString`), all three aggregates, all three 4 either way.

**Why the ratchet cannot be recovered, and why 4 is the conservative branch.** c2 latches
the ratchet in cl's EMISSION order inside the ORIGINAL TU, and neither half of that is
available to a manifest. For `.bss` the emission order is c1xx's end-of-TU hash walk over
the ORIGINAL identifiers — our names are reconstructions, so the order is unknowable in
principle. For `.data`/`.rdata` the order IS declaration order = ascending retail rva, but a
legacy row is by definition one NOT placed in a candidate section, so the manifest sees a
SUBSET of the section's objects and cannot know whether one it cannot see latched first.
The ratchet only ever decides a 4..8-byte AGGREGATE, and 4 there never fabricates padding
the original may not have had — and every case the retail image can adjudicate agrees with
it (`?g_pathStr@@3VCString@@A` at `0x22c25c`, `_g_chatTextWidth` at `0x22b434` and eighteen
more sit at rva ≡ 4 mod 8, which an 8-aligned object cannot).

**Absolute RVA is not an object-alignment oracle.** c2 aligns a member inside its
object contribution; the linker then places the contribution. A divisibility mismatch
is therefore a review lead, not a proof. Applying the old absolute test to all current
source-backed pins rejects 131 established rows. The three rows that originally
motivated it now separate into one independently confirmed correction and two controls:

| rva | symbol | size | c2 says | image says |
|---|---|---|---|---|
| `0x253c9e` | `g_clut` | `0x30002` | 8 | **2** — confirmed independently: `u8 g_clut[0x30000]` starts at `0x253ca0`, ends exactly at `g_lut16`, and every use site carried the same compensating `+2`. Absolute alignment alone would not prove it. |
| `0x2bf28c` | `g_imageClip` | `0x10` | 8 | **4** — false positive. A VC5/MFC A/B instead identifies a plain `RECT`: `RECT = *RECT*` emits retail's direct four-word copy, while both CRect assignment forms call imported `CopyRect`. |
| `0x2c127d` | `??_B?1??GetRandomNumber@@YAHXZ@51` | 1 | 4 | **1** — expected: an inline function's local-static guard is a COFF COMMON placed by the linker. |

**(2) A legacy row was appended to a COMDAT** (`nix/patches/vostok-legacy-data-not-into-comdat.patch`).
`ObjectFile::with_sections` adopted the FIRST manifest section of each storage class as the
container for definitions the manifest does not place — and with the candidate section
manifest that is a per-symbol COMDAT. A COMDAT holds exactly the one symbol cl put in it,
so the appended definition and the alignment gap in front of it are content the base object
does not have. `fadereffects` is the clean demonstration: its sixteen ordinary `.rdata`
globals were appended to `??_7CFaderFlat@@6B@`'s `0x14`-byte COMDAT, taking it to `0x70`
and starting the run at a 4-aligned offset, which pushed every `double` off the 8-aligned
slot cl gave it. Only an ORDINARY manifest section may be the fallback; when there is none,
`data_section()`/`rdata_section()` already create a fresh one lazily — which is also where
cl puts a unit's non-COMDAT globals. (`.bss` was never affected: no `.bss` section is ever
declared in the section manifest, so its fallback was always a fresh section.)

The two defects interact, which is why they land together: with the correct alignment and
the wrong container, `gruntsteps .data` fell 100 → 98.18 (the `char[9]` `_s_ToyTiles`
correctly asked for 8 and got six pad bytes inside the `YOYOGRUNT` literal's COMDAT).

**(3) butemgr's `.bss` band was enrolled under names cl does not use.** The tree's last
non-exact `.bss` (88 B at 8.70%). `CButeMgr::GetRect`'s `static ButeIntRect s_default;` and
its guard are a NON-INLINE function's local statics, so cl emits them once, into butemgr's
own `.bss`, as private decorated symbols — `_?s_default@?1??GetRect@CButeMgr@@QAEPAUButeIntRect@@PBD0@Z@4U3@A$S20265`
(`docs/compiler-data-layout.md`, "Function-local statics: the six cases"). They need a
reviewed `config/retail/data_compgen.tsv (class=copy)` row only because a `DATA()` pin cannot spell a
name cl invents — but the rows named them `_s_default_rect_butemgr`, so the two sides never
paired. The rows now carry cl's spelling verbatim. BOTH `$S<n>` counters in such a name are
volatile — c2's `outdname` appends the trailing one, and c1xx spells the guard's own
unnamed object `?$S<n>@…` — so `canonicalize_data_symbols.STATIC_ORDINAL` masks *every*
`$S<n>` run rather than only the last, and the pair content-addresses on the enclosing
function. Proven collision-free: 13 symbols tree-wide carry more than one `$S<n>` and no
base obj holds two that mask together. `_s_default_string_butemgr` was also 8 B where
`s_empty` is a 4-byte `CString`; the extra 4 is the pad in front of the 8-aligned
`ButeDoubleVector` that follows it.

| | before | after |
|---|---|---|
| `matched_data` | 720539/720835 = **99.96%** | 723351/723351 = **100.00%** |
| size-weighted | `.bss` 99.99 · `.data` 100.00 · `.rdata` 100.00 | **100.00 / 100.00 / 100.00** |
| data sections at exactly 100.0 | 513 / 515 | **515 / 515** |

`matched_functions` 3498 and the whole per-function ledger are unmoved — nothing in `src/`
changed, so the base objs are bit-identical; `assert_relocs` and `data_relocs --gate` are
unchanged (0 defect rows, 0 orphans).

### 3e. Band-completion gap rows — a hole in a TU's band now COSTS (2026-08-10)

§3d-ii's 100.00% is over a denominator **we chose**: a datum `src/` never models — or
models too small, an `int` where retail has `int[10]` — is carved into neither object, so
it enters neither side of the pair and the section scores 100.0 around the hole
(`gruntz verify data-coverage`'s defect class, now inside the loop instead of beside it).

`data_manifest.gap_rows()` closes the loop from RETAIL's side: every byte run strictly
between two enrolled claims of **one** unit is carved into that unit's target object as a
`$gap_<rva>` row with **no base counterpart**, so the unit's data section drops below
100.0 until the datum is actually modelled. Evidence rules (fail-closed, sum asserted):

* **Ownership by contribution contiguity, single-owner witnesses only.** An object's data
  contribution is one contiguous block, so bytes between two claims of unit U belong to U —
  but a folded COMDAT (`??_C@`/`??_7`/`??_R*`) enrolls once per owning unit and makes
  "same unit on both sides" vacuous. Measured: without the single-owner filter the rule
  hands gruntvoice the 85 KB MFC/CRT RTTI band at `0x1f5584`. Frontier gaps (next claim
  is another unit, or unattributed library territory) are withheld: contiguity says
  nothing about them.
* **Only NONZERO payloads enroll** — cl's inter-symbol padding is zero, so a nonzero
  retail byte cannot be padding. All-zero gaps (missing zero-init datum vs pad,
  undecidable from the PE alone) are withheld, never carved.
* **`GAP_CAP` 0x100** — every library band misattributed by adjacency is bigger, every
  confirmed find smaller. Over-cap gaps are withheld BY NAME (`library_data.c`'s 13.5 KB
  MFC band at `0x1eb070` is the standing one — the library-data enrolment campaign).
* **`provisional-` provenance** — the delinker carves the bytes but the row never owns
  the address, so no relocation anywhere can be re-spelled through a gap name.

First run: **5 rows, 107 B**, and every one decodes to something real —
`0x20ceab` creditsstate 0x45 B holds the `??_R0` type descriptor of
`CArray<PLAYLISTINFOSTRUCT*>` (the `KNOWN_ORPHAN_UNITS` movieplayer class, §4);
`0x1f0868` fadereffects is a `float 2.0` FP-pool entry (one of the 7 `$T` slots with no
relocation-paired referrer, §3d-i); `0x213656` projectile is the pooled literal `"1"`;
`0x212748` play and `0x20d164` fortconquered are unmodelled initialized ints (32, 33).

A/B on one tree (gap rows off / on): `matched_functions` 3504, `fuzzy` 91.55447 and
`matched_code` 475479 **bit-identical**; `matched_data` 725371/725783 → 722207/725895 —
five units (`projectile` 52.6, `creditsstate` 68.8, `play` 28.4, `fortconquered` 0.0,
`fadereffects` 3.6) go sub-100 and are the worklist. Withheld census: 1506 all-zero,
1289 unowned, 827 outside initialized storage, 1 over cap. The drain move is always the
same: model the real datum in the owner TU; the gap row then dissolves on the next
manifest generation and the section returns to 100.0 with the byte in the denominator.

**Drain of the first five (same day).** Four dissolved, each by a different lever:

* Three were **short pooled literals that content-inference must withhold** (a 1–2 byte
  payload matches dozens of retail addresses): `" "` at `0x212754` (`t += " "` in
  `CPlay::DrawDebugStatsFull`), `"!"` at `0x20d168` (the `"… was conquered by …" + "!"`
  chat line in FortConquered.cpp), `"1"` at `0x213658` (`key + "1"` in
  `CProjectile::LoadProjectileSprites`). Each took a `DATA_COMPGEN` wrap at its use
  site — reading the retail bytes named the datum: `21 00` is `"!"`, not an int 33.
* The creditsstate `??_R0` was a **missing catalog bridge in `rtti_rows()`**:
  `vtable_rows()` bridges a template specialization (`?$CArray@PAU…`) through
  `data_vtables.tsv` because the registry key decodes its RTTI name into a symbol no
  object defines, but `rtti_rows()` never got the same bridge, so the class's ENTIRE
  RTTI graph (COL `0x1f4320` → R3 `0x1f4308` → R1 `0x1f42d8` → R0 `0x20ceb0`) went
  unwalked. Bridged (rva still has to be a registry-located base-0 vtable), the graph
  enrolls for its three emitters and the gap dissolves.
* `fadereffects`' `2.0f` at `0x1f0868` **stays open by design**: retail's `.reloc`
  proves NOTHING references it (a c1xx-allocated pool slot whose use folded), so any
  source spelling would be a guess. It is coupled to the sub-100 sine-fader bodies
  (`CFaderSine::RenderFrame` 79.4, `RenderWarpTile` 60.5) and should fall out of their
  reconstruction; the standing gap row keeps it visible.

Enrolling the walked `??_R1A@?0A@A@zErrHandling@@8` also exposed a KNOWN deferral:
`gametext`/`chatboxowner` (−24 B each) additionally emit `??_R4zPTree@@6BzErrHandling@@@`,
a secondary/MI RTTI record the enrolment machinery still withholds — the target-side R1
made the combined `.rdata$r` comparable and the deferred MI record now costs. That is
the MI-RTTI "later pass" surfacing, not a regression of these units' modeling.

## 4. The reloc-TARGET audit (`gruntz verify data-relocs`, gated at `--normal`)

Everything above measures whether the right BYTES are in the right place. This
measures whether the POINTERS in them point where retail's do, which no byte
comparison can: a relocated word's own bytes are a placeholder the linker
overwrites, so both sides hold the same placeholder. A vtable slot bound to the
wrong method, an RTTI base-class array pointing at the wrong `??_R1`, a pointer
table ordered differently — none of it moves a byte. It is the DATA analogue of
`assert_relocs`, and it is what the size-weighted numbers above can still be
hiding at 95–99%.

**Two oracles, resolving to ADDRESSES rather than comparing names.**

* `retail` — for a datum whose retail RVA is pinned, the retail image itself
  answers. Its `.reloc` table lists every HIGHLOW fixup, so the set of words
  retail relocates inside the datum's extent is a FACT and each stored value is
  the address retail points at. No symbol name enters into it. It also needs no
  delinked object, so it reaches data the delinker never carved.
* `paired` — for a datum both objects define, each side's referent resolves to an
  RVA (the Model, Ghidra's address-carrying auto-labels, the delinker's
  `const_<rva>`, plus `functions_static_libs.tsv` minus its `vtable-slot-oracle` rows,
  because a label derived from a vtable slot cannot adjudicate a vtable slot).

Addresses are the design, and NAMES were the first draft's mistake. Ported from
`global_refs`, the name comparison had to drop "a name only one side has ever
heard of" to survive the pooled-literal naming split (`??_C@_0BE@MAOF@…` against
`DAT_002126ec`) — and an injected wrong vtable slot is exactly that shape, so the
control walked straight through. It reported **0 rows over 9806 words while being
structurally blind**. Addresses have no such hole: two spellings of one address
agree, one spelling of two addresses does not, and the whole artefact catalogue
(pooled literals, twin FID labels, the delinker's unsized-datum fallback
`?g_gruntDirNorthEast + 0x2b0` / `_inflate_mask + 0x3db4`, `$S<hash>` suffixes)
is absorbed structurally rather than by a heuristic.

**Windowing is per SYMBOL, not per section.** That is the necessary deviation from
homm2's `coff_reloc_topology`, whose site key is `(section, offset)`: that works
there because its delink target is candidate-shaped, ours packs, and
`?g_projPhase0@@3NB` is `.rdata`+0 for cl and `.rdata$r`+0x28 for the delinker.
The first symbol in a section also owns the bytes BEFORE it — a `/GR` vtable
COMDAT opens with the `??_R4` COL pointer four bytes ahead of the `??_7`.

**Both sides go through `resolve_thunk`.** Retail was linked incrementally, so a
vtable slot holds the ILT `jmp` thunk's address, not the body's. Without it every
`/GR` vtable reads as wrong: 4725 rows, 3530 of them inside 100.00%-exact sections.

**Calibration and control.** `--calibrate` restricts to the data sections objdiff
scores at exactly 100.0; a row there is a detector bug. **0 of 8803 compared words
(2026-08-09).** objdiff does compare a data relocation's target name — redirecting
one `boomerang` vtable slot takes its `.rdata` 100.00 → 99.37 — so those sections
agree on relocations too and the calibration set is real. `--selftest` (which
`--gate` runs on every invocation, ~0.4 s) injects a redirected slot, a moved
addend and a deleted relocation record and requires WRONG, WRONG, MISSING back.

**Result and coverage, stated rather than hidden** (`--coverage`). 5870 words
checked against the retail image, 2933 against the delinked object, **zero
defects** — **8803 of the 10012 relocated words in `.rdata`/`.data`, 87.9%**. The
1209 not reached are 1078 whose referent resolves to no RVA (mostly NAFXCW bodies
with no FID label, and `??_R4` COL records) and 131 in a datum that is neither
pinned nor paired.

A further **4212 words are out of scope and reported separately**: `.xdata$x` (the
/GX EH state tables, 3509 DIR32s naming `$L` funclet labels) and `.CRT$XC*` (703,
the static-initializer pointer arrays). Both are compiler-generated metadata that
neither side pins and the delinker never carves; folding them into the denominator
would understate real `.rdata`/`.data` coverage by a third. The largest single
"unpaired data symbol" in the tree is one of them — `grunt`'s 0x388-byte
`.xdata$x` table with 113 relocations.

`--unpaired` lists the 4412 words inside the 2661 data symbols only one side
defines: mostly `$anon_data_<hash>` content-addressed private data whose two sides
hash differently (the hash covers recorded relocations the delinker does not
reproduce), plus 28 `??_7`/`??_R` records our objects emit that retail's delinked
side does not.

**The gate also fails on an orphan payload** — an enrolled datum carved into an
object `objdiff.json` never opens, so its bytes are withheld from every
measurement with nothing to report it. This is the ordinary-data twin of the
`data_vtables.tsv` row that named a dropped unit; `vtable_catalog.validate` checks
names and RVAs, not the unit column. Twenty-eight rows exist today, recorded with
their evidence in `KNOWN_ORPHAN_UNITS`:

| unit | rows | storage | why |
|---|---|---|---|
| `movieplayer` | 1 | `.rdata` | `data_vtables.tsv:88` puts `??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@6B@` (0x1e971c, 0x14, six relocated slots) on a unit `units.toml` does not declare. Three real units emit it — `arrayserialize`, `creditsstate`, `gruntzmgr` — so all three show it unpaired. Retail neighbours: `creditsstate` `.rdata` ends 0x1e9710, `grunt` starts 0x1e9738. Suggestive, not proof. |
| `ghidra` | 27 | `.bss` | `data_compgen.tsv (class=copy)`'s documented holding unit for the GruntDirStatics copies whose TU is not yet partitioned. No bytes exist, so nothing is withheld from scoring. |

The same family, third form: **a live unit for which the delinker produced no
object at all**. objdiff pairs nothing and scores the empty pairing **100.00% on
every measure with zero totals**, so the unit reports MATCHING in the per-unit
table while being entirely unscored. `logicdispatchinit` is the one today —
`src/Gruntz/LogicDispatchInit.cpp` is a single `.bss` template static
(`CActRegPool<CEyeCandyAni>::s_table` at 0x246060) and gets no delinked side,
though 72 other units do get `.bss` target sections. Recorded in
`KNOWN_UNPAIRED_UNITS`; a new one fails the gate.

**Spelling divergence in paired data is essentially nil: 9806 of 9812 words carry
the IDENTICAL symbol name on both sides.** So homm2's `canonicalize_relocs.py`
paired-target pass — which fixes the nearest-symbol-plus-addend spelling in the
pipeline instead of per-sieve — would recover ~0 data score here; it is a `.text`
concern (13.4% of `global_refs`' filtered rows), not a data one.

The six exceptions are one finding, and they name an unenrolled retail datum:
`?messageMap@CDialog@@1UAFX_MSGMAP@@B` is at **RVA 0x1eb068** (VA 0x005eb068).
Six dialog TUs (`battlezdlgcolors`, `checkpointdlg`, `customleveldlg`, `dialogs`,
`multihelpdlg`, `multistartdlg`) each store their message map's base pointer
there; the delinker has no name for it and spells all six
`??_7CGruntVoice@@6B@ + 0xfc`. The bytes at 0x1eb068 are `{0x005eb2e0,
0x005eb070}`, the `AFX_MSGMAP` `{pBaseMessageMap, lpEntries}` shape, which makes
**0x1eb2e0 `?messageMap@CWnd@@1UAFX_MSGMAP@@B`**. Not landed here:
`functions_static_libs.tsv` is a FUNCTION carve-out list feeding the executable map, so a
data row belongs in the data pin channel, not in it.

## The access map (`gruntz verify data-access`)

The mis-typed-globals audit: `gruntz verify data-access` decodes EVERY
`.text` reloc site into `.rdata`/`.data` (objdump over the whole section, one
pass) into an access event — width, direct/indexed/imm/lea/indcall mode,
read/write, movzx/movsx signedness, FPU f32/f64 witness — plus every data-side
reloc cell (fn-ptr vs data-ptr content). Attribution charges a symbol only up
to its DECLARED size (reviewed the Model, else `globals.json` sizeof);
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
`config/retail/data_compgen.tsv (class=copy)` folded in at merge_labels), 45 FP pool
constants as `DATA_COMPGEN` at their use sites, the CButeMgr getter-static
band, message maps for CBattlezDlgColors/CMultiStartDlg (11 handler stubs
claimed from pfn evidence), the DirectInput GUID triple, and ~40 per-slot
zero-init globals in owner TUs. The 31 residual runs are ALL accessed only
by unreconstructed (<gap>) code - their owner TUs are unprovable until the
accessor fns are matched, so they stay on the queue rather than taking
fabricated homes (`gruntz verify data-access --unclaimed`).
Library runs (CRT/MFC/zlib data) are policy-excluded like library code;
string-pool runs are the unpinnable pooled literals.

### The per-ACCESS map (2026-08-09, lane/data-access-map)

`gruntz verify data-access` above is a per-SYMBOL aggregate recomputed on every
invocation. Its per-access evidence — which byte range each instruction covers,
how wide, in which direction, through which addressing form — is now persisted
and queryable as `gruntz verify data-access`
(**`docs/data-access-map.md`**): a sqlite index plus a grep-able TSV, with
`--at` / `--range` / `--symbol` / `--fn` / `--sql`, an offset-resolved field map
per claim, five derived categories, a control-set calibration, and an
injected-defect self-test. The two share the same `.reloc` oracle and the same
premise: **we choose the extent of every claim, so a too-small claim always
scores 100**; only retail's own access widths can contradict a declared type.
It also emits `build/gen/data_touched_ranges.tsv`, the set of bytes retail
actually touches, for intersection with the from-our-side completeness analysis
(uncovered AND touched = unmodelled data; uncovered AND untouched = padding).

### Working the joined worklist (2026-08-09, lane/unmodelled-data)

The join was run. **`unclaimed` fell from 4,229 accessed bytes over 69 runs to
1,221 over 55**, and the shape of what is left is the finding: the sieve's
worklist was never 69 unrelated unmodelled objects.

**47 of the 69 runs were GruntDirStatics banks with no tsv row.** Each cell's
identity comes from its own `$E` initializer - the `(row, column, direction)`
triple it stores - so it is read, not guessed. Decoding all 107 initializers in
the image gives the same answer every time: **address order == declaration
order** (East, North, South, West, NE, NW, SE, Center, SW), including for the
banks the linker split around another static. Three banks are separately claimed
as real globals (`?g_gruntDir*`, `?g_gruntMoveDir*`, `_g_dirVec`) and all three
agree cell-for-cell, which is the cross-check. 200 rows landed; the addresses are
`.bss`, so this buys attribution and closes the findings, not bytes.

**Two of those banks are blocked, each on a claim that contradicts the binary:**

* `0x244970` is claimed by **`_g_dirVec`, an `i32[9][4]` in `GruntCombat.cpp`**
  that models nine cells as one array. Retail never indexes it - all 45 reads
  and 27 writes are `direct` absolute addresses, no `indexed` form at any scale -
  so the storage is nine separately-addressed cells and the array (with its
  `AT()` offset macro at the use sites) is a fabricated aggregate.
* `0x249818`'s NE..SW half lands at `0x24a458`, **inside `?g_netCmdSendMsg`'s
  declared `0x850`**. `g_netCmdSendMsg` is at `0x24a058`, is touched only at
  `+0..+0xe`, and retail constructs direction cells at `+0x400`; the next real
  object after those cells is `g_netGruntRecMsg` at `+0x850`. So the struct is
  `0x400`, not `0x850`, and `m_payload[0x842]` was sized off the distance to the
  next claim. Correcting it exposes a second unmodelled `0x400` buffer at
  `0x24a4a8` that nothing references.

**Seven further banks need the rva-suffixed spelling** because their owner unit
already holds a same-named plain row - two retail TUs merged into one of ours.
That spelling enrols a target-side symbol whose base-side twin is spelled plainly,
so it never pairs: the 74 rows that already use it are **896 of the 995 unmatched
`.bss` bytes in the whole tree**. They are TU-partition work, not naming, and were
left open rather than paid for.

**The residual 1,221 B is dominated by `??_B` guard bytes** - the
`mov cl,[g]; mov al,1; test al,cl; jne; or cl,al; mov [g],cl` prologue of a
function-local `static`. `config/retail/data_compgen.tsv` is their
natural home but ratchets COFF **COMMON**s, and these are ordinary per-TU `.bss`
from main-file statics whose functions are not reconstructed, so there is no
base-side symbol to pair.

**Sieve B's uncovered-nonzero bytes are 48,567, and >99% of them are library.**
`0x1ef814` (580 B) and `0x1f0530` (536 B) are `dxguid.lib` GUID tables, `0x1ee8ee`
(3,386 B) is CRT locale/math name strings, `0x1f5584` (85 KB) and `0x1eb070`
(13 KB) are RTTI and MFC message-map records, and every remaining `.data` run
under `0x229400` sits between two `??_C@` literals - the unpinnable pooled-string
class. **Exactly one game-owned initialized run survived that filter**, and it is
now claimed: `?g_table_20fa78@@3PAHA` (`GruntzMgr.cpp`, 64 B, +64 B compared and
matching). The reloc table holds no entry anywhere inside it, so its "no reader"
is proven, not merely unfound; the sieve's `unaccessed` category now reports it
back, which is the intended behaviour and not a defect.

**Two `stride/high` findings are false positives of one class**: a flat
`i32[2N]` written `tbl[i * 2]` / `tbl[i * 2 + 1]` lowers to `[reg*8 + base]`, so
the recorded scale is 8 while the element is 4. `_g_bootyLetterCoords` (0x1e8fe8)
and `?g_levelMsgIconPos@@3PAHA` (0x20b8b8) are both that, and the flat spelling is
load-bearing for the first: retail's walk relocates `base+4` and `base+0x24`, i.e.
it starts at the second `int`, which a `Coord*` loop cannot produce. Retyping
either to a pair struct would be aggregation without a stride witness.

## 5. The coverage partition covers `.bss` (2026-08-10, data-coverage-close)

the data census partitions the loader zero-fill region with the
same fail-closed machinery as initialized data, under `.bss`-specific oracles —
payload proves nothing in zero fill, so a verdict rests on:

* **who references the run** — every absolute-address operand is a `.reloc`
  HIGHLOW site; the containing function's game/library classification
  (`functions_static_libs.tsv`) is the attribution. Referenced only by proven library
  code ⇒ `library-private data`, excluded.
* **library pointer propagation** — a pointer WRITTEN INSIDE a node already
  proven library-private claims the `.bss` it targets (single-hop by
  construction: `.bss` holds no bytes, so no edge can originate there). The CRT
  stdio buffer family (4 KB at `0x2c15e0`, reached only through the `__iob`
  table run at `0x218c88`) is excluded on this evidence. Game visibility always
  wins; propagation never enters a visible node.
* **alignment slack** — a hole strictly smaller than the delink manifest's
  stated alignment (`data_layout.obj_align`, c2's own rule) for the claim
  starting at the hole's end exists BECAUSE of that alignment.
* everything else is `UNCLASSIFIED (.bss)` and stays ELIGIBLE — fail-closed,
  uncertainty can never improve the score.

Two initialized-side refinements landed with it:

* **a `.data` run containing reloc sites cannot be a pooled literal** (no
  FP/string pool carries a relocation). 11.0 KB re-proven library-private,
  3.4 KB pulled back INTO the eligible denominator — the literal blanket had
  been over-excluding. Initialized reconstructable 98.00 → 95.15 (honest drop).
* **typedef-for-linkage records reach `structs.json`**: clang's record-layout
  dump spells `typedef struct {...} NAME` as `(unnamed at file:line:col)`, so
  such records never got sizes and their globals were withheld sizeless.
  `parse_record_layouts` recovers the linkage name from the source at that
  location (only the exact typedef shape; else skipped as before). +58 records;
  the three `vendor/sfman-1.01` globals enrolled on the spot, with retail's own
  `mov WORD [g_sfCaps], 0x66` confirming the `#pragma pack(2)` layout.

The `.cpp`-local static-init guard family (`_?$S<n>@?N??<fn>@4EA$S<id>`, a named
STATIC cl 5.0 emits with no source spelling) was censused across every base obj:
11 guards, 9 already enrolled, the two `CPlay` ones pinned via
`config/retail/data_compgen.tsv (class=copy)` (the butemgr-band precedent).

Scoreboard after: initialized 95.15% / `.bss` 91.72% reconstructable, fidelity
100.00 both. The eligible-unenrolled remainder (the worklist, largest first):
`dircellmethods` editor-buffer band (13.1 KB + 3.4 KB), `gameinfostring`
(7.8 KB), `multi` (4.9 KB after `g_chatPacket` + 2×1,012 B after the
`g_chanStat42x` packets — retail pushes `sizeof=0xc`, so those are NOT struct
tails but unreferenced siblings), `netcmdslot` pools (2.5 KB + 1.1 KB),
NetLobby (1.3 KB + 2.3 KB), the `CActRegPool<T>::s_table` inter-band runs
(~2 KB, initialized by un-inventoried `$E` funclets), and the 46
`unclaimed/high` access-map findings (each: bytes accessed by name-attributed
game code past a claim's end — per-item modeling with `sema disasm`).
`?TickKillCues@CDDrawChildGroup` constructs two static OBJECTS at
`0x2bf390`/`0x2bf3a8` (ctor at VA `0x5b55e9`-ish thunk) that src currently
models as loose `g_val_*` ints — a shape defect, not a coverage one.

### 5a. Coverage campaign results (2026-08-10, data-coverage-close)

Scoreboard: **initialized 99.59%** (.rdata 99.24 / .data 99.70) — **.bss 92.32%**
reconstructable, fidelity 100.00 everywhere, all gates green, no function
regressions (one −0.02 current-% dip blessed, MAX held).

**Closed** (the drain, in landing order): the .bss partition + library-pointer
propagation + alignment slack; reloc-bearing runs un-blanketed from the literal
exclusion; typedef-linkage records into structs.json (SFMAN globals enrolled);
the two CPlay guard pins; the `.CRT$XCU` walk (retired) — the XCU walk — plus the
ctor-immediate decode oracle that named every GruntDirectionCell copy
(22 TUs gained `<Gruntz/GruntDirStatics.h>` retail provably compiled, ~200 cells
enrolled/renamed, five direction-misnamed aliases corrected); the library-gap
rule (MFC funclet blobs → the static CWnd/CMemoryException objects excluded);
RTTI/EH records recognised by CONTENT (TypeDescriptor vptr anchor + ≤4-cell
chains — .data ??_R0s and .xdata$x throw records, initialized UNCLASSIFIED
4,137 → 21 B); the CDialog/CWnd/CCmdTarget message-map chain and six
game-named SDK GUIDs enrolled via `data_static_libs.tsv` → `library_data`.

**Parked, with reasons** (the residue and why static evidence is exhausted):

* `.bss` UNCLASSIFIED ~46.8 KB — zero-init buffers/dead statics NOTHING
  references: the multi bands (4.9 KB after g_chatPacket; 2×1,012 B after the
  g_chanStat42x packets — retail pushes sizeof=0xc, so NOT struct tails),
  netcmdslot pools, gameinfostring 7.8 KB, the dircellmethods editor band
  (13.1 KB + 3.4 KB). No reloc reaches them, no funclet constructs them, no
  imm addresses them. Next evidence source: GruntDem.exe (the demo sibling may
  reference the same statics from live code).
* Cell sets whose owner is a spanless comdat TU (0x229318: gamewnd/gameapp
  ambiguous), bootystateactivate's partial set (4 cells hide under other
  claims — remodel, not rows), the 0x245278 five (the 0x2452xx region's
  existing attributions are suspect), 0x24bfe0 four (play vs droppedobject),
  and the 0x2293f8 East cell (straddles the .data raw-size boundary; the
  delinker's storage rule correctly refuses it).
* Initialized ~460 B: the CRuntimeClass/vtable triplets at 0x1eafc0 (need
  per-record literal names from NAFXCW), the game-thrown EH island
  0x1e949c/0x1f40d0/0x20c6b8 (zPtrColl-family ThrowInfo/TypeDescriptors —
  the .xdata$x pairing campaign's shape), four unpinned pooled literals
  ("Software", "GAME_BADSELECT", the CRC push-constant, z_errmsg tail), and
  0x20b978 (a one-past-end LIMIT address, never dereferenced).
* `?TickKillCues@CDDrawChildGroup` constructs TWO `static CObArray` objects
  (ctor `??0CObArray@@QAE@XZ` proven) at 0x2bf390/0x2bf3a8 with guard byte
  0x2bf388; src models them as loose `g_val_*` ints. A precise matcher task -
  identity is solved.

Retail's GruntDirStatics declaration ORDER differs from ours (constant
9-cell permutation, proven by the store-order rotation) — reordering the
header would align the XCU walk's name pairing but shifts every `$S` counter
in every including TU (sidecar churn); deliberately not done.

### 5b. Band carving + the demo verdict (2026-08-10, late)

**The whole band surface is carved** (`gap_rows` extended): all-zero
initialized gaps at/above the next claim's stated alignment and `.bss` gaps
between single-owner witnesses now become `provisional-band-gap-*` rows -
668 rows / 24,267 B carved with no base counterpart. Coverage is untouched
(the rows are excluded from `enrolled_runs` - pressure, not credit); FIDELITY
now includes the missing-data surface: initialized 98.16, `.bss` 96.67, with
115 units sub-100. That per-unit list is the gated modelling worklist; nothing
can hide behind a 100.0 again.

**Demo oracle verdict on the dead bands.** GruntDem.exe (same build session,
different link) was probed via masked anchor search: all four anchors
(ButeAttributezDlgProc, BroadcastChatLine, the netcmdslot pool init,
FormatGameInfoString) resolved uniquely with ONE consistent data shift
(-0x51f8), and the four big dead bands (dircellmethods 13.1 KB, multi
4.9 KB, netcmdslot 2.5 KB, gameinfostring 7.8 KB) have **zero inbound
references in the demo too**. They are dead in both links: debug/editor-era
statics whose names exist only in the original source. Modelling them by
name would be fabrication; their proven facts (extent, owner band, zero-fill)
are carried by the gap rows. For the eventual bit-identical LINK, only
correctly-sized allocations are needed - a link-phase decision.

**g_emptyString was a fiction** (dissolved): vendored zutil.c's z_errmsg
points at 0x2293f4, so that address is the pooled `??_C@_00A@?$AA@` fold
survivor (132 refs image-wide); the named global src invented for it cited
itself. The data-unprovable-tail rule now consults the claiming unit's own
base obj to resolve the FileAlignment-tail ambiguity (cl put the symbol in
.bss there, so retail's cl did too).

### 5c. TickKillCues fixed; the static-object detector (2026-08-10)

**`?TickKillCues@CDDrawChildGroup` is FIXED, not specified.** src carried five
dead `g_val_<hex>` placeholders (zero uses) whose addresses are the innards of
the two function-local `static CObArray` the function already declared:
0x2bf394/0x2bf3ac are each array's `m_pData`, 0x2bf398/0x2bf3b0 its `m_nSize`,
0x2bf388 the shared init guard. Retail proves the shape (`mov ecx,<addr>;
call ??0CObArray@@QAE@XZ` at both sites) and the extents agree on both sides:
cl allocates 0x18 per `CObArray` in `.bss` (sizeof 0x14 rounded to 8), exactly
retail's 0x2bf390 -> 0x2bf3a8 spacing. Placeholders deleted, `DATA()` pins on
the statics, guard pinned under cl's verbatim `_?$S28@...` spelling.
**TickKillCues 100.0; wwdobjmgr `.bss` 38.6 -> 100.0** - the band-gap rows that
had been carving the unmodelled interior are gone, which is the surface doing
exactly its job.

**The generalizable detector** (worth re-running after any modelling wave):
scan `.text` for `mov ecx,<static data addr>` followed within ~24 B by a
`call ??0<Class>`, then compare each construction site against the manifest.
56 static constructions found; the informative outcomes are

* **UNCLAIMED at an object start** -> a real missing pin. Three `static CString`
  objects surfaced this way (s_custom 0x229e44, buf 0x22af0c, s_alert
  0x2446fc), each previously a parked "unclaimed/high 4 B" access-map row.
* **interior of an existing claim** -> benign when the claim is an ARRAY
  (`g_levelMsgStrings`/`g_areaNames`/`g_gruntNames` construct elements at
  interior offsets); a defect only when the claim is a scalar, which is how
  TickKillCues read.
* library-owned constructions (static `CWnd` family, `AFX_CLASSINIT`) stay
  unclaimed by design - the library-gap rule excludes them.

**Tool defect it exposed:** the obj-matching predecessor of today's rewrite
wildcarded only DECIMAL scope ordinals, but MSVC spells an ordinal >10 as hex
digits A..P terminated by `@` (`?BD@??Fn@@...` = 0x13 = 19). Every
function-local static nested deep in a function silently MISSed and stayed
unnamed. The scope ordinal is now MASKED on both sides
(`core.msvc_names.LOCAL_STATIC_SCOPE`, canonical `?1`), so its spelling can
no longer decide whether a claim binds.

**Not a defect:** the 20 dead `g_val_<hex>` placeholders elsewhere in src are
all real retail-referenced data (5-15 access sites each, mostly Win32 handles
around IAT calls) - naming debt for the rename-last phase, not mis-models.
