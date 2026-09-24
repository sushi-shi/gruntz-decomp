# A macro boundary changes VC5 x87 CSE even when the expanded expression is equivalent

tags: cpp:macro cpp:float cpp:expr | asm:fld asm:fmul asm:fxch | topic:codegen-idiom topic:regalloc topic:wall
symptoms: an x87 loop has the correct calls and branch skeleton but folds repeated loads that retail rematerializes; equivalent parenthesizations form several stable compiler islands
confidence: 9/10

MSVC 5.0 does not always reduce a helper-macro spelling and its hand-expanded
equivalent to the same optimizer input. Macro-origin/source-location state can
change x87 common-subexpression elimination, stack scheduling, and spill-slot
selection even when the arithmetic AST and relocation targets are unchanged.

## Controlled witness

`CShadeTableCache::FlashTable` 0x14df40 computes each bright RGB channel twice
through `HSV_MIN`. Retail reads `g_percentScale` six times: once in the compare
and once in the selected arm for each channel.

With the byte-parameter `FindNearestColor` ABI and every other source line held
fixed, spelling only the bright addend as a helper produces a different object
from writing that term at each use:

```cpp
// Hand-expanded at each use.
(static_cast<float>(endPct) * uu)
    * (static_cast<float>(channel) * g_percentScale)

// A narrow helper expands to the same association.
#define FLASH_BRIGHT_TERM(pct, channel, t) \
    ((static_cast<float>(pct) * (t)) \
     * (static_cast<float>(channel) * g_percentScale))
```

The inlined spelling scores 94.217130%, is 1540 bytes, and has 39/36 raw
function relocations. The narrow helper scores 93.936260%, is 1548 bytes, and
also has 39/36 raw relocations. Both preserve all six semantic
`g_percentScale` reads; only x87 scheduling and stack homes differ. The narrow
helper is evidence, not a source model: naming one addend of one caller is the
wrong abstraction level.

A separate eight-association sweep found a tempting 95.511955% island at 1520
bytes, but it has only 34/36 raw relocations because VC5 folds the six scale
reads to one. A 576-cell Cartesian composition over operand order, association,
`inv` declaration form, and `uu` conversion form produced eight compiler
islands and did not restore the missing reads on that high-score island. The
best all-six-read island in that first matrix was the percent-first term above.

The structural follow-up moved the boundary outward. A four-argument weighted
interpolation macro reached 96.585655%, retained all six reads, and emitted
1556 bytes. The decisive form lets the general operation own the complementary
weight:

```cpp
#define INTERPOLATE(start, end, amount) \
    ((start) * (g_one - (amount)) + (end) * (amount))

HSV_MIN(
    INTERPOLATE(
        static_cast<float>(channel),
        (static_cast<float>(endPct) * static_cast<float>(channel))
            * g_percentScale,
        amount
    ),
    g_255
)
```

That general three-argument macro scores 99.187250% at 1528 bytes against
retail's 1524. The normalized pair has the same 15 calls, 40 branches, one
return, 36 relocations, displacement/store/immediate multisets, and ordered
referent sequence. Its whole residue is two surplus `fxch` instructions (507
versus 505 instructions). A percentage-specific three-argument macro, moving
the clamp inside the macro, reversing both multiplications, and swapping the
addends are byte-identical. Sixty-four mixed TU-state trials are also flat.
Splitting the general operation into `INTERPOLATE` plus a nested four-weight
macro is byte-identical too: adding a second preprocessor boundary does not
account for the final two `fxch` instructions.

An `inline float Interpolate(float,float,float)` is not interchangeable with
the macro: VC5 folds the scale computations across the inlined calls, producing
only 34/36 raw relocations and 85.529880%. The retail rematerialization therefore
supports a macro boundary, while the operation's proper abstraction supports
the general `INTERPOLATE` spelling rather than `FLASH_BRIGHT_TERM`.

## Dark-phase negative control (PR #79 reassessment)

At `ea8b34a71`, the dark RGB phase still expanded the weighted operation by
hand. Replacing all six comparison/selected-arm expressions with the existing
`INTERPOLATE`, removing the unused complementary-weight local, and then
separately composing `HSV_MIN` are both byte-flat in the real VC5 TU:
99.203186%, 1,528 bytes, 507 instructions, 15 calls, 40 branches and one return.
The two surplus exchanges remain in the bright phase. Thus the older positive
macro witness does **not** imply that every hand-expanded use changes codegen;
it also did not establish complete consumer adoption.

The complete dark region at offsets `0x1c8..0x3b5` agrees with the original
instructions and fixup offsets. Independently resolving the raw original
stream proves all **35 non-EH** references, including two `g_one`, twelve
channel-limit and six percentage-scale references. The semantic-diff tool's
32-entry summary is not that raw census. The first EH-table reference and
three compiled FS:0 references are separately scoped, not silently counted as
ordinary data references.

`scripts/test_flash_interpolation_consumers.py` covers the complete dark
region and raw stream, rejecting changed signed division, clamp branches,
missing/repeated/wrong references and DIR32 addends. It does not certify the
whole bright body or compiler-generated EH owner. Canonical adoption and
remaining alternatives are `reassess-flash-interpolate`,
`reassess-flash-min` and `reassess-interpolate-literal-token-profile`.

## Parentheses around a macro argument can reverse the FP-pool constant

The surviving LithTech `ROUND` macro provides a second, smaller witness where
the macro boundary was already correct but its adapted token tree was not:

```cpp
// Extra grouping introduced by the adaptation.
#define ROUND(value) static_cast<int>((value) + 0.5)

// Surviving x+0.5 grouping, retaining the project's named cast.
#define ROUND(value) static_cast<int>(value + 0.5)
```

Under the pinned VC5 build, the first form emits `fsub` against a pooled `-0.5`.
Retail emits `fadd` against `+0.5`; because the base object contains the wrong
payload, the data-attribution gate correctly refuses to bind retail slot
`0x1e9aa0`. Removing only those parentheses restores `fadd`, the `+0.5` pool
entry, and exact code in both users: `CGruntHealthSprite::BindToGrunt`
98.2353 -> 100.0000 and `HealthUpdate` 99.1304 -> 100.0000.

This is not permission to drop ordinary defensive parentheses around macro
arguments. It is a reverse-use clue for a proved historical macro whose exact
body survives: preserve its expression grouping, and let the FP payload plus
opcode adjudicate adaptations that are mathematically equivalent.

## Compose independently supported helpers before judging a dip

The Projectile cohort at `321ac0613` supplies a source-composition control,
not another exact closure. Its launch owner initially scored 95.41684% at
1,769 bytes/476 instructions versus retail's 1,773/478. Required `BDefs` and
`Globals` includes were each compiled separately and were byte-flat. Four
double-square macro uses alone moved launch to 92.13552% without changing its
extent, calls, branches, returns or 92 relocation offsets/identities. The first
new difference was at `+0x491`: duration multiplication and unsigned conversion
moved relative to the square products and square root.

Composing the independently supported `Max(ddx, ddy)` tile-count selector
restored the two missing transfer instructions and retail extent, producing
96.223816%. All 92 ordered relocation identities and addends, including internal
switch destinations, then agreed. The initial square dip was therefore not a
valid reason to discard the sourced base.

A complete four-site `Sqr(const T&)` template profile, without an added
`inline` specifier, then restored retail's launch product/IMUL/conversion/FSQRT
schedule and reached 99.505135%. Genuine shared-header placement preserved all
scored bodies. The double homes still exchange `+0x14` and `+0x1c`, and an early
integer scheduling difference remains; this does not prove exact provenance or
bound the entire owner. In the companion motion owner, macro and const-reference
squares emit identical bodies on the same composed clamp base.

That clamp composition is an independent result-ownership control. Assigning
`localX = Min(xRes, target); xRes = localX;` in both directed X arms made motion
1,812 bytes/466 instructions at 95.8141%. Assigning the actual updated result
first, `xRes = Min(xRes, target); localX = xRes;`, removed redundant stack copies
and recovered retail's 1,804 bytes/464 instructions and EBX result at 97.74786%.
The negative-velocity arm uses `Max`; both Y arms use the corresponding helper.
The complete composition keeps 20 calls, 73 branches, three returns and all 49
ordered identities/addends. It also emits the shadow X snapshot before the
second integer conversion, as retail does. Merely moving that snapshot's source
declaration earlier is byte-flat: the feature was already present after the
result-ownership correction.

Reverse use: separate header context, arithmetic boundary, selector boundary,
and result ownership. Compare the first divergence and the actual desired
feature against the original baseline before composing. Neither a flat helper
nor a temporary dip disproves its source family. These controls use production
VC5 compilation and direct object/image reads; test suites remain deferred to
authorized squash merge. Per-option dispositions and reopening conditions live
only under `reassess-sqr-projectile`, `projectile-*`, and
`reassess-selector-screen-projectile-goal` in the lineage ledger.

## Reverse-use rule

When retail repeatedly relocates the same constant/global but the base keeps a
single x87 value, count the ordered relocations before ranking candidates. Keep
an inline or macro spelling open when neither is exact, and compose from the
one whose rematerialization pattern matches retail even if its fuzzy score is
lower. Then widen the helper boundary to the complete reusable operation: a
codegen-shaped helper for one partial term is not vindicated merely because it
moves the compiler. A higher score with the wrong referent multiplicity is a
local maximum, not proof of the source.

Do not add a macro merely as opaque steering. Retain it only when it expresses
a real repeated operation and the source remains something a period developer
could plausibly have written.

## Related

- [`integer-square-macro-preserves-expression-origin.md`](integer-square-macro-preserves-expression-origin.md)
- [`x87-named-temp-keeps-fcom-cse.md`](x87-named-temp-keeps-fcom-cse.md)
- [`equal-frame-residual-census.md`](equal-frame-residual-census.md)
