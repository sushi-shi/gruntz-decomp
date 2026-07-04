# Cleanup plan (2026-07-04 survey)

Where the cleanup actually stands, measured against the live tree
(`config/cleanliness-baseline.tsv` deltas are all 0 — the TSV is current), and
the ordered plan to finish it. Match state at survey: 1,859/3,314 exact
(56.1%), 68.7% fuzzy. Worklist mirror: the session task tracker (P0–P6).

## Verdicts on the standing worries

- **Explicit vtbls — effectively DONE as a conversion problem.** 0 actionable
  own-class manual vtables remain; the 12 surviving `= &g_*Vtbl` stamps and the
  56 `*Vtbl` dispatch structs are all documented terminal (grand-base/vptr-middle
  walls, foreign-PMF dispatch, `docs/vtable-audit.md` categories) or die with the
  view fold / DX-header migration. Remaining vtable work is *anchoring*: ~296
  vtable-bearing classes unanchored (`vtable_hierarchy --coverage`), mostly view
  scaffolding that P1 folds away; survivors get `VTBL()`, then the build gate
  flips fatal.
- **Multiple views — the main structural debt, and it is bounded.** The true
  multi-view count is **186 type names defined in >1 file** (103 with divergent
  shapes); ~96% are offset-0 aliases → FOLD. The 367 single-def RVA-suffixed
  placeholders are *already* modeled-as-real distinct objects — they need
  identity recovery (rename), not folding.
- **Mismatched classes — small, concentrated.** `--name-audit` = 9 (8 weak
  structural hits on fabricated `*Base` intermediates at the CObject grand-base
  0x1e8cb4, plus the deliberate CFileIO/CFile modeling). The real live
  conflations: the g_gameReg void* slots 0x54–0x7c (per-TU divergent types),
  `Dispatcher_0cfbd0` (1 RVA modeled as 9 classes), `*_faec0` (1 RVA as 6),
  `OptCfg_c4b30` (×3 files), CBootyState sibling fusion, 13 `ClassUnknown_N`.
- **MFC/vendor library hygiene — real defects found (P0).** 23 HIGH-confidence
  NAFXCW methods are hand-reconstructed in src/ *and* FID-carved (double-claimed
  bytes): CFile ×10 (`Io/FileStream.cpp`), CWinApp profile/registry ×8
  (`ConfigStore*.cpp` — new finding, on no prior queue), CFile::Get/SetStatus +
  Afx helpers (`FilePathInfo.cpp`/`RegHelpers.cpp`). Plus ~42 LOW/AMBIG FID
  false-positive rows mislabeling game fns, 37 hand-rolled CRT decls in 20
  files, 5 fabricated `Afx*` names. **Unfounded half of the worry:** `Mfc.h`/
  `Win32.h` are real VC5 SDK wrappers (zero divergence), zlib is vendored
  properly, the iostream/`ios` cluster is carved correctly.
- **TU-local struct views — big but mostly downstream.** 2,034 type defs in 351
  .cpp files (~65% of all defs). Sequenced after the folds so we don't promote
  views that are about to be deleted.

## Ordered plan

**P0 — library-identity reconciliation** (small, correctness-critical, do first):
carve the 23 double-claimed MFC bodies (declared-only externs under real names;
accept the ~23-exact drop), prune the ~42 FID false rows, then add a fatal
`src RVA() ∩ library_labels.csv = ∅` build guard so the class can't recur.
**P0b**: real CRT headers over the 37 hand decls; alias the 5 fabricated Afx names.

**P1 — fold the 186 multi-def views** into single canonical header defs.
Clusters: DDraw surface family (CDDrawSurfacePair ×5, CDDrawSurfaceMgr ×5,
LeafScan ×5, the CDDSurface 3-dtor split), SBI widgets (CSBI_MenuItem ×5 with 3
different bases, CSBI_ImageSet ×5, …), game-object self-views (CGrunt/CMulti/
CBootyState/CGameReg), collections (CTypeKeyColl/CSymTab/CSymParser/…), the
double-viewed global C646778. Per-TU build-verify (codegen-leak precedent);
irregardless-of-% applies. Excluded: the required 0x24556c MFC/Win32 dual-view.

**P2 — identity-recover the 367 RVA placeholders + fix live conflations.**
Classifier work by cluster (Boundary* headers ~110, ApiCallers 49, ReconBatch2
14, TileTriggerFactory 11, …); xref/RTTI/new-size evidence, RTTI names
authoritative (mind the delinker-packs-RVA rename gotcha). Includes the
g_gameReg slot typing (type the canonical fields, never cast-to-reach), the
0cfbd0/faec0 one-address-many-classes splits, ClassUnknown_N, stale
`CButeNodeBase.cpp` refs, Pages-vs-Draco naming.

**P2b — TU topology reorg** (see **`docs/tu-topology-plan.md`**, 2026-07-04
audit): src/Gruntz is a dump — 76 TUs sit entirely in the engine RVA band,
~25 are pure aggregate buckets, ~22 are `*Eh.cpp` flag-splits. Execution
order: same-class merges → artifact renames (UnknownClassArrays→
CBattlezMapConfig etc.) → whole-TU moves to engine modules (DirPal→DDrawMgr,
Wwd*→Wwd, SoundStream*→Dsndmgr, …) → per-fn dissolution of Boundary*/
Discovered*/Orphan*/Api*/ReconBatch* buckets. Overlaps P2 (the buckets are
where the placeholder classes live).

**P3 — finish `docs/comdefs-removal-plan.md`** (parallel-safe): DirectDraw →
real `<ddraw.h>`, reparent engine interfaces, `WaveFormatX.h` → `WAVEFORMATEX`,
delete `ComDefs.h`.

**P4 — header promotion** (after P1): cross-TU-used .cpp-local types → shared
headers; top files Play.cpp 64, GruntzMgr.cpp 54, DDrawSubMgr.cpp 43, ….

**P4b — extern consolidation (one-home rule).** Measured: 810 `extern "C"`
lines + 1,937 plain `extern` lines across 305 files. Every declaration gets
exactly one home: a real SDK/vendor header (CRT/Win32/MFC/DX) · `Globals.h`
(+ MFC-side twin) for data · the owning module header for engine functions.
**No extern decl in any .cpp** — lint-gate it, flip fatal when drained.
Buckets: 329 `g_*` data decls + ~1,885 per-TU plain-extern blocks → the
Globals.h single-source pass (+ dangling-globals audit); ~260 engine fn decls →
owning headers, after deduping same-RVA fabricated aliases (found:
`OpNew`=`RezOpNew`=`SdOperatorNew`=0x1b9b46, `ZAlloc`=malloc 0x120b60); 56+ CRT
names + dup `timeGetTime` → real headers (with P0b); 39 ILT/`Handler_` thunk
decls → one generated carve-out header; 55 `FUN_`/RVA-hex externs dissolve via
P2. Run the data half as a solo window (touches ~every TU).

**P5 — naming/cast burn-down + gates**: m_&lt;hex&gt; 15,276 · g_&lt;hex&gt; 1,246 ·
placeholder vslots 772 · Method/Stub/FUN 263 · casts ()this 278, )m_ 808,
(char*) 883, void* m_ 418). Then anchor surviving intermediates and flip the
VTBL/SIZE gates fatal.

**P6 — match-% recovery** (after P1+P2, per the all-cleanup-first sequencing):
Wwd 21.7%, userbaselink 0/10, warlord 3/10, ddrawshadeblit 1/9, chatboxowner
2/7, gruntspawnconfig 9/18, userlogic 28/64, the 64 ApiCallers stubs (unblocked
by P2's SPINE/NET typing), the 51 heavy stubs, the `@early-stop` sweep; then
archive the stub tooling.

Standing rules throughout: build-gate every integration, re-bless
`cleanliness-baseline.tsv` + `match_baseline.tsv` at integration, README/docs
refresh with each landed phase.
