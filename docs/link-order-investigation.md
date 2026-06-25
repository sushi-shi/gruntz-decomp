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

## Update — de-pooled recovery + the re-link confirmation

`link_order.py` now **recovers and persists** the order. Two fixes/findings:

### De-pool first, or the sort is garbage

A naive min-RVA sort is contaminated: a class's ctor/dtor are emitted as COMDATs and
end up in the shared low-address runs (`0x10000–0x14000`, `0x80000–0x90000`) far from
the class's code, so every game-object unit's *min-RVA is its pooled destructor*, not
its block — the old output flagged almost everything "conflated" and ordered units by
where their dtor landed. The tool now drops the special members (mangled
`??0`/`??1`/`??_E`/`??_G`) and positions each unit by the **min-RVA of its largest
non-special code cluster**. e.g. `userlogic` moves from a contaminated `0x10a10` (its
pooled dtor) to its real block at `0x29a50`. Units are grouped into **modules**
(`config/units.toml` source dirs); the recovered module order is

```
Utils → Wap32 → Gruntz(bulk) → Net → Io → DinMgr2 → Dsndmgr → Rez → Bute
      → Image → DDrawMgr → Wwd → Crt → Crypto → Font → zlib
```

— game/engine low, static libs (`Crt`=LIBCMT, `Crypto`, `Font`, `zlib`) last, matching
the leaked `C:\Proj\{…}` layout. **Anchor:** zlib comes out as one clean ordered run
(`uncompr → inflate → … → inffast`), exactly the known order. 219 units, only ~49
module-transitions (Gruntz is the interspersed bulk; engine/lib modules are 1–4 clean
runs). The full ordered list is persisted to **`config/link-order.tsv`** (regenerate as
coverage grows — we don't have all TUs yet).

### Re-link confirms: objects are contiguous; the dtor-pool is a *source* fact

Running `gruntz link` and reading the candidate `.map`: all **223 objects lay out as
exactly one contiguous block each** (222 object-transitions in RVA order) — the linker
does **not** interleave or "smear" objects within a link. And in a real single-class
TU object (`cpathhazard.obj`, `grunt.obj`, `image.obj`, …) the **destructor sits right
with its methods** in one block, while retail puts `~CPathHazard` ~640 KB from its
methods — *same linker*. So the retail destructor-pool is **not generic object-block
interleaving**.

**What it actually is remains OPEN — do not jump the gun.** Our test only used
*regular out-of-line* dtors, so it shows those don't get separated; it does not tell us
why retail's are. Competing hypotheses, none yet confirmed:
- (a) retail defined those dtors in a **different TU** than the methods (the
  `GameObjectCtors.cpp` central-ctor/dtor pattern);
- (b) retail's dtors were **COMDAT** (header-inline / compiler-generated), which the
  linker can place apart from regular `.text` — whereas our out-of-line definitions are
  regular, so they don't move. Under (b) the fix is the *opposite* of (a): make the
  dtors inline/COMDAT, not centralize them;
- (c) `/OPT:ICF` (retail on; our layout link forced NOICF) — **TESTED, RULED OUT.**
  Re-linking with `/OPT:NOREF /OPT:ICF` folds ~1,580 identical functions (EXE −25 KB;
  1,678 names sharing an address vs 95 under NOICF), so ICF definitely engages — yet
  every surviving function stays in *its* object's block and `cpathhazard`'s dtor still
  sits with its methods. ICF dedups, it does not pool or relocate; and retail's pooled
  dtors are at ~35 *distinct* addresses (so they did not fold together). Not the cause.

So the live hypotheses are (a) separate TU vs (b) COMDAT placement. Remaining experiment
(TODO): force a dtor to COMDAT (inline in the header) and re-link to see if the linker
then separates it from regular `.text`; and check whether retail's pooled dtors are
COMDAT-mangled (`??_G`/`??_E`) vs regular (`??1`). Until then this is an open question,
**not** a matching recommendation. (The de-pooled link-order recovery above is
unaffected — it holds whatever the mechanism.)
