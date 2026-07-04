"""env.py - the dev-shell environment, derived from the provisioned layout.

This is the Windows counterpart of flake.nix's `shellHook`, MINUS everything
wine: no WINEPREFIX / WINEDEBUG / WINEDLLOVERRIDES, no wineserver. MSVC 5.0 runs
natively, so its header/lib search dirs are exported as the ordinary INCLUDE/LIB
environment variables (on Linux the flake writes those into the wine registry).

Single source of truth for both activate.py (emits activate.ps1/.bat) and
bootstrap --init/--shell (applies the same map to os.environ before spawning).
"""

from __future__ import annotations

import os
from pathlib import Path


def win_toolchain_root(repo: Path) -> Path:
    return repo / "build" / "win-toolchain"


def compute(repo: Path) -> tuple[dict[str, str], list[Path]]:
    """Return (env_vars, path_prepend_dirs) for the activated shell.

    Paths are absolute; values are plain strings. INCLUDE/LIB/PYTHONPATH/PATH are
    returned as the bits to PREPEND (the renderers join them with the inherited
    value) - everything else is an outright assignment.
    """
    root = win_toolchain_root(repo)
    tc = root / "toolchain"
    msvc = tc / "msvc"
    dx = tc / "dx"
    llvm = root / "llvm"

    env = {
        "GRUNTZ_DIR": str(repo),
        "GRUNTZ_EXE": str(root / "assets" / "GRUNTZ.EXE"),
        "GRUNTZ_RUNTIME": str(root / "assets"),
        "GRUNTZ_TOOLCHAIN": str(tc),
        "MSVC_DIR": str(msvc),
        "DXSDK_DIR": str(dx),
        "NINJA_DIR": str(tc / "ninja"),
        "GHIDRA_INSTALL_DIR": str(root / "ghidra"),
        "JAVA_HOME": str(root / "jdk"),
        "GRUNTZ_CLANG": str(llvm / "bin" / "clang.exe"),
    }

    # The wine-registry INCLUDE/LIB equivalent: native cl.exe/link.exe read these
    # from the process environment. VC5 dirs first (CRT + Win32 + MFC, bundled),
    # then DX6 - same order as configure_registry on Linux.
    include = [str(msvc / "include"), str(dx / "Include")]
    lib = [str(msvc / "lib"), str(dx / "Lib")]

    # PATH: loose tools (objdiff, rg, jq, vostok-delinker, ninja) + LLVM + JDK.
    path_prepend = [
        root / "bin",
        tc / "ninja",
        llvm / "bin",
        root / "jdk" / "bin",
    ]

    # scripts/ is THE package root (PYTHONPATH), same as the flake shells.
    pythonpath_prepend = repo / "scripts"

    env["_INCLUDE_PREPEND"] = os.pathsep.join(include)
    env["_LIB_PREPEND"] = os.pathsep.join(lib)
    env["_PYTHONPATH_PREPEND"] = str(pythonpath_prepend)
    return env, path_prepend


def apply_to_environ(repo: Path, environ: dict | None = None) -> dict:
    """Mutate (a copy of) `environ` in place with the computed env, for spawning
    `gruntz` as a child (bootstrap --init / --shell). Returns the dict."""
    e = dict(os.environ if environ is None else environ)
    env, path_prepend = compute(repo)
    for k, v in env.items():
        if k.startswith("_"):
            continue
        e[k] = v
    sep = os.pathsep
    e["PATH"] = sep.join(str(p) for p in path_prepend) + sep + e.get("PATH", "")
    e["INCLUDE"] = env["_INCLUDE_PREPEND"] + (sep + e["INCLUDE"] if e.get("INCLUDE") else "")
    e["LIB"] = env["_LIB_PREPEND"] + (sep + e["LIB"] if e.get("LIB") else "")
    e["PYTHONPATH"] = env["_PYTHONPATH_PREPEND"] + (sep + e["PYTHONPATH"] if e.get("PYTHONPATH") else "")
    return e
