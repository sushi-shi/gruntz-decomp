# Local declaration order steers the schedule of commuting instructions

**Tags:** cpp:local cpp:decl cpp:sibling | asm:sub asm:mov asm:rep | topic:codegen-idiom topic:scheduling

## Symptom

`gruntz walls diagnose` reports REGALLOC/SCHEDULING with identical sizes, calls,
branches, returns, and relocations. The only residue is the order of two
independent operations on the same destination:

```
retail:  sub esi,edx    ; subtract current minutes
         sub esi,ebp    ; subtract the seconds carry
base:    sub esi,ebp
         sub esi,edx
```

Changing the arithmetic's parenthesization does not move the instructions.

## Cause and lever

VC5's scheduler retains the front end's local-handle order even when the two
operations commute. Reordering the declarations of the two scalar temporaries
can therefore change their instruction order without changing their values,
lifetimes, or the control-flow graph.

Use this only when the operations are proven independent and the declarations
remain in their authentic scope. Do not restore misleading names to recover an
old compiler state.

## Controlled evidence

`CTimer::AddTime` 0x0009c0e0 was 100% exact before correcting its reversed public
roles from `(seconds, minutes)` to `(minutes, seconds)`. With honest `secs` and
`mins` locals, it became 99.8214%; the 0xa3-byte body still had 57 instructions,
zero calls, five branches, and one return, with the first difference at +0x72.

These expression variants all emitted the same base-only order:

- `0x63 - onClock - carry`
- `0x63 - carry - onClock`
- two explicit subtraction statements

The sole effective A/B was declaration order:

```
// 99.8214
u32 onClock;
u32 carry = 0;

// 100.0000 exact
u32 carry = 0;
u32 onClock;
```

The final spelling keeps every semantic name correct and restores the retail
`sub onClock; sub carry` schedule.

## Scalar placement around aggregate initialization is the longer-range form

The same lever can begin much earlier than the visible residue.  In
`CDDSurface::SaveRle16` 0x00144640, the reconstruction declared `height` after
zeroing `BITMAPINFO` and `width` after zeroing the following file-header record.
Retail loaded height before the first `rep stos` and width later, so transcribing
that emitted order did not reveal the mistake.  The exact sibling
`CDDSurface::SaveTga` proves the authored order instead: `width` lies between the
two record initializations and `height` follows the second.

Putting the scalars in that sibling-proven order changed the whole allocation
texture while preserving the algorithm: 217 instructions became retail's 216,
the first divergence moved from +0x28 to +0xcc, and fuzzy rose **91.8957 ->
97.9000**.  Moving the independent `bfSize` assignment after the bitmap
plane/bit-count stores then made the prefix byte-identical through +0x105 and
raised it to **98.4360** in the disposable-object scorer (98.47 in the normalized
compare).

The boundary was measured rather than guessed.  Forty-seven atomic AST variants,
all 299 compatible declaration-hoist combinations through depth three, and a
30-state mixed handle-stride panel produced no higher state.  The remaining
residue is only a symmetric ESI/EDI coloring of `Lock()`'s result and the two
loops.  Thus the source-backed gain is the placement of real locals and stores;
the final register rename is not a reason to add another fake local.

## Reverse-use heuristic

When a semantic rename leaves an otherwise identical function with only a pair
of commuting instructions swapped, compare the declaration order of the two
source values before adding temporaries or distorting the expression.  When the
first divergence brackets inlined aggregate initialization (`rep stos`, record
copy, or constructor expansion), use an exact sibling to recover which scalar
was declared on each side of that boundary; emitted load order is scheduler
output. Confirm the candidate against the baseline and retail, because this is
a scheduling lever, not evidence that declaration order carries runtime meaning.
