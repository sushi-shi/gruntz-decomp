# Mark a NON-trivial, RVA-keyed mid-base dtor `inline` so thin derived dtors FOLD it (the retail double-pass), not CALL it
tags: cpp:dtor cpp:eh cpp:inline cpp:virtual | asm:mov asm:call | topic:codegen-idiom topic:eh
symptoms: a most-derived `~Mid` is its own RVA-keyed `??1Mid` function, AND thinner variants `D : Mid` (own most-derived vtable) re-stamp + re-run the SAME teardown then fold Mid; recompiling `D::~D` with `Mid::~Mid` defined OUT-OF-LINE emits a single `call ??1Mid` (frame collapses, ~53%) where retail INLINES the whole Mid teardown into D (double worker pass + member dtors, ~93%+)
confidence: 9/10
variants: inline-base-dtor-folds-into-leaves.md, eh-dtor-multilevel-polymorphic-chain.md, comdat-inline-ctor-no-standalone.md

The `inline-base-dtor-folds-into-leaves` case is a *trivial/empty* base whose dtor
folds. This is its **non-trivial, self-RVA-keyed** sibling: the mid level `Mid`
(here `CWwdGameObject`, vtable 0x5f0020) is a real instantiated class whose dtor at
RVA 0x15b4f0 does the full teardown (four worker scalar-deletes + a CString member
+ two sentinel-handle members). Thin factory variants `A`/`C`/`F` derive from it
(`CWwdGameObjectF : Mid`, own vtable 0x5f0060), and retail's `~F` STAMPS F's vtable,
re-runs the worker pass, then **inlines** Mid's entire teardown (re-stamp 0x5f0020,
second worker pass, CString dtor, sentinel folds, grand-base 0x5e8cb4 stamp).

MSVC5 only inlines a base subobject's destruction when the base dtor is **`inline`**.
An out-of-line user-defined `Mid::~Mid(){…}` is treated as a regular function and
`~F` emits `call ??1Mid` — the /GX frame collapses and the double-pass disappears
(53%). Because `Mid::~Mid` is *virtual*, marking it `inline` still emits the
standalone COMDAT `??1Mid` (the vtable needs it), so the RVA key survives — and now
every thin `~D` folds the teardown inline, exactly as retail. No duplicate-RVA fires
because the `RVA()` sits on the non-deleting `??1Mid` (the auto `??_GMid`
scalar-deleting dtor is a *separate* symbol).

```cpp
struct WwdSeverusBase { virtual ~WwdSeverusBase(); };   // grand-base, vtable 0x5e8cb4
inline WwdSeverusBase::~WwdSeverusBase() {}             // empty -> stamp sinks last

class CWwdGameObjectE : public WwdSeverusBase {          // Mid, vtable 0x5f0020
    ~CWwdGameObjectE();
    WwdEdgeB m_04; WwdEdgeA m_20; WwdWorker* m_7c; /*…*/ WwdName m_dc;
};
RVA(0x0015b4f0, 0xde)
inline CWwdGameObjectE::~CWwdGameObjectE() {             // <-- INLINE is load-bearing
    WORKER_FREE(m_7c); /*…*/ m_c0 = (i32)0x80000000; m_d8 = -1;
    m_20.c = (i32)0x80000000; m_20.a = (i32)0x80000000; m_20.b = -1;
}                                                        // CString + edges + severus fold

class CWwdGameObjectF : public CWwdGameObjectE { ~CWwdGameObjectF(); };  // own vtable 0x5f0060
RVA(0x0015bad0, 0x153)
CWwdGameObjectF::~CWwdGameObjectF() { WORKER_FREE(m_7c); /*…*/ }          // folds Mid inline
```

Two further sub-points the WWD family pins down:
- The grand-base (`WwdSeverusBase`/severus 0x5e8cb4) must have an **empty** body with
  the sentinel field-clears living in destructible members (`WwdEdgeA`/`WwdEdgeB`) of
  the DERIVED `Mid` — each member is a top-level /GX unwind state, so the trylevel
  rises to 3 (matching retail) and the call-free base body lets cl **sink** the
  grand-base vptr store to the function tail (`groupY` then stamp 0x5e8cb4).
- These vtables are **non-RTTI** (`Vtbl_*`, no `??_7…@@6B@` in the catalog); the
  compiler-emitted `??_7CWwdGameObject*` stamps still reloc-mask against the target's
  `?g_wwd*Vtbl` DIR32 relocs — the symbol NAME never has to agree.

Evidence: `CWwdGameObjectE` 79.5%→88.8% (standalone, walled on zero-register-pinning),
`A` 75.1%→95.5%, `F` 87.0%→93.4%, `C` 88.3%→93.6% — each driven by the inline-fold.
The same conversion (manual `*(void**)this=&g_…Vtbl` → real `virtual ~`) flipped the
clean 2-level `CSeverusWorkerX` 0x17f330 87.8%→**100%** (stamp now in the prologue).
Residuals on the WWD leaves are the documented `zero-register-pinning` /
`const-materialize-into-reg-vs-immediate` regalloc walls (callee-saved const coloring),
not the fold.
