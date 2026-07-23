# Volatile `_$E<n>` helpers: a normalization refresh can expose false exact matches

symptoms: after refreshing normalized objects, many tiny static-initialization helpers
lose exact matches together; common size quartets are `0xa/0x15/0xe/0x1f` or nearby
variants; source contains `RVA_COMPGEN(..., _$E<n>)` claims; the real source-level
static objects and their constructors remain structurally correct

`_$E<n>` is an MSVC per-object emission ordinal, not semantic identity. Adding,
removing, or reordering compiler emissions can renumber it. Earlier doctrine
incorrectly treated any such placeholder as safe because the normalizer
content-hashed both sides.

That assumption is incomplete. The base object can carry relocations inside a helper
while the delinked retail helper lacks the corresponding relocation records. One hash
then normalizes relocation sites and the other retains resolved address bytes. A stale
normalized target copy can hide this difference and temporarily inflate exact/MAX
measurements; a genuine refresh exposes the dip.

The proven firing was the static-activation conversion in commit `28a6f9848`.
Real template/static objects replaced manual fake initialization functions, while
numbered helper labels were added. Refreshing the target normalization later removed
the apparent helper matches. The source structure was correct; the label identity and
the old explanation were wrong.

## Policy

- Keep the real source-level static object and its authentic construction/destruction.
- Do not write `RVA_COMPGEN` for `_$E<n>`.
- Record observed RVA, size, ordinal, unit, and former source location in
  `config/compiler-generated-functions.tsv`; the file is navigation evidence and is
  intentionally outside the label/objdiff pipeline.
- Preserve MAX/high-water history. Never lower it to make the refreshed current report
  look clean.

## Reverse audit

When a refresh causes a broad exact-match loss concentrated in tiny static-init helper
quartets, search for stale numbered compiler-private claims and compare base/target
relocation tables before touching the real static objects. The same signature can find
other cached or mislabeled ordinal islands.

`name$S<n>` data is a narrower case: a `DATA_SYMBOL(..., name$S*)` binding is allowed
only when fresh COFF proves one real source static with that semantic prefix and the
numeric suffix is the sole ambiguity. The wildcard does not authorize anonymous
compiler-generated function labels.
