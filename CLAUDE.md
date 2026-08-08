# gruntz — Claude working notes

Binary-matching decompilation of **Gruntz** 

Goal: C++ that, compiled with the original toolchain (**MSVC 5.0**), produces COFF
objects matching the retail `GRUNTZ.EXE`, verified with **objdiff**.

`src/` holds the reconstructed C++ and is **the single source of truth**; the **`gruntz` CLI**
(`python -m gruntz`, `scripts/gruntz/cli.py`) drives everything. For the current score, run
`gruntz status` — never trust a number written down here.

`scripts/gruntz/` is THE package — ALL importable code, one package per role:
the pipeline (`{build,ghidra,init}/`, path-invoked by ninja/the CLI), the
shared engine library (`core/`: pe/symbols/report/vtables/exe_map/clangd),
match scoring + integrity gates (`match/`), the cleanliness board + quality
gates (`cleanliness/`), the permuter climbers (`permute/`), the `gruntz sema`
navigation surface (`sema/`, one module per subcommand), and one-shot campaign
audits (`audit/`, incl. the `fid/` matcher). Run the non-pipeline tools as
`python -m gruntz.<area>.<module>`; `scripts/` is on `PYTHONPATH` (set by the
nix shells + the `gruntz` wrapper). Nothing importable lives outside the package.

See **`docs/build-system.md`** (the build, the `gruntz` CLI, and how `src/` became
the single source of truth) and **`docs/gotchas.md`** (measurement/build/matching
traps + the mislabeled-bug audit playbook + which cleanliness tooling is live).

## Tools come from Nix

- **One shell for everything:** `nix develop` — analysis (`vostok-delinker`,
  `objdiff`/`objdiff-cli`, `ghidra`, `llvm-pdbutil`, python/rg/file/xxd/jq) **and** the
  MSVC 5.0 toolchain under `wine` for the base/recompile side. The `gruntz-toolchain` tarball is packaged
  (fetched + pinned in `flake.nix`); `gruntz init` (auto-run on shell entry) builds the
  local env — wine prefix, clangd DB, Ghidra DB — a few minutes cold, fast/idempotent
  after (see the build-speed note under Conventions).

`GRUNTZ_EXE` is exported pointing at the Internet-Archive-fetched binary.

## Target facts

- `GRUNTZ.EXE` (`$GRUNTZ_EXE`, flake-fetched) — EN v1.0, 2,511,872 B, MD5 `81c7f648…`.
- Built with **MSVC 5.0**; **CRT + MFC statically linked**.
- `.reloc` **present** → the EXE is delinkable. **No PDB** → `synth_pdb.py` fakes one, and
  contribution ranges must be *recovered*, not read (`docs/tu-partition-brief.md`).
- Leaked source paths give retail's compiland layout:
  `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,Gruntz}\` over shared `incs\`.

## The pipeline

**One-time, cached (`gruntz init`):** GRUNTZ.EXE → Ghidra (import + auto-analyse + RTTI +
FLIRT + leaked names) → exports. Not part of the build loop.

**Every `gruntz build`** — `src/` drives both sides; they meet at objdiff:

1. **compile** — `src/` → base objs (`cl /O2 /MT` under wine).
2. **labels** — `RVA()`/`DATA()` annotations (read from LLVM IR) ∩ base objs → per-TU
   fragments → merge → `build/gen/symbol_names.csv`. **The hinge**: the delink re-fires on it.
3. **synth PDB** — `symbol_names.csv` + Ghidra exports → fake PDB (`synth_pdb.py`).
4. **delink** — GRUNTZ.EXE + fake PDB (+ data/section manifests) → per-unit *target* objs
   (`vostok-delinker`).
5. **normalize** — base + target → content-addressed comparison copies (objdiff pairs by name).
6. **objdiff** — normalized base vs target → `report.json`, then the gates.

**Opt-in (`ninja candidate`):** link → candidate `.EXE` + `.map` — the layout/contribution audits
(`gruntz link`, `exe-diff`). See `docs/data-attribution.md` + `docs/tu-partition-brief.md`.

## Conventions

- Keep `README.md` and the relevant `docs/` (esp. `build-system.md`) current when
  the build/diff flow, tools, or paths change.
- **Win32/MFC types & functions come from the real headers** (`<Mfc.h>` for MFC TUs,
  `<Win32.h>` for pure-Win32/DirectX) — don't hand-roll typedefs/externs. See
  `docs/patterns/win32-import-decl-stdcall.md`.
- **Label macros have ONE canonical spelling** (gated FATAL, `gruntz.audit.label_style`):
  addresses zero-padded to 8 hex digits (`0x00xxxxxx`), size args unpadded lowercase hex
  (`0x0` = unknown), one line per invocation. No label ever lives in a comment
  (`RVA_COMPGEN` is the compiler-generated pin). There is no data *macro* analog:
  `DATA_SYMBOL` is RETIRED and removed from `rva.h` — every datum is a real C++
  definition carrying `DATA(rva)`, so reintroducing it is a compile error. The
  DATA analog of `RVA_COMPGEN` is a **manifest**,
  `config/retail/compiler-generated-data.tsv` (gated FATAL,
  `gruntz.audit.compgen_data`): a datum cl emits as a COFF **COMMON** from a
  header-inline's local static — plus its `??_B` guard byte, which has no source
  spelling at all — has no owning TU to host a source pin, so only its retail
  ADDRESS is stated and every other column is re-proven against the base objs.
  Details: `docs/build-system.md` § "Compiler-generated DATA pins".
- **Formatting is automated; don't hand-format.** Rust-like clang-format (root
  `.clang-format`) via a pre-commit hook + `gruntz format`; whitespace-only, so
  matching-neutral. **Never format `vendor/`.** Details: `docs/build-system.md`.
- **The `#include` block is canonical** (gated, `gruntz.audit.include_order`): no
  duplicates, every header self-sufficient (standalone-compile-proven), and one
  order everywhere — config `#define`s, `<rva.h>`, the TU's own header, platform
  preludes (`<Mfc.h>`/`<MfcNoInline.h>`/`<MfcWin.h>`/`<Win32.h>`, dependency-ranked),
  project headers sorted, libraries sorted. Never hand-order:
  `python -m gruntz.audit.include_order --fix-dupes --fix`. No TU includes an
  `<afxwin.h>` directly — the `_AFX_ENABLE_INLINES` devices live in the two Mfc*
  wrappers. `<Mfc.h>` is a superset of `<Win32.h>`; including both trips MFC's
  C1189. Rules + traps: `docs/patterns/include-order.md`.
- **MAX match is the metric — never revert on a current-% dip.** A byte-evidenced change
  (shape seen in the target disasm) is KEPT even if its fn's current-% stalls, a sibling
  craters, or Overall drops. Revert only when the change's OWN evidence fails or the build
  breaks. Gate on BUILD, not %. **A sibling drop is NOT a problem and never a reason to
  stop** — the `permute` skill (forests × islands, banked by MAX) is what recovers those.
  The ledger (`config/match_baseline.tsv`) has TWO peaks per function, with opposite jobs:
  - **`best_pct` is scoped to the IMPLEMENTATION** (the per-function `src_hash`, a clangd
    extent — NOT the whole `.cpp`). Same hash + a different % ⇒ the variance is TU
    composition, so **bank the max**. A CHANGED hash ⇒ **`best` resets to `cur`**, because
    the old peak was scored by source that no longer exists. **`best` == 100 means ALREADY
    FIXED — park it.** So bank the MAX *before* rewriting if the old peak is worth keeping.
  - **`hist_pct` never resets** — the all-time peak. **`hist` > `best` means KNOWN
    HEADROOM**: a better implementation existed and was lost. That is the worklist
    (`gruntz.audit.max_divergence --history`).
  - Reaching 100 with the function's own source unchanged **proves that source correct** —
    the residue is TU state. Perturb the TU, `status update`, then revert the perturbation:
    the proof is banked and nobody need look at the function again. Never leave the
    perturbation in the tree; an unused include kept to steer regalloc is a fitted artifact.
- **Builds are FAST — don't engineer around build time.** A full from-scratch
  `gruntz clean && gruntz init` is a few minutes; `gruntz build` (incremental) is faster.
  Run them in the foreground and verify changes with a real build — don't background out of
  fear or skip verification. `--fast` skips the gate tail; run one full build before a commit.
- **Read the target with `gruntz sema disasm`, and START with `--blocks`.**
  `--blocks --diff --lite` gives the basic-block topology (in-edges, branch arrows,
  loop back-edges, shared ret tails) — getting the CONTROL-FLOW STRUCTURE right is
  what makes a reconstruction match. Rebuilding the shape by hand from jump targets
  is wasted effort. `--rich` once the shape is right and you're chasing which
  *statement* produced which instructions. **Trap:** `--diff` and `--blocks --diff`
  MASK address operands, so a pure control-flow divergence prints "identical" while
  the function is <100% — then use **`--branches --diff`**, which names each target by
  branch index (`docs/patterns/masked-diff-hides-branch-target.md`). The Ghidra
  decompiler is banned; assembly only.
- **Numeric domains are named types, not `i32`** (gated, `gruntz.audit.enum_domains`):
  one domain per meaning, declared once through the `GZ_ENUM_*` layer in
  `<Enums.h>`, `SCREAMING_SNAKE` enumerators with a domain prefix and an explicit
  `= value`. MSVC 5.0 has no `enum class` and sizes every enum as 4 bytes, so the
  layer expands two ways: real `enum`s for the matching build, `enum class` +
  `GzEnumStorage<N,S>` for a C++20 clang type-check (`gruntz.audit.strict_enums`).
  Retyping a member/param/return `i32` -> enum is BYTE-NEUTRAL (measured); only a
  signature's mangling moves. Use `GZ_ENUM_FORWARD(N)` instead of a new `#include`
  to type a header without the regalloc butterfly. Details:
  `docs/enum-modeling-plan.md`, `docs/patterns/enum-domains.md`.
- **Every body lives in its real owner TU** — owner proven by xref / vtable-slot, never by RVA
  proximity (`docs/tu-partition-brief.md`; a contribution must be contiguous).
- **Game semantics** (what WWD fields/ids/logic MEAN): `docs/domain/` (distilled) over
  `docs/reference/gooroosgruntz/` (mirrored community docs); the +0x114 union is
  Score/Points/Powerup/Damage/Smarts/Health.
- **On-disk formats** (what the shipped files ARE): `docs/formats/` — derived from the
  archived bytes + retail's own reader disassembly, never from `src/`. `tools/gruntz-rez`
  reads AND writes the REZ v1 container; `src/Rez/` is the file-driver layer, not the
  container, so it is not an authority on it.
- **Cleanliness endgame + cast policy** (`docs/cast-metric-policy.md`): drive EVERY
  metric in `config/cleanliness/cleanliness-{text,semantic}-baseline.tsv` to 0. Mis-model casts (views, `)this`) are ELIMINATED by
  real typing; a genuinely-needed cast uses a **C++ named cast** (`static_cast` for math/numeric,
  `reinterpret_cast`/`const_cast`/`dynamic_cast` otherwise) so the C-style-pattern metrics slide to 0;
  **offset-casts `(char*)x + N` are BANNED outright** (named member `&x->m_field`, never even a C++
  cast). `m_<hex>` naming is last.
- **Function-state markers (comments):** `// @stub` = an empty, not-yet-
  reconstructed body; `// @early-stop` (reason on the next line) = a complete reconstruction
  parked below 100% match; `// @identity-TODO` = an unproven class/owner identity — leave it,
  never fabricate. A reconstructed method is either ~100% (unmarked) or `@early-stop`; the
  final-sweep worklist is `rg '@early-stop' src`. The full (closed, gated) marker
  vocabulary: `docs/comment-markers.md`.
