# A scattered/incomplete unit usually plateaus per function; test TU-state exceptions
tags: topic:tu-layout topic:scoring-artifact topic:wall topic:regalloc
symptoms: correct-shape functions stuck at 70-99% across a whole unit, 0 byte-exact, unit
  spans scattered RVAs; or an unchanged function moves after a predecessor is reconstructed
confidence: 7/10
variants: within-tu-order-vs-field-order.md, preceding-function-state-recolors-later-comdat.md

A unit whose reconstructed functions are all close-but-not-exact (70-99%, 0 exact) is a
**usually a per-function reconstruction gap**, not a layout effect. MSVC5 /O2 emits each
function as its own COMDAT, and several controlled small-function reorder/removal
experiments were byte-neutral (see
[within-tu-order-vs-field-order.md](within-tu-order-vs-field-order.md)). That is the useful
default, not a guarantee: a later controlled experiment proved that adding a substantial
earlier definition can change scheduling and register choices in an unchanged later
COMDAT. See
[preceding-function-state-recolors-later-comdat.md](preceding-function-state-recolors-later-comdat.md).

So when a whole unit plateaus, first fix each function on its own merits (respell the
idiom, correct a struct field OFFSET—field order is load-bearing), and split conflated
units so each maps to one target object. If an unchanged function moves when a real
predecessor lands, A/B-test that predecessor and use TU-state trials; do not mislabel the
movement as an edit in the victim. Ascending-RVA definition order remains the evidence-
backed default for retail link layout and can also restore authentic predecessor state.

WALL by default, TU-state exception when proven. Evidence: the grunt unit plateau cleared
function-by-function in the original test; `BlitIntoDesc` later disproved the universal
claim by recoloring source-identical `ShadeRect`.
