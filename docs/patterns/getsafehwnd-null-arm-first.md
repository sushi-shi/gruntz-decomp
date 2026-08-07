# `test/je` past the load with NO `xor` on the null arm: that is MFC's `GetSafeHwnd()`
tags: cpp:ternary cpp:branch mfc:cwnd cpp:inline | asm:test asm:je asm:xor | topic:codegen-idiom
symptoms: test eax,eax / je <join> / mov eax,[eax+0x1c], missing xor eax,eax, jmp
confidence: 9/10

A guarded window-handle fetch where retail spends **two** instructions and the
recompile spends four. `p ? p->m_hWnd : 0` puts the non-null arm first, so cl must
materialise the zero (`jmp J / xor eax,eax`); MFC's inline is written the other way
round - `this == NULL ? NULL : m_hWnd` - and the null case simply falls through with
`eax` already zero from the `test`.

```cpp
return Init(m_videoWnd->GetSafeHwnd(), mode, coopFlags);   // not `m_videoWnd ? m_videoWnd->m_hWnd : 0`
```
```asm
mov    eax,DWORD PTR [esi+0x53c]
test   eax,eax
je     0x17c3b3                  ; null arm: eax is already 0, no xor, no jmp
mov    eax,DWORD PTR [eax+0x1c]  ; m_hWnd
0x17c3b3:
```

Steerable, and it is a correctness fix as well - calling the accessor on a null
`CWnd*` is what MFC itself does. CMoviePlayer::CreateVideoWindow 0x17c2a0 98.01 ->
100.00 EXACT (its whole 2-instruction deficit). Generalises to any hand-rolled
null-guarded member fetch: spell the NULL arm first.
