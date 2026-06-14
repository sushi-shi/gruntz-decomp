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

### Wap32 / CGameApp (DONE — ctor + 4 methods; unit `gameapp`)
- Matched `??0CGameApp@@QAE@XZ` (ctor) + `?CloseResources@…XXZ` @0x13d8c0,
  `?InitializeAccelerators@…HPBD@Z` @0x13dc20, `?ReportError@…XIJ@Z` @0x13dcb0,
  `?InitializeDefaultWindowClass@…XXZ` @0x13d9b0 — all byte-exact (commit 7323fe5).
- Layout pinned (`src/Wap32/Wap32.h`): m_4/m_8 = `CGameResource*` (polymorphic,
  vtable slot0 = scalar-deleting dtor; m_4 also exposes HWND@+0x4 + guard@+0xc),
  m_c=HINSTANCE, m_10=HACCEL, m_18 flag byte (bit1=system arrow cursor),
  m_a0 cursor/icon name buf, m_160 class-name buf, `WNDCLASSA m_wc@+0x1e8`,
  m_244/m_248(one-shot guard)/m_24c/m_250 error fields.
- UNLOCKED: `GameWindowProc` @0x13cff0 (`?GameWindowProc@CGameApp@@SGJPAXIIJ@Z`,
  static __stdcall) now anchored in the TU; its body is in-scope.

## Matching idioms confirmed here (candidates for matching-patterns.md)
- Member inits emit in the **optimizer's schedule order**, not declaration order
  (e.g. CGameApp stores +0x10 before +0x0c) — mirror that order in the source.
- Names/namespaces are **placeholders**: only offsets and code bytes are
  load-bearing. Pick any mangling; make `symbol_names.csv`'s name equal exactly
  what `cl` emits for your source symbol (objdiff pairs base↔target by name).
  Read the real mangled name from the base obj: `llvm-objdump -t
  build/objdiff/base/<unit>.obj | grep <hint>`.
- vtable/global stores read ~99.5% *fuzzy* though byte-exact (REL32 vs DIR32 on a
  differently-named symbol) — confirm by reloc-masked byte-compare; not a real diff.
- **`delete pObj`** reproduces the scalar-deleting-dtor `mov eax,[ecx]; push 1;
  call [eax]` exactly — a class with one `virtual ~T()` puts `??_G…` at vtable
  slot 0; `delete` emits the inline null-check + `push 1; call [vptr]`. No manual
  vtable forging.
- **Win32 imports**: declare a minimal `__declspec(dllimport) … __stdcall` block
  (reproduces `FF15 [IAT]`); do NOT `#include <windows.h>` — keep the visible
  symbol SET small (the compiler hashes it; entropy follows header churn).
- **`rep stosd` of N dwords** = a zero-init of an N-dword struct (e.g. WNDCLASSA =
  10 dwords); reproduce with an N-iteration `int*` zero loop.

## Blocked / deferred
- (none yet — populate as matchers hit walls)
