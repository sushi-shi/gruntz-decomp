# Term order in a pure-load integer sum is inert - cl reassociates it
tags: cpp:expression | asm:add | topic:wall
symptoms: retail accumulates member offsets in a different order than the source lists them; reordering the source terms leaves the emitted `add` sequence byte-identical
confidence: 8/10

A long `a + b + c + ...` of plain member loads is reassociated by cl 5.0 /O2,
so the emitted accumulation order is chosen by the register allocator, not by
the source. Permuting the terms to chase retail's order changes nothing.

```asm
; retail
mov ebp,DWORD PTR [eax+0x444]
add ebp,DWORD PTR [eax+0x3f0]
add ebp,DWORD PTR [eax+0x3f4]
add ebp,DWORD PTR [eax+0x3ec]
; ours - a different order, and permuting the source does not move it
mov ebp,DWORD PTR [eax+0x3ec]
add ebp,DWORD PTR [eax+0x3f4]
add ebp,DWORD PTR [eax+0x3f0]
add ebp,DWORD PTR [eax+0x444]
```
Wall for this shape. Measured on `CNetSession::Checksum` 0xc0590: all three of
its sums were rewritten into retail's emitted order and the emitted order did
not move at all (89.757 -> 89.743, jitter). The corollary is the useful part -
a differing accumulation order is NOT evidence of a differing source order, so
do not read member identities off it. `CNetSession::BuildGruntzCrcInfo` 0xbf1d0
proves the identities instead, because its `wsprintfA` argument list is
positional: `[health=%d]` is fed `[esi+0x3ec]` on BOTH sides. What DOES move
the sum order there is register pressure - retail keeps a dead 8-byte
`Coord` copy in the frame that our body folds away.
