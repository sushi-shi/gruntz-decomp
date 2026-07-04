# Link order ⇄ TU layout: recovering the retail build order

**Question.** Is there a correlation between a function's address in retail
`GRUNTZ.EXE` and the translation unit (`.cpp`) it came from? If so, can we recover
the order the linker processed the object files, and replicate it — which is what
we need to make whole-TU layout match?

**Answer.** Yes, and the model is now *empirically verified with the original
linker*. This doc records the method, the evidence, and the actionable
consequences for matching.

> **See also** [tu-spatial-structure.md](tu-spatial-structure.md) — what sits
> *inside* one TU's block (tight method runs vs. the exiled-destructor COMDAT
> pools) and how to attribute *unnamed* functions to a class by RVA proximity
> (`gruntz.analysis.tu_layout` / `gen_attributed_stubs`).

## The missing tool: we can now link

The investigation needs the genuine VC5 linker, and we now run it. The toolchain
ships the real **`link.exe` version 5.10.7303** — the exact linker that built
retail GRUNTZ.EXE (PE optional-header linker version 5.10). One snag: it statically
imports **`MSDIS100.DLL`** (the VC5 disassembler engine, used only by the
`link /dump /disasm` path), which the toolchain tarball omits, so under wine it
failed to load (`status c0000135`). `scripts/gruntz/build/msdis_stub.py` resolves
this — it prefers a real sourced `MSDIS100.DLL`, else synthesizes a tiny stub that
exports the 11 symbols link.exe imports (never called during linking, so link
output is identical). It is installed into the wine prefix's 32-bit system dir
(`syswow64` on the WoW64 prefix).

```sh
gruntz link            # base objs -> build/exe/GRUNTZ.candidate.EXE + .map
gruntz link --analyze  # + print the layout/link-order report
ninja candidate        # same, as a ninja target
```

The reconstruction is partial (~700 of ~9700 funcs), so link.py passes **`/FORCE`**
and the EXE does **not run** — the deliverable is the **`.map`**, which lists every
function's link-assigned RVA and *source object*. Layout study uses
`/OPT:NOREF /OPT:NOICF` so every COMDAT survives into the map.

## The three-level layout model (verified)

`scripts/gruntz/analysis/link_order.py` cross-references the candidate `.map` (our link's
RVA + object per function) with `build/gen/symbol_names.csv` (retail RVA → unit).

### 1. Intra-TU order = source-definition order

MSVC `/O2` emits each function as a COMDAT, in **source-definition order**; the
linker concatenates an object's COMDATs in that order. So a TU's functions land in
`.text` in the order they are *written in the `.cpp`*.

**Proof.** For `grunt` (19 funcs), our candidate-link order is *identical* to the
order of `RVA()` macros in `src/Gruntz/Grunt.cpp` — and the candidate order is
computed with **no reference to retail addresses**. The compiler/linker layout is
purely a function of source text order.

**Corollary (the matching lever).** Two functions' *relative* retail addresses tell
us their *original source order*. The candidate link reproduces retail intra-TU
order **iff** our `.cpp` defines functions in retail-RVA order. Evidence: all
**zlib** TUs (faithful copies, original order) match retail exactly
(`trees` ×17, `deflate`, `inflate`, `inftrees`, `infblock`, `infcodes`, `font`,
`zutil` …); reconstructed game TUs, written in arbitrary order, do not.

> **Actionable:** order each `.cpp`'s function definitions by retail RVA.
> `link_order.py` prints the exact "REORDER" worklist (e.g. `gameapp` 6 inversions,
> `grunt` 1: the `Stub_047a10/048400/048470` block belongs *before* the
> `Create*Sprite` block).

### 2. Cross-TU order = object link order

Retail `.text` is a sequence of **contiguous, mostly-disjoint per-object blocks**.
Sorting the TUs by their retail address footprint recovers the order the objects
were fed to the linker — the **build order** we want to replicate.

**Cleanest evidence:** the statically-linked **zlib** library lays out as a clean
ordered run with tiny spans and zero overlap:

```
font → uncompr → inflate → deflate → infblock → adler32 → zutil
     → trees → inftrees → infcodes → infutil → inffast      (0x179700–0x18b440)
```

The engine base classes (`CGameApp`, `CGameMgr`, `CRezDir`, `CFileIO`, `CNetMgr`)
cluster at **high** addresses (~0x133xxx–0x1bfxxx), i.e. the WAP32 engine objects
were linked as a block *after* the Gruntz game objects — matching the leaked
`C:\Proj\{NetMgr,…}` modular layout.

### 3. Separation: contiguous blocks; multi-region units flag conflated TUs

TUs are separated as disjoint address intervals. A `unit` whose matched functions
occupy **two distant regions** (gap > 0x40000) almost certainly conflates **two
real TUs** — typically an engine base class plus its game-side glue that we lumped
under one name. `link_order.py` flags them; current candidates:

```
dialogs, gametext, gamemode, winapi, gruntzapp, gameapp, rezmgr, netmgr, filestream
```

e.g. `netmgr` splits 0xb5xxx (×21) vs 0x177xxx (×11) — engine `CNetMgr` base vs
game glue. These should be split into separate units as reconstruction proceeds.

## Caveats

- Matching is **sparse**, so a unit's matched functions are interspersed with its
  not-yet-matched ones; exact TU block boundaries need fuller coverage. The
  min-RVA ordering is the inferred link order, robust at the unit level.
- A handful of wide-span units are driven by a single outlier function (a shared
  inline, or a mislabel in `symbol_names.csv`) rather than true interleaving —
  worth a case-by-case look, not a layout conclusion.
- `/OPT:ICF` (COMDAT folding) collapses identical functions to one address; retail
  used it, so some "missing" functions are folds, not gaps.

## What this unlocks

A concrete path to whole-binary layout matching: **(a)** reorder each `.cpp`'s
functions to retail-RVA order (fixes intra-TU layout); **(b)** drive the link with
the objects in retail-block order (fixes cross-TU layout); **(c)** split the
flagged conflated units. `gruntz link` + `link_order.py` make each step verifiable
against the real linker before committing to source.

## Whole-EXE metric: `gruntz exe-diff`

`gruntz link` gives the candidate; **`gruntz exe-diff`**
(`gruntz.analysis.exe_diff`) diffs that whole image against retail — one level up
from per-object objdiff. It parses both PEs (headers / section table), name-aligns
every reconstructed function (candidate `.map` RVA ⇄ retail `symbol_names.csv` RVA),
and reports layout + linked-byte fidelity. The tracked numbers, measured at
1868/3354 objdiff-exact (65.5% fuzzy):

| number | now | what moves it |
|---|---|---|
| `exe-layout-order%` | 29.5% | funcs whose TU is in retail intra-TU order — the **reorder** lever; reordering all flagged `.cpp`s takes this toward 100% |
| `exe-layout-block%` | 1.9% | funcs in a *byte-exact contiguous* retail block (order **and** every func at the retail offset-from-anchor) — reorder **+** byte-exact bodies |
| `exe-byte%` | 32.9% | name-aligned linked-`.text` byte identity (relocations applied) — rises with matching **and** coverage |
| `exe-byte(exact)%` | **87.5%** | same, over the objdiff-exact subset — the ~12% residual is *purely* unresolved-extern displacement bytes (`/FORCE`→0), i.e. code purity is high; the gap is coverage, not codegen |
| `abs-rva%` | 0% | funcs at the correct **absolute** retail RVA — the final target |

**What drives the gap (measured).** `abs-rva%` is 0 and *stays 0 even after a
counterfactual relink with the retail-inferred cross-TU order* — because absolute
placement needs the correct link order **and** full coverage (every preceding object
present at retail size); with ~38%-of-retail bytes reconstructed, a single missing
object shifts everything downstream. So today the RVA-reorder campaign's payoff is in
the **relative** numbers: it drives `exe-layout-order%` → 100% and lets byte-exact TUs
become byte-exact *blocks* (`exe-layout-block%`), which is the prerequisite that makes
`abs-rva%` climb once coverage nears complete. The `exe-byte%` numbers are
placement-independent (name-aligned), so they are a clean code-fidelity signal now:
`exe-byte(exact)%`=87.5% confirms the reconstructed code is genuinely right and the
whole-EXE byte gap is dominated by not-yet-reconstructed callees, not by wrong bytes.
