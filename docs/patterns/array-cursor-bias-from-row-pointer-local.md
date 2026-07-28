# A strength-reduced row cursor's BIAS is picked by how the row is addressed — subscript every field, don't name a row pointer
tags: cpp:loop cpp:local cpp:struct | asm:lea asm:mov | topic:codegen-idiom topic:regalloc
symptoms: every field store/load in a loop body has the SAME displacement error (`[ebx-0x2]` vs `[ebx-0x1]`, `[ebx+0x6]` vs `[ebx+0x7]`); the absolute addresses are identical; only the `lea <cursor>,[esp+N]` preheader and `add <cursor>,<stride>` bracket it
confidence: 9/10

cl5 strength-reduces `packet.rows[i].field` into ONE walking cursor plus disp8s, and it
picks the cursor's BIAS (which byte of the record the register points at) from the *form*
of the address expression, not from the field set. Three forms, three biases, all
otherwise byte-identical: a named row pointer (`CNetChannelRow* rec = &pkt.rows[i];`)
anchors at **rows+2**, a walking cursor (`rec++` per iteration) at **rows+0**, and
plain subscripting at every field at **rows+1** — which is retail's.

```cpp
// NO - `rec` biases the cursor to rows+2 (lea ebx,[esp+0x22])
CNetChannelRow* rec = &packet.m_rows[i];
rec->m_liveGate = (u8)ch->m_liveGate;
rec->m_slotKey  = ch->m_slotKey;

// YES - subscript each field; cursor lands at rows+1 (lea ebx,[esp+0x21])
packet.m_rows[i].m_liveGate = (u8)ch->m_liveGate;
packet.m_rows[i].m_slotKey  = ch->m_slotKey;
```
```asm
lea    ebx,[esp+0x21]               ; the BIASED cursor (record base + 1)
mov    BYTE PTR [ebx-0x1],al        ; rows+0
mov    BYTE PTR [ebx],al            ; rows+1
mov    DWORD PTR [ebx+0x7],edx      ; rows+8
add    ebx,0x20
```
STEERABLE. CMulti::BroadcastChannelTable 0x0ba810 98.69 -> 100 EXACT and its inverse
CMulti::ParseChannelTable 0x0ba980 98.19 -> 100 EXACT — both had been filed
"retail anchors at rows+1, eight disp bytes, permute found nothing".
