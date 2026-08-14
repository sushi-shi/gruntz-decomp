"""gruntz.tool.wine - the shared era-toolchain plumbing.

    gruntz tool wine --init [--force] | --verify | --shutdown

Everything cl/link/rc need to run under wine, in one place: tool lookup,
path translation, the persistent wineserver, prefix initialisation (registry
PATH/INCLUDE/LIB + MSDIS100.DLL for link.exe), and the one hang-proof runner.
Callers above tool/ never see wine.
"""

from __future__ import annotations

import os
import shutil
import signal
import subprocess
import tempfile
from pathlib import Path

from gruntz.core.paths import dxsdk_dir, msvc_dir
from gruntz.tool import ToolError


def find_ci(d: Path, name: str) -> Path | None:
    """Case-insensitive lookup (the toolchain mixes CL.EXE / cl.exe case)."""
    if not d.is_dir():
        return None
    low = name.lower()
    return next((p for p in d.iterdir() if p.name.lower() == low), None)


def era_tool(name: str) -> Path:
    """$MSVC_DIR/bin/<name>, or a ToolError naming the fix."""
    p = find_ci(msvc_dir() / "bin", name)
    if p is None:
        raise ToolError(f"{name} not found under {msvc_dir()}/bin - run inside "
                        "`nix develop` (toolchain release r2+ carries rc.exe)")
    if shutil.which("wine") is None:
        raise ToolError("wine not found - run inside `nix develop`")
    return p


def winepath(p: Path | str) -> str:
    """Unix path -> windows path. stderr is discarded on purpose: winepath can
    be the call that boots the persistent wine session, and a daemonised
    session inheriting our stderr holds the caller's pipe open forever."""
    return subprocess.check_output(["winepath", "-w", str(p)],
                                   text=True, stderr=subprocess.DEVNULL).strip()


def ensure_wineserver() -> None:
    """`wineserver -p`: persist the server past the last client, so parallel
    `wine cl` invocations under ninja skip the cold start. Idempotent."""
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-p"], check=False, stdin=subprocess.DEVNULL,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def shutdown_wineserver() -> None:
    """`wineserver -k`: reap the persistent server (leaked servers slow builds
    and hold deleted files open - kill between long sessions)."""
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-k"], check=False)


def run(argv: list[str], *, cwd: Path | None = None,
        timeout: float | None = None,
        success: Path | None = None) -> tuple[str, int]:
    """Run one wine tool hang-proof; return (combined output, returncode).

    Wine intermittently leaves a finished-but-unreaped grandchild
    (mspdbsrv/conhost/...) holding the inherited stdio, which wedges a capture
    PIPE forever even though the artifact is already written. So: output to a
    temp FILE, the tool in its own process group, a bounded wait; on a stall
    SIGKILL the group and let `success` (the artifact the caller expects)
    decide the verdict.
    """
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    ensure_wineserver()
    if timeout is None:
        timeout = float(os.environ.get("GRUNTZ_WINE_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(argv, cwd=str(cwd) if cwd else None,
                                stdin=subprocess.DEVNULL, stdout=logf,
                                stderr=subprocess.STDOUT, start_new_session=True)
        try:
            proc.wait(timeout=timeout)
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            proc.wait()
            rc = 0 if success is not None and success.exists() else 1
        logf.seek(0)
        return logf.read().decode("utf-8", "replace"), rc


# --------------------------------------------------------------------------- #
# prefix initialisation
# --------------------------------------------------------------------------- #

_ENV_KEY = (r"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control"
            r"\Session Manager\Environment")


def _reg(*args: str, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(["wine", "reg", *args], check=False, text=True,
                          capture_output=capture)


def ensure_link_deps() -> None:
    """link.exe statically imports MSDIS100.DLL, which itself imports
    MSVCP50.DLL. Toolchain r3+ bundles both beside link.exe (the app dir wins
    the DLL search), so this is a presence check, not provisioning."""
    for name in ("msdis100.dll", "msvcp50.dll"):
        if find_ci(msvc_dir() / "bin", name) is None:
            raise ToolError(f"{name} missing from the toolchain - re-pin the "
                            "r3+ toolchain release")


def init_prefix(force: bool = False) -> None:
    """Boot the prefix and set PATH/INCLUDE/LIB in the wine registry so era
    tools find binaries/headers/libs. DX6 comes FIRST in INCLUDE/LIB: VC5
    ships DirectX 3-era DDRAW.H/DPLAY.H which would shadow the DX6 SDK's
    (IID_IDirectPlay4A would not resolve)."""
    prefix = Path(os.environ.get("WINEPREFIX") or Path.home() / ".wine")
    if force or not (prefix / "drive_c").is_dir():
        prefix.mkdir(parents=True, exist_ok=True)
        subprocess.run(["wineboot", "--init"], check=True)
        subprocess.run(["wineserver", "--wait"], check=False)

    msvc, dx = msvc_dir(), dxsdk_dir()
    vc_bin = winepath(msvc / "bin")
    include = ";".join([winepath(dx / "Include"), winepath(msvc / "include")])
    lib = ";".join([winepath(dx / "Lib"), winepath(msvc / "lib")])

    cur = _reg("query", _ENV_KEY, "/v", "PATH", capture=True)
    cur_path = next((l.split()[-1] for l in cur.stdout.splitlines() if "REG_" in l), "")
    if not cur_path:
        _reg("add", _ENV_KEY, "/v", "PATH", "/t", "REG_EXPAND_SZ",
             "/d", f"{vc_bin};%SystemRoot%\\system32;%SystemRoot%", "/f")
    elif vc_bin not in cur_path:
        _reg("add", _ENV_KEY, "/v", "PATH", "/t", "REG_EXPAND_SZ",
             "/d", f"{vc_bin};{cur_path}", "/f")
    _reg("add", _ENV_KEY, "/v", "INCLUDE", "/t", "REG_SZ", "/d", include, "/f")
    _reg("add", _ENV_KEY, "/v", "LIB", "/t", "REG_SZ", "/d", lib, "/f")
    ensure_link_deps()


def verify_prefix() -> None:
    """Fail unless the registry INCLUDE exists and lists dx before msvc."""
    got = _reg("query", _ENV_KEY, "/v", "INCLUDE", capture=True)
    val = "".join(l for l in got.stdout.splitlines() if "REG_" in l).lower()
    if "include" not in val:
        raise ToolError("wine registry INCLUDE unset - run init_prefix() "
                        "(a cold wineserver can fail the first winepath)")
    if "dx" not in val or val.find("dx") > val.find("msvc"):
        raise ToolError("wine registry INCLUDE does not put dx/Include before "
                        "msvc/include - run init_prefix(force=True)")


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--init", action="store_true", help="initialise the prefix")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--shutdown", action="store_true", help="kill the wineserver")
    a = ap.parse_args()
    try:
        if a.init:
            init_prefix(force=a.force)
        if a.verify:
            verify_prefix()
            print("prefix OK")
    except ToolError as e:
        print(f"[wine] {e}", file=sys.stderr)
        return 1
    if a.shutdown:
        shutdown_wineserver()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
