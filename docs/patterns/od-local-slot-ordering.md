# /Od local stack-slot ordering differs: instruction stream identical, `[ebp-N]` displacements permuted

**Tags:** cpp:local | asm:mov | topic:wall topic:regalloc
**Confidence:** 6/10

## Symptom

A function retail compiled **unoptimized** (`/Od`, full `push ebp; mov ebp,esp;
sub esp,N` frame, every local through memory) recompiles with the **exact same
instruction stream** — same mnemonics, same operands, same order, same frame size
— yet objdiff shows ~85% *byte* match (but ~99.5% *fuzzy*, because fuzzy scores
mnemonic+operand-shape, not the displacement immediate). The **only** difference
is the `[ebp-N]` displacement on every local access: retail lays the locals out
**sequentially in source declaration order** (first local `-4`, next `-8`, …),
while the recompile assigns the same locals to a **permuted** set of the same
slots.

Confirmed on the CFileImage `/Od` RLE decoders (DIRSURF.CPP): DecodeRun8
(0x140aa0), DecodeRun24 (0x140c50), RunDecode1 (0x145270), RunDecode3 (0x1453f0).
All four reproduce retail's instruction stream byte-for-byte and plateau ~85%
byte / 99.5% fuzzy purely on the slot permutation.

## Why

MSVC 5.0 `/Od` assigns each local a fixed `[ebp-N]` slot. Retail's slots come out
in a clean monotone run that matches one specific source declaration/scope order.
Our recompile, even with the **identical** flat declaration order at function
scope, produces a different (stable, deterministic) permutation. The permutation
is **not** a pure function of declaration position — reordering the declarations
to the observed inverse permutation only nudges it (e.g. RunDecode1 86%→88%), so
it is usage/scope-sensitive, not position-sensitive.

The residual is therefore driven by where in the **lexical scope tree** retail
declared each local (function scope vs `for`-init vs loop-body block scope), which
the recovered code does not reveal — the generated instructions are identical
regardless of scope, so the scope nesting cannot be inferred from the disassembly.

## Not steerable (from the disasm alone)

- `/Od` vs `/Od /Oi` (`od` vs `odi` profile) makes **no** difference — `/Oi` only
  affects recognized intrinsics, not local layout.
- Reordering the flat function-scope declarations to the inverse of the observed
  permutation moves a few slots but does not converge (the permutation shifts).
- The fix would require reconstructing retail's exact lexical scoping of each
  local (which block each was declared in), which is unobservable from the
  identical instruction stream.

A later sweep with a scope-search tool (try every {function-scope, for-init,
loop-body} placement per local until the slot run matches) could close these.
Until then: reconstruct the full correct body, confirm the instruction stream is
byte-identical (so it is purely the slot permutation), and `@early-stop`.

## Distinguish from

- [`stack-slot-coalesce-frame-4b`](stack-slot-coalesce-frame-4b.md) — that is a
  *frame-size* difference (a coalesced/extra slot) under `/O2`; here the frame
  size is identical and only the slot-to-local mapping is permuted under `/Od`.
- [`o2-optimizer-bailout-framed`](o2-optimizer-bailout-framed.md) — that is about
  `/O2` *emitting* an unexpected framed body; here the body is genuinely `/Od`.
