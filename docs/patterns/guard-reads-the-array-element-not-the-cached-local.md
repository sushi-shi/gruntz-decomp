# A doubled `test` survives when the guard re-reads the ARRAY ELEMENT, not the cached local
tags: cpp:branch cpp:loop cpp:expr | asm:test asm:jcc | topic:codegen-idiom
symptoms: retail has the SAME `test reg,reg` twice back-to-back on one register, with the two `je`s going to DIFFERENT destinations (one continues the loop, one falls into a materialised false arm); the base has only the first; the load feeding both is emitted once on both sides
confidence: 9/10 (measured to EXACT on one site; mechanism is the syntactic peephole already documented for `cmp`)
variants: redundant-test-elimination-is-syntactic.md, and-chain-must-materialize-write-the-ternary.md

cl5's redundant-branch peephole is **syntactic** (see
`redundant-test-elimination-is-syntactic.md`): it deletes the second test only when it
recognises the two conditions as the same *expression*, not the same *value*. Caching the
pointer in a local makes the two conditions identical and the second test dies. Spelling
the guard on the array element and the body on the cached local keeps both — cl still
CSEs the two loads into one register, so the ASM shows one load and two tests.

```cpp
// ONE test - cl folds the ternary's condition into the outer guard
CSBI_SideTab* p = m_hitRects[i];
if (p && p->m_enabled) {
    i32 hit = p->m_enabled ? PtInRect(&p->m_rect14, x, y) : 0;
    if (hit) { return i; }
}

// TWO, which is retail
if (m_hitRects[i] && m_hitRects[i]->m_enabled) {
    CSBI_SideTab* p = m_hitRects[i];
    i32 hit = p->m_enabled ? PtInRect(&p->m_rect14, x, y) : 0;
    if (hit) { return i; }
}
```

```asm
; TARGET, and the second spelling            ; BASE, first spelling
  mov  ebx,[ecx+0x4]                           mov  ebx,[ecx+0x4]
  test ebx,ebx                                 test ebx,ebx
  je   CONTINUE                                je   CONTINUE
  test ebx,ebx                                 cmp  edi,[ecx+0x1c]
  je   FALSE                                   …
  cmp  edi,[ecx+0x1c]
  …
```

Measured 2026-08-08: `CStatusBarMgr::HitTest` @0x105280 **88.50 -> 100.00 EXACT**.

Two spellings that do NOT reach two tests, and are worth not re-trying:
- assigning the pointer to a second local of the base type (`CStatusBarItem* q = p;`) and
  reading `q->m_enabled` — cl copy-propagates, still one test (88.50);
- moving the whole guard into an inline member of `CStatusBarItem`
  (`return m_enabled ? PtInRect(&m_rect14, x, y) : 0;`) — inlines to
  byte-identical code, still one test (88.50). Writing that member with `&&` instead of
  `?:` additionally loses the materialisation (80.63).
