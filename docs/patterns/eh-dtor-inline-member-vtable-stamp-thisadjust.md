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

NESTED-REGION variant (multiply-derived member with a 2-vptr / multi-call teardown). When the
inlined member's dtor is itself a *multi-call multiply-derived* teardown — re-stamp both vptrs,
`call ClearBody` (the most-derived body), `neg ecx;sbb ecx,ecx;and ecx,ebx;call RestoreBase2` (the
masked second-base-`this` adjust), `call BaseDtor` — retail destructs the member as a NESTED EH
region: the OUTER member region takes its own state slot (`[esp+0x2c]` = 8/a/c per member) while
the inner base teardowns use the primary slot (`[esp+0x28]` = 7/9/b then 2/1/0), and the member-
`this` cleanup pointer is stored (`mov [esp+0x1c],edi`). An inline manual-stamp member dtor
reproduces ALL the instruction selection (the two `mov [edi]/[ebx],<vtbl>` stamps, `mov ecx,edi;
call`, the `neg/sbb/and` second-base mask) but COLLAPSES the member into a single trylevel (2/1/0,
no outer 8/a/c slot, no `[esp+N],edi` re-point) — which also shrinks the frame 0x10 (`push ecx`
vs retail `sub esp,0x14`; `add esp,0x10` vs `add esp,0x20`), shifting every `[esp+N]` operand.
You can't make a non-polymorphic manual-vtable member emit the nested two-slot region: real C++
bases would synthesize divergent vtables (the virtuals live in other TUs), and out-of-lining the
member dtor turns the inline teardown into a `call ~Member` (retail inlines). Evidence:
`CButeMgr::~CButeMgr` (0x213c0) — 68.6%, three CButeStore sub-trees each inlined at +0x18/+0x48/
+0x74; body/CFG/instruction-selection/reverse-decl-order all byte-correct, only the EH-state
machine's region granularity + the coupled 0x10 frame-size differ.
