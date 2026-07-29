# Building a rect in a LOCAL and publishing it with `member = local` — the `lea` + the second zero

tags: cpp:struct cpp:local cpp:member | asm:lea asm:xor asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail dedicates a 4th callee-saved register to `lea ebx,[this+off]` for a member
aggregate it only STORES to, materialises the constant 0 TWICE (`xor eax,eax` + `xor ecx,ecx`)
and uses `test r,r` guards, where the recompile writes `[this+off+N]` directly, pins one zero
register and derives its guards from it (`xor;cmp`)
confidence: 9/10

## Symptom

A function fills the same box into a member aggregate AND a local one, then passes the
local on:

```asm
; retail
lea  ebx,[esi+0x10]         ; &this->m_planeCtx  - a whole callee-saved reg for a STORE target
xor  eax,eax
xor  ecx,ecx                ; the zero, a SECOND time
dec  edx
mov  [ebx],eax   / mov [esp+0xc],eax
dec  edi
mov  [esp+0x18],edi
mov  [ebx+0x4],ecx / mov [esp+0x10],ecx
mov  [esp+0x14],edx / mov [ebx+0x8],edx / mov [ebx+0xc],edi
```

Writing the eight fields as interleaved `m_planeCtx.left = 0; rect.left = 0; …` statements
produces neither the `lea` nor the second zero: cl addresses the member directly off `this`
and pins ONE zero register, which then also supplies the `w <= 0` / `h <= 0` guards as
`cmp r,zero` instead of retail's `test r,r`. Every interleaving of the eight stores keeps
that shape.

## The fix

Fill only the LOCAL, then publish it with a whole-struct assignment:

```cpp
LevelCoordRect rect;
rect.left = 0;
rect.top = 0;
rect.right = maxX;
rect.bottom = maxY;
m_planeCtx = rect;          // <- this is what emits `lea ebx,[esi+0x10]`
```

The assignment is a copy whose DESTINATION address is a value cl keeps in a register, and
the copy is its own expression, so its zero source is a separate constant from the local's.
With no zero left over to pin, the two positive-extent guards fall back to `test`.

## Evidence

`CGameLevel::SetExtentsAndBuildAll` @0x15d700 — **68.29 % → 100 % EXACT**, having been filed
a "regalloc/zero-pin wall … not steerable on a function this small". Fourteen orderings of
the interleaved-store form, a `LevelCoordRect&` alias and a `LevelCoordRect*` local were all
byte-identical to the baseline; only the struct assignment moved it.

Counter-check: the sibling `CGameLevel::SetCoordExtents` @0x15d030 writes the member rect
with NO local involved (retail stores the four fields straight through `ecx`), and forcing a
local + assignment there costs 10 points. Read the `lea` — it is the tell.

## Related

* [[member-aggregate-copied-not-field-by-field]] — the READ direction of the same idiom.
