#!/usr/bin/env python3
"""clangd-backed identifier rename/audit helper for reconstructed src/.

Examples:
  python -m gruntz.analysis.rename_identifiers audit
  python -m gruntz.analysis.rename_identifiers rename src/Gruntz/CPlay.cpp 1311 bottom textBottom
  python -m gruntz.analysis.rename_identifiers batch renames.json

The `rename` command finds the named identifier on the given 1-based line and
delegates all edits to clangd's semantic `textDocument/rename` implementation.
When renaming members, keep the repository convention: member names start with
`m_`. The `audit` command is intentionally conservative: it only reports generic
source-level names that still need human judgement.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
CDB_DIR = ROOT / "build" / "clangd"
CDB = CDB_DIR / "compile_commands.json"

GENERIC_RE = re.compile(
    r"\b("
    r"a[0-9][0-9a-f]{0,2}|"
    r"b[0-9][0-9a-f]{0,2}|"
    r"c[0-9][0-9a-f]{0,2}|"
    r"m_[0-9a-f]{1,4}|"
    r"field_[0-9a-f]{1,4}|"
    r"unk(?:nown)?[A-Za-z0-9_]*|"
    r"ClassUnknown_[A-Za-z0-9_]*|"
    r"FUN_[0-9A-Fa-f]+|"
    r"DAT_[0-9A-Fa-f]+|"
    r"LAB_[0-9A-Fa-f]+"
    r")\b"
)

DECL_HINT_RE = re.compile(
    r"^\s*(?:struct|class|union)\s+(?P<record>[A-Za-z_][A-Za-z0-9_]*)\b|"
    r"^\s*(?:(?:const|volatile|static|extern|register|signed|unsigned|struct|class)\s+)*"
    r"(?:[A-Za-z_][A-Za-z0-9_:<>]*[\s*&]+)+(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
    r"(?:\s*(?:\[|=|,|;|\)|\{))"
)


class Clangd:
    def __init__(self) -> None:
        if not CDB.is_file():
            sys.exit("[rename-identifiers] build/clangd/compile_commands.json not found - "
                     "run `gruntz clangd` inside the build shell")
        self.proc = subprocess.Popen(
            ["clangd", "--background-index", "--log=error",
             f"--compile-commands-dir={CDB_DIR}"],
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        self._id = 0
        self._request("initialize", {
            "processId": os.getpid(),
            "rootUri": ROOT.as_uri(),
            "capabilities": {
                "workspace": {"applyEdit": True, "workspaceEdit": {"documentChanges": True}},
                "textDocument": {"rename": {"prepareSupport": True}},
            },
        })
        self._notify("initialized", {})

    def _send(self, payload: dict) -> None:
        assert self.proc.stdin is not None
        body = json.dumps({"jsonrpc": "2.0", **payload}).encode()
        self.proc.stdin.write(f"Content-Length: {len(body)}\r\n\r\n".encode() + body)
        self.proc.stdin.flush()

    def _recv(self) -> dict:
        assert self.proc.stdout is not None
        headers = b""
        while not headers.endswith(b"\r\n\r\n"):
            b = self.proc.stdout.read(1)
            if not b:
                sys.exit("clangd exited while waiting for a response")
            headers += b
        length = int(re.search(rb"Content-Length: (\d+)", headers).group(1))
        return json.loads(self.proc.stdout.read(length))

    def _notify(self, method: str, params: dict) -> None:
        self._send({"method": method, "params": params})

    def _request(self, method: str, params: dict, timeout_messages: int = 10000):
        self._id += 1
        request_id = self._id
        self._send({"id": request_id, "method": method, "params": params})
        for _ in range(timeout_messages):
            msg = self._recv()
            if msg.get("method") == "workspace/applyEdit":
                self._send({"id": msg["id"], "result": {"applied": True}})
                continue
            if msg.get("id") == request_id:
                if "error" in msg:
                    message = msg["error"].get("message", msg["error"])
                    raise RuntimeError(f"clangd {method} error: {message}")
                return msg.get("result")
        raise RuntimeError(f"clangd: no reply to {method}")

    def open_file(self, path: Path) -> None:
        self._notify("textDocument/didOpen", {"textDocument": {
            "uri": path.as_uri(),
            "languageId": "cpp",
            "version": 1,
            "text": path.read_text(errors="replace"),
        }})

    def rename(self, path: Path, line0: int, col0: int, new_name: str) -> dict:
        self.open_file(path)
        params = {"textDocument": {"uri": path.as_uri()},
                  "position": {"line": line0, "character": col0}}
        self._request("textDocument/prepareRename", params)
        return self._request("textDocument/rename", {**params, "newName": new_name}) or {}

    def close(self) -> None:
        try:
            self._request("shutdown", {}, timeout_messages=100)
            self._notify("exit", {})
        finally:
            self.proc.terminate()


@dataclass(frozen=True)
class Edit:
    path: Path
    start_line: int
    start_col: int
    end_line: int
    end_col: int
    text: str


def _path_from_uri(uri: str) -> Path:
    prefix = "file://"
    if not uri.startswith(prefix):
        raise ValueError(f"unsupported URI: {uri}")
    return Path(uri[len(prefix):])


def _iter_edits(workspace_edit: dict) -> Iterable[Edit]:
    for change in workspace_edit.get("documentChanges", []):
        path = _path_from_uri(change["textDocument"]["uri"])
        for edit in change.get("edits", []):
            r = edit["range"]
            yield Edit(path, r["start"]["line"], r["start"]["character"],
                       r["end"]["line"], r["end"]["character"], edit["newText"])
    for uri, edits in workspace_edit.get("changes", {}).items():
        path = _path_from_uri(uri)
        for edit in edits:
            r = edit["range"]
            yield Edit(path, r["start"]["line"], r["start"]["character"],
                       r["end"]["line"], r["end"]["character"], edit["newText"])


def apply_workspace_edit(workspace_edit: dict) -> int:
    edits_by_path: dict[Path, list[Edit]] = {}
    for edit in _iter_edits(workspace_edit):
        try:
            edit.path.relative_to(ROOT)
        except ValueError:
            raise RuntimeError(f"refusing to edit outside repository: {edit.path}") from None
        edits_by_path.setdefault(edit.path, []).append(edit)

    count = 0
    for path, edits in edits_by_path.items():
        text = path.read_text(errors="replace")
        lines = text.splitlines(keepends=True)
        offsets = [0]
        for line in lines:
            offsets.append(offsets[-1] + len(line))

        def absolute(line: int, col: int) -> int:
            return offsets[line] + col

        for edit in sorted(edits, key=lambda e: (e.start_line, e.start_col), reverse=True):
            start = absolute(edit.start_line, edit.start_col)
            end = absolute(edit.end_line, edit.end_col)
            text = text[:start] + edit.text + text[end:]
            count += 1
        path.write_text(text)
    return count


def identifier_column(path: Path, line: int, old_name: str) -> int:
    text = path.read_text(errors="replace").splitlines()[line - 1]
    for m in re.finditer(r"[A-Za-z_][A-Za-z0-9_]*", text):
        if m.group(0) == old_name:
            return m.start()
    sys.exit(f"{path.relative_to(ROOT)}:{line}: identifier `{old_name}` not found")


def cmd_rename(args: argparse.Namespace) -> None:
    path = (ROOT / args.file).resolve()
    col = identifier_column(path, args.line, args.old_name)
    lsp = Clangd()
    try:
        edit = lsp.rename(path, args.line - 1, col, args.new_name)
    finally:
        lsp.close()
    count = apply_workspace_edit(edit)
    print(f"renamed {args.old_name} -> {args.new_name}: {count} edit(s)")


def cmd_batch(args: argparse.Namespace) -> None:
    specs = json.loads(Path(args.file).read_text())
    lsp = Clangd()
    total = 0
    try:
        for spec in specs:
            path = (ROOT / spec["file"]).resolve()
            col = identifier_column(path, int(spec["line"]), spec["old"])
            edit = lsp.rename(path, int(spec["line"]) - 1, col, spec["new"])
            count = apply_workspace_edit(edit)
            total += count
            print(f"{spec['file']}:{spec['line']}: {spec['old']} -> {spec['new']} ({count})")
    finally:
        lsp.close()
    print(f"total edits: {total}")


def _audit_file(path: Path, *, include_stubs: bool) -> list[tuple[int, str, str]]:
    findings: list[tuple[int, str, str]] = []
    lines = path.read_text(errors="replace").splitlines()
    in_stub_comment = False
    for idx, line in enumerate(lines, start=1):
        stripped = line.strip()
        if "@stub" in stripped:
            in_stub_comment = True
        if not include_stubs and in_stub_comment and "RVA(" in stripped:
            in_stub_comment = False
            continue
        if stripped.startswith("//") or stripped.startswith("#"):
            continue
        if "__asm" in line or re.match(r"\s*(mov|push|pop|call|jmp|cmp|add|sub|lea)\b", stripped):
            continue
        for match in GENERIC_RE.finditer(line):
            name = match.group(1)
            hint = "use clangd rename if the recovered source gives a meaning"
            decl = DECL_HINT_RE.match(line)
            if decl:
                if decl.group("record") == name:
                    hint = "generic record name"
                elif decl.group("name") == name:
                    hint = "generic local/parameter/member declaration"
            findings.append((idx, name, hint))
    return findings


def cmd_audit(args: argparse.Namespace) -> None:
    roots = [ROOT / p for p in (args.paths or ["src"])]
    total = 0
    for root in roots:
        files = [root] if root.is_file() else sorted(root.rglob("*.cpp"))
        for path in files:
            if not args.include_stubs and "/src/Stub/" in str(path):
                continue
            findings = _audit_file(path, include_stubs=args.include_stubs)
            if not findings:
                continue
            for line, name, hint in findings:
                print(f"{path.relative_to(ROOT)}:{line}: {name}: {hint}")
                total += 1
    print(f"generic identifier hits: {total}", file=sys.stderr)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    audit = sub.add_parser("audit")
    audit.add_argument("paths", nargs="*")
    audit.add_argument("--include-stubs", action="store_true")
    audit.set_defaults(func=cmd_audit)
    rename = sub.add_parser("rename")
    rename.add_argument("file")
    rename.add_argument("line", type=int)
    rename.add_argument("old_name")
    rename.add_argument("new_name")
    rename.set_defaults(func=cmd_rename)
    batch = sub.add_parser("batch")
    batch.add_argument("file")
    batch.set_defaults(func=cmd_batch)
    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
