# Force the standalone COMDAT of an inline base ctor, and isolate a depth-dependent inline callee in its own TU
tags: cpp:ctor cpp:inline cpp:eh | asm:call asm:mov | topic:codegen-idiom topic:eh

symptoms: an inline base ctor (folded into every leaf) whose OWN out-of-line copy also exists in retail at its own RVA; a leaf ctor CALLS a registration/init helper (`call thunk`) but the standalone base ctor at the same RVA INLINES that same helper; a base ctor that sets N fewer members than the folded-into-leaf copies
confidence: 8/10
variants: inline-base-dtor-folds-into-leaves.md

The ctor analog of inline-base-dtor-folds-into-leaves. A base ctor `B(obj)` that is
`inline` in the shared header (so leaf ctors FOLD it) still has a standalone
out-of-line COMDAT in retail (called by big factory fns that don't inline it).
Unlike a dtor, a ctor has NO synthesized caller (`??_G`), so nothing forces the
COMDAT — you must force it and pin it:

- **Force emission.** A small ctor is always inlined; wrap a `new B()` forcer in
  `#pragma inline_depth(0)` to emit the standalone COMDAT. A big ctor is emitted
  standalone once it overflows the caller's inline budget — call `new B(o)` a few
  times in the forcer at DEFAULT depth (so its OWN inlinable callees still fold).
- **Pin by name** with `RVA_COMPGEN(<rva>, <size>, ??0B@@QAE@…@Z)` (an inline ctor
  can't hang `RVA()`).
- **Depth-dependent callee (the trap).** If the standalone ctor must INLINE a helper
  (e.g. a Lookup-based registrar) that the leaf copies keep as a CALL, the helper
  needs a visible inline body — but if that body is visible in the leaves' TU it
  folds into every leaf at depth 2 and REGRESSES them all. Put the forcer **+ the
  inline helper body in their OWN unit** so only the standalone COMDAT (depth-1 root)
  inlines it; the leaves' TU sees only the no-body decl and keeps calling it.
- **Leaf-only member stores.** If the real base ctor sets FEWER members than the
  folded model (the extra stores really belong to each leaf), guard them
  `#ifndef B_STANDALONE_CTOR` in the header inline and `#define` it only in the
  emit TU — matching-neutral for every leaf (their preprocessed source is byte-identical).

```cpp
// emit TU only (own [[unit]]):
#define USERLOGIC_STANDALONE_CTOR          // drop leaf-only fields for the standalone
#include <Gruntz/UserLogic.h>
inline void CUserLogic::BuildLogicTypeTable(CLogicTypeBuilder* o) { /*…Lookup-based…*/ }
RVA_COMPGEN(0x000138d0, 0x4b, ??0CUserLogic@@QAE@XZ)
RVA_COMPGEN(0x00058cd0, 0x195, ??0CUserLogic@@QAE@PAUCGameObject@@@Z)
#pragma inline_depth(0)
void ForceNoArg() { g_sink = new CUserLogic(); }        // small -> depth-0 forces it out
#pragma inline_depth()
void Force1Arg(CGameObject* o) {                         // big -> budget overflow forces it out
    g_sink = new CUserLogic(o); g_sink = new CUserLogic(o);
    g_sink = new CUserLogic(o); g_sink = new CUserLogic(o);
}
```
STEERABLE (emission + isolation). Evidence: CUserLogic::CUserLogic() @0x138d0
5%->100%; CUserLogic::CUserLogic(CGameObject*) @0x58cd0 0.9%->89.0% (residual =
the same factory-fn DIR32 naming + Lookup arg-push scheduling that caps the
reference standalone BuildLogicTypeTable @0x8a40 at 87.9%), with ZERO regression
to the ~25 folded leaf 1-arg ctors (all held at their ~90% baseline).
