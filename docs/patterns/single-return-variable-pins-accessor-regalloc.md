# One result variable + one `return` pins an accessor's regalloc (multi-return swaps this/arg)
tags: cpp:return cpp:local cpp:method | asm:mov asm:pop | topic:codegen-idiom topic:regalloc
symptoms: a small `T* Get(i32 i)`-shaped container accessor sits ~83-84% with EVERY instruction
byte-faithful but esi/edi swapped throughout: the recompile puts `this` in esi and the index in edi,
retail puts the index in esi and `this` in edi; retail's arms also end `... ; mov eax,esi` where the
recompile computes straight into eax
confidence: 9/10

An accessor written with a `return` in every arm hands cl an independent value per arm, so it keeps
`this` in the first callee-saved register and computes each result into `eax`. Written with ONE
result variable assigned in each arm and returned once, cl coalesces the result into the register
that already holds the index - freeing edi for `this` and producing retail's trailing `mov eax,<r>`.

```cpp
// MULTI-RETURN  -> this->esi, id->edi, result in eax        : 83.78%
T* zDArray<T>::Resolve(i32 id) {
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) return base_at(id);
    if (GrowTo(id, 0))            return base_at(id);
    ...; m_errSink->Set(this, item, 0xc);
    return (T*)m_spare;
}

// SINGLE RESULT -> id->esi (reused as the result), this->edi : 100% EXACT
T* zDArray<T>::Resolve(i32 id) {
    T* r;
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) { r = base_at(id); }
    else if (GrowTo(id, 0))       { r = base_at(id); }
    else { ...; m_errSink->Set(this, item, 0xc); r = (T*)m_spare; }
    return r;
}
```

Retail's proof is the in-place mutation plus the copy-out:

```asm
sub    esi,eax                 ; esi holds id and BECOMES the result
mov    eax,DWORD PTR [edi+0x10]
imul   esi,DWORD PTR [edi+0x18]
add    esi,eax
mov    eax,esi                 ; <- only a single result variable produces this
pop    edi
pop    esi
ret    0x4
```

cl still duplicates the two-pop epilogue into all three arms, so the *shape* is unchanged - only the
register roles move. `_zvec::IndexToPtr` (src/Wap32/ZVec.cpp) was already written this way and is
exact; `zDArray<CActHandler>::Resolve` (src/Gruntz/FortressFlag.cpp @0x464e0) was not, and carried an
`@early-stop` reading "esi/edi regalloc wall ... not steerable". It was the multi-return spelling.

related: pin-local-for-callee-saved-reg.md, zero-register-pinning.md,
act-registrar-report-outline-budget.md
