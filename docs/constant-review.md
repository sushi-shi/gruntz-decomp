# Reviewing integer constants

Run inside `nix develop`, with `GRUNTZ_DIR` set to the current worktree. After
adding a vendor include directory, regenerate the analysis compilation database
with `python3 -m gruntz.graph.compdb` before running either census.

```
gruntz verify constants
gruntz verify enum-reuse
```

Both commands require every project C++ translation unit to parse. A failed
parse is an incomplete census, never evidence that the affected files have no
findings. Reports are derived files under `build/gen/`; regenerate them after
source changes.

## Literal coverage and destinations

`bare_constants.tsv` records source position, enclosing function, expression
classification, value, semantic destination and coverage origin. AST rows carry
the compiler's expression context. A lexical backstop also records integers in
unused macros, inactive branches, unreferenced headers and resource scripts.
Resource-script rows are identified separately from C++ expressions. These rows explicitly
lack AST evidence. Retail address/size annotations are identified separately;
unparsed source is still review work.

`constant_contexts.tsv` groups sites by declaration identity: a callee parameter,
field or variable, comparison operand, switch discriminator, array index/extent,
or function return. Each group includes a total and up to twenty example
locations; the site report retains every location. Overloaded functions and
methods with the same spelling in different classes have different keys.
Macro-expanded arguments are distinguished by their AST ancestor paths; their
source extents can overlap even when they occupy different parameters.
Intermediate operations remain in the
key, so an operand used to construct a value is distinguishable from the value
itself. Sites without a proven destination keep separate source keys.

These are review leads, not equivalence proofs. The same `Read` length parameter
can receive sizes for unrelated records. Follow the destination buffer and its
declaration before substituting `sizeof`; a deliberate partial read must keep
its existing bound. Likewise, `0`, `1` and `-1` can be state codes, versions or
sentinels as well as arithmetic identities. Small values are not automatically
classified as safe to leave unnamed.

For each domain, inspect its complete producers and consumers. Use an existing
name when the identity agrees, introduce only evidence-backed names, and keep
obvious arithmetic/data literals. A pending report row means this review has
not been completed. `constants --gate` checks only the small compiler-proven
replacement set and legacy boolean spellings; passing it cannot establish that
every literal has been accounted for.

## Named constants and uniqueness

`enum_reuse.tsv` includes evaluated enum members, object-like constant macros and
const integral declarations. Clang evaluates expressions using the target ABI;
runtime const locals and non-integral declarations are not integer constants.
`named_constant_coverage.tsv` records the evaluation/exclusion evidence. An
unresolved declaration must remain visible rather than being assigned zero or
silently dropped.

If named-constant coverage is incomplete, numeric reports use `.partial.tsv`
suffixes and `enum_reuse_status.json` records the incomplete state. Old successful
reports are removed so they cannot be mistaken for results of the failed run.
The status file's `complete` flag describes census coverage; pending ledger
decisions still fail the separate review check.

`enum_value_collisions.tsv` groups equal values and shows shared named/literal
destinations. `enum_domain_pairs.tsv` ranks shared semantic uses before numeric
overlap alone. Neither report authorizes merging names: equality of integers
does not establish equality of concepts.

Record decisions in the existing `config/reviews/enum-reuse.tsv` ledger.
`enum-reuse --extend-ledger` appends newly discovered domains as pending; it
does not mark them reviewed. Retain distinct domains with a concrete explanation
of their different meanings, or map duplicate names to their canonical owner.
Verify any source/signature change with the normal build and merge gates before
banking it.

When a source alias is replaced by an object-size expression or a vendor name,
use the `replace` decision instead of retaining an unnecessary declaration.
Map each removed member as
`OLD=expression:src/Owner.cpp::sizeof(buffer)` (or the SDK identifier) in
`member_reuse`. Keep ordinary mappings for any surviving members of the same
domain; `current_enums` lists only their current domains. Preserve the original
member/value snapshot and explain the replacement's meaning and evidence in
`reason`.

The check requires the retired name to be absent from project code and the
replacement tokens to occur in the specified source file. These checks detect
stale decisions; they do not establish semantic equivalence or evaluate the
replacement. Review the complete use family, SDK declaration or object layout,
and verify the compiled result before accepting the decision. A named
compile-time value does not imply a stored retail datum or justify a `DATA` tag.
