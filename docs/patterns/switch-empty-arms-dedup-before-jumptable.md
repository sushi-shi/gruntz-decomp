# Empty `break` switch arms are de-duplicated BEFORE the jump-table decision — spell them `return <v>`

tags: cpp:switch cpp:return | asm:jmp asm:cmp | topic:codegen-idiom
symptoms: retail has a dense `add eax,-<lo> / cmp eax,<n> / ja <default> / jmp [eax*4+<table>]`
over a contiguous case range in which only two or three arms do anything; the recompile emits a
`sub/je / dec/je / sub/jne` compare ladder over just the ACTIVE case values; the `.rdata` jump
table in the EXE has one entry per case label but only 2-3 distinct targets
confidence: 9/10
variants: switch-cmpje-tree-vs-jumptable.md, switch-density-byte-index-table-vs-tree.md

The lowering choice is a density test on the number of *case labels* cl still has when it makes
the decision — and cl5 collapses arms whose bodies are **identical** first. Four arms that are
just `break;` are identical to each other AND to the switch's fall-through, so a 6-label switch
with 4 empty arms is already a 2-label switch by the time the test runs, and you get the ladder.
Giving each inactive arm its own `return <the same value>;` keeps six distinct arms alive through
the decision; the table is emitted, and the arms tail-merge back to one block afterwards — which
is why retail's table has 6 entries and only 3 distinct targets.

**The ACTIVE arms must still `break`.** Spelling `case 4: if (!Save(s)) return 0; return 1;`
makes cl recognise `return Save(s) != 0` and emit the bool-normalise `neg/sbb/neg`; retail's
`test eax,eax / jne <the shared return 1>` only comes out of a `break` into the shared tail.

```cpp
// retail: jump table over 3..8, cases 4 and 7 active
switch (kind) {
    case 3: return 1;                              // inactive: own `return`
    case 4: if (!Save(s)) return 0; break;         // active: `break` to the shared tail
    case 5: return 1;
    case 6: return 1;
    case 7: if (!Load(s)) return 0; break;
    case 8: return 1;
}
return 1;
```

Probe it in 30 s rather than guessing — five spellings of the same switch in one `.cpp`, one
`cl /O2 /MT`, `llvm-objdump -d`: `break` arms → ladder, `return` arms → table, mixed → table plus
retail's `test/jne`. Read the retail table out of the EXE first (`jmp [eax*4+T]` gives T; the
entry count is `cmp eax,N` + 1) — it tells you exactly how many case labels the source had.

Evidence (2026-07-28): `CDDrawWorkerHost::SerializeDispatch` @0x163710 — filed as a
"jump-table-shape wall … forcing 6 explicit cases still merges them (78%)"; the missing half was
the `return 1` on the inactive arms. All 66 bytes now agree with retail (the objdiff score
*drops* to 68% because the jump table splits our symbol into `$L<n>` pieces — see
jumptable-data-overlap.md).
