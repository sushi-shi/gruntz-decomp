# Branch-local null checks can share only the success tail

tags: cpp:branch cpp:pointer cpp:local cpp:goto | asm:cmp asm:je asm:jmp asm:call | topic:codegen-idiom topic:cfg
symptoms: calls, returns, and relocations agree, but retail has one extra pointer null comparison and branch immediately before a shared call tail
confidence: 10/10
variants: merged-call-arms-expose-the-if-else-order.md, map-lookup-ternary-ifconverts.md, goto-fail-shares-one-exit-block.md

A ternary that selects one of two pointers before testing it gives cl one null
check:

```cpp
T* value = condition ? left : right;
if (value != NULL) {
    Use(value);
    return;
}
Fail();
```

That is not equivalent as source structure to two arm-local checks which share
only their successful tail:

```cpp
T* value;
if (condition) {
    value = left;
    if (value != NULL) {
        goto use;
    }
} else {
    value = right;
    if (value != NULL) {
        goto use;
    }
}
Fail();
return;

use:
Use(value);
```

`CProjectile::AdvanceMotion` 0x0dfd00 is the controlled case. Retail loads and
checks `m_frames[PF_FALL]` in one arm, loads and checks
`m_frames[PF_IMPACT]` in the other, then cross-jumps both successful edges into
one `CAniAdvanceCursor::SetAnimation` call. The ternary source emitted 462
instructions, 72 branches, 20 calls, three returns, and 49 relocations against
retail's 464/73/20/3/49, with one missing `cmp`/`je` pair and a 96.92 score.

Duplicating the full `if (value != NULL) { Use(value); return; }` body in both
arms is the negative control: cl retained two `SetAnimation` calls and produced
475 instructions, 21 calls, four returns, and 50 relocations. The explicit
arm-local checks plus the shared `use` label instead produce retail's exact
0x70c extent and 464/73/20/3/49 topology, raising the function to 97.00. Its
remaining difference is register allocation and instruction selection; the
semantic operand and ordered-referent sequences agree.

Use the target's branch placement as the discriminator. If each arm contains a
load followed immediately by its own null check, while both non-null edges
reach one call, do not collapse the source to a ternary. Conversely, one null
check after the arm join is evidence for the ternary/shared-check form.
