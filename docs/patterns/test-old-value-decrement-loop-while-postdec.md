# Counted loop guarded by `mov eax,n; dec n; test eax; je; inc n` — spell it `while (n-- != 0)`
tags: cpp:loop cpp:local | asm:dec asm:test asm:inc | topic:codegen-idiom
symptoms: a count-down loop's entry guard is `mov eax,edi; dec edi; test eax,eax; je end; inc edi` (load the count, pre-decrement, test the OLD value, re-increment on entry) — your `for(n=count;n;n--)` or `if(n){do{}while(--n)}` emits a plain `test edi,edi; je` and diverges right at the guard
confidence: 8/10

MSVC5 has a specific lowering for a count-down loop whose counter it wants to
**post-decrement against zero**: at the loop preheader it loads the count, decrements
it, tests the *pre-decrement* value for the zero-trip guard, then re-increments to
restore the count for the body. The asm tell is the `dec/test-old/inc` triple:

```
mov  eax, edi        ; eax = n
dec  edi             ; n-1 (speculative)
test eax, eax        ; guard on the ORIGINAL n
je   end
inc  edi             ; restore n for the loop body
loop: ... ; dec edi; jne loop
```

The source spelling that reproduces it is the **post-decrement while**:

```cpp
i32 n = count;
while (n-- != 0) {        // <- NOT for(;n;n--) and NOT do/while(--n)
    ...
}
```

`for (n=count; n!=0; n--)` and `if(n){do{...}while(--n);}` both emit the simpler
`test n,n; je` guard (no speculative dec/inc) and miss this peel. The
`while (n-- != 0)` form makes cl evaluate `n` then decrement, which is exactly the
`test-old-value-then-decrement` shape.

STEERABLE. Evidence: `zDArray::IndexToPtr` (0x310f0), the per-element member-pointer
fixup loop over `m_grown` freshly-grown slots — `for(n=m_grown;n;n--)` plateaued at
94.0% on the guard; `while (n-- != 0)` closed it to 99.55% (reloc-masked exact).
Related but distinct from [[predecrement-guard-lea-recover-count]] (that one recovers
the counter with `lea edi,[eax+1]` after the saves; this one re-increments in place).
