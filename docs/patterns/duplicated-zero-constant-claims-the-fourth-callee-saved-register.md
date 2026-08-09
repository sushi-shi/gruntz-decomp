# A duplicated ZERO constant claims the 4th callee-saved register — and the spellings that fix it cost more

tags: cpp:local cpp:struct cpp:branch | asm:xor asm:push asm:cmp | topic:wall topic:regalloc
symptoms: a small (~60-instruction) function is +6 instructions and every
`[esp+N]` offset is shifted by 4; retail has ONE `xor <reg>,<reg>` before the
entry test and tests the parameter with `cmp <param>,<zeroreg>`, we have TWO
`xor`s and `test <param>,<param>`; we carry a `push ebx`/`pop ebx` retail does
not, and the parameter lives in ebx where retail keeps it in eax
confidence: 9/10 (11 source spellings + a 400-iteration hill-climb + a 25-point
declaration-count sweep, all measured with objdiff)

Two live copies of the *same* constant is one value too many for cl 5.0's four
allocatable registers, so it claims a callee-saved one for the parameter. The
symptom therefore reads as a whole-frame divergence for what is really one
redundant `xor`.

```cpp
void CMapMgr::Clip(const RECT* src) {   // 0x2b340
    RECT a, b;
    b.left = b.top = 0;                 // retail: ONE zero reg for both,
    b.right = m_width;                  // reused for the `cmp src,zero`,
    b.bottom = m_height;                // second zero created INSIDE the else
    if (src) { ... } else { a = b; }
    ...
}
```

**The trap is that reaching retail's prologue does not reach retail's function.**
Writing the else arm out field-by-field instead of `a = b;` *does* collapse the
two zeros to one, *does* delete the `push ebx` and *does* turn `test` into
`cmp <param>,<zeroreg>` — and scores **worse**, because the four explicit stores
cost more than the frame saves. Do not chase the prologue tells on their own.

## Measured, all with objdiff (baseline 87.68 is the best cell)

| spelling | result |
|---|---|
| `b.left = 0; b.top = 0;` split, or reversed, or via a named zero local | byte-identical to the chain |
| `if (src != NULL)` | byte-identical |
| `RECT b = { 0, 0, m_width, m_height };` | 87.58 |
| `SetRect(&b, 0, 0, m_width, m_height)` | +8 instructions, worse |
| else arm written field-by-field | **prologue tells all correct**, 85.82 |
| b initialised after the `if` | 63.60 |
| `w`/`h` locals, no `dst` local, `dst` hoisted, `src` copied to a local, inverted null test, `b.right/b.bottom` first | all ≤ baseline |
| `gruntz.permute.permute` 400 iterations | `FINAL 87.683 (no change)` |
| 0–24 throwaway declarations above the first include | **completely flat** — 66 insns, `push ebx`, 2 early `xor`s at every N |

That last row matters twice: it says the
[declaration-count window](declaration-count-window-steers-regalloc.md) does
**not** reach this wall, so the bank-the-MAX recipe is unavailable here and the
function cannot be parked as "proven correct" — only as a wall.

Same family, different trigger:
[known-zero-reload-before-call](known-zero-reload-before-call.md) (a
copy-propagated zero is the extra use that claims the 4th register) and the
inverse [rect-fill-through-a-base-register-is-a-byvalue-rvalue](rect-fill-through-a-base-register-is-a-byvalue-rvalue.md)
(retail materialises a zero PER FIELD where we reuse one).
