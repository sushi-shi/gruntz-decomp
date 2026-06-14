#!/usr/bin/env python3
"""One-shot clangd queries over the generated compile_commands.json - source
navigation for humans and agents without an editor: where is this defined,
who references it, what is its type.

  python3 scripts/clangd_query.py symbol <fuzzy-name>           # workspace symbols
  python3 scripts/clangd_query.py def  <file> <line> [<col>]    # definition
  python3 scripts/clangd_query.py refs <file> <line> [<col>]    # references
  python3 scripts/clangd_query.py hover <file> <line> [<col>]   # type/doc at point
  python3 scripts/clangd_query.py index                         # build the background
                                                                # index, wait until done

Lines/columns are 1-based (as printed by grep/editors). When <col> is omitted,
identifiers on the line are probed right-to-left (the declared name sits
rightmost on C++ declaration lines) until one yields a result.

Cross-TU answers (symbol, refs into other TUs) come from clangd's background
index, built incrementally under .cache/clangd/ on first use - early runs may
return partial results until it fills (the tool prints indexing progress).
Definition/hover on an open file need no index. clang is a READER of this
MSVC5 dialect (it parses our reconstructed src/ against the VC5/MFC/DX headers
via build/clangd/compile_commands.json): navigation is reliable, its
diagnostics are NOT build truth - the wine `cl` build + objdiff are. Regenerate
the compdb with `python3 scripts/gen_clangd.py` after adding a unit.

Adapted from srp-survarium/vostok scripts/clangd_query.py.
"""

import argparse
import json
import os
import re
import select
import subprocess
import sys
import time
from pathlib import Path

ROOT = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
CDB_DIR = ROOT / "build" / "clangd"
CDB = CDB_DIR / "compile_commands.json"


class Clangd:
    def __init__(self):
        self.proc = subprocess.Popen(
            ["clangd", "--background-index", "--log=error",
             f"--compile-commands-dir={CDB_DIR}"],
            cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        self._id = 0
        self._request("initialize", {
            "processId": os.getpid(),
            "rootUri": ROOT.as_uri(),
            # workDoneProgress lets clangd report background-index progress
            "capabilities": {"window": {"workDoneProgress": True}},
        })
        self._notify("initialized", {})

    def _send(self, payload: dict) -> None:
        body = json.dumps({"jsonrpc": "2.0", **payload}).encode()
        self.proc.stdin.write(f"Content-Length: {len(body)}\r\n\r\n".encode() + body)
        self.proc.stdin.flush()

    def _recv(self) -> dict:
        headers = b""
        while not headers.endswith(b"\r\n\r\n"):
            headers += self.proc.stdout.read(1)
        length = int(re.search(rb"Content-Length: (\d+)", headers).group(1))
        return json.loads(self.proc.stdout.read(length))

    def _notify(self, method: str, params: dict) -> None:
        self._send({"method": method, "params": params})

    def _request(self, method: str, params: dict, timeout: float = 120.0):
        self._id += 1
        self._send({"id": self._id, "method": method, "params": params})
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            msg = self._recv()
            if msg.get("id") == self._id:
                if "error" in msg:
                    sys.exit(f"clangd error: {msg['error'].get('message')}")
                return msg.get("result")
        sys.exit(f"clangd: no reply to {method} within {timeout}s")

    def wait_for_index(self) -> None:
        """Block until background indexing reports 'end' (or stays idle)."""
        begun = False
        last = time.monotonic()
        while True:
            ready, _, _ = select.select([self.proc.stdout], [], [], 5.0)
            if not ready:
                # pre-begin: clangd first validates existing shards quietly,
                # which can take a while on a warm cache - be patient
                if time.monotonic() - last > (30.0 if begun else 90.0):
                    print("index: idle - nothing (left) to index" if not begun
                          else "index: no progress for 30s - assuming done")
                    return
                continue
            msg = self._recv()
            last = time.monotonic()
            # clangd asks permission to create the progress token - grant it
            if msg.get("method") == "window/workDoneProgress/create":
                self._send({"id": msg["id"], "result": None})
                continue
            if msg.get("method") != "$/progress":
                continue
            value = msg["params"]["value"]
            if value.get("kind") == "begin":
                begun = True
            elif value.get("kind") == "report":
                pct = value.get("percentage")
                msg_txt = value.get("message", "")
                print(f"\rindex: {pct}% {msg_txt[:60]:<60}", end="", flush=True)
            elif value.get("kind") == "end" and begun:
                print("\nindex: done")
                return

    def open_file(self, path: Path) -> None:
        self._notify("textDocument/didOpen", {"textDocument": {
            "uri": path.as_uri(), "languageId": "cpp", "version": 1,
            "text": path.read_text(errors="replace"),
        }})

    def close(self) -> None:
        try:
            self._request("shutdown", {}, timeout=5)
            self._notify("exit", {})
        except Exception:  # noqa: BLE001 - best-effort teardown of a one-shot
            pass
        self.proc.terminate()


def rel(uri_or_path: str) -> str:
    p = uri_or_path.removeprefix("file://")
    try:
        return str(Path(p).relative_to(ROOT))
    except ValueError:
        return p


def fmt_location(loc: dict) -> str:
    r = loc["range"]["start"]
    return f"{rel(loc['uri'])}:{r['line'] + 1}:{r['character'] + 1}"


def positions_to_probe(path: Path, line: int, col: int | None):
    """1-based -> 0-based; without a column, probe each identifier on the line."""
    if col is not None:
        yield line - 1, col - 1
        return
    text = path.read_text(errors="replace").splitlines()[line - 1]
    # right-to-left: on `CGameApp::CGameApp( )` the declared name - not the
    # return type - is what the caller means
    for m in reversed(list(re.finditer(r"[A-Za-z_][A-Za-z0-9_]*", text))):
        yield line - 1, m.start()


def at_point(method: str, path: Path, line: int, col: int | None, lsp: Clangd):
    lsp.open_file(path)
    for ln, ch in positions_to_probe(path, line, col):
        result = lsp._request(method, {
            "textDocument": {"uri": path.as_uri()},
            "position": {"line": ln, "character": ch},
            **({"context": {"includeDeclaration": True}}
               if method == "textDocument/references" else {}),
        })
        if result:
            return result
    return None


def _first_compdb_file() -> Path:
    entry = json.loads(CDB.read_text())[0]
    p = Path(entry["file"])
    if not p.is_absolute():
        p = (Path(entry.get("directory", ROOT)) / p)
    return p


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("index")
    sub.add_parser("symbol").add_argument("query")
    for name in ("def", "refs", "hover"):
        p = sub.add_parser(name)
        p.add_argument("file")
        p.add_argument("line", type=int)
        p.add_argument("col", type=int, nargs="?")
    args = ap.parse_args()

    # Gitignored, so absent on fresh clones/worktrees; without it clangd
    # falls back to a generic command and every query silently degrades.
    if not CDB.is_file():
        sys.exit("[clangd-query] build/clangd/compile_commands.json not found - "
                 "run `python3 scripts/gen_clangd.py` (inside `nix develop .#build`)")

    lsp = Clangd()
    try:
        if args.cmd == "index":
            # CDB discovery (and thus background indexing) starts lazily on
            # the first didOpen - kick it with the compdb's first entry.
            lsp.open_file(_first_compdb_file())
            lsp.wait_for_index()
            return

        if args.cmd == "symbol":
            # same lazy-CDB kick as `index`: shards load only after a didOpen,
            # and loading the shards takes a moment - retry briefly
            lsp.open_file(_first_compdb_file())
            for _ in range(10):
                syms = lsp._request("workspace/symbol", {"query": args.query}) or []
                if syms:
                    break
                time.sleep(2)
            for s in syms:
                container = s.get("containerName") or ""
                print(f"{container}{'::' if container else ''}{s['name']}  "
                      f"{fmt_location(s['location'])}")
            if not syms:
                print("(no symbols - background index may still be building)")
            return

        path = (ROOT / args.file).resolve()
        if not path.is_file():
            sys.exit(f"no such file: {args.file}")
        method = {"def": "textDocument/definition",
                  "refs": "textDocument/references",
                  "hover": "textDocument/hover"}[args.cmd]
        result = at_point(method, path, args.line, args.col, lsp)
        if not result:
            print("(no result)")
        elif args.cmd == "hover":
            print(result["contents"]["value"])
        else:
            for loc in result if isinstance(result, list) else [result]:
                print(fmt_location(loc))
    finally:
        lsp.close()


if __name__ == "__main__":
    main()
