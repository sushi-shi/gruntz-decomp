# `movzx cx,cl` instead of `and ecx,0xff`: the DESTINATION width picks the widen
tags: cpp:expr cpp:local cpp:cast | asm:movzx asm:and asm:mov | topic:codegen-idiom topic:mis-model
symptoms: retail widens byte terms with the 66-prefixed `movzx <16>,<8>` (`movzx cx,cl`,
`movzx dx,dl`) where the base emits 32-bit `and r,0xff` / `and r,0xf`; the base has ONE
`movzx` — the term nearest the store — where retail has one per term
confidence: 8/10 (controlled three-way A/B on CShadeTableCache::GreyTable, 2026-08-23)
variants: spilled-dword-with-byte-reads-is-a-struct-local.md,
sequenced-accumulator-beats-or-tree-canonicalization.md

## Symptom

`CShadeTableCache::GreyTable` @0x0014eef0 packs a 16-bit grey index per 16-bit colour.
Retail's loop:

```asm
mov  ecx,eax  ; mov edx,eax
sar  ecx,0xb  ; sar edx,0x6
and  dl,0xf
movzx cx,cl                      ; 66 0f b6 c9 - FOUR bytes
movzx dx,dl
shl  ecx,0x4
add  edx,ecx
mov  ecx,eax ; sar ecx,1 ; and cl,0xf
movzx cx,cl
shl  edx,0x4
add  edx,ecx
mov  WORD PTR [esi-0x2],dx
```

The tree's `i32`-accumulator spelling emits 32-bit masks instead, and only one `movzx`:

```asm
and  edx,0xff                    ; the (u8) cast of v >> 11
and  eax,0xf
```

## Mechanism

`movzx cx,cl` is **longer** than `movzx ecx,cl` (4 bytes against 3), so cl is not picking
it to save space — it is picking it because the value it is producing is a **16-bit**
quantity. The final store is `mov WORD PTR [esi-2],dx`: only the low 16 bits of the
accumulator are ever read, so cl maintains exactly 16 valid bits through the whole
expression. A byte operation (`and dl,0xf`, or just using `cl` after a `sar`) leaves bits
8..15 dirty, which WOULD corrupt those 16, so each term gets a byte-to-word widen — and
nothing wider is needed, which is why there is no `and r,0xff` anywhere.

Introduce an `int` intermediate and the 16-bit context is gone: every term now has to be a
valid 32-bit value, so the `(u8)` cast lowers to `and r,0xff` and only the term feeding the
`(u16)` store keeps a widen.

```cpp
i32 acc = static_cast<u8>(v >> 0xb) << 4;                    // 32-bit context
acc = (acc + static_cast<u8>((v >> 6) & 0xf)) << 4;          // and edx,0xff
*out++ = static_cast<u16>(acc + static_cast<u8>((v >> 1) & 0xf));

*out++ = static_cast<u16>(                                   // 16-bit context
    (((static_cast<u8>(v >> 0xb) << 4) + static_cast<u8>((v >> 6) & 0xf)) << 4)
    + static_cast<u8>((v >> 1) & 0xf));                      // movzx cx,cl x3
```

## Measured, and the cost that is NOT reachable from the source

Three spellings, one full build each, same TU:

| spelling | loop texture | objdiff |
|---|---|---|
| `i32 acc`, three statements (in the tree) | 1 `movzx`, `and edx,0xff`, `and eax,0xf` | 92.86 |
| `u16 r/g/b` locals + `i32 acc` fold | unchanged from the above | 92.71 |
| one expression stored to the `u16` lvalue | retail's multiset exactly | 80.76 |
| `u8 r/g/b` locals, one store expression | retail's multiset exactly | 83.67 |

The last two reproduce retail's loop instruction-for-instruction under register stripping
(`walls residue` drops the row from `immediate` to a frame-only `selection` residue), and
still score twelve points LOWER, because cl then needs a fourth callee-saved register:
retail writes `movzx cx,cl` with src == dst and keeps the loop in `eax/ecx/edx/esi`, while
ours writes `movzx si,bl` — the byte source and the word destination in DIFFERENT
registers — because the scheduler hoists the third term's `mov ebx,eax` above the
`add edx,ecx` that would have freed the temp. That is a `push ebx` retail does not have
(4 pushes against 3) and a different epilogue.

So the 16-bit context is the right READ of retail's source and the wrong thing to ship at
this site; the tree keeps the higher-scoring `i32` spelling and the row is parked.

## Reverse use

* A base-only `and r,0xff` / `and r,0xffff` facing a retail `movzx <16>,<8>` says the
  retail expression's destination is **16 bits wide**. Look for an `int` intermediate to
  remove, or a temp to retype — do not read it as a wrong mask.
* The 32-bit twin is the same tell inverted: `CDDrawShadeBlit::BlitShadedForward`
  @0x0014a200 has `and eax,0xffff` on a value cl had ALREADY zero-extended
  (`xor eax,eax; mov al,[ecx]; mov ax,WORD PTR [ecx+eax*2]`). A redundant mask is cl
  materializing a `u16` variable as an `int`, so that temp is `u16` in the source.
* `movzx cx,cl` with src == dst means the term is consumed before the next term is
  started; src != dst means the scheduler started the next term early, which costs a
  register. If the shape is right and the score is not, check the push count first.
