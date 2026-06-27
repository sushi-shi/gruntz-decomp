# HANDOFF — reconstructing the `engine_unmatched` worklist

This is the standing brief for a matcher (human or agent) picking up the
`engine_unmatched` backlog. It carries the **doctrine** (so you don't bail or
defer), the **per-function workflow**, the **codegen idioms** that close matches,
and points at the **worklist itself**.

## The two files (the split)

`src/Stub/` splits the leftover unmatched functions into:

- **`Unmatched.cpp`** — the **WORKLIST**: real game/engine function bodies that
  still need reconstructing. **This is your TODO list.** Each `RVA(rva,size) void
  Unmatched_<rva>() {}` is one target; the trailing comment is the Ghidra name hint.
- **`UnmatchedLibrary.cpp`** — **DO NOT TOUCH.** CRT / MFC / zlib / compiler
  *implementations* (zlib `deflate`/`trees`, CRT math funclets, iostream
  `ipfx`/`opfx`, debug-heap, SEH `__getptd`, `$E` atexit thunks, `AfxWndProc`).
  These are matched by *using* the statically-linked library or by FID-tagging —
  never hand-written. They're split out so you don't waste effort on them.

A function that merely *calls* `CString`/`CObject`/`operator new` is **game code**
and stays in the worklist — only true library *implementations* live in the lib file.

## HARD RULES (so you don't bail)

A prior analysis confirmed **every** worklist entry is structurally reconstructable.

1. **Do NOT defer, skip, or leave a function as a bare stub.** Reconstruct it.
2. **Push every function to 100%.** If it plateaus, that is almost always a fixable
   codegen-shape bug in *your source*, not a wall — iterate different spellings.
3. **The ONLY acceptable non-100% is a maximized `@early-stop`:** a COMPLETE correct
   reconstruction (full body, all logic) where you have PROVEN with
   `llvm-objdump -dr` (base obj vs target obj) that the *code bytes* are byte-exact
   and the residual is a genuine codegen/delinker artifact — with the byte-level
   reason written in an `// @early-stop` comment. Never a partial that under-counts
   because you stopped early.
4. **Size is not a reason to defer.** Reconstruct large bodies leaf-first, in full.
5. **You are ONE worker. NEVER spawn subagents.** Do fewer functions if budget is
   tight and report the rest as not-done — do not delegate.

## Per-function workflow

All commands run from the worktree; build inside one `nix develop .#build` shell.

1. **Pull the target:** `python -m gruntz.analysis.dump_target 0xNNNN` (disasm +
   relocs — the reloc names give you the externs/globals/strings). Use the Ghidra
   decomp + `gruntz.analysis.*` tools (`clangd_query`, `extern_harvest`,
   `string_xref`, `this_cluster`, `tu_layout`).
2. **Find the home TU.** Most belong to an existing `src/.../*.cpp`. If a function
   is its own contiguous cluster with no home, CREATE a new TU: add an `[[unit]]`
   block to `config/units.toml` (`flags="base"`, or `"eh"` if it has a `/GX` EH
   frame — `push -1 / push <handler> / mov fs:0,esp`), then create the `.cpp`.
   `configure.py` auto-discovers it; `symbol_names.csv` is generated from your
   `RVA()`/`DATA()` macros — don't hand-edit it.
3. **Reconstruct** the types (model real structs from the offsets; each extern
   callee/global modeled with NO body so its `call`/`DIR32` reloc-masks; real
   Win32/MFC via `<Win32.h>`/`<Mfc.h>`; `#include <string.h>` enables inline
   `strlen`→`repne scasb` / `memset`→`rep stos` / `memcpy`→`rep movs`). Put
   `RVA(0x........, 0xNN)` (zero-padded 8 hex) above each, ascending-RVA order.
4. **Remove the matched RVA from `Unmatched.cpp`** (so the duplicate-RVA guard
   passes) — delete its `RVA()` line + the following stub line.
5. **Build + verify:** `gruntz build`, then `objdiff-cli report generate -p
   build/objdiff -o build/objdiff/report.json` and read the per-function
   `fuzzy_match_percent` from the report.
6. **Diagnose a residual** with `llvm-objdump -dr build/objdiff/base/<unit>.obj` vs
   `.../target/<unit>.c.obj`, instruction by instruction. **GREP
   `docs/patterns/INDEX.md` by symptom FIRST** (`cpp:switch`, `cpp:eh`, `asm:neg`,
   `topic:wall`) — most MSVC5 /O2 idioms are cataloged with a steerable source
   spelling. Document a genuinely-new idiom (a `docs/patterns/<name>.md` + one
   INDEX line) as part of finishing the match.
7. **Commit incrementally** (every function or tight cluster) so progress isn't lost.
   Do NOT run `gruntz format` (it reformats the whole tree). End commit messages with
   the `Co-Authored-By` trailer.

## Codegen idioms that close matches (learned the hard way)

- **Bool tail.** `return Fn() != 0;` lowers to `neg/sbb/neg`. If the target instead
  BRANCHES to a shared `return 1`, the source is the branch form. For a multi-case
  switch where each case aborts on failure and shares one success:
  `case k: if (Fnk()) break; return 0; ... } return 1;` (every case's success jumps
  to one `mov eax,1; ret`). This took a 5-way dispatcher 42%→79%.
- **Struct-return vs out-param.** A method doing `mov eax,[esp+4]; …stores…; ret 4`
  that returns the slot is an explicit out-param fill `T* Fill(T* out){ …; return
  out; }`, not necessarily a by-value return (avoids the 8-byte EDX:EAX ambiguity).
- **Callbacks / fn-ptrs passed as args:** model the callee as `extern <ret>
  __cdecl Name(...);` (no body) and pass `Name`/`&Name` — NEVER `(T*)0xADDR` (a bare
  immediate carries no relocation and caps below 100%). The push/DIR32 reloc-masks.
- **`if (w==A && h==B) return X;` chains** match the target's `cmp;jne;cmp;jne;mov
  eax,X` directly; MSVC speculates the return value before the last `je`.
- **Leaf CUserLogic dtor archetype** = `mov [this],0x5e705c; <destruct +0x18 via
  ~EngStr 0x16d2a0>; mov [this],0x5e70b4` under a `/GX` frame → `class X : public
  CUserLogic { ~X(); };  X::~X(){}` (flags `"eh"`). `<Gruntz/UserLogic.h>` has it.
- **EH ctors/dtors map the layout:** each `lea ecx,[this+N]; call <ctor/dtor>` is a
  constructible/destructible member of that type at `+N`, in (reverse-for-dtor)
  declaration order. Model those as REAL value members so MSVC reconstructs the same
  `/GX` frame + EH-state transitions — that's what moves an EH function from ~41%
  (frame missing) to 80–98%. See the `eh-dtor-*` patterns.
- **Dense `switch` jump-table:** code can be byte-exact yet plateau ~79% because the
  delinker inlines the table into the fn symbol while MSVC emits a separate `$Lnnn`
  symbol — a `topic:wall` artifact (`switch-jumptable-separate-comdat.md`), not a
  source bug; confirm with `llvm-objdump` and `@early-stop`.

Names are placeholders — only **offsets + code bytes** are load-bearing. If a
function's owning class is genuinely unidentifiable, use a clearly-marked placeholder
class in a small TU, but model the REAL layout from the offsets.

## Status snapshot at handoff

`engine_unmatched` (this worklist) = **176 reconstruction targets**; the **15**
CRT/MFC/zlib library implementations are split into `UnmatchedLibrary.cpp` (skip).
Roughly: ~110 small/small-medium bodies (≤0x100), ~14 medium (0x100–0x300), ~10
large (>0x300, leaf-first/final-sweep), plus ~11 trivial leaves (ret-const type-tags,
setvtbl ctors, singleton tail-jmps). `Boundary.cpp` / `ApiCallers.cpp` / `Backlog.cpp`
are *separate, larger* stub backlogs beyond this worklist.
