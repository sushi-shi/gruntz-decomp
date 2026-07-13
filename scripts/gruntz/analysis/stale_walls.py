#!/usr/bin/env python3
"""stale_walls.py - mechanically re-derive the STATED BLOCKER of every wall claim in src/.

WHY THIS EXISTS.

A wall note is a claim about the binary, written at a moment in time. It is never re-checked.
So it ROTS: the phantom it blamed gets killed, the class it called SIZE_UNKNOWN gets a proven
size, the header it said "can never coexist" ends up with a single includer - and the note
still sits there in the present tense, blocking work. Four inherited walls have now been
falsified this way, each one on a premise that was mechanically checkable:

  * "CGrunt has a phantom +0x120 gap"        -> sizeof(CGrunt) is 0x8d8 from its ALLOCATION
                                                SITE, and the reconstruction already computed
                                                exactly that. The class was SIZE_UNKNOWN, so
                                                nothing had ever contradicted the note.
  * "GameText.h can never coexist (C2011)"    -> that header has exactly ONE includer.
  * "?ClearRecursive has 7 external callers"  -> a typedef re-mangles every call site.
  * "these 8 RVAs are not function starts"    -> correct, and thunk-resolving them recovered
                                                four already-claimed bodies.

The common shape: the claim's PREMISE is testable and nobody tests it. This tool tests it.

It does not decide whether a wall is real - it decides whether the wall's stated REASON still
holds. A claim whose premise is dead is STALE and must be re-derived from the binary before it
is trusted again. A genuine wall survives on byte-level proof (mechanism named, at the
instruction level), never on a citation.

    python -m gruntz.analysis.stale_walls              # audit everything, ranked by yield
    python -m gruntz.analysis.stale_walls --oracle     # just dump the allocation-size oracle
    python -m gruntz.analysis.stale_walls --file F     # one file
"""
import argparse
import csv
import glob
import os
import re
import struct
import sys
from pathlib import Path


def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists():
                return p
    return Path(__file__).resolve().parents[3]


REPO = _find_repo()
IMAGE_BASE = 0x400000

# ---------------------------------------------------------------------------
# the retail image
# ---------------------------------------------------------------------------


class Image:
    """The retail .text, addressable by RVA, plus one-hop ILT thunk resolution."""

    def __init__(self, path):
        b = self.b = open(path, "rb").read()
        pe = struct.unpack_from("<I", b, 0x3C)[0]
        nsec = struct.unpack_from("<H", b, pe + 6)[0]
        optsz = struct.unpack_from("<H", b, pe + 20)[0]
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + optsz + i * 40
            name = b[o:o + 8].split(b"\0")[0].decode("latin1")
            vsz, va, rsz, rptr = struct.unpack_from("<IIII", b, o + 8)
            self.secs.append((name, va, max(vsz, rsz), rptr))

    def off(self, rva):
        for _n, va, sz, rptr in self.secs:
            if va <= rva < va + sz:
                return rptr + (rva - va)
        return None

    def text(self):
        for n, va, sz, rptr in self.secs:
            if n == ".text":
                return va, self.b[rptr:rptr + sz]
        return 0, b""

    def call_target(self, site, rel_off):
        """Target RVA of an E8/E9 rel32 whose rel32 field starts at `rel_off` (RVA)."""
        o = self.off(rel_off)
        if o is None:
            return None
        rel = struct.unpack_from("<i", self.b, o)[0]
        return (rel_off + 4 + rel) & 0xFFFFFFFF

    def thunk(self, rva):
        """Follow ONE `jmp rel32` hop (the incremental-link thunk table). ILT entries are the
        reason a recorded 'RVA' can look like it is not a function start: the call site names
        the thunk, not the body."""
        o = self.off(rva)
        if o is None or o >= len(self.b):
            return rva
        if self.b[o] == 0xE9:
            t = self.call_target(rva, rva + 1)
            return t if t is not None else rva
        return rva


# ---------------------------------------------------------------------------
# the allocation-site oracle:  push <n> ; call ??2@YAPAXI@Z ; ... ; call <ctor>
# ---------------------------------------------------------------------------


def operator_new_rvas():
    out = set()
    p = REPO / "config/library_labels.csv"
    if p.is_file():
        for r in csv.reader(open(p)):
            if len(r) >= 2 and r[1].startswith("??2@YAPAXI@Z"):
                try:
                    out.add(int(r[0], 16))
                except ValueError:
                    pass
    return out


def load_symbols():
    """rva -> mangled name, and the reverse for functions."""
    rva2name, name2rva = {}, {}
    p = REPO / "build/gen/symbol_names.csv"
    if not p.is_file():
        return rva2name, name2rva
    for r in csv.DictReader(open(p, encoding="latin-1")):
        try:
            a = int(r["rva"], 16)
        except (ValueError, KeyError):
            continue
        rva2name[a] = r["name"]
        name2rva.setdefault(r["name"], a)
    return rva2name, name2rva


def allocation_oracle(img, opnew, rva2name):
    """{class_name: size} measured from the binary.

    Retail allocates a heap object as
        push <sizeof>            68 <imm32>
        call ??2@YAPAXI@Z        E8 <rel32>      (often via an ILT thunk)
        add  esp,4
        ...                      (mov ecx,eax / test / jz ...)
        call <ctor>              E8 <rel32>      -> ??0<Class>@@...
    so sizeof is GROUND TRUTH from the instruction stream - not an inference from the field
    map, and not bounded by how complete the ctor reconstruction happens to be. This is the
    check that killed the CGrunt "+0x120 gap": push 0x8d8 => sizeof(CGrunt)=0x8d8.
    """
    base, text = img.text()
    out, i, n = {}, 0, len(text)
    while True:
        i = text.find(b"\x68", i)
        if i < 0 or i + 5 > n:
            break
        size = struct.unpack_from("<I", text, i + 1)[0]
        i += 1
        if not (0 < size < 0x10000):
            continue
        # 1) the FIRST call after the push must be operator new (allow the odd store /
        #    register setup in between, but no other call - otherwise the push is not the
        #    size argument).
        k, j = i + 4, None
        lim = min(k + 24, n - 5)
        while k < lim:
            if text[k] == 0xE8:
                j = k
                break
            k += 1
        if j is None:
            continue
        t0 = img.call_target(base + j, base + j + 1)
        if t0 is None or not (t0 in opnew or img.thunk(t0) in opnew):
            continue
        # 2) the allocation must then be handed to a ctor as `this`: retail always does
        #    `mov ecx,eax` (8B C8) on the fresh block before `call <ctor>`. Requiring that
        #    edge is what keeps the oracle SOUND - without it the scan happily grabs the
        #    next unrelated call and invents a size for the wrong class.
        k = j + 5
        lim = min(k + 64, n - 5)
        seen_ecx = False
        while k < lim:
            if text[k] == 0x8B and text[k + 1] == 0xC8:  # mov ecx,eax
                seen_ecx = True
            elif text[k] == 0xE8:
                if not seen_ecx:
                    break
                c = img.call_target(base + k, base + k + 1)
                if c is not None:
                    nm = rva2name.get(img.thunk(c), "")
                    m = re.match(r"\?\?0(\w+)@@", nm)
                    if m:
                        out.setdefault(m.group(1), size)
                break
            k += 1
    return out


# ---------------------------------------------------------------------------
# the source side: declared sizes + what actually exists in the tree
# ---------------------------------------------------------------------------

SRC_GLOBS = ("src/**/*.cpp", "src/**/*.h", "include/**/*.h")
SIZE_RE = re.compile(r"\bSIZE\(\s*(\w+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
SIZE_UNK_RE = re.compile(r"\bSIZE_UNKNOWN\(\s*(\w+)\s*\)")
DEF_RE = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*(?::[^;{]*)?\{", re.M)
TYPEDEF_RE = re.compile(r"^\s*typedef\s+\w+\s+(\w+)\s*;", re.M)
INCLUDE_RE = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]', re.M)


def library_classes():
    """Every class the REAL toolchain headers define (MFC/CRT/Win32/DirectX).

    Without this the phantom test cries wolf on CString / CObject / CNoTrackObject: they are
    not in OUR tree because they were never ours. A tool that reports false walls is worse
    than no tool - it teaches people to ignore it."""
    out = set()
    for env in ("MSVC_DIR", "DXSDK_DIR"):
        d = os.environ.get(env)
        if not d:
            continue
        for f in glob.glob(os.path.join(d, "include", "**", "*.h"), recursive=True):
            try:
                txt = open(f, encoding="latin-1").read()
            except OSError:
                continue
            out.update(re.findall(r"\b(?:class|struct)\s+(?:AFX_\w+\s+)?(\w+)", txt))
    return out


def reconstructed_sizes():
    """sizeof() as OUR reconstruction actually computes it (clang, via build/gen/structs.json).

    Against the allocation-site oracle this is the sharpest check in the file: the binary says
    how many bytes retail allocated, clang says how many bytes our class occupies, and any gap
    is a LAYOUT BUG - a wrong base, a phantom pad, a missing member. It stays invisible while
    the class is SIZE_UNKNOWN, because nothing compares the two. That is exactly how the CGrunt
    "+0x120 gap" survived: had this check existed, it would have said MATCH and the wall would
    never have been written."""
    import json
    p = REPO / "build/gen/structs.json"
    if not p.is_file():
        return {}

    # structs.json is NOT a ninja target - `gruntz build` never refreshes it; only an explicit
    # `gruntz structs` (or `init`) does. So it goes STALE the moment a header's layout changes,
    # and this check then compares the binary against a snapshot of our OLD sizes. That is
    # dangerous in BOTH directions: it invents layout bugs that are already fixed, and - far
    # worse - it can report MATCH for a class whose layout we have since broken. Measured
    # 2026-07-13: it reported all 9 layout bugs as still-live for hours after they were fixed.
    # Refuse to answer on stale data; tell the caller exactly how to refresh it.
    newest_hdr = max(
        (f.stat().st_mtime for pat in ("include/**/*.h", "src/**/*.h")
         for f in REPO.glob(pat)),
        default=0,
    )
    if newest_hdr > p.stat().st_mtime:
        print("  !! build/gen/structs.json is STALE (a header changed after it was generated).")
        print("     The size check is SKIPPED - it would compare the binary against old sizes,")
        print("     which can hide a real layout bug. Refresh it:  gruntz structs")
        return {}

    return {e["name"]: e.get("size") for e in json.load(open(p)) if e.get("size")}


def scan_sources():
    files, sizes, unknown, defined, includes = {}, {}, set(), set(), {}
    # class -> the files that DEFINE it. A C2011 needs a class with >= 2 definitions; the
    # includer count of a header is irrelevant to that (see T3).
    defs_by_class = {}
    defs_by_file = {}
    for g in SRC_GLOBS:
        for f in glob.glob(str(REPO / g), recursive=True):
            rel = os.path.relpath(f, REPO)
            t = open(f, encoding="latin-1").read()
            files[rel] = t
            for m in SIZE_RE.finditer(t):
                sizes[m.group(1)] = int(m.group(2), 16 if m.group(2).startswith("0x") else 10)
            for m in SIZE_UNK_RE.finditer(t):
                unknown.add(m.group(1))
            here = set(DEF_RE.findall(t))
            defined.update(here)
            defined.update(TYPEDEF_RE.findall(t))
            defs_by_file[os.path.basename(rel)] = here
            for c in here:
                defs_by_class.setdefault(c, set()).add(rel)
            for h in INCLUDE_RE.findall(t):
                includes.setdefault(os.path.basename(h), set()).add(rel)
    return files, sizes, unknown, defined, includes, defs_by_class, defs_by_file


# ---------------------------------------------------------------------------
# claim extraction + the premise tests
# ---------------------------------------------------------------------------

MARKERS = re.compile(r"@early-stop|@identity-TODO")
BLOCKER = re.compile(
    r"\b(C2011|irreducible|cannot coexist|never coexist|blocked on|BLOCKED|is blocked|"
    r"would clash|would collide|walled out|dual-view|dual model|ODR split|"
    r"not a function start|unclaimed|no source definition)\b",
    re.I,
)
CLASS_TOK = re.compile(r"\b((?:C[A-Z]\w+|z[A-Z]\w+|_z\w+))\b")
RVA_TOK = re.compile(r"\b0x0*([0-9a-f]{4,6})\b", re.I)
HDR_TOK = re.compile(r"<([\w/]+\.h)>|\b(\w+\.h)\b")
SIZE_PREMISE = re.compile(r"SIZE_UNKNOWN|size (?:is )?(?:not|un)known|unknown size|size TBD", re.I)
# A note that ALREADY records a wall as dead/corrected is not a live claim. Without this the
# tool re-flags its own fix notes forever ("STALE-WALL NOTE, corrected: ... the wall is DEAD"),
# which is the fastest way to teach people to ignore it.
RESOLVED_RE = re.compile(
    r"is DEAD|was false|STALE-WALL NOTE|now folded|is folded|Do not resurrect|no longer holds",
    re.I,
)
SLOT_PREMISE = re.compile(r"(vtable|vtbl|slot)[^.\n]{0,40}(member|field|size)", re.I)


def comment_blocks(text):
    """Contiguous // comment runs -> (start_line, text)."""
    out, cur, start = [], [], None
    for i, ln in enumerate(text.splitlines(), 1):
        s = ln.strip()
        if s.startswith("//"):
            if start is None:
                start = i
            cur.append(s.lstrip("/ ").rstrip())
        else:
            if cur:
                out.append((start, "\n".join(cur)))
            cur, start = [], None
    if cur:
        out.append((start, "\n".join(cur)))
    return out


PHANTOM_RE = re.compile(r"\\b(phantom|fake view|stand-in|placeholder)\\b", re.I)


def near(blk, premise_re, token, window=140):
    """Is `token` cited close to the premise phrase? A wall note often lists a dozen unrelated
    RVAs/classes in passing; only the ones next to the premise ARE the premise."""
    for pm in premise_re.finditer(blk):
        lo, hi = max(0, pm.start() - window), pm.end() + window
        if token in blk[lo:hi]:
            return True
    return False


def audit(img, opnew):
    rva2name, _n2r = load_symbols()
    files, sizes, unknown, defined, includes, defs_by_class, defs_by_file = scan_sources()
    libcls = library_classes()
    oracle = allocation_oracle(img, opnew, rva2name)

    findings = []
    for rel, text in sorted(files.items()):
        for line, blk in comment_blocks(text):
            is_claim = MARKERS.search(blk) or BLOCKER.search(blk)
            if not is_claim:
                continue
            cited_classes = set(CLASS_TOK.findall(blk))
            hits = []

            # T1 - a SIZE/layout premise about a class whose size the BINARY now gives us.
            if SIZE_PREMISE.search(blk):
                for c in cited_classes:
                    if not near(blk, SIZE_PREMISE, c):
                        continue
                    if c in oracle:
                        hits.append(
                            "STALE size premise: %s is SIZE-KNOWN from its allocation site "
                            "(push 0x%x) - re-derive the layout claim" % (c, oracle[c])
                        )
                    elif c in sizes:
                        hits.append(
                            "STALE size premise: %s now carries SIZE(%s, 0x%x)"
                            % (c, c, sizes[c])
                        )

            # T2 - "phantom/view X blocks this", but X is gone from the tree.
            if PHANTOM_RE.search(blk):
                for c in cited_classes:
                    if c.isupper() or c in libcls:
                        continue  # not a tree class: a library type, or not a name at all
                    if c in defined or c in sizes or c in unknown:
                        continue  # still exists
                    if not near(blk, PHANTOM_RE, c):
                        continue  # cited in passing, not as the premise
                    if any(re.search(r"\b%s\b" % re.escape(c), x) for f, x in files.items()
                           if not x.count("//") or True) and sum(
                            len(re.findall(r"\b%s\b" % re.escape(c), x)) for x in files.values()
                            ) > 3:
                        continue  # still referenced all over the tree -> not a dead phantom
                    hits.append(
                        "STALE phantom premise: %s no longer exists in the tree" % c
                    )

            # T3 - a C2011 / "can never coexist" premise that no duplicate definition backs.
            #
            # This used to ask "does the cited header have <= 1 includer?" and call the wall
            # stale if so. That inference is UNSOUND and it cried wolf: a C2011 fires when TWO
            # DEFINITIONS of one class meet in ONE TU, so what matters is whether a duplicate
            # definition exists at all - the header's includer COUNT says nothing about it. (It
            # mislabelled the live CUserLogic true-0x30/fat-0x40 ODR walls as dead.)
            #
            # The sound test: the premise is dead only if NO class defined by the cited header
            # is defined anywhere else in the tree - then there is nothing to redefine, and
            # including it cannot raise C2011.
            if RESOLVED_RE.search(blk):
                pass  # the note already records the wall as dead/corrected - not a live claim
            elif re.search(r"C2011|coexist|would clash|would collide|walled out", blk, re.I):
                for a, b in HDR_TOK.findall(blk):
                    h = os.path.basename(a or b)
                    hdr_classes = defs_by_file.get(h)
                    if not hdr_classes:
                        continue  # we do not have the header's defs -> cannot judge; stay quiet
                    dupes = sorted(c for c in hdr_classes if len(defs_by_class.get(c, ())) > 1)
                    if not dupes:
                        hits.append(
                            "STALE C2011/coexist premise: %s defines %d class(es) and NOT ONE is "
                            "defined anywhere else - there is nothing to redefine, so it cannot "
                            "raise C2011. (Necessary, not sufficient: confirm by actually "
                            "compiling the co-inclusion before removing the wall.)"
                            % (h, len(hdr_classes))
                        )

            # T4 - "RVA is not a function start / unclaimed", but it thunk-resolves to a
            #      body we already claim (this recovered 4 of 8 leaf hooks).
            #
            # "no body" alone is NOT evidence of an unbindable premise: across this tree it is
            # overwhelmingly the ordinary phrase for a DECLARED-ONLY extern ("all engine callees
            # are reloc-masked (no body)"), which says nothing about whether an RVA is claimed.
            # Left in, it fired on every reloc-masked-callee note that happened to mention an RVA
            # within 100 chars - two such false hits in TileTriggerSwitchLogic.cpp alone. A tool
            # that cries wolf gets ignored, so "no body" only counts when it is NOT the
            # declared-only idiom.
            UNBIND_RE = re.compile(r"not a function start|unclaimed|nothing defines", re.I)
            NO_BODY_RE = re.compile(r"no body", re.I)
            DECL_ONLY_RE = re.compile(r"reloc-masked|declared[- ]only|extern|no body\)", re.I)
            if NO_BODY_RE.search(blk) and not DECL_ONLY_RE.search(blk):
                UNBIND_RE = re.compile(
                    r"not a function start|unclaimed|no body|nothing defines", re.I
                )
            if UNBIND_RE.search(blk):
                for r in RVA_TOK.findall(blk):
                    if not near(blk, UNBIND_RE, "0x" + r, 100) and not near(
                            blk, UNBIND_RE, r, 100):
                        continue
                    a = int(r, 16)
                    if a in rva2name:
                        hits.append(
                            "STALE unbindable premise: 0x%06x IS claimed now (%s)"
                            % (a, rva2name[a])
                        )
                        continue
                    t = img.thunk(a)
                    if t != a and t in rva2name:
                        hits.append(
                            "STALE unbindable premise: 0x%06x is an ILT thunk -> 0x%06x (%s)"
                            % (a, t, rva2name[t])
                        )

            # T5 - vtable slot count used to bound DATA members. Unsound; a sibling retracted
            #      exactly this inference. Never STALE on its own - always re-derive.
            if SLOT_PREMISE.search(blk):
                hits.append(
                    "SUSPECT inference: vtable/slot count says nothing about data members"
                )

            for h in hits:
                findings.append((rel, line, h))

    return findings, oracle, sizes, unknown


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--oracle", action="store_true", help="dump the allocation-size oracle")
    ap.add_argument("--file", help="audit one file")
    a = ap.parse_args()

    exe = os.environ.get("GRUNTZ_EXE")
    if not exe or not os.path.exists(exe):
        print("stale_walls: $GRUNTZ_EXE not set")
        return 2
    img = Image(exe)
    opnew = operator_new_rvas()
    if not opnew:
        print("stale_walls: could not find ??2@YAPAXI@Z in config/library_labels.csv")
        return 2

    findings, oracle, sizes, unknown = audit(img, opnew)

    if a.oracle:
        print("allocation-size oracle: %d class(es) sized from the binary\n" % len(oracle))
        for c, s in sorted(oracle.items()):
            src = ("SIZE(0x%x)" % sizes[c]) if c in sizes else (
                "SIZE_UNKNOWN" if c in unknown else "-")
            flag = ""
            if c in sizes:
                if sizes[c] != s:
                    flag = "   <-- MISMATCH vs source (a real layout bug)"
            elif c in unknown:
                flag = "   <-- recoverable (source says unknown)"
            print("  %-34s push 0x%-5x  src %-14s%s" % (c, s, src, flag))
        return 0

    if a.file:
        findings = [f for f in findings if f[0] == a.file]

    stale = [f for f in findings if f[2].startswith("STALE")]
    suspect = [f for f in findings if f[2].startswith("SUSPECT")]

    for rel, line, h in stale:
        print("  %-46s:%-5d %s" % (rel, line, h))
    if suspect:
        print()
        for rel, line, h in suspect:
            print("  %-46s:%-5d %s" % (rel, line, h))

    # the ranked-by-yield list the sweep should work first
    ours = reconstructed_sizes()
    bugs = [(c, ours[c], oracle[c]) for c in sorted(oracle)
            if c in ours and ours[c] != oracle[c]]
    recov = sorted(c for c in unknown if c in oracle and c not in sizes)
    print("\nallocation-size oracle: %d class(es) sized straight from the binary" % len(oracle))
    if bugs:
        print("  LAYOUT BUG - our sizeof disagrees with the binary's allocation (%d, HIGHEST"
              " YIELD: every field past the gap is mis-addressed):" % len(bugs))
        for c, o, b in bugs:
            print("    %-34s ours 0x%-6x binary 0x%-6x  (off by 0x%x)"
                  % (c, o, b, abs(o - b)))
    if recov:
        print("  SIZE_UNKNOWN but recoverable from the binary (%d) - pin SIZE() so no future"
              " note can claim the size is unknown:" % len(recov))
        print("    " + ", ".join(recov))
    print("\nstale_walls: %d STALE claim(s), %d SUSPECT claim(s)" % (len(stale), len(suspect)))
    print("A claim whose PREMISE is dead is not a wall. Re-derive it from the binary before")
    print("trusting it. A real wall survives on byte-level proof, never on a citation.")
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
