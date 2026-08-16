# We declared `i32` where retail declared `void` - and it costs the whole prologue

tags: cpp:return | asm:xor asm:ret | topic:codegen-idiom
symptoms: base has `xor eax,eax` / `mov eax,1` immediately before a `ret` that retail does
not have; retail's `ret` blocks leave eax holding whatever the last call returned; often
paired with an extra callee-saved `push` in the prologue
confidence: 9/10

[void-return-collapses-the-guard-ret](void-return-collapses-the-guard-ret.md)
covers functions we declared `void` that retail returns a value from. The **inverse** is just as common and much easier to miss, because a wrong
`int` return type has no tell of its own - it only shows as a couple of `xor eax,eax`
rows plus, very often, a whole rotated register allocation, since the extra live value
costs a register and blocks
[shrink-wrapping](shrink-wrapped-prologue-needs-one-tail-return.md).

`CTriggerMgr::ReinitGroup` 0x79b80 (2026-08-08): declared `i32`, `return 0;` from the
guard and `return 1;` at the tail. Retail sets eax on **neither** path - the guard jumps
straight to the shared `mov ecx,[esp+0x10]; mov fs:0,ecx; add esp,0x1c; ret 8` and the
body falls into it after popping. Flipping the declaration to `void` (its single caller
discards the result) took it **83.06 -> 97.31**, and the last 2.7 was a separate
spelling in the else-arm: **`m_byteArr.Add(x)` is right, `m_byteArr.SetAtGrow(GetSize(),
x)` is wrong** -> **100.00 EXACT**. (Measured both ways; the direction matters. `Add`'s
inlined body is `int n = m_nSize; SetAtGrow(n, x);` and cl then reloads `this` from its
home slot after the call instead of parking it in a callee-saved register, which is what
retail does.)

## The screen

The tree-wide return-type sieve is retired; `gruntz walls diagnose <rva>` gives
the per-function evidence (`--asm` for the pair). The verdict is the conjunction
of two conditions, and **condition 1 alone is a 90% false-positive
rate** - it is what produces the `MoveRising` family:

1. retail materialises nothing into the return register at any `ret` (read off the
   objdiff object pair), and
2. every retail call site discards the result - forward eax-liveness from each `call`,
   both edges of every branch, chased through the ILT thunk band, over the whole `.text`.
   A `ret` in the caller resolves against the CALLER's own return type: a `void` caller
   genuinely drops it, an `int` caller is forwarding it.

Calibrated whole-tree: **0 false fires over 786 functions that are declared `void` AND
already 100% exact**; it fires on 1319 of the 1517 declared-int exact ones, and the 13%
it misses are exactly this `return <callee result>` shape. Of 10 condition-1 candidates,
1 was a real `void`.

Two traps the sieve had to learn, both of which produce confident wrong answers:

- **Read `%al`, not just `%eax`.** A `bool` (`_N`) return is materialised BYTE-wide
  (`xor al,al`, `mov al,[esp+0x24]`), so an eax-only reader hands back the whole
  `CButeMgr` `_N` family - a library with a published `bool` signature.
- **Bound the backward walk by the BASIC BLOCK, not by an instruction count.**
  `CBootyState::LoadGameAssetNamespaces` parks its `mov eax,0x1` exactly ten instructions
  ahead of the `ret`, one past a 10-deep window, and read as materialising nothing.

## How to confirm before you flip

1. `gruntz sema disasm <rva> --lite | grep -n 'eax\|al,'` - if the only writes to
   the return register are call results and address arithmetic, and no `ret` is preceded
   by a constant load into it, the function is `void`.
2. `gruntz sema xref <rva>` the callers and check the result is discarded. **A caller that
   uses it refutes the theory** - `CGameLevel::MoveRising`/`MoveFalling`/`MoveToward` look
   identical on condition 1 but they are genuine `int` returns whose value is simply
   already in eax from the last callee.
3. A function with **no rel32 caller** is not a hit: it is a virtual reached through its
   vtable slot (the sieve prints the slot count), and the base class fixes the type.
4. Flipping the return type rewrites the mangled name (`QAEHHH@Z` -> `QAEXHH@Z`); update
   the header decl and every call site in the same change, then confirm the `RVA()` still
   binds by re-reading the per-function row from `gruntz sema match <unit>`.

A VALUE row the call graph rejects is still worth a look: the return type is right, but
we materialise a constant on an exit path retail does not have.

Sieve: `gruntz walls diagnose --asm` will usually flag these too, because the
spurious return value is what blocks the shrink-wrap.

related: [void-return-collapses-the-guard-ret.md](void-return-collapses-the-guard-ret.md),
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
