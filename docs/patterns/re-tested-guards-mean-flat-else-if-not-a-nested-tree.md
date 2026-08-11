# Every arm re-testing BOTH values means flat `else if`s, not a nested decision tree
tags: cpp:branch cpp:if | asm:test asm:jcc | topic:codegen-idiom
symptoms: a 4-way quadrant/sign dispatch where retail spends a `test`+`jcc` pair on EACH
value in EVERY arm (8 tests for 4 arms) and threads a failing arm straight onto the next
arm's second `jcc` — the jump lands ON a `jcc`, not on the `test` that sets its flags —
while the recompile emits the minimal 3-test tree and the arm ORDER differs
confidence: 9/10

A nested tree

```cpp
if (dx > 0) { if (dy > 0) A; else C; } else { if (dy > 0) B; else D; }
```

costs three tests and emits the arms in the order A, C, B, D. Retail's shape

```
test ecx,ecx ; jle L2        <- dx <= 0
test eax,eax ; jle L1        <- dy <= 0
A
L1: test ecx,ecx             (dx, re-tested)
L2: jge L4                   <- entered here from the first jle, flags still from
test eax,eax ; jle L3           the FIRST `test ecx,ecx`
B
L3: test ecx,ecx
L4: jle L6
test eax,eax ; jge L5
C
L5: test ecx,ecx
L6: jge END
test eax,eax ; jge END
D
```

is four INDEPENDENT `&&` conditions written flat, in the order (+,+) (-,+) (+,-) (-,-):

```cpp
if      (dx > 0 && dy > 0) { A }
else if (dx < 0 && dy > 0) { B }
else if (dx > 0 && dy < 0) { C }
else if (dx < 0 && dy < 0) { D }
```

cl emits each arm's redundant `test` and then jump-threads the previous arm's failure
directly onto the following `jcc`, because the flags it needs are already live. **The
tell is the landing address**: a `jcc` whose target is another `jcc` (skipping that
block's own `test`) is a re-tested flat chain; a `jcc` that lands on a `test` is a tree.
Read each arm's polarity pair off its two `jcc`s (`jle`+`jle` = `>0 && >0`, `jge`+`jle`
= `<0 && >0`, ...) and the arm ORDER falls out with it.

Measured on `CGrunt::LoadGruntCombatAnimations` @0x597a0 (the diagonal-blocked probe at
0x5a7b2..0x5a8e4): **70.36% -> 72.75%**, the four arms and all six gate blocks going from
`!!` to `==`. The same commit hoists `i32 w = grid->m_width;` above the bounds check —
retail stores the width to a slot AT the `jae` (`mov edx,[ecx+0xc] / cmp edi,edx /
mov [esp+0x20],edx`) and never re-reads it, while the height it compares in place, which
is a one-line read of which of the two is a source local.
