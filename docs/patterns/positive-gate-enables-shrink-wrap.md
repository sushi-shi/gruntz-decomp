# Positive-form gate (`if (ready) { body }`) is what makes cl shrink-wrap a callee-save push
tags: cpp:branch cpp:return cpp:local | asm:push asm:pop asm:jmp | topic:codegen-idiom
symptoms: retail's prologue saves FEWER callee-saved registers than the recompile and emits the
missing `push esi`/`push edi` after a ready/null gate; retail's early exit jumps to a label BELOW
the corresponding `pop`; recompile pushes all of them up front and the early `return 0` pops all of
them; ~90-93% with the body otherwise byte-exact
confidence: 9/10

[`shrink-wrapped-callee-save-push.md`](shrink-wrapped-callee-save-push.md) files this as a wall.
For the *guard* case it is not one — it is decided by whether the gate is written as an **early
return** or as a **positive-form `if` wrapping the whole body**.

The proof is in retail's own exit structure. `CWarlord::BuildFortSplashParticles` @0x44f80:

```asm
    mov   eax,ds:[g_engineFrameDelta]
    push  ebx
    push  edi                 ; only ebx + edi at entry
    ...
    cmp   dword ptr [eax+0x28],ebx
    je    0x450a2             ; <- the early exit target
    cmp   dword ptr [eax+0x20],ebx
    jne   0x450a2
    mov   eax,dword ptr [edi+0x10]
    push  esi                 ; esi saved ONLY on the path that uses it
    ...
    pop   esi
    or    dword ptr [edi+0x8],0x10000
0x450a2:
    pop   edi                 ; the gate lands HERE - below the pop esi
    xor   eax,eax
    pop   ebx
    ret
```

The gate branches *past* `pop esi`, so `push esi` is provably conditional.

```cpp
// EARLY RETURN - cl saves esi in the prologue, gate pops it: 93.1%
if (sub->m_28 == 0 || sub->m_20 != 0) {
    return 0;
}
/* body */
return 0;

// POSITIVE FORM - cl shrink-wraps push esi past the gate: 98.1%
if (sub->m_28 != 0 && sub->m_20 == 0) {
    /* body */
}
return 0;
```

Same family as [`do-while is an echo`](mfc-map-walk-while-not-guard-dowhile.md): an early return is
an *echo* of a nested block, and the echo is what perturbs the frame. Prefer the positive form
whenever the retail early exit lands below a `pop`.

Two measured cases in one TU, both previously marked `@early-stop` with a "shrink-wrap wall":
- `CWarlord::BuildFortSplashParticles` @0x44f80 — 93.11% → **98.07%** (with the y-before-x
  declaration order that fixes the paired member-load schedule)
- `CWarlord::AdvanceMovingAnim` @0x44e70 — 90.33% → **95.85%**

Residual in both is the unrelated `add eax,0x290` disp8 rebase (see the `@early-stop` note there).

The INVERSE direction in `shrink-wrapped-callee-save-push.md` (retail eager-pushes, recompile
shrink-wraps, `CWormhole::SpawnPartners`) is still a wall — this lever only runs one way.

## Second, independent effect: BLOCK LAYOUT + epilogue tail-merge

The same lever also decides *where the miss handler physically lands*, and that is worth more than
the push. `CInGameText::Update` @0x997c0 has four `return 0` exits. Retail lays them out as

```asm
    ...body...
    <success tail>
    xor eax,eax ; pop edi/esi/ebp/ebx ; add esp,0xc ; ret     ; epilogue A
0x9998f:
    mov  DWORD PTR [ebp+0x58],0xffffffff
    mov  eax,DWORD PTR [ebp+0x38]
    and  DWORD PTR [eax+0x40],0xfffffffe                      ; <- memory RMW
0x9999d:
    pop edi ; pop esi ; pop ebp ; xor eax,eax ; pop ebx ; add esp,0xc ; ret   ; epilogue B
```

The miss handler sits **past** the success return and *falls into* epilogue B, which the three
inner early exits (`jne 0x9999d` / `je 0x9999d`) tail-merge into. Written as an early return
(`if (found == 0) { …; return 0; }`) cl emits that handler INLINE right after the test with its
own 12-instruction epilogue — one basic block too many, and the diff then cascades through every
register assignment downstream. Written as the positive gate with the miss tail after the closing
brace, the layout and the tail-merge both reproduce, and the `&= ~1` even switches from
load/and/store to retail's `and DWORD PTR [mem],imm8` RMW on its own. **79.46% → 93.83%** in one
edit. (Two further shape fixes in the same function — the screen coords loading y before x, and
taking `g_gameReg` into a local *after* both coord loads so cl stops hoisting the singleton —
carried it to 96.59%.)

So: whenever the recompile has an early-exit block cl placed inline and the target has the same
handler parked at the END of the function, that is this pattern, not regalloc.

related: shrink-wrapped-callee-save-push.md, mfc-map-walk-while-not-guard-dowhile.md,
retry-loop-bail-while-goto-no-peel.md
