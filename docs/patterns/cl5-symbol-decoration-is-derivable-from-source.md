# cl 5.0's symbol spelling is a deterministic rewrite of the mangled name + linkage

**Confidence: 10/10.** Measured over the whole claim corpus (5,685 source claims across
300 TUs); re-proven every build by
`gruntz verify selftest -k SourceNameRewrite`.

## The claim

Given a C++ declaration, cl 5.0's COFF symbol for it is a pure function of clang's
mangled name for the same declaration and of the declaration's linkage. No object has
to be read to learn it — which matters because an object is a build artifact that can
be stale, and a labelling pipeline that consults one lets a stale artifact answer for
source that has changed.

Three rules, and nothing else, cover the corpus:

| # | rule | clang | cl 5.0 |
|---|------|-------|--------|
| 1 | the i386 COFF global prefix | `s_MAIN` (IR value name, unprefixed) | `_s_MAIN` |
| 2 | top-level array storage class | `?g_unitIndexBitTable@@3QBGB` | `?g_unitIndexBitTable@@3PBGB` |
| 3 | TU-local storage | `s_MAIN` / `?s_x@?1??F@@QAEHXZ@4HA` | `_s_MAIN$S<n>` / `_?s_x@?<m>??F@@QAEHXZ@4HA$S<n>` |

Rule 1 is LLVM's, not cl's: the i386-windows-msvc backend prefixes `_` to every symbol
it did not mangle itself, and leaves an MSVC-mangled `?…` name alone. It shows up as a
rewrite only because the two clang probes disagree about WHEN it is applied — the IR
value name is undecorated, libclang's `mangledName` already carries it. Getting this
backwards silently loses the join to `var_facts`, which is how ~284 file statics were
carrying no declared extent at all.

Rule 3's `_` and `$S` arrive TOGETHER and depend on storage, not on mangling: a file
static, a namespace-scope `const`, and a function-local static all reach the object as
`_<mangled>$S<n>`, while a class-scope static (external linkage) gets neither.

## The two volatile numbers

`<n>` in rule 3 is c2's per-object CodeView counter, and `<m>` is c1's lexical-scope
number for the block enclosing a function-local static (MSVC spells 1..10 as the digits
`0`..`9` and larger values in hex as `A..P@`; clang always writes `?1`). BOTH renumber
on any edit to the translation unit, so neither is identity and neither is ever stored:
`core.msvc_names.mask` reduces either side of a name join to the ordinal-free form —
canonical scope `?1`, bare `$S` — and that is what makes a source-derived claim and
cl's own object symbol meet. Proven injective: no base object holds two distinct
symbols that mask together (13 symbols tree-wide carry more than one `$S<n>`, 43 a
scope number, no pair).

## When the family needs a number back

Several TUs can hold a same-named TU-local static; cl tells them apart with its
counter, and the delink data manifest needs one name per address image-wide. The
discriminator is the retail RVA, in the slot the counter occupied
(`_s_gruntDirEast$S2378096`) — the convention `$T<rva>` FP-pool slots already use — and
`mask` folds it straight back onto the family, so compare still pairs the copies by
content. 36 rows need it (20 `_$S31$S`, 6 `_$S58$S`, 8 `_s_gruntDir*$S`,
2 `_kMsToSeconds$S`).

## Detection signature

A labelling pipeline that name-matches an object will show a rule gap as a SILENT
per-claim drop — the label just stops existing, and the loss is invisible unless some
other claim covers the same address. The measurable form: a source macro whose address
appears in no claim fragment, or a datum whose declared extent is blank while its
neighbours have one. The fix is a corpus-wide control instead of a per-claim test: one
loud assertion that `rewrite(clang_name)` equals the emitting object's symbol modulo
the masked ordinals, for every claim, once per build.

## Negative control

Undo rules 2 and 3 over the extracted corpus and the control fails naming cl's actual
spelling (`data 0x1e9178 g_col1Rects -> cl spells it _g_col1Rects$S`). That control is
itself a test in the suite, so the gate cannot go green on an incomplete rewrite.
