# A switch arm's trailing store belongs AFTER the switch - cl duplicates it back in

tags: cpp:switch cpp:store | asm:add asm:sub asm:jmp | topic:codegen-idiom
symptoms: retail's arms end in the SAME two stores yet share suffixes we do not; our
`x -= N` lowers as `add reg,-N` where retail has `sub reg,N`; our two constant adjustments
come out in the opposite order to the source; retail has FEWER arm bodies than there are
`case` labels
confidence: 10/10 (isolated on 14 scratch cells + one live function, 76.52 -> 94.03)

## The signal

Retail's `case` arms all end with an identical tail, and some arms are physically the
*prefix* of another - `NORTHEAST` is `sub edx,4` immediately followed by the whole
`EAST` body, with no jump between them. Eight `case` labels, six emitted bodies.

Ours has eight separate bodies, and inside each one the arithmetic is wrong in two ways:

```asm
;  retail                              ;  ours
   sub  edx,0x4                        ;   add  ecx,0x4        <- x first
   add  ecx,0x4                        ;   add  edx,-0x4       <- and `add -4`, not `sub 4`
   mov  [esi+0x5c],ecx
   mov  ecx,[edi]
   mov  [ecx+0x60],edx
```

## The cause is not the operator - it is where the arm ends

Fourteen probe cells, cl 5.0 `/O2 /MT`, one scratch TU. `y -= 4`, `y = y - 4`,
`y += -4`, `y -= d` (a local holding 4), the `unsigned` form, `0 - 4 + y`, and the
store-through form `p->f = p->g - 4` **all emit `sub reg,4`**. So does the same
statement inside an `if`/`else`. So does it inside a `switch` **when the store lives
after the switch**.

It becomes `add reg,-4` in exactly one configuration: **the store is inside the arm.**

| shape | lowering |
|---|---|
| `y -= 4; e->sy = y;` at file scope | `sub ecx,0x4` |
| `if (k) { y -= 4; } else { y += 4; } e->sy = y;` | `sub eax,0x4` |
| `switch (k) { case 0: y -= 4; break; ... } e->sy = y;` | `sub ecx,0x4` |
| `switch (k) { case 0: y -= 4; e->sy = y; break; ... }` | **`add edx,-0x4`** |

And the flip comes with a re-ordering: with the store in the arm cl also emits the two
constant adjustments in register order (`ecx` before `edx`) instead of source order, so
the arm's suffix no longer matches the cardinal arm's whole body and cl's cross-jumper
declines every merge.

## The fix

Hoist the shared trailing statements out of the switch. **cl tail-DUPLICATES them back
into every arm**, so the emitted code still has a full copy per arm - you do not lose the
per-arm stores, you only stop paying for the canonicalisation:

```cpp
switch (dir) {
    case DIRECTION_RING_NORTH:     y -= 4;          break;
    case DIRECTION_RING_NORTHEAST: y -= 4; x += 4;  break;
    case DIRECTION_RING_EAST:               x += 4; break;
    /* ... */
}
(*p)->m_screenX = x;
(*p)->m_screenY = y;
```

The winning probe cell is byte-for-byte retail's arm, including the reload of `*p`
between the two stores that cl inserts on its own:

```asm
sub  edx,0x4
add  esi,0x4
mov  [eax+0x5c],esi
mov  eax,[edi]
mov  [eax+0x60],edx
```

## Worked example

`CBootyState::MoveLettersByDir` (0x00019b90, `src/Gruntz/BootyStateActivate.cpp`):
**76.52 -> 94.03**, eight arm bodies down to retail's six, every `add reg,-4` gone. The
residue is a whole-function register permutation.

## Do not read the symptom as an operator problem

This function carried an `@early-stop` note asserting that "`y - 4`, `y -= 4` and the
store-through form all canonicalise to `add`" - measured, but measured only inside the
arm, so the conclusion generalised a property of the *arm* onto the *operator* and closed
the case. When a lowering looks like it cannot be spelled, widen the probe past the
statement to the block that contains it.
