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

## 3. Staged: per-symbol data delink + full data loop

The needle-mover for `matched_data` is a delinker that emits **each DATA() global as its own
named COFF symbol** with real storage class + interior relocs→externals, so objdiff pairs
data by name. Enablers are already present:

- **Delinker**: homm2's data-topology `vostok-delinker` (`--data-manifest`,
  `--data-section-manifest`, `--contribution-manifest`, `--reloc-alias-manifest`) is a
  superset of Gruntz's current CLI and is **already built in `/nix/store`**. Bump
  `flake.nix`'s `vostok-delinker-src` to that revision. It changes the delinker's default
  symbol identities across all target objs, so it is a validated migration: re-delink the
  whole tree, then gate on exact-match ≥ 2366 before adopting.
- **DATA() sizes**: extend `labels.py` to emit each `DATA()` global's `sizeof` (homm2
  `annotated_data.py` VarDecl inventory; Gruntz already has clang record layouts in
  `structs.json`). Feed `reviewed_data.py`-style extents into the `--data-manifest`.
  Substitute Gruntz's candidate `.map` (`link_order.py`) for homm2's NB09 as the
  contribution-range source (GRUNTZ.EXE has no debug stream).
- **Patched objdiff**: homm2's objdiff-cli 3.7.1 + `objdiff-data-symbol-details.patch`
  (adds per-`DiffSymbol` `section` + `data_relocations` JSON rows) is **already in
  `/nix/store`**; it unlocks the project-neutral `strict_allocation_diff.py` for
  byte+reloc-exact per-allocation audits.
- **Reloc canonicalization**: adopt a `delink_reloc_aliases.tsv` + `--reloc-alias-manifest`
  for array-index/negative-addend spellings (pairs with the `assert_relocs` work).

What does NOT port: homm2's NB09/`sstModule`-sourced ordering, contribution ranges, and
`cv-public-data` inventory (no debug stream in GRUNTZ.EXE — use Ghidra + candidate `.map` +
`DATA()`); and homm2's VC4.0 LINK 3.00 `/Od` flags (Gruntz is VC5 `/O2` LINK 5.10).
