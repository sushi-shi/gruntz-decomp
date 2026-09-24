# An empty value constructor can exhaust the /Ob1 budget

`TmDeflectStep` is a useful warning when a large caller suddenly gains many
external calls after a small value-type change. Its 192 local `Coord` objects
are default constructed. With the math-helper `Coord() {}` and two-argument
constructor in `CoordNode.h`, the current VC5 object scores 83.3681% against
retail; the earlier trivial `Coord` state scored 99.4577%. The current object
calls `CellFlagsAt` 111 times versus retail's 75, `GetTileGrid` 110 versus 75,
`Coord::Set` 112 versus 75, and `TmFlagsAllow` 56 versus 37. The TU emits its
own COMDATs for these functions, so they are eligible for `/Ob1` expansion.

A source-body control restored the earlier `TmDeflectStep` expressions while
leaving the new `Coord` class in place. It stayed at 83.3681%. Moving the
additional math methods out of the class, and separately reducing the header
to the methods required by this TU, also left that score flat. A disposable
family control removed both user-defined `Coord` constructors, supplied the
two-argument spelling through a macro-backed factory, and adapted the TU's
local initializers. The same real VC5 TU then returned to 99.4577%. This
controls the constructor family, not either constructor in isolation; the
extra default-constructor candidates are the strongest explanation for the
caller-wide budget shift, but the experiment does not identify a single
threshold or an original source declaration.

The family control was **reverted**. In a full-build trial it left 38 fresh
MAX regressions and changed `CSpriteRef::Build` from exact to 96.4676%.
Changing `Coord` in one TU would also violate the shared class model. The
proper response is to keep the exact-match family intact and test authentic
macro expansions at the mismatching call sites, with the ordered call and
relocation topology as the arbiter. A higher score for one enormous caller
does not prove a globally different value type.

Recognition signature: a caller with many implicit value-type locals has
the same semantic stores and near-identical CFG, but several distinct
inline-visible callees all become external at roughly the same frequency.
Before adjusting individual calls, test the complete constructor/API family
in a disposable real-TU build and check every exact owner that uses it.
