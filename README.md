# gruntz

Binary-matching decompilation of **Gruntz** (Monolith Productions, 1999) — a 2D
isometric puzzle game built on Monolith's **WAP32** engine (the same engine as
*Claw* (1997) and *Get Medieval* (1998)). The goal is C++ sources that, compiled
with the original **MSVC 5.0** toolchain, produce object files matching the
retail `GRUNTZ.EXE`, verified with [objdiff](https://github.com/encounter/objdiff).

> **Status — investigation + scaffolding.** The reverse-engineering survey is
> done and the Nix build environment is wired. The matching loop itself is not
> running yet; the one missing piece is the packaged MSVC 5.0 toolchain (see
> [Toolchain](#toolchain-the-one-missing-piece)). The full analysis and plan
> live in **[`REVERSE_ENGINEERING_PLAN.md`](REVERSE_ENGINEERING_PLAN.md)**.

## Why this is tractable

- **Shared engine.** Gruntz, Claw, and Get Medieval are the same WAP32 codebase
  (confirmed at the binary level: identical `CGameApp/CGameMgr/CGameWnd/CWapX`
  base classes, the same `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr}` source tree,
  the same MSVC 5.0 toolchain). Existing RE — OpenClaw / libwap32 for the asset
  formats — transfers directly.
- **A friendly target.** `GRUNTZ.EXE` retains **231 RTTI class names** (the whole
  object model), leaks original source-file paths, statically links the CRT + MFC
  (large FLIRT surface), and — crucially — **keeps its `.reloc` table**, which is
  what makes delinking possible (Claw/Get Medieval stripped theirs).
- **A proven harness to copy.** The pipeline mirrors `srp-survarium/vostok`
  (a working X-Ray 2.0 matching decomp); its delinker and flake mechanism are
  reused here.

No Gruntz PDB ever shipped or leaked, so we synthesise one (Ghidra + pdbgen).

## Repo layout

```
REVERSE_ENGINEERING_PLAN.md   the full survey + ranked plan (source of truth)
flake.nix                     Nix build environment (two dev shells)
CLAUDE.md                     working notes for Claude agents
binaries/        (gitignored) GRUNTZ.EXE, CLAW.EXE, MEDIEVAL.EXE, v1.01 patch …
refs/            (gitignored) vostok (private reference harness), tomalla-gruntz
```

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

This needs the `gruntz-toolchain` tarball, which is **not packaged yet**
(`flake.nix` has a `lib.fakeHash` placeholder).

## The pipeline

```
GRUNTZ.EXE → Ghidra (auto-analyse + RTTI + FLIRT + import leaked names)
           → fake PDB  (wandel/pdbgen; a CodeView entry is patched into the PE)
           → vostok-delinker  → per-symbol COFF "target" objects
           → objdiff  (x86/COFF)  vs "base" objects compiled with MSVC 5.0 / Wine
           → iterate the C++ until each function matches
```

objdiff supports x86 + COFF — exactly our target. Engine functions are
cross-validated 3-way against `CLAW.EXE`/`MEDIEVAL.EXE` and against OpenClaw's
semantics. Full rationale, the delinker's exact requirements, and the
prerequisites are in [`REVERSE_ENGINEERING_PLAN.md`](REVERSE_ENGINEERING_PLAN.md)
§14–15.

## Toolchain (the one missing piece)

Gruntz was built with **MSVC 5.0** (PE linker 5.10; Rich C/C++ build 8034). The
plan reuses vostok's "package the original compiler once, fetch it as a release
tarball, run it under Wine" mechanism — but with the older VC++ 5.0 payload
(extracted from InstallShield/CAB media with `7z`, plus a DirectX 6 SDK and the
static-MFC / Miles / Smacker libs Gruntz links). Build it once, publish it, and
fill the `gruntz-toolchain` `url` + `sha256` in `flake.nix`
(`nix build .#gruntz-toolchain` prints the real hash). Recipe: plan §15.

## Provenance & scope

This is a preservation / interoperability decompilation, in the spirit of other
classic-game decomp projects. The retail binaries are fetched from the Internet
Archive for analysis and are **not committed** (`.gitignore`). `refs/vostok` is a
**private** reference repo and is likewise excluded.
