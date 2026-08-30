# A bounded MFC scan owns the empty guard and materializes `GetAt` after it
tags: cpp:loop cpp:mfc cpp:inline cpp:local | asm:test asm:jl asm:mov | topic:codegen-idiom topic:cfg topic:regalloc
symptoms: retail tests the array size before materializing its data pointer, advances a spilled cursor beside a register index, takes a `jl` back-edge, and has a local exhaustion epilogue; base hoists `GetData`, guards size separately, and uses an unbounded loop
confidence: 10/10

`CBattlezMapConfig::StepRowSpawn` 0x26470 began at 84.4107 with 219
instructions, 21 branches and 4 returns against retail's 226, 20 and 5. The
source had an explicit `GetSize() <= 0` guard, a function-scope raw pointer from
`MfcPtrArrayData`, and an unbounded `for (;;)` whose tail returned on
exhaustion. Merely reversing the tail condition was byte-flat.

The useful exploratory descent was a bounded loop:

```cpp
for (; i < array.GetSize(); i++) {
    item = static_cast<T*>(array.GetAt(i));
    if (IsUsable(item)) {
        goto found;
    }
}
return failure;

found:
```

The first bounded-loop trial fell to 71.6696 because it also retained a
redundant post-loop size check. Nevertheless, it introduced three retail
features absent from the 84.4107 baseline: EBP held the index, an advancing
array cursor was spilled, and exhaustion reached a local return epilogue. A
`do`/`while` spelling canonicalized back to the old island and was rejected by
the baseline-delta check. Sending success directly to `found` reached 86.04;
removing the standalone empty guard reached 86.93 and made the CFG exact at 20
branches and 5 returns. Finally, restoring the authentic inline
`CPtrArray::GetAt(i)` expression instead of pre-hoisting the backing pointer
reached 88.8080, with base/retail agreeing on frame 0x24, 4 calls, 20 branches,
5 returns, 7 relocations, every store, displacement, FP operation and ordered
referent.

The last mismatch is separately bounded: cl widens a byte member of the copied
frame-local `BrickzCell` with a dword load plus `and 0xff`, while retail uses
`xor` plus a byte load. Eleven aggregate/lvalue variants and a fresh
target-adjacent 128-state C1 forest found no alternative source island; see
`stack-aggregate-byte-member-loads-a-dword.md`.

## Reverse use

When retail tests size before materializing an MFC array's data pointer, then
uses both a register index and an advancing cursor, do not infer a standalone
empty guard followed by a raw-data loop. Test the authored bounded loop first:
its condition owns the empty-array behavior, and its in-body `GetAt` call is
the abstraction cl strength-reduces into the cursor. Preserve the inline MFC
access even when the emitted arithmetic makes a hoisted `GetData` transcription
look equivalent. If the first bounded form dips but restores those features,
keep it as the base and remove redundant guards or checks one at a time.
