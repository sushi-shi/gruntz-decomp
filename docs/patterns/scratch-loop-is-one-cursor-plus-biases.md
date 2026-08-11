# A memcpy-to-scratch pixel loop runs off ONE cursor plus integer BIASES, not N cursors
tags: cpp:loop cpp:local cpp:pointer | asm:sub asm:add asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail's preheader has `mov reg,<scratch> / sub reg,<other>` pairs and the loop body indexes `[cursor+bias]`; base instead carries a SECOND induction variable spilled to a stack slot (`mov edx,[esp+N] / add edx,2 / mov [esp+N],edx` every iteration); `reloc_multiset` shows the scratch global referenced FEWER times in base than in retail
confidence: 9/10

The blit/convert arms copy the destination row into a scratch buffer and then
walk destination, source and scratch together. Retail keeps exactly ONE walking
pointer and expresses the other two as loop-invariant integer BIASES computed in
the preheader; spelling two or three parallel cursors makes cl strength-reduce
each into its own induction variable, and the extras get spilled. Which pointer
is the surviving cursor is per-arm - read the preheader's `sub` operands.

```cpp
// NO - two cursors: cl carries `sc` as a second spilled IV
u8* sc = g_scratch;
memcpy(g_scratch, d, count * 2);
while (count-- > 0) { u32 i = pal2[Load16(sc)]; ...; Store16(d, v); d += 2; sc += 2; }

// YES - one cursor (d) plus a bias
memcpy(g_scratch, d, count * 2);
i32 sc = g_scratch - d;
while (count-- > 0) { u32 i = pal2[Load16(d + sc)]; d += 2; ...; Store16(d - 2, v); }
```
```asm
mov    edi,0x6bed08          ; scratch
sub    edi,ebp               ; bias = scratch - d      (preheader)
mov    dx,WORD PTR [eax+edi] ; Load16(d + bias)
add    eax,0x2               ; the ONLY cursor moves
mov    WORD PTR [eax-0x2],cx
```
STEERABLE. `CDDrawShadeBlit::BlitShadedForward` 0x14a200 68.82 -> 70.33 (the
DST_BY_SRC_16 and ALPHA_16 arms at 0x14a7b5/0x14a874). Two corollaries measured
in the same function: `u8* sc = g_scratch;` belongs AFTER the `memcpy`, not
before (retail rematerialises `mov ecx,0x6bed08` once the rep-movs is done), and
`Store16(sc+db,v); Store16(sc+db+rd,v);` must be split through an explicit
`u8* p = sc + db;` or cl reassociates to `sc + (db+rd)` and folds `rd` into a
scaled `lea`.
