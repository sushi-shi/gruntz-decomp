# One flag local carries BOTH tests: `f = a; if (f) f = b(); if (f) ...`

tags: cpp:local cpp:branch cpp:loop | asm:test asm:xor asm:mov | topic:codegen-idiom topic:regalloc
symptoms: a list-walk hit test whose block topology, instruction selection and SIZE all match retail exactly, but the two hot values have SWAPPED registers throughout - the loop's POSITION sits in a scratch register and the boolean in a callee-saved one, where retail has it the other way round; the entry `mov <callee-saved>,[this+off]` moves to before/after the `push` that saves it
confidence: 10/10

The predicate is spelled as ONE reused `int`, not as a guard plus a fresh result:

```cpp
// 95.27 - the enabled test lives in the guard, the rect test in a fresh local
if (r && r->m_enabled) {
    i32 hit = PtInRect(&r->m_rect14, x, y);
    if (hit) { return r; }
}

// 100.00 EXACT - one local holds the enabled flag, then is OVERWRITTEN by the
// rect result, and is tested once at the outer level
if (r) {
    i32 hit = r->m_enabled;
    if (hit) {
        hit = PtInRect(&r->m_rect14, x, y);
    }
    if (hit) { return r; }
}
```

Both lower to the identical stream (`mov edx,[eax+4]; test edx,edx; je L; <4 cmps>;
mov edx,1; jmp T; L: xor edx,edx; T: test edx,edx; jne found`). What changes is
cl5's colouring: in the guard form the `m_enabled` temp and the `hit` local are two
webs that get coalesced late and win the callee-saved register over the loop
POSITION; written as one variable spanning both tests the flag is a single short
web that takes a scratch register and the POSITION gets `esi`, which is retail.

**Nesting matters.** `if (hit) { hit = PointInRect(...); if (hit) return r; }` -
the second test INSIDE the first - stays at 95.27. The two `if (hit)` must be
siblings. Declaring `hit` at loop scope with `= 0` (89.34), hoisting it to function
scope (95.27), `r->m_enabled && PointInRect(...)` (76.61) and dropping the local
entirely (76.61) are all worse.

Neutral for this residue, all measured on the same function: 25 graded file-scope
declaration counts, 80 `tu_state_*` islands, and a 400-cell `identifier_rename` +
`declaration_hoist` forest - every cell exactly 95.271736. The register rotation is
NOT TU state; do not spend islands on it.

`CStatusBarMgr::HitTestRects` 0x000ffcb0, three byte-identical list walks,
95.27 -> **100.00 EXACT**.

related: [guard-reads-the-array-element-not-the-cached-local.md](guard-reads-the-array-element-not-the-cached-local.md)
(the sibling `CStatusBarMgr::HitTest` 0x105280, opposite direction - there the guard
must re-read the array element), [redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
