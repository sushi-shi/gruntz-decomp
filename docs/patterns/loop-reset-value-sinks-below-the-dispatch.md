# A member read that lands AFTER the dispatch test means the assignment lives INSIDE each branch

tags: cpp:branch cpp:local cpp:member | asm:mov asm:test | topic:codegen-idiom
symptoms: `mov <reg>,[this+0xNN]` on the wrong side of a `test`/`jne`, a
loop-reset constant assigned above an if/else-if chain, near-clone row loops that
each re-establish the same value
confidence: 9/10

## Symptom

A function assigns a loop's reset value, then dispatches on a clip/mode condition:

```cpp
x = m_width;
if (clip->left == 0 && clip->right == m_width - 1)      { ... }
else if (clip->left != 0)                                { ... }
else if (clip->right != m_width - 1)                     { ... }
```

cl must load `m_width` before the dispatch, so the base obj reads it first:

```asm
mov ecx,DWORD PTR [esi+0x4]     ; m_width      <-- ours, BEFORE the test
mov ebp,DWORD PTR [esp+0x34]    ; clip
mov eax,DWORD PTR [ebp]         ; clip->left
test eax,eax
jne  <arm2>
```

Retail reads `clip->left` **first**:

```asm
mov  eax,DWORD PTR [edx]        ; clip->left
test eax,eax
jne  <arm2>
mov  ecx,DWORD PTR [esi+0x4]    ; m_width      <-- retail, AFTER the test
lea  ebp,[ecx-0x1]
```

`m_width - 1` is needed by the `&&`'s second operand, so retail's load position is
exactly where the short-circuit puts it — which is only possible if **nothing above the
dispatch needs `m_width`**. The reset assignment therefore sits at the head of each
branch, not above them.

## Fix

```cpp
if (clip->left == 0 && clip->right == m_width - 1) {
    x = m_width;
    ...
} else if (clip->left != 0) {
    x = m_width;
    ...
} else if (clip->right != m_width - 1) {
    x = m_width;
    ...
}
```

Measured on `CDDrawShadeBlit::BlitCopyMirrored` 0x149d00 **58.65 → 66.29** and
`BlitShadedMirrored` 0x14b770 52.95 → 53.60 in the same edit. The forward siblings
reset to a literal (`x = 0`), which costs no load, and retail's `xor <reg>,<reg>` there
IS above the dispatch — so the hoisted spelling is correct for them. Read the position,
don't assume the family is uniform.

## Rule

Whenever the first few instructions of a diff show one side loading a member the other
side has not touched yet, the difference is a STATEMENT POSITION, not regalloc. The
short-circuit operand order pins where retail's load may legally sit; anything loaded
earlier than that is a statement you hoisted and retail did not.

related:
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md),
[decl-order-and-assign-order-are-two-knobs.md](decl-order-and-assign-order-are-two-knobs.md)
