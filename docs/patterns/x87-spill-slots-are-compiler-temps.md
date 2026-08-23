# x87 spill slots are COMPILER temps: their frame order is inert to declaration order, and a dead conversion temp's REUSE sets the frame size
tags: cpp:local cpp:float | asm:fld asm:fstp asm:fild asm:sub-esp | topic:regalloc topic:wall
symptoms: frame differs by exactly one 8-byte slot; every `fst`/`fld`/`fmul`
`[esp+N]` displacement is permuted; the GENERAL-purpose registers already match
retail instruction for instruction; declaring/renaming/reordering the `double`
locals is byte-identical
confidence: 9/10
variants: local-slot-order-is-declaration-order.md, od-local-slot-ordering.md,
folded-local-frame-slot-roles.md

Two shipped patterns explain frame-slot ORDER for *source* locals:
`local-slot-order-is-declaration-order.md` (address-taken scalars, `/O2`) and
`od-local-slot-ordering.md` (`/Od`, ordered by the identifier's hash). **Neither
applies to a `double` that never has its address taken.** Such a value lives in
the x87 stack and only acquires a frame slot when the scheduler spills it, so
the slot is an ALLOCATOR temp, and its position is decided by the FP schedule.

Detection signature, all three together:

* the frame differs by a multiple of 8 while the instruction COUNT is equal;
* every `[esp+N]` on an x87 mnemonic is shifted or permuted;
* the integer registers already agree — `esi`/`edi`/`eax`/`ecx`/`edx` pair
  one-for-one with retail across the whole function.

The third bullet is what separates this from the callee-saved re-colour in
`folded-local-frame-slot-roles.md`: there is no colour to steer, because the
colouring is already correct.

## Where the extra slot comes from: an unsigned->double temp that is not reused

`(double)someU32` lowers to a 64-bit stack temp (`mov [esp+N+4],0` /
`mov [esp+N],reg` / `fild QWORD PTR [esp+N]`). The temp is DEAD after the
`fild`, so a later spill may reuse its slot — and whether it does is decided by
where the scheduler put the `fild`.

`CBoomerang::AdvanceMotion` 0xe08b0, with retail's rotation association
(`m_originX + (vy * s - vx * c)`, proven because retail's `fsubrp` precedes the
`faddl 0x240(%esi)`), reaches retail's instruction count and byte length exactly
— and allocates FIVE 8-byte slots where retail allocates four:

```asm
; retail: the fild kills the i64 temp at [esp+0x8] BEFORE `sin` is spilled,
; so `sin` reuses that slot -> four slots, sub esp,0x20
fstl   0x10(%esp)      ; vx
fstl   0x18(%esp)      ; vy
fildll 0x8(%esp)       ; amp   <- i64 temp dies here
fstpl  0x8(%esp)       ; sin   <- REUSES it
fstpl  0x20(%esp)      ; cos
```
```asm
; ours: `sin` and `cos` are spilled first, the fild comes after the reloads,
; so nothing can share -> five slots, sub esp,0x28
fstl   0x8(%esp)       ; vx
fstl   0x10(%esp)      ; vy
fstpl  0x20(%esp)      ; sin
fstpl  0x28(%esp)      ; cos
fildll 0x18(%esp)      ; amp
```

## Measured-inert levers (do not re-derive)

Against `AdvanceMotion`, all with the same retail-shaped expression tree:

| lever | result |
|---|---|
| declare the conversion local FIRST instead of third | **byte-identical** |
| hoist the rotation into `rx`/`ry` locals | **byte-identical** (it is the DAG, not the spelling) |
| drop the local, inline `(double)g_frameDelta * m_velScale` | 134 insns, frame 0x18 — further away |
| move the `m_phase +=` statement after both position stores | fixes the STORE order, does not move the `fild` |

Declaration order steers frame slots for address-taken scalars. It does not
steer x87 temps: cl sinks the conversion to its use, and the schedule places
the `fild`. If the slot map is the only residue and the GPRs already match,
this is a park, not a work item.
