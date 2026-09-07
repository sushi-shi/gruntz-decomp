# Declaration name audit

Run the AST census in the pinned environment:

```sh
nix develop -c python3 scripts/audit-declaration-names.py --tracked-only
```

The script parses the compilation database with libclang, then parses otherwise
uncovered owned headers with the Grunt.cpp flags. It includes unused template
bodies by disabling delayed template parsing. SDK and vendor declarations are
excluded. Omit `--tracked-only` to include untracked source and headers too.

Outputs under `build/audits/declaration-names/`:

- `declarations.tsv`: declarations with type, scope, storage class, linkage,
  file/line/column and USR. Prototypes and definitions remain separate.
- `all.unique.txt` and per-category `*.unique.txt`: sorted unique spellings for
  fields, globals, arguments, locals, functions and static members. `static`
  collects static data and explicitly static function declarations.
- `prefix-violations.tsv`: named data declarations violating the conventions.
- `summary.json`: counts, parse errors, supplemental headers and coverage gaps.

Instance fields use `m_`; external globals use `g_`; static data uses `s_`.
The name after the prefix starts with a lowercase letter, including constants.
The static-data rule includes class and function-local statics and file-scope
constants with internal linkage. Function names have no required prefix.
The process exits nonzero for parse errors or prefix violations. Unique names
alone do not prove semantic quality: numbered geometry components, external
API spellings and unknown modeled storage still require contextual review.

The census covers the active preprocessor configuration. Unnamed arguments
remain in the declaration dump but do not appear in the unique-name lists.
The dump records source declarations, not the original game's lost debug names.
Do not rename SDK members or literals through a text-wide replacement of a
common identifier; use symbol identities and verify macro bodies separately.
Compiler-ignored annotation arguments are not AST references: update the owner
spelling in `RVA_DYNINIT` and any explicit COMMON-symbol pins when renaming data.

A rename must pass the full pinned build. Compare actual COFF objects while
preserving payloads and ordered typed referents; storage-name changes can alter
[uninitialized section placement](patterns/identifier-renames-can-reorder-uninitialized-storage.md).

The prefix rule is project spelling policy, not evidence that the lost original
source used these prefixes. Existing names with numeric suffixes are not all
synthetic: line endpoints, consecutive polygon vertices, RGB components,
interface versions and repeated buffer regions have legitimate numeric domains.

The review retains uncertain names rather than inventing identities. Examples
include the unnamed storage spans in the `MotionEntity` identity-TODO model,
reserved fields in partially recovered layouts, `CGrunt::m_daFlag`, and the
`r0`–`r3` arguments of the declaration-only `CStatusBarMgr::ConfigureRect` API.
No implementation or caller currently establishes that API's parameter roles.
These require source/type recovery, not a prefix substitution. `VirtualFoo` in
the Lith list family is a surviving-source spelling and is not a compiler label.
