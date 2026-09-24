# Inline one comparison; macro the repeated short-circuit chain

`CBattlezMapConfig::StepRowUnits` (0x267c0) had six seven-name animation
checks nested as `eq = compare; if (!eq)` or `ne = compare; if (ne)`. Both
result locals are `char`. The lookup can grow `g_typeColl`, so the repeated
checks must keep their order and short-circuit behavior.

The controlled VC5 `/O2` A/B was:

| Source shape | Fuzzy | Base instructions | Branches |
| --- | ---: | ---: | ---: |
| Nested assignments | 86.7956% | 3301 | 529 |
| One bare `&&` chain | 85.9878% | 3279 | 529 |
| Two bare `&&` chains | 85.0508% | — | — |
| Six compact chains retaining `eq`/`ne` assignments | 86.7956% | 3301 | 529 |
| Whole-chain inline function using `||` | 85.8125% | 3285 | 531 |
| Whole-chain inline function using a `char` result | 86.4026% | — | — |
| Nested whole-chain and one-name inline functions | 83.8226% | 3185 | 506 |
| One-name inline function called directly in a compact chain | 86.7956% | 3301 | 529 |
| One-name inline function inside a whole-chain macro at four sites | 86.7956% | 3301 | 529 |

The nested-function candidate was not double-inlined: the base gained a
`HasAnimationActName` call and lost five `IndexToPtr` calls. The same one-name
function inlined when called directly from the guard. The final macro keeps
the seven checks as one source operation while allowing the inner function to
expand at each site. The final base retained its 0x2768 byte size, 139 calls,
and 276 relocations. Its remaining target gap is the preexisting six
`CPtrList::GetNext` calls; this cleanup did not close that wall.

When a repeated guard appears to contain an inline helper, test each proposed
boundary separately. A nested call can exceed `/Ob1`'s budget even when its
callee expands at the caller directly. A macro can preserve the proposed
helper boundary at the larger expression without forcing a retail-absent call.
Keep the comparison-result local and storage width where the compiler uses
them; do not cache the lookup or remove a repeated comparison without retail
evidence.

These measurements used the older opaque container model. Restoring the complete
typed construction/error layer changes the inliner population: this table does
not establish a permanent requirement for caller-owned `char` results or a
whole-chain macro. The [typed-template follow-up](animation-name-accessors-restore-template-call-cuts.md#steprowunits-follow-up-complete-comparison-phases)
explicitly tests that assumption. Removing the unused result coupling and
composing whole-guard and flag-update inlines recovers 45/52 typed calls at
85.3681%. Lookup order and short-circuit behavior remain required, but the
source spelling must be re-tested against the complete family. Neither pass
reaches exact, so neither proves that its retained spelling is original.
