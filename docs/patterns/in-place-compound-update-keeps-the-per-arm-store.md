# A local copy written back merges the arms' stores; the in-place compound update keeps them

tags: cpp:local cpp:if cpp:pointer | asm:add asm:sub asm:mov asm:jmp | topic:codegen-idiom topic:regalloc
symptoms: `walls diagnose` says REGALLOC with equal calls/branches/rets; retail stores the
result inside EACH arm of an `if/else if` and jumps past the join, while ours computes both
arms into ONE register and shares a single store at the join (our `jmp` targets the store,
retail's jumps over it)
confidence: 9/10

## Symptom

`CDDrawWorkerHost::WrapCoord` 0xa000, one axis of a two-axis wrap:

```
 ours                              retail
 14: test eax,eax                  14: test eax,eax
 16: jge  0x1c                     16: jge  0x1e
 18: add  eax,edx                  18: add  edx,eax     ; result in the WRAP register
 1a: jmp  0x22   -> the store      1a: mov  [ebx],edx    ; ... stored HERE
 1c: cmp  eax,edx                  1c: jmp  0x26         ; ... and jumps PAST the join
 1e: jl   0x24                     1e: cmp  eax,edx
 20: sub  eax,edx                  20: jl   0x26
 22: mov  [ebx],eax  ; ONE store   22: sub  eax,edx
 24: (join)                        24: mov  [ebx],eax    ; second, distinct store
                                   26: (join)
```

## Cause

The source was

```cpp
LONG x = *px;                       // a local COPY of the caller's coordinate
if (x < 0)            *px = m_wrapW + x;
else if (x >= m_wrapW) *px = x - m_wrapW;
```

Both arms assign the same lvalue from a value derived from one local, so the global
optimizer factors the two assignments onto a single temp with a phi. Once they share a
temp, the non-commutative arm (`x - m_wrapW`) pins that temp to the coordinate's register,
which forces the other arm to `add eax,edx` as well — and the two tails become the
byte-identical `mov [ebx],eax`, so they also cross-jump.

Retail's arms end in `mov [ebx],edx` and `mov [ebx],eax`. Different registers, therefore
no shared temp, therefore no merge. Writing the fixups as compound updates OF THE POINTER,
with no local copy, reproduces that:

```cpp
if (*px < 0)             *px += m_wrapW;
else if (*px >= m_wrapW) *px -= m_wrapW;
```

**Measured: `CDDrawWorkerHost::WrapCoord` 0xa000, 91.49 -> 100.0000 EXACT**, both axes, in
one change. Swapping the add's operand order (`x + m_wrapW` vs `m_wrapW + x`) is byte-flat
and does NOT reach it — cl canonicalizes commutative operands, so the operand order in the
source is not the lever; the presence of the LOCAL is.

## How to spot it

Read the two arms' STORES, not their arithmetic. Ours share one register and one store
site; retail's use different registers and store twice. That is the factoring, and it is
upstream of the cross-jump: `cl5-crossjump-merges-suffixes-not-blocks.md` describes the
merge, but with distinct registers the tails are not identical and the merge can never
fire. When the assigned lvalue is reached through a pointer or reference parameter, drop
the local copy first.

## Bounds

Measured 2026-08-22, pinned cl 5.0 SP3, `/O2 /MT /GX /GR`, on the real TU
(`src/Gruntz/ActionOptionsMenuBar.cpp`). The inverse direction (introducing a local to
FORCE a merge) is untested.

related: cl5-crossjump-merges-suffixes-not-blocks.md,
guard-result-zero-per-arm-not-an-initializer.md
