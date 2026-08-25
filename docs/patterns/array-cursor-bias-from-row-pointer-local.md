# A strength-reduced row cursor's BIAS is picked by how the row is addressed — subscript every field, don't name a row pointer
tags: cpp:loop cpp:local cpp:struct | asm:lea asm:mov | topic:codegen-idiom topic:regalloc
symptoms: every field store/load in a loop body has the SAME displacement error (`[ebx-0x2]` vs `[ebx-0x1]`, `[ebx+0x6]` vs `[ebx+0x7]`); the absolute addresses are identical; only the `lea <cursor>,[esp+N]` preheader and `add <cursor>,<stride>` bracket it
confidence: 9/10

cl5 strength-reduces `packet.rows[i].field` into ONE walking cursor plus disp8s, and it
picks the cursor's BIAS (which byte of the record the register points at) from the *form*
of the address expression, not from the field set. Three forms, three biases, all
otherwise byte-identical: a named row pointer (`CNetPlayerRecord* rec = &pkt.rows[i];`)
anchors at **rows+2**, a walking cursor (`rec++` per iteration) at **rows+0**, and
plain subscripting at every field at **rows+1** — which is retail's.

```cpp
// NO - `rec` biases the cursor to rows+2 (lea ebx,[esp+0x22])
CNetPlayerRecord* rec = &packet.m_rows[i];
rec->m_active = static_cast<u8>(player->m_active);
rec->m_networkPlayerId = player->m_networkPlayerId;

// YES - subscript each field; cursor lands at rows+1 (lea ebx,[esp+0x21])
packet.m_rows[i].m_active = static_cast<u8>(player->m_active);
packet.m_rows[i].m_networkPlayerId = player->m_networkPlayerId;
```
```asm
lea    ebx,[esp+0x21]               ; the BIASED cursor (record base + 1)
mov    BYTE PTR [ebx-0x1],al        ; rows+0
mov    BYTE PTR [ebx],al            ; rows+1
mov    DWORD PTR [ebx+0x7],edx      ; rows+8
add    ebx,0x20
```
STEERABLE. CMulti::BroadcastPlayerTable 0x0ba810 98.69 -> 100 EXACT and its inverse
CMulti::ApplyPlayerTable 0x0ba980 98.19 -> 100 EXACT — both had been filed
"retail anchors at rows+1, eight disp bytes, permute found nothing".

## The three forms do not span the bias space

`CNetSession::Verify(i32)` @0x0c0290 is the negative. Its loop walks
`CNetCmdSlot m_slots[4]` (stride 0x64) and reads four fields at +0x00, +0x04,
+0x08 and +0x14; the absolute addresses agree with retail everywhere, only the
cursor's bias differs. Measured, one full build each:

| source form | bias | score |
|---|---|---|
| `CNetCmdSlot* s = &m_slots[i];` (base) | **row+0x14** | 89.53 |
| `CNetCmdSlot* s = m_slots + i;` | row+0x14 (byte-identical) | 89.53 |
| walking `CNetCmdSlot* s = m_slots; … s++` | **row+0x00** | 78.65 |
| every field subscripted `m_slots[i].f` | row+0x00 | 83.28 |
| `s` from an inline `SlotAt(this, i)` helper | row+0x14 (byte-identical) | 89.53 |
| swapping the last comparison's operands | row+0x14 (byte-identical) | 89.53 |
| **retail** | **row+0x04** | — |

So the named-pointer / walking-cursor / subscript triple collapses to TWO biases
here, and retail's is neither. The bias is not "the last field referenced"
(swapping the final compare's operands is inert) and it is not the encoding
optimum (row+0x04 and row+0x00 both cost five displacement bytes against the
named form's seven). Do not spend a spelling matrix on a bias this pattern's
three forms cannot reach; the row also carries a second readout of the same
fact — retail spells the two zero tests `cmp DWORD PTR [esi],0` where we emit
`mov reg,[esi-0x10]; test reg,reg`, which is not a displacement-zero peephole
(EXACT functions tree-wide use `cmp mem,imm` at nonzero displacements 159 times
against 138 at zero).
