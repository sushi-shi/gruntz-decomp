# An Inline Early Return Preserves The Free-List Pop Boundary

## Detection signature

A caller is semantically complete and its normalized base/retail pair has the same
extent, instruction count, calls, branches, returns, relocations, and operand
multisets, but the first divergence is a saved-register choice in the prologue.
The caller contains a small operation that is plausible as an inline helper, and
the operation has a natural failure return before its successful mutation.

`CMapMgr::LinkClosedNode` at `0x09f470` was exactly this shape: both sides were
`0x62` bytes, 43 instructions, zero calls, three branches, two returns, and zero
relocations. All 21 displacement keys, 11 stores, and 43 mnemonics agreed. The
open-coded free-list pop scored 85.90698%.

## Controlled A/B

Extracting only the cell-node free-list pop into a TU-local inline helper changed
the caller to 93.95349% without changing any of those topology or semantic counts:

```cpp
static inline BrickzCellNode* PopFreeCellNode(BrickzCellNode*& freeList) {
    BrickzCellNode* node = freeList;
    BrickzCellNode* next = node->m_cellNext;
    if (next == NULL) {
        return NULL;
    }
    freeList = next;
    next->m_cellPrev = NULL;
    return node;
}
```

The expansion moves the argument load behind two saved-register pushes and keeps
the node/head pair in saved registers, leaving only the ESI/EDI choice against
retail. A pointer to the pool and a reference to the free-list head produce the
same bytes, so that spelling is not the lever. The helper boundary plus its exit
topology is.

Three controls separate that fact from generic declaration noise:

- changing the helper to one shared `return node` after an `if/else` returns the
  caller to 85.90698%;
- inverting the guard to a successful arm followed by `return NULL` falls to
  85.11628%;
- a separate inline cell-head lookup is byte-flat, and 64 target-adjacent VC5
  declaration-forest trials all remain at 93.95349%.

The analogous search-node free-list pop is a negative control. Applying the same
helper mechanically to `FindPath` changes 97.4015% to 87.1515% and to
`ExpandNeighbor` changes 82.2885% to 82.2016%; those uses remain open-coded. This
is therefore evidence for the source boundary at one site, not permission to
deduplicate every similar sequence.

## Reverse-use rule

When a complete same-topology wall starts with a prologue/register divergence,
look for a repeated semantic operation whose failure naturally exits an inline
helper. Test the complete helper and its guard direction as separate structural
axes. Do not treat shared-return and early-return spellings as equivalent merely
because the emitted branch skeleton is equal, and do not propagate a successful
helper to sibling sites without measuring them independently.
