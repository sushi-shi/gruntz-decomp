# Two conditional branches to the SAME target = `if (x)` + `while (...)`, never a `do/while`
tags: cpp:loop cpp:if | asm:jcc asm:test | topic:codegen-idiom
symptoms: `test eax,eax` followed immediately by `je L` and then `jle L` - two jumps, one target; the reconstruction has a `do { ... } while (cond);` and is stuck 20-30 points below a source-identical sibling
confidence: 9/10

`test eax,eax; je L; jle L` is redundant read as one test (`je` already covers
`== 0`, so `jle` can only fire on `< 0`). It is not one test. It is **two source
conditions collapsed onto one exit**:

```cpp
if (clip->top != 0) {          // -> test / je L
    while (row < clip->top) {  // -> the loop's zero-trip guard, jle L
        ...
    }
}
```

A `do/while` has no entry guard, so it emits only the first branch and every
subsequent block is shifted - which is why this shows up as a large score gap
rather than a two-instruction one.

The corollary is the cheap check: **count `jcc`s that share a target**. N
branches to one label means N source tests, and the reconstruction must spell all
N. `--diff` shows this fine (the branches are adjacent); `--blocks --diff` shows
it as an early `!!` kind mismatch.

`CDDrawShadeBlit`'s four blitters all open with this RLE row-skip loop.
`BlitCopyForward` 0x149950 was already spelled `while` and sat at 72.87 while its
three source-identical siblings, spelled `do/while`, sat at 42-53. Converting
them moved `BlitCopyMirrored` 42.42 -> 45.83 with the rest of its prologue
becoming instruction-for-instruction identical modulo register naming.

Same site, second lesson: retail keeps the RLE byte in `cl` and masks with
`and ecx,0xff`, while a named `u8 b = m_rleData[pos];` local made cl5 home it and
reload it. Re-reading `m_rleData[pos]` at each use is what the original wrote.

## Related

- `docs/patterns/do-while-is-an-echo-write-while.md`
- `docs/patterns/while-not-do-while-keeps-the-inline-return.md`
