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
