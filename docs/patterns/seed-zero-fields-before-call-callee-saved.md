# Zero-register pinning IS steerable when a call splits the seeds: write the pre-call seeds first
tags: cpp:local cpp:branch | asm:mov asm:xor asm:call | topic:codegen-idiom topic:regalloc
symptoms: frameless/framed field-seed method (Clear/Reset/ctor) with a member op=/ctor CALL amid the `=0` stores stuck ~50%, base `xor eax,eax`+reuse eax (caller-saved) vs retail `push edi;xor edi,edi`+reuse edi (callee-saved), extra/'missing' push, whole stack frame shifted 4
confidence: 8/10

A field-seeding method (re-empty a CString member via `op=`, then seed a scalar block)
plateaus low when a **member call** sits between the seeds. If your source writes all the
`= 0` seeds AFTER the call, cl materializes the zero in a **caller-saved** reg (eax) — a
call would clobber it, but there's no call after, so eax is fine — and picks a different
frame. Retail keeps the zero live ACROSS the call (it seeds some fields *before* the call
too), so its zero MUST live in a **callee-saved** reg (edi/esi, pushed/popped). Reproduce
it by moving the fields retail seeds pre-call ahead of the call in source order: the zero
now spans the call → cl is forced to pin it callee-saved, matching retail.

```cpp
// NO (~52%): all zero-seeds after the m_name op= call → cl pins zero in caller-saved eax
m_name = g_emptyString;         // CString::operator= (a CALL)
m_018 = -2; m_020 = 0; m_014 = 1; /* ...more =0... */

// YES (~95%): the fields retail seeds pre-call go BEFORE the call → zero spans the call
m_018 = -2; m_020 = 0; m_014 = 1;   // one is `= 0`; it must now survive the op= call
m_name = g_emptyString;             // the CALL
/* ...the remaining =0 seeds... */
```
```asm
; retail: zero lives across the call in edi
push edi / xor edi,edi
mov [esi+0x18],... / mov [esi+0x20],edi / mov [esi+0x14],1   ; pre-call seeds
call CString::operator=
mov [esi+8],edi / ...                                         ; post-call seeds, same edi
```
STEERABLE (this closes most of the gap). Contrast the CALL-LESS sibling case
([sentinel-seed-ctor-store-schedule.md](sentinel-seed-ctor-store-schedule.md),
[zero-register-pinning.md](zero-register-pinning.md)): with no call there is no
callee-saved constraint, so the zero-reg choice + the `-1`/`0xf` immediate float are a
free scheduler coin-flip that NO reorder flips. Evidence: GruntzPlayer::Clear @0x0da960
52→94.65% (residual = the sibling's `m_comboSel=0xf` immediate-float wall); the Reset
sibling @0x0da9e0 (no reorderable call) stays 94.9% on that same immediate float.
