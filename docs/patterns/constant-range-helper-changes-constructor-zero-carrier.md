# A constant-range random helper can change an earlier constructor's zero carrier

tags: cpp:inline cpp:constructor cpp:local cpp:rand | asm:xor asm:test asm:mov | topic:regalloc topic:source-shape

A single `rand()` call does not prove that the original source wrote the
modulo expression directly. The empty-range arm of the already recovered
`GetRandom(lo, hi)` disappears when both bounds are constant. Its remaining
inline boundary can still affect register allocation earlier in the caller.

`CMenuSparkle::CMenuSparkle` (0xadbe0) exposes this after the action-ID adapter
correction. The first divergence is in its expanded base constructor, not
at the final random-delay assignment. Base creates `xor ebp,ebp`, tests the
registration flag against EBP, and carries that zero through calls into two
member-pointer stores and a later animation argument. Retail uses `test
eax,eax` and immediate zeros instead.

Controlled builds in the real TU:

| Delay source | Bytes | Instructions | Exact |
| --- | ---: | ---: | --- |
| `rand() % 0xfa1 + 0x3e8` | 369 | 99 | no, 97.04211% |
| Same expression using the existing 1000/5000 constants | 369 | 99 | no |
| Include `GameRand.h`, retain that expression | 369 | 99 | no |
| `GetRandom(g_menuSparkleLo, g_menuSparkleHi)` | 376 | 98 | yes |
| Retail | 376 | 98 | yes |

Every state has twelve calls, one branch, one return and the same 26 ordered
relocations. The constants are real four-byte retail `.rdata` objects at
0x1ea3d4 and 0x1ea3d8. Their definitions precede their uses so the compiler
folds the bounds; the serializer continues to reference the same objects.
The restart-delay site uses the same helper and bounds.

The include-only control rules out an inert declaration explanation. Reusing
the named constants alone is also insufficient. This is a call-boundary
effect on an earlier constant carrier, not a missing base-constructor store,
an EH reconstruction change, or proof of a numerical inline-budget deficit.

For reverse use, inspect constant-bound modulo expressions as well as the
variable-bound signature in [the random-range pattern](rand-modulo-peel.md).
Use the existing helper with its actual closed interval, retain the CRT
`rand()` source, and compare from the first divergence across the whole
caller. `GetRandomNumber()` is a different generator and is not a substitute.
