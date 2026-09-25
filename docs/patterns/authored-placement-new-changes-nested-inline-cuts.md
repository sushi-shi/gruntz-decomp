# Authored placement new can change nested inline cuts

tags: cpp:template cpp:inline cpp:new cpp:constructor | asm:call | topic:source-oracle topic:inline-budget

An exact standalone template accessor does not prove that its placement-new
source is interchangeable inside larger callers. ZTools supplies an inline
four-argument allocation overload and calls `new (p, 0, 0) T`. Substituting
standard placement new preserves the element lifetime operation but can
change VC5's nested expansion decisions.

The source and retained adaptations are tracked by lineage ledger entry
`nolf-zdarray-typed-family`. The overload belongs in the shared ZTools headers,
not a synthetic emitter TU. Both constructor and indexing sites use it.

Controlled builds of `RegisterGruntActions` (0x5be30), retaining the same
PMF-conversion and table-store macro helpers:

| Placement source | Bytes | Instructions | Calls | Branches | Returns | Relocations |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Standard `new (p) T` | 2796 | 814 | 116 | 84 | 3 | 387 |
| Authored `new (p, 0, 0) T` | 2691 | 786 | 115 | 79 | 3 | 375 |
| Authored calls plus standard-new include | 2691 | 786 | 115 | 79 | 3 | 375 |
| Standard calls, both overload declarations visible | 2796 | 814 | 116 | 84 | 3 | 387 |
| Retail | 2533 | 734 | 113 | 73 | 1 | 358 |

Grouping the accessor's pointer locals as in the source is byte-flat by
itself. The include/declaration controls show that the placement call boundary,
not merely parsing an extra declaration, changes the nested expansion.
Both typed indexers remain exact: CString is 141 bytes/56 instructions and
the handler specialization is 116 bytes/45 instructions. The six-action
Warlord and two-action Icon registrars also remain exact in these controls.

Composing a value-returning PMF conversion helper on this base gives 749
instructions, but still differs from retail's call set. Passing its input by
const reference is byte-flat. Neither experiment establishes exact recovery
or a numerical budget deficit.

A named handler reference also gives 749 instructions but loses the retained
typed-handler call. Separate inline conversion and binding functions give
846 instructions and fourteen typed-handler calls instead of retail's two.
The retained conversion and binding abstractions therefore remain macros;
these experiments do not close the registrar's remaining inline/call-set gap.

For reverse use, inspect the complete allocation-overload and template family
when exact small callees expand differently in a large caller. Test the
authored call sites with unchanged declarations as the negative control.
Do not invent dummy parameters or overloads to tune a score: here the complete
overload and both uses survive in the original source. Do not force individual
registration sites to use different accessor APIs.

The original compiler-artifact scanner matched the `operator new(` token in
this definition as an explicit allocation call. Its correction admits only
the complete source-proven definition at its owning path, exactly once.
Full-gate controls still reject an explicit call, a duplicate, an altered
body, a missing definition, and the same definition outside its owner.
Real-VC5 header controls compile both placement forms in either include order
and instantiate nontrivial element construction, indexing and destruction.
