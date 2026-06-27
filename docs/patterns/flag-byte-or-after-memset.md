# Flag byte set after a memset: write `|=`, not `=` (read-modify-write)
tags: cpp:struct cpp:local | asm:or asm:mov | topic:codegen-idiom
symptoms: a packet/blob field is `memset` to 0 then one bit set; retail does `movb [x],cl; or cl,0x80; movb cl,[x]` where you wrote a direct `movb $0x80,[x]`; the surrounding flag/id stores also shift

A stack packet/blob is zeroed (memset → rep stos) and then a flag byte gets a bit
set (commonly the high bit, `0x80`). Retail reads the just-zeroed byte back,
ORs the bit in, and stores — i.e. the source wrote `field |= 0x80`, a
**read-modify-write**, NOT `field = 0x80`. Writing the direct assignment emits a
single `movb $imm,[x]` and (because the value never passes through a register)
also re-permutes the adjacent flag/id stores, so the whole prologue diverges.

```cpp
memset(&blob, 0, sizeof(blob));
blob.m_0 |= 0x80;   // retail: load the memset'd 0, or 0x80, store back
// NOT blob.m_0 = 0x80;  -> single movb imm, shifts the neighbouring stores
```
```asm
movb 0xc(%esp), %cl      ; reload the memset'd flag byte
movl $0x416, 0x10(%esp)  ; (a neighbouring field, scheduled between)
orb  $-0x80, %cl         ; |= 0x80
movb %cl, 0xc(%esp)      ; store back
```
Steerable (try-and-measure). CNetMgr::SaveConfig @0x000bccd0 jumped 89.9%→96.4% on
this one change (the residual after it is only EH-cookie + reloc-name). The tell is
the `or` against a freshly-`rep stos`-zeroed slot — never a plain `mov imm` when
retail shows an `or`. Context note: when the flag sits in a long block of adjacent
byte-stores whose schedule already diverges (e.g. CreateLocalPlayer @0xbc750, a
push-shifted dest), the `|=` can re-permute those neighbours and net slightly worse
than `=` — so apply it and KEEP whichever scores higher.
