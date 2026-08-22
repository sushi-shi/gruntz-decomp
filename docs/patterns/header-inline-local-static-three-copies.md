# Three retail copies of one header-inline's local static - the linkage trichotomy, probe-proven

tags: cpp:inline cpp:static cpp:linkage msvc5:common | topic:method topic:identity
symptoms: one utility function (CREDITZ-printed GetRandomNumber) but the
image holds THREE guard/seed pairs with a caller partition exactly on
module boundaries (game band / wwd lib / fader lib)
confidence: 10/10 for the mechanism; the exact era spelling of the two lib
copies is inferred (names are not in the image)

## The retail facts

| copy | guard | seed | callers |
|---|---|---|---|
| A | 0x2c127d | 0x2c1288 | 24 refs, 12 game TUs, ALL below 0x99xxx |
| B | 0x2c278c | 0x2c2798 | 2 refs, only CAniRecordView::Rng2Next (wwd lib band) |
| C | 0x2c279c | 0x2c27a8 | 8 refs, only CFaderSine::ApplyInit/RenderFrame (fader lib band) |

B and C are the LAST allocations in .bss (the band ends at 0x2c27ac);
all three pairs are guard-BEFORE-seed with other tiny COMMONs interleaved.

## The probes (cl 5.0 SP3 + era link, 2026-08-22)

1. `__inline int GetRandomNumber()` (external) in two TUs: each obj emits
   `?holdrand@?1??GetRandomNumber@@YAHXZ@4JA` COMMON(4) + `??_B...@51`
   COMMON(1). Linking both objs -> exactly ONE guard + ONE seed in the map,
   class `<common>`, guard first. Same-name COMMONs FOLD across objs (and
   libs). => identical shared-header source can NEVER yield 3 copies.
2. `static __inline` (internal): each obj emits `_?holdrand@...$S168` and a
   guard `_?$S1@...$S170` in the TU's OWN section, seed@+0 and guard@+4 -
   seed BEFORE guard, exactly 4 apart, placed with the TU's data. Retail's
   guard-before-seed, ~12 apart, at the .bss COMMON tail DISPROVES this
   form for B and C.

## The conclusion

All three copies were EXTERNAL COMMONs under three DIFFERENT mangled names.
Under one toolchain that requires three different source spellings - i.e.
three revisions of the utility, one per linked module: the game's shared
header (the CREDITZ text, free `__inline`), plus a per-module variant in
the wwd lib and the fader lib. The reconstruction spells those as class
members (CAniRecordView::/CFaderSine::GetRandomNumber), the minimal
mangling-changing spelling consistent with each module's context: the ani
class already owns a bespoke RNG family (Rng2Next), and the fader module's
`GetRandom(lo,hi)` provably builds on GetRandomNumber while the game's
builds on `rand()` - two independent proofs that the modules carried
DIVERGED REVISIONS of the rand utility. Any other mangling-changing
spelling (signature, namespace, other class) fits the image equally; the
member spelling is a model choice, the THREE-NAMES fact is proven.
Different-compiler-per-lib decoration cannot explain B != C between two
libs of the same engine and is not needed.

## Reuse

When N retail copies of "one" header static appear: partition the callers
by module band first. Then classify each copy by the guard/seed LAYOUT:
seed-then-guard exactly +4 inside a TU's data = `static` internal per-TU
copy (the GruntDirStatics device, data_compgen class=copy); guard-and-seed
scattered among the gathered COMMONs = external per-NAME copy (class=common,
one distinct source spelling per copy).
