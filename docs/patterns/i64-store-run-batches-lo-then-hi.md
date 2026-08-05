# A RUN of 64-bit member zero-stores comes out all-lows-then-all-highs

tags: cpp:member cpp:struct cpp:union | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: N adjacent `m_x.m_v = 0;` (i64 / union with an `i64 m_v`) statements emit the N
low-half stores first and the N high-half stores after, but retail interleaves them in
groups of two (lo,lo,hi,hi) — every instruction and every offset otherwise identical
confidence: 9/10

cl5 does not emit a 64-bit store as an adjacent `mov [x],r; mov [x+4],r` pair when several
64-bit stores are adjacent: it BATCHES the whole run, emitting every low half and then
every high half. Four consecutive `m_v = 0` statements therefore come out

    28 30 38 40 | 2c 34 3c 44        (lo x4, then hi x4)

where retail has

    28 30 2c 34 | 38 40 3c 44        (two groups of lo,lo,hi,hi)

i.e. retail's run was only TWO 64-bit stores long twice over. Nothing about the i64
spelling reaches that grouping, so the residue looks like an unreachable scheduling
choice. **Write the two 32-bit halves and you pick the grouping yourself.**

```cpp
// NO - one batch of four; the four lows precede the four highs
m_baseTime.m_v = 0;
m_accum.m_v = 0;
m_startStamp.m_v = 0;
m_unusedStamp.m_v = 0;

// YES - byte-exact: two groups of two
m_baseTime.m_lo = 0;
m_accum.m_lo = 0;
m_baseTime.m_hi = 0;
m_accum.m_hi = 0;
m_startStamp.m_lo = 0;
m_unusedStamp.m_lo = 0;
m_startStamp.m_hi = 0;
m_unusedStamp.m_hi = 0;
```

`CTimer::Init` @0x0009bab0: 99.79 -> **100 EXACT**.

## What does NOT work (measured, so you can stop early)

All of these emit the identical lo x4 / hi x4 batch:

- chained assignment, either direction (`m_accum.m_v = m_baseTime.m_v = 0;`);
- an intervening 32-bit member store between the two pairs (it SINKS below the batch,
  so it does not cut the run — the eight i64 halves stay in one group of four);
- reordering the four statements.

The per-object natural half order (`lo,hi` per clock, i.e. `28 2c 30 34 …`) is emitted
verbatim and is a DIFFERENT wrong answer — the halves must be written lo,lo,hi,hi.
`m_accum.m_lo = m_baseTime.m_lo = 0; m_accum.m_hi = m_baseTime.m_hi = 0;` is byte-identical
to the four-statement form, so the bytes cannot arbitrate between them.

## Corollary

A run of 64-bit stores is a SCHEDULING UNIT. If a ctor/Init is byte-complete except that
its 64-bit clears are grouped differently from retail, do not look for a scheduling lever —
count retail's groups and write that many halves per group.
