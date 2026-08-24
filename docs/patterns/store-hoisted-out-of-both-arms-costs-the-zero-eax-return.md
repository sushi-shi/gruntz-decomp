# A store written in BOTH arms and hoisted by cl costs the free `return 0`
tags: cpp:branch cpp:return cpp:member | asm:xor asm:test asm:mov | topic:codegen-idiom
symptoms: base is exactly ONE instruction short; retail has a `xor eax,eax` before an epilogue where you have none, and the guard just above is `test eax,eax` / `<store>` / `jne`; the same function has an EARLIER `test eax,eax` guard where retail also omits the `xor`
confidence: 9/10

After `test eax,eax; jne <cont>`, the fall-through path has `eax == 0`, so cl
normally lets a `return 0;` there reuse it and emits no `xor`. It only keeps that
knowledge while **eax is still live at the branch**. If the instruction between
the test and the jcc is a store *of eax*, eax stays live and the `xor` is elided;
if it is a store of some other register, eax's live range ends at the test, the
register allocator frees it, and the `return 0` must rematerialize the zero.

The asymmetry inside one function is the tell:

```asm
    test eax,eax
    mov  [esi+0x2c],eax      ; stores the tested value -> eax still known 0
    jne  <cont>
    pop  edi                 ; no xor
...
    test eax,eax
    mov  [esi+0x2c],edi      ; stores something ELSE
    jne  <cont>
    xor  eax,eax             ; <- the extra instruction
    pop  edi
```

What puts an unrelated store there is a statement written in **both** arms that
cl hoisted above the branch:

```cpp
// ours - one store, above the test: cl keeps eax live, no xor
i32 faded = LoadTitlePage(...);
m_stateBank = saved;
if (faded == 0) {
    return 0;
}

// retail - the restore is written in both arms; cl hoists it, eax dies
i32 faded = LoadTitlePage(...);
if (faded == 0) {
    m_stateBank = saved;
    return 0;
}
m_stateBank = saved;
```

Both spellings emit ONE store in the same place. Only the liveness differs, and
only the `xor` shows it. `CCreditsState::InitAttractTitle` 0x039570
99.01 -> 100.00 EXACT.

## The duplicated zero store can also trigger a function-wide zero register

The source duplication is visible to the optimizer before the two stores are
factored. When the common statement is `member = 0`, those two source uses can
cross cl 5.0's threshold for pinning zero in a callee-saved register. The final
assembly still contains only one store, but the chosen zero register also feeds
earlier zero member seeds and call arguments.

`CMulti::Connect` 0x0b67f0 was the exact control. With the call result in a local
and one reset after the call, base used immediate-zero stores, saved only ESI,
and omitted the second failure path's `xor eax,eax` (78.51%). Testing the call
directly and writing `m_pumpGuard = 0` in both arms made cl hoist one
`mov [esi+0x57c],edi` between the `test` and `jne`, pin EDI as zero from the
prologue, and reproduce retail byte-for-byte (**100.00%**). Calls 3/3, branches
2/2, returns 3/3, relocations 3/3, and the ordered referent sequence were already
equal before the edit. A named zero/result local was byte-identical to the bad
form, so the lever is the duplicated arm statement, not an artificial carrier.

Inverse reading: if retail has NO `xor` where you emit one, the store retail put
between the test and the branch is a store *of the tested value* - look for a
member assignment whose right-hand side is the call result.
