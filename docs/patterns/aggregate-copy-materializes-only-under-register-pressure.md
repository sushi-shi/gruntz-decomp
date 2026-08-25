# A struct-copy local that retail STORES and we fold is a register-pressure reading, not a different source shape

tags: cpp:struct cpp:local | asm:mov | topic:wall topic:regalloc
symptoms: retail writes an 8-byte aggregate local to the frame (`mov [esp+N],edx` /
`mov [esp+N+4],eax`) beside the same values it pushes as arguments, while our build
pushes only; the two sides otherwise agree; the stores have no later reader
confidence: 8/10

## Symptom

`CGrunt::StepHitAndRunnerBehavior` 0xed9f0, three sites (SEEK / CHASE / ATTACK arms):

```asm
; retail                                  ; ours
mov  edx,[edi+0x17c]                      mov  ecx,[edi+0x180]
mov  eax,[edi+0x180]                      mov  eax,[edi+0x17c]
mov  [esp+0x18],edx    ; cp.m_x           mov  edx,[edi+0x1f0]
mov  ecx,edx                              push ecx
mov  edx,eax                              push eax
push eax                                  ...
push ecx
...
mov  [esp+0x2c],edx    ; cp.m_y
call CommitNeighbor
```

Source, identical on both sides:

```cpp
Coord cp = g->m_lastTilePx;
CommitNeighbor(g->m_playerIndex, g->m_unitIndex, cp.m_x, cp.m_y);
```

Nothing reads `cp` afterwards, so the stores look like a source difference: a
field-by-field build, a by-value parameter, an inline helper's local.

## Cause

None of those. cl 5.0 scalar-replaces a small aggregate local **only while it has
registers to spare**. When the surrounding body is under pressure the aggregate
keeps its frame slot and the copy's stores are emitted — and they are not
dead-store-eliminated, because DSE needs a later store to the same
`(BASE, offset)` (globalopt catalogue §5) and there is none.

Measured, `/O2 /MT /GX /GR`, one scratch TU:

| probe | shape | result |
|---|---|---|
| `Coord cp = g->lp;` then `Take(hi,lo,cp.m_x,cp.m_y)` | copy-init | **folded**, no stores |
| `Coord cp; cp = g->lp;` | assign | folded |
| `Coord cp; cp.m_x = g->lp.m_x; cp.m_y = g->lp.m_y;` | field-by-field | folded |
| `Coord` with a member function instead of a POD | class type | folded (identical bytes) |
| same copy inside a `static inline` helper | inline body | folded |
| **the copy plus six other live values across two calls** | pressure | **`mov [esp+0x14],ecx` / `mov [esp+0x18],edx` — materialized** |
| `Esc(&cp);` anywhere in the function | address escapes | materialized |

So there are exactly two ways to make cl keep it: take the address, or run out of
registers.

## What it tells you

The stores are a **symptom of retail having MORE live values at that point than
your reconstruction does** — they are not reachable by rewriting the copy. Do not
sweep spellings of the aggregate; go and find the value your body is missing
(usually a member kept in a register across a call, a local retail bound that you
re-derive, or an arm you folded away).

The converse holds and is the useful direction: if YOUR build materializes a copy
retail folds, your body is carrying a value retail did not.

## Related

The address-escape row is the steerable half and it is a different pattern: when
the aggregate's address IS taken (`g->GetScreenPos(c)`), every store to it
survives, which is how `StepHitAndRunnerBehavior`'s `c[0].m_x = c[0].m_x >> TILE_SHIFT_PX;`
in-place shift was recovered - see
[interior-subobject-pointer-is-a-source-local](interior-subobject-pointer-is-a-source-local.md)
and the alias model in
[`../relevations/wall-reasons-globalopt.md`](../relevations/wall-reasons-globalopt.md) §1.
