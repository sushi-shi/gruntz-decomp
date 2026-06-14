#!/usr/bin/env python3
"""gen_clangd.py - generate a clangd compilation database for the Gruntz tree.

This is ADDITIVE editor tooling that runs ALONGSIDE the real matching build; it
does not touch it. The matching build compiles with MSVC 5.0's CL.EXE under wine
(scripts/cc_wrap.py), which clangd - being clang-based - cannot invoke. So we
emit our OWN compilation database, in clang-cl driver form, that points clang at
the toolchain's MSVC/MFC/DirectX headers and asks it to *emulate* MSVC 5.0
(cl 11.00 == _MSC_VER 1100). clang reads those headers as plain files, so no wine
and no CL.EXE are needed - this is parse-only, never a real compile.

Output: build/clangd/compile_commands.json   (git-ignored; build/ is in .gitignore)

The DB lives under build/clangd/ - NOT the repo root - so it never collides with
the matching build's own repo-root compile_commands.json (emitted by configure.py
for the `cl`-under-wine invocations). The committed .clangd file points clangd at
build/clangd/ via CompileFlags.CompilationDatabase.

Source of truth: config/units.toml (read-only here). One clang-cl entry is
emitted per [[unit]] - currently the 9 vendored zlib .c TUs plus any src/*.cpp
that the manifest has grown. New units are picked up automatically on re-run.

Include resolution (in priority order):
  1. $MSVC_DIR/include and $DXSDK_DIR/Include - exported by `nix develop .#build`.
  2. otherwise `nix build .#gruntz-toolchain` and read msvc/include + dx/Include
     from the result symlink.

Usage:
    # inside `nix develop .#build` (env already set), or plain - it will fall
    # back to building the toolchain:
    python3 scripts/gen_clangd.py
"""

import json
import os
import subprocess
import sys
import tomllib
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
MANIFEST = REPO / "config" / "units.toml"
OUT_DIR = REPO / "build" / "clangd"
OUT_FILE = OUT_DIR / "compile_commands.json"

# MSVC 5.0 == cl 11.00 == _MSC_VER 1100. Tell clang to emulate that compiler so
# the ancient MFC 4.2 / CRT headers take their MSVC-version codepaths.
MSC_COMPAT = "11.00"
TARGET = "i386-pc-windows-msvc"

# Preprocessor defines matching the matching build's environment:
#   _X86_ / WIN32 / _WINDOWS - 32-bit Windows app.
#   _MBCS                    - static ANSI/MBCS MFC 4.2 (NAFXCW.LIB), the build
#                              we link against; _UNICODE is NOT defined.
#   _AFXDLL is deliberately NOT defined  - static MFC, not the shared DLL build.
DEFINES = ["/D_X86_", "/DWIN32", "/D_WINDOWS", "/D_MBCS"]


def resolve_include_dirs() -> tuple[Path, Path, str]:
    """Return (msvc_include, dx_include, source) for the toolchain headers.

    Prefers the env vars exported by `nix develop .#build`; otherwise builds
    .#gruntz-toolchain and reads the include dirs out of the result path.
    """
    msvc_dir = os.environ.get("MSVC_DIR")
    dx_dir = os.environ.get("DXSDK_DIR")
    if msvc_dir and dx_dir:
        msvc_inc = Path(msvc_dir) / "include"
        dx_inc = Path(dx_dir) / "Include"
        if msvc_inc.is_dir() and dx_inc.is_dir():
            return msvc_inc, dx_inc, "env ($MSVC_DIR / $DXSDK_DIR)"

    # Fall back to building the toolchain and reading its result path.
    print("[gen_clangd] MSVC_DIR/DXSDK_DIR not in env - "
          "running `nix build .#gruntz-toolchain` ...", file=sys.stderr)
    try:
        out = subprocess.check_output(
            ["nix", "build", ".#gruntz-toolchain", "--no-link", "--print-out-paths"],
            cwd=str(REPO), text=True,
        ).strip().splitlines()
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise SystemExit(
            "[gen_clangd] ERROR: could not resolve the toolchain headers. "
            "Run inside `nix develop .#build` (sets MSVC_DIR/DXSDK_DIR), or "
            f"ensure `nix build .#gruntz-toolchain` works.\n  cause: {e}")
    root = Path(out[-1])
    msvc_inc = root / "msvc" / "include"
    dx_inc = root / "dx" / "Include"
    if not msvc_inc.is_dir() or not dx_inc.is_dir():
        raise SystemExit(
            f"[gen_clangd] ERROR: toolchain at {root} is missing "
            f"msvc/include or dx/Include.")
    return msvc_inc, dx_inc, f"nix build .#gruntz-toolchain ({root})"


def load_units(path: Path) -> list[dict]:
    with path.open("rb") as f:
        data = tomllib.load(f)
    units = data.get("unit", [])
    if not units:
        raise SystemExit(f"{path}: no [[unit]] entries")
    return units


def base_flags(msvc_inc: Path, dx_inc: Path) -> list[str]:
    """The clang-cl flag set shared by every unit.

    /imsvc marks the toolchain headers as SYSTEM includes so clangd suppresses
    diagnostics that originate inside the ancient MFC/CRT headers (we only care
    about diagnostics in our own sources).
    """
    return [
        f"--target={TARGET}",
        f"-fms-compatibility-version={MSC_COMPAT}",
        "-fms-extensions",
        # MFC's headers are full of un-instantiated templates that only parse
        # under MSVC's lazy two-phase lookup; delay template parsing to match.
        "-fdelayed-template-parsing",
        # Treat the old toolchain headers as system headers (diagnostics there
        # are silenced); resolves WINDOWS.H, AFX*.H, ddraw.h, etc.
        "/imsvc", str(msvc_inc),
        "/imsvc", str(dx_inc),
        *DEFINES,
    ]


def main() -> None:
    msvc_inc, dx_inc, source = resolve_include_dirs()
    units = load_units(MANIFEST)
    shared = base_flags(msvc_inc, dx_inc)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    entries = []
    for u in units:
        src = u["source"]
        entries.append({
            "directory": str(REPO),
            "file": src,
            # clang-cl driver form. clangd parses internally; no clang-cl binary
            # and no wine are required.
            "arguments": ["clang-cl", "/c", src, *shared],
        })

    with OUT_FILE.open("w", encoding="utf-8") as f:
        json.dump(entries, f, indent=2)
        f.write("\n")

    # Print the exact flag line so it is easy to reproduce a single-file check.
    print(f"[gen_clangd] wrote {OUT_FILE.relative_to(REPO)} "
          f"({len(entries)} units)")
    print(f"[gen_clangd] include dirs ({source}):")
    print(f"    MSVC/MFC : {msvc_inc}")
    print(f"    DirectX  : {dx_inc}")
    print("[gen_clangd] clang-cl flags per unit:")
    print("    clang-cl /c <src> " + " ".join(shared))


if __name__ == "__main__":
    main()
