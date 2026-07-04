# A scattered/incomplete unit plateaus — but NOT because of function order (order is byte-neutral)
tags: topic:tu-layout topic:scoring-artifact topic:wall
symptoms: correct-shape functions stuck at 70-99% across a whole unit, 0 byte-exact, unit spans scattered RVAs
confidence: 8/10
variants: within-tu-order-vs-field-order.md

A unit whose reconstructed functions are all close-but-not-exact (70-99%, 0 exact) is a
**per-function reconstruction gap**, not a layout effect. MSVC5 /O2 emits each function as
its own COMDAT and compiles it independently, so a function's bytes do NOT depend on the
ORDER or the SET of its TU-siblings — proven by controlled reorder/removal experiments
(bytes byte-identical after swapping or deleting neighbours; see
[within-tu-order-vs-field-order.md](within-tu-order-vs-field-order.md)). **Correction of an
earlier claim:** regalloc / scheduling / EH-state numbering are per-function, NOT "a
function of the set and order of functions in the TU" — RVA-reordering a stuck function
does not crack it, and completing the contiguous run does not resolve a neighbour's entropy.

So when a whole unit plateaus: fix each function on its own merits (respell the idiom,
correct a struct field OFFSET — field order IS load-bearing), and split conflated units
(two real retail TUs lumped under one name — `iconloaders`/`spriteresource`) so each maps
to one target object. Ascending-RVA definition order is still worth keeping (it matches
retail LINK placement for a future whole-EXE layout match — `docs/link-order-investigation.md`),
but it is a readability/link-layout convention, not a %-lever.

WALL: the plateau clears function-by-function, not by reordering. Evidence: grunt unit
(17/19 reconstructed, 0 exact) — the residual is per-function, unaffected by order.
