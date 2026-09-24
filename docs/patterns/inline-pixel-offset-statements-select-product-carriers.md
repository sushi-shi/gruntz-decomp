# Sequence the real pixel offset inside its inline helper

tags: cpp:inline cpp:local cpp:expression | asm:imul asm:mov | topic:regalloc topic:codegen-idiom

`CWwdDotObject::BltDirty` (`0x1661d0`) had a 194-byte, 62-instruction
register/scheduling residue at 99.80645%. The first difference, at `+0x42`,
was inside the existing `CDDSurface::GetPixel` expansion: base loaded pitch
into ECX and bytes-per-pixel into EDX, whereas retail loaded bytes-per-pixel
into ECX and pitch into EDX. The corresponding multiplies exchanged carriers;
the rest of the caller agreed. Both sides had four calls, four branches, one
return and two relocations.

Keep the pixel helper and give its actual address calculation a statement
boundary. The controlled source change is entirely inside `GetPixel`:

```cpp
// A: one subscript expression
u8 color = bits[m_bytesPerPixel * x + m_apiDesc.lPitch * y];

// B: accumulate the actual byte offset, then read the byte
i32 offset = m_bytesPerPixel * x;
offset += m_apiDesc.lPitch * y;
u8 color = bits[offset];
```

B makes the complete normalized caller identical to retail, without changing
its size, calls, branches, return count or relocation count. The caller source,
the `PutPixel` helper, coordinate widths, successful-lock guard and byte result
held across `Unlock` are unchanged. There is no new API, escaped local, fake
storage, helper erasure or compiler-state padding. This is a source-plausible
local, not proof that the original author used its particular name.

The earlier recovery of `BltDirtyRegions` in commit `aa4441788` had moved this
unchanged caller from 100% to 99.80645%. Retaining those recovered argument
pointers is important: removing a correct neighboring source layer is not the
fix. See [the argument-pointer control](call-argument-pointer-locals-preserve-base-addressing.md).

Reverse use: when two products exchange carriers inside an otherwise matching
inline address calculation, inspect the helper's own local census and
statement grouping before flattening it into callers or probing unrelated TU
declarations. Test in its real consumers and compare from the first divergence.
This experiment does not establish that merely naming one complete expression,
commuting its terms, or every sequenced sum will have the same effect; those are
different controls, not results of this A/B.

The real second consumer, `CFaderRadial::ApplyInit` (`0x17fa40`), still has
its 499-byte / 171-instruction scheduling residue (five calls, twelve branches,
two returns and nine relocations); this is not a universal cure. The recovered
neighbor `BltDirtyRegions` remains identical at 307 bytes / 116 instructions.
The full-report raw-referent audit covers 3,919 functions at or above 99.5%
and finds no fake or wrong targets. These controls distinguish the recovered
caller from aggregate current-score movement caused by the shared header.

The first full build flagged sixteen fresh score dips. All sixteen retained
their own source fingerprint, call set and branch skeleton; review classified
them as allocation/scheduling movement, not lost implementations. In particular,
`Blit168` reordered the red/green product evaluation and its corresponding
shift loads, `Blit1624` exchanged the two byte carriers, and `CSBI_MenuItem::Render`
rescheduled receiver/coordinate loads without changing the outgoing arguments.
The snapshot can be refreshed while preserving the per-source and historical
maxima; a recovered target is not a claim that the aggregate current exact
count increased.

After that adjudication, the full `gruntz build` passes every gate. An explicit
RVA-keyed comparison preserves all 4,429 historical maxima. Current exacts
recover `BltDirty` and `CDDrawWorkerHost::Load`, while `Blit1624`,
`CBoomerang::AdvanceMotion` and `CSBI_MenuItem::Render` move below current
exact; the net current count is 3,829, down one. The independently retained
checkpoint helper composition changes that function's source fingerprint and
therefore resets its per-source best to its unchanged 98.3602%; its historical
100% proof remains intact. These are distinct measurements, not a new global
historical-MAX gain.
