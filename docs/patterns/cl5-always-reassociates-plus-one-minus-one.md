# cl 5.0 always reassociates a `+1 … -1` pair away, and always folds the last `*p++`

tags: cpp:expression cpp:pointer cpp:local | asm:lea asm:add asm:dec | topic:wall topic:codegen-idiom
symptoms: retail computes `lea eax,[reg+1]; sub eax,m; dec eax` (or `add
reg,4; mov eax,[reg]`) where ours emits the folded `mov eax,reg; sub eax,m`
(or `mov eax,[reg+4]`), and the two sides otherwise agree instruction for
instruction
confidence: 9/10 (isolated cl 5.0 probes, six and four spellings, 2026-08-23)

Two folds cl 5.0 performs unconditionally. Both look like a source-shape
question and neither is one: no spelling of the expression reaches retail's
form, so a row whose only residue is one of these is PARKED, not steerable.

## 1. `(a + 1) - m - 1` collapses to `a - m`

Retail's shape, from `CGameLevel::ResolveFloorCollision` 0x15ede0:

```asm
lea  eax,[esi+0x1]        ; the "one past the last failing tile"
mov  edx,DWORD PTR [ecx+0x140]
sub  eax,edx
dec  eax
```

Six spellings compiled with cl 5.0 `/O2 /MT`, all producing the identical
folded `mov eax,esi; sub eax,edx`:

```cpp
return y + 1 - t->ext.bottom - 1;          // inline
int hi = y + 1;  return hi - b - 1;        // named temp
return y + 1 - (t->ext.bottom + 1);        // constant moved right
int hi = y+1, d = hi - b;  return d - 1;   // split statements
++y;             return y - b - 1;         // mutate the variable
int hi = y + 1;  return hi - 1 - b;        // constants adjacent
```

C2 collects the constants across the whole `+`/`-` chain, so the only way to
keep the pair is for the `+1` side to be OPAQUE — a genuinely live variable
whose value C2 cannot see. Retail's *is* opaque, which means retail
rematerialised a spilled loop-carried variable as `esi + 1`; three probes at
matching register pressure (a call in the loop, four callee-saved values live)
show cl 5.0 spilling it instead, never rematerialising. Both the fold and the
missing remat are C2 decisions with no source handle.

Practical consequence: keeping the loop-carried variable scores HIGHER than
removing it, because the removed version also lets C2 cross-jump the two
now-identical exits and lose a whole block.

## 2. The penultimate `*p++` folds into the final read's displacement

Retail, `CImageSet2::Parse` 0x166990 — every bump materialised except the last:

```asm
mov edx,[eax]  /  add eax,0x4  /  mov [ecx+0x1c],edx
mov eax,[eax]                                       ; final read, no bump
```

Ours folds one bump further, `mov eax,[eax+0x4]`, costing exactly one
instruction (86.67% on the 0x24-byte `CImageSet1::Parse`, 94.07% on
`CImageSet2::Parse`). Four spellings of the walk are byte-identical under cl
5.0 — `*p++` throughout, a final `*p` with no bump, `++p` then `p[-1]`, and
`*p; p++;` split into two statements — so the walk's spelling is not the
lever. Do not rewrite `READ_TILE_IMAGE_DIMENSIONS`-style macros chasing it.
