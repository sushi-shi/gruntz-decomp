# Nested error helper and direct template casts restore inline cuts

tags: cpp:template cpp:inline cpp:cast cpp:ternary | asm:call asm:coff | topic:source-oracle topic:inline-budget

An erased accessor's arithmetic can be correct while the surrounding template
has the wrong inline tree. The two-action registrars expose this through four
array accesses with three out-of-line error reports and one expanded report.
The source decision is shared; there is no need for caller-specific accessors
that prescribe which error path may inline.

The surviving source behind ledger entry `nolf-zdarray-call-boundaries` uses
`zDArray<T>::operator[]` -> `_zdvec::get` -> `zErrHandling::handle_inl`.
The raw accessor returns one conditional expression. Its typed caller casts
the returned pointer directly; it does not call a cast helper.

Controlled VC5 `/O2 /MT` builds of `RegisterIconActions` (0x979e0):

| Source | Fuzzy | Bytes | Calls | Branches | Relocations |
| --- | ---: | ---: | ---: | ---: | ---: |
| PR baseline: expanded error body, split raw accessor, `AsElem` helper | 65.3184% | 532 | 16 | 16 | 70 |
| Restore nested header-inline `Report` | 80.4975% | 575 | 15 | 21 | 74 |
| Compose surviving conditional-expression accessor | 98.3582% | 667 | 16 | 26 | 86 |
| Remove invented `AsElem`; retain direct named casts | 100% | 684 | 17 | 26 | 89 |
| Retail | 100% | 684 | 17 | 26 | 89 |

All variants have three returns. The final normalized instructions and ordered
relocations are identical to retail. The intermediate 98.3582% body expands
all four raw accesses but calls `Report` four times; removing the cast helper
allows the second name-registration error site to expand into `GetRetAddr`
and `CVariantSlot::Set`, leaving exactly three `Report` calls. This is a
controlled boundary effect, not evidence that cast arithmetic itself costs
runtime instructions. No numerical inline-budget estimate is claimed.

Completing the source pointer boundary retains these closures: the raw
accessor's result, initialization pointer and overflow pointer are `void*`,
while the arithmetic buffer remains `char*`. Typed construction now needs one
direct `static_cast<T*>` at each erased boundary. Only destruction converts
the byte buffer through `void*`; no extra nested-cast sites are introduced by
removing `AsElem`. The raw accessor's annotation follows its deliberate
`PAD` -> `PAX` return-type mangling change. Both typed indexers, the raw
accessor and the emitted error helper are exact; growth remains at its prior
91.8584% with unchanged behavior.

The full rebuild reproduces exactness in all nine two-action registrars:
InGameIcon, DroppedObject, GruntVoice, PathHazard, SpotLight, StaticHazard,
CheckpointTrigger, and both Wormhole registrars. The typed indexers and the
erased accessor remain separate real source layers.

For the six-action `RegisterWarlordActions` (0x447a0), the first five entries
already match. The last name accessor remains a call while retail expands it;
both sides call the last handler accessor. Uniformly naming the handler result
as `CActHandler& slot_ = table[id_]` before assigning through it closes the
whole function: 819 bytes, 243 instructions, 36 calls, 24 branches, one return,
and 114 relocations. This ordinary reference spelling removes no abstraction
and does not choose raw/typed accessor APIs by call-site position.

Negative controls in the larger Grunt registrar matter: the same named
reference over-expands further (777 -> 820 instructions versus retail 734),
and direct aggregate initialization of its PMF union is byte-flat. Reusing its
assigned local ID for all name accesses changes early instructions but not the
wrong call set. Those unsuccessful caller edits are not retained. Neither
this result nor a lower fuzzy score certifies that registrar's remaining
inline-boundary question as bounded.

A further uniform PMF-conversion helper reduces that registrar from 777 to
749 instructions (retail: 734), but all 38 raw accessor calls remain; retail
instead calls 35 raw accessors and three typed accessors. Composing a named
handler reference returns to 777 instructions. Wrapping the complete bind
operation leaves 13 typed-handler calls (14 when conversion is separated from
the four-byte setter), versus retail's two, and grows to 872 instructions.
These are real nested-boundary effects, but none reproduces the retail call
topology. The function-form experiments do not establish their inline cuts.

The conversion and typed table-write abstractions are retained as
`CONVERT_GRUNT_ACT_PMF` and `STORE_GRUNT_ACT`, composed uniformly by
`BIND_GRUNT_ACT`. The macro form evaluates each input once and preserves the
original union-local/assignment sequence without an additional callable
boundary. Its complete normalized GruntCombat object is unchanged. This keeps
both useful source operations without unused shadowed function definitions,
new emitted symbols or caller-specific expansion selectors. Their function
forms can be revisited when the remaining inline topology is understood.

The first nested-helper-only test did not emit `Report` in its annotated
owner, `BattlezSpecialAnim.cpp`. Composing the surviving accessor expression
restored natural emission there. Do not add a dummy caller or force emission
to repair an intermediate composition.

Reverse use: when repeated generic accesses disagree at different nested call
sites, inspect the complete surviving helper tree, including conversion-only
wrappers and expression grouping. Restore authored helper boundaries, but
remove invented ones rather than preserving every abstraction indiscriminately.
Compare from the first divergence after each composition; retain templates,
reference returns, typed placement construction, and actual pointer types.
