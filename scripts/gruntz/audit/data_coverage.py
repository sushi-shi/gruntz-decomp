#!/usr/bin/env python3
"""data_coverage - is every datum MODELLED, not merely byte-equal at its extent?

THE DEFECT CLASS THIS EXISTS FOR
--------------------------------
We choose the extent, so a TOO-SMALL claim always scores 100. If retail has

    struct float2 { float x, y; };  float2 g_v;      /* 8 bytes at 0x1000 */

and `src/` models `float g_v;`, objdiff compares four bytes, finds them equal and
calls the section exact. `y` is silently unmodelled - it becomes a gap, or gets
absorbed into a neighbour's inferred extent, or is claimed by the wrong owner.
**The score cannot see this, because the score only ever compares what we told it
to compare.** It runs the other way too: `?g_idleGeom@@3PAUBzGeomPair@@A` was an
invented array whose member order was `{m_y; m_x}` - both models produced the same
X column so it survived byte comparison, and it turned out to be a phantom.

So this tool audits from OUR side: which bytes does no claim cover, which claims
overlap, which claims does nothing ever point at. Its sibling
(`build/gen/data_access_map.tsv`, lane `data-access-map`) audits from RETAIL's
side: what width and form the code uses at each address. **Neither alone tells
padding from an unmodelled field** - an uncovered byte retail READS is unmodelled
data, an uncovered byte nothing ever touches is padding - so the gap census here
is emitted as a join-shaped TSV keyed by address range.

THE ORACLES, ALL FROM RETAIL, NONE FROM OUR PAYLOAD
---------------------------------------------------
`claims`   `build/gen/delink_data_manifest.tsv` - every enrolled datum's retail
           rva + size + storage + owning object. This is what objdiff compares.
`payload`  retail's own bytes at an uncovered range. cl's inter-symbol padding is
           ZERO, so a NON-ZERO uncovered byte is content nobody modelled. This is
           the strongest single tell and it needs no disassembly.
`.reloc`   retail's HIGHLOW fixup table. Two independent uses:
             (a) a relocated word INSIDE an uncovered range is a POINTER nobody
                 modelled - conclusive, and it cannot be alignment padding;
             (b) every absolute address operand in `.text`/`.rdata`/`.data` is
                 relocated, so reading the stored dword at each site yields the
                 set of data addresses THE CODE POINTS AT, with no disassembly at
                 all. An address in that set inside an uncovered range is a datum;
                 a claim no site in that set ever names is a phantom candidate.

A CONTENT-DERIVED ADDRESS IS SELF-CONFIRMING, so nothing here is inferred from our
own bytes: every range comes from the enrolled extents, and every verdict from
retail's payload or retail's relocation table.

ADJACENCY PROVES NOTHING. A gap is reported, never closed by inventing an
aggregate to fill it; `data_runs` already documents why a packed run of globals is
MSVC's default layout and not evidence of a struct.

VERDICTS
--------
  PAD          uncovered, all-zero, shorter than the alignment its successor
               needs. cl's own inter-symbol padding. Benign; the calibration
               floor.
  ZERO-GAP     uncovered, all-zero, too long to be padding. Either `.bss`-like
               zero-initialized data nobody claimed, or the library frontier.
  NONZERO      uncovered bytes that are not zero. Content nobody modelled.
  POINTER      NONZERO and retail relocates a word inside it. Unmodelled pointer
               data - conclusive, since padding is never relocated.
  ADDRESSED    the range contains an address the code itself points at. Whatever
               else it is, it is a datum with a live reference.

  OVERLAP      two claims covering the same byte with different extents (a folded
               COMDAT seen from N objects is not an overlap and is excluded).
  UNADDRESSED  a claim no relocation anywhere names - a phantom CANDIDATE, not a
               phantom: string literals are reached by an operand inside a bigger
               claim, and `.bss` scratch may be reached off a neighbour.

USAGE
-----
    python -m gruntz.audit.data_coverage                  # summary + worklist
    python -m gruntz.audit.data_coverage --tsv PATH       # the join-shaped census
    python -m gruntz.audit.data_coverage --sections       # ordinary_sections proof
    python -m gruntz.audit.data_coverage --calibrate      # false-positive rate
    python -m gruntz.audit.data_coverage --near 0x20b8f8  # one address in context
"""
import argparse
import bisect
import csv
import json
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core.pe import PE, REPO

#: The two initialized data sections. `.bss` is inside `.data`'s virtual tail in
#: this image (the section table has no separate `.bss`), so a range past
#: `.data`'s raw size reads as zero and is handled by `payload_of`.
DATA_SECTIONS = (".rdata", ".data")
MANIFEST = "build/gen/delink_data_manifest.tsv"
SECTIONS = "build/gen/delink_data_section_manifest.tsv"
#: cl aligns a standalone global to its own element size, capped at 8 for the
#: ordinary sections (16 shows up only on `__declspec(align)`, which MSVC 5 lacks).
MAX_PAD = 8


# --------------------------------------------------------------------------- #
# the claim set
# --------------------------------------------------------------------------- #
def load_claims(path=None):
    """Every enrolled datum, as dicts with int `rva`/`size`.

    The same retail extent appears once per object that defines it (a folded
    COMDAT literal or vtable), so callers that need the covered byte SET must
    dedupe on (rva, size) - `coverage()` does.
    """
    p = Path(path or REPO / MANIFEST)
    if not p.exists():
        sys.exit("no %s - run `gruntz build` first" % p)
    out = []
    with p.open() as f:
        for r in csv.DictReader(f, delimiter="\t"):
            r["rva"] = int(r["rva"], 16)
            r["size"] = int(r["size"], 16)
            out.append(r)
    return out


def load_sections(path=None):
    """Every PLACED candidate section - the ones that claim a retail range.

    A section row is the OTHER half of the coverage story and the first draft of
    this sieve got it wrong by ignoring it. A `/GR` vtable COMDAT is 4 bytes
    longer than the enrolled `??_7` extent, because the complete-object-locator
    pointer sits in front of it and has no datum of its own; the delinker still
    rebuilds and compares those 4 bytes. Counting them as uncovered manufactured
    ~180 false positives along the vtable frontier alone.
    """
    p = Path(path or REPO / SECTIONS)
    if not p.exists():
        return []
    out = []
    with p.open() as f:
        for r in csv.DictReader(f, delimiter="\t"):
            if r["rva"] == "-":                     # non-affine: no retail claim
                continue
            out.append({"object": r["object"], "name": r["name"],
                        "rva": int(r["rva"], 16), "size": int(r["size"], 16),
                        "storage": r["storage"]})
    return out


def coverage(claims, sections=()):
    """Maximal covered runs `[start, end)`.

    A byte is COVERED when an enrolled datum claims it OR a placed candidate
    section rebuilds it. Both are compared by objdiff; only what neither reaches
    is genuinely unmodelled.
    """
    ext = {(c["rva"], c["size"]) for c in claims}
    ext |= {(s["rva"], s["size"]) for s in sections}
    merged = []
    for a, sz in sorted(ext):
        b = a + sz
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return [(a, b) for a, b in merged]


def overlaps(claims):
    """Claims that cover a shared byte with DIFFERENT extents.

    One retail COMDAT defined by N objects is the normal case and agrees on both
    rva and size, so it is not an overlap. A genuine overlap is two DIFFERENT
    extents intersecting: at least one of them is the wrong shape.
    """
    extents = sorted({(c["rva"], c["size"]) for c in claims})
    byext = defaultdict(list)
    for c in claims:
        byext[(c["rva"], c["size"])].append(c)
    out = []
    for i, (a, sz) in enumerate(extents):
        for a2, sz2 in extents[i + 1:]:
            if a2 >= a + sz:
                break
            out.append(((a, sz, byext[(a, sz)]), (a2, sz2, byext[(a2, sz2)])))
    return out


# --------------------------------------------------------------------------- #
# retail's own oracles
# --------------------------------------------------------------------------- #
class Retail:
    """retail payload + the two readings of the HIGHLOW table."""

    def __init__(self, pe=None):
        self.pe = pe or PE()
        self.sites = self.pe.reloc_sites
        self.lo = min(s["rva"] for s in self.pe.sections if s["name"] in DATA_SECTIONS)
        hi = max((s for s in self.pe.sections if s["name"] in DATA_SECTIONS),
                 key=lambda s: s["rva"])
        self.hi = hi["rva"] + max(hi["virtual_size"], hi["raw_size"])
        self._pointed = None

    def payload(self, rva, n):
        """Retail's bytes at `[rva, rva+n)`; a virtual-only tail reads as zero."""
        out = bytearray()
        for i in range(n):
            o = self.pe.off(rva + i)
            out.append(0 if o is None else self.pe.data[o])
        return bytes(out)

    def relocs_in(self, rva, n):
        """HIGHLOW sites inside `[rva, rva+n)`."""
        lo = bisect.bisect_left(self.sites, rva)
        hi = bisect.bisect_left(self.sites, rva + n)
        return self.sites[lo:hi]

    @property
    def pointed(self):
        """address -> how many relocated operands hold it, over the WHOLE image.

        Every absolute address operand is relocated, so the stored dword at each
        HIGHLOW site is an address the image itself points at. `.text` sites are
        code operands; `.rdata`/`.data` sites are pointer table entries. Both
        count as "something names this address".
        """
        if self._pointed is None:
            c = Counter()
            ib = self.pe.image_base
            for s in self.sites:
                o = self.pe.off(s)
                if o is None:
                    continue
                t = struct.unpack_from("<I", self.pe.data, o)[0] - ib
                if self.lo <= t < self.hi:
                    c[t] += 1
            self._pointed = c
        return self._pointed

    @property
    def pointed_sorted(self):
        if not hasattr(self, "_ps"):
            self._ps = sorted(self.pointed)
        return self._ps

    def pointed_in(self, rva, n):
        ps = self.pointed_sorted
        lo = bisect.bisect_left(ps, rva)
        hi = bisect.bisect_left(ps, rva + n)
        return ps[lo:hi]


# --------------------------------------------------------------------------- #
# the census
# --------------------------------------------------------------------------- #
def gaps(claims, retail, sections=()):
    """Every uncovered range strictly between two covered runs, with a verdict.

    Ranges before the first claim and after the last are the library frontier,
    not a modelling defect, and are excluded: they are territory we have never
    attributed, and reporting them would drown the signal.
    """
    runs = coverage(claims, sections)
    ends, starts = defaultdict(list), defaultdict(list)
    for c in claims:
        starts[c["rva"]].append(c)
        ends[c["rva"] + c["size"]].append(c)
    for s in sections:                              # a section can bound a gap too
        starts[s["rva"]].append({"name": "<section %s>" % s["name"],
                                 "object": s["object"], "rva": s["rva"],
                                 "size": s["size"]})
        ends[s["rva"] + s["size"]].append({"name": "<section %s>" % s["name"],
                                           "object": s["object"], "rva": s["rva"],
                                           "size": s["size"]})

    out = []
    for (_, b1), (a2, _) in zip(runs, runs[1:]):
        n = a2 - b1
        pay = retail.payload(b1, n)
        nz = sum(1 for x in pay if x)
        rel = retail.relocs_in(b1, n)
        ptd = retail.pointed_in(b1, n)
        # a real datum names the boundary better than the section that holds it
        key = (lambda c: (c["name"].startswith("<"), c["name"]))
        prev = sorted(ends.get(b1, []), key=key)
        nxt = sorted(starts.get(a2, []), key=key)
        po = {c["object"] for c in prev}
        no = {c["object"] for c in nxt}
        shared = po & no

        if rel and nz:
            verdict = "POINTER"
        elif nz:
            verdict = "NONZERO"
        elif n < MAX_PAD:
            verdict = "PAD"
        else:
            verdict = "ZERO-GAP"
        out.append({
            "rva": b1, "length": n, "section": retail.pe.sec_name(b1) or "?",
            "verdict": verdict,
            "addressed": len(ptd),
            "payload_nonzero": nz,
            "relocs": len(rel),
            "owning_unit": sorted(shared)[0] if len(shared) == 1 else "-",
            "prev_object": prev[0]["object"] if prev else "-",
            "prev_name": prev[0]["name"] if prev else "-",
            "next_object": nxt[0]["object"] if nxt else "-",
            "next_name": nxt[0]["name"] if nxt else "-",
            "first_bytes": pay[:16].hex(),
        })
    return out


GAP_COLUMNS = ["rva", "length", "section", "verdict", "addressed",
               "payload_nonzero", "relocs", "owning_unit", "nearest_claim",
               "prev_object", "prev_name", "next_object", "next_name",
               "first_bytes"]


def write_tsv(rows, path):
    """The join-shaped census. `rva`+`length` is the join key against the access
    map; `nearest_claim` is the enrolled datum immediately before the range."""
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("w", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(GAP_COLUMNS)
        for r in rows:
            w.writerow([
                "0x%06x" % r["rva"], r["length"], r["section"], r["verdict"],
                r["addressed"], r["payload_nonzero"], r["relocs"],
                r["owning_unit"], r["prev_name"],
                r["prev_object"], r["prev_name"], r["next_object"], r["next_name"],
                r["first_bytes"],
            ])
    return p


# --------------------------------------------------------------------------- #
# claims nothing points at
# --------------------------------------------------------------------------- #
def unaddressed(claims, retail):
    """Enrolled claims no relocated operand anywhere in the image names.

    A PHANTOM CANDIDATE, never a verdict on its own. Three benign families make
    up most of it and are labelled rather than filtered, so the caller can see
    the shape of its own false positives:
      * a string literal or FP slot reached as `base + k` from inside a bigger
        pooled claim - the operand names the pool, not the member;
      * an RTTI record reached from a `??_R2` base-class array we also enroll -
        the array's word IS a relocation, so this is rare but real;
      * a datum only ever reached off a neighbour's address.
    """
    ptd = retail.pointed
    out = []
    for (a, sz) in sorted({(c["rva"], c["size"]) for c in claims}):
        if any(t in ptd for t in range(a, a + sz)):
            continue
        out.append((a, sz))
    return out


# --------------------------------------------------------------------------- #
# ordinary_sections completeness
# --------------------------------------------------------------------------- #
def section_proof():
    """Per-object diagnosis of cl's ordinary (non-COMDAT) `.data`/`.rdata`.

    `data_manifest.ordinary_sections` publishes such a section only when it is
    PROVABLY complete, and silently skips it otherwise. A skipped section is a
    modelling gap by construction - some symbol cl put there has no enrolled
    definition - so this re-runs the same proof and reports WHY each one failed.
    """
    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff                       # noqa: E402
    from gruntz.build.data_manifest import (            # noqa: E402
        LNK_COMDAT, ORDINARY_STORAGE, candidates)

    base_dir = REPO / "build/objdiff/base"
    # The REAL enrollment rows, not the emitted TSV: cl spells a function-local
    # static `_s_Foo$S41554` and an FP pool slot `$T36228`, and the manifest
    # addresses both under a different public name. Only `candidates()` carries
    # the per-object `member` spelling the section proof has to key on.
    rows = candidates()[0]
    by_obj = defaultdict(dict)
    for r in rows:
        by_obj[r["object"]][r.get("member") or r["name"]] = r

    report = []
    for obj in sorted(by_obj):
        path = base_dir / (obj[:-2] + ".obj")
        if not path.exists():
            continue
        c = _Coff(path)
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None or sec["characteristics"] & LNK_COMDAT:
                continue
            members = c.section_members(sec["index"])
            if not members:
                continue
            payload = c.section_payload(sec["index"])[:sec["size"]]
            covered = bytearray(sec["size"])
            reasons = []
            for offset, name, _scl in members:
                r = by_obj[obj].get(name)
                if r is None:
                    reasons.append("unenrolled member %s @+0x%x" % (name, offset))
                    continue
                if r["storage"] != storage:
                    reasons.append("%s storage %s != %s" % (name, r["storage"], storage))
                    continue
                end = offset + r["size"]
                if end > sec["size"]:
                    reasons.append("%s overruns (+0x%x+0x%x > 0x%x)"
                                   % (name, offset, r["size"], sec["size"]))
                    continue
                if any(covered[offset:end]):
                    reasons.append("%s overlaps a sibling at +0x%x" % (name, offset))
                    continue
                covered[offset:end] = b"\1" * r["size"]
            holes = [i for i in range(sec["size"]) if not covered[i] and payload[i]]
            if holes:
                reasons.append("%d NON-ZERO uncovered byte(s), first +0x%x"
                               % (len(holes), holes[0]))
            enrolled = sorted(((offset, by_obj[obj][name])
                               for offset, name, _s in members
                               if name in by_obj[obj]), key=lambda t: t[0])
            report.append({"object": obj, "section": sec["name"],
                           "size": sec["size"], "members": len(members),
                           "ok": not reasons, "reasons": reasons,
                           "nonzero_holes": len(holes), "enrolled": enrolled})
    return report


# --------------------------------------------------------------------------- #
# calibration
# --------------------------------------------------------------------------- #
def calibrate(rows, claims, retail, report=None):
    """Both error rates, each against a control the sieve cannot see.

    A CONTROL MUST NOT BE TAUTOLOGICAL. The first attempt scored 0.00% by taking
    "bytes a placed section rebuilds" as the control - but a gap is BY
    DEFINITION outside every covered run, so no gap can ever land there. That
    measures nothing. Both controls below are independent of the coverage bitmap.

    FALSE POSITIVES - "does the sieve cry unmodelled where nothing is missing?"
      Control: the inter-member ranges of an ORDINARY candidate section that
      passes `data_manifest`'s completeness proof. That proof asserts, from the
      CANDIDATE object, that every member is enrolled and every byte between them
      is zero padding cl emitted. Retail lays those members out at the same
      relative offsets, so the corresponding retail ranges are padding too - and
      the sieve, which never looks at a candidate object, must call them PAD or
      ZERO-GAP. A NONZERO or POINTER verdict there is a false positive.
      (It is also the one place a real defect could hide - candidate padding
      where retail has content means our layout is wrong - so a hit is printed,
      not swallowed.)

    FALSE NEGATIVES - "can the sieve see the defect class at all?"
      Injection, the way `data_relocs --selftest` does it: take enrolled claims
      that are provably followed by content (retail's next byte is non-zero, or
      it is a relocated word) and shrink them. A shrunk claim is exactly the
      too-small model this tool exists to catch, so the sieve must raise a row at
      the truncated tail with the matching verdict. Missing one is a false
      negative. The injection is in-memory only; nothing is written.
    """
    proof = section_proof()

    # --- false positives ---------------------------------------------------
    # A control range must be padding by an oracle the sieve cannot consult. Two
    # conditions, both from the CANDIDATE object:
    #   1. the section passes the completeness proof, so every uncovered byte in
    #      it is zero and every member is enrolled;
    #   2. the two members are ADJACENT in that section AND retail spaced them by
    #      exactly the candidate's distance, so retail reproduced cl's layout and
    #      the retail range between them is that same padding.
    # Dropping (2) was the second bad control: a unit's COMDATs are scattered
    # across retail's `.rdata`, so "consecutive claims of one object" bracketed
    # 3 KB of OTHER units' data and the sieve was right to flag it.
    by_verdict = {r["rva"]: r for r in rows}
    control, fp = [], []
    for p in proof:
        if not p["ok"]:
            continue
        for (o1, c1), (o2, c2) in zip(p["enrolled"], p["enrolled"][1:]):
            cand_gap = o2 - (o1 + c1["size"])
            if cand_gap <= 0:
                continue
            end, nxt = c1["rva"] + c1["size"], c2["rva"]
            if nxt - end != cand_gap:
                continue                        # retail did not keep the layout
            control.append((end, cand_gap))
            r = by_verdict.get(end)
            if r and r["verdict"] in ("NONZERO", "POINTER"):
                fp.append(r)

    # --- false negatives (injection) ---------------------------------------
    # Shrink every eligible claim by its last dword and ask whether the sieve
    # would raise the truncated tail. Done incrementally rather than by rebuilding
    # `gaps()` 3000 times: the tail becomes an uncovered range exactly when no
    # OTHER claim or section already covers it, and the verdict then follows from
    # retail's payload and relocations at that range - the same rule `gaps()`
    # applies. An UNSEEN row is a genuine blind spot: some other claim overlaps
    # the truncated tail, so shrinking one model hides behind another.
    inj_total, inj_caught, inj_missed = 0, 0, []
    extents = sorted({(c["rva"], c["size"]) for c in claims})
    others = []
    for a, sz in extents:
        others.append((a, a + sz))
    ostart = [a for a, _ in others]

    def covered_by_other(lo_, hi_, skip):
        i = bisect.bisect_right(ostart, lo_) - 1
        while i >= 0 and others[i][1] > lo_:
            if others[i] != skip and others[i][1] >= hi_:
                return True
            i -= 1
        return False

    for a, sz in extents:
        if sz < 8:
            continue
        tail = a + sz - 4
        pay = retail.payload(tail, 4)
        rel = retail.relocs_in(tail, 4)
        if not any(pay) and not rel:
            continue                            # a zero tail is not visible content
        inj_total += 1
        if covered_by_other(tail, a + sz, (a, a + sz)):
            inj_missed.append((a, sz, "hidden by an overlapping claim"))
            continue
        inj_caught += 1

    return {"control_ranges": len(control),
            "control_bytes": sum(n for _, n in control),
            "false_positives": len(fp), "fp_rows": fp,
            "injected": inj_total, "caught": inj_caught, "missed": inj_missed,
            "pad_addressed": [r for r in rows
                              if r["verdict"] == "PAD" and r["addressed"]]}


# --------------------------------------------------------------------------- #
def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--tsv", nargs="?", const="build/gen/data_coverage_gaps.tsv",
                    help="write the join-shaped gap census")
    ap.add_argument("--sections", action="store_true",
                    help="ordinary_sections completeness, with failure reasons")
    ap.add_argument("--overlaps", action="store_true")
    ap.add_argument("--unaddressed", action="store_true",
                    help="claims no relocation anywhere names (phantom candidates)")
    ap.add_argument("--calibrate", action="store_true")
    ap.add_argument("--near", help="dump the claims and gaps around one rva")
    ap.add_argument("--verdict", help="only rows with this verdict")
    ap.add_argument("--min-len", type=int, default=0)
    ap.add_argument("--max-len", type=int, default=1 << 30)
    ap.add_argument("--unit", help="only rows bracketed by this object")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--limit", type=int, default=40)
    a = ap.parse_args(argv)

    claims = load_claims()
    sections = load_sections()
    retail = Retail()

    if a.near:
        return _near(int(a.near, 0), claims, retail, sections=sections)

    if a.sections:
        rep = section_proof()
        good = [r for r in rep if r["ok"]]
        bad = [r for r in rep if not r["ok"]]
        print("ordinary sections: %d complete / %d total  (%d incomplete)"
              % (len(good), len(rep), len(bad)))
        for r in sorted(bad, key=lambda r: -r["nonzero_holes"])[:a.limit]:
            print("  %-28s %-8s size=0x%-5x members=%-4d" %
                  (r["object"], r["section"], r["size"], r["members"]))
            for why in r["reasons"][:4]:
                print("      %s" % why)
        return 0

    if a.overlaps:
        ov = overlaps(claims)
        print("overlapping claims (different extents sharing a byte): %d" % len(ov))
        for (a1, s1, c1), (a2, s2, c2) in ov[:a.limit]:
            print("  0x%06x+0x%-5x %-44s [%s]" % (a1, s1, c1[0]["name"][:44],
                                                  c1[0]["object"]))
            print("  0x%06x+0x%-5x %-44s [%s]" % (a2, s2, c2[0]["name"][:44],
                                                  c2[0]["object"]))
        return 1 if ov else 0

    rows = gaps(claims, retail, sections)

    if a.unaddressed:
        ua = unaddressed(claims, retail)
        byname = defaultdict(list)
        for c in claims:
            byname[(c["rva"], c["size"])].append(c)
        print("claims no relocated operand names: %d of %d distinct extents"
              % (len(ua), len({(c['rva'], c['size']) for c in claims})))
        kinds = Counter()
        for rva, sz in ua:
            n = byname[(rva, sz)][0]["name"]
            kinds["string" if n.startswith("??_C@") else
                  "rtti" if n.startswith("??_R") else
                  "vtable" if n.startswith("??_7") else
                  "fp-pool" if n.startswith("$T") or "$S" in n else "OTHER"] += 1
        print("  by kind:", dict(kinds))
        for rva, sz in ua[:a.limit]:
            c = byname[(rva, sz)][0]
            if c["name"].startswith(("??_C@", "??_R", "??_7")):
                continue
            print("  0x%06x+0x%-5x %-50s [%s]" % (rva, sz, c["name"][:50],
                                                  c["object"]))
        return 0

    if a.calibrate:
        cal = calibrate(rows, claims, retail)
        fpr = 100.0 * cal["false_positives"] / max(1, cal["control_ranges"])
        fnr = 100.0 * len(cal["missed"]) / max(1, cal["injected"])
        print("CALIBRATION")
        print("  FALSE POSITIVES - control: inter-member ranges of an ordinary")
        print("  section that passes data_manifest's completeness proof, i.e.")
        print("  bytes an INDEPENDENT oracle says are cl's zero padding.")
        print("    control ranges : %d  (%d B)"
              % (cal["control_ranges"], cal["control_bytes"]))
        print("    NONZERO/POINTER verdicts there : %d  -> FP rate %.2f%%"
              % (cal["false_positives"], fpr))
        for r in cal["fp_rows"][:a.limit]:
            print("      0x%06x len=%-5d %-8s %s / %s"
                  % (r["rva"], r["length"], r["verdict"], r["prev_name"][:34],
                     r["next_name"][:34]))
        print("  FALSE NEGATIVES - control: enrolled claims shrunk by their last")
        print("  dword (an injected too-small model), in memory only.")
        print("    injected : %d   caught : %d   missed : %d  -> FN rate %.2f%%"
              % (cal["injected"], cal["caught"], len(cal["missed"]), fnr))
        for a_, sz, v in cal["missed"][:a.limit]:
            print("      0x%06x+0x%-5x %s" % (a_, sz, v))
        print("  CROSS-CHECK - PAD ranges the image itself points into (a PAD")
        print("  verdict there is the sieve calling a live datum padding):")
        print("    %d of %d PAD rows" % (len(cal["pad_addressed"]),
                                         sum(1 for r in rows
                                             if r["verdict"] == "PAD")))
        for r in cal["pad_addressed"][:a.limit]:
            print("      0x%06x len=%-3d addressed=%d  %s -> %s"
                  % (r["rva"], r["length"], r["addressed"],
                     r["prev_name"][:34], r["next_name"][:34]))
        return 0

    sel = [r for r in rows
           if a.min_len <= r["length"] <= a.max_len
           and (not a.verdict or r["verdict"] == a.verdict)
           and (not a.unit or a.unit in (r["prev_object"], r["next_object"]))]

    if a.tsv:
        p = write_tsv(rows, a.tsv)
        print("wrote %s (%d rows, join key rva+length)" % (p, len(rows)))

    if a.json:
        print(json.dumps(sel, indent=1))
        return 0

    tally = Counter(r["verdict"] for r in rows)
    bytes_by = Counter()
    for r in rows:
        bytes_by[r["verdict"]] += r["length"]
    covered = sum(b - x for x, b in coverage(claims, sections))
    print("enrolled claims %d (%d distinct extents) + %d placed sections, "
          "covering %d B" % (len(claims), len({(c['rva'], c['size'])
                                               for c in claims}),
                             len(sections), covered))
    print("uncovered interior ranges: %d, %d B" % (len(rows), sum(bytes_by.values())))
    for v in ("POINTER", "NONZERO", "ZERO-GAP", "PAD"):
        print("  %-9s %5d ranges  %9d B  %5d addressed"
              % (v, tally[v], bytes_by[v],
                 sum(r["addressed"] for r in rows if r["verdict"] == v)))
    print()
    print("worklist (verdict != PAD, sorted by addressed then nonzero):")
    work = sorted([r for r in sel if r["verdict"] != "PAD"],
                  key=lambda r: (-r["addressed"], -r["payload_nonzero"]))
    for r in work[:a.limit]:
        print("  0x%06x len=%-6d %-8s addr=%-4d nz=%-5d rel=%-4d %s"
              % (r["rva"], r["length"], r["verdict"], r["addressed"],
                 r["payload_nonzero"], r["relocs"],
                 r["owning_unit"] if r["owning_unit"] != "-"
                 else "%s | %s" % (r["prev_object"], r["next_object"])))
        print("      %-46s -> %s" % (r["prev_name"][:46], r["next_name"][:46]))
    return 0


def _near(rva, claims, retail, span=0x80, sections=()):
    lo, hi = rva - span, rva + span
    print("=== claims overlapping [0x%06x, 0x%06x) ===" % (lo, hi))
    for c in sorted(claims, key=lambda c: (c["rva"], c["name"])):
        if c["rva"] + c["size"] <= lo or c["rva"] >= hi:
            continue
        print("  0x%06x+0x%-5x %-8s %-46s [%s]"
              % (c["rva"], c["size"], c["storage"], c["name"][:46], c["object"]))
    hits = [s for s in sections
            if s["rva"] < hi and s["rva"] + s["size"] > lo]
    if hits:
        print("=== placed candidate sections here ===")
        for s in sorted(hits, key=lambda s: s["rva"]):
            print("  0x%06x+0x%-5x %-8s %s"
                  % (s["rva"], s["size"], s["name"], s["object"]))
    print("=== retail payload ===")
    pay = retail.payload(lo, hi - lo)
    rel = set(retail.relocs_in(lo, hi - lo))
    ptd = set(retail.pointed_in(lo, hi - lo))
    for off in range(0, len(pay), 16):
        at = lo + off
        marks = "".join("R" if (at + i) in rel else
                        "*" if (at + i) in ptd else " " for i in range(16))
        print("  0x%06x  %-47s  |%s|" % (at, pay[off:off + 16].hex(" "), marks))
    print("  (R = retail relocates this word, * = something points AT this address)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
