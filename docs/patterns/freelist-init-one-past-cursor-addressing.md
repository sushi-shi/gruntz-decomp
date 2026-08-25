# Free-list init loops where retail stores through the one-past cursor - suspected inlined link-helper

tags: cpp:idiom msvc5:c1-inline | asm:lea asm:neg-disp | topic:wall topic:method
symptoms: an array-init linking loop scores mid-80s; retail addresses the
current element's stores as NEGATIVE displacements off the NEXT cursor
(`mov [eax-0xc],...` where eax = cur+stride) while ours addresses them
cur-relative (`mov [eax+0x18],...`); compare/value arithmetic still uses cur
confidence: mechanism proven by A/B; the source construct is UNMAPPED

## The composed A/B ladder (CBrickzNodePool::Allocate 0x9e740, 2026-08-22)

| spelling | score |
|---|---|
| hoisted two-cursor walk (`next` outside, `++e; ++next;`) - banked | 87.52 |
| per-iteration `next = e + 1; ... e = next;` (single-cursor tail) | 84.72 |
| `e->m_openNext = e + 1;` inline, no next local (IV hypothesis) | 66.64 |
| `e = m_freeList` + stores through `next[-1]` (COMPOSED probe) | **95.80** |

The composed probe is the exploratory-descent production: each lever is
INVISIBLE alone. `e = m_freeList` (a member read-back of the value just
stored) compiles to retail's `mov ecx,eax` copy ONLY when the `next[-1]`
stores are also present - alone, SP3 copy-propagates it away to byte-
identical. Together they reproduce retail byte-for-byte except FOUR
instructions: the zero-trip bound load homes in edi (retail: eax, a dying
transient the next lea overwrites) and the `next` lea hoists above the
guard (retail: below it). SP3's IV creation does not fire on the inline
`e + 1` spelling (66.64), so retail's leading-cursor addressing plus its
weaker copy-prop is an ERA-COMPILER trait (RTM family), not a reachable
source shape: every humane spelling of stores through the leading cursor
must be written `next[-1]`, which no dev writes (user ruling).

## Disposition

Parked at the banked humane spelling (87.52). The 95.80 composed state is
evidence, not source. Sibling with identical state: CBrickzCellNodePool::Allocate
0x9e860 (87.65 banked). If the RTM toolchain lands (provenance test), retry
the banked spelling under it before any further source work. The earlier
inlined-link-helper hypothesis is DEMOTED: the composed probe explains the
texture without a helper.
