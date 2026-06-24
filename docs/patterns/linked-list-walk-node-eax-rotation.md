# Singly-walked CObList delete-loop rotates node through eax (twin copies) — regalloc wall
tags: cpp:loop cpp:local | asm:mov asm:cmp | topic:wall topic:regalloc
symptoms: list walk `mov reg,[this+head]`; target head→eax + `mov ecx,eax`/`mov esi,eax` twin copies + `mov eax,[eax]` advance + arg hoisted to edx; recompile keeps head/node in ONE callee-saved esi and derefs it directly
confidence: 7/10

A `find-node-then-RemoveAt(+delete)` walk over a CObList/CPtrList (head @ a
member offset; node {next@+0x00, data@+0x08}) where the matched node must survive
an in-loop call (`RezFree`/element-dtor) so it lives callee-saved.  Retail loads
the head into **eax**, hoists the search arg to **edx** before the loop, and each
iteration copies node into TWO regs (`mov ecx,eax` for the data deref, `mov
esi,eax` as the surviving RemoveAt position) then advances `mov eax,[eax]`.  No
natural source spelling (do-while, while, for; single `node` vs `cur`+`node`
split; arg copied to a local) reproduces this: MSVC5 coalesces head+node into one
callee-saved **esi** and derefs `[esi+8]`/`[esi]` directly, a tighter but
differently-rotated loop.

```cpp
for (Node* n = m_list.m_pNodeHead; n != 0; n = n->m_next) {
    Elem* e = (Elem*)n->m_data;
    if (e == (Elem*)arg) { /* delete e */ m_list.RemoveAt(n); return 1; }
}
```
```asm
; retail: head in eax, arg hoisted to edx, twin node copies
mov eax,[edi+0x58] ; head
mov edx,[esp+0xc]  ; arg (hoisted out of loop)
mov ecx,eax        ; node copy (data deref)
mov esi,eax        ; node copy (RemoveAt position, survives the call)
mov eax,[eax]      ; advance
mov ecx,[ecx+8]    ; data
cmp ecx,edx
```
WALL: logic + body identical, only the loop's register rotation differs.  Evidence:
CTileTriggerContainer DelFromList1 69%, DelFromList3 82%, MoveList1ToList2 51%,
FilterList2 84% — all the same head→esi-direct vs head→eax-twin-copy shape; the
sibling FindInLists12 (no in-loop call, node need not survive) reaches 100%.
