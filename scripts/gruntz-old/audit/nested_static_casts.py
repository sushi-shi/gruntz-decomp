#!/usr/bin/env python3
"""Find directly nested ``static_cast`` expressions with libclang.

The text prefilter only decides which files are worth parsing. A finding exists
only when the AST contains a ``CXX_STATIC_CAST_EXPR`` whose operand, after
transparent parentheses/implicit wrappers, is another ``CXX_STATIC_CAST_EXPR``.
Adjacent casts in separate statements therefore do not count.

Run as ``python -m gruntz.audit.nested_static_casts`` to list every site. Each
finding prints the source, intermediate, and final types so a reviewer can tell
a redundant detour from a deliberate sign, width, or truncation transition.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import clang.cindex as cidx


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
CDB = REPO / "build" / "clangd" / "compile_commands.json"
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
STATIC_CAST = re.compile(r"\bstatic_cast\s*<")
STRICT_MARKER = "GZ_STRICT_ENUMS"
TRANSPARENT = {cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR}


def _cast_argument_open(text: str, start: int) -> int:
    """Find the argument ``(`` after a balanced ``static_cast<...>`` target."""
    target = text.find("<", start)
    if target < 0:
        return -1
    depth = 0
    for i in range(target, len(text)):
        if text[i] == "<":
            depth += 1
        elif text[i] == ">":
            depth -= 1
            if depth == 0:
                return text.find("(", i + 1)
    return -1


def _has_nested_spelling(text: str) -> bool:
    """Cheap, permissive prefilter; AST parsing makes the final decision."""
    starts = [match.start() for match in STATIC_CAST.finditer(text)]
    for start in starts:
        open_paren = _cast_argument_open(text, start)
        if open_paren < 0:
            continue
        depth = 0
        for i in range(open_paren, len(text)):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    if STATIC_CAST.search(text, open_paren + 1, i):
                        return True
                    break
    return False


def _flags(entry: dict, *, header: bool) -> list[str]:
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for arg in args[1:]:
        if (
            arg == "/c"
            or arg == src
            or arg.endswith(src)
            or arg == "-fdelayed-template-parsing"
        ):
            continue
        out.append(arg)
    # clang-cl otherwise follows MSVC's delayed-template mode and omits
    # uninstantiated bodies from the AST, hiding header-only cast chains.
    out.append("-fno-delayed-template-parsing")
    if header:
        out.append("/TP")
    return out


def _strict_flags(flags: list[str]) -> list[str]:
    """Select the repository's otherwise-inactive C++20 enum-model branch."""
    return [
        arg
        for arg in flags
        if not arg.startswith("-std=") and not arg.lower().startswith("/std:")
    ] + ["/std:c++20", "/Zc:__cplusplus"]


def _rel(cursor, repo: Path) -> str | None:
    file = cursor.location.file
    if file is None:
        return None
    try:
        return str(Path(file.name).resolve().relative_to(repo))
    except ValueError:
        return None


def _operand(node):
    children = list(node.get_children())
    if not children:
        return None
    probe = children[-1]
    for _ in range(12):
        if probe.kind not in TRANSPARENT:
            return probe
        children = list(probe.get_children())
        if not children:
            return probe
        probe = children[-1]
    return probe


def scan(repo: Path = REPO, cdb: Path = CDB) -> list[tuple[str, int, int, str, str, str]]:
    """Return ``(file,line,column,outer_type,inner_type,source_type)`` per pair."""
    repo, cdb = Path(repo), Path(cdb)
    if not cdb.is_file():
        raise FileNotFoundError(f"{cdb}: run gruntz init to create the compile database")
    entries = json.loads(cdb.read_text())
    by_source = {}
    for entry in entries:
        source = Path(entry["file"])
        source = source if source.is_absolute() else repo / source
        by_source[source.resolve()] = entry
    if not entries:
        raise RuntimeError(f"{cdb}: empty compile database")

    candidates = []
    for root in (repo / "src", repo / "include"):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix in SOURCE_SUFFIXES and path.is_file():
                try:
                    if _has_nested_spelling(path.read_text(errors="ignore")):
                        candidates.append(path.resolve())
                except OSError:
                    continue

    index = cidx.Index.create()
    hits = set()
    failed = []
    fallback = entries[0]
    candidate_names = {str(path.relative_to(repo)) for path in candidates}
    for path in candidates:
        entry = by_source.get(path, fallback)
        relpath = str(path.relative_to(repo))
        flags = _flags(entry, header=path.suffix in {".h", ".hpp", ".inl"})
        modes = [("retail", flags)]
        if STRICT_MARKER in path.read_text(errors="ignore"):
            modes.append(("strict", _strict_flags(flags)))
        for mode, mode_flags in modes:
            try:
                tu = index.parse(str(path), args=mode_flags)
            except cidx.TranslationUnitLoadError:
                failed.append(f"{relpath} ({mode})")
                continue
            stack = [tu.cursor]
            while stack:
                node = stack.pop()
                stack.extend(node.get_children())
                if node.kind != cidx.CursorKind.CXX_STATIC_CAST_EXPR:
                    continue
                inner = _operand(node)
                if inner is None or inner.kind != cidx.CursorKind.CXX_STATIC_CAST_EXPR:
                    continue
                source = _operand(inner)
                if source is None:
                    continue
                rel = _rel(node, repo)
                if rel not in candidate_names:
                    continue
                hits.add((rel, node.location.line, node.location.column,
                          node.type.spelling, inner.type.spelling, source.type.spelling))
    if failed:
        raise RuntimeError("libclang could not parse nested-cast candidate(s): "
                           + ", ".join(sorted(failed)))
    return sorted(hits)


def main() -> int:
    hits = scan()
    for path, line, column, outer, inner, source in hits:
        print(f"{path}:{line}:{column}: nested static_cast "
              f"({source} -> {inner} -> {outer})")
    print(f"nested static_casts: {len(hits)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
