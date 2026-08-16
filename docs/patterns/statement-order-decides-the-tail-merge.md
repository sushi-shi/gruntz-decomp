# Two arms tail-merge that retail kept apart: the STATEMENT ORDER inside them is the knob

tags: cpp:branch cpp:return cpp:local | asm:dec asm:mov asm:jmp | topic:codegen-idiom topic:tail-merge
symptoms: `gruntz walls diagnose --asm` shows one base block ending in `ret` where retail has
  `N i [jmp B<k>]` (or the reverse), the two sides' instruction streams are otherwise
  identical, and the merged side turns a read-modify-write (`dec DWORD PTR [m]`,
  `or DWORD PTR [m],imm`) into the shared block while retail's copy has the load split
  out (`mov ecx,[m] ... dec ecx ... mov [m],ecx`)
confidence: 9/10
variants: identical-return-epilogue-tailmerge.md, dup-exit-means-a-shared-goto-label.md

cl5 tail-merges two blocks only when their instruction TAILS are literally identical.
When one arm does an unrelated store and then a counter update, the ORDER of those two
statements decides whether the counter update stays a memory RMW (mergeable with a
sibling arm that does only the RMW) or gets scheduled as load / other-store / dec /
store (not mergeable).

```cpp
// NO - `dec DWORD PTR [esi+0x28]` is the whole tail, so it merges with the
//      m_step == 0 arm's bare decrement; three arms collapse to one block
m_frameIndex = m_frameEnd;
m_redrawFrames--;
return 1;

// YES - the decrement's load is now free to hoist ABOVE the clamp store, the
//      block is no longer tail-identical, and it keeps its own epilogue
m_redrawFrames--;
m_frameIndex = m_frameEnd;
return 1;
```
```asm
; retail, both clamp arms:
mov ecx,DWORD PTR [esi+0x28]     ; the counter load, hoisted above the store
mov DWORD PTR [esi+0x38],eax     ; m_frameIndex = m_frameEnd
dec ecx
mov eax,0x1
mov DWORD PTR [esi+0x28],ecx
pop edi
pop esi
ret
```

STEERABLE. `CSBI_ImageSetAni::Render` @0xe7b00 **91.49 -> 99.44** on the swap alone
(13 instructions); adding an explicit `return 1;` after the store changed nothing by
itself - only the order did. Screen with `gruntz walls diagnose --asm`. THIS knob is for
the direction where WE merged and retail did not: base `1i [fall Bk]` (or a short
block flowing into a shared tail) against retail `Ni [ret]` with a full inline
epilogue. The OPPOSITE reading - base `Ni [ret]` against retail `Mi [jmp Bk]`, M < N,
i.e. retail merged and we duplicated - is a different problem, and there the fix is
usually the `||` / `goto fail;` family instead.

A sibling knob for the SAME symptom on a guarded store: an explicit `return` right
after the store forces cl to give that arm its own epilogue instead of falling into
the shared one - `CActionOptionsMenuBar::HitClick` @0x9650 88.89 -> 92.59 from
rewriting `if (s == NORMAL) { s = SELECTED; }` as `if (s != NORMAL) return 1; s =
SELECTED;`, and 92.59 -> **100.00 EXACT** once the store also got its own `return 1;`.
Once the compare constant and the return value are the same literal, cl materialises
it (`mov eax,1; cmp edx,eax`) and the store arm returns without reloading it.

related: positive-gate-enables-shrink-wrap.md, while-not-do-while-keeps-the-inline-return.md
