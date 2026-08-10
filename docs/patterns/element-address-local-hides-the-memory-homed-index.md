# A `T* p = &arr[i];` local hides retail's memory-homed index — index the array at every use

tags: cpp:local cpp:member cpp:expr | asm:and asm:lea asm:mov | topic:codegen-idiom topic:regalloc

symptoms: retail stores an index BYTE into a stack slot and reloads it as a dword with
  `mov ecx,[esp+N]; and ecx,0xff` before each `lea r,[base+ecx*4]`, where base keeps the
  index zero-extended in a register (`xor eax,eax; mov al,[p]`) and computes the element
  address ONCE; retail's frame has one more live value and one fewer dead parameter home

confidence: 9/10

variants: cached-member-pointer-local-pins-a-register.md,
spilled-local-vs-rematerialised-indexed-member.md

The sibling of
[`cached-member-pointer-local-pins-a-register.md`](cached-member-pointer-local-pins-a-register.md):
that one is about caching `m_member`, this one about caching an ARRAY ELEMENT ADDRESS.
The cost is bigger, because the local does not merely pin a register — it kills the
index's live range, and with it the *type* of the index's home.

```cpp
// base: the index dies at the lea, so cl zero-extends it in a register and never
//       homes it; the second index is then rematerialised by re-loading *b AFTER
//       the intervening call, which reshuffles the whole frame
u8 ia = *static_cast<const u8*>(a);
u8 ib = *static_cast<const u8*>(b);
PALETTEENTRY* pa = &g_pal[ia];
u8 la = ... pa->peBlue ... pa->peGreen ... pa->peRed ...;
PALETTEENTRY* pb = &g_pal[ib];
u8 lb = ... pb->peBlue ... ;

// retail: the index is live across every channel read, so cl gives it a home,
//         stores it as a BYTE and widens it per use
u8 ia = *static_cast<const u8*>(a);
u8 ib = *static_cast<const u8*>(b);
u8 la = ... g_pal[ia].peBlue ... g_pal[ia].peGreen ... g_pal[ia].peRed ...;
u8 lb = ... g_pal[ib].peBlue ... g_pal[ib].peGreen ... g_pal[ib].peRed ...;
```

```asm
; retail: both indices read at entry and homed as bytes in the dead parameter slots
mov  cl,BYTE PTR [eax]          ; *a
mov  al,BYTE PTR [edx]          ; *b        <- both read BEFORE the first call
mov  BYTE PTR [esp+0x28],cl     ; ia -> a's own parameter home
mov  ecx,DWORD PTR [esp+0x28]
mov  BYTE PTR [esp+0x2c],al     ; ib -> b's own parameter home
and  ecx,0xff
lea  eax,[edx+ecx*4]

; base with the `pa`/`pb` locals: ia stays in a register, `b` stays a POINTER in its
; home, and *b is re-loaded after the call
xor  eax,eax
mov  al,BYTE PTR [ecx]
lea  eax,[esi+eax*4]
...
mov  ecx,DWORD PTR [esp+0x10]   ; b, still the pointer
mov  al,BYTE PTR [ecx]          ; *b re-loaded here, not at entry
```

## The rule

Where the target re-derives `base + index*scale` at every field read, the source has
**no element-address local** — it subscripts the array at each use. The `p->field`
spelling is not equivalent: it converts a long-lived index into a long-lived pointer,
and cl then homes a pointer where retail homed a byte.

The two shapes are trivially distinguishable in the diff: retail's `and reg,0xff`
against a stack slot IS the memory-homed narrow index; a register-only
`xor r,r; mov rl,[p]` is base's.

Do NOT generalise this into "never write a pointer local" — the inverse case exists:
`CShadeTableCache::HueRampTable` @0x14e830 keeps `PALETTEENTRY* p = &pal[i];` in the
i-loop and dropping it costs 2.1 points and 2 bytes of size. Read whether the target
re-derives the address per field (drop the local) or computes it once per outer
iteration (keep it).

Evidence: `CShadeTableCache::CompareLuma` @0x14ed10 70.76 -> **98.08** and
`CShadeTableCache::CompareHue` @0x14fa60 86.71 -> **99.99** on dropping `pa`/`pb`
alone (both then closed to 100.00 with one more fix each);
`CShadeTableCache::GammaTable` @0x14e9f0 91.12 -> **94.07** on dropping `pr`/`pc`
(a nested loop over `pal[i]`/`pal[j]`, where keeping only one of the two locals scores
between the extremes: `pr` only 91.82, `pc` only 93.29, neither 94.03).
