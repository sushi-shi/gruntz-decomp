# Multi-way error layout: nest as success-DEEPEST with errors as the trailing else-cascade, NOT early-returns — floats cold blocks to the tail
tags: cpp:branch | asm:jcc asm:jmp | topic:codegen-idiom
symptoms: error/failure blocks emitted INLINE via `jne`-skip where the target lays them at the function TAIL reached by forward `je`; 21-50% on an otherwise-correct getter
confidence: 8/10

When the target lays its failure blocks at the function TAIL (reached by forward `je`) and keeps
the success path inline, reproduce it by nesting so the SUCCESS path is DEEPEST and each error is
a trailing `else`/fall-through, NOT a series of early `return`s:

```cpp
if (grp) {
    rec = Find(key);
    if (rec) { <type-check> err1; return E; }   // success deepest
    err2; return E;
} err3; return E;                                // errors cascade to the tail
```

Early-returns (`if(!grp){err;return} …`) emit each error INLINE via `jne`-skip (21-50%). `goto`s
to bottom labels do NOT help — cl inlines them; the nested-if-success-deepest form is what floats
the cold blocks to the tail. (Same family as the AdvancedOptions/Method02 single-`goto Fail;`
shared-epilogue idiom: separate `return 0`s create an extra epilogue.)

STEERABLE. Evidence: CButeMgr 3-way error getters → 99%+ (early-returns: 21-50%); CGameApp::
Method02 chained-`&&` validation sharing one `goto Fail;`. related: switch-cmpje-tree-vs-jumptable.md
(the type-check polarity inside), default-then-override-flag.md.
