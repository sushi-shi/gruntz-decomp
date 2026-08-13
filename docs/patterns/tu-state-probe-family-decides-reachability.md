# A FLAT declaration-count sweep is evidence about the PROBE, not about the function — vary the declaration KIND

tags: cpp:include cpp:class cpp:typedef cpp:enum | asm:mov asm:sar asm:shl | topic:wall topic:regalloc topic:codegen-idiom
symptoms: a function whose residue is a canonical operand/term order or a register rotation
sits at exactly the same fuzzy% across a whole sweep of N throwaway prototypes (N = 1..24,
no digit of the score moves), so the wall is written off as unreachable — yet its `hist_pct`
proves a much higher score once existed
confidence: 9/10 (12 functions, 3 TUs, 81 measured TU states)
variants: declaration-count-window-steers-regalloc.md, commutative-operand-order-is-canonical.md

[`declaration-count-window-steers-regalloc.md`](declaration-count-window-steers-regalloc.md)
sweeps **N throwaway prototypes**. That is one probe FAMILY, and it only reaches the
functions whose parse-state phase that family happens to step through. Three families over
the same six TUs, 81 states, measured 2026-08-10:

| probe family | where | `CLightFxRender::BuildRockyRoadzPalette` | `CShadeTableCache::HsvShiftTable` | `CSpriteRef::Build` |
|---|---|---|---|---|
| `int f_i(int);` x N, N=1..24 | after the includes | **97.58 flat, all 24** | **84.85 flat, all 24** | 83.2 - 99.82 |
| `struct { int m_i; int f_i(int){...} }` x N, N=1..26 | above the includes | 96.8 - 97.52 | 83.1 - 86.52 | 96.47 flat |
| **MIXED kinds, split across both points** | both | 96.7 - **99.50** | 83.1 - **94.31** | 83.0 - **100.00** |

The mixed family draws each declaration from `typedef` / `enum` / `struct` / class-with-an-
inline-member / `extern` / file-scope `static` datum / prototype / `static` function WITH A
BODY, seeded by N, and splits them between the two insertion points. Each kind advances cl
5.0's parse state differently (the same finding the probe matrix in
[`commutative-operand-order-is-canonical.md`](commutative-operand-order-is-canonical.md)
reports for a single slot), so a mixture visits states no uniform run reaches.

```python
kind = rng.randrange(8)                  # the whole point: NOT one kind
"typedef int T;" | "enum E { A=0 };" | "struct S { int a; };"
"class C { public: int a; int f(int x){ return x + a; } };"
"extern int x;" | "static int d = 1;" | "int p(int,int);"
"static int fn(int x) { return x * 2; }"
top = rng.randrange(0, n + 1)            # and NOT one insertion point
```

WALL-CLASSIFICATION rule, not a matching trick: **"the sweep was flat" is only a valid
wall report when the sweep varied the declaration KIND.** Eight `CLightFxRender` palette
builders, `HsvShiftTable`, `SubTable` and `CDDrawWorkerHost::Draw` all read as immovable
under a uniform prototype run and every one of them moved 2-13 points under the mixed one;
`CSpriteRef::Build` and `CShadeTableCache::AlphaTable` reached 100.00 and were parked as
proven correct. The probes are diagnostics: bank the MAX, then delete them.

`CGameLevel::ProbeFootSoft` provides a stronger calibration at the last few bytes. Its
99.9863 residue was only the order of two independent member loads feeding one addition;
its CFG, size, and zero-relocation multiset already agreed. Three local expression and
statement forms compiled byte-identically, but **44 of 60** mixed TU states emitted the
391-byte retail function exactly. Trial 1 was retained as a historical 100.00 MAX for the
unchanged source hash `a7b2facaa76c`, then the declarations were discarded. Thus an
almost-exact, relocation-free two-load rotation can still be a C1 handle-state wall, and
a small local spelling matrix does not refute that classification.

## Quantified (2026-08-13): the mechanism is C1XX symbol-handle renumbering, and each probe kind has a measured stride

The hypothesis above — "each kind advances cl 5.0's parse state differently" — is now a
measured number, proven at the IL boundary. MSVC 5.0's front end can be tapped
(`/d1il<prefix>` captures the four C1XX→C2 streams `ex`/`gl`/`in`/`sy`; `/d2il<prefix>`
feeds them back), and an appended unused declaration renumbers every later symbol handle
while the symbol NAME sequence stays identical — HoMM3's C1 signature, reproduced on our
compiler. The codegen delta reproduces from the IL bytes alone through one unchanged C2
(`fed(IL_B) == plain B`), so the verdict for declaration-count and include-set
perturbations is FRONT-END, not a C2 codegen theory.

Measured handle strides per appended probe kind (SpriteRef.cpp, cpp-rtti; the C front
end differs — a C struct costs +1):

| probe kind | handle Δ |
|---|---|
| `typedef` / `extern` datum | +1 |
| `enum` | +2 |
| prototype / `static` function with body | +3 |
| `struct` | +7 |
| class with an inline member | +11 |

A uniform sweep of one kind steps the counter in a fixed stride and can only visit that
residue class — that is WHY flat single-family sweeps stay flat. Mixing kinds is what
changes the stride.

State-reachability is PER-TU, and the tap sorts a TU in ~8 minutes before any
sweep is spent on it: run the five-kind probe panel through the causation leg
(`causation.py <tu> <profile> "<probe>"`) and count .text diff bytes. Zero
across the panel = the TU's residue is C2-ANCHORED (proven for
gruntphasestep, font, grunt, playercommandstep on 2026-08-13 - every object
delta was symbol-table text); the movers this pattern originally measured
(the DDrawMgr palette TUs) are the reachable class. Panel FIRST, sweep only
on reachable TUs. Full recipe, normalization rules (the `ex` stream carries u8/u16
source-line records that must be masked), and the probe scripts: regenerate with the
capture commands in `build/il-probe/REPORT.md` (evidence run 2026-08-13; scripts
`ilcap.py` / `sweep.py` / `causation.py` beside it).
