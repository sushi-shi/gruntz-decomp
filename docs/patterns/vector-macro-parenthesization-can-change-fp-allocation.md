# Vector macro parenthesization can change VC5 FP allocation

tags: cpp:macro cpp:scope cpp:expression | asm:fxch asm:fstp | topic:codegen-idiom topic:scheduling

The `CSpotLight::Update` production-TU control distinguishes an authentic vector
macro expansion from merely adding a lexical block around field assignments.
Do not infer that a scope alone explains a macro's scheduling effect.

At source checkpoint `d1aebab0f`, the two ordinary rotation-result assignments
emit 291 bytes / 92 decoded instructions, 73.26966%. Replacing that pair with
the existing width-preserving `VEC2_SET` emits 281 bytes / 87 instructions,
80.25843%. Both have one call, three branches, one return and five ordered
references. The independent focus initialization macro is byte-flat. The other
17 scored TU records are unchanged.

The controlled spellings are:

| Rotation statement form | Result |
| --- | --- |
| Original two field assignments | 73.26966%, 291 bytes |
| Same assignments in a plain block | Identical to original |
| Literal macro expansion: parenthesized receiver and RHS, block plus trailing semicolon | Identical to macro, 80.25843%, 281 bytes |
| Same parenthesized expansion without the trailing semicolon | Identical to macro |

Thus the bare block and trailing empty statement are not sufficient causes.
The parenthesized expression form accounts for this measured difference; which
individual parentheses drive C1's change was not separately isolated. Do not
generalize this as a semantic arithmetic change, a source-line grouping effect,
or a universal optimizer rule.

The first difference still precedes these stores: retail reserves 32 stack
bytes, both baseline and macro reserve 24. The macro recolors the x87 calculation
and removes instructions, but does not close that scratch/lifetime mismatch.
Both original and retained code write the rotated position before the optional
focus update, then translate and store position again. Preserve all three phases.

Safe reverse use: where a sourced macro is applicable, test its complete token
expansion in the real TU. Compare against the original baseline, not just a
dipped member-inline state. Never retain bare scopes or extra empty statements
as steering devices; keep the actual helper call. This checkpoint retains three
real `VEC2_SET` uses and no experiment scaffolding.

Canonical adoption and deferred alternatives are the
`reassess-spotlight-vector-macro-sites` and `reassess-spotlight-*-controls` ledger
rows. The vector constructor/operator family and Tick's shared-epilogue gap
remain open. `scripts/test_spotlight_vector_helpers.py` checks bounded write
phases, widths, the unsigned frame delta and all five Update references against
the original PE, with negative mutations. It does not certify complete FP
equivalence or the unresolved Tick CFG.
