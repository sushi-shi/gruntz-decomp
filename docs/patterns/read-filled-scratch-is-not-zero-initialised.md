# A stream scratch that `Read(&x, 4)` fills is NOT zero-initialised — the `= 0` is an extra store AND permutes the frame slots
tags: cpp:local cpp:serialize | asm:mov | topic:codegen-idiom
symptoms: `mov DWORD PTR [esp+N],0x0` immediately before a `Read(&local, 4)` call that retail does not have; every `[esp+N]` for the deserializer's scratch dwords is permuted at the SAME frame size
confidence: 9/10

A deserializer's scratch dwords are declared uninitialised and filled by the stream
read. Writing `i32 seq = 0; s->Read(&seq, 4);` emits a dead store retail lacks, and
because the extra store gives the local an earlier definition point it also changes
which local wins the pushed slot vs. the dead parameter home — so every scratch
displacement in the function shifts, not just the one.

```cpp
// NO
i32 seq = 0;
s->Read(&seq, 4);

// YES - Read is the definition
i32 seq;
s->Read(&seq, 4);
```
```asm
lea    eax,[esp+0x14]
push   0x4
push   eax
mov    ecx,esi
call   DWORD PTR [edx+0x2c]          ; no `mov [esp+..],0` in front of it
mov    edx,DWORD PTR [esp+0x14]
```
STEERABLE. CStatusBarMgr::Deserialize 0x109520 96.11 -> 99.20: two such `= 0`s
(`seq`, `cnt`) plus the null-JOIN spelling of the looked-up sprite. Only genuine
out-params that the callee may leave untouched (an MFC `Lookup` out-slot) keep the
`= 0` — and that store's position is the known-immovable one
(out-param-zero-init scheduling).
