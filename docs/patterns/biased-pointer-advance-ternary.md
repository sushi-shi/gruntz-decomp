# Biased pointer advance in a loop: spell it as a single ternary, not load-then-conditional-fixup
tags: cpp:loop cpp:branch cpp:local | asm:je asm:jmp asm:xor | topic:codegen-idiom
symptoms: a `while(p)` chain walk whose advance reads `p=p->next` then applies a constant pointer bias (`p -= 4`) only when non-null; recompile SHORT-CIRCUITS the null case straight to the loop exit (`mov [p+off],p; test; je end; sub C; test; jne top`) while retail keeps a single loop-back test (`mov [p+off],p; test; je SETNULL; sub C; jmp TEST; SETNULL: xor p,p; TEST: test; jne top`)

An intrusive list whose nodes are stored "biased" (the chain threads `node = entry+K`,
so each step is `entry = entry->next ? entry->next - K : 0`). Writing the advance as
`p = p->next; if (p) p -= K;` lets MSVC fuse the inner null-test with the `while(p)`
loop test into one early exit. Writing it as a **single ternary** assigning `p` once
(`p = (n = p->next) ? n - K : n`) preserves retail's structure: the else-branch
materializes `0` (`xor`), both arms converge at one loop-back `test/jne`.

```cpp
// NOT:  e = e->next; if (e) e = (T*)((char*)e - 4);   // folds to an early je-end
T* n = e->m_next;
e = n ? (T*)((char*)n - 4) : n;                          // one assignment -> single loop test
```
```asm
mov  eax,[eax+off]        ; n = e->next
test eax,eax
je   SETNULL             ; null arm
add  eax,-4              ; n - K
jmp  TEST
SETNULL: xor eax,eax     ; e = 0
TEST:    test eax,eax
jne  TOP                 ; while(e)
```
STEERABLE. Evidence: CHash Walk (0x13c270/0x13c3f0) + FindInt (0x13c360) each 94%→100% / 92%→100% on the ternary rewrite (src/Bute/Hash.cpp); the four chain-walks went exact.
