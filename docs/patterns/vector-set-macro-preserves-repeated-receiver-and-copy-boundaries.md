# Vector SET macros preserve repeated receivers and raw double copies

tags: cpp:macro cpp:inline cpp:aggregate cpp:double | asm:lea asm:mov asm:fld asm:fstp | topic:source-shape topic:aliasing
symptoms: member Init caches a receiver that retail recomputes, or turns repeated global double copies into floating-point argument temporaries
confidence: 10/10

## Complete caller control

`CGrunt::Activate` initializes direction and step vectors for nine cells. Retail
reloads each direction's row/column globals separately before every coordinate
store. The original expanded reconstruction is fully exact: 1,508 bytes and 75
ordered references, including the three double constants.

Replacing all eighteen pairs by the existing `DoubleVector2::Init` makes a
960-byte caller at 48.8303%, with only 39 references. The first divergence is
the receiver-address calculation: the inlined member retains one address for
both components. This is not merely a later register rotation. The double
fields, source constants, arithmetic and existing tail are unchanged.

A SET-style macro repeats its receiver expression for each typed member store.
The dimension/width-adapted `VEC2_SET` restores the entire normalized caller and
all 75 references; all 46 scored TU bodies are identical to the original
baseline. The shared member Init remains available for ordinary receivers.

This is an adaptation of the released vector-macro family, not recovery of an
original Gruntz macro name. Do not copy its float casts into a double owner.
Canonical provenance and scope are `reassess-vector-activate-macro` and
`reassess-vector-set-float-narrowing` in the lineage ledger.

## Constructor composition

The same boundary matters in `CMotionState::InitBounds`. The previous six
member Init uses leave the standalone constructor at 69.9750%: 340 bytes and 20
references versus retail's 388 bytes and 24 references. Its first divergence
is the ordering of low/high dword zero stores across adjacent vectors; later
the two maximum-vector initializations use FP argument copies where retail
loads and stores integer halves separately for each coordinate.

| Complete source composition | Constructor score | Bytes | References |
| --- | ---: | ---: | ---: |
| Existing six Init uses, alternating bound fields | 69.9750 | 340 | 20 |
| Also group minimum/maximum into two Init calls | 39.8250 | 295 | 16 |
| Six original sites use VEC3_SET, bound pair still Init | 71.7250 | 343 | 20 |
| Six VEC3_SET sites, original alternating bound fields | 100 | 388 | 24 |

The macro base restores the original zero-prefix scheduling and repeated
integer copies. Composing the independently observed alternating bound stores
then restores the complete constructor. No helper was flattened to retain a
score, and no unused declarations, forced emitters or compiler-state probes
were added. This recovers an existing historical exact match, not a new
historical maximum. The bound-pair experiment remains a narrowly recorded
alternative, not proof that every vector operation must be a macro.

The full rebuild also recovers the derived `CMovingLogic` and `CProjectile`
constructor primary bodies, from 76.8317% and 47.1132% respectively to 100%.
These too were historically exact. The focused original-byte regression below
covers Activate and the standalone motion constructor; it is not an audit of
the derived owners' separate exception-handler bands.

## Reverse use and limits

First compare the full baseline with retail. A missing receiver reload can
distinguish a member call from repeated-lvalue source even when all values are
mathematically identical. Likewise, a double-valued argument creates a different
front-end boundary from a raw component assignment. Test the actual family,
including computed components, not just constant-zero examples.

Macros repeat receiver evaluation; effectful receivers require separate
semantic review. The retained sites use ordinary member/global expressions,
not incrementing indices or calls. Preserve double widths and verify raw
relocation addends and repeated-reference multiplicity. Matching a normalized
reference set alone would miss half of Activate's direction lookups.

`scripts/test_activate_vector_helpers.py` checks both complete production owners
against original executable bytes and all 99 ordered references. Negative
controls cover wrong direction/bound identities, high-word addends, narrowed
FP stores, writes into an untouched vector, and missing repeated fixups with
unchanged bytes and deduplicated target sets. A pinned compile-only header
probe checks both layouts, member Init signatures/defaults and the shared macros.

This does not close the broader vector/rectangle declaration and consumer
reviews. In particular, member constructors, arithmetic/assignment boundaries,
Spotlight users and Query's rectangle initialization remain separate candidates.
