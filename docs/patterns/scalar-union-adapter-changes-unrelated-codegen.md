# A scalar union adapter can change unrelated code generation

tags: cpp:inline cpp:cast cpp:union cpp:template | asm:mov asm:lea | topic:tu-state topic:source-shape

An unnecessary union local inside a shared inline can affect functions that
never call that inline. This must be distinguished from the inline boundary
of the function whose instructions changed.

The action registry stores integer action IDs in pointer-valued symbol-table
slots. `ActFindId` and `ActInsertId` used `AddrWord<i32>` locals solely to copy
between those representations. Direct, reviewed `reinterpret_cast`s express
that 32-bit seam without an extra union instance. Both named inline helpers
and the typed `zSymTab<i32>` remain.

With no edit to `CGruntHealthSprite::HealthUpdate` (0x7f180), this replacement
moves 95.072464% to an identical normalized pair: 180 bytes, 69 instructions,
two calls, six branches, two returns and all four ordered relocations. The
registry load returns to its retail position before the index arithmetic.
Replacing the now-unused `AddrWord.h` include with `Ints.h` retains exactness;
the old include is not retained as a state carrier. The full direct-conversion
build also makes the unchanged `CTimer::AddTime` exact.

This is a TU-state consequence, not evidence that HealthUpdate lost its
lookup helper. Its proven `FindGruntByIdentity` boundary stays intact.
Const-pointer, const-member and reference-receiver variants had not closed
the old state. Unchanged exact registrars remain exact, although other caller
states can move and must be covered by the full build.

A separate macro control is not interchangeable with the direct conversions:
turning the two action-ID adapters into macros over-expands the Grunt and
Warlord registrars, to 916 and 279 instructions versus retail's 734 and 243.
The retained adapters therefore remain inline functions. This does not affect
the separate PMF conversion/table-store macros in GruntCombat.

For reverse use, audit inferred representation adapters against the actual
ABI and surviving source. Prefer the real conversion over an unnecessary
union, but never change a correct value type merely to move parser state.
Separate helper-use controls from declaration/type-state controls and retain
only the justified source, not unused declarations.
