# gruntz

Binary-matching decompilation of **Gruntz** (Monolith Productions, 1999) — a 2D
isometric puzzle game built on Monolith's **WAP32** engine (the same engine as
*Claw* (1997) and *Get Medieval* (1998)). The goal is C++ sources that, compiled
with the original **MSVC 5.0** toolchain, produce object files matching the
retail `GRUNTZ.EXE`, verified with [objdiff](https://github.com/encounter/objdiff).

## Repo layout

```
scripts/gruntz.py             the one CLI: build / ghidra-refresh / init / status …
scripts/gruntz/               pipeline package — build/ ghidra/ init/
scripts/analysis/             one-shot analysis/discovery tools
src/                          reconstructed C++ — the single source of truth
structure/                    engine-wide comprehension headers (shrink as matched)
config/units.toml             per-TU build manifest (configure.py -> build.ninja)
docs/                         build system, matching notes, the consolidation design
flake.nix                     Nix build environment (two dev shells)
CLAUDE.md                     working notes for Claude agents
build/           (gitignored) ALL generated/fetched state: objs, fake PDB, labels,
                              clangd db, wine prefix, Ghidra exports, report …
```

`GRUNTZ.EXE` (and the cross-diff `CLAW.EXE`/`MEDIEVAL.EXE`) are **fetched by the
flake** and exposed via `$GRUNTZ_EXE` — there is no tracked `binaries/` dir.

## Requirements

Linux (x86_64) with [Nix](https://nixos.org/download) and flakes enabled
(`experimental-features = nix-command flakes`).

## Quickstart

```sh
nix develop          # default shell — works today, no MSVC needed
```

Provides `vostok-delinker`, `objdiff` / `objdiff-cli`, `ghidra`, `llvm-pdbutil`,
and python/ripgrep/etc. `GRUNTZ_EXE` points at the retail binary (fetched from
the Internet Archive). This shell covers everything **except** the recompile
half: Ghidra analysis, the target-side delink, and objdiff inspection.

```sh
nix develop .#build  # adds MSVC 5.0 under Wine for the base/recompile side
```

The VC5 toolchain is packaged (fetched + pinned in `flake.nix`). Run `gruntz init`
once to build the local environment — Wine prefix, clangd DB, and the Ghidra DB
(import + analyze GRUNTZ.EXE → `functions.csv`/`symbols.csv`); heavy on first run,
idempotent after. Then `gruntz build` runs the loop (compile → labels → delink → objdiff).

## The pipeline

```
GRUNTZ.EXE → Ghidra (auto-analyse + RTTI + FLIRT + import leaked names)
           → fake PDB  (synth_pdb.py: llvm-pdbutil yaml2pdb over the Ghidra exports + a DBI-header patch)
           → vostok-delinker  → per-symbol COFF "target" objects
           → objdiff  (x86/COFF)  vs "base" objects compiled with MSVC 5.0 / Wine
           → iterate the C++ until each function matches
```
