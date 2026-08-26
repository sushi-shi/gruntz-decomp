# Gruntz 1.01 target migration

This branch targets the complete English 1.01 `Gruntz.exe`. GooRoo's
`Gruntz101.exe` download is an ordinary ZIP archive with a misleading suffix;
it is not the older RTPatch executable and it contains the full replacement
game executable. Therefore 1.01 is a valid primary matching target.

## Pinned inputs

| artifact | bytes | SHA-256 |
|---|---:|---|
| GooRoo `Gruntz101.exe` archive | 1,017,533 | `36cf4cc8433d41d42a4450fe781cf40f972991c4f91caadaa44de8dfd924e39c` |
| extracted 1.01 `Gruntz.exe` | 2,512,896 | `ef636e84cd547efe3e835811aefa6cd20964dadb9c2b427aa13860e52b2228d4` |
| extracted 1.01 `GRUNTZ.ZZZ` | 95,433 | `da53080e4b887a4bc375d2c9345c074948e6fc7ac3b11f3d030e279f1ee16f37` |
| extracted `Info_101.txt` | 1,016 | `4a3139d9a1cf8917cf81b5005aff80e423e984cd15c232760da5ee4bdaa84a83` |
| Lady of the Cake `Grnt_101.zip` | 304,098 | `f387d14a6cb6359b01e7333410699c98637e546e8dec12debf66f77d296e70b1` |
| extracted RTPatch `Grnt_101.exe` | 395,254 | `b7787b81dec4e42f8a303de45077c207df1df262d7a8d3701254d25f058b3f3a` |
| former 1.00 target `GRUNTZ.EXE` | 2,511,872 | `7073c2536106ae4cca32e3e82db21001f319678b214c4eae2c689c54902808b3` |

The archive's MD5 is `16c50598279023463d3eb2935c7f4ba9`, independently
listed by the ModDB mirror. `flake.nix` pins the archive hash, extracts only
`Gruntz.exe`, then independently checks the executable size and SHA-256. The
shell exports:

- `GRUNTZ_EXE`: active 1.01 executable;
- `GRUNTZ_EXE_V101`: named 1.01 control;
- `GRUNTZ_EXE_V100`: former target, retained only for migration evidence;
- `GRUNTZ_UPDATE_V101`: the complete GooRoo full-file update archive;
- `GRUNTZ_RTPATCH_V101`: historical Lady of the Cake patcher ZIP, retained
  only for static study and preservation.

The later Lady of the Cake `Grnt_101.exe` is a 395,254-byte RTPatch
Professional 4.11 GUI patcher. It may reveal patch-tool provenance, but it is
not a target because it contains a binary delta rather than a complete linked
image. No UI or patch application is part of this workflow.

### What that says about the original patch build

Yes—with “1.01” in place of “2.1”—the normal RTPatch workflow is the one you
described. Monolith first needed a complete revised installation/file set.
RTPatch's developer-side **Build** component compared the old system with that
revised system and emitted a compact byte-level difference; the distributed
GUI executable contains the **Apply** component plus that payload. Pocket
Soft's own [technology overview](https://www.pocketsoft.com/rpatch_technology.pdf)
describes those separate Build and Apply components.

That is a tool-workflow inference, not proof of Monolith's exact internal build
script. Static inspection proves that `Grnt_101.exe` is the RTPatch 4.11 apply
artifact. The GooRoo file appears to be a later full-file repack: its included
`Info_101.txt` calls the original patch “self-installing,” while the outer
`Gruntz101.exe` is actually a ZIP. For matching, this distinction is harmless:
the complete post-update executable is the target, whereas the apply stub is
only historical evidence about how that output was distributed.

## Image movement

The two files have the same Rich header/toolchain signature. The important PE
movement is:

| region | 1.00 | 1.01 |
|---|---:|---:|
| entry RVA | `0x120950` | `0x120b60` |
| `.text` virtual size | `0x1e526b` | `0x1e55c1` |
| `.rdata` RVA | `0x1e7000` | `0x1e7000` |
| `.data` RVA | `0x208000` | `0x209000` |
| `.idata` RVA | `0x2c3000` | `0x2c4000` |
| `.rsrc` RVA | `0x2c7000` | `0x2c8000` |
| `.reloc` RVA | `0x2e6000` | `0x2e7000` |

This is not one uniform RVA translation. Link contributions moved in several
bands and some bodies changed, so adding a global delta would corrupt claims.

## Completed migration

The active source tree and configuration now describe 1.01 throughout:

- all 5,793 `RVA*` / `DATA*` source and header annotation occurrences map to
  their surviving 1.01 identities;
- `functions.tsv` contains 11,269 admitted 1.01 starts and `data.tsv` contains
  11,420 admitted 1.01 starts;
- every address-bearing retail table, the 131 wall-review keys and the active
  match baseline use 1.01 RVAs;
- the address-bearing portions of 923 per-TU header-static names, 14 CRT data
  slot names and 104 synthetic library function names were regenerated for
  their 1.01 positions rather than retaining misleading 1.00 spellings;
- the 1.00 MAX/historical score ledger was retired. The new ledger starts from
  the verified 1.01 comparison while preserving identity-surviving attempt
  counts; MAX and historical fuzzy scores are target-specific.

The migration initially exposed four target-only starts. The semantic version
audit subsequently reconstructed the new `CRezArchive` banner setter at
`0x13c4d0` and corrected the old debug-page wrapper's claim to `0x115d70`.
The remaining target-only starts are the distinct surface-text helper at
`0x115c90` and the CString-induced EH funclets at `0x1d9c90` and `0x1d9cb0`.
Seven removed string/padding identities were dropped rather than guessed onto
unrelated data. The linked MFC
`CCmdTarget` vtable at `0x1eb7d4` is genuinely `0x40` bytes in 1.01, so the
following `CCmdUI` vtable begins at `0x1eb814`.

The final 2026-08-26 full build delinked all 293 target objects and passed the
MAX, fast and normal verification tiers. Its initial 1.01 matching baseline is
4,427 scored functions, 3,749 exact and 95.43% overall fuzzy. That percentage
is the remaining source-reconstruction objective, not an address-migration
failure.

## Cross-version score audit

The two target-specific match ledgers can still be joined by unit and mangled
function identity. RVAs must not be used as the join key, and a score difference
alone is not source evidence: first compare `src_hash` and each target's
historical MAX.

The 2026-08-26 audit found no function whose 1.01 historical MAX was 100 while
its 1.00 historical MAX remained below 100. The only two functions currently
exact on 1.01 but not current-exact on 1.00 had the same source hash, and 1.00
had already reached 100 historically:

| function | 1.00 current / historical MAX | 1.01 current / historical MAX |
|---|---:|---:|
| `CDDrawWorkerHost::Load` | 99.9841 / 100 | 100 / 100 |
| `CNetSession::ReadyForSequence` | 89.5349 / 100 | 100 / 100 |

Those rows expose target- or TU-state-dependent current codegen, not missing
1.00 source structure. The reverse comparison was immediately productive.
Five post-branch source reconstructions were current-exact on 1.00 and stale on
1.01; applying the same humane source form with the 1.01 address annotations
made all five 1.01 bodies exact:

| function | 1.01 before | 1.01 after | 1.00 source evidence |
|---|---:|---:|---|
| `ButeGroup_Apply` | 87.7283 | 100 | `e0da5a006` |
| `CDDSurface::Blit1624` | 95.1484 | 100 | `4f1fa0686` |
| `CNetMgr::AddPlayer` | 88.1651 | 100 | `608221d3f` |
| `CDDrawChildGroup::BoxesOverlap` | 85.1494 | 100 | `59f590e83` |
| `CWwdSpatialMgr::DeactivateOutside` | 86.8795 | 100 | `a7dec134c`, `65634858c` |

This is the safe cross-version workflow: join identities, prefer a changed
source hash that is exact on the sibling target, verify that the executable
body is unchanged or structurally compatible, transplant source without its
RVA labels, then require a normal strict build against the destination target.
Same-hash score asymmetries are still useful, but only as compiler-state clues;
there is no different source spelling to copy from them.

## Conservative migration audit (historical)

The following commands produced the pre-rewrite audit from the 1.00 census:

```sh
gruntz sema version-delta
gruntz sema version-delta --json > build/version-delta-100-101.json
```

Run them from a pre-migration revision. The command now refuses an active
census that does not describe `--old`, because interpreting the already-moved
1.01 rows as 1.00 spans would produce false evidence.

The audit masks corresponding PE HIGHLOW relocation payloads, searches for
unique complete admitted spans, and then tries neighbour-proven placements.
It deliberately does not rewrite source or configuration. A `placed-rel32`
row is only a candidate: all observed differences occupy apparent `E8`/`E9`
or `0F 8x` displacement fields, but the lightweight recognizer is not an x86
decoder. Disassembly and referent checks must promote those rows.

Pre-migration audit census:

| population | exact mechanical | rel32 candidate | adjudicate |
|---|---:|---:|---:|
| source annotation RVAs | 2,326 / 5,793 | 2,816 | 651 |
| tracked configuration address cells | 13,825 / 31,394 | 5,878 | 11,691 |
| admitted function spans | 3,701 / 11,265 | 4,227 | 3,337 |
| admitted data spans | 2,357 / 11,434 | 0 | 9,077 |

The function exact total is 2,854 globally unique spans plus 847 spans placed
and byte-verified at a neighbouring exact delta. The four remaining ambiguous
function spans and repeated data are intentionally not guessed.

Per tracked table:

| table | exact | rel32 | adjudicate |
|---|---:|---:|---:|
| `retail/functions.tsv` | 4,721 | 3,309 | 3,235 |
| `retail/functions_static_libs.tsv` | 1,855 | 272 | 2 |
| `retail/functions_zlib.tsv` | 64 | 1 | 0 |
| `retail/data.tsv` | 4,356 | 0 | 7,078 |
| `retail/data_compgen.tsv` | 1 | 0 | 1,018 |
| `retail/data_static_libs.tsv` | 81 | 0 | 54 |
| `retail/data_vtables.tsv` | 6 | 0 | 225 |
| `retail/data_zlib.tsv` | 14 | 0 | 16 |
| `retail/link_bands.tsv` | 10 | 0 | 18 |
| `retail/link_order.tsv` | 254 | 323 | 7 |
| `retail/reloc_referents.tsv` | 21 | 9 | 14 |
| `match_baseline.tsv` | 2,442 | 1,964 | 24 |

## Migration procedure used

1. Pin the active target and preserve the audit report as evidence.
2. Apply only `unique` and `placed-exact` translations, retaining an old→new
   provenance record for every rewritten cell.
3. Decode and referent-check each `placed-rel32` span before promotion.
4. Rebuild the Model and target objects; repair the 17 load-bearing
   `DATA_COMPGEN` sites from their 1.01 payloads before treating downstream
   delink errors as independent failures.
5. Adjudicate changed named functions and repeated data structurally.
6. Refresh the baseline only after a full `gruntz build` is green against
   1.01. Historical 1.00 docs remain labeled evidence; they are not blindly
   search-and-replaced.

The first 1.01 all-unit label audit built all 293 base objects. It stopped in 11
root units at stale 1.00 `DATA_COMPGEN` payload pins (`projectile`,
`registryhelper`, `rezarchive`, `gruntcombat`, `gruntentrancemove`,
`gruntassetloaders`, `attractstate`, `play`, `wormholeacts`, `netsessionmgr`,
and `bracketvalue`). That was the expected first migration gate, not evidence
that 1.01 is an unusable target.
