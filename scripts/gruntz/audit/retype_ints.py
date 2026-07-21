#!/usr/bin/env python3
"""gruntz.audit.retype_ints - whole-tree raw-int -> fixed-width-alias sweep.

Replaces raw C integer types in OUR sources (src/**, include/**) with Rust-style
fixed-width aliases (i8/u8/i16/u16/i32/u32/i64/u64), defined in <Ints.h>. The
target is 32-bit Win32 / MSVC 5.0, where `long`==`int`==4 bytes, so the mapping
PRESERVES WIDTHS and is matching-neutral (byte-identical codegen):

    int  / signed int  / signed long / long          -> i32
    unsigned / unsigned int / unsigned long          -> u32
    short / signed short                              -> i16    unsigned short -> u16
    signed char                                       -> i8     unsigned char  -> u8
    __int64 / long long                               -> i64    unsigned __int64 / unsigned long long -> u64

It is DELIBERATELY CONSERVATIVE - when in doubt it SKIPS (leaves the token raw).
A partial sweep is fine: the aliases are additive, raw `int` still compiles.

Re-runnable: idempotent (the alias names are not in the from-set), so it can be
re-run on files added later (e.g. when matchers resume).

  python3 -m gruntz.audit.retype_ints [--check] [--root <repo>] [paths...]

  --check : report would-be edits, write nothing (exit 1 if any would change).
  paths   : limit the sweep to these files/dirs (default: src/ + include/).

WHAT IT LEAVES RAW (by design - see the refactor brief):
  - vendor/** (upstream zlib), the MSVC/MFC/DX SDK headers (not ours).
  - SDK aliases used in our code (BOOL/DWORD/WORD/BYTE/UINT/INT/LONG/HWND/...).
  - plain `char` (text; `signed char`!=`char` as a type). Only signed/unsigned char map.
  - numeric literals (1L, 0xFFUL), comments, string/char contents, identifiers
    that merely CONTAIN int/long (Point, sprintf, m_longName), enum tags, and
    the RVA()/DATA()/SIZE() macro bodies (handled as ordinary tokens - those
    macros take addresses/types/sizes, not bare int keywords, so they're safe).

Headers it rewrites get `#include <Ints.h>` injected (idempotent) so the aliases
resolve even when the header is included before <rva.h> in a TU.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Multi-word types MUST come before single-word so e.g. "unsigned int" maps to
# u32 as a unit (not "unsigned"->u32 + a stray "int"). Within a width, the
# longest spelling is listed first. Tokens are matched as whole-word sequences
# with arbitrary whitespace between words (handled by the regex below).
MULTI_WORD = [
    (["unsigned", "__int64"], "u64"),
    (["unsigned", "long", "long"], "u64"),
    (["signed", "__int64"], "i64"),  # rare, but explicit
    (["long", "long"], "i64"),
    (["unsigned", "char"], "u8"),
    (["signed", "char"], "i8"),
    (["unsigned", "short", "int"], "u16"),
    (["unsigned", "short"], "u16"),
    (["signed", "short", "int"], "i16"),
    (["signed", "short"], "i16"),
    (["short", "int"], "i16"),
    (["unsigned", "int"], "u32"),
    (["unsigned", "long", "int"], "u32"),
    (["unsigned", "long"], "u32"),
    (["signed", "long", "int"], "i32"),
    (["signed", "long"], "i32"),
    (["signed", "int"], "i32"),
    (["long", "int"], "i32"),
]
SINGLE_WORD = [
    ("__int64", "i64"),
    ("short", "i16"),
    ("long", "i32"),
    ("int", "i32"),
    # bare `signed` == signed int == i32; bare `unsigned` == unsigned int == u32.
    ("unsigned", "u32"),
    ("signed", "i32"),
]

# A token is a C identifier run; everything else (punctuation, whitespace) is a
# separator. We tokenize so string/char/comment spans can be masked out and word
# boundaries are exact. NOTE: `__int64` contains no separator (leading __ + word
# chars), so the identifier regex captures it whole.
IDENT_RE = re.compile(r"[A-Za-z_]\w*")

# Build the multi-word matcher as: identifier (\w+) sequences separated by
# whitespace/comments-collapsed. We operate on a token stream, so the matcher is
# implemented over tokens, not a text regex (cleaner + comment-safe).
MULTI_WORD_SEQS = [(tuple(seq), repl) for seq, repl in MULTI_WORD]
SINGLE_WORD_MAP = dict(SINGLE_WORD)
ALL_FROM_WORDS = {"int", "long", "short", "signed", "unsigned", "__int64", "char"}
ALIAS_NAMES = {"i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64"}


def mask_spans(text: str):
    """Return a list of (kind, start, end) for spans we must NOT touch:
    line comments, block comments, string literals, char literals, and
    preprocessor lines that are #include / #error / #pragma (their tokens are
    not type positions). Returns a boolean 'protected' array per char index.
    """
    n = len(text)
    protected = bytearray(n)  # 1 => do not rewrite tokens overlapping here
    i = 0
    at_line_start = True
    while i < n:
        c = text[i]
        # Preprocessor directive line: protect the whole line. Type renames in
        # ordinary code (#define of a type alias) are rare in our sources and a
        # directive's tokens are not plain declarations; skip the line to be safe
        # ONLY for include/error/pragma/line. For #define/#if we still want to
        # skip because a #define body type would need the alias visible anyway
        # and these are not the sweep's target.
        if at_line_start:
            j = i
            while j < n and text[j] in " \t":
                j += 1
            if j < n and text[j] == "#":
                # protect to end of (possibly backslash-continued) line
                k = j
                while k < n:
                    if text[k] == "\\" and k + 1 < n and text[k + 1] == "\n":
                        k += 2
                        continue
                    if text[k] == "\n":
                        break
                    k += 1
                for p in range(i, min(k, n)):
                    protected[p] = 1
                i = k
                at_line_start = True
                continue
        if c == "\n":
            at_line_start = True
            i += 1
            continue
        if c not in " \t":
            at_line_start = False
        # line comment
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            k = i
            while k < n and text[k] != "\n":
                k += 1
            for p in range(i, k):
                protected[p] = 1
            i = k
            continue
        # block comment
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            k = i + 2
            while k < n and not (text[k] == "*" and k + 1 < n and text[k + 1] == "/"):
                k += 1
            k = min(k + 2, n)
            for p in range(i, k):
                protected[p] = 1
            i = k
            continue
        # string literal
        if c == '"':
            k = i + 1
            while k < n:
                if text[k] == "\\":
                    k += 2
                    continue
                if text[k] == '"':
                    k += 1
                    break
                k += 1
            for p in range(i, min(k, n)):
                protected[p] = 1
            i = k
            continue
        # char literal
        if c == "'":
            k = i + 1
            while k < n:
                if text[k] == "\\":
                    k += 2
                    continue
                if text[k] == "'":
                    k += 1
                    break
                k += 1
            for p in range(i, min(k, n)):
                protected[p] = 1
            i = k
            continue
        i += 1
    return protected


def tokenize(text: str):
    """Yield (kind, value, start, end). kind is 'id' for identifier runs or
    'sep' for everything between identifiers (whitespace/punct/comments)."""
    toks = []
    pos = 0
    for m in IDENT_RE.finditer(text):
        if m.start() > pos:
            toks.append(("sep", text[pos : m.start()], pos, m.start()))
        toks.append(("id", m.group(0), m.start(), m.end()))
        pos = m.end()
    if pos < len(text):
        toks.append(("sep", text[pos:], pos, len(text)))
    return toks


def sep_is_plain_ws(value: str) -> bool:
    """True if the separator between two type words is ONLY whitespace (spaces,
    tabs, newlines, line-continuations). If a comment sits between `unsigned`
    and `int`, we refuse to merge them (conservative)."""
    return all(ch in " \t\r\n\\" for ch in value)


def rewrite(text: str):
    """Return (new_text, counts) where counts maps alias -> n replacements."""
    protected = mask_spans(text)
    toks = tokenize(text)
    out = []
    counts = {}
    i = 0
    ntok = len(toks)

    def id_tok_protected(t):
        return any(protected[p] for p in range(t[2], t[3]))

    while i < ntok:
        kind, value, start, end = toks[i]
        if kind != "id" or value not in ALL_FROM_WORDS or id_tok_protected(toks[i]):
            out.append(value)
            i += 1
            continue
        # Try multi-word sequences first (longest already ordered in MULTI_WORD).
        matched = False
        for seq, repl in MULTI_WORD_SEQS:
            if value != seq[0]:
                continue
            # Walk forward across id tokens, requiring plain-whitespace seps.
            ids = [i]
            ok = True
            k = i + 1
            for want in seq[1:]:
                # skip a single separator (must be plain whitespace)
                if k < ntok and toks[k][0] == "sep":
                    if not sep_is_plain_ws(toks[k][1]):
                        ok = False
                        break
                    k += 1
                else:
                    ok = False
                    break
                if k < ntok and toks[k][0] == "id" and toks[k][1] == want and not id_tok_protected(toks[k]):
                    ids.append(k)
                    k += 1
                else:
                    ok = False
                    break
            if ok:
                out.append(repl)
                counts[repl] = counts.get(repl, 0) + 1
                i = k
                matched = True
                break
        if matched:
            continue
        # Safety: a bare modifier (`unsigned`/`signed`/`long`/`short`) immediately
        # followed - across a separator that is NOT plain whitespace, i.e. a
        # comment sits between - by another type-word would have been part of a
        # multi-word type but couldn't merge. Rewriting just the modifier would
        # emit two adjacent types (u32 /*x*/ i32). So leave the modifier RAW.
        # (This case doesn't occur in our tree; the guard keeps the script safe
        # for files added later.) `int`/`__int64` are never modifiers, so exempt.
        if value in ("unsigned", "signed", "long", "short"):
            # Look only at what IMMEDIATELY follows the modifier: a separator that
            # contains a comment, then (skipping the comment's tokenized words) a
            # type-word that would have completed a multi-word type. Only then do
            # we refuse. We do NOT scan past ordinary punctuation - `(short*)`
            # after an `i16*` decl on the same line must still convert; the `*)((`
            # between `short` and a later `char` is punctuation, not a comment.
            sep_has_comment = i + 1 < ntok and toks[i + 1][0] == "sep" and ("/*" in toks[i + 1][1] or "//" in toks[i + 1][1])
            if sep_has_comment:
                # Find the next real token, skipping the comment span (sep tokens
                # and protected ids that are the comment's words). Stop at the
                # first NON-protected id or any non-comment punctuation.
                k = i + 1
                landed = None
                while k < ntok:
                    tk = toks[k]
                    if tk[0] == "sep":
                        k += 1
                        continue
                    if id_tok_protected(tk):  # word inside the comment
                        k += 1
                        continue
                    landed = tk
                    break
                if landed is not None and landed[1] in ALL_FROM_WORDS:
                    # Emit the modifier AND everything up to & including the
                    # trailing type-word verbatim - never split one type in two.
                    for j in range(i, k + 1):
                        out.append(toks[j][1])
                    i = k + 1
                    continue
        # Single-word.
        if value in SINGLE_WORD_MAP:
            repl = SINGLE_WORD_MAP[value]
            out.append(repl)
            counts[repl] = counts.get(repl, 0) + 1
            i += 1
            continue
        # `char` alone or unmatched word: leave raw.
        out.append(value)
        i += 1
    return "".join(out), counts


INCLUDE_INTS = "#include <Ints.h>"


def ensure_ints_include(text: str) -> str:
    """For a HEADER we rewrote: inject `#include <Ints.h>` (idempotent) so the
    aliases resolve even if the header is included before <rva.h>. Place it just
    after the header guard's #define if present, else after the first #include,
    else at the very top after the leading comment block."""
    # Already visible: the header pulls <Ints.h> directly, or <rva.h> (which
    # includes <Ints.h>). Don't add a redundant include.
    if re.search(r"#\s*include\s*[<\"](?:Ints\.h|rva\.h)[>\"]", text):
        return text
    lines = text.splitlines(keepends=True)
    # 1) after an include guard: #ifndef X \n #define X
    for idx in range(len(lines) - 1):
        if re.match(r"\s*#ifndef\s+\w+", lines[idx]) and re.match(r"\s*#define\s+\w+", lines[idx + 1]):
            insert_at = idx + 2
            lines.insert(insert_at, "\n" + INCLUDE_INTS + "\n")
            return "".join(lines)
    # 2) after the first #include
    for idx, ln in enumerate(lines):
        if re.match(r"\s*#include", ln):
            lines.insert(idx + 1, INCLUDE_INTS + "\n")
            return "".join(lines)
    # 3) prepend
    return INCLUDE_INTS + "\n" + text


def gather_files(root: Path, paths):
    roots = []
    if paths:
        roots = [root / p if not Path(p).is_absolute() else Path(p) for p in paths]
    else:
        roots = [root / "src", root / "include"]
    files = []
    for r in roots:
        if r.is_file():
            files.append(r)
        elif r.is_dir():
            for ext in ("*.cpp", "*.h", "*.hpp", "*.cc", "*.cxx"):
                files.extend(r.rglob(ext))
    # Never touch vendor; never touch the alias header itself; never touch the
    # SDK-boundary shim headers (Win32.h / Mfc.h) - their `typedef int INT_PTR`
    # and `unsigned long` timeGetTime return are the REAL SDK ABI spellings and
    # must stay raw to pin our externs (renaming risks the match).
    EXCLUDE_NAMES = {"Ints.h", "Win32.h", "Mfc.h"}
    out = []
    for f in sorted(set(files)):
        rel = f.relative_to(root) if str(f).startswith(str(root)) else f
        parts = set(rel.parts)
        if "vendor" in parts:
            continue
        if f.name in EXCLUDE_NAMES:
            continue
        out.append(f)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description="raw-int -> fixed-width-alias sweep over src/+include/")
    ap.add_argument("--check", action="store_true", help="report only, write nothing (exit 1 if changes)")
    ap.add_argument("--root", default=None, help="repo root (default: auto-detect from this file)")
    ap.add_argument("paths", nargs="*", help="limit to these files/dirs (default src/ include/)")
    args = ap.parse_args(argv)

    if args.root:
        root = Path(args.root).resolve()
    else:
        # scripts/gruntz/audit/retype_ints.py -> repo root is 3 up
        root = Path(__file__).resolve().parents[3]

    files = gather_files(root, args.paths)
    total_counts = {}
    changed = []
    for f in files:
        text = f.read_text(encoding="utf-8", errors="surrogateescape")
        new, counts = rewrite(text)
        if new == text:
            continue
        # Header that we rewrote: make sure the alias is visible.
        if f.suffix in (".h", ".hpp"):
            new = ensure_ints_include(new)
        changed.append((f, counts))
        for k, v in counts.items():
            total_counts[k] = total_counts.get(k, 0) + v
        if not args.check:
            f.write_text(new, encoding="utf-8", errors="surrogateescape")

    print(f"files {'would change' if args.check else 'changed'}: {len(changed)}")
    for alias in ("i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64"):
        if alias in total_counts:
            print(f"  {alias}: {total_counts[alias]}")
    if args.check and changed:
        for f, c in changed:
            print(f"  {f.relative_to(root)}: {c}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
