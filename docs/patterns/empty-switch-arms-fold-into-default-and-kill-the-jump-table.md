# Empty switch arms fold into `default` and kill the jump table — `default: goto tail;` keeps them
tags: cpp:switch cpp:goto | asm:jmp asm:cmp | topic:codegen-idiom
symptoms: retail `add eax,-N / cmp eax,M / ja / jmp [eax*4+tbl]`, ours a `cmp/je` chain, ~50%
confidence: 9/10
variants: switch-cmpje-tree-vs-jumptable.md

Retail lowers a switch to a dense jump table whose entries for the EMPTY arms point at the same
block the out-of-range `ja` does. cl5 will not reproduce that from the obvious source: it folds
every arm whose only statement is `break` into the default FIRST, so the jump-table density
decision sees only the non-empty arms and emits a `cmp/je` chain instead. Adding the empty
`case` labels does nothing on its own. **Give `default` a syntactically distinct target and the
arms survive the decision** — `goto` a label placed exactly where the switch already falls
through, so the emitted target is still the shared tail.

```cpp
// NO - cl folds 3/5/6/8 into default, sees 2 arms, emits cmp/je:
switch (kind) { case 3: break; case 4: A(); break; case 5: break;
                case 6: break; case 7: B(); break; case 8: break; }
Tail();

// YES - six table entries, all the empty ones landing on the shared tail:
switch (kind) { case 3: break; case 4: A(); break; case 5: break;
                case 6: break; case 7: B(); break; case 8: break;
                default: goto tail; }
tail:
Tail();
```
```asm
add    eax,0xfffffffd          ; kind - 3
cmp    eax,0x5                 ; six entries, 3..8
ja     <tail>
jmp    DWORD PTR [eax*4+<tbl>] ; entries for 3/5/6/8 == <tail>
```
STEERABLE. `CGameLevel::EditDispatch` 0x160f70 **49.78% -> 92.55%** (filed as a
"switch jump-table-density wall"; the remaining residue is the delinker's jump-table
symbol naming, see jumptable-data-overlap). CAVEAT: spelling the tail out *inside* `default`
instead of `goto`-ing also builds the table but leaves a full duplicate of the tail (+50 B).
This refutes "you cannot force the lowering from source" in
[[switch-cmpje-tree-vs-jumptable]] for the empty-arm case.
