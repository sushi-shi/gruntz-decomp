# MFC map/list scan with early-return: top-tested `while (pos)`, NOT `if (pos) do{}while(pos)` — kills the loop peel

tags: cpp:loop cpp:mfc cpp:branch | asm:test asm:jcc asm:call | topic:codegen-idiom
symptoms: a `CMapStringToOb`/`CMapStringToPtr` `GetStartPosition()` + `GetNextAssoc()` scan with an early `return` emits the loop body TWICE (peeled first iteration), ~61% on an otherwise-correct predicate
confidence: 9/10

An MFC associative-container scan that early-returns on a hit —

```cpp
POSITION pos = m_map.GetStartPosition();
if (pos != 0) {                              // outer guard
    do {
        m_map.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) return 1;
    } while (pos != 0);
}
return 0;
```

— makes MSVC5 **peel the first iteration**: it emits the `GetNextAssoc`+compare body once
(unconditionally, guaranteed by the guard), then the `pos != 0` re-test, then a SECOND copy of the
body that loops back to the re-test. Two body copies → ~61%. Retail is a **single** body that loops
to the top (`add ebx,0x10` hoisted once, one `GetNextAssoc`, `je hit`, `mov eax,[pos]; jne top`).

Fix — spell it as a top-tested `while`; MSVC rotates it into `test; je end; loop: body; test; jne
loop` = retail's single-body form:

```cpp
POSITION pos = m_map.GetStartPosition();      // real MFC call; inlines to neg;sbb (count!=0?-1:0)
while (pos != 0) {
    m_map.GetNextAssoc(pos, key, val);
    if (strncmp(key, str, len) == 0) return 1;
}
return 0;
```

Two more load-bearing details for the exact match:

1. **Use the real `GetStartPosition()`**, not a `(POSITION)(m_map.GetCount() != 0 ? -1 : 0)` hand-
   roll wrapped in a `*(volatile int*)&pos` guard. Both inline to the same `mov eax,[this+0x1c];
   neg eax; sbb eax,eax`, but the `volatile` forces a spurious `mov eax,[pos]` re-read at the guard
   AND helps trigger the peel.
2. **Declare the `CObject* val` local AFTER the `CString key` local.** The `val = 0` store then
   schedules *after* the `CString` ctor call (which clobbers eax) → retail's `mov dword [esp+N], 0`
   immediate store. Declared first, MSVC reuses the `eax = 0` left over by the inlined `strlen`
   (`repne scasb`) → `mov [esp+N], eax`, a 1-instruction miss (~96% instead of 100%).

**Accumulate/side-effect variant** (a `RemoveKey`+count loop with NO early return, e.g.
`RemoveKeysEqual_*`): the `while` conversion still applies (no peel here, but keeps the CFG), and the
init-scheduling levers matter even more. Retail schedules the `int n = 0` accumulator into the
middle of the inlined `GetStartPosition` (between `sbb` and `test`), and `val = 0` as an immediate.
To reproduce, declare the locals in this order and MSVC schedules each init exactly where retail
does:

```cpp
CString key;
CObject* val = 0;             // BEFORE n → val=0 stores an immediate, not eax/ebp reuse
POSITION pos = m_map.GetStartPosition();
int n = 0;                    // AFTER pos → n=0 (xorl ebp) lands after the EH-state store, not before
```

`n` declared before `pos` puts `xorl ebp,ebp` ahead of the compiler's `mov byte [esp+N],1` EH-state
store (a 2-instruction order swap, ~98%); moving it after `pos` fixes it. `RemoveKeysEqual_1527d0/
157c70/155360` 91.67% → 100%.

**By-value `CString` return variant** (a reverse-lookup returning the found key, e.g.
`KeyOfValue_*`/`FindKeyOfValue_*`): the `while` conversion still kills the peel, and two extra
levers close what was mis-filed as an "NRVO wall":
- The no-match tail `return key;` must be preceded by `key.Empty();` when retail emits a trailing
  `lea ecx,[key]; call CString::Empty` there — the reconstruction had simply dropped it.
- A no-match `return CString();` that retail lowers as a MATERIALIZED empty temp + copy-ctor (an
  extra 4-byte CString slot → `sub esp,N+4`) must be spelled `CString empty; return empty;`; a bare
  `return CString()` RVOs the temp straight into the return slot (`sub esp,N`, ~87%).

`KeyOfValue_152d30`/`FindKeyOfValue_158570`/`FindKeyOfValue_165360` 68.77/70.77/79.06% → 100%.

STEERABLE — supersedes the old "optimizer loop-peel wall / zero-register-pinning / NRVO wall"
@early-stop on these. Evidence: `CDDrawSubMgrLeaf::HasKeyPrefix_152c50`, `CDDrawSubMgrLeafScan::HasKeyEqual_1583c0`,
`CDDrawWorkerRegistry::HasKeyEqual_155550` — all 61.36% → 100%. Same peel family as
[linked-list-advance-before-process](linked-list-advance-before-process.md) and
[retry-loop-bail-while-goto-no-peel](retry-loop-bail-while-goto-no-peel.md).
