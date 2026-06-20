# gruntz — Claude working notes

Binary-matching decompilation of **Gruntz** (Monolith Productions, 1999), which
runs on the **WAP32** 2D engine (shared with *Claw* and *Get Medieval*). Goal:
C++ that, compiled with the original toolchain (**MSVC 5.0**), produces COFF
objects matching the retail `GRUNTZ.EXE`, verified with **objdiff**.

**Current stage: the matching loop runs.** `src/` holds the reconstructed C++
and is the single source of truth; the **`gruntz` CLI** (`python -m gruntz`,
`scripts/gruntz/cli.py`) drives everything
(`gruntz build` = compile base objs under wine `cl` → generate
`build/gen/symbol_names.csv` from `src/` `RVA()`/`DATA()` annotation macros
(`src/rva.h`, read from LLVM IR) → synth fake PDB → delink the retail EXE →
objdiff). ~57/100 functions byte-exact across 23 TUs.
`scripts/gruntz/` is THE package — ALL importable code: the pipeline
(`{build,ghidra,init}/`, path-invoked by ninja/the CLI), the match tooling
(`match/`: `status`, `fingerprints`, `verify_stubs`), and one-shot analysis
tools (`analysis/`, incl. the `fid/` matcher). Run the non-pipeline tools as
`python -m gruntz.<area>.<module>`; `scripts/` is on `PYTHONPATH` (set by the
nix shells + the `gruntz` wrapper). Nothing importable lives outside the package.

See **`docs/build-system.md`** (the build, the `gruntz` CLI, and how `src/` became
the single source of truth).

## Tools come from Nix

- `nix develop` (default) — works today, **no MSVC**: `vostok-delinker`,
  `objdiff`/`objdiff-cli`, `ghidra`, `llvm-pdbutil`, python/rg/file/xxd/jq.
  Enough for analysis + **target-side delink** + objdiff.
- `nix develop .#build` — adds the MSVC 5.0 toolchain under `wine` for the
  **base/recompile** side. The `gruntz-toolchain` tarball is packaged (fetched +
  pinned in `flake.nix`); run `gruntz init` once to build the local env (wine
  prefix, clangd DB, Ghidra DB) — a few minutes on a cold run, fast/idempotent
  after (see the build-speed note under Conventions).

`GRUNTZ_EXE` is exported pointing at the Internet-Archive-fetched binary.

## Target facts — MEASURED, do not re-derive

- `GRUNTZ.EXE` (`$GRUNTZ_EXE`, flake-fetched) — EN v1.0, 2,511,872 B, MD5 `81c7f648…`.
- Built with **MSVC 5.0** (PE optional-header linker **5.10**; Rich C/C++ module
  build **8034**, cvtres **1668**). **CRT + MFC statically linked.**
- **231 RTTI mangled class names**; `.reloc` **present**; **no debug directory /
  no PDB** (none ever shipped or leaked for any WAP32 game).
- Leaked source paths reveal the modular layout:
  `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,Gruntz}\` over shared `incs\`.
- Engine siblings for cross-diff: `CLAW.EXE` (v1.2) and `MEDIEVAL.EXE` (also
  flake-fetched). Same base classes
  (`CGameApp/CGameMgr/CGameWnd/CWapX/CUserLogic`) + same toolchain. **Anchor diff
  pair: GRUNTZ ↔ CLAW.** Claw/GM have `.reloc` **stripped** → only Gruntz is
  delinkable.

## The pipeline

```
GRUNTZ.EXE → Ghidra (auto-analyse + RTTI + FLIRT + leaked names)
           → fake PDB (synth_pdb.py: llvm-pdbutil yaml2pdb over the Ghidra exports + a DBI-header patch)
           → vostok-delinker → per-symbol COFF "target" objects
           → objdiff vs "base" objects compiled with MSVC 5.0 under wine
```

Gotchas baked in from reading the delinker source:
- It needs Public + per-module **Procedure** (length-carrying) + Data symbols;
  **no section contributions** (so `synth_pdb.py`'s output is structurally enough).

## Conventions

- Keep `README.md` and the relevant `docs/` (esp. `build-system.md`) current when
  the build/diff flow, tools, or paths change.
- **Win32/MFC types & functions come from the real headers, not hand-rolled decls.**
  Include `<Mfc.h>` (MFC TUs — pulls `<afx.h>` → `<windows.h>` the period-correct way) or
  `<Win32.h>` (pure-Win32/DirectX TUs); don't re-`typedef` `BOOL`/`HWND`/`INT_PTR`/… or
  re-`extern` `PostMessageA`/`timeGetTime`/… The real decls keep the signature/mangling from
  drifting, and bringing windows.h via the umbrellas is matching-neutral (PR #44). See
  `docs/patterns/win32-import-decl-stdcall.md`.
- `flake.lock` is committed; `.gitignore` already excludes generated outputs.
- **Builds are FAST — don't engineer around build time.** A full from-scratch
  `gruntz clean && gruntz init` (cold Ghidra import+analyze, wine re-init, full
  recompile, warmup) is ~2–3 min; back-to-back `clean → init` x2 is ~5 min;
  `gruntz build` (incremental) is faster. Just run them in the foreground and
  verify changes with a real build — don't background out of fear, avoid clean
  builds, or skip verification.
- **`src/Stub/` is the labeled-but-unmatched backlog** (the `engine_label_stubs`
  unit, aggregated by `All.cpp`). These stubs ARE delinked and diffed like any
  unit — they show in objdiff (initially ~0%) as the matching worklist, count in
  the started-units denominator, and are covered by the duplicate-RVA guard +
  `gruntz.match.verify_stubs`' stub-vs-matched cross-check. The goal is to **move
  each stub into its real class's TU** and reconstruct it there; `src/Stub/`
  shrinks toward empty. See `src/Stub/All.cpp`.
