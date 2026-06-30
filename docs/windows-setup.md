# Native Windows setup (`bootstrap_windows.py`) — the flake.nix port

`flake.nix` provisions the dev environment on Linux and runs MSVC 5.0 **under
wine**. On Windows the same MSVC 5.0 runs **natively**, so wine disappears
entirely. `bootstrap_windows.py` is the Windows counterpart of the flake: it
downloads the same pinned toolchain, writes activate scripts that export the same
environment (minus the wine vars), and the ordinary `gruntz <cmd>` workflow then
runs unchanged — `cl.exe`/`link.exe` are launched directly.

This is a **straight port**, not a fork: the artifact versions track the pinned
nixpkgs (`64c08a7c`) and the flake's own pins, and the SHA256s were cross-checked
against the flake (the VC5 toolchain hash is byte-identical). The Linux/wine path
is untouched — every Windows code path is guarded by `os.name == "nt"`.

## Prerequisites

- **Windows x64** (MSVC 5.0 is a 32-bit app; it runs under WOW64).
- **Python 3.11+** on `PATH` (`py --version`). Only the standard library is used
  to bootstrap, so a clean interpreter is enough.
- **git** on `PATH`.
- **Rust nightly (rustup)** — *only* to build `vostok-delinker` (the delink half).
  Everything else is a pinned binary download. Install from <https://rustup.rs>;
  the build uses a `nightly` toolchain (the source needs `#![feature(...)]`),
  exactly as the flake does.
- Recommended: enable **Win32 long paths** (Ghidra/LLVM extract to deep trees).
  `reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /v LongPathsEnabled
  /t REG_DWORD /d 1 /f` (the provisioner also `\\?\`-prefixes extraction roots as
  a fallback).
- Prefer a checkout path **without spaces** (MSVC 5.0 is old and finicky).

## Quick start

```bat
py bootstrap_windows.py                 :: download + verify + unpack, write activate scripts
. .\build\win-toolchain\activate.ps1    :: PowerShell — enter the dev environment
                                        :: (cmd.exe:  build\win-toolchain\activate.bat)
gruntz init                             :: one-time: Ghidra DB + clangd DB (several minutes)
gruntz build                            :: compile (native cl) + delink + objdiff
```

Or do it in one shot (provision, then run `gruntz init` in the computed env):

```bat
py bootstrap_windows.py --init
```

Useful flags:

| flag | effect |
|---|---|
| `--no-gui` | skip the objdiff **GUI** binary (keep `objdiff-cli`, which the build uses) |
| `--shell` | provision, then drop into an activated PowerShell (`--cmd` for cmd.exe) |
| `--force` | re-download and re-extract everything |
| `--only KEY …` | provision just these artifacts (e.g. `--only vostok_src` after installing Rust) |
| `--print-hashes` | print the pinned manifest and exit |
| `--skip-provision` | only (re)write the activate scripts |

## What it provisions

Hash-pinned into `build\win-toolchain\` (git-ignored, like everything under
`build/`). One artifact per flake package, in its Windows form:

| key | what | version | source |
|---|---|---|---|
| `toolchain` | MSVC 5.0 SP3 + DX6 SDK + `ninja.exe` | VC5 SP3 | the project's `gruntz-toolchain-vc50` release (same tarball as the flake) |
| `gruntz_exe` | retail `GRUNTZ.EXE` (the decomp target) | EN v1.0 | archive.org |
| `mss32`, `smackw32` | runtime DLLs the rebuilt EXE loads | — | archive.org |
| `objdiff`, `objdiff_cli` | the diff tool (GUI + CLI) | 3.7.1 | GitHub release (Windows builds) |
| `llvm` | clang, clangd, clang-format/-tidy, llvm-pdbutil/-nm/-undname | 21.1.8 | LLVM `…-pc-windows-msvc` release |
| `ghidra` | Ghidra (comprehension DB + RTTI/FID) | 12.0.4 | NSA GitHub release |
| `jdk` | Temurin JDK (Ghidra's runtime) | 21 GA | Adoptium |
| `ninja`, `ripgrep`, `jq` | build driver + search + JSON | 1.13.2 / 15.1.0 / 1.8.1 | GitHub releases |
| `vostok_src` | the delinker **source** (built with `cargo +nightly`) | pinned commit | forked GitHub branch |

`sfman32` stays a placeholder (no genuine Miles DLL located), exactly as in
`flake.nix`; it is `LoadLibrary`'d at runtime, so its absence only disables
SoundFont music.

Ghidra is pinned to **12.0.4** to match nixpkgs — the tracked
`config/library_labels.csv` is calibrated to 12.x's function carving (see the
"Ghidra 12 carves fewer functions" note). The JDK patch level is irrelevant: it
is Ghidra's runtime, not part of the matching surface.

### Layout

```
build\win-toolchain\
  toolchain\   msvc\{bin,include,lib} + dx\{Include,Lib} + ninja\ninja.exe
  llvm\        bin\ (clang.exe, clangd.exe, clang-format.exe, llvm-pdbutil.exe, …)
  ghidra\      a Ghidra 12.0.4 install (support\, Ghidra\, ghidraRun.bat)
  jdk\         a Temurin JDK 21 (bin\, lib\)
  bin\         objdiff(-cli).exe, ninja.exe, rg.exe, jq.exe, vostok-delinker.exe
  assets\      GRUNTZ.EXE + MSS32.DLL + SMACKW32.DLL
  cache\       downloaded archives + per-artifact .installed-* markers (idempotency)
  activate.ps1 / activate.bat
```

## The environment (`activate.ps1` / `activate.bat`)

The Windows equivalent of the flake's `shellHook`. It exports the same variables
(`GRUNTZ_DIR`, `GRUNTZ_EXE`, `GRUNTZ_CLANG`, `MSVC_DIR`, `DXSDK_DIR`, `NINJA_DIR`,
`GHIDRA_INSTALL_DIR`, `JAVA_HOME`, `GRUNTZ_RUNTIME`, `PYTHONPATH=…\scripts`),
prepends the tool dirs to `PATH`, and enables the repo's pre-commit format hook
(`core.hooksPath=.githooks`). Two differences from the flake shell:

- **No wine variables** (`WINEPREFIX`/`WINEDEBUG`/`WINEDLLOVERRIDES`) and no
  `wineserver`.
- **`INCLUDE` / `LIB` are real env vars.** On Linux the flake writes the MSVC/DX
  header+lib search dirs into the *wine registry*; native `cl.exe`/`link.exe`
  read them from the process environment, so the activate scripts set them
  directly (`msvc\include;dx\Include`, `msvc\lib;dx\Lib`).

A Python process can't mutate its parent shell, so — like `vcvarsall.bat` — you
*source/run* the activate script. (`--init`/`--shell` apply the same map to a
child process for you.)

## How the build runs without wine

The pipeline (`docs/build-system.md`) is unchanged; only the compiler/linker
launch differs. Each wine-specific step is guarded by `os.name == "nt"`:

- **`scripts/gruntz/build/cc_wrap.py`** — runs `cl.exe` directly (no `wine`
  prefix, no `winepath -w`, no persistent `wineserver`); on a timeout it kills the
  process via `CREATE_NEW_PROCESS_GROUP` + `proc.kill()` instead of `os.killpg`.
- **`scripts/gruntz/build/link.py`** — runs `link.exe` directly; no
  `WINEPREFIX` requirement and no wine-prefix `MSDIS100.DLL` install (Windows
  resolves it next to `link.exe`). Phase-2/opt-in, as on Linux.
- **`scripts/gruntz/init/toolchain.py`** — native MSVC needs no wine prefix and no
  wine registry; it just verifies `cl.exe` and relies on the `INCLUDE`/`LIB` the
  activate scripts export (with an optional native `--smoke` compile).
- **`scripts/gruntz/init/clangd.py`** — skips the lowercase-symlink header mirror;
  Windows' case-insensitive filesystem resolves `<string.h>` → `STRING.H` on its
  own (and symlinks would need admin/developer mode anyway).
- **`scripts/gruntz/cli.py`** — unchanged: its `wineserver` start/reap helpers
  already no-op when `WINEPREFIX` is unset.

## Idempotency & re-runs

A per-artifact marker (`build\win-toolchain\cache\.installed-<key>`) makes a
re-run a fast no-op; downloads are cached and verified by hash, so a re-run never
re-fetches a good archive. `--force` redoes everything; `--only KEY` scopes the
work (handy for `--only vostok_src` once Rust is installed). Deleting
`build\win-toolchain\` and re-running rebuilds it from scratch.

## Troubleshooting

- **`cargo not on PATH`** — vostok-delinker can't be built; install rustup +
  nightly, then `py bootstrap_windows.py --only vostok_src`. Everything else still
  provisions; only delink/objdiff need the delinker.
- **`SHA256 MISMATCH`** — a download was corrupted or a URL moved; the partial
  file is discarded. Re-run; if it persists, the pin in
  `scripts/gruntz/win/manifest.py` needs refreshing.
- **Ghidra/LLVM extraction errors deep in the tree** — enable long paths (above).
- **`cl.exe` can't find headers** — confirm `INCLUDE` is set (you sourced the
  activate script) and points at `…\toolchain\msvc\include`.
