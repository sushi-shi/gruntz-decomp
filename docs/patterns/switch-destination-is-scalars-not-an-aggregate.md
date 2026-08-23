# A destination assigned in switch arms is TWO SCALARS when retail's final stores are separated
tags: cpp:switch cpp:local cpp:struct cpp:store | asm:mov asm:add | topic:codegen-idiom
symptoms: aggregate copy, Coord, POINT, reload before the final member stores, `mov ecx,esi` between two stores, undefined default arm, `ja` default loads two frame slots
confidence: 9/10

A value computed by a `switch` and written to a two-field member at the end has two
models: one aggregate local (`Coord next;` + `m_pos = next;`) or two scalars. cl 5.0
lowers an aggregate as ONE copy fed by a reload of the struct's frame home, so the
two member stores come out ADJACENT and preceded by two loads. Two scalars are two
independent statements the scheduler may separate. Read which one retail emits.

```cpp
// retail: two stores, the next call's receiver setup BETWEEN them
Coord tile = m_lastTilePx;   // keep the aggregate SOURCE: it is what lets cl
i32 nextX;                   // coalesce next onto tile destructively (`add ebx,0x20`)
i32 nextY;                   // ... and NO initializer: see the default-arm test
switch (m_entranceCell.direction) {
    case DIR_NORTHEAST: nextX = tile.m_x + 0x20; nextY = tile.m_y - 0x20; break;
    case DIR_SOUTHWEST: nextX = tile.m_x - 0x20;   // fall through
    case DIR_SOUTH:     nextY = tile.m_y + 0x20; break;
}
m_lastTilePx.m_x = nextX;
m_lastTilePx.m_y = nextY;
```
```asm
; retail - separated stores, values still in their registers
mov DWORD PTR [esi+0x17c],ebx
mov ecx,esi
mov DWORD PTR [esi+0x180],edi
call CGrunt::ComputeFacing
; aggregate model instead - adjacent stores fed by a reload of the struct home
mov eax,DWORD PTR [esp+0x18]
mov ecx,DWORD PTR [esp+0x1c]
mov DWORD PTR [esi+0x17c],eax
mov DWORD PTR [esi+0x180],ecx
mov ecx,esi
```

**The default arm dates the declaration.** When the `ja` out-of-range block loads the
destination from frame slots NOTHING in the function ever writes, the destination is
genuinely undefined on that path — so it is declared WITHOUT an initializer and
assigned only inside the arms. Initializing it before the switch instead makes cl home
both halves in the PROLOGUE, which retail does not do.

Steerable. CGrunt::ClaimSwitchTile 0x52c70 70.59 -> 71.40, and the residue's shape is
now one allocation swap (retail binds the destination to the callee-saved pool, we bind
tx/ty there). Three negative controls fix each half of the model: dropping the `Coord`
source copy DOES move the destination into retail's EDI/EBX but sinks the member loads
into all eight arms (150 insns / 15 branches vs 133 / 13, 64.11); spelling the source
copy as two scalars loses the destructive coalescing, so arms become
`lea edx,[eax+0x20]` where retail has `add ebx,0x20` (64.39); and pre-initializing plus
`+=` forces the prologue home (61.19). Statement order around neighbouring fetches is
codegen-inert here — cl reschedules it.
