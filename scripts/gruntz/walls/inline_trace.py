#!/usr/bin/env python3
"""gruntz.walls.inline_trace - record cl 5.0's /Ob inline decisions for a unit.

    gruntz walls inline-trace <unit> [--root TEXT] [--json]

Compiles the unit with its manifest profile through a private copy of the
pinned c2.exe (cl's `/B2` back-end override) whose stand-in mspdb50.dll
(`c2shim/c2trace.c`) patches logging hooks into the inline pass when
GRUNTZ_C2_TRACE is set. The hooks re-execute the instructions they
displace, so the traced compile must emit the unit's build object byte for
byte, up to the TimeDateStamp and the per-compile number in anonymous-namespace
names (compared structurally, without file offsets); a trace whose object
differs is refused.

Per root (every function the pass visits, in compile order) the trace
records its front-end cost `cb` and, per candidate call site in tuple order,
the callee and its cost, the site's depth and depth allowance, the running
budget, the remaining-site count, the running size, the argument-count gate,
whether every gate passed (`admit`) and the nested walk of an expanded body.
`gruntz walls inline-solve` replays and edits these trees.

The overlay lives under build/c2-shim/ and is rebuilt when the shim source
changes. The trace is written to build/inline-trace/<unit>.json and reused
while the build object, the unit's source and the headers are unchanged.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
from pathlib import Path

from gruntz.core.paths import BUILD, REPO

SHIM_SOURCE = Path(__file__).resolve().parent / "c2shim"
OVERLAY = BUILD / "c2-shim"
TRACES = BUILD / "inline-trace"
FREE = 40


def _sha(*paths: Path) -> str:
    digest = hashlib.sha256()
    for path in paths:
        digest.update(path.read_bytes())
    return digest.hexdigest()


def _shim_sha() -> str:
    return _sha(SHIM_SOURCE / "c2trace.c", SHIM_SOURCE / "mspdb50.def")


def overlay() -> Path:
    """The traced back end: c2.exe, the real mspdb50 as mspdb50r.dll and the
    stand-in mspdb50.dll, side by side. Returns the c2.exe path."""
    from gruntz.tool import ToolError
    from gruntz.tool.wine import era_tool, run

    OVERLAY.mkdir(parents=True, exist_ok=True)
    for name, target in (("c2.exe", "c2.exe"), ("mspdb50.dll", "mspdb50r.dll")):
        source, copy = era_tool(name), OVERLAY / target
        if not copy.is_file() or copy.read_bytes() != source.read_bytes():
            shutil.copyfile(source, copy)
            copy.chmod(0o644)
    sources = (SHIM_SOURCE / "c2trace.c", SHIM_SOURCE / "mspdb50.def")
    stamp, dll = OVERLAY / "shim.sha256", OVERLAY / "mspdb50.dll"
    want = _shim_sha()
    if dll.is_file() and stamp.is_file() and stamp.read_text().strip() == want:
        return OVERLAY / "c2.exe"
    work = OVERLAY / "shim-build"
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir()
    for source in sources:
        shutil.copyfile(source, work / source.name)
    output, _rc = run(["wine", str(era_tool("cl.exe")), "/nologo", "/O1", "/MT", "/LD",
                       "c2trace.c", "/link", "/DEF:mspdb50.def", "/OUT:mspdb50.dll"],
                      cwd=work, success=work / "mspdb50.dll")
    if not (work / "mspdb50.dll").is_file():
        raise ToolError("c2 shim build failed:\n" + output[-2000:])
    shutil.copyfile(work / "mspdb50.dll", dll)
    stamp.write_text(want + "\n")
    return OVERLAY / "c2.exe"


def compile_traced(src: Path, flags: list[str], out: Path,
                   rules: list[str] | None = None) -> str:
    """Compile `src` through the traced back end; returns the trace text.

    `rules` (shim rule lines: root, owner, callee, occurrence, E|K) force
    those inline decisions; an empty list forces nothing."""
    from gruntz.tool import cl
    from gruntz.tool.wine import winepath

    c2 = overlay()
    log = out.with_suffix(".trace")
    log.unlink(missing_ok=True)
    env = {"GRUNTZ_C2_TRACE": winepath(log)}
    if rules is not None:
        rule_file = out.with_suffix(".rules")
        rule_file.write_text("".join(f"{line}\n" for line in rules))
        env["GRUNTZ_C2_FORCE"] = winepath(rule_file)
    previous = {key: os.environ.get(key) for key in ("GRUNTZ_C2_TRACE", "GRUNTZ_C2_FORCE")}
    os.environ.pop("GRUNTZ_C2_FORCE", None)
    os.environ.update(env)
    try:
        cl.compile(src, out, [*flags, f"/B2{winepath(c2)}"])
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
    if not log.is_file():
        raise ValueError("the traced c2.exe wrote no log")
    return log.read_text(encoding="latin1")


def _fields(words: list[str]) -> dict[str, str]:
    return dict(word.split("=", 1) for word in words)


def parse(text: str) -> dict:
    """Every root of one traced back-end pass, in compile order.

    A site's `expanded` is True when C2 admitted it (every gate passed) and
    walked its body. C2 refuses some admitted sites later, from the call's
    context: `charged` records whether that happened after the site's cost
    was charged to the budget and running size."""
    lines = text.splitlines()
    if lines and lines[0].startswith("refused"):
        raise ValueError("the shim refused to hook: " + lines[0])
    if "begin" not in lines or "end" not in lines:
        raise ValueError("the trace is truncated (no begin/end record)")
    names: dict[str, str] = {}
    roots: list[dict] = []
    root: dict | None = None
    site: dict | None = None
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "sym":
            address, name = rest.split(" ", 1)
            names[address] = name
        elif kind == "main":
            words = rest.split()
            fields = _fields(words[1:])
            cb = int(fields["cb"])
            root = dict(symbol=names[words[0]], cb=cb,
                        initial_budget=min(35000, max(1000, 2 * cb)), sites=[])
            roots.append(root)
            site = None
        elif kind == "site":
            fields = _fields(rest.split())
            if root is None or names.get(fields["root"]) != root["symbol"]:
                raise ValueError("an inline site precedes its root record")
            cb, budget = int(fields["cb"]), int(fields["budget"])
            site = dict(symbol=names[fields["callee"]], owner=names[fields["owner"]],
                        depth=int(fields["depth"]), allow=int(fields["allow"]),
                        budget=budget, remaining=int(fields["remain"]), cb=cb,
                        running=int(fields["running"]),
                        budget_allows=cb <= FREE or budget >= cb,
                        args_match=False, admitted=False, charged=False, expanded=False)
            root["sites"].append(site)
        elif kind == "args":
            if site is None:
                raise ValueError("an argument-count record follows no site")
            fields = _fields(rest.split())
            site["args_match"] = fields["actual"] == fields["formal"]
        elif kind == "force":
            if site is None:
                raise ValueError("a force record follows no site")
            site["forced"] = _fields(rest.split())["action"]
        elif kind == "admit":
            if site is None:
                raise ValueError("an admit record follows no site")
            site["admitted"] = True
        elif kind == "charge":
            if site is None or not site["admitted"]:
                raise ValueError("a charge record follows no admitted site")
            site["charged"] = True
        elif kind == "body":
            if site is None or not site["admitted"]:
                raise ValueError("a body walk follows no admitted site")
            fields = _fields(rest.split())
            site["expanded"] = True
            site["share"] = int(fields["share"])
    return dict(roots=roots)


def unit_source(unit: str) -> tuple[Path, list[str]]:
    from gruntz.graph.emit import load_units

    _manifest, units = load_units()
    for entry in units:
        if entry["unit"] == unit:
            return REPO / entry["source"], entry["cflags"]
    raise SystemExit(f"inline-trace: no unit {unit!r} in config/units.toml")


#: cl 5.0 names an anonymous namespace after its file and a number that
#: changes between compiles (`?x@?A0xfadersine.cpp1177419789@@...`); its
#: length varies, so file offsets move with it.
_ANONYMOUS = re.compile(rb"(\.(?:cpp|c))\d+(?=@)")


def _comparable(data: bytes) -> tuple:
    """The object as two compiles of unchanged source agree on it: sections
    (data and relocations) and symbols (resolved names, values, auxiliary
    records), with no file offsets, no TimeDateStamp and anonymous-namespace
    numbers removed."""
    from gruntz.compare.canonicalize import SYMBOL_SIZE, CoffObject

    coff = CoffObject(data)

    def mask(raw: bytes) -> bytes:
        return _ANONYMOUS.sub(rb"\1", raw)
    # An uninitialized section (.bss) has a size and no file data.
    sections = tuple(
        (section.name, section.characteristics, section.raw_size if not section.raw_offset else
         mask(coff.data[section.raw_offset:section.raw_offset + section.raw_size]),
         tuple((r.site, r.symbol_index, r.typ) for r in coff.relocations
               if r.section == section.index))
        for section in coff.sections)
    symbols = tuple(
        (mask(symbol.name.encode("latin-1")), symbol.value, symbol.section, symbol.typ,
         symbol.storage_class,
         coff.data[symbol.offset + SYMBOL_SIZE:symbol.offset + SYMBOL_SIZE * (1 + symbol.aux_count)])
        for symbol in coff.symbols.values())
    return sections, symbols


def _object_sha(path: Path) -> str:
    return hashlib.sha256(repr(_comparable(path.read_bytes())).encode()).hexdigest()


def _input_key(src: Path) -> str:
    """What a cached trace was made from. Front-end costs can change while
    the object does not, so the source and the newest header count too."""
    newest = max((p.stat().st_mtime_ns for p in (REPO / "include").rglob("*.h")), default=0)
    return hashlib.sha256(src.read_bytes() + str(newest).encode()).hexdigest()


def trace_unit(unit: str) -> dict:
    """Trace one unit; refuse unless the traced object is the build object."""
    from gruntz import graph

    src, flags = unit_source(unit)
    expected = REPO / graph.BASE_DIR / f"{unit}.obj"
    if not expected.is_file():
        raise SystemExit(f"inline-trace: {expected} missing - run `gruntz build` first")
    work = TRACES / unit
    work.mkdir(parents=True, exist_ok=True)
    out = work / f"{unit}.obj"
    text = compile_traced(src, flags, out)
    if _comparable(out.read_bytes()) != _comparable(expected.read_bytes()):
        raise SystemExit(f"inline-trace: the traced object differs from {expected}; "
                         "rebuild the unit (`gruntz match "
                         f"{unit}`) before tracing")
    report = parse(text)
    report.update(unit=unit, source=str(src.relative_to(REPO)), flags=flags,
                  shim_sha256=_shim_sha(), input_sha256=_input_key(src),
                  object_sha256=_object_sha(expected))
    (TRACES / f"{unit}.json").write_text(json.dumps(report, indent=1) + "\n")
    return report


def load(unit: str, *, refresh: bool = False) -> dict:
    """A unit's trace, re-traced when missing, stale or `refresh`."""
    from gruntz import graph

    path = TRACES / f"{unit}.json"
    expected = REPO / graph.BASE_DIR / f"{unit}.obj"
    if not refresh and path.is_file() and expected.is_file():
        report = json.loads(path.read_text())
        if report.get("shim_sha256") == _shim_sha() \
                and report.get("object_sha256") == _object_sha(expected) \
                and report.get("input_sha256") == _input_key(unit_source(unit)[0]):
            return report
    return trace_unit(unit)


def _print(report: dict, needle: str | None) -> None:
    for root in report["roots"]:
        if needle and needle not in root["symbol"]:
            continue
        if not root["sites"] and needle is None:
            continue
        print(f"{root['symbol']}  cb={root['cb']} budget={root['initial_budget']}")
        for index, site in enumerate(root["sites"]):
            verdict = ("expands" if site["expanded"] else
                       "context refusal, charged" if site["charged"] else
                       "context refusal" if site["admitted"] else
                       "args differ" if not site["args_match"] else
                       "depth" if site["depth"] > site["allow"] else
                       "budget" if not site["budget_allows"] else
                       "size cap" if site["running"] > 35000 else "refused")
            print(f"  #{index:<3} {'  ' * (site['depth'] - 1)}{site['symbol']}  "
                  f"cb={site['cb']} budget={site['budget']} remain={site['remaining']} "
                  f"running={site['running']}  {verdict}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls inline-trace",
                                 description=__doc__.split("\n\n")[1])
    ap.add_argument("unit")
    ap.add_argument("--root", help="only roots whose mangled name contains TEXT")
    ap.add_argument("--refresh", action="store_true", help="re-trace even when current")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    report = load(args.unit, refresh=args.refresh)
    if args.json:
        roots = [r for r in report["roots"] if not args.root or args.root in r["symbol"]]
        json.dump(dict(report, roots=roots), sys.stdout, indent=1)
        print()
    else:
        _print(report, args.root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
