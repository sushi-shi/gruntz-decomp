# A `push` of a callee-saved register BELOW the entry guard means the source had exactly ONE tail return
tags: cpp:branch cpp:local | asm:push asm:pop | topic:codegen-idiom
symptoms: push esi push edi push ebx after the guard, shrink wrap, prologue split, early-out pops fewer registers, entry pushes, +2 instruction count, delayed register save
confidence: 10/10

cl 5.0 shrink-wraps: registers a function's *entry guard* does not use get their
`push` emitted at the register's first definition, deep in the body, and the
guard's `return` then pops only what the entry pushed. It only does this when the
guard's `return` is the **sole early return** — any other `return` in the body
pins every save back into the prologue, so our base pushes all of them at entry
and every early-out pops them all (base is +2/+4 instructions and every register
rotates). The decision is made on the source-level flow graph, so it survives cl's
later tail duplication: retail functions with six `ret`s are still shrink-wrapped
because their source had one tail `return` that cl copied into each block.

```cpp
// NO - the second `return` pins all four pushes into the prologue
if (!p) { Report(); return 0; }
int x = q(b);
if (x == 0) { return 0; }        //  <-- this line is the whole bug
y = q(c);
return 0;

// YES - guard + ONE tail return; cl sinks push edi/ebx past the guard
if (!p) { Report(); return 0; }
int x = q(b);
if (x != 0) { y = q(c); }
return 0;
```
```asm
push   ebp                  ; only what the guard needs
push   esi
mov    esi,ecx
mov    ebp,DWORD PTR [esi+0xb0]
test   ebp,ebp
jne    L1
xor    eax,eax
pop    esi                  ; <-- the early-out pops TWO, not four
pop    ebp
ret
L1:
push   edi                  ; <-- sunk saves, in FIRST-USE order (not canonical)
push   ebx
```
Steerable. Nested `if`/`else` with a single tail return; a result variable also
works (`int r = 0; … r = call(); return r;`) but only if it is declared where the
guard does not need a zero register. Sieve: leading-push count base vs target
(33 candidates found tree-wide). `DSoundCloneInst::GetItem` 0x135d70 90.31 ->
100.00 EXACT, `CGrunt::UpdateGruntStatus` 0x617c0 94.63 -> 100.00 EXACT,
`CDDrawWorkerHost::DeactivateDistantObjects` 0x163370 87.88 -> 100.00 EXACT,
`CPlay::StepGridWalk` 0xd0a60 66.67 -> 100.00 EXACT (that one was filed as a
WALL - see the related link).

related: [tail-block-placement-cross-jump-wall.md](tail-block-placement-cross-jump-wall.md) (the wall this breaks), [trailing-error-block-is-a-crossjump-magnet.md](trailing-error-block-is-a-crossjump-magnet.md) (the half that is still a wall: merging guards with `||` feeds cl's cross-jump).
