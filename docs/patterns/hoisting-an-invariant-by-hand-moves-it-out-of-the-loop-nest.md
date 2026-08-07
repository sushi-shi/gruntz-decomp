# A pre-loop `float f = (float)param;` local is the WRONG shape - leave it inline

tags: cpp:loop cpp:local cpp:float cpp:int | asm:fild asm:fstp asm:sub | topic:codegen-idiom

symptoms: a nested table-builder whose inner loop is instruction-for-instruction
right but whose FRAME is 12-28 bytes smaller than retail's; retail converts the
integer parameters to float in a block that sits INSIDE the outer loop, right
after the inner loop's zero-trip guard, storing each result into its own slot
through one shared `fild`/`fstp` scratch; ours computes them once before the
outer loop and each conversion has a permanent home instead.

confidence: 10/10

## What retail looks like

`CShadeTableCache::HueRampTable` @0x14e830, immediately after `test ebp,ebp /
jle <skip inner loop>`:

```
fild  DWORD PTR [esp+0x44]        ; (float)steps   - the PARAMETER's home
mov   ecx,eax                     ; packedColor
shr   ecx,0x10
fstp  DWORD PTR [esp+0x20]        ; -> fSteps slot
and   ecx,0xff
mov   DWORD PTR [esp+0x1c],ecx    ; ONE int scratch, reused three times
fild  DWORD PTR [esp+0x1c]
mov   DWORD PTR [esp+0x1c],edx
fstp  DWORD PTR [esp+0x24]        ; -> (float)blue
fild  DWORD PTR [esp+0x1c]
...
```

The tell is the **single reused int scratch** (`[esp+0x1c]` here, `[esp+0x2c]`
in `HsvShiftTable`): the integer operands are all live in REGISTERS at that
point and are pushed through one slot one at a time. A hand-hoisted
`i32 cb = ...;` before the loop cannot produce that - it has to give each
integer its own home, because each one is live across the whole nest.

## The rule

**Write the conversion at its USE SITE.** cl5's loop-invariant hoisting lifts
`(float)steps` and `(float)((packedColor >> 16) & 0xff)` out of the INNER loop
and parks them in the OUTER loop body; that is where retail's slots come from.
Hoisting it yourself in source moves the computation one level too far out and
re-colours the whole function.

```cpp
// NO - three int locals + three float locals live across the whole nest
i32 cb = (packedColor >> 0x10) & 0xff;
float fSteps = (float)steps;
for (i32 i = 0; i < 256; i++)
    for (i32 j = 0; j < steps; j++)
        ... (float)j / fSteps ... t1 * (float)cb ...

// YES - cl hoists both into the outer loop body, after the inner guard
for (i32 i = 0; i < 256; i++)
    for (i32 j = 0; j < steps; j++)
        ... (float)j / (float)steps ... t1 * (float)GetBValue(rgb) ...
```

Same for `(double)gamma` fed to `pow`, `(float)(pct - 100)`, and
`(baseArg & 0xff) + pal[i].peRed`.

## Corollary: a constant loaded BEFORE the divisor is one expression

`float t0 = g_one - t1;` with `t1` from the previous statement emits
`fild j / fdiv steps / fld g_one / fsub st,st(1)` - the constant last, because
`t1` is already a materialized value. Retail emits `fld g_one / fild j /
fdiv steps / fxch st(1) / fsub st,st(1)`: the LHS is pushed first and the
division is then CSE'd for the later `t1 * ...` uses. That is the shape of

```cpp
float t0 = g_one - (float)j / (float)steps;
float t1 = (float)j / (float)steps;   // CSE'd - cl emits ONE fdiv
```

## Evidence

| function | before | after |
|---|---|---|
| `CShadeTableCache::HueRampTable` 0x14e830 | 67.51 | **99.29** |
| `CShadeTableCache::HsvShiftTable` 0x14e540 | 63.24 | 84.85 (also needed `pal[i].peX` + `HSV_MIN`) |

`HsvShiftTable`'s frame went `sub esp,0x28` -> retail's `sub esp,0x44` from this
change alone; the "7 unmodelled dword locals" it had been filed under were the
hoisted `(double)gamma` (8 B), `(float)(pct-100)`, `(float)steps` and the three
palette cursors.

## Related

- [call-killed-invariant-is-a-source-local](call-killed-invariant-is-a-source-local.md)
  is the INVERSE and its screen: when the loop body contains a CALL, cl cannot
  hoist, so a value retail parks in a slot across it IS a named source local.
  Both of these table builders call `FindNearestColor` in the inner loop - the
  conversions still hoist because they are pure and the call is not in their way
  (the hoist target is the OUTER loop body, which the call does not dominate).
- [loop-bound-local-vs-inline-invariant](loop-bound-local-vs-inline-invariant.md)
- [cse-defeat-uncached-global-rewalk](cse-defeat-uncached-global-rewalk.md)
