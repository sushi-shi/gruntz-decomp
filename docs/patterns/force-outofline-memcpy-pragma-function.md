# Target `call _memcpy` where you emit `rep movs` — force the call with `#pragma function(memcpy)`
tags: cpp:builtin | asm:rep asm:call | topic:codegen-idiom
symptoms: retail does `call _memcpy` (a real out-of-line call, delinker names the rel32 `memcpy`) for a runtime-variable-size copy, but your recompile inlines it to `rep movsl; rep movsb`; the adjacent zero-fill is `rep stosd/stosb` in BOTH
confidence: 8/10

The companion pattern [[inline-mem-ops-rep-movs]] says MSVC5 /O2 /Oi inlines `memcpy`
to `rep movs`. That is the DEFAULT, but it is **not universal**: for some
runtime-variable sizes retail emitted an out-of-line `call _memcpy` instead. When
your block-copy inlines (`rep movsl`) but the target shows `call memcpy`, switch
that translation unit's `memcpy` from the intrinsic to the real library function:

```cpp
extern "C" void* memcpy(void* d, const void* s, u32 n); // reloc-masked engine extern
#pragma function(memcpy)   // disable the intrinsic -> emit `call _memcpy`
```

`#pragma function(memcpy)` (MSVC's counterpart to `#pragma intrinsic`) makes cl
emit a real `call` for every subsequent `memcpy`. It is **per-function selectable**:
the pragma applies from its point to end of TU (or until a matching
`#pragma intrinsic(memcpy)`), so place it so only the functions that need the call
form see it. `memset` is usually still wanted inline (`rep stos`) — do NOT add it to
the pragma; leave the zero-fill as a plain `memset(p,0,n)` / `int*` loop.

STEERABLE. Evidence: `_zvec::GrowTo` (0x16da80, the WAP32 dynamic-vector realloc):
the band-shift `memcpy((char*)p + shift*stride, p, oldbytes)` inlined to
`rep movsl/movsb` (62%); `#pragma function(memcpy)` recovered the retail `call memcpy`
and lifted it to 80% (residual = an unrelated this/arg regalloc wall). The two
zero-fills stayed `memset` → `rep stosd/stosb`, matching retail.
