# Match learnings — running log (campaign)

Per-function / per-subsystem insights gathered while byte-matching. Durable,
generalizable findings graduate into `docs/matching-patterns.md`; this file is the
fast-moving scratchpad. Newest at top within each section.

## How the loop runs (orchestrator + matcher contract)
- Matchers run **sequentially in the main worktree** (the `build/` tree —
  delinkable EXE, ghidra DB, toolchain — is gitignored, so git worktrees can't
  build; and `symbol_names.csv`/`units.toml`/the wine prefix/`build/` are shared).
- Each matcher = one TU = **its own atomic commit** (adds only its own files) +
  `git push`. After each, the orchestrator refreshes the comprehension DB
  (`build/ghidra-named`) when the matcher pinned new names/types, so the next
  matcher stands on prior work ("shoulders of giants"), then re-runs
  `scripts/gen_match_queue.py` to refill `docs/match-queue.md`.
- Per-function asm diff: `objdiff-cli diff -p build/objdiff -u <unit> <mangled-sym>
  -o - --format json-pretty`. Roll-up: `scripts/rebuild.py`.

## Subsystem notes
### zlib (DONE — 51 fns byte-exact)
- 1.0.4, statically linked, locked flags `/O2 /MT` (cdecl). The REZ entry payloads
  are raw deflate → `_inflate*`/`_uncompress` are the live decompressors.
- `WwdFile::InflateMainBlock @0x160790`: algorithm-exact but plateaus ~88.7% on an
  un-steerable MSVC5 register allocation — left per the entropy doctrine. First
  real-world confirmation that a high plateau with no source diff IS done.

### REZ / WWD asset load
- Pinned on-disk structs (clean-room from OpenClaw + binary): `WwdHeader 0x5F4`,
  `WwdPlaneHeader 0xA0`, `WwdObjectRecord 0x11C`, `PidHeader 0x20`
  (`structure/formats/`). `imageSet` is a length-prefixed string, then a `sound`
  string. RezMgr container offsets still @unconfirmed (blocks `CRezDir::Load`).

## Matching idioms confirmed here (candidates for matching-patterns.md)
- Member inits emit in the **optimizer's schedule order**, not declaration order
  (e.g. CGameApp stores +0x10 before +0x0c) — mirror that order in the source.
- Names/namespaces are **placeholders**: only offsets and code bytes are
  load-bearing. Pick any mangling; make `symbol_names.csv`'s name equal exactly
  what `cl` emits for your source symbol (objdiff pairs base↔target by name).
- vtable/global stores read ~99.5% *fuzzy* though byte-exact (REL32 vs DIR32 on a
  differently-named symbol) — confirm by reloc-masked byte-compare; not a real diff.

## Blocked / deferred
- (none yet — populate as matchers hit walls)
