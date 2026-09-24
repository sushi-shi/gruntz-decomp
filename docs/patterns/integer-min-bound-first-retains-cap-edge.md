# A by-value integer Min keeps the cap's tie edge through argument order

tags: cpp:inline cpp:local cpp:branch | asm:cmp asm:jle asm:jl | topic:source-shape topic:codegen-idiom
symptoms: restoring an authored Min helper changes only the equal-value branch of a scalar upper cap
confidence: 10/10

## Controlled composition

`CRandomAmbientSound::Update` has two integer half-duration caps. Retail stores
the full sampled duration, shifts its unsigned representation right by one,
then uses signed `cmp value,1000; jle keep; mov value,1000`. The shared authored
template returns `a < b ? a : b` by value.

Actual `worldsoundset` TU controls, retaining both range/RNG helper calls:

| Source form at both sites | Current fuzzy | Complete Update body |
| --- | ---: | --- |
| Expanded cap | 92.81884 | 358 bytes, 139 instructions |
| `half = Min(half,1000)` after the shift | 91.94927 | Same extent; two `jle` become `jl` |
| Shift and value-first Min in one initializer | 91.94927 | Identical to the preceding control |
| `Min(1000,shiftedValue)` in the initializer | 92.81884 | Identical to the complete expanded baseline |

All controls have seven calls, 30 branches, three returns and ten ordered
relocations. The other 35 compared TU bodies remain byte/reference/extent-flat.
Bound-first ordering is already present in the surviving `Clamp` composition:
`Min(max, Max(val,min))`. No global template change or forcing device is needed.
The baseline already had the correct inclusive edge; restoring it through the
helper is source recovery, not a new byte-matching gain.

An independent historical correction then restored the full-width phase
toggle, `m_playPhase ^= 1`, instead of `!m_playPhase`. The original retail load,
XOR-one and store prove that operation; history `026d29d81` identifies the
normalizing cleanup that lost the headroom. With both Min calls retained this
recovers 95.03623%, 352 bytes/136 instructions versus retail's 360/138. Calls,
branch/return counts and ordered referents still agree. This recovers the old
historical maximum; it is not an exact match or a newly bounded wall.

## Reverse use and limits

When an integer min/max helper changes only an equality branch, inspect the
authored comparison and argument order before rejecting the abstraction or
changing its comparison globally. Check both symmetric callers together.
First establish that the arguments are pure values of the same width and that
either equal arm returns identical bits. This result does **not** establish
commutativity for reference-returning selectors, side-effecting macros,
floating NaNs/signed zero, or mixed-width overloads.

The retained local remains a signed `i32`; the inner unsigned conversion keeps
the retail logical shift. Moving the cap before the countdown store would
change state, and using arithmetic shift would change negative sampled values.
The `b32` phase remains a full-width integer, not a byte or C++ `bool`.

`scripts/test_ambient_range_helpers.py` checks the bounded phase/cap/call
protocol against both the actual production object and original PE, and resolves
all ten ordered function references to original addresses. Negative
controls alter the toggle, destination, shift, cap, equality edge and fade
referent. The control does not certify the whole caller: its earlier register
roles and the two range-span `inc` versus retail `lea; test` remain open.
Canonical source adoption and alternatives are `reassess-ambient-min` and its
scoped companion rows in the lineage ledger.
