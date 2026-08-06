# Inline base-constructor standalone emission is a convergence wall
tags: cpp:ctor cpp:inline cpp:eh | asm:call asm:mov | topic:wall topic:eh

symptoms: a base constructor is folded into leaf constructors, while retail also
contains its standalone COMDAT; the reconstructed compiler emits no standalone copy
because every real call inlines it

confidence: 9/10
variants: inline-base-dtor-folds-into-leaves.md

A shared-header inline constructor may have its own retail body because one original
caller exhausted MSVC 5.0's inline budget. If the reconstructed caller differs, the
same compiler can flatten every call and emit no copy.

The correct model remains one inline constructor definition. Artificial allocations,
volatile sinks, repeated calls, per-TU member-store switches, and emitter-only units
are rejected: they manufacture a source-level reason for the desired COMDAT and can
isolate helper visibility in ways the original program did not.

Bind a retail RVA only in a TU whose real code naturally emits that symbol. Otherwise
leave the retail inventory row unmatched until caller structure converges. This keeps
the current build honest while the historical MAX ledger retains already measured
results.

Evidence from the former UserLogic emitter cleanup:

- `CUserLogic::CUserLogic()` at `0x138d0` is naturally emitted by
  `SerialObjectFactory.cpp`, so its compiler-generated binding belongs there.
- `CUserLogic::CUserLogic(CGameObject*)` at `0x58cd0` has no natural current emitter.
  Its former 89.0% result came from a synthetic TU and is historical evidence, not a
  current source claim.
- Removing the emitter-only TU also removed a duplicate inline
  `BuildLogicTypeTable` definition and two invented allocations.

Classification: **WALL until a real caller emits the copy.** Improving the caller can
change emission without touching the constructor; recheck after substantive caller or
declaration recovery.
