# Repeating a pure inline expression can change the CSE value's identity

tags: cpp:inline cpp:expr cpp:local cpp:loop | asm:lea asm:add asm:mov | topic:codegen-idiom topic:regalloc topic:scheduling
symptoms: retail computes a value once with a three-operand `lea` and retains it across two calls, but a named source local computes it in place with `add` and swaps the value and receiver's callee-saved registers
confidence: 10/10

A named common local and the same expression repeated at two semantic use sites
do not necessarily create the same VC5 optimizer value. The repeated form can
still be common-subexpression-eliminated to one machine computation while
giving the value a different creation point and lifetime.

## Exact witness

`CTileTriggerContainer::SetCell` 0x117f60 derives the integer cell key for two
independent lookups. The named-local reconstruction was:

```cpp
i32 key = CellKey(tileX, tileY);
CTileActionEvent* elem = FindActionByCellKey(key);
// ...
if (FindLogic(key, TRIGID_COVERED_POWERUP_26) != NULL) {
```

It scored 85.4412%. VC5 held the key in ESI, held `this` in EDI, and formed the
key in place with `add`. Retail instead holds `this` in ESI, computes the
shifted X term in EAX, and uses `lea edi,[eax+ebp]` for the key.

Writing the semantic operation at both consumers produces retail's prologue
and register assignment even though C2 still computes the key only once:

```cpp
CTileActionEvent* elem = FindActionByCellKey(CellKey(tileX, tileY));
// ...
if (FindLogic(CellKey(tileX, tileY), TRIGID_COVERED_POWERUP_26) != NULL) {
```

That change reached 93.60%. A repeated raw `(tileX << 8) + tileY` expression
was byte-identical, so the inline boundary is semantically preferable and is
not a codegen trick.

The remaining literal lifetime was a separate source-shape fact. Four explicit
`flags[n] = 1` statements made VC5 hoist `1` into ESI across the player-index
test and reuse it for both the stores and returns. The ordinary source loop

```cpp
for (i32 i = 0; i < 4; i++) {
    flags[i] = 1;
}
```

fully unrolls to the same four stores, but confines the fill value to the
all-players arm. Composed with the repeated key operation it closes the
function at 100.0000%, exact 0xa1 bytes, 69 instructions, five calls, three
branches, four returns, and five relocations. Duplicating the action-code tail
inside both source arms dropped to 88.97%, and an inline single-player setter
was byte-flat, which separates the loop's literal lifetime from tail merging
and accessor spelling.

## Reverse-use rule

Use this when retail and base already agree on calls and CFG, but retail uses a
fresh destination `lea` for a value that the base computes in place and the
two callee-saved roles are exchanged. If a named common local feeds separated
semantic consumers, repeat a pure inline operation at those consumers and let
C2 decide whether to CSE it. Confirm the result against a raw-expression
control; retain the helper when both are byte-flat.

For a short fixed-size fill, also test the natural source loop before treating
a hoisted fill literal as irreducible scheduling. Verify that VC5 fully unrolls
it and that retail has the same store run.

## Related

- [`derived-value-local-forces-in-place-arith.md`](derived-value-local-forces-in-place-arith.md)
- [`pair-store-run-off-a-rederived-element-pointer.md`](pair-store-run-off-a-rederived-element-pointer.md)
- [`one-use-local-is-a-regalloc-knob.md`](one-use-local-is-a-regalloc-knob.md)
