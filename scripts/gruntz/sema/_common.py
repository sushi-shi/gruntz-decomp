"""gruntz.sema._common - shared infra for the sema subcommand modules.

Paths, the subprocess delegation helper, the CSV/annotation lookups and the
usage log. Sema modules import from here only; nothing here imports cli.py
(dependency direction: cli -> sema -> analysis).
"""
import os
import subprocess
import sys
import tomllib
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SCRIPTS = REPO / "scripts"
BUILD_PKG = Path(__file__).resolve().parents[1] / "build"  # cc_wrap.py, codeview.py
MANIFEST = REPO / "config" / "units.toml"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"
REPORT = REPO / "build" / "objdiff" / "report.json"
GHIDRA_FUNCTIONS = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"


def die(msg: str) -> None:
    print(f"[gruntz] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def pkg_env() -> dict:
    """os.environ with scripts/ guaranteed on PYTHONPATH for child `python -m`."""
    env = dict(os.environ)
    existing = env.get("PYTHONPATH", "")
    if str(SCRIPTS) not in existing.split(os.pathsep):
        env["PYTHONPATH"] = os.pathsep.join(p for p in (str(SCRIPTS), existing) if p)
    return env


def run_tool(module: str, argv: list) -> int:
    """Stream a read-only navigation tool's output (no `[gruntz] $` log noise)."""
    return subprocess.run([sys.executable, "-m", module, *map(str, argv)],
                          cwd=str(REPO), env=pkg_env()).returncode


def units() -> list[dict]:
    with MANIFEST.open("rb") as f:
        return tomllib.load(f).get("unit", [])


def flags_for(udef: dict) -> list:
    """Resolve a unit's flags-profile name to its cl flag list (units.toml [flags])."""
    with MANIFEST.open("rb") as f:
        profiles = tomllib.load(f).get("flags", {})
    return list(profiles.get(udef.get("flags", ""), []))


def csv_find(path: Path, rva: int, key: str = "rva"):
    """First CSV row whose `key` column parses to `rva` (hex), or None."""
    import csv
    if not path.is_file():
        return None
    for r in csv.DictReader(path.open()):
        try:
            if int(r[key], 16) == rva:
                return r
        except (ValueError, KeyError):
            pass
    return None


def src_loc_of(rva: int):
    """(relpath, lineno) of the RVA(0x..)/RVAU(0x..) macro that defines the fn at
    `rva`, scanning src/ + include/. None if the fn is not annotated in source.
    Padding-agnostic: matches 0x0*<hex> so 0x0017fa40 and 0x17fa40 both hit."""
    import re
    pat = re.compile(r"\bRVAU?\s*\(\s*0x0*%x\s*[,)]" % rva, re.IGNORECASE)
    for sub in ("src", "include"):
        base = REPO / sub
        if not base.is_dir():
            continue
        for f in sorted(base.rglob("*")):
            if f.suffix not in (".c", ".cpp", ".cc", ".cxx", ".h", ".hpp"):
                continue
            try:
                for i, line in enumerate(f.read_text(errors="ignore").splitlines(), 1):
                    if pat.search(line):
                        return (f.relative_to(REPO).as_posix(), i)
            except OSError:
                continue
    return None


def log_invocation(rc: int) -> None:
    """One line per `gruntz sema` invocation (ALL subcommands); must NEVER break
    the tool. Metadata first, command after the `: ` (shell-quoted):
        [2026-07-04][19:55:01][0]: gruntz sema xref 0x00080850 --raw
    Usage-analysis feed: what agents actually run -> tool improvements."""
    try:
        import datetime
        import shlex
        now = datetime.datetime.now()
        cmd = shlex.join(["gruntz", *sys.argv[1:]])  # exactly what was invoked
        path = REPO / "build" / "gruntz_sema.log"
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "a") as f:
            f.write("[{}][{}][{}]: {}\n".format(
                now.date(), now.strftime("%H:%M:%S"), rc, cmd))
    except Exception:
        pass  # logging is best-effort by design
