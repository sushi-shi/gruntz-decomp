# A zero-store GROUP after a call is a LOOP — that is what gives it its own `xor eax,eax`
tags: cpp:loop cpp:array cpp:ctor | asm:xor asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: base pushes ONE extra callee-saved register (`push edi`/`pop edi`) and pins the constant 0 in it; retail emits `xor eax,eax` TWICE — once before a call and once after it — and pushes one register fewer; everything else byte-identical
confidence: 9/10

The classic "retail pins 0 in a callee-saved register, we don't" note
([`zero-register-pinning.md`](zero-register-pinning.md)) has an **inverse that IS
source-steerable**, and it is the more common one in ctor/reset bodies:

```
base:                                retail:
  push edi                             (no extra push)
  xor edi,edi                          xor eax,eax
  mov [esi+0x00],edi  ... x8           mov [esi+0x00],eax ... x8
  call ClearCmds                       call ClearCmds
  mov [esi+0x3c],edi                   xor eax,eax          <- SECOND zero
  mov [esi+0x40],edi                   mov [esi+0x3c],eax
  ...                                  ...
```

`eax` is caller-saved, so a zero constant that must survive a call has to move to a
callee-saved register — and cl pays a `push edi`/`pop edi` plus a frame shift for it. Retail
does **not** pay that, because in retail the two zero groups are *two different constants*:
the pre-call group and the post-call group each get their own `xor eax,eax`.

What splits them is the **shape of the second group in the source**. Four constant-index
member stores

```cpp
m_ackFlags[0] = 0; m_ackFlags[1] = 0; m_ackFlags[2] = 0; m_ackFlags[3] = 0;
```

are folded into the same constant node as the eight stores above them. Writing the group as
a **loop** gives it its own:

```cpp
for (i32 i = 0; i < 4; i++) {
    m_ackFlags[i] = 0;
}
```

cl unrolls the four-iteration loop into the same four `mov [esi+N],eax` stores — the *bytes*
of the group are unchanged — but it materialises the fill value separately, which removes the
cross-call live range, the `push edi`/`pop edi`, and the frame shift that cascades from it.

Same family as [`adjacent-same-value-stores-are-a-loop.md`](adjacent-same-value-stores-are-a-loop.md):
a run of same-value stores is a loop in the source, and cl's fill expansion is what proves it.
When the run is over a real **array member**, `memset(arr, 0, sizeof(arr))` goes one step
further and materialises a strength-reduced **row pointer** (`lea edx,<base>` + `[edx]`,
`[edx+4]`, …) — which is how you read array-vs-scalar members straight off a ctor: retail
zeroes scalars with the pooled constant and each array with a fresh zero plus its own `lea`.

## Evidence

`src/Gruntz/Multi.cpp` (2026-07-29) — `CNetCmdSlot::CNetCmdSlot` @0x0bbec0 was filed
"zero-register-pinning wall (78.8%) … not source-steerable"; the ack-flag loop took it to
**100% EXACT** with no other change. `CNetSession::ResetAll` @0x0bbf80, which inlines the same
slot reset, went 75.20 → **79.91** from the identical fix.

`src/Gruntz/Grunt.cpp` / `include/Gruntz/Grunt.h` — `CGrunt`'s ctor @0x47a10 zeroes 0x394,
0x3a0 and 0x3c0 with the pooled `edi` but emits a `lea` row pointer + a fresh `xor eax,eax`
for [0x398..0x39c], [0x3a4..0x3a8], [0x3ac..0x3bc], [0x3d0..0x3d4] and [0x3c4..0x3cc]. Those
five runs are five ARRAYS (`m_poseAttack[2]`, `m_poseStruck[2]`, `m_poseIdle[5]`,
`m_poseItem[2]`, `m_poseToy[3]`) that had been modelled as fourteen numbered scalars — and
four call sites were already reaching past them with `(&m_poseToy1)[i]`.
