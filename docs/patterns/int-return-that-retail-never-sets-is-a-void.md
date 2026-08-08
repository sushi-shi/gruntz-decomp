# We declared `i32` where retail declared `void` - and it costs the whole prologue

tags: cpp:return | asm:xor asm:ret | topic:codegen-idiom
symptoms: base has `xor eax,eax` / `mov eax,1` immediately before a `ret` that retail does
not have; retail's `ret` blocks leave eax holding whatever the last call returned; often
paired with an extra callee-saved `push` in the prologue
confidence: 9/10

`gruntz.audit.void_return_type` finds functions we declared `void` that retail returns a
value from. The **inverse** is just as common and much easier to miss, because a wrong
`int` return type has no tell of its own - it only shows as a couple of `xor eax,eax`
rows plus, very often, a whole rotated register allocation, since the extra live value
costs a register and blocks
[shrink-wrapping](shrink-wrapped-prologue-needs-one-tail-return.md).

`CTriggerMgr::ReinitGroup` 0x79b80 (2026-08-08): declared `i32`, `return 0;` from the
guard and `return 1;` at the tail. Retail sets eax on **neither** path - the guard jumps
straight to the shared `mov ecx,[esp+0x10]; mov fs:0,ecx; add esp,0x1c; ret 8` and the
body falls into it after popping. Flipping the declaration to `void` (its single caller
discards the result) took it **83.06 -> 97.31**, and the last 2.7 was a separate
`CByteArray::Add` vs `SetAtGrow(GetSize(), …)` spelling -> **100.00 EXACT**.

## How to confirm before you flip

1. `gruntz sema disasm <rva> --target --lite | grep -n 'eax'` - if the only eax writes are
   call results and address arithmetic, and no `ret` is preceded by a constant load into
   eax, the function is `void`.
2. `gruntz sema xref <rva>` the callers and check the result is discarded. **A caller that
   uses it refutes the theory** - `CGameLevel::MoveRising`/`MoveFalling` look identical to
   the sieve (no eax setup at the `ret`) but they are genuine `int` returns whose value is
   simply already in eax from the last callee.
3. Flipping the return type rewrites the mangled name (`QAEHHH@Z` -> `QAEXHH@Z`); update
   the header decl and every call site in the same change, then confirm the `RVA()` still
   binds by re-reading the per-function row from `gruntz sema match <unit>`.

Sieve: `python -m gruntz.audit.shrink_wrap` will usually flag these too, because the
spurious return value is what blocks the shrink-wrap.

related: [void-return-collapses-the-guard-ret.md](void-return-collapses-the-guard-ret.md),
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
