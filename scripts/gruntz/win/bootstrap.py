"""bootstrap.py - the orchestrator behind the repo-root bootstrap_windows.py.

The Windows replacement for `nix develop`: provision the pinned toolchain, write
the activate scripts, and (optionally) enter the environment or run `gruntz init`.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from . import activate, provision
from .env import apply_to_environ
from .manifest import ARTIFACTS, SFMAN32_PLACEHOLDER


def _find_repo(start: Path) -> Path:
    for p in [start, *start.parents]:
        if (p / "flake.nix").exists():
            return p
    raise SystemExit("[bootstrap] could not locate the repo root (no flake.nix above "
                     f"{start}).")


def _print_hashes() -> None:
    print(f"{'key':12s} {'sha256':64s}  filename")
    for key, a in ARTIFACTS.items():
        print(f"{key:12s} {a.sha256:64s}  {a.filename}")
    if SFMAN32_PLACEHOLDER:
        print("\nsfman32: placeholder (not fetched), exactly as in flake.nix.")


def _enter_shell(repo: Path, prefer_cmd: bool) -> int:
    env = apply_to_environ(repo)
    root = repo / "build" / "win-toolchain"
    if os.name != "nt":
        print("[bootstrap] --shell only makes sense on Windows.", file=sys.stderr)
        return 1
    import shutil
    if not prefer_cmd and shutil.which("powershell"):
        ps1 = root / "activate.ps1"
        return subprocess.run(["powershell", "-NoExit", "-NoLogo", "-Command",
                               f". '{ps1}'"], env=env).returncode
    bat = root / "activate.bat"
    return subprocess.run(["cmd", "/k", str(bat)], env=env).returncode


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        prog="bootstrap_windows.py",
        description="Provision the Gruntz decomp toolchain on native Windows "
                    "(the flake.nix port). Downloads are SHA256-pinned.")
    ap.add_argument("--no-gui", action="store_true",
                    help="skip the objdiff GUI binary (keep objdiff-cli).")
    ap.add_argument("--force", action="store_true",
                    help="re-download and re-extract everything.")
    ap.add_argument("--only", nargs="+", metavar="KEY",
                    help=f"provision only these artifacts ({', '.join(ARTIFACTS)}).")
    ap.add_argument("--skip-provision", action="store_true",
                    help="only (re)write the activate scripts.")
    ap.add_argument("--init", action="store_true",
                    help="after provisioning, run `gruntz init` (builds the Ghidra DB).")
    ap.add_argument("--shell", action="store_true",
                    help="after provisioning, launch an activated subshell.")
    ap.add_argument("--cmd", action="store_true",
                    help="with --shell, prefer cmd.exe over PowerShell.")
    ap.add_argument("--print-hashes", action="store_true",
                    help="print the pinned manifest and exit.")
    args = ap.parse_args(argv)

    if args.print_hashes:
        _print_hashes()
        return 0

    repo = _find_repo(Path(__file__).resolve())

    if os.name != "nt":
        print("[bootstrap] NOTE: this provisions a WINDOWS toolchain (native MSVC 5.0).\n"
              "            Downloads/verification work anywhere, but the native build "
              "needs Windows.", file=sys.stderr)

    if not args.skip_provision:
        provision.provision(repo, no_gui=args.no_gui, force=args.force, only=args.only)

    ps1, bat = activate.write_scripts(repo)
    print(f"\n[bootstrap] wrote {ps1}")
    print(f"[bootstrap] wrote {bat}")

    if args.shell:
        return _enter_shell(repo, prefer_cmd=args.cmd)

    if args.init:
        env = apply_to_environ(repo)
        print("[bootstrap] running `gruntz init` ...")
        return subprocess.run([sys.executable, "-m", "gruntz", "init"],
                              cwd=str(repo), env=env).returncode

    rel = "build\\win-toolchain"
    print("\nNext - enter the dev environment, then build:")
    print(f"  PowerShell :  . .\\{rel}\\activate.ps1")
    print(f"  cmd.exe    :  {rel}\\activate.bat")
    print("  then       :  gruntz init   (first run builds the Ghidra DB, ~minutes)")
    print("               gruntz build  (compile + delink + objdiff)")
    print("\n  or in one step:  py bootstrap_windows.py --init")
    return 0
