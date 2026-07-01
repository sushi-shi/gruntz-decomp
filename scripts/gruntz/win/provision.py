"""provision.py - realise the pinned artifacts into build/win-toolchain/.

The Windows analogue of `nix build`: download (hash-checked) + unpack each
manifest entry into the layout env.py expects, then `cargo build` the pinned
vostok-delinker source into bin/vostok-delinker.exe. Idempotent: a re-run skips
artifacts whose install marker is present (use force=True to redo one/all).
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

from . import fetch
from .env import win_toolchain_root
from .manifest import Artifact, selected


def _install(art: Artifact, archive: Path, root: Path) -> None:
    dest = root / art.dest
    if art.kind == "raw":
        dest.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(archive, dest / (art.rename or art.filename))
        return
    if art.kind in ("tarxz", "targz"):
        fetch.extract_tar(archive, dest, art.strip)
    elif art.kind == "zip":
        fetch.extract_zip(archive, dest, art.strip)
    else:
        raise SystemExit(f"[provision] unknown kind {art.kind!r} for {art.filename}")
    for pick in art.bin_picks:
        srcs = list(dest.rglob(pick))
        if not srcs:
            raise SystemExit(f"[provision] {pick} not found under {dest}")
        bind = root / "bin"
        bind.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(srcs[0], bind / pick)


def _marker(root: Path, key: str) -> Path:
    return root / "cache" / f".installed-{key}"


def provision(repo: Path, *, no_gui: bool = False, force: bool = False,
              only: list[str] | None = None) -> Path:
    root = win_toolchain_root(repo)
    cache = root / "cache"
    cache.mkdir(parents=True, exist_ok=True)
    arts = selected(no_gui=no_gui)
    if only:
        arts = {k: v for k, v in arts.items() if k in only}

    for key, art in arts.items():
        marker = _marker(root, key)
        if marker.exists() and not force:
            print(f"[{key}] installed (marker present)")
            continue
        print(f"[{key}]")
        archive = fetch.download(art.url, cache / art.filename, art.sha256, force=force)
        _install(art, archive, root)
        marker.write_text(art.sha256 + "\n", encoding="utf-8")

    if (not only) or ("vostok_src" in only):
        _build_vostok(repo, root, force=force)
    _verify(root, no_gui=no_gui)
    return root


def _build_vostok(repo: Path, root: Path, *, force: bool) -> None:
    """`cargo build --release` the pinned source into bin/vostok-delinker.exe.

    This is the one artifact with no pinned binary - the flake builds the forked
    branch from source with a Rust NIGHTLY (it uses #![feature(os_string_truncate)]).
    We need a nightly cargo on PATH; we do not pin the Rust toolchain (neither does
    the flake - it tracks rust-overlay's `nightly.latest`)."""
    exe = root / "bin" / "vostok-delinker.exe"
    if exe.exists() and not force:
        print("[vostok] built (bin/vostok-delinker.exe present)")
        return
    src = root / "vostok-src"
    cargo = shutil.which("cargo")
    if not cargo:
        print("[vostok] WARNING: `cargo` not on PATH - cannot build vostok-delinker.\n"
              "         Install rustup + a nightly toolchain (https://rustup.rs), then\n"
              "         re-run:  py bootstrap_windows.py --only vostok_src\n"
              "         Everything else is provisioned; only delink/objdiff need this.")
        return
    # Keep ALL Rust state under build/ - RUSTUP_HOME/CARGO_HOME point into the
    # toolchain dir so installing nightly + caching crates never touch the user's
    # global ~/.rustup or ~/.cargo. Only the rustup/cargo launchers (which the user
    # installed) live outside build/; everything they download lands locally.
    renv = dict(os.environ)
    renv["RUSTUP_HOME"] = str(root / "rust" / "rustup")
    renv["CARGO_HOME"] = str(root / "rust" / "cargo")
    # Prefer an explicit nightly; fall back to the default toolchain if cargo is
    # already nightly. `rustup` is optional - if absent we just try `cargo build`.
    toolchain_arg = []
    if shutil.which("rustup"):
        subprocess.run(["rustup", "toolchain", "install", "nightly", "--profile", "minimal"],
                       check=False, env=renv)
        toolchain_arg = ["+nightly"]
    print("[vostok] cargo build --release (this compiles the delinker; Rust state "
          "stays under build/) ...")
    r = subprocess.run([cargo, *toolchain_arg, "build", "--release"],
                       cwd=str(src), check=False, env=renv)
    if r.returncode != 0:
        raise SystemExit("[vostok] cargo build failed - see output above. "
                         "Ensure a Rust nightly is installed (rustup toolchain install nightly).")
    built = next((src / "target" / "release").glob("vostok-delinker*.exe"), None)
    if not built:
        raise SystemExit("[vostok] build produced no vostok-delinker.exe under target/release.")
    (root / "bin").mkdir(parents=True, exist_ok=True)
    shutil.copyfile(built, exe)
    print(f"[vostok] -> {exe}")


def _verify(root: Path, *, no_gui: bool) -> None:
    """Sanity-check the realised layout - the tools the build graph invokes by
    name must actually be where env.py points PATH/MSVC_DIR/etc."""
    checks = {
        "MSVC cl.exe": root / "toolchain" / "msvc" / "bin",   # case-insensitive on Win
        "clang.exe": root / "llvm" / "bin" / "clang.exe",
        "llvm-pdbutil.exe": root / "llvm" / "bin" / "llvm-pdbutil.exe",
        "clang-format.exe": root / "llvm" / "bin" / "clang-format.exe",
        "ninja.exe": root / "bin" / "ninja.exe",
        "objdiff-cli.exe": root / "bin" / "objdiff-cli.exe",
        "rg.exe": root / "bin" / "rg.exe",
        "jq.exe": root / "bin" / "jq.exe",
        "GRUNTZ.EXE": root / "assets" / "GRUNTZ.EXE",
        "Ghidra support/": root / "ghidra" / "support",
        "JDK bin/java": root / "jdk" / "bin",
    }
    missing = []
    for label, p in checks.items():
        ok = p.exists() if p.suffix or p.name == "support" else any(p.iterdir()) if p.is_dir() else False
        if label == "MSVC cl.exe":
            ok = p.is_dir() and any(c.name.lower() == "cl.exe" for c in p.iterdir())
        elif label == "JDK bin/java":
            ok = (p / "java.exe").exists()
        if not ok:
            missing.append(f"  - {label}: {p}")
    if missing:
        print("[provision] WARNING: expected tools missing from the layout:\n"
              + "\n".join(missing), file=sys.stderr)
    else:
        print("[provision] layout verified - all core tools present.")
