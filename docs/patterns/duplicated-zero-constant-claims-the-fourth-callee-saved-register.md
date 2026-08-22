# A duplicated ZERO constant claims the 4th callee-saved register — compose the two levers, do not test them apart

tags: cpp:local cpp:struct cpp:branch | asm:xor asm:push asm:cmp | topic:regalloc topic:codegen-idiom
symptoms: a small (~60-instruction) function is +6 instructions and every
`[esp+N]` offset is shifted by 4; retail has ONE `xor <reg>,<reg>` before the
entry test and tests the parameter with `cmp <param>,<zeroreg>`, we have TWO
`xor`s and `test <param>,<param>`; we carry a `push ebx`/`pop ebx` retail does
not
confidence: 9/10 (broken with objdiff on CMapMgr::Clip 0x2b340, 90.82 -> 98.22)

Two live copies of the *same* constant is one value too many for cl 5.0's four
allocatable registers, so it claims a callee-saved one to hold a member load.
The symptom reads as a whole-frame divergence for what is really one redundant
`xor`.

The fix is TWO levers that only work TOGETHER:

1. write the else arm field-by-field instead of the struct copy — this is what
   collapses the two zeros to one and deletes the `push ebx`;
2. hoist the member loads into locals — this is what stops the field-by-field
   arm from re-loading them.

```cpp
void CMapMgr::Clip(const RECT* src) {   // 0x2b340
    RECT a, b;
    i32 w = m_width;                    // lever 2: one load, reused by both arms
    i32 h = m_height;
    b.left = 0;
    b.top = 0;
    b.right = w;
    b.bottom = h;
    if (src) {
        a = *src;
        a.right++;
        a.bottom++;
    } else {
        a.left = 0;                     // lever 1: NOT `a = b;`
        a.top = 0;
        a.right = w;
        a.bottom = h;
    }
    ...
}
```

## The trap this pattern originally recorded — and why it was wrong

Lever 1 alone scores **85.82** against a `a = b;` baseline of 87.68: the else
arm's four stores each re-load `m_width`/`m_height`, so the frame it saves is
paid back twice over. Lever 2 alone is <= baseline: with the struct copy the
else arm never re-loads anything, so hoisting buys nothing and only perturbs
allocation. Measured separately, each lever therefore reads as a falsification,
and the wall was written up as exhausted (11 spellings, a 400-iteration
hill-climb, a flat 25-point declaration-count sweep).

Composed, they reach 98.22: retail's exact prologue, the single `xor edi,edi`
reused for `cmp eax,edi`, no `ebx`, and every `[esp+N]` on retail's offsets.

This is the concrete case for EXPLORATORY DESCENT: a lever that dips but moves
the codegen toward retail's texture (here: the correct prologue and frame) is a
BASE to compose on, not a result to revert.

## Residue at 98.22 (two instructions, cl's scheduler)

* retail stores `b.left` before `b.top`; cl emits them the other way round.
  Split, reversed, chained and named-zero-local spellings are all byte-identical
  here, so source order does not reach this.
* retail materialises a SECOND zero inside the else block (`xor eax,eax`, then
  `a.top` from `edi` and `a.left` from `eax`); cl uses `edi` for both.

Same family, different trigger:
[known-zero-reload-before-call](known-zero-reload-before-call.md) (a
copy-propagated zero is the extra use that claims the 4th register) and the
inverse [rect-fill-through-a-base-register-is-a-byvalue-rvalue](rect-fill-through-a-base-register-is-a-byvalue-rvalue.md)
(retail materialises a zero PER FIELD where we reuse one).
