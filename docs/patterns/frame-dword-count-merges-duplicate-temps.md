# Retail's frame dword COUNT is the count of scalar locals — a repeated idiom that shares ONE retail slot is ONE variable
tags: cpp:local cpp:loop | asm:sub | topic:codegen-idiom
symptoms: long serializer/streamer whose every instruction matches but the frame is 4/8/... bytes too big and every `[esp+M]` local is permuted; the same "write a count, then loop over it" idiom appears N times
confidence: 8/10
variants: stack-buffer-size-drives-frame.md, stack-slot-coalesce-frame-4b.md

MSVC 5.0 gives each *variable* a slot and reuses a slot across **disjoint lexical
scopes** only. So when retail's frame has FEWER dwords than your distinct locals, and a
body repeats one idiom (`n = coll.GetSize(); ar->Write(&n,4); for (i<n) ...`) several
times at *function* scope, retail declared **one** temp and reused it — three `i32 c0`,
`cnt`, `c1` locals cannot share, but one `count` trivially does. Count retail's dwords
first (frame minus the buffers), then merge until the counts agree.

```cpp
i32 count;                       // ONE reused count temp - retail has 3 dwords total
count = markerCount();     s->Write(&count, 4); for (u32 i=0;i<(u32)count;i++) ...
for (i32 k=0;k<4;k++) { count = arrCount(k); s->Write(&count,4); for (...) ... }
count = arr488Count();     s->Write(&count, 4); for (...) ...
```
```asm
sub  esp,0x28c   ; = 0x200 buffer + 0x80 buffer + 3 dwords (NOT 5)
mov  DWORD PTR [esp+0x10],ecx    ; the ONE count slot, written at all three sites
```
STEERABLE. `CPlay::SyncWrite19fb` @0xd79d0 99.77 -> **100 EXACT** (frame 0x294 -> 0x28c;
the two 0x80 message buffers already shared one slot because their blocks are disjoint,
which is the same rule seen from the other side). Note the second, separate fix in the
same body: a block that reads a member *before* zeroing its out-param stores the 0 as an
IMMEDIATE, while zeroing first lets cl reuse the `rep stos` zero register — so hoist the
member read when retail's store is `mov [esp+M],0x0`.
