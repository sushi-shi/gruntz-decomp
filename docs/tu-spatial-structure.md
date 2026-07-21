# TU spatial structure: proximity as a relatedness & attribution signal

**Question.** How far apart in retail RVA space are the functions of one class/TU,
and does that layout let us say which methods are *related* — even attribute
*unnamed* functions to a class? Measured over the matched `src/` (1.5k+ RVA-annotated
functions) joined with the Ghidra boundary export. Companion to
`docs/link-order-investigation.md` (intra-TU source order + cross-TU link order);
this doc covers what sits *inside* one TU's block and how to exploit it.

Tooling: `python -m gruntz.audit.tu_layout` (the analysis) and
`python -m gruntz.analysis.gen_attributed_stubs` (turns attributions into stubs).

## The core finding

Distance from each matched function to its nearest same-TU sibling, by kind:

| kind | n | median | p90 | within 8 KB |
|---|---|---|---|---|
| **method** (ordinary *and* virtual overrides) | ~1300 | `0xa0` | `0xb10` | **93%** |
| constructor | ~90 | `0x160` | `0xf90` | 92% |
| **destructor** | ~110 | `0x7b0` | **`0x9dac0`** (~645 KB) | **57%** |

The structuring axis is **not** virtual-vs-simple. Virtual overrides (`Tick`,
`GetTypeTag`, slot methods) cluster right next to the plain methods. The outlier is
the **destructor**: MSVC emits the vtable-referenced *deleting* destructor as a
COMDAT, and the linker pools those into shared low-address runs — `0x10000-0x14000`
(~35 classes) and `0x80000-0x90000` (~17 classes) — alternating
`scalar_deleting_destructor` thunks and `~Class` bodies from *unrelated* classes.

**Mechanism.** Out-of-line methods written in one `.cpp` land as a contiguous
source-order block (the linker keeps an object's COMDATs together — see the
link-order doc). Special members and other implicitly/inline-emitted functions are
COMDATs the linker pools elsewhere. So a class's *own block* is tight; its dtor is
exiled.

## Relatedness model

- **Proximity ⇒ same class/TU** holds cleanly for ordinary + virtual methods: a
  matched method's tight RVA neighbours are its siblings (the "batch siblings"
  matching lever). `tu_layout --neighbors 0xRVA` lists them.
- **Destructors are the exception** — pooled, so proximity can't tie them to a
  class; recover them by leaked name / vtable / RTTI (which `config/match-queue.md`
  already does).

## Exceptions & intermingling

1. **Special-member pools** (above) — the dominant exception.
2. **Scattered classes** — ~20 of ~72 multi-method classes have <50% of methods in
   their longest contiguous run. Two flavours: *conflated TUs* (engine base + game
   glue under one name: `CNetMgr`, `CGameLevel`, `CFileIO`) and *COMDAT-heavy
   families*. **`CAttract` is the worked example**: virtual-slot overrides at
   `0x14xxx`, EH dtor pooled at `0x8cxxx`, and a `LoadTitleConfig`/`Activate` pair at
   `0xa0xxx` *interleaved with `CMenuState`* — a state-class soup.
3. **Lone far methods** — ~68 plain methods >0x4000 from any sibling, mostly the
   only matched method of a class so far (resolves as coverage grows).

**Do classes intermingle?** Mostly no. At the method-block level (pools excluded):
~832 same-class adjacent pairs vs ~412 boundaries, and only ~62 true A-B-A splices
(~5%). And the splices are almost all between **sibling classes** of one family
(`CButeMgr`/`CButeValue`, the State family, DDraw/Trigger/Command pairs) — so a
mis-attribution lands on a same-family neighbour, not a random class.

## Attribution: tie unnamed functions to a class

`tu_layout --attribute` brackets each classless function: if it sits between two
matched functions of the **same class C**, within `--gap` (default `0x4000`), it is
C. Confidence:

- **HIGH** — C sits in a contiguous run of ≥3 of its own methods, and neither
  bracket end is in a pool. ~260 bodies.
- **MED** — short/mixed run or pool-adjacent (right family, not always exact). ~110.

Two target sets:

- **Unnamed `FUN_` bodies** (Ghidra boundary export) → ~365 attributed → new class
  stubs via `gen_attributed_stubs` (the `engine_attributed` unit below).
- **Already-labeled catch-all stubs** — `ApiCallers.cpp` (generated from
  `docs/api-caller-name-plan.tsv`), `Backlog.cpp`, `EngineExternFns.cpp`, and any
  stub under a placeholder class (`EngineLabelBacklog`, `ThisStubOwnerUnknown`). Of
  343, proximity ties ~80 (56 HIGH) to a real class — e.g. `0x0143e0` (a
  `PostMessageA` caller) → `CAttract`, the `ThisStubOwnerUnknown` run → `CBattlezDlg`.
  These already carry a typed `@stub` body, so they are a **relocation worklist**
  (move the stub into that class's TU — for `ApiCallers.cpp` via its generator
  input), *not* new stubs; emitted with `kind=relocate` in `--emit`.
- **Class-boundary functions** — where the two matched neighbours are *different*
  classes, the both-sides rule abstains (picking the nearer class is only ~58%
  exact). These are not dropped: `boundary_targets()` records **both** adjacent
  classes, and `gen_boundary_stubs` emits them as the **`engine_boundary`** unit —
  neutral `void Boundary_<rva>()` free-function stubs trailed by
  `// proximity: <below>@-0xN | <above>@+0xM`, plus an inline `@proximity` notice on
  the hand-maintained `Backlog.cpp` boundary stubs. ~628 of them; a verify-then-pick
  worklist, deliberately *not* committed to one class. Why so many: catch-all/free
  helpers cluster at TU-block edges, so most sit at a boundary rather than inside a
  block.

**Validation (leave-one-out on matched methods):** hide each known method and predict
from its neighbours — the HIGH (both-sides) rule is **~91% exact, ~94% same-family**.
That precision is measured against ground truth, independent of any runtime trace
(the trace's `Discovered.cpp` labels are loose, especially single-observation, so
they are *not* used as an oracle here).

97% of unnamed bodies have a matched function within `0x4000`, so attribution
coverage rises automatically as matching progresses.

### From attributions to backlog stubs

`python -m gruntz.analysis.gen_attributed_stubs` emits the HIGH attributions as
class-tied stubs, modelled exactly on `gen_class_stubs` (the trace integrator):

- `include/Stub/attributed.h` — minimal `class C { void C_<rva>(); }` decls.
- `src/Stub/Attributed.cpp` — `RVA(rva,size) void C::C_<rva>() {}` empty `__thiscall`
  stubs, the **`engine_attributed`** unit (registered in `config/units.toml`).

The build-side duplicate-RVA guard is honoured (RVAs already labeled anywhere,
incl. the trace's `Discovered.cpp`, are skipped). Each stub is then delinked and
tracked in objdiff like any backlog entry (initially ~0% — the worklist). A matcher
reconstructs it and, if the proximity class was slightly off, moves it to the right
TU. The proximity and trace signals are **complementary**: in practice their function
sets are disjoint (trace reaches runtime-called functions; proximity reaches
bracketed ones). Current run: **258 stubs across 47 classes**, zero overlap with the
127 trace-discovered stubs.

## Limits

- HIGH is ~91% precise, so ~1 in 11 lands on the wrong (usually same-family) class —
  acceptable for a backlog worklist a matcher re-checks, not for silent ground truth.
- Pool regions and scattered/COMDAT-heavy families (the State classes) are where
  proximity is weakest; gate on purity and cross-check vtable/RTTI there.
- On the already-CURATED catch-all backlog, proximity is redundant-or-wrong and must
  be cross-checked against existing evidence. E.g. it brackets the icon-loader
  `LoadToyBoxIcon` under `CTriggerMgr`, but its siblings were hand-graduated to
  `IconLoaders.cpp` — the curation wins. Treat catch-all hits as a prompt to verify
  (a `@proximity:` notice on the stub), not to relocate.
- Coverage is bounded by current matching: sparse regions have no matched brackets
  yet.
