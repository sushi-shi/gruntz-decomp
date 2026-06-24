# Singly-linked walk: advance the node BEFORE processing to avoid the loop peel
tags: cpp:loop cpp:local | asm:mov asm:test asm:jcc | topic:codegen-idiom
symptoms: a `node = node->next` list walk recompiles with a peeled first iteration (the body emitted twice), retail has ONE body that reads `next` at the loop top

When you walk `for (node=head; node; node=node->next)` and read `node->next` at the
BOTTOM (or with the increment in the `for`-clause), MSVC5 /O2 often **peels the first
iteration** — emitting the body twice — because the head guard makes entry non-null.
Retail instead carries the *node-to-process* in one register, reads `node->next` at the
loop TOP, then processes the saved node, looping on the freshly-read next. Spell that
shape literally: copy `node` to a temp, advance `node` first, then process the temp.

```cpp
// after the `if (head == 0) return …;` guard:
Node* node = head;
while (node != 0) {
    Node* cur = node;       // save the node to process
    node = node->m_next;    // advance FIRST (eax = node->next at loop top)
    ... use cur ...
    if (hit) return 1;
}
return 0;
```
```asm
loop:  mov  ecx, eax          ; cur  = node
       mov  eax, (%eax)       ; node = node->next   (advance before process)
       ...                    ; ... use cur (ecx) ...
       test eax, eax
       jne  loop              ; while next != 0
```
STEERABLE. Both the `do { next = node->next; …; node = next; } while(next)` and
`for(;;){ next=…; …; if(!next)break; node=next; }` spellings PEEL (~46-63%); only the
advance-first `while` closes it. Evidence: UnknownClassArrays::Method_030530 46%→**100%**
(occupied-coord blocked-tile scan); same fix lifts Method_0305b0's inner list loop.

Also fixes the **pool-drain** variant (`for each node: delete node->data; RemoveAll()`)
where reading `node->data` (off the live walker `esi`) BEFORE advancing is byte-different
from retail's `mov eax,esi`(twin) / `mov esi,[esi]`(advance) / `mov ecx,[eax+8]`(deref via
twin): the same `cur=node; node=node->next; item=cur->data;` reorder materialises the eax
twin and closes it. Evidence: CDDrawPtrCollections::EmptyPoolA (0x142120) 92.4%→**100%**,
EmptyPoolB (0x142ed0) 87%→**99.9%**. (The find+RemoveAt walk where the node must survive an
in-loop call is still the wall — see related.)
related: linked-list-walk-node-eax-rotation.md.
