# A by-value accessor only materialises retail's frame home when it is an UNNAMED temporary
tags: cpp:inline cpp:struct cpp:member cpp:temporary | asm:mov | topic:codegen-idiom
symptoms: retail stores an 8-byte Coord/POINT into the frame and never reads it back, loads one member TWICE, and the base emits neither; a named `T c = obj->m_field;` copy was tried and the home still disappears
confidence: 10/10
variants: a-dead-stack-store-is-not-an-era-anomaly.md, dead-second-field-load-is-a-struct-copy.md

The dead-store pattern says an inlined by-value accessor materialises a frame temp
whose unread half is left stored dead. The step that is easy to miss, and that
parked five `bounded` rows: **naming the result defeats it.** `Coord c =
obj->LastTilePx();` is copy-elided into an ordinary local, and cl 5.0 deletes an
ordinary local's field stores - byte-for-byte identical to `Coord c =
obj->m_lastTilePx;`. The temp survives only where it is never named, i.e. one
accessor call per field, at the use site.

```cpp
// NO - byte-identical to the raw member copy; C2 deletes the home
Coord c = target->LastTilePx();
CommitNeighbor(target->m_playerIndex, target->m_unitIndex, c.m_x, c.m_y);

// YES - two temps share one 8-byte slot, both stores dead, both reads folded
CommitNeighbor(target->m_playerIndex, target->m_unitIndex,
               target->LastTilePx().m_x, target->LastTilePx().m_y);
```
```asm
mov  eax,[edi+0x17c]      ; m_x for the .m_y-call's temp
mov  ecx,[edi+0x17c]      ; m_x AGAIN - the folded read that becomes the argument
mov  [esp+0x10],eax       ; temp+0 = m_x   DEAD
mov  eax,[edi+0x180]      ; m_y, feeds the argument AND the other temp's store
mov  edx,eax
push eax
...
mov  [esp+0x14],edx       ; temp+4 = m_y   DEAD
call ?CommitNeighbor@CGrunt@@QAEHHHHH@Z
```

The tell is the DOUBLE LOAD of one member with a single dead store of it: a named
copy would load each member once. Measured 2026-08-23 on `COMMIT_GRUNT_NEIGHBOR`
across 29 sites in 14 TUs: 14 functions moved, 13 up and one down 0.22, net
+51.04, `CGrunt::StepPostGuardBehavior` 86.54 -> 100.00 EXACT, `StepDumbChaserBehavior`
83.91 -> 90.06, `StepDiggerBehavior` 84.24 -> 90.01 (which also closed one of its
two branch deltas), `StepScrollGruntBehavior` 85.82 -> 91.16. It overturned five
`bounded` verdicts, four of which had explicitly recorded the surviving home or
the "retail-only aggregate reload" as the residue with no lever left - the
exhaustion was over spellings that all NAME the copy.
