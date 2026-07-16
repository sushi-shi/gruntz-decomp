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

`scripts/gruntz/analysis/data_audit.py` ports homm2's pure-PE evidence core
(`link_exe.py: read_pe / classify_pe_storage / read_pe_payload_evidence`). Reading ONLY the
retail `GRUNTZ.EXE` (no delinker/PDB/wine), for every `kind=data` symbol in
`symbol_names.csv` it:

- classifies PE storage (`.rdata` ro / `.data` initialized / `.data` loader-zero tail /
  other / outside);
- resolves an EXTENT: reviewed `symbol_names.csv` size, else the next-data-symbol gap
  (flagged `next-symbol-gap`);
- reads that span, zeroes every HIGHLOW base-relocation field, and records
  `sha256`, `normalized_sha256`, and the HIGHLOW offset set — a base-independent
  content+relocation fingerprint.

Output `build/gen/data_attribution.tsv` is the reviewable attribution ledger; `--json`
emits full per-symbol evidence, `--rva 0x..` probes one symbol. Current census: 925 data
symbols (rdata=306, data-init=115, bss-tail=409, other=95); 417 rdata/data spans
fingerprinted. **916/925 extents come from the next-symbol gap** — the "DATA() has no size"
gap (§3). Identical vtables surface immediately: `CActionArea`/`CUserLogic`/`CGuardPoint`
share `normalized_sha256 10eef285…`.

This is the retail oracle a real data-byte loop gates against: once source data
initializers relink (or the delinked target carries typed data), compare candidate bytes
(relocs normalized) to these digests.

## 2b. Link-side DATA static-storage audit — `gruntz exe-diff` §E (wired)

`gruntz link` (Phase 2, VC5 link.exe 5.10.7303, `/FORCE`) links the base objs into a
candidate `GRUNTZ.EXE` + `.map`. `gruntz exe-diff` already audits `.text` layout/bytes; it
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

Current: `matched_data` = **4 / 69184 bytes (0.006%)** vs homm2's **305328/305328 = 100%**.

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

  **Why the -3, and why it is not wired in yet:** a data manifest is the topology
  AUTHORITY for the objects it names — data it does not enroll stops being
  materialized into those target objects. Enrolling only the `DATA()` globals drops
  each unit's compiler-emitted data (string literals `??_C@…`, the 393 unsized
  globals, `$T` pools), and three functions reference exactly that: `soundfontpath
  BuildSoundFontPath` and `gametext _$E1`/`_$E4` lose their `??_C@` strings. homm2
  covers this with **5216** supplemental rows derived from the candidate COFFs.
  Completing per-object coverage is the remaining work; until then `delink.py` does
  not pass `--data-manifest` and the code gate holds at its **2385** floor.

- **Bonus: the sizeof extents are a contradiction check.** A reviewed extent must fit
  the span to its neighbour. Six overlaps fell out, each proving one of a pair is
  mis-modelled (neither is enrolled — we do not know which is wrong):
  `g_singleCmdList`/`g_multiCmdList`/`g_pool` (`CPtrList` 0x1c swallows the adjacent
  `…Count` 0xc in), `g_smallFont` (`Font` 0x18 swallows `g_loadedFlag`), `g_panTable`
  (mangles `PAHA` = `int*` = 4 but the declared type sized 0x20), `g_imageCache`
  (`CPtrArray` 0x14 swallows `g_imageCacheIndex`). A real defect worklist:
  `python -m gruntz.build.data_manifest --report`.

**Contribution ranges are BLOCKED (measured).** Placing definitions at candidate
`section_offset`s, and attributing the withheld COMDAT strings / `$T` pools, both need
per-compiland contribution intervals. GRUNTZ.EXE has no NB09, so they must come from our
TU partition — which does not hold: only **8 of 86** per-(unit,storage) bands overlap no
other band (rdata 2/15, data 5/17, bss 1/54; aggregates excluded), and **0 of 296**
ambiguous COMDAT copies are attributable by band containment. Measure overlap
**all-pairs**, never adjacent-only — that error reports ~70–80% clean where the truth is
~9%. See **`docs/tu-partition-brief.md`**.

**Ordering + gate.** (a) → re-delink → gate `code exact >= 2385`; then (b) incrementally,
enrolling reviewed extents in batches and re-gating each time. Also available, already in
`/nix/store`: homm2's objdiff-cli 3.7.1 + `objdiff-data-symbol-details.patch` (per-symbol
`section` + `data_relocations` JSON rows) which unlocks the project-neutral
`strict_allocation_diff.py`; and `--reloc-alias-manifest` for array-index/negative-addend
spellings (pairs with `assert_relocs`).

What does NOT port: homm2's NB09/`sstModule`-sourced ordering, contribution ranges, and
`cv-public-data` inventory (no debug stream in GRUNTZ.EXE — use Ghidra + candidate `.map` +
`DATA()`); and homm2's VC4.0 LINK 3.00 `/Od` flags (Gruntz is VC5 `/O2` LINK 5.10).
