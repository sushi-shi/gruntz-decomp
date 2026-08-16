# Two lockstep array cursors collapse into one index - declare them ABOVE the intervening call

**Tags:** cpp:local cpp:loop cpp:array cpp:global | asm:lea asm:sub | topic:codegen-idiom topic:regalloc

## Symptom

A loop walks two file-scope arrays in lockstep and reads one member from each.
Retail keeps **two independent pointers**, each pre-biased by the member offset:

```
lea    ebx,[esi+0x6a2d00]        ; &g_rasterEdgeL[minY].fx
lea    esi,[esi+0x685708]        ; &g_rasterEdgeR[minY].fx
...
mov    edx,DWORD PTR [esi]
mov    ecx,DWORD PTR [ebx]
add    esi,0x1c
add    ebx,0x1c
```

Ours collapses them onto one shared byte index, with a giveaway `lea`/`sub` pair
that builds the pointer and immediately subtracts the base back off:

```
lea    ebx,[4*eax + <DIR32 _g_rasterEdgeL>]
sub    ebx, <DIR32 _g_rasterEdgeL>          ; ebx = minY * 0x1c
add    ebx,0x10
mov    eax,[ebx + <DIR32 _g_rasterEdgeR>]
mov    ecx,[ebx + <DIR32 _g_rasterEdgeL>]
```

Both are correct and read the same field. The cheap tells that it is this and not a
wrong-member read: **the reloc COUNT is higher on our side** (the `lea`+`sub` pair
spends two references where retail's `lea` spends one - 23 vs 21 in `FillPolygon`),
and `gruntz verify assert-relocs` reports an addend mismatch on the array whose bias got
folded.

## Mechanism

cl's induction-variable simplification notices both cursors are affine in the same
basic IV and rewrites them as one index plus two `disp32` relocations, saving a
register. Retail's compiler did not, and the lever is **where the cursor is
declared relative to a call that sits between the index computation and the loop**.

Declared *inside* the guarded block, after the call, the whole address computation
happens there and cl is free to re-associate it into a shared index. Declared
*before* the call, each cursor must survive it in a callee-saved register, cl keeps
the multiplication live across the call (retail: `shl esi,0x2` before `Lock`, the
two `lea`s after the `jge`), and the loop-body shape falls out with it.

```cpp
// before - 68.33%
i32 stride = surf->m_pitch;
u8* bits = static_cast<u8*>(surf->Lock(0));
...
if (minYi < maxYi) {
    ClipVtx* pDesc = &g_rasterEdgeL[minYi];
    ClipVtx* pAsc = &g_rasterEdgeR[minYi];

// after - 76.52%
ClipVtx* pDesc = &g_rasterEdgeL[minYi];
ClipVtx* pAsc = &g_rasterEdgeR[minYi];
i32 stride = surf->m_pitch;
u8* bits = static_cast<u8*>(surf->Lock(0));
```

`FillPolygon` @0x146fe0 68.33 -> 76.52. A 14-cell placement matrix showed the axis
saturates: any position **above** the `Lock` scores 76.4-76.5 and any position
below scores 66.9-68.3, and the two orderings of the pair are worth nothing.

`WarpTextureBlit` @0x146a20 independently confirms the lifetime lever across two
`Lock` calls. Moving both edge cursors above the calls and rotating the row loops
to an explicit guard plus `do` raises the current-source result from 71.4791% to
71.7824%. It is only a partial break there: the same pair feeds three mutually
exclusive mode arms, and cl still folds the left cursor into an index in each
arm. The remaining signature is one extra `_g_rasterEdgeL` reference per arm,
not evidence for three different source arrays.

## What did NOT move it

Measured on the same function, all flat - do not re-walk them:

- the four commutative/statement spellings of the `(-topX - botX) / height`
  numerator (cl canonicalizes `-a-b` to `-(a+b)` and sinks the `neg` past the
  `idiv` regardless);
- hoisting `top`/`bottom` above the y-inequality test (74.8, worse);
- reordering the four function-top locals (four permutations, +/-0.7).

The residue after the hoist is register coloring - retail keeps `topX` in `edi`
across three `ftol` calls and lets `cur` die into its frame slot, ours holds `cur`
in `ebx` and spills `topX`.
