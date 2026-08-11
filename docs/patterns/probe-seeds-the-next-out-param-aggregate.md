# A probe's UNUSED component is stored into the aggregate the NEXT probe overwrites
tags: cpp:local cpp:struct cpp:call | asm:mov | topic:codegen-idiom topic:regalloc
symptoms: dead store, second store of the same value, out-param, GetScreenPos, GetScreenTile, Coord, frame 4 bytes short, extra `mov [esp+N],eax` before a call
confidence: 9/10
variants: dead-store-survives-only-across-an-unrelated-store.md, struct-copy-defeats-the-field-load-cse.md

A run of probes into distinct out-param aggregates (`GetScreenPos(&c)` /
`GetScreenTile(&c)`), each of which uses only ONE of the two fields, emits an
extra store of the OTHER field into a slot nothing reads. The slot is always
the aggregate a LATER probe in the same run fills, so the store is dead the
moment that probe runs — cl cannot prove it dead, because the address escapes
to the call, so it survives and it is load-bearing for the whole run's
register colouring. Reading it as a spill is the trap: the store is in the
SOURCE.

```cpp
Coord cA, cB, cC, cD;
unit->GetScreenTile(&cA);
cD.m_x = cA.m_x;     // seed: cA's unused half, into the LAST probe's Coord
bottom = cA.m_y;
unit->GetScreenTile(&cB);
cD.m_y = cB.m_y;     // seed
right = cB.m_x;
unit->GetScreenTile(&cC);
cD.m_x = cC.m_x;     // seed
top = cC.m_y;
unit->GetScreenTile(&cD);
left = cD.m_x;
```
```asm
    sar    eax,0x5
    push   ecx
    mov    ecx,ebx
    mov    DWORD PTR [esp+0x34],eax     ; cA.m_x  (the probe's own slot)
    mov    DWORD PTR [esp+0x64],eax     ; SEED - cD.m_x, never read
    call   0x36c0                       ; next probe
```
STEERABLE. `CBattlezMapConfig::RouteToNearbyEnemy` 66.08 -> 67.48 (the whole
four-probe header then matches retail instruction for instruction modulo
register naming); `CBattlezMapConfig::ResolveTileClaim` 77.45 -> 79.68.
`CGrunt::StepPhase` already spelled it (`pb.m_y = pa.m_y;` before
`GetScreenPos(&pb)`) — that block is the corroborating second site.
The seed target must be a probe target that ALREADY exists: adding a fresh
Coord for it grows the frame and loses more than the seed buys
(`ResolveArrival` 87.53 -> 86.99 with a function-scope `Coord c`, frame 0x94
-> 0x9c), and seeding a SECOND run inside a loop cost 67.48 -> 66.89. Also
refuted here: a by-value `Coord GetScreenTile()` — it explains the extra
Coord-sized temps, but cl elides the return copy so the predicted stores never
appear.
