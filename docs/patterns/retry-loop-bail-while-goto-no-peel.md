# Retry loop with early bail: top-tested `while` + `goto` (not `if{do-while}`, not `break`)
tags: cpp:loop cpp:branch | asm:cmp asm:jcc asm:jmp | topic:codegen-idiom
symptoms: lock/restore retry loop, first iteration peeled (body duplicated ~74%), or an extra `mov <callee-reg>,eax` keeping the loop var live (~95%)
confidence: 8/10
variants: linked-list-advance-before-process.md, loop-preheader-vs-exit-block-order.md

A "do X; while it reports `RETRY`, fix-up and redo X" loop whose fix-up can
*bail past* the success block (DirectDraw Lock → on `DDERR_SURFACELOST` Restore +
re-Lock, but a failed Restore abandons the frame). Three spellings, three shapes:

- `if (hr==RETRY) { do { if (bail) goto L; hr = X(); } while (hr==RETRY); }` —
  MSVC **peels** the first iteration (Restore+X emitted twice), ~74%.
- `while (hr==RETRY) { if (bail) break; hr = X(); }` — loop shape is right, but
  `break` leaves `hr` live after the loop so it is pinned in a callee-saved reg
  (`mov edi,eax` after every X), ~95%.
- `while (hr==RETRY) { if (bail) goto L; hr = X(); } ... L:` — **100%**. The
  top-tested `while` gives the right rotation (loop top = the fix-up), and the
  `goto` past the success block leaves `hr` dead on the bail path, so retail can
  keep it in `eax` straight off X with no spill.

```cpp
hr = pSurf->vptr->Lock(pSurf, 0, &desc, 1, 0);
while (hr == (i32)0x887601c2) {       // DDERR_SURFACELOST
    if (pSurf->vptr->Restore(pSurf) != 0) {
        goto afterLock;               // bail PAST the decode/unlock block
    }
    hr = pSurf->vptr->Lock(pSurf, 0, &desc, 1, 0);
}
if (hr == 0) { /* decode frame, Unlock */ }
afterLock:;
```
```asm
        call    [ecx+0x64]            ; Lock
        cmp     eax, 0x887601c2
        jne     L_after_loop
Ltop:   call    [edx+0x6c]            ; Restore  (loop top, single copy)
        test    eax, eax
        jne     L_bail
        call    [ecx+0x64]            ; re-Lock
        cmp     eax, 0x887601c2
        je      Ltop
L_after_loop:
        test    eax, eax              ; hr still in eax, no spill
```
Steerable. MoviePlayer Smacker frame render @0x17caa0: `if{do-while}` 74.6% →
`break` 95.2% → top-`while`+`goto` 100%.
