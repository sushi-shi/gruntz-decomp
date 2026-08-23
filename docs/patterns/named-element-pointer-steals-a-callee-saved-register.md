# A named pointer to an array ELEMENT is a call-crossing value; retail indexes the member array at each use
tags: cpp:local cpp:member cpp:loop | asm:push asm:pop asm:xor | topic:codegen-idiom
symptoms: shrink wrap, split epilogue, prologue push order, extra xor reg reg, loop counter in a different register, one side materializes two zeros, `mov ebx,[esi+0x50]` plus a stack reload where retail has `add ebx,eax`, walls diagnose says REGALLOC/SCHEDULING with equal instruction counts
confidence: 9/10
variants: shrink-wrapped-prologue-needs-one-tail-return.md

Writing `T* c = &m_array[i];` at the head of a loop body makes `c` a **named
value that is live across the loop's calls**, so it binds one of `ESI, EDI, EBX,
EBP`. That consumes a callee-saved register the rest of the function then cannot
have, and every downstream decision rotates: the constant `0` that fed the
function's entry arguments gets pushed one slot further along `{EAX, ECX, EDX,
ESI, EDI, EBX, EBP}`, it ends up sharing a register with the loop counter, and
because that register is now needed by the ENTRY guard its `push` is pinned into
the prologue - which suppresses the shrink wrap retail has. Retail's spelling has
no such local: it indexes `m_array[i]` at each use and lets cl's own
strength-reduced offset temp be the value that lives in the register.

```cpp
// NO - `c` is a fourth call-crossing value; it takes EBX, pushes the zero to
// EBP, and EBP's `push` moves up into the prologue
for (i32 i = 0; i < n; i++) {
    CFaderRadialCell* c = &m_cells[i];
    float d = c->m_radius - base;
    ...
    dst[...] = c->m_pixel;
}

// YES - the element address is a compiler temp, so the zero keeps EBX and the
// counter keeps EBP, and `push ebp` sinks below the entry guard
for (i32 i = 0; i < n; i++) {
    float d = m_cells[i].m_radius - base;
    ...
    dst[...] = m_cells[i].m_pixel;
}
```

The strength reduction is identical either way - both sides keep a byte offset in
a stack slot and advance it by `sizeof(T)` - so the *code* the loop body runs is
unchanged. Only the register binding moves.

## Detection signature

Two tells, both visible without any masking:

* **Two zeros against one.** Retail materializes `xor ebx,ebx` for the argument
  zeros and a second `xor ebp,ebp` for the loop counter after the guard; ours
  CSEs them into a single `xor ebp,ebp` before the guard and pays for it with an
  extra reload of the offset accumulator inside the loop. Equal total instruction
  counts, so no size sieve sees it.
* **The prologue/epilogue split.** Retail: `push ebx / push esi` at entry, `push
  ebp` immediately after the guard's `je`, `push edi` after the loop-count guard,
  and the guard's exit branches into the final pop run *below* `pop ebp`. Ours
  pushes `ebp / esi` at entry and both sunk saves land together inside the loop
  preheader, so the guard's exit lands on the *head* of the pop run.

The second tell is exactly the split-epilogue census in
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md),
run in its forward direction: retail splits and we do not.

## Evidence

`CFaderRadial::RenderFrame` 0x0017fc60 **88.58 -> 99.72**, one edit, no other
change: `CFaderRadialCell* c = &m_cells[i]` deleted and its three uses respelled
`m_cells[i]`. `walls diagnose` had classified it REGALLOC/SCHEDULING with 108
instructions, 9 calls, 9 branches, 1 ret and 9 relocations on BOTH sides - the
class was right and the cause was still a source spelling. Screened inert on the
same body afterwards: `Lock(0)` vs `Lock(NULL)`, and an up-front `u8* base;`
declaration split from its initializer.

The residue is six instructions in the single tail block, where retail retires
`this` early and reuses `esi` as the scratch for `m_dstSurface` (`mov
esi,[esi+0x3c]`) while ours takes `edx`; the two vtable loads and the `scratch`
reload rotate with it. That is one cursor position, in the register-colour class.
