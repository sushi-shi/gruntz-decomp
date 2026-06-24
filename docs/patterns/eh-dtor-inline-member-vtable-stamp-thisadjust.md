# /GX dtor that INLINES a member subobject's manual-vtable-stamp teardown: the member-`this` adjust (`add esi,4`) + state value don't reproduce
tags: cpp:dtor cpp:eh cpp:member cpp:inline | asm:add asm:mov | topic:wall topic:eh
symptoms: retail `~Class` has the /GX frame + descending `[esp+N]` trylevel, then `add esi,4` to point esi at the embedded subobject, re-points the EH cleanup `[esp+4]=esi`, and writes the subobject teardown through `[esi+0]`/`[esi+4]` with the trylevel going 0→1; your recompile keeps esi=this and writes through `[this+4]`/`[this+8]` with trylevel 0→-1 (no `add esi,4`, no `[esp+4]` re-point), body otherwise exact, ~80%
confidence: 7/10

When the destructible member subobject's destructor STAMPS FOREIGN vtables (a transitional
manual `*(void**)&m = &g_fooVtbl` because the real vtable contents live in another TU), you keep
the stamps in the member's `~T()` rather than letting the compiler synthesize a (divergent)
vtable. Modeling the member as a real destructible subobject — `struct Member { ...; ~Member(); };`
as a class field + a real `~Class()` — DOES recover the /GX frame and lifts the dtor off the
frameless `eh-dtor-needs-base-subobject.md` ~50-60% wall (here 41%→80%). But two residuals remain:

- **member-`this` adjust absent.** Retail destructs the member as a *separate* EH cleanup region
  whose `this = class + offsetof(member)`, so it emits `add esi,<off>` + `mov [esp+4],esi` (the EH
  funclet's adjusted cleanup pointer). When you mark `~Member()` **inline**, MSVC5 folds the body
  but keeps `esi = this` and reaches the member fields through `[this+off+k]` — no `add`, no
  re-point.
- **trylevel value.** Retail's nested member region uses an ascending state (`…0→1`); the inlined
  single-region form uses the completing value (`0→-1`).

The inline keyword is load-bearing the OTHER way: making the member ctor/dtor **non-inline** (so
they fold as the member-`this` adjusted form) reproduces the `add esi,4`/state — but then the
*ctor* regresses hard (the cache ctor CALLs the member ctor instead of inlining the vtable-stamp +
field-zero, 100%→~7%). You cannot have both with one inline/out-of-line choice for the member.

WALL (for the manual-stamp case): inline keeps the ctor at 100% and the dtor at ~80%; out-of-line
flips them. The clean exit is the `eh-dtor-model-members-as-destructible.md` form — a member whose
`~T()` calls an **external** `EngineDtor()` (not manual stamps) reaches 100% because the external
call forces the member-`this` adjust naturally. Defer until the member subobject's real vtable +
external dtor are modelled. Evidence: `CShadeTableCache::~` (0x14de50) — ctor 100% + dtor 80.13%
with inline `~CShadeTableArray()` doing the two foreign-vtable stamps; out-of-line → ctor 6.6% /
dtor 60.7%. related: eh-dtor-model-members-as-destructible.md, eh-dtor-needs-base-subobject.md.
