# Adjacent live coordinate pairs are `Coord` aggregates, not independent scalars

tags: cpp:struct cpp:local cpp:copy | asm:mov asm:sub | topic:codegen-idiom topic:eh
symptoms: retail loads an x/y pair from one object and stores it into adjacent frame
dwords, or derives x/y together and homes them at `base+0`/`base+4`; every later use
consumes the two values as one position; scalar reconstruction gives nonadjacent homes
and displaced EH cleanup operands
confidence: 10/10

## The signal

Two adjacent dwords are not enough by themselves. The stronger signature is complete
value flow: retail obtains both halves from one `Coord` (or derives both halves of one
position), stores them at adjacent field offsets, and later consumes both as the same
point in comparisons and calls. That is a source aggregate:

```cpp
Coord target = *CoordTail()->m_coord;

Coord start;
start.m_x = m_object->m_screenX >> TILE_SHIFT_PX;
start.m_y = m_object->m_screenY >> TILE_SHIFT_PX;
```

Two independent `i32` locals allow C1/C2 to assign unrelated homes even when the
expressions are equivalent. The primary body may barely move, while every cleanup
funclet reveals the corrected local identity.

## Calibration

`CGrunt::PathScan` (`0x57db0`) originally modeled the tail tile as `tcol`/`trow`.
Retail copied the source `Coord` into adjacent dwords; candidate placed the scalars in
nonadjacent slots. Restoring `Coord target` reduced the local reservation from `0x8c`
to `0x84` and moved 88.99% to 89.15%. Modeling the similarly adjacent start tile as
`Coord start` reduced it again to `0x7c`, moved the body only to 89.19%, and made both
`CPtrList` unwind funclets exact. All 60 conditional branches and the return shape
remained unchanged.

`CGrunt::StepCompassMove` (`0x51c00`) supplies the negative/partial control. Retail
keeps the current pixel position and proposed pixel position as two adjacent copied
pairs. Replacing four scalars with two field-wise-copied `Coord` objects moves the
CString cleanup four bytes toward retail (`-0xc` to `-0x8`) while preserving the
already-correct 83-branch/two-return counts. Whole-object assignment is not an
interchangeable spelling here: it lets VC5 collapse the body to 71 branches. The
aggregate type is proven, but its remaining frame residue stays open until the arrow
switch block layout agrees; an aggregate signal does not license padding.

This is not the dead-eight-byte-spill pattern. There, stores are never read and the
same values stay live in registers; inventing an aggregate adds unsupported source.
Here both fields have one object identity and are repeatedly read as that identity.
