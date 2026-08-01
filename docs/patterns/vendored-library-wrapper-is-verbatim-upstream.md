# An engine "wrapper" around a vendored library is that library's function VERBATIM

tags: cpp:branch cpp:return cpp:local | asm:mov asm:ret | topic:codegen-idiom topic:wall
symptoms: a small engine function that only sets up a library struct and calls three
  library entry points plateaus in the high 80s with the *body* byte-exact; the residual
  is an extra `mov <callee-saved>,eax` right after the FIRST library call, a redundant
  `mov esi,eax / mov eax,esi` pair at the tail, and one more `ret` than retail; every
  merged-exit / nested-`if` re-spelling of the same logic is flat or worse
confidence: 9/10

## Shape

The engine did not *write* these functions. It **pasted the library's own source** (or
compiled the library's own `.c`) and renamed it. So the thing to reconstruct is not
"logic that behaves the same" — it is the upstream file's **statement shape**, verbatim,
including the parts a decompiler-minded rewrite naturally "cleans up":

- the **early return** on the first error (`if (err != Z_OK) return err;`)
- the **ternary** for the second (`return err == Z_OK ? Z_BUF_ERROR : err;`)
- the pointless-looking **`err = f(); return err;`** at the tail instead of `return f();`

Each of those is load-bearing, because each one is what **splits the local's live range**
the way the original allocator saw it.

## Evidence

`WapUncompress` @0x1853b0 (`src/Wap32/WapUncompress.cpp`) is zlib 1.0.4's `compress()`
character for character. It had been parked `@early-stop` as a *"regalloc register-choice
wall — MSVC pins the long-lived pDestLen in ebx where retail uses edi… not
source-steerable"*. It was not a regalloc wall at all.

An 18-cell matrix over six body spellings x three struct-init orders:

| body spelling | score |
|---|---|
| **zlib's own `compress()`, verbatim** | **100.00 EXACT** |
| same but `return deflateEnd(&s);` at the tail | 95.56 |
| early-return init + nested if/else | 87.06 |
| fully nested `if (err == 0) { … }` + one trailing `return err` | 89.35 |
| that + the Z_BUF_ERROR ternary | 89.35 |

Read the three deltas — they are the whole lesson:

1. **Early return vs nesting the body** (89.35 -> the 100 cell). Nesting keeps `err` live
   across the whole function, so cl gives it a callee-saved register *immediately* and
   emits `mov esi,eax` right after `deflateInit_`. Retail has no such move: with the early
   return, the init result's live range **ends at the return**, `err` is redefined by
   `deflate`, and cl splits it — init result stays in `eax`, deflate result takes `esi`.
2. **The ternary** lowers to exactly retail's select
   (`mov eax,-5 / test esi,esi / je / mov eax,esi`); the `if (err == 0) { err = -5; }`
   statement form does not.
3. **`err = deflateEnd(&s); return err;` beats `return deflateEnd(&s);` by 4.4 points.**
   This is the counter-intuitive one. The "redundant" assignment is what stops cl
   emitting the `mov esi,eax / mov eax,esi` round trip — do not tidy it away.

## How to recognise it

Before treating a small function as a regalloc wall, ask whether it *is* library code
wearing an engine name. Tells:

- it only marshals a library struct and calls that library's entry points;
- `gruntz sema xref` gives it one caller, in engine code;
- the arg list matches an upstream signature (`(dest, destLen, source, sourceLen)`).

Then fetch the upstream source out of `vendor/` and transcribe it, statement for
statement, **before** trying any re-spelling. `vendor/zlib-1.0.4/compress.c`,
`uncompr.c`, `gzio.c` are all vendored here — `docs/zlib-matching.md` had listed
`compress.c` as "probably NOT linked", which is why nobody looked: it is not linked as
`compress.c`, it was *copied into the WAP32 module* and lives at 0x1853b0, far from the
zlib band at 0x188xxx.

## Bound

This is about *statement shape*, not about the library's identity being in a FID table.
It applies wherever a hand-copied library routine has been reconstructed from its
disassembly instead of from its source. It does NOT say "any function that calls zlib is
zlib" — check the caller and the signature first.

related: zlib-send-bits-macro.md, one-use-local-is-a-regalloc-knob.md,
positive-gate-enables-shrink-wrap.md
