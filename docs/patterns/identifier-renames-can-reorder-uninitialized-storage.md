# Identifier renames can reorder VC5 uninitialized storage

tags: cpp:static cpp:member | asm:coff | topic:tooling topic:evidence-discipline
symptoms: a declaration rename preserves instruction bytes and relocation targets but changes symbol offsets or the size of an uninitialized section
confidence: 9/10

## Controlled observation

The pre-rebase data-prefix control started from working snapshot `eb3cb9544`
and changed 323 data identifiers
using AST declarations and symbol-scoped references. The changes preserve
storage classes, types, initializers, declaration order and function statements.
Macro-body references required separate updates, including Blowfish's pasted
`byte##i` identifier. The three header-inline random-state COMMON pins were
renamed with their owning local statics.

A full pinned `gruntz build` passed. Comparing all 293 base COFF objects while
mapping only the explicitly renamed data symbols found identical non-debug
payload bytes and identical relocation sites, types and resolved symbol names.
288 objects also retained their complete section layout. Five `.bss` sections
changed member placement:

| Unit | Size before | Size after |
| --- | ---: | ---: |
| bootystateactivate | 4752 | 4752 |
| butemgr | 96 | 98 |
| cimage | 348 | 356 |
| debugprintf | 156 | 160 |
| wwdobjmgr | 45 | 45 |

For each of these sections, the complete symbol-name/storage-class multiset
remained identical after the explicit rename mapping. Only member offsets and,
in three cases, the section extent changed. For example, the scanner's two-byte
local static moved from offset 48 to offset 96 in butemgr; the two CObArray
local statics in wwdobjmgr exchanged offsets 0 and 24. These are section-layout
changes, not changes to either object's C++ member layout.

This batch establishes that identifier-only edits can change VC5's placement
of uninitialized storage. It does not isolate a particular identifier or prove
a hash algorithm, compiler pass, or general prediction of the resulting order.
Do not attribute unrelated instruction scheduling to this observation. The
cleanup was subsequently replayed onto remote main; this table describes the
controlled rename batch, not arbitrary differences between those two bases.

## Reverse audit

Save the actual base objects before renaming and compare them after the full
build. Exclude debug sections, preserve payload bytes (including relocation
addends), and compare typed relocation sites and symbol identities. Preserve
compiler ordinal suffixes rather than stripping them. Qualify member names by
owner: renaming a reconstructed dialog's message map must not rename the SDK's
`CDialog::messageMap`. Distinguish TU-local constants from external globals with
the same spelling.

When only `.bss` placement differs, compare the entire member multiset and
verify that source storage, types and initialization stayed unchanged. Report
the changed layout explicitly; do not call the objects byte-identical or ignore
arbitrary data differences. A green relocation/data-access audit is still
required. Retain semantic names instead of steering storage placement with
padding or inert declarations.

A successful clangd rename is not a complete reference audit: macro bodies and
token-pasted member names can be omitted. Reparse all translation units with
libclang, including unused template bodies, and run the real compiler before
accepting the rename.
