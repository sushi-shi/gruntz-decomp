# `and reg,C1` followed by `test reg,C2` with C1 & C2 == 0 - the mask was a NAMED LOCAL
tags: cpp:expr cpp:local | asm:and asm:test | topic:codegen-idiom
symptoms: retail materializes `and <reg>,<mask>` and then immediately tests a bit the mask
cannot contain (`test ecx,0x20000000` right after `and ecx,0x987`), and your source with the
same two masks emits a single `test <reg>,<mask>` because cl folded the impossible test away
confidence: 9/10

cl 5.0 reassociates `(x & C1) & C2` into `x & (C1 & C2)` - and drops the whole test when that
is zero - **only while the inner `&` is still a single-use subexpression**. Bind the inner mask
to a named local and the reassociation never fires: the local has two uses from the start, cl
materializes it with a real `and`, and each later test is emitted verbatim, dead or not.

```cpp
// NO - cl folds `(c1 & 0x987) & 0x20000000` to 0, deletes the branch,
//      and CSEs the second read into a bare `test ecx,0x987`:
i32 c1 = CellFlagsAt(col, row);
if ((c1 & 0x987) & BRICKZ_CELL_OCCUPIED) { return 1; }
if (c1 & 0x987)                          { return 1; }

// YES - the AND is materialized, both tests survive:
i32 c1 = CellFlagsAt(col, row) & 0x987;
if (c1 & BRICKZ_CELL_OCCUPIED) { return 1; }
if (c1)                        { return 1; }
```
```asm
and    ecx,0x987
test   ecx,0x20000000
jne    out
test   ecx,ecx            ; cl now knows ecx == 0 on the fall-through
jne    out
push   ecx                ; ... and spends it as the two `0` arguments
push   0x1
push   0x987
push   ecx
```

STEERABLE. The dead test is not a transcription error to "clean up": it is the evidence that
retail's source named the masked value. The fall-through also teaches cl that the register is
zero, so it pays for itself in `push ecx` (1 byte) instead of `push 0` (2 bytes) at every
following zero argument - which is why deleting the dead test costs more than the test itself.

`CBattlezMapConfig::ResolveArrival` 0x2c690 82.58 -> 82.99, tail block byte-exact against
retail (2026-08-08).
