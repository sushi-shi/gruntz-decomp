# A shared literal-return label can be source-equivalent to direct returns

tags: cpp:goto cpp:return | asm:jmp asm:ret | topic:codegen-idiom topic:source-shape
symptoms: many guards jump to a label containing only `return <literal>`; the label performs no cleanup, state change, or common work
confidence: 9/10

A source-level `goto` is not required merely because retail shares a return
epilogue. MSVC 5.0 can cross-jump repeated direct returns into the same machine
tails. When the destination consists only of a literal return, replacing each
jump with that return is a bounded cleanup A/B that adds no nesting.

`CGrunt::StepObjectGuardBehavior` at `0x0f1c70` is the large control. Replacing
all twenty `goto tail` statements with `return 1` and deleting
`tail: return 1` leaves the compiled function unchanged at `0x5f4` bytes, 483
instructions, 24 calls, 59 branches, 13 returns, and 30 relocations. Its fuzzy
score remains 84.2845%; the residue still begins in register scheduling at
`+0x1c`. Five separate `goto resetState` statements remain because that label
performs a real state transition before returning.

This does not supersede the partial-exit regimes in
[`goto-fail-shares-one-exit-block.md`](goto-fail-shares-one-exit-block.md).
Keep a shared label when it owns cleanup, rollback, a state update, or any other
operation besides the return, and keep it when the return/branch census proves
that direct returns factor differently. In particular, do not flatten a
source-attested cleanup ladder.

Reverse-use checklist:

1. Prove that the label body is exactly one literal return and is not a switch
   identity target or lifetime boundary.
2. Replace only jumps to that label; leave other semantic labels unchanged.
3. Compare calls, branches, returns, relocations, EH state, and the first real
   divergence in the complete function.
4. Keep the direct form only when the retail topology and historical-MAX gate
   remain intact.
