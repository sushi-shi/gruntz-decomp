# Retail's `dec ctr / jne` loops are a COUNTER + a CURSOR, not an indexed loop
tags: cpp:loop cpp:pointer | asm:dec asm:jne asm:cmp asm:jl | topic:codegen-idiom
symptoms: retail `add <val>,K / dec <ctr> / jne`, ours `inc i / cmp i,N / jl`; jcc sieve reports the back-edge as `jl->jne`
confidence: 9/10

Retail loop back-edges over arrays and quantization ramps look like this:

```asm
mov  ecx,[esp+0x14]      ; the cursor
mov  eax,[esp+0x10]      ; the trip counter
add  ecx,0x4             ; cursor += stride
dec  eax                 ; counter--
mov  [esp+0x14],ecx
mov  [esp+0x10],eax
jne  <top>
```

Two live variables: a **trip counter counted down to zero** and a **value/pointer cursor
advanced by the stride**. An indexed loop cannot produce it — `for (i = 0; i < N; i++)`
with `arr[i]` in the body gives `inc i / cmp i,N / jl` and re-forms the address every
pass. Write what retail has:

```cpp
// NO - one indexed variable:
for (i32 n = 0; n < 7; n++) { ... m_frames[n] ... }
for (i32 r = 8; r < 0x100; r += 0x10) { ... r ... }

// YES - a plain counted loop plus a cursor advanced in the body:
CAniElement** fp = m_frames;
for (i32 fi = 0; fi < 7; fi++) { ... *fp ... ; fp++; }

i32 r = 8;
for (i32 ri = 0; ri < 0x10; ri++) { ... r ... ; r += 0x10; }
```

**Both halves are required, and there are two ways to lose the reversal:**

1. **Folding the cursor back into the counter.** `i32 r = ri * 0x10 + 8;` *inside* the
   loop does NOT work: cl strength-reduces the multiply into an induction variable and
   then re-derives the trip test from it, landing back on `cmp val,0x100 / jl`. The
   value must be a variable declared OUTSIDE the counted loop and advanced in the body.
2. **Keeping the counter live past the loop.** cl reverses a loop only when the counter
   is dead after it. Re-using the loop variable for anything afterwards (`n` doubling as
   a serialized id) pins the increment direction. Give the counter its own name.

MEASURED (both 2026-08-01, both closed the sieve's `jl->jne` row and made the branch
sequences agree):
- `CShadeTableCache::AddTable` 0x14f080 — three colour loops — **74.23% -> 84.04%**
- `CProjectile::SerializeDispatch` 0xe0d40 — the 7-frame write loop — **91.62% -> 94.34%**
  (cursor alone: 93.09%; cursor + separate counter: 94.34%)

Found with `gruntz walls diagnose <rva> --asm`: the back-edge is a `jl` -> `jne`
mnemonic flip, which an address-masked comparison cannot show because it masks the
branch displacement.
