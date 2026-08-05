#!/usr/bin/env python3
"""Inventory and review every explicit enum conversion in reconstructed C++.

The enum layer deliberately makes conversions visible.  Two spellings deserve
individual review because they often indicate that the producer or consumer has
the wrong type:

* ``IDX(expr)`` exits a domain to an integer.
* ``static_cast<Domain>(expr)`` enters a declared enum domain.

This module builds a durable occurrence ledger.  Rows do not disappear when a
conversion is removed: an absent pending row becomes ``missing`` and must still
be reviewed.  That prevents a broad rewrite from making unexamined work vanish.

    python -m gruntz.audit.enum_conversions --sync-review
    python -m gruntz.audit.enum_conversions --summary
    python -m gruntz.audit.enum_conversions --gate
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import re
import sys
from dataclasses import dataclass
from pathlib import Path


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
REVIEW = REPO / "config" / "cleanliness" / "enum-conversion-review.tsv"
SOURCE_SUFFIXES = {".cpp", ".h"}
STATES = {"pending", "missing", "fixed", "reviewed"}

DOMAIN_DECL = re.compile(
    r"\bGZ_ENUM_(?:BEGIN|BEGIN_SPLIT|FLAGS_BEGIN|FORWARD|FORWARD_SPLIT)"
    r"\(\s*([A-Za-z_]\w*)")
BARE_ENUM_DECL = re.compile(r"\b(?:typedef\s+)?enum\s+([A-Za-z_]\w*)\b")
DOMAIN_BLOCK = re.compile(
    r"\bGZ_ENUM_(?:BEGIN|BEGIN_SPLIT|FLAGS_BEGIN)"
    r"\(\s*([A-Za-z_]\w*)\s*(?:,\s*[A-Za-z_]\w*\s*)?\)"
    r"(?P<body>.*?)\bGZ_ENUM_(?:END|END_SPLIT|FLAGS_END)\(", re.S)
ENUMERATOR = re.compile(r"^[ \t]*([A-Z][A-Z0-9_]*)\s*=", re.M)
STATIC_CAST = re.compile(
    r"\bstatic_cast\s*<\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*>\s*\(")
IDX = re.compile(r"\bIDX\s*\(")


@dataclass
class Finding:
    key: str
    state: str
    path: str
    line: int
    kind: str
    domain: str
    expression: str
    context: str
    notes: str = ""


def sources() -> list[Path]:
    return sorted(
        path
        for root in (REPO / "include", REPO / "src")
        for path in root.rglob("*")
        if path.suffix in SOURCE_SUFFIXES
    )


def mask_comments(text: str) -> str:
    """Blank comments without changing offsets or line numbers."""
    out = list(text)
    i = 0
    quote = ""
    while i < len(text):
        c = text[i]
        if quote:
            if c == "\\":
                i += 2
                continue
            if c == quote:
                quote = ""
            i += 1
            continue
        if c in ('"', "'"):
            quote = c
            i += 1
            continue
        if text.startswith("//", i):
            end = text.find("\n", i)
            if end < 0:
                end = len(text)
            for j in range(i, end):
                out[j] = " "
            i = end
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = len(text) if end < 0 else end + 2
            for j in range(i, end):
                if out[j] != "\n":
                    out[j] = " "
            i = end
            continue
        i += 1
    return "".join(out)


def balanced_argument(text: str, open_paren: int) -> tuple[str, int] | None:
    depth = 0
    quote = ""
    i = open_paren
    while i < len(text):
        c = text[i]
        if quote:
            if c == "\\":
                i += 2
                continue
            if c == quote:
                quote = ""
            i += 1
            continue
        if c in ('"', "'"):
            quote = c
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return text[open_paren + 1:i], i + 1
        i += 1
    return None


def normalize(value: str) -> str:
    return " ".join(value.replace("\t", " ").split())


def line_context(text: str, offset: int) -> str:
    start = text.rfind("\n", 0, offset) + 1
    end = text.find("\n", offset)
    if end < 0:
        end = len(text)
    return normalize(text[start:end])


def catalog(paths: list[Path]) -> tuple[set[str], dict[str, str]]:
    domains: set[str] = set()
    enumerators: dict[str, str] = {}
    for path in paths:
        text = mask_comments(path.read_text(errors="replace"))
        domains.update(DOMAIN_DECL.findall(text))
        domains.update(BARE_ENUM_DECL.findall(text))
        for block in DOMAIN_BLOCK.finditer(text):
            domain = block.group(1)
            for name in ENUMERATOR.findall(block.group("body")):
                enumerators.setdefault(name, domain)
    return domains, enumerators


def make_key(
    kind: str,
    path: str,
    domain: str,
    expression: str,
    context: str,
    ordinal: int,
) -> str:
    raw = "\0".join(
        (kind, path, domain, normalize(expression), normalize(context), str(ordinal))
    )
    return hashlib.sha1(raw.encode()).hexdigest()[:16]


def scan() -> list[Finding]:
    paths = sources()
    domains, enumerators = catalog(paths)
    findings: list[Finding] = []
    repeats: dict[tuple[str, str, str, str, str], int] = {}

    for source in paths:
        rel = str(source.relative_to(REPO))
        original = source.read_text(errors="replace")
        text = mask_comments(original)
        raw: list[tuple[int, str, str, str]] = []

        for match in IDX.finditer(text):
            parsed = balanced_argument(text, match.end() - 1)
            if parsed is None:
                continue
            expression, _end = parsed
            mentioned = sorted({enumerators[name]
                                for name in re.findall(r"\b[A-Z][A-Z0-9_]*\b", expression)
                                if name in enumerators})
            raw.append((match.start(), "IDX", ",".join(mentioned), normalize(expression)))

        for match in STATIC_CAST.finditer(text):
            target = match.group(1)
            leaf = target.rsplit("::", 1)[-1]
            if leaf not in domains:
                continue
            parsed = balanced_argument(text, match.end() - 1)
            if parsed is None:
                continue
            expression, _end = parsed
            raw.append((match.start(), "static_cast-enum", target, normalize(expression)))

        for offset, kind, domain, expression in sorted(raw):
            context = line_context(original, offset)
            repeated = (kind, rel, domain, expression, context)
            ordinal = repeats.get(repeated, 0) + 1
            repeats[repeated] = ordinal
            findings.append(Finding(
                key=make_key(kind, rel, domain, expression, context, ordinal),
                state="pending",
                path=rel,
                line=text.count("\n", 0, offset) + 1,
                kind=kind,
                domain=domain,
                expression=expression,
                context=context,
            ))
    return findings


def read_review() -> list[Finding]:
    if not REVIEW.exists():
        return []
    with REVIEW.open(newline="") as stream:
        rows = csv.DictReader(stream, delimiter="\t")
        return [Finding(
            key=row["id"], state=row["state"], path=row["path"],
            line=int(row["line"]), kind=row["kind"], domain=row["domain"],
            expression=row["expression"], context=row["context"],
            notes=row.get("notes", ""),
        ) for row in rows]


def write_review(rows: list[Finding]) -> None:
    fields = ("id", "state", "path", "line", "kind", "domain", "expression", "context", "notes")
    buffer = io.StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=fields, delimiter="\t", lineterminator="\n")
    writer.writeheader()
    for row in rows:
        writer.writerow({
            "id": row.key, "state": row.state, "path": row.path, "line": row.line,
            "kind": row.kind, "domain": row.domain, "expression": row.expression,
            "context": row.context, "notes": row.notes,
        })
    REVIEW.write_text(buffer.getvalue())


def sync_review() -> list[Finding]:
    old = {row.key: row for row in read_review()}
    old_by_signature: dict[tuple[str, str, str, str, str], list[Finding]] = {}
    for row in old.values():
        signature = (row.kind, row.path, row.domain, row.expression, row.context)
        old_by_signature.setdefault(signature, []).append(row)
    current = scan()
    current_ids = {row.key for row in current}
    consumed_old: set[str] = set()
    merged: list[Finding] = []
    for row in current:
        previous = old.get(row.key)
        if previous is None:
            signature = (row.kind, row.path, row.domain, row.expression, row.context)
            candidates = old_by_signature.get(signature, [])
            previous = next(
                (candidate for candidate in candidates if candidate.key not in consumed_old),
                None,
            )
        if previous:
            consumed_old.add(previous.key)
            row.state = "pending" if previous.state == "missing" else previous.state
            row.notes = previous.notes
        merged.append(row)
    for row in old.values():
        if row.key in current_ids or row.key in consumed_old:
            continue
        if row.state == "pending":
            row.state = "missing"
        merged.append(row)
    merged.sort(key=lambda row: (row.path, row.line, row.kind, row.key))
    write_review(merged)
    return merged


def mark_rows(
    ids: list[str],
    paths: list[str],
    domains: list[str],
    kinds: list[str],
    state: str,
    notes: str,
) -> list[Finding]:
    rows = sync_review()
    selected = set(ids)
    selected_paths = set(paths)
    selected_domains = set(domains)
    selected_kinds = set(kinds)
    matched = 0
    for row in rows:
        explicit = row.key in selected
        filtered = (
            row.state in {"pending", "missing"}
            and (not selected_paths or row.path in selected_paths)
            and (not selected_domains or row.domain in selected_domains)
            and (not selected_kinds or row.kind in selected_kinds)
            and (selected_paths or selected_domains or selected_kinds)
        )
        if explicit or filtered:
            row.state = state
            row.notes = notes
            matched += 1
    unknown = selected - {row.key for row in rows}
    if unknown:
        raise SystemExit(f"unknown review id(s): {', '.join(sorted(unknown))}")
    if matched == 0:
        raise SystemExit("no review rows matched")
    write_review(rows)
    print(f"[enum-conversions] marked {matched} row(s) {state}")
    return rows


def summary(rows: list[Finding]) -> None:
    current = {row.key for row in scan()}
    counts: dict[tuple[str, str], int] = {}
    for row in rows:
        key = (row.state, row.kind)
        counts[key] = counts.get(key, 0) + 1
    print(f"enum conversion review: {len(current)} current, {len(rows)} ledger row(s)")
    for (state, kind), count in sorted(counts.items()):
        print(f"  {state:8s} {kind:16s} {count:5d}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sync-review", action="store_true",
                        help="merge current occurrences into the durable review ledger")
    parser.add_argument("--summary", action="store_true", help="print counts only")
    parser.add_argument("--gate", action="store_true",
                        help="fail if the ledger is stale or contains pending/missing rows")
    parser.add_argument("--mark", action="append", default=[], metavar="ID",
                        help="review occurrence ID (repeatable)")
    parser.add_argument("--mark-path", action="append", default=[], metavar="PATH",
                        help="review every occurrence in a completely audited source file")
    parser.add_argument("--mark-domain", action="append", default=[], metavar="DOMAIN",
                        help="review unresolved occurrences in an audited enum domain")
    parser.add_argument("--mark-kind", action="append", default=[],
                        choices=("IDX", "static_cast-enum"), metavar="KIND",
                        help="restrict selector marking to one conversion kind")
    parser.add_argument("--state", choices=("fixed", "reviewed"),
                        help="resolution for --mark/--mark-path")
    parser.add_argument("--notes", default="", help="evidence or fix recorded on marked rows")
    args = parser.parse_args()

    if args.mark or args.mark_path or args.mark_domain or args.mark_kind:
        if args.state is None or not args.notes:
            parser.error("mark selectors require --state and non-empty --notes")
        rows = mark_rows(
            args.mark,
            args.mark_path,
            args.mark_domain,
            args.mark_kind,
            args.state,
            args.notes,
        )
    else:
        rows = sync_review() if args.sync_review else read_review()
    if not rows and not REVIEW.exists():
        print(f"{REVIEW.relative_to(REPO)} is missing; run --sync-review", file=sys.stderr)
        return 1
    summary(rows)

    current = {row.key for row in scan()}
    listed_current = {row.key for row in rows if row.key in current}
    unresolved = [row for row in rows if row.state in {"pending", "missing"}]
    stale = current - listed_current
    bad_states = [row for row in rows if row.state not in STATES]
    if args.gate and (unresolved or stale or bad_states):
        print(f"[enum-conversions] FATAL: {len(unresolved)} unresolved, "
              f"{len(stale)} unlisted current, {len(bad_states)} bad-state row(s)")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
