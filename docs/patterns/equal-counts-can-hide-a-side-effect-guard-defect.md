# Equal counts can hide a side-effect guard defect

tags: cpp:branch cpp:inline | asm:jcc asm:call | topic:verification topic:source-model

Call multisets, branch/return counts and ordered references are useful screens,
not proof of control-flow equivalence. Moving one existing call across a
predicate can preserve all of them, including store/immediate multisets.

The control is LoadEntranceConfig at `19c1b9013`. Its base/retail pair has
13 calls, 19 branches, one return and 24 relocations on each side. `diagnose`
classified it as register/scheduling; `semdiff` found no exclusive operand
keys and no ordered-reference difference. A direct retail-edge inspection
nevertheless disproved the source guard. Canonical details and scope are in
lineage rows `fk-reset-current-player` and `fk-x-unconditional-resetcell`.

Moving only that existing call inside its owning guard changes the real-TU
score from 89.0307 to 89.0526 while leaving the size and counts unchanged.
That small movement is a correctness repair, not evidence that the defect
was unimportant. Subsequent complete helper composition changes the frame
and score again; it must preserve the corrected edge.

`scripts/test_entrance_player_guard.py` checks freshly compiled owners and
original retail executable bytes. Its bounded contract distinguishes the
two current-player outcomes and follows call traces through the safety
configuration call. In-memory negative controls redirect only a retail branch
destination, preserving counts and ordered calls. They must fail the same
checker that accepts the real compiled owners, not a separate recognizer-only
test. It never modifies or launches the executable.

Reverse-use signature: a small score gap, same call population and semantic
multisets, but a side-effect call lies near a predicate's merge point. Inspect
the actual target and fallthrough paths. Check which calls each path reaches
before considering register allocation or compiler-state experiments.

The diagnostic output now explicitly calls its register/scheduling result
provisional: the generic ladder does not verify branch destinations. This
bounded owner test does not upgrade that screen into a whole-program semantic
CFG verifier, nor establish that other similarly shaped guards are correct.
