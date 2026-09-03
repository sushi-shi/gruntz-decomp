# Retry loop with early bail: preserve the dead result with `goto` or an inline phase return
tags: cpp:loop cpp:branch | asm:cmp asm:jcc asm:jmp | topic:codegen-idiom
symptoms: lock/restore retry loop, first iteration peeled (body duplicated ~74%), or an extra `mov <callee-reg>,eax` keeping the loop var live (~95%)
confidence: 8/10
variants: linked-list-advance-before-process.md, loop-preheader-vs-exit-block-order.md

A "do X; while it reports `RETRY`, fix-up and redo X" loop whose fix-up can
*bail past* the success block (DirectDraw Lock → on `DDERR_SURFACELOST` Restore +
re-Lock, but a failed Restore abandons the frame). Three direct spellings produce
three shapes:

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

## Exact no-`goto` form: return from an expanded phase helper

The `goto` is not the only authentic source boundary. Put the complete
lock/retry/decode phase in a TU-local `inline void` helper and let failed
`Restore` return from that helper:

```cpp
inline void DecodeFrame(Player* p) {
    i32 hr = p->surface->Lock(...);
    while (hr == RETRY) {
        if (p->surface->Restore() != 0) {
            return;
        }
        hr = p->surface->Lock(...);
    }
    if (hr == 0) {
        /* decode and unlock */
    }
}

void Player::Frame() {
    DecodeFrame(this);
    /* blit continuation */
}
```

Under the retail `/Ob1` environment the helper disappears completely. Its early
return becomes the same jump to the caller continuation, and `hr` is still dead
on that edge. `CMoviePlayer::Frame` remains byte-exact at 0x13b bytes/116
instructions/12 calls/11 branches/2 returns/7 relocations; the whole `ddpagemgr`
symbol comparison is unchanged and no helper symbol is emitted.

This is a controlled exception to the direct-spelling rule, not permission to
extract arbitrary blocks. Require all of the following:

1. the extracted region is one coherent source operation;
2. every helper return means "skip the rest of this operation" and reaches the
   statement immediately after the call;
3. `/Ob1` emits no helper call or out-of-line symbol;
4. the complete caller and containing TU compare exactly after extraction.

The same signature occurs in menu input dispatch: expanded helper returns replace
six shared-tail gotos in `CMenuState::Render` (0x1d0, exact) and an inline predicate
replaces the splash input goto in `CSplashState::Render` (0x108, exact). See
[inline-phase-return-replaces-shared-goto.md](inline-phase-return-replaces-shared-goto.md).
