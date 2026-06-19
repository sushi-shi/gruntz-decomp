# matching-xai extraction — verified ≥90% matches

A cheap model produced a large `matching-xai` branch of "simple function" matches.
Its output was **not trusted**, so this change re-derives the trustworthy subset
from scratch and ports only what survives verification.

## What was verified, how

1. **Ground-truth %** — `matching-xai` was built (`gruntz build`, real MSVC-under-wine
   + objdiff), and the per-function `fuzzy_match_percent` from `build/objdiff/report.json`
   was used as the only source of truth (NOT the model-generated `match_baseline.tsv`).
   Of 433 functions in the report, **267 were ≥90%; 202 already existed on `matching`;
   65 were genuinely new ≥90% matches.**

2. **Class placement — vtable cross-check.** Vtables were reconstructed directly from
   `GRUNTZ.EXE` (`.rdata` dword runs pointing into `.text`, independent of any model
   label). For each placeholder-class virtual (`UnknownRemus::VirtualMethodUnknownXX`
   etc.) we confirmed the RVA actually occupies a vtable slot. Finding: the
   `VirtualMethodUnknown<hex>` suffix is a sequential project codename, **not** the
   literal vtable byte offset (true even for already-matched functions) — so the suffix
   is not a correctness signal; vtable membership is.

3. **Class placement — `.text` adjacency / source order.** MSVC 5.0 emits each TU's
   functions contiguously in `.text`. Cross-referencing every candidate's RVA against
   the full Ghidra function list (its immediate `.text` neighbours' units) confirmed
   each new function sits among same-unit or unmatched (`FUN_`) neighbours. Caveat
   measured: a class's methods are *mostly* contiguous but can be split across `.text`
   regions (e.g. `UnknownRemus` clusters at 0x15ccd0–0x15d680 but two methods sit
   ~0x4000 away at 0x16119x, sharing vtable 0x1f0150) — so adjacency corroborates but
   does not by itself delimit a TU; combine it with vtable membership.

## Outcome

**39 functions ported** (all build green under MSVC 5.0, all ≥90% objdiff, **no
regressions** vs the committed baseline):

| unit | n | unit | n | unit | n |
|---|---|---|---|---|---|
| font | 12 | unknownremus | 3 | unknownsirius | 2 |
| butemgr | 6 | unknownworkerfuncs | 3 | tileswitchlogic | 2 |
| unknownlucius | 2 | unknownpettigrew | 2 | cplay/dialogs/fonts/grunt/image/unknownfilch/unknownsalazar | 1 each |

Stub reconciliation: each ported function that previously existed as an `@stub`
(in `src/Stub/` or inline in a real unit) had its stub entry pruned (covered by the
`verify_stub_labels.py` cross-check); two emptied stub files were dropped from
`src/Stub/All.cpp`.

### Dropped (not new value)
- **4 `UnknownRemus` functions** (0x15cdf0/0x15ceb0/0x15cf70/0x15d0d0): `matching`
  already matches these at 100% with richer types (`RemusCoords*` vs the model's
  generic `DWORD_PTR*`) — kept the existing better versions.
- **`RezMgr::ReportError`** (0x08dc60): only 77% against `matching`'s `RezMgr` layout
  (< 90%) — left as a stub.
- **`CGameWnd` deleting-dtor thunk** (gruntzwnd): the model modelled it as a second
  `CGruntzWnd::~CGruntzWnd`, which conflicts with the existing dtor.

### Deferred — verified ≥90% but need engine/win32 scaffolding imported into the
existing curated TUs (a clean follow-up; placement already verified above):
- `unknownhermiona` (4), `unknownhagrid` (3), `unknownalbus` (2) — share the
  `CMapStringToOb` / `CString` / node-list "DDraw surface family" scaffolding.
- `harrypotter` (4) — `CreateChildSurface`, Lucius child layout, `RelayHwnd`.
- `unknownseverus` (2: `MapTeardown`, `StringCopy`) — `CMapStringToOb` iteration, `_strncpy`.
- `gruntzapp` (3) — `U10O` type + the win32 message-dialog path.
- `rezmgr` (1: `HandleDebugPosition`) — `PostMessageA` / `CheckDbgVal`.

These are the matching worklist's next pickups; their RVAs and class placement are
already confirmed, so reconstruction only needs the supporting type declarations.
