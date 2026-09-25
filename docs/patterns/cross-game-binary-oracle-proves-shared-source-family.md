# Sibling binaries corroborate implementations, not source spelling

A source-path or library anchor plus a complete matching function in a sibling
binary is evidence of a shared compiled implementation family.

The [recorded Claw/Gruntz Blit824 comparison](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/cross-game-binary-oracle-proves-shared-source-family.md#exact-witness)
reports two 0x30b-byte bodies, differing only in a call displacement and
independently explained member offsets. This is historical evidence about the
two retail binaries, not a current reconstruction score or a bounded-wall verdict.

For reuse, verify the complete decoded extent and control flow, and account for
every differing byte using relocation, ABI, or revision evidence. Keep the binary
versions and addresses identifiable. A short byte signature is insufficient.

Even complete binary agreement does not recover macros versus inlines, original
local names, or a unique source expression. It also does not prove a remaining
reconstruction mismatch is solely compiler state. An absent call does not prove
that no source helper existed.

Gruntz retail remains the authority. See [source evidence](surviving-source-lineage-restores-typed-layers-and-order.md)
for available source and debug-object evidence.
