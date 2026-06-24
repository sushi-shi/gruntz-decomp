# Constant-shift RGB pack via `mov ch,al; mov cl,bl` byte-insert
tags: cpp:bitop | asm:mov asm:shl | topic:wall topic:regalloc
symptoms: packing three `rand()%256` channels into a 24-bit color; retail assembles two channels into one 16-bit register with `mov ch,al; mov cl,bl; shl ecx,8; or ecx,esi` while the recompile emits two separate `shl 8; or`

Three 8-bit channels (`r,g,b`, each `rand() % 256` — the signed-modulo
`cdq; xor; sub; and 0xff; xor; sub` idiom) packed into a single 24-bit color
`(b<<16)|(g<<8)|r`. When the optimizer happens to land `g` in a byte-addressable
register (`ebx→bl`) and `b` in `al`, it assembles the high two channels with a
**byte-insert**: `mov ch,al` (b → bits 8..15), `mov cl,bl` (g → bits 0..7),
`shl ecx,8` → `(b<<16)|(g<<8)`, `or ecx,esi` (`r`). The recompile instead keeps
`g` in a non-byte register (`edi`) and emits two `shl 8; or` shifts — same value,
different bytes.

```asm
; retail (target): r→esi, g→ebx(bl), b→eax(al)
mov ch,al ; mov cl,bl ; shl ecx,8 ; or ecx,esi ; mov eax,ecx ; mov [edi+0x1b8],eax
; recompile: g→edi
mov edi,eax ; ... ; shl eax,8 ; or eax,edi ; shl eax,8 ; or eax,esi
```

```cpp
i32 r = rand() % 256, g = rand() % 256, b = rand() % 256;
color = (b << 16) | ((g & 0xff) << 8) | (r & 0xff); // value-correct, byte-merge not steerable
```

**Not source-steerable.** The byte-insert depends purely on which channel the
allocator parks in a byte-addressable register at the merge point; reordering the
expression (`((b<<8|g)<<8)|r`) makes it *worse* (66% vs 70%). The body is complete
and value-correct — accept the ~70% plateau. Compounded here by a shrink-wrapped
callee-save push (retail defers `push esi/edi` past the two early-return guards;
see shrink-wrapped-callee-save-push.md). Evidence: CCreditsState::FlashColor
(0x39d00), gamemode unit.
