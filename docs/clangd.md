# clangd LSP for the reconstructed tree

This is **additive editor tooling**. It gives any clangd-enabled editor (or an
agent driving clangd) go-to-definition, completion, hover types, and diagnostics
over `src/` and the vendored sources - parsed against the project's real
**MSVC 5.0 / MFC 4.2 / DirectX 6** headers.

It runs **alongside** the matching build and does not touch it. The Wine
MSVC 5.0 build (`configure.py` -> `build.ninja` -> `scripts/gruntz/build/cc_wrap.py`) remains
the **sole verdict on correctness**; clangd is only a *reader* of the code.

## The wrinkle: clangd can't use the real compiler

The matching build compiles each unit with **MSVC 5.0's `CL.EXE` run under Wine**
(via `scripts/gruntz/build/cc_wrap.py`). clangd is clang-based and cannot invoke that wrapper.
So we maintain a **separate** compilation database, in **clang-cl driver form**,
that points clang at the toolchain's MSVC/MFC/DirectX headers and asks it to
*emulate* MSVC 5.0. clang reads those headers as plain files - **no Wine and no
`CL.EXE`** are involved; this is parse-only, never a real compile.

MSVC 5.0 == `cl 11.00` == `_MSC_VER 1100`, hence `-fms-compatibility-version=11.00`.

## The compilation database

There is one `compile_commands.json`, in **clang-cl** form, for the clang-based
tools:

| DB                                   | Producer                | Form               | Consumer                          |
| ------------------------------------ | ----------------------- | ------------------ | --------------------------------- |
| `build/clangd/compile_commands.json` | `gruntz/init/clangd.py` | `clang-cl` (clang) | clangd, ghidra_metadata_generate, clangd_query |

The matching build itself needs no compdb (`configure.py` emits only `build.ninja`
+ the objdiff project; ninja tracks deps). The committed `.clangd` points clangd
at `build/clangd/`.

## Usage

1. Get the toolchain env (so the MSVC/DX include dirs resolve):

   ```sh
   nix develop .#build      # exports MSVC_DIR and DXSDK_DIR
   ```

   `scripts/gruntz/init/clangd.py` also works **outside** the dev shell - if the env vars are
   absent it falls back to `nix build .#gruntz-toolchain` and reads the include
   dirs out of the result path.

2. Generate the database (idempotent - re-run whenever `config/units.toml`
   grows a new unit):

   ```sh
   python3 scripts/gruntz/init/clangd.py
   ```

   It prints the resolved include dirs and the exact per-unit clang-cl flag line.

3. Open the repo in any clangd-enabled editor (VS Code clangd, Neovim LSP,
   Emacs eglot/lsp-mode, ...). It reads `.clangd` and the generated DB
   automatically.

   `clangd` itself ships in the dev shell via `pkgs.clang-tools` in the flake's
   `commonTools`, so it is on `PATH` inside `nix develop`.

### One-off check from the CLI

```sh
clangd --compile-commands-dir=build/clangd --check=vendor/zlib-1.0.4/adler32.c
```

A successful run ends with `All checks completed, 0 errors`.

## How it is wired

- **`scripts/gruntz/init/clangd.py`** - reads `config/units.toml` (read-only) and writes
  `build/clangd/compile_commands.json` (git-ignored; `build/` is in
  `.gitignore`). One clang-cl entry per unit:

  ```
  clang-cl /c <src> --target=i386-pc-windows-msvc \
    -fms-compatibility-version=11.00 -fms-extensions -fdelayed-template-parsing \
    /imsvc <MSVC_DIR>/include /imsvc <DXSDK_DIR>/Include \
    /D_X86_ /DWIN32 /D_WINDOWS /D_MBCS
  ```

  - **`/imsvc`** marks the toolchain headers as *system* includes, so clangd
    suppresses diagnostics that originate inside the ancient MFC/CRT/DirectX
    headers - we only care about diagnostics in *our* sources.
  - **`_MBCS`** (and **not** `_AFXDLL`): we link **static ANSI/MBCS MFC 4.2**
    (`NAFXCW.LIB`), not the shared `_AFXDLL` build, and not `_UNICODE`.

- **`.clangd`** (repo root, committed) - points clangd at `build/clangd/`, mirrors
  the target / ms-compat / ms-extensions / delayed-template-parsing flags
  globally, and adds **`-ferror-limit=0`**. That last one is essential: the
  MSVC-dialect cascades in the MFC headers exceed clangd's default ~20-error
  limit, which would **abort the parse mid-TU** and lose the AST below the abort
  point. clangd's command mangler drops `-ferror-limit` when it comes from the
  compdb, so it must live in `.clangd`. A few noisy clang-cl warnings
  (`-Wmicrosoft`, pragma/pack, nonportable include path) are suppressed.

## Caveats / what needs refinement

- **MSVC5 emulation is best-effort.** clang's `-fms-compatibility` does not model
  every MSVC 5.0 dialect quirk in MFC 4.2. clangd is forgiving (it recovers and
  still serves navigation/hover even on a partially-erroring TU), but expect some
  spurious diagnostics in heavy MFC code. They do **not** reflect the real build.
- **Full MFC-header validation is deferred.** Today's `src/*.cpp` units are small
  self-contained ctors whose project headers do not yet drag in `<afxwin.h>` /
  `<windows.h>` / `<ddraw.h>`, so they parse cleanly (`0 errors`). The real test
  of the MFC/DX header parse comes once the reconstructed `src/` actually
  `#include`s those toolchain headers. Tune the `.clangd` `Suppress` list then.
- **Re-run `gruntz clangd` after adding a unit** to `config/units.toml`; the DB
  is not regenerated automatically (unlike the build's, which `configure.py`
  emits).
