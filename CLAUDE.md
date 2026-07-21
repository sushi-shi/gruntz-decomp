# gruntz — Claude working notes

Binary-matching decompilation of **Gruntz** 

Goal: C++ that, compiled with the original toolchain (**MSVC 5.0**), produces COFF
objects matching the retail `GRUNTZ.EXE`, verified with **objdiff**.

`src/` holds the reconstructed C++ and is **the single source of truth**; the **`gruntz` CLI**
(`python -m gruntz`, `scripts/gruntz/cli.py`) drives everything. For the current score, run
`gruntz status` — never trust a number written down here.

`scripts/gruntz/` is THE package — ALL importable code: the pipeline
(`{build,ghidra,init}/`, path-invoked by ninja/the CLI), the match tooling
(`match/`: `status`, `fingerprints`, `verify_stubs`), the cleanliness board +
quality gates (`cleanliness/`), the `gruntz sema`
navigation surface (`sema/`, one module per subcommand), and one-shot analysis
tools (`analysis/`, incl. the `fid/` matcher). Run the non-pipeline tools as
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
- **Addresses are zero-padded to 8 hex digits** in every `RVA()`/`DATA()` macro and
  in `config/match-queue.md` (`0x00xxxxxx`); the RVA size arg stays unpadded.
- **Formatting is automated; don't hand-format.** Rust-like clang-format (root
  `.clang-format`) via a pre-commit hook + `gruntz format`; whitespace-only, so
  matching-neutral. **Never format `vendor/`.** Details: `docs/build-system.md`.
- **Builds are FAST — don't engineer around build time.** A full from-scratch
  `gruntz clean && gruntz init` is a few minutes; `gruntz build` (incremental) is faster.
  Run them in the foreground and verify changes with a real build — don't background out of
  fear or skip verification. `--fast` skips the gate tail; run one full build before a commit.
- **Every body lives in its real owner TU** — owner proven by xref / vtable-slot, never by RVA
  proximity (`docs/tu-partition-brief.md`; a contribution must be contiguous).
- **Game semantics** (what WWD fields/ids/logic MEAN): `docs/domain/` (distilled) over
  `docs/reference/gooroosgruntz/` (mirrored community docs); the +0x114 union is
  Score/Points/Powerup/Damage/Smarts/Health.
- **Cleanliness endgame + cast policy** (`docs/cast-metric-policy.md`): drive EVERY
  `config/cleanliness-baseline.tsv` metric to 0. Mis-model casts (views, `)this`) are ELIMINATED by
  real typing; a genuinely-needed cast uses a **C++ named cast** (`static_cast` for math/numeric,
  `reinterpret_cast`/`const_cast`/`dynamic_cast` otherwise) so the C-style-pattern metrics slide to 0;
  **offset-casts `(char*)x + N` are BANNED outright** (named member `&x->m_field`, never even a C++
  cast). `m_<hex>` naming is last.
- **Function-state markers (comments, ignored by tooling):** `// @stub` = an empty, not-yet-
  reconstructed body; `// @early-stop` (reason on the next line) = a complete reconstruction
  parked below 100% match; `// @identity-TODO` = an unproven class/owner identity — leave it,
  never fabricate. A reconstructed method is either ~100% (unmarked) or `@early-stop`; the
  final-sweep worklist is `rg '@early-stop' src`.
