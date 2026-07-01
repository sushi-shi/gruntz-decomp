"""fetch.py - download, SHA256-verify, and extract artifacts (stdlib only).

The flake delegates this to Nix's fetchurl (hash-checked) + the per-package
unpack/strip. Here it is plain urllib + hashlib + tarfile/zipfile, so the only
prerequisite on a fresh Windows box is a Python 3.11+ interpreter.
"""

from __future__ import annotations

import hashlib
import os
import shutil
import tarfile
import urllib.request
import zipfile
from pathlib import Path

_UA = {"User-Agent": "gruntz-win-bootstrap/1.0"}
_CHUNK = 1 << 20


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for blk in iter(lambda: f.read(_CHUNK), b""):
            h.update(blk)
    return h.hexdigest()


def _winlong(p: Path) -> str:
    r"""\\?\-prefix an absolute path on Windows so deep trees (Ghidra, LLVM)
    dodge the legacy 260-char MAX_PATH limit during extraction."""
    if os.name != "nt":
        return str(p)
    s = str(p.resolve())
    return s if s.startswith("\\\\?\\") else "\\\\?\\" + s


def download(url: str, dest: Path, expected_sha: str, *, force: bool = False) -> Path:
    """Fetch `url` to `dest`, verifying SHA256. Skips the download when `dest`
    already matches the hash (idempotent), unless `force`."""
    if dest.exists() and not force:
        if sha256_file(dest) == expected_sha:
            print(f"  cached    {dest.name}")
            return dest
        print(f"  stale     {dest.name} (hash mismatch) - re-downloading")
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    print(f"  download  {dest.name}  <- {url}")
    req = urllib.request.Request(url, headers=_UA)
    with urllib.request.urlopen(req) as resp, tmp.open("wb") as out:
        total = int(resp.headers.get("Content-Length", 0))
        got = 0
        last = -1
        for blk in iter(lambda: resp.read(_CHUNK), b""):
            out.write(blk)
            got += len(blk)
            if total:
                pct = got * 100 // total
                if pct != last and pct % 5 == 0:
                    print(f"            {pct:3d}%  ({got >> 20} / {total >> 20} MiB)", end="\r")
                    last = pct
    if total:
        print()
    actual = sha256_file(tmp)
    if actual != expected_sha:
        tmp.unlink(missing_ok=True)
        raise SystemExit(
            f"[fetch] SHA256 MISMATCH for {dest.name}\n"
            f"        expected {expected_sha}\n        got      {actual}\n"
            f"        url      {url}")
    tmp.replace(dest)
    return dest


def _safe_members_tar(tf: tarfile.TarFile, strip: int):
    for m in tf.getmembers():
        parts = m.name.split("/")
        if strip:
            if len(parts) <= strip:
                continue                       # the stripped top dir itself
            parts = parts[strip:]
        m.name = "/".join(parts)
        if not m.name or m.name.startswith(("/", "..")) or ".." in parts:
            continue                           # path-traversal guard
        yield m


def extract_tar(archive: Path, dest: Path, strip: int) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive, "r:*") as tf:
        members = list(_safe_members_tar(tf, strip))
        # filter="data" (3.12+) blocks unsafe members; harmless to omit on 3.11.
        kw = {"filter": "data"} if hasattr(tarfile, "data_filter") else {}
        tf.extractall(_winlong(dest), members=members, **kw)


def extract_zip(archive: Path, dest: Path, strip: int) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as zf:
        for info in zf.infolist():
            if info.is_dir():
                continue
            parts = info.filename.split("/")
            if strip:
                if len(parts) <= strip:
                    continue
                parts = parts[strip:]
            if not parts or ".." in parts:
                continue
            target = Path(_winlong(dest)) / Path(*parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with zf.open(info) as src, open(target, "wb") as dst:
                shutil.copyfileobj(src, dst)
