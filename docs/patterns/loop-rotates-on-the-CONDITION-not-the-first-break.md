# One extra branch in a loop: cl rotates the loop CONDITION, so pick which test is the condition

tags: cpp:loop cpp:branch | asm:jmp asm:cmp | topic:codegen-idiom
symptoms: `sema disasm --branches --diff` reports **base has exactly one more conditional branch
than target**; the extra one is a DUPLICATE of a test that also appears at the top of the loop, and
retail's back-edge jumps to that top test while ours jumps past it into the body
confidence: 9/10

## The claim

A loop with two exits - the `while`/`for` condition and an in-body `break` - has two source
spellings that are semantically identical and are NOT the same code:

```cpp
while (A) { if (B) break; body; }      // cl rotates A
while (B') { body; if (!A) break; }    // cl rotates B'
```

cl5 rotates **the loop condition** and leaves the in-body `break` alone. Rotating means: peel the
condition into an entry guard, put the loop's *first block* at the top, and make the bottom test
the back-edge. So the test you choose as the CONDITION is the one that gets duplicated (entry guard
+ back-edge), and the test you leave as a `break` stays single, at the loop head, and is what the
back-edge lands on.

That is a **one-branch difference**, and it is the entire signal `--branches --diff` reports.

## Measured

`src/Font/Font.cpp`, the three word-wrap bodies' inner per-char split loop
(`FontRenderer::MeasureWrapped` 0x17ad10, `DrawWrapped` 0x17a460, `LayoutWrapped` 0x17b120). Retail
`LayoutWrapped`:

```asm
  17b351: mov  eax,[esp+0x1c]      ; \  peeled entry guard on the LENGTH
  17b355: mov  ecx,[eax-0x8]       ;  |  head.GetLength()
  17b358: test ecx,ecx             ;  |
  17b35a: jle  0x17b416            ; /
  17b360: mov  ecx,[esp+0x6c]      ; \  LOOP HEAD - the y-test, appears ONCE
  17b364: mov  eax,[esp+0x18]      ;  |
  17b368: cmp  eax,ecx             ;  |
  17b36a: jge  0x17b416            ; /
          ...body...
  17b40b: mov  ecx,[eax-0x8]       ; \  back-edge test on the LENGTH
  17b40e: test ecx,ecx             ;  |
  17b410: jg   0x17b360            ; /  -> jumps to the LOOP HEAD, not the body
```

so the condition is the length and `y < bottom` is a break:

```cpp
while (head.GetLength() > 0) {          // 89.02 / 90.39 / 76.33, branches AGREE
    if (y >= bottom) break;
    ...
}
```

Spelled the other way round it scores 86.66 / 88.75 / 75.21 with one extra branch: cl peels the
y-test as the entry guard, then emits a SECOND copy of it after the length test, and the back-edge
targets the body.

```cpp
while (y < bottom) {                    // one extra branch
    ...
    if (head.GetLength() <= 0) break;
}
```

`MeasureWrapped` indexes with `j`, so its condition is `j < head.GetLength()`; with `j == 0` cl
constant-folds the peeled guard to the same `test ecx,ecx / jle`, which is why the entry guard looks
like a plain length test in all three.

## The trap: hand-rotating does NOT work

The obvious "just write what the compiler emits" move is refuted:

```cpp
if (head.GetLength() > 0) {             // still one extra branch
    do {
        if (y >= bottom) break;
        ...
    } while (head.GetLength() > 0);
}
```

Given an explicit `do..while`, cl treats the leading `if (...) break` as the rotatable test and
duplicates the y-test at the bottom anyway - the same defect, unchanged. Only *promoting the right
test to the loop condition* moves it.

This also explains the earlier measured negative that `while`, `for(;;)`+top-`break` and `do/while`
compile byte-identically: those all keep the SAME test as the condition, and the keyword is indeed
not the lever. Which test is the condition is.

## The same defect with the peeled guard ALREADY transcribed into the source

`CTriggerMgr::SpawnGrunt` 0x7c110 shows the sibling form: the reconstruction had written cl's
own rotation *back into the C++*, entry guard and all, which is why it read as a plain
"guard + counted loop" and not as a rotation question at all.

```cpp
// NO - the `if (m_grid[...] != NULL)` IS cl's peeled guard, and `free < 15` is now the
//      condition, so cl rotates THAT: base `test; je / mov; add; inc; test; je; cmp; jl`
i32 free = 0;
if (m_grid[row * TM_GRID_COLS] != NULL) {
    CGrunt** p = &m_grid[row * TM_GRID_COLS];
    while (free < 15) { p++; free++; if (*p == NULL) break; }
}

// YES - the pointer test is the condition (cl re-peels it itself), the bound is the break
i32 free = 0;
CGrunt** p = &m_grid[row * TM_GRID_COLS];
while (*p != NULL) { if (free >= 15) break; p++; free++; }
```

Retail: `test edi,edi / je out` (cl's own peel), then `LOOP: cmp ebp,0xf / jge out /
mov edi,[ecx+4] / add ecx,4 / inc ebp / test edi,edi / jne LOOP`. 74.49 -> 80.31 and both
`OTHER` rows (`je->jge`, `jl->jne`) closed on that edit alone. **Tell:** the sieve bucket is
`OTHER`, not `POLARITY` - the two branches swap *kind* (`je`/`jl` vs `jge`/`jne`) because the
entry guard and the back-edge trade tests. A hand-written entry guard that duplicates a test
the loop already makes is the fingerprint; delete it and promote its test to the condition.

## How to read it off the target

`--diff` and `--blocks --diff` mask address operands and will report the two functions identical.
Use `--branches --diff`: it reports the count mismatch. Then `--blocks --lite --target` and find the
back-edge (`LOOP`) block - **whatever block the back-edge jumps to is the loop head, and whatever
test that block contains is NOT the condition**. The condition is the test in the back-edge block
itself.
