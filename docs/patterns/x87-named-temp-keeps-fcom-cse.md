# x87: a named float temp keeps the compare on-stack (fcom); respelling the expression recomputes (fcomp)

**Signature.** An x87 compare-then-use: retail pops at the compare (`fcomp`)
and **recomputes the whole product chain** in one arm — or the mirror image,
retail keeps the value on the stack (`fcom` … `fstp st(0)`) while our build
recomputes. The fuzzy% gap is large (this exact wall parked ~23 points on
`CShadeTableCache::FlashTable`).

**Mechanism (cl 5.0 /O2).** Whether the compared value survives the compare is
decided by CSE over the *source spelling*, not by scheduling:

- A **named `float` temp** used in both the compare and the arm is one value —
  cl keeps it on the x87 stack: `fcom` at the test, `fstp st(0)` when done.
- Writing the **expression twice** (once in the condition, once in the ternary
  arm) defeats the CSE — cl compares with `fcomp` (pop) and re-emits the whole
  chain in the arm, exactly retail's recompute shape.

Pick the spelling that matches retail's compare opcode: `fcomp` + duplicated
chain ⇒ write the expression out twice; `fcom` + reuse ⇒ name the temp.

**Trap.** A dossier claim that two f-spellings are "measured byte-identical"
can be stale — this exact pair was recorded as identical and was not
(opus-A wave, FlashTable 60.9 → 83.8). Re-measure before trusting a parked
x87 verdict; see [[our-guesses-cite-themselves-as-evidence]].

**Related:** [[cse-partial-term-looks-like-a-constant]],
[[struct-return-rvo-idioms]] (adjudicate per site).
