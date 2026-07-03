#!/usr/bin/env python3
"""gruntz.analysis.rename_member - type-aware bulk member renamer via clangd LSP.

Renames a class's data members across the WHOLE tree using clangd's
`textDocument/rename`, which returns a complete WorkspaceEdit - the declaration
plus EVERY reference in every TU, in every syntactic form (member access,
offsetof, `&C::m_x`, designated init) - in one call. Because rename keys on the
symbol's identity (USR), it renames ONLY the named class's member and NEVER a
same-named field of a different struct. That type-awareness is the whole point:
dozens of unrelated classes reuse names like `m_5c`, and a text sed would wreck
them.

  # positional old=new pairs
  python3 -m gruntz.analysis.rename_member --class CGameObject m_5c=m_screenX m_60=m_screenY

  # or a mapping file (one `old=new` or `old new` per line, # comments allowed)
  python3 -m gruntz.analysis.rename_member --class CGameObject --map cgo.map

  # preview only (no writes): per-field + per-file edit counts
  python3 -m gruntz.analysis.rename_member --class CGameObject --map cgo.map --dry-run

The class's field decls are found in its header (auto-located under include/, or
pass --header). For each old->new pair the tool locates the member decl's
identifier, asks clangd to rename it, verifies every returned edit currently
reads the OLD name, and applies the merged edits bottom-to-top per file so
positions stay valid. Only OUR files (src/**, include/**) are written; vendor/
and the SDK headers are skipped (and reported if clangd ever targets them).

All fields are renamed against clangd's ORIGINAL in-memory buffers (nothing is
written until every rename is collected), so a batch is internally consistent
and the tool is idempotent - a re-run finds the (already-renamed) old names
absent and skips them.

Cross-file rename needs clangd's background index built; the tool blocks until
indexing settles before the first rename. The wine `cl` build is the safety
net: if clangd's index missed a site, its decl was still renamed, so the stale
access fails to compile (`m_old is not a member of C`) - re-run with a warm
index, or use --audit to list residual member accesses via clang-query.

Must be run inside `nix develop .#build` (clangd + python on PATH) from the
worktree root (so --compile-commands-dir and .cache/clangd resolve there).
"""
from __future__ import annotations

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


# ---------------------------------------------------------------------------
# clangd JSON-RPC driver (Content-Length framed JSON over stdio). Modeled on
# gruntz.analysis.clangd_query, extended with rename + workspace-edit caps.
# ---------------------------------------------------------------------------
class Clangd:
    def __init__(self):
        self.proc = subprocess.Popen(
            ["clangd", "--background-index", "--log=error",
             "--header-insertion=never",
             f"--compile-commands-dir={CDB_DIR}"],
            cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        self._id = 0
        self._request("initialize", {
            "processId": os.getpid(),
            "rootUri": ROOT.as_uri(),
            "capabilities": {
                "window": {"workDoneProgress": True},
                "workspace": {
                    # advertise documentChanges so clangd may return either
                    # form; we normalize both below
                    "workspaceEdit": {"documentChanges": True},
                    "symbol": {},
                },
                "textDocument": {
                    "rename": {"prepareSupport": True,
                               "dynamicRegistration": False},
                    "documentSymbol": {
                        "hierarchicalDocumentSymbolSupport": True},
                },
            },
        })
        self._notify("initialized", {})

    def _send(self, payload: dict) -> None:
        body = json.dumps({"jsonrpc": "2.0", **payload}).encode()
        self.proc.stdin.write(f"Content-Length: {len(body)}\r\n\r\n".encode() + body)
        self.proc.stdin.flush()

    def _recv(self) -> dict:
        headers = b""
        while not headers.endswith(b"\r\n\r\n"):
            chunk = self.proc.stdout.read(1)
            if not chunk:
                sys.exit("clangd: stream closed unexpectedly")
            headers += chunk
        length = int(re.search(rb"Content-Length: (\d+)", headers).group(1))
        return json.loads(self.proc.stdout.read(length))

    def _notify(self, method: str, params: dict) -> None:
        self._send({"method": method, "params": params})

    def _request(self, method: str, params: dict, timeout: float = 180.0):
        self._id += 1
        self._send({"id": self._id, "method": method, "params": params})
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            msg = self._recv()
            # server may interleave a create-progress request we must answer
            if msg.get("method") == "window/workDoneProgress/create":
                self._send({"id": msg["id"], "result": None})
                continue
            if msg.get("id") == self._id:
                if "error" in msg:
                    sys.exit(f"clangd error ({method}): {msg['error'].get('message')}")
                return msg.get("result")
        sys.exit(f"clangd: no reply to {method} within {timeout}s")

    def wait_for_index(self) -> None:
        """Block until background indexing reports 'end' (or stays idle)."""
        begun = False
        last = time.monotonic()
        while True:
            ready, _, _ = select.select([self.proc.stdout], [], [], 5.0)
            if not ready:
                # pre-begin clangd quietly validates existing shards; be patient
                if time.monotonic() - last > (30.0 if begun else 90.0):
                    print("index: idle - nothing (left) to index" if not begun
                          else "\nindex: no progress for 30s - assuming done",
                          file=sys.stderr)
                    return
                continue
            msg = self._recv()
            last = time.monotonic()
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
                txt = value.get("message", "")
                print(f"\rindex: {pct}% {txt[:60]:<60}", end="", file=sys.stderr,
                      flush=True)
            elif value.get("kind") == "end" and begun:
                print("\nindex: done", file=sys.stderr)
                return

    def open_file(self, path: Path) -> None:
        self._notify("textDocument/didOpen", {"textDocument": {
            "uri": path.as_uri(), "languageId": "cpp", "version": 1,
            "text": path.read_text(errors="replace"),
        }})

    def rename(self, path: Path, line: int, char: int, new_name: str):
        """0-based line/char. Returns a WorkspaceEdit or None."""
        return self._request("textDocument/rename", {
            "textDocument": {"uri": path.as_uri()},
            "position": {"line": line, "character": char},
            "newName": new_name,
        })

    def close(self) -> None:
        try:
            self._request("shutdown", {}, timeout=5)
            self._notify("exit", {})
        except Exception:  # noqa: BLE001 - best-effort teardown
            pass
        self.proc.terminate()


def _first_compdb_file() -> Path:
    entry = json.loads(CDB.read_text())[0]
    p = Path(entry["file"])
    if not p.is_absolute():
        p = Path(entry.get("directory", ROOT)) / p
    return p


# ---------------------------------------------------------------------------
# Source helpers: mask comments/strings, locate a class body + a field decl.
# ---------------------------------------------------------------------------
def mask_noncode(text: str) -> str:
    """Return a copy the same length as `text` with comment/string/char spans
    replaced by spaces (newlines preserved), so brace-matching and identifier
    scans ignore braces/names inside comments and literals. Positions are
    preserved 1:1 for offset->line/col conversion."""
    n = len(text)
    out = list(text)
    i = 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            k = i
            while k < n and text[k] != "\n":
                out[k] = " "
                k += 1
            i = k
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            k = i
            while k < n and not (text[k] == "*" and k + 1 < n and text[k + 1] == "/"):
                if text[k] != "\n":
                    out[k] = " "
                k += 1
            end = min(k + 2, n)
            for p in range(k, end):
                if text[p] != "\n":
                    out[p] = " "
            i = end
            continue
        if c in "\"'":
            quote = c
            k = i + 1
            out[i] = " "
            while k < n:
                if text[k] == "\\" and k + 1 < n:
                    out[k] = out[k + 1] = " "
                    k += 2
                    continue
                if text[k] == quote:
                    out[k] = " "
                    k += 1
                    break
                if text[k] != "\n":
                    out[k] = " "
                k += 1
            i = k
            continue
        i += 1
    return "".join(out)


def find_class_block(text: str, masked: str, class_name: str):
    """Locate the DEFINITION (has a body) of struct/class `class_name`. Returns
    (open_brace_offset, close_brace_offset_exclusive) or None. Forward decls and
    SIZE_UNKNOWN(...) mentions (no `{`) are skipped."""
    pat = re.compile(r"\b(?:struct|class)\s+" + re.escape(class_name)
                     + r"\b(?:\s*:\s*[^{;]+)?\s*\{")
    for m in pat.finditer(masked):
        brace = masked.find("{", m.start())
        if brace < 0:
            continue
        depth = 0
        i = brace
        n = len(masked)
        while i < n:
            ch = masked[i]
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    return brace, i + 1
            i += 1
    return None


def offset_to_linecol(text: str, offset: int):
    """0-based (line, character) for a byte offset (ASCII source: byte==col)."""
    line = text.count("\n", 0, offset)
    line_start = text.rfind("\n", 0, offset) + 1
    return line, offset - line_start


def find_field_decl(text: str, masked: str, block, field: str):
    """Within the class block, find the field's DECLARATION identifier position.
    A decl is `<type> field [array];` - the name is directly followed (ignoring
    ws) by `[`, `:` (bitfield) or `;`. Returns (line, col) 0-based, or None."""
    start, end = block
    ident = re.compile(r"\b" + re.escape(field) + r"\s*(?:\[[^\]]*\])?\s*(?::|;)")
    hits = [m for m in ident.finditer(masked, start, end)]
    if not hits:
        return None
    if len(hits) > 1:
        print(f"  warn: {field}: {len(hits)} decl-like matches in class body; "
              f"using the first", file=sys.stderr)
    return offset_to_linecol(text, hits[0].start())


# ---------------------------------------------------------------------------
# WorkspaceEdit normalization + application.
# ---------------------------------------------------------------------------
def uri_to_path(uri: str) -> Path:
    p = uri
    if p.startswith("file://"):
        p = p[len("file://"):]
    return Path(p)


def normalize_edits(ws_edit) -> dict:
    """WorkspaceEdit -> {Path: [TextEdit,...]} handling both `changes` and
    `documentChanges` response shapes."""
    out: dict[Path, list] = {}
    if not ws_edit:
        return out
    if "changes" in ws_edit:
        for uri, edits in ws_edit["changes"].items():
            out.setdefault(uri_to_path(uri), []).extend(edits)
    for dc in ws_edit.get("documentChanges", []) or []:
        uri = dc["textDocument"]["uri"]
        out.setdefault(uri_to_path(uri), []).extend(dc.get("edits", []))
    return out


def line_starts(text: str):
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    return starts


def is_ours(path: Path) -> bool:
    try:
        rel = path.resolve().relative_to(ROOT)
    except ValueError:
        return False
    parts = rel.parts
    if not parts or parts[0] not in ("src", "include"):
        return False
    if "vendor" in parts:
        return False
    return True


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--class", dest="cls", required=True,
                    help="class/struct whose members to rename")
    ap.add_argument("--header", default=None,
                    help="header holding the class def (default: auto-locate under include/)")
    ap.add_argument("--map", default=None,
                    help="mapping file: `old=new` or `old new` per line (# comments)")
    ap.add_argument("pairs", nargs="*", help="inline old=new pairs")
    ap.add_argument("--dry-run", action="store_true",
                    help="print edit counts, write nothing")
    ap.add_argument("--audit", action="store_true",
                    help="after (or instead of) rename, run clang-query to list "
                         "residual member accesses of the OLD names")
    args = ap.parse_args(argv)

    if not CDB.is_file():
        sys.exit("[rename_member] build/clangd/compile_commands.json not found - "
                 "run `gruntz clangd` inside `nix develop .#build`")

    # --- parse the mapping (file + inline pairs) -------------------------------
    mapping: list[tuple[str, str]] = []
    seen = set()

    def add_pair(old: str, new: str):
        if old in seen:
            print(f"  warn: duplicate old field {old}; ignoring later", file=sys.stderr)
            return
        seen.add(old)
        mapping.append((old, new))

    if args.map:
        for raw in Path(args.map).read_text().splitlines():
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = re.split(r"[=\s]+", line)
            if len(parts) != 2:
                sys.exit(f"[rename_member] bad map line: {raw!r}")
            add_pair(parts[0], parts[1])
    for p in args.pairs:
        if "=" not in p:
            sys.exit(f"[rename_member] inline pair must be old=new: {p!r}")
        old, new = p.split("=", 1)
        add_pair(old.strip(), new.strip())
    if not mapping:
        sys.exit("[rename_member] no old=new pairs given (use --map or positional)")

    # --- locate the class header + field decl positions ------------------------
    if args.header:
        header = (ROOT / args.header).resolve() if not Path(args.header).is_absolute() \
            else Path(args.header)
    else:
        header = None
        rx = re.compile(r"\b(?:struct|class)\s+" + re.escape(args.cls) + r"\b[^;{]*\{")
        for h in sorted((ROOT / "include").rglob("*.h")):
            m = mask_noncode(h.read_text(errors="replace"))
            if rx.search(m):
                header = h
                break
        if header is None:
            sys.exit(f"[rename_member] could not auto-locate a header defining "
                     f"{args.cls} under include/ (pass --header)")
    print(f"class {args.cls}: header {header.relative_to(ROOT)}", file=sys.stderr)

    htext = header.read_text()
    hmask = mask_noncode(htext)
    block = find_class_block(htext, hmask, args.cls)
    if block is None:
        sys.exit(f"[rename_member] no definition (with body) of {args.cls} in {header}")

    positions = {}
    missing = []
    for old, new in mapping:
        pos = find_field_decl(htext, hmask, block, old)
        if pos is None:
            missing.append(old)
            print(f"  skip {old}: no decl in {args.cls} body (already renamed?)",
                  file=sys.stderr)
            continue
        positions[old] = (pos, new)

    if args.audit:
        run_audit(args.cls, [old for old, _ in mapping])
        if not positions:
            return 0

    if not positions:
        print("nothing to rename (all fields missing).")
        return 1

    # --- drive clangd: build the index, then rename every field ----------------
    lsp = Clangd()
    per_file: dict[Path, list] = {}       # merged TextEdits to apply
    per_field_stats = {}                  # old -> (files, sites)
    skipped_foreign = set()
    try:
        # kick lazy CDB discovery -> background indexing, then wait it out
        lsp.open_file(_first_compdb_file())
        lsp.open_file(header)
        print("index: waiting for background index to settle "
              "(cross-file rename needs it)...", file=sys.stderr)
        lsp.wait_for_index()

        for old, ((line, col), new) in positions.items():
            ws = lsp.rename(header, line, col, new)
            edits = normalize_edits(ws)
            if not edits:
                print(f"  warn: {old}->{new}: clangd returned NO edits "
                      f"(index gap? decl position wrong?)", file=sys.stderr)
                continue
            nfiles = nsites = 0
            for path, tedits in edits.items():
                if not is_ours(path):
                    skipped_foreign.add(str(path))
                    continue
                # verify each edit currently reads the OLD name (guards against a
                # mis-resolved symbol renaming the wrong token)
                content = path.read_text()
                starts = line_starts(content)
                bad = 0
                for e in tedits:
                    r = e["range"]
                    s = starts[r["start"]["line"]] + r["start"]["character"]
                    t = starts[r["end"]["line"]] + r["end"]["character"]
                    if content[s:t] != old:
                        bad += 1
                if bad:
                    sys.exit(f"[rename_member] {old}->{new}: {bad} edit(s) in "
                             f"{path} do NOT read {old!r} - aborting (stale index?)")
                per_file.setdefault(path, []).extend(
                    (r0["range"], r0["newText"], old) for r0 in tedits)
                nfiles += 1
                nsites += len(tedits)
            per_field_stats[old] = (nfiles, nsites, new)
            print(f"  {old} -> {new}: {nsites} sites in {nfiles} files")
    finally:
        lsp.close()

    if skipped_foreign:
        print("NOTE: clangd targeted non-src/include files (skipped):",
              file=sys.stderr)
        for f in sorted(skipped_foreign):
            print(f"  {f}", file=sys.stderr)

    # --- apply merged edits, bottom-to-top per file ----------------------------
    total_sites = sum(v[1] for v in per_field_stats.values())
    print(f"\ntotal: {total_sites} sites across {len(per_file)} files "
          f"({len(per_field_stats)} fields renamed)")

    if args.dry_run:
        print("[dry-run] no files written.")
        return 0

    for path, edits in per_file.items():
        content = path.read_text()
        starts = line_starts(content)
        resolved = []
        for rng, new_text, old in edits:
            s = starts[rng["start"]["line"]] + rng["start"]["character"]
            t = starts[rng["end"]["line"]] + rng["end"]["character"]
            resolved.append((s, t, new_text))
        resolved.sort(key=lambda x: x[0], reverse=True)
        # overlap guard
        prev_start = None
        for s, t, _ in resolved:
            if prev_start is not None and t > prev_start:
                sys.exit(f"[rename_member] overlapping edits in {path} - aborting")
            prev_start = s
        for s, t, new_text in resolved:
            content = content[:s] + new_text + content[t:]
        path.write_text(content)
    print(f"wrote {len(per_file)} files.")
    return 0


# ---------------------------------------------------------------------------
# Optional clang-query audit/fallback: list type-scoped member accesses so a
# clangd index gap can be spotted (and hand-fixed if ever needed).
# ---------------------------------------------------------------------------
def run_audit(cls: str, fields: list[str]) -> None:
    db = json.loads(CDB.read_text())
    tus = [e["file"] for e in db if e["file"].startswith("src/")
           or e["file"].startswith("include/")]
    print(f"[audit] clang-query over {len(tus)} TUs for {cls} member accesses",
          file=sys.stderr)
    for field in fields:
        matcher = (
            f'match memberExpr(member(hasName("{field}")), hasObjectExpression('
            f'anyOf(hasType(pointsTo(recordDecl(hasName("{cls}")))),'
            f'hasType(recordDecl(hasName("{cls}")))))).bind("x")'
        )
        try:
            proc = subprocess.run(
                ["clang-query", "-p", str(CDB_DIR), "-c", matcher, *tus, "--"],
                cwd=ROOT, capture_output=True, text=True, timeout=600)
        except FileNotFoundError:
            print("[audit] clang-query not on PATH (need nix develop .#build)",
                  file=sys.stderr)
            return
        sites = sorted(set(re.findall(r"(\S+:\d+:\d+): note: \"x\" binds",
                                      proc.stdout)))
        print(f"  {cls}::{field}: {len(sites)} memberExpr access sites")
        for s in sites:
            print(f"    {s}")


if __name__ == "__main__":
    sys.exit(main())
