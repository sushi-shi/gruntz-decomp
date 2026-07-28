# A repeated inline block's LOCAL DECL ORDER picks which dead parameter home it spills into

**Tags:** `cpp:local` `cpp:inline` `cpp:macro` | `asm:mov` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 9/10

## Symptom

A function that inlines the same block twice (a force-inline macro / an `__inline`
helper) matches the FIRST copy byte-for-byte and differs in the SECOND by exactly
two things:

```
   base                              target
   mov ebx,[esp+0x1c]   ; x          mov eax,[esp+0x20]   ; y      <- two loads swapped
   mov eax,[esp+0x20]   ; y          mov ebx,[esp+0x1c]   ; x
   ...
   mov [esp+0x1c],ecx   ; spill      mov [esp+0x20],ecx           <- spill lands in the OTHER param home
```

`[esp+0x1c]`/`[esp+0x20]` are the two incoming parameter homes, both dead by then.
Everything else - registers, offsets, CFG, block order - is identical. It reads
like an unsteerable regalloc coin-flip. It is not.

## Cause

The local area is already full (here `sub esp,8`: one slot for the spilled `this`,
one for the first copy's dead-store local), so the second copy's local must reuse a
dead incoming parameter's home. cl5 picks the home of the parameter it *consumed
first*, and it consumes the parameters in the DECLARATION ORDER of the block's own
locals - i.e. of the two locals seeded from the two arguments:

```cpp
i32 px_ = (X);   // declared first  -> x loaded first -> the spill takes x's home
i32 py_ = (Y);
```

In the first inlined copy this is invisible: the argument loads get hoisted into the
prologue and the local area still has a free slot, so both orders emit the same
bytes. Only the second copy exposes it.

## Fix

Swap the two declarations inside the inlined block (NOT at the call site - a
pre-computed outer temp does not move it; the inner copies' declaration order is
what counts).

```cpp
// before
#define PROBE_TILE(LVL, X, Y, RESULT) do { \
    i32 px_ = (X); \
    i32 py_ = (Y); \
    ...

// after - matches retail
#define PROBE_TILE(LVL, X, Y, RESULT) do { \
    i32 py_ = (Y); \
    i32 px_ = (X); \
    ...
```

## Evidence

`include/Gruntz/GameLevel.h`'s `PROBE_TILE`. Proven twice: hand-inlining the second
copy with `py_` first made `CGameLevel::ProbeStepEdge` (0x15fc30) byte-identical, and
flipping the two decls in the macro itself reproduced that with the macro call
restored. Whole-tree effect of the one-line swap (38 call sites over
`GameLevel.cpp` + `GameLevelMove.cpp`): +4 EXACT overall, with
`ProbeStepEdge` 99.91 -> 100, `ProbeColumn` 92.58 -> 100, `ProbeFeetKind` 92.58 -> 100,
`ProbeHeadSoft` 94.91 -> 100, `VisitVisible` 92.33 -> 100, `ProbeFootBlocked`
82.6 -> 99.07. No unit regressed below its previous MAX.

## Related

- [`sib-base-index-follows-local-decl-order.md`](sib-base-index-follows-local-decl-order.md)
  - the same "cl5 orders by declaration" rule, seen in SIB operand roles.
