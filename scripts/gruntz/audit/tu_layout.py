#!/usr/bin/env python3
"""tu_layout.py - intra-TU spatial structure of the matched source.

Answers: how far apart in retail RVA space are the functions of one class/TU, and
does that layout give us a usable notion of which methods are "related"?

It parses the matched `src/` directly - every RVA() macro is a function we
have placed at a known retail address (src/Stub/ is skipped: those are the
not-yet-matched backlog). No build artifacts are needed; this runs in the default
`nix develop`.

The measured answer (see docs/tu-spatial-structure.md):

  * Ordinary AND virtual methods cluster TIGHTLY - a matched method's nearest
    same-TU sibling is typically a few hundred bytes away. So `proximity => same
    class/TU` is a reliable relatedness signal for them: a matched method's RVA
    neighbours are its siblings, ready to batch.
  * Destructors are the EXCEPTION - MSVC emits the vtable-referenced deleting
    destructor as a COMDAT, and the linker pools those into shared low-address runs
    (e.g. 0x10000-0x14000) far from each class's own block. ~40% of dtors sit far
    from any sibling, so proximity does NOT find them; they come in by class
    identity (leaked name / vtable / RTTI) instead.

Report sections + queries:

  1. RELATEDNESS METRIC - nearest same-TU-neighbour distance per function kind.
  2. PER-TU CLUSTERING  - each TU's functions, their cluster split, and span with
     vs. without the special members (the destructor-exile signature).
  3. SPECIAL-MEMBER POOLS - the shared low-address runs where ctors/dtors from many
     unrelated classes congregate.
  4. INTERMINGLING     - do different classes' method blocks interleave? Mostly no
     (~95% of adjacent method pairs that aren't a boundary are same-class), and the
     splices that exist are between SIBLING classes of one family (CButeMgr/CButeValue,
     the State family, ...). Classes split into DENSE (one contiguous run) vs
     SCATTERED (conflated TUs / COMDAT-heavy families like the State classes).
  --neighbors 0xRVA    - the probable class siblings of one matched function.
  --attribute          - tie classless functions to a class by same-class bracketing.
     Two target sets: (a) still-unclaimed FUN_ bodies (from the Ghidra boundary
     export, minus current RVA/RVA_COMPGEN source claims) -> new class stubs
     (gen_attributed_stubs); (b) already-labeled CATCH-ALL stubs
     (ApiCallers/Backlog/EngineExternFns + placeholder classes) -> a RELOCATION
     worklist (move the typed stub to that class's TU). The both-sides HIGH rule is
     validated at ~91% exact / 94% same-family by leave-one-out on matched methods.

Usage:
    python3 -m gruntz.audit.tu_layout
    python3 -m gruntz.audit.tu_layout --neighbors 0xb4020
    python3 -m gruntz.audit.tu_layout --attribute [--emit attributions.csv]
"""
from __future__ import annotations

import argparse
import re
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.audit.link_order import regions

# repo root + src tree (same idiom as gruntz.match.verify_stubs).
REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
INCLUDE = REPO / "include"

# RVA(0x.., 0x..) carries a matched function's retail address + size.
# the unsized variant (an unpinned thunk). Same macros gruntz.match.verify_stubs reads.
RVA_RE = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
# RVA_COMPGEN(<rva>, <size>, <mangled>) - a compiler-generated fn pin (rva.h);
# participates in the intra-TU order check like RVA().
RVA_COMPGEN_RE = re.compile(
    r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*([^\s,)]+)\s*\)")
# the Class::method (or Class::~Class / operator) on the definition line below it.
SIG_RE = re.compile(r"([A-Za-z_]\w*)::(~?[A-Za-z_]\w*|operator[^\(]*)")

# Intra-TU clustering wants a smaller gap than link_order's cross-TU REGION_GAP
# (0x40000): a class's own block spans a few KB, so a > ~0x4000 jump is a new run.
DEFAULT_GAP = 0x4000
# "related" window: methods within this of a sibling are co-located in the TU block.
DEFAULT_NEAR = 0x2000
# the two empirical special-member pools (low COMDAT-dtor runs).
POOLS = ((0x10000, 0x14000), (0x80000, 0x90000))
# the Ghidra boundary export feeding --attribute (all .text starts + sizes + names).
DEFAULT_FUNCTIONS = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
# subsystem keywords: an attribution that misses the exact class usually lands on a
# sibling of the SAME family (intermingling is within-family, not cross-subsystem).
FAMILY_KEYS = ("Bute", "State", "Attract", "Menu", "Booty", "Credits", "Multi",
               "DDraw", "DDSurface", "FileImage", "Trigger", "Command", "Net",
               "Grunt", "Tile", "Sound", "Ambient", "Image", "Anim")


def pooled(rva: int) -> bool:
    """True if rva is in a COMDAT special-member pool (dtors/ctors of many classes)."""
    return any(lo <= rva < hi for lo, hi in POOLS)


def family(cls):
    """Coarse subsystem bucket for a class name (for same-family attribution credit)."""
    if cls:
        for k in FAMILY_KEYS:
            if k in cls:
                return k
    return cls


class Func:
    __slots__ = ("rva", "size", "cls", "meth", "tu")

    def __init__(self, rva, size, cls, meth, tu):
        self.rva, self.size, self.cls, self.meth, self.tu = rva, size, cls, meth, tu

    @property
    def kind(self) -> str:
        if not self.meth:
            return "other"
        if self.meth.startswith("~"):
            return "dtor"
        if self.cls and self.meth == self.cls:
            return "ctor"
        return "method"

    @property
    def name(self) -> str:
        return f"{self.cls}::{self.meth}" if self.cls and self.meth else (self.meth or "?")

    @property
    def label(self) -> str:
        return self.meth or "?"


def _parse_size(s: str) -> int:
    # NOT core.symbols._psize: inputs are regex-guaranteed valid, so garbage must
    # RAISE (loud bug), and a literal 0 stays 0 - different contract, not a dup.
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def load_funcs(src: Path = SRC) -> list[Func]:
    """Every non-stub RVA/RVA_COMPGEN function claim in src/, sorted by RVA."""
    out: list[Func] = []
    for path in sorted(src.rglob("*.cpp")):
        if "Stub" in path.parts[len(src.parts):]:  # skip src/Stub/ backlog
            continue
        tu = path.stem
        lines = path.read_text(errors="replace").splitlines()
        for i, ln in enumerate(lines):
            m = RVA_RE.search(ln)
            cm = RVA_COMPGEN_RE.search(ln)
            if not m and not cm:
                continue
            match = m or cm
            rva, size = int(match.group(1), 16), _parse_size(match.group(2))
            cls = meth = None
            if cm:
                meth = cm.group(3)
            else:
                for j in range(i, min(i + 4, len(lines))):
                    sm = SIG_RE.search(lines[j])
                    if sm:
                        cls, meth = sm.group(1), sm.group(2)
                        break
            out.append(Func(rva, size, cls, meth, tu))
    out.sort(key=lambda f: f.rva)
    return out


def load_claimed_extents(roots=(SRC, INCLUDE)) -> list[tuple[int, int]]:
    """Every function extent claimed by reconstructed source or a shared header.

    Inline and compiler-generated bodies are often annotated at their class
    definition in include/. They are real claims even though load_funcs() keeps
    its TU-clustering input restricted to .cpp files. Complete extents matter:
    Ghidra can place a false FUN_ boundary inside an already reconstructed body.
    """
    claimed = []
    for root in roots:
        for pattern in ("*.cpp", "*.h"):
            for path in root.rglob(pattern):
                text = path.read_text(errors="replace")
                for rx in (RVA_RE, RVA_COMPGEN_RE):
                    for match in rx.finditer(text):
                        rva = int(match.group(1), 16)
                        size = _parse_size(match.group(2))
                        claimed.append((rva, rva + max(size, 1)))
    return sorted(set(claimed))


def by_tu(funcs: list[Func]) -> dict[str, list[Func]]:
    d: dict[str, list[Func]] = defaultdict(list)
    for f in funcs:
        d[f.tu].append(f)
    for fs in d.values():
        fs.sort(key=lambda f: f.rva)
    return d


def nearest_neighbors(tus: dict[str, list[Func]]) -> dict[Func, int]:
    """Each function -> distance to its nearest same-TU sibling (start to start)."""
    nn: dict[Func, int] = {}
    for fs in tus.values():
        for idx, f in enumerate(fs):
            d = []
            if idx > 0:
                d.append(f.rva - fs[idx - 1].rva)
            if idx < len(fs) - 1:
                d.append(fs[idx + 1].rva - f.rva)
            if d:
                nn[f] = min(d)
    return nn


def _pct(sorted_vals, q):
    return sorted_vals[min(len(sorted_vals) - 1, int(len(sorted_vals) * q))]


def report_relatedness(tus, nn, near: int) -> None:
    print("=" * 72)
    print("1. RELATEDNESS METRIC  (distance to nearest same-TU sibling, by kind)")
    print("=" * 72)
    buckets: dict[str, list[int]] = defaultdict(list)
    for f, d in nn.items():
        buckets[f.kind].append(d)
    print(f"  {'kind':8} {'n':>4} {'median':>9} {'p90':>9}  within {near:#x}")
    for k in ("method", "ctor", "dtor", "other"):
        v = sorted(buckets.get(k, []))
        if not v:
            continue
        close = sum(1 for x in v if x < near) / len(v)
        print(f"  {k:8} {len(v):4d} {_pct(v, .5):9x} {_pct(v, .9):9x}  {close:7.0%}")
    print("\n  methods (ordinary + virtual overrides) cluster tightly; destructors are")
    print("  the outlier - linker-pooled away from their class (see section 3).")


def report_clustering(tus, gap: int, top: int) -> None:
    print("\n" + "=" * 72)
    print(f"2. PER-TU CLUSTERING  (gap>{gap:#x} splits a run; span shrink = dtor/ctor exile)")
    print("=" * 72)
    rows = []
    for tu, fs in tus.items():
        if len(fs) < 3:
            continue
        rvas = [f.rva for f in fs]
        reg = [f.rva for f in fs if f.kind not in ("ctor", "dtor")]
        span_all = rvas[-1] - rvas[0]
        span_reg = (max(reg) - min(reg)) if len(reg) >= 2 else 0
        nreg = len(regions(rvas, gap))
        rows.append((span_all, span_reg, nreg, len(fs), tu))
    rows.sort(key=lambda r: -(r[0] - r[1]))  # biggest exile first
    print(f"  {'span_all':>9} {'span_reg':>9} {'shrink':>6} {'cls':>3} {'n':>3}  unit")
    for span_all, span_reg, nreg, n, tu in rows[:top]:
        shrink = (1 - span_reg / span_all) if span_all else 0.0
        print(f"  {span_all:9x} {span_reg:9x} {shrink:6.0%} {nreg:3d} {n:3d}  {tu}")
    print(f"\n  {len(rows)} TUs with >=3 funcs; showing the {min(top, len(rows))} with the")
    print("  largest span collapse when ctors/dtors are dropped (the exile signature).")


def report_pools(funcs) -> None:
    print("\n" + "=" * 72)
    print("3. SPECIAL-MEMBER POOLS  (low-address runs shared across unrelated classes)")
    print("=" * 72)
    for lo, hi in POOLS:
        pool = [f for f in funcs if lo <= f.rva < hi]
        kc = Counter(f.kind for f in pool)
        classes = sorted({f.cls for f in pool
                          if f.kind in ("ctor", "dtor") and f.cls})
        print(f"\n  {lo:#08x}-{hi:#08x}: {dict(kc)}  ({len(pool)} funcs, "
              f"{len(classes)} distinct ctor/dtor classes)")
        if classes:
            head = ", ".join(classes[:14])
            print(f"     classes: {head}{' ...' if len(classes) > 14 else ''}")
    print("\n  these are MSVC COMDAT deleting-destructors, grouped by the linker; the")
    print("  contributing classes are otherwise unrelated. Proximity can't tie a")
    print("  pooled dtor to its class - use the leaked name / vtable / RTTI.")


def report_neighbors(funcs, target: int, near: int) -> None:
    """Probable class siblings of one matched function: its tight RVA neighbours."""
    by_rva = {f.rva: f for f in funcs}
    f = by_rva.get(target)
    print("=" * 72)
    if f is None:
        # nearest matched function, for orientation
        nearest = min(funcs, key=lambda g: abs(g.rva - target))
        print(f"no matched function at {target:#08x}; nearest is "
              f"{nearest.name} @ {nearest.rva:#08x} (in {nearest.tu})")
        return
    print(f"--neighbors {target:#08x}: {f.name}  [{f.kind}]  unit={f.tu}")
    print("=" * 72)
    sib = sorted((g for g in funcs if g is not f and abs(g.rva - f.rva) <= near),
                 key=lambda g: g.rva)
    pooled = any(lo <= f.rva < hi for lo, hi in POOLS)
    print(f"  within {near:#x} of this function ({len(sib)} probable siblings):")
    for g in sib:
        d = g.rva - f.rva
        flag = "  <- same unit" if g.tu == f.tu else ""
        print(f"    {g.rva:#08x}  {d:+#08x}  {g.name:34} [{g.kind}]{flag}")
    if pooled:
        print("\n  NOTE: this function sits in a special-member POOL - its RVA neighbours")
        print("  are unrelated pooled ctors/dtors, NOT its class. Find siblings by name.")
    elif not sib:
        print("    (none - isolated; likely a pooled special member or a lone match)")


# ---- intermingling: do different classes' method blocks interleave? -----------

def _matched_seq(funcs, methods_only=False, drop_pooled=False):
    """Sorted list[Func] of matched funcs carrying a class (optionally filtered)."""
    out = []
    for f in funcs:
        if not f.cls:
            continue
        if methods_only and f.kind != "method":
            continue
        if drop_pooled and pooled(f.rva):
            continue
        out.append(f)
    out.sort(key=lambda f: f.rva)
    return out


def _run_bounds(seq):
    """Per index: maximal same-class run length reaching back / forward."""
    n = len(seq)
    back, fwd = [1] * n, [1] * n
    for i in range(1, n):
        back[i] = back[i - 1] + 1 if seq[i].cls == seq[i - 1].cls else 1
    for i in range(n - 2, -1, -1):
        fwd[i] = fwd[i + 1] + 1 if seq[i].cls == seq[i + 1].cls else 1
    return back, fwd


def report_intermingling(funcs) -> None:
    print("\n" + "=" * 72)
    print("4. INTERMINGLING  (method blocks, pools excluded; do classes interleave?)")
    print("=" * 72)
    seq = _matched_seq(funcs, methods_only=True, drop_pooled=True)
    same = sum(1 for i in range(len(seq) - 1) if seq[i].cls == seq[i + 1].cls)
    diff = len(seq) - 1 - same
    splices = Counter()
    for i in range(1, len(seq) - 1):
        a, b, c = seq[i - 1].cls, seq[i].cls, seq[i + 1].cls
        if a == c and a != b:
            splices[tuple(sorted((a, b)))] += 1
    print(f"  adjacent method pairs: same-class={same}  boundary(diff)={diff}  "
          f"true A-B-A splices={sum(splices.values())}")
    if splices:
        print("  splices are between SIBLING classes of one family:")
        for (a, b), n in splices.most_common(8):
            tag = "" if family(a) != family(b) else "  (same family)"
            print(f"     {n:2d}x  {a} <-> {b}{tag}")
    # dense vs scattered: largest contiguous same-class run / class method count
    longest, total = defaultdict(int), Counter()
    cur, runlen = None, 0
    for f in seq:
        runlen = runlen + 1 if f.cls == cur else 1
        cur = f.cls
        longest[f.cls] = max(longest[f.cls], runlen)
        total[f.cls] += 1
    multi = [c for c in total if total[c] >= 3]
    dense = [c for c in multi if longest[c] >= total[c] * 0.8]
    scattered = sorted(c for c in multi if longest[c] < total[c] * 0.5)
    print(f"\n  classes with >=3 matched methods: {len(multi)}  ->  "
          f"DENSE(one run) {len(dense)},  SCATTERED(<50% in run) {len(scattered)}")
    print(f"  scattered (conflated TUs / COMDAT-heavy families): "
          f"{', '.join(scattered[:18])}")


# ---- attribution: tie an UNNAMED function to a class by RVA proximity ----------
# Validated rule (leave-one-out on matched methods; see docs/tu-spatial-structure.md):
# an unknown body flanked on BOTH sides, within --gap, by the same class C that sits
# in a contiguous run of >=3 of its own methods (and not in a pool) is C with ~92%
# precision (94% to a same-family sibling). Pool-adjacent or short/mixed runs are
# downgraded to MED (right family, not always the exact class).

def is_thunk(name: str, size: int) -> bool:
    return size <= 6 or name.startswith("thunk_") or name.startswith("j_")


def load_boundaries(path: Path):
    """All .text function boundaries from the Ghidra export: [(rva, size, name)]."""
    import csv
    out = []
    with path.open() as fh:
        for r in csv.DictReader(fh):
            out.append((int(r["entry_rva"], 16), int(r["byte_size"]), r["name"]))
    out.sort()
    return out


# catch-all stub homes: functions parked in a classless / placeholder unit, NOT yet
# tied to their real class. These are ALREADY labeled (typed @stub bodies), so they
# are a RELOCATION worklist - move the typed stub into the attributed class's TU -
# not new-stub material. ApiCallers.cpp is generated (docs/api-caller-name-plan.tsv).
CATCHALL_FILES = {"ApiCallers.cpp", "Backlog.cpp", "EngineExternFns.cpp"}
CATCHALL_CLASSES = {"EngineLabelBacklog", "ThisStubOwnerUnknown"}


def _attribute_targets(seq, back, fwd, gap, target_rvas):
    """Assign each target RVA the class of a same-class matched bracket it falls in."""
    import bisect
    ts = sorted(target_rvas)
    out = {}
    for i in range(len(seq) - 1):
        a, b = seq[i], seq[i + 1]
        if a.cls != b.cls or (b.rva - a.rva) > gap:
            continue
        conf = ("HIGH" if max(back[i], fwd[i + 1]) >= 3
                and not pooled(a.rva) and not pooled(b.rva) else "MED")
        for t in ts[bisect.bisect_right(ts, a.rva):bisect.bisect_left(ts, b.rva)]:
            out[t] = (a.cls, conf)
    return out


def attribute(funcs, boundaries, gap, claimed_extents=None):
    """Tie each unclaimed FUN_ body to a class. Returns (attributions, count)."""
    import bisect

    seq = _matched_seq(funcs)
    back, fwd = _run_bounds(seq)
    extents = (list(claimed_extents) if claimed_extents is not None
               else [(f.rva, f.rva + max(f.size, 1)) for f in funcs])
    extents.sort()
    starts = [lo for lo, _ in extents]
    max_ends = []
    for _, hi in extents:
        max_ends.append(max(hi, max_ends[-1] if max_ends else hi))

    def is_claimed(rva):
        i = bisect.bisect_right(starts, rva)
        return i > 0 and max_ends[i - 1] > rva

    unnamed = [b[0] for b in boundaries
               if b[2].startswith("FUN_") and not is_thunk(b[2], b[1])
               and not is_claimed(b[0])]
    return _attribute_targets(seq, back, fwd, gap, unnamed), len(unnamed)


def load_catchall_stubs(src: Path = SRC):
    """Classless / placeholder stubs (already labeled) that want relocating to a real
    class: every RVA stub in a catch-all file, plus any stub classed as a placeholder.
    Returns [(rva, current_class_or_None, file)]."""
    out = []
    for p in sorted((src / "Stub").glob("*.cpp")):
        catchall_file = p.name in CATCHALL_FILES
        lines = p.read_text(errors="replace").splitlines()
        for i, ln in enumerate(lines):
            m = RVA_RE.search(ln)
            if not m:
                continue
            rva = int(m.group(1), 16)
            cur = None
            for j in range(i, min(i + 3, len(lines))):
                sm = SIG_RE.search(lines[j])
                if sm:
                    cur = sm.group(1)
                    break
            if catchall_file or cur in CATCHALL_CLASSES:
                out.append((rva, cur, p.name))
    return out


def attribute_catchall(funcs, gap, src: Path = SRC):
    """Proximity class for each catch-all stub (a relocation worklist).
    Returns ({rva: (cls, conf, current_class, file)}, n_catchall)."""
    seq = _matched_seq(funcs)
    back, fwd = _run_bounds(seq)
    stubs = load_catchall_stubs(src)
    hit = _attribute_targets(seq, back, fwd, gap, [r for r, _, _ in stubs])
    meta = {r: (cur, f) for r, cur, f in stubs}
    return ({r: (c, conf, *meta[r]) for r, (c, conf) in hit.items()}, len(stubs))


def boundary_targets(funcs, target_rvas, near=DEFAULT_NEAR):
    """Targets that sit at a class BOUNDARY - the two matched neighbours are different
    classes (so the both-sides bracket abstains) - with the nearer one within `near`.
    These are the ~58%-exact one-sided cases; document BOTH candidate classes rather
    than commit to one. Returns {rva: (below_cls, below_dist, above_cls, above_dist)}."""
    import bisect
    seq = _matched_seq(funcs)
    srv = [f.rva for f in seq]
    out = {}
    for rva in target_rvas:
        i = bisect.bisect_left(srv, rva)
        if i == 0 or i >= len(seq):
            continue
        lo, hi = seq[i - 1], seq[i]
        if lo.cls == hi.cls or lo.rva == rva or hi.rva == rva:
            continue
        if min(rva - lo.rva, hi.rva - rva) > near:
            continue
        out[rva] = (lo.cls, rva - lo.rva, hi.cls, hi.rva - rva)
    return out


def validate_loo(funcs, gap):
    """Leave-one-out precision of the both-sides bracket on KNOWN methods.

    Hide each matched method; if its two matched neighbours agree on a class within
    --gap, that's the bracket's prediction - compare to the held-out method's real
    class. Returns (n_both, n_exact, n_family).
    """
    seq = _matched_seq(funcs, methods_only=True, drop_pooled=True)
    both = exact = fam_ok = 0
    for i in range(1, len(seq) - 1):
        lo, mid, hi = seq[i - 1], seq[i], seq[i + 1]
        if lo.cls == hi.cls and max(mid.rva - lo.rva, hi.rva - mid.rva) <= gap:
            both += 1
            exact += (lo.cls == mid.cls)
            fam_ok += (family(lo.cls) == family(mid.cls))
    return both, exact, fam_ok


def report_attribution(funcs, functions_csv: Path, gap: int, emit) -> None:
    print("=" * 72)
    print("ATTRIBUTION  (tie unnamed FUN_ bodies to a class by same-class bracket)")
    print("=" * 72)
    if not functions_csv.exists():
        print(f"  needs the Ghidra boundary export: {functions_csv}")
        print("  run `gruntz build` (or point --functions at an existing functions.csv).")
        return
    boundaries = load_boundaries(functions_csv)
    attr, n_unnamed = attribute(funcs, boundaries, gap, load_claimed_extents())
    n_hi = sum(1 for _, c in attr.values() if c == "HIGH")
    n_med = len(attr) - n_hi
    both, exact, fam_ok = validate_loo(funcs, gap)
    print(f"  remaining unnamed FUN_ bodies (non-thunk, source claims excluded): {n_unnamed}")
    print(f"  attributed by same-class bracket (gap<={gap:#x}): {len(attr)} "
          f"({len(attr) / n_unnamed:.0%})  ->  HIGH {n_hi}, MED {n_med}")
    if both:
        print(f"  leave-one-out precision of the HIGH (both-sides) rule on known "
              f"methods:\n     {exact}/{both} exact ({exact / both:.0%}), "
              f"{fam_ok}/{both} same-family ({fam_ok / both:.0%})")
    gained = Counter(c for c, _ in attr.values())
    print("  top classes by unnamed bodies recovered:")
    for c, n in gained.most_common(12):
        print(f"     {c:24} +{n}")

    # ---- catch-all relocation worklist (already-labeled classless stubs) --------
    cattr, n_cat = attribute_catchall(funcs, gap)
    c_hi = sum(1 for v in cattr.values() if v[1] == "HIGH")
    print(f"\n  catch-all stubs (ApiCallers/Backlog/EngineExternFns + placeholder "
          f"classes): {n_cat}")
    print(f"  of those, proximity ties to a real class: {len(cattr)} "
          f"-> HIGH {c_hi}, MED {len(cattr) - c_hi}.  These already carry a typed @stub")
    print("  body, so this is a RELOCATION worklist (move the stub to that class's TU),")
    print("  not new stubs. sample (rva  current -> proximity):")
    for rva in sorted(cattr)[:10]:
        cls, conf, cur, f = cattr[rva]
        print(f"     0x{rva:06x}  {str(cur):20} -> {cls:22} {conf:4} [{f}]")

    if emit:
        import csv
        with open(emit, "w", newline="") as fh:
            w = csv.writer(fh)
            w.writerow(("rva", "class", "confidence", "kind", "current", "file"))
            for rva in sorted(attr):
                c, conf = attr[rva]
                w.writerow((f"0x{rva:06x}", c, conf, "new-stub", "", ""))
            for rva in sorted(cattr):
                c, conf, cur, f = cattr[rva]
                w.writerow((f"0x{rva:06x}", c, conf, "relocate", cur or "", f))
        print(f"\n  wrote {len(attr)} new-stub + {len(cattr)} relocate "
              f"attributions -> {emit}")


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gap", type=lambda s: int(s, 0), default=DEFAULT_GAP,
                    help="intra-TU cluster split threshold (default 0x4000)")
    ap.add_argument("--near", type=lambda s: int(s, 0), default=DEFAULT_NEAR,
                    help="'related' window for the metric/--neighbors (default 0x2000)")
    ap.add_argument("--top", type=int, default=25,
                    help="rows to show in the per-TU clustering table")
    ap.add_argument("--neighbors", type=lambda s: int(s, 0), default=None,
                    metavar="0xRVA", help="show the probable siblings of one function")
    ap.add_argument("--functions", type=Path, default=DEFAULT_FUNCTIONS,
                    help="Ghidra boundary export for --attribute (functions.csv)")
    ap.add_argument("--attribute", action="store_true",
                    help="tie unnamed FUN_ bodies to a class by same-class bracketing")
    ap.add_argument("--emit", type=Path, default=None, metavar="CSV",
                    help="with --attribute, write rva,class,confidence to this file")
    args = ap.parse_args()

    funcs = load_funcs()
    if args.neighbors is not None:
        report_neighbors(funcs, args.neighbors, args.near)
        return
    if args.attribute:
        report_attribution(funcs, args.functions, args.gap, args.emit)
        return

    tus = by_tu(funcs)
    nn = nearest_neighbors(tus)
    print(f"matched (non-stub) RVA-annotated funcs: {len(funcs)} across {len(tus)} units\n")
    report_relatedness(tus, nn, args.near)
    report_clustering(tus, args.gap, args.top)
    report_pools(funcs)
    report_intermingling(funcs)


if __name__ == "__main__":
    main()
