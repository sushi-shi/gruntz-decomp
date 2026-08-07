# A dead frame slot that only ever gets HALF a struct stored = a by-value accessor, re-read per use

- **confidence**: 9/10
- **tags**: `cpp:struct` `cpp:local` `cpp:call` | `asm:mov` `asm:add` | `topic:codegen-idiom`
- **measured**: `CChatBoxOwner::HitTest` 0x21140 71.09 -> **100.00 EXACT**

## Symptoms

Retail allocates a small frame (`sub esp,0x8`) whose only writes are

```
mov ecx,DWORD PTR [eax+0x8c]
mov DWORD PTR [esp+0x8],ecx        ; <- never read back
mov ecx,DWORD PTR [eax+0x90]
add ecx,0xffffffc0                 ; <- `add r,-K`, NOT `sub r,K`
```

repeated **once per use** of the *other* half, and the recompile has no frame at all
(or one copy for the whole function) plus `sub reg,0x40` / `lea reg,[eax-0x40]`.

Three tells, all required:

1. **A frame slot that is written and never read.** Only the FIRST dword of an
   aggregate is stored; the second is consumed straight out of the source memory.
2. **`add reg,-K` where you emit `sub reg,K`.** cl5 canonicalises an offset applied
   to a *struct member of a temporary* into the add-negative form.
3. **The store repeats at every use site**, sharing one slot.

## Reading

The source does not hold ONE local copy of the aggregate - it produces a fresh
**rvalue** of the whole struct at each use, i.e. an inline accessor returning it by
value:

```cpp
static __inline tagSIZE ModeSize() { return g_gameReg->m_modeSize; }
...
if ((x < 0x40 && y >= ModeSize().cy - 0x40) || (x > 0x40 && y >= ModeSize().cy - 0x20))
```

cl5 materialises the temp per call, copy-propagates the member that is read
(`.cy` leaves no store, its value comes straight from `[eax+0x90]`) and leaves the
*unread* half's store standing - it does not do partial-aggregate DSE. The single
frame slot is reused because the temps' live ranges are disjoint.

## What does NOT work

- **One function-scope local** (`tagSIZE mode = g_gameReg->m_modeSize;` once per arm)
  gives exactly ONE store per arm instead of one per use: 59.50.
- **Statement-splitting the `||` so each use can re-assign the local** produces the
  right store count but a separate epilogue per `return`, destroying retail's
  tail-merge: 54.67.
- **Reading the member directly** (`g_gameReg->m_modeSize.cy - 0x40`) emits no frame
  at all and `sub reg,0x40`: the 71% baseline.

Only the by-value accessor gives all three tells at once.

## Related

- [[member-store-direct-not-via-temporary]] - the opposite direction for scalars.
- [[call-result-local-flips-callee-saved-set]] - the other "bind it or don't" knob.
