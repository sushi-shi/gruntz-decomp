# A crit-bit descent with early leaf exits needs a guarded bottom-tested loop

tags: cpp:loop cpp:pointer cpp:tree | asm:test asm:jcc asm:lea | topic:codegen-idiom topic:regalloc
symptoms: a crit-bit/tree descent has the right calls and referents, but a top-tested loop is seven instructions and two branches short; retail has both a zero-root entry guard and a separate backedge test
confidence: 9/10

`zPTree::FindOrInsert` at `0x1933b0` records both the current descent node and
the candidate leaf while collecting child directions for a later splice.  Its
retail loop has two distinct tests: a zero-root entry guard and a bottom test
after the selected child becomes the next descent cursor.  Write those two
source facts directly:

```cpp
if (m_descentCursor != NULL) {
    do {
        // select child, record it, and handle leaf exits
        m_descentCursor = child;
    } while (m_descentCursor != NULL);
}
```

Do not collapse this to `while (m_descentCursor != NULL)`.  In the controlled
A/B with all child-link spellings held fixed, the top-tested form emitted
`0x287` bytes, 228 instructions, and 33 branches.  The guarded bottom-tested
form emitted 235 instructions--the retail count--and 34 branches; retail is 235
instructions and 35 branches.  Keeping the self-link as its two semantic arms
preserved the remaining control decision that cl5 otherwise folded, producing
the final 237-instruction, 35-branch frontier at 78.736170%.  Retail lowers
those arms through a selected address and common store; the current compiler
state still duplicates the store.

The companion splice rule is to select a child slot as a pointer and then store
through it.  Both later splice walks use the sibling `zPTree::Insert` shape:

```cpp
CButeTreeNode** slot = node->m_child;
if (dir) {
    ++slot;
}
*slot = next;
```

This avoids duplicating the store and matches retail's common store block.
For the initial descent the polarity is reversed: retail first forms
`&node->m_child[1]`, then replaces it with `&node->m_child[0]` when `dir` is
zero.

`zPTree::Insert` 0x16db90 has a separate initial self-link discriminator.  Its
retail block selects one of the two child addresses through explicit arms and
then performs one common store:

```cpp
CButeTreeNode** selfLink;
if (dir) {
    selfLink = &node->m_child[1];
} else {
    selfLink = &node->m_child[0];
}
*selfLink = node;
```

Starting from `node->m_child` and conditionally incrementing the pointer folds
away the arm-closing jump and leaves the function at 89.9948%.  Assigning the
slot in both arms reproduces retail's selected-address/common-store block and
moves it to 93.0619%.  Directly assigning `node` to each child preserves the
branch count but duplicates the store in this TU and reaches only 90.0722%.
The 12-way real-local lifetime matrix, eight inline-helper bodies, six guarded
loop forms crossed with both self-link shapes, and 64 mixed-kind TU-state
forests did not alter those source-shape islands.  The retained 93.0619% state
is still open: retail keeps `dir` in EAX and `node` in ESI, which adds a cold
no-loop trampoline; the base keeps `dir` in ESI and `node` in ECX and falls
directly into the splice.

After calls, returns, branches, and all 10 ordered relocations agreed, a
128-trial parser-visible TU-state campaign found exactly one compiler island at
78.736170%.  That is the stopping signature: preserve the structural source,
report that only one island was found, and continue structurally rather than
retaining declaration noise.
