#!/usr/bin/env python3
"""inline_clones.py - find SHARED INLINE HELPERS we transcribed as duplicated code.

A small header inline (`GetRandomNumber`, a clamp, a fixed-point scale) is expanded by
the compiler at every call site.  Whoever reconstructed each TU in isolation saw only
the expansion, so they open-coded it - once per site.  The result is N near-identical
blocks in N different TUs where the shipped source had ONE function called N times.
That is a systematic modelling defect, and this tool is how you find the population.

Two independent detectors over the retail `.text`, because they fail differently:

  **--consts**  a histogram of every integer constant that is NOT an address (the
    `.reloc` table is the oracle: a relocated imm32 is a pointer, never a constant).
    A rare value appearing in many unrelated functions is a helper's fingerprint -
    this is what made `GetRandomNumber` findable via `0x269ec3` (= 2531011L).
    Cheap and high precision; blind to helpers with no distinctive constant.

  **--ngrams**  normalized instruction-sequence clustering.  Every instruction is
    reduced to a shape token - registers alpha-renamed by first appearance inside the
    window, frame displacements and relocated addresses and branch targets masked,
    immediates and struct displacements KEPT (they are the discriminators).  Windows
    of `-n` consecutive tokens are hashed; a hash occurring in several unrelated
    functions is a candidate inlined body.  Catches constant-free helpers; pays for it
    with a long tail of false positives (prologues, MFC container walks, CRT idioms).

False-positive profile, both detectors:
  - compiler/CRT boilerplate (`__ehhandler` frames, `_chkstk`, `rep movs` inlining)
  - MFC inline accessors (`CPtrArray::GetAt`, `GetNext`) - real inlines, but library
  - genuine per-site duplication the devs wrote by hand (copy-paste in one subsystem)
  - a helper we ALREADY model as a shared function - the cluster is a true positive
    for the detector and needs no fix.  Those are the calibration points.

Usage:
    python -m gruntz.audit.inline_clones --consts               # ranked constants
    python -m gruntz.audit.inline_clones --consts --value 0x269ec3
    python -m gruntz.audit.inline_clones --ngrams -n 8 --min-fns 4
    python -m gruntz.audit.inline_clones --show <cluster-id>    # the motif + sites
    python -m gruntz.audit.inline_clones --src <rva> [<rva>...] # what src/ has there
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import pickle
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core import retail_functions
from gruntz.core.pe import PE, IMAGEBASE

REPO = Path(__file__).resolve().parents[3]
CACHE = REPO / "build/gen/inline_clones.pkl"

LINE = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*\t(.*)$")
REGS = re.compile(
    r"\b(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh]|"
    r"st\(\d\)|mm\d|xmm\d+|[cd]s|[es]s|fs|gs)\b")
NUM = re.compile(r"0x[0-9a-f]+")
BRANCH = re.compile(r"^(j\w+|call|loop\w*)$")
FRAME = re.compile(r"\[(esp|ebp)\b[^\]]*\]")

# Constants so common they carry no identity: small ints, byte/word masks, the
# power-of-two shift/size family, and the ubiquitous -1.
BORING = {v for v in range(0, 65)} | {
    0xff, 0xffff, 0xffffffff, 0xfffffffe, 0xfffffffd, 0xfffffffc, 0xffffff00,
    0xffff0000, 0x7f, 0x7fff, 0x7fffffff, 0x80, 0x8000, 0x80000000, 0x100,
    0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000,
    0x40000, 0x80000, 0x100000, 0x1000000, 0x3ff, 0xfff, 0x3fff, 0xffffff,
}


# ---------------------------------------------------------------- corpus ----
def _objdump(exe: Path) -> str:
    cmd = ["llvm-objdump", "-d", "-M", "intel", "--section=.text", str(exe)]
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def build_corpus(exe: Path | None = None) -> dict:
    """Disassemble retail `.text` once and bucket it into admitted functions."""
    pe = PE(exe) if exe else PE()
    insns = []
    for ln in _objdump(pe.path).splitlines():
        m = LINE.match(ln)
        if not m:
            continue
        raw = bytes.fromhex(m.group(2).replace(" ", ""))
        txt = re.sub(r"\s*<[^>]*>", "", m.group(3).split("#")[0].rstrip())
        parts = txt.split(None, 1)
        insns.append((int(m.group(1), 16) - IMAGEBASE, raw,
                      parts[0] if parts else "",
                      parts[1].strip() if len(parts) > 1 else ""))
    funcs = retail_functions.read()
    starts = [f["rva"] for f in funcs]
    # per-function instruction slices, by linear scan (99.7% of starts align)
    at = {ins[0]: i for i, ins in enumerate(insns)}
    bodies = {}
    for f in funcs:
        i = at.get(f["rva"])
        if i is None:
            continue
        end = f["rva"] + f["size"]
        j = i
        while j < len(insns) and insns[j][0] < end:
            j += 1
        bodies[f["rva"]] = (i, j)
    return {"insns": insns, "funcs": funcs, "starts": starts, "bodies": bodies,
            "relocs": set(pe.reloc_sites)}


def corpus(refresh: bool = False) -> dict:
    if not refresh and CACHE.exists():
        try:
            return pickle.loads(CACHE.read_bytes())
        except Exception:
            pass
    data = build_corpus()
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_bytes(pickle.dumps(data, protocol=4))
    return data


def owner_of(c: dict, rva: int):
    i = bisect.bisect_right(c["starts"], rva) - 1
    if i < 0:
        return None
    f = c["funcs"][i]
    return f if rva < f["rva"] + f["size"] else None


# ------------------------------------------------------------ detector 1 ----
def is_address(c: dict, rva: int, raw: bytes, value: int) -> bool:
    """True when `value` sits at a base-relocation site inside this instruction."""
    le = value.to_bytes(4, "little")
    off = raw.find(le)
    while off != -1:
        if rva + off in c["relocs"]:
            return True
        off = raw.find(le, off + 1)
    return False


BRACKET = re.compile(r"\[[^\]]*\]")
DISP_IS_CONST = 0x8000     # a non-relocated displacement this large is arithmetic


def numerics(rva: int, raw: bytes, ops: str):
    """Yield (value, kind) for each numeric operand: 'imm' or 'disp'.

    A bracketed number is a memory displacement - usually a struct offset, which is
    everywhere and carries no identity.  Except when it is large: cl strength-reduces
    `x*K + C` into `lea r, [r + s*r + C]`, so the ADDEND of a magic multiply hides in
    a displacement field.  That is exactly where `GetRandomNumber`'s 0x269ec3 lives.
    """
    inner = BRACKET.findall(ops)
    outer = BRACKET.sub("", ops)
    for tok in NUM.findall(outer):
        yield int(tok, 16), "imm"
    for chunk in inner:
        for tok in NUM.findall(chunk):
            v = int(tok, 16)
            yield v, ("imm" if v >= DISP_IS_CONST else "disp")


def unusual(v: int) -> bool:
    """A value that cannot pass for a size, a count, a mask or a struct offset."""
    if v < 0x100 or v in BORING:
        return False
    if v > 0xfffff000:                      # small negatives
        return False
    if v % 4 == 0 and v < 0x10000:          # aligned offset / buffer size
        return False
    if v % 100 == 0 or v % 1024 == 0:       # round decimal / binary size
        return False
    return True


def constants(c: dict, kinds=("imm",)):
    """value -> [instruction index], for every non-address numeric operand."""
    hits = defaultdict(list)
    for i, (rva, raw, mnem, ops) in enumerate(c["insns"]):
        if not ops or BRANCH.match(mnem):
            continue                  # branch/call targets are not constants
        for v, kind in numerics(rva, raw, ops):
            if kind not in kinds or v in BORING:
                continue
            if is_address(c, rva, raw, v):
                continue
            hits[v].append(i)
    return hits


def cmd_consts(c: dict, args):
    hits = constants(c, ("imm", "disp") if args.all_kinds else ("imm",))
    rows = []
    for v, idxs in hits.items():
        if not args.all_kinds and not unusual(v):
            continue
        fns = {}
        for i in idxs:
            f = owner_of(c, c["insns"][i][0])
            if f:
                fns.setdefault(f["rva"], []).append(c["insns"][i][0])
        if len(fns) < args.min_fns:
            continue
        rows.append((len(fns), len(idxs), v, fns))
    rows.sort(key=lambda r: (-r[0], -r[1]))
    cl = claims()
    print(f"# unusual constants in >= {args.min_fns} distinct functions "
          f"({len(rows)} candidates); `game` counts sites outside the CRT/MFC band")
    print(f"{'value':>12}  {'fns':>4} {'game':>4} {'units':>5}  first game sites")
    out = []
    for nf, nu, v, fns in rows:
        game = [r for r in fns if (cl.get(r) or {}).get("unit")]
        units = {cl[r]["unit"] for r in game}
        if len(game) < args.min_fns:
            continue
        out.append((len(game), len(units), nf, v, game))
    out.sort(key=lambda r: (-r[0], -r[1]))
    for ng, nun, nf, v, game in out[:args.limit]:
        sites = " ".join(f"0x{r:06x}" for r in sorted(game)[:6])
        print(f"{v:>12} = 0x{v:<8x} {nf:>4} {ng:>4} {nun:>5}  {sites}")


def cmd_value(c: dict, args):
    v = int(args.value, 0)
    hits = constants(c, ("imm", "disp")).get(v, [])
    by = defaultdict(list)
    for i in hits:
        f = owner_of(c, c["insns"][i][0])
        by[f["rva"] if f else None].append(c["insns"][i])
    print(f"# {v} (0x{v:x}) used at {len(hits)} sites in {len(by)} functions")
    for fr in sorted(by, key=lambda x: (x is None, x)):
        tag = describe(c, fr) if fr is not None else "(unowned)"
        print(f"\n=== {tag} ===")
        for rva, raw, mnem, ops in by[fr]:
            print(f"  0x{rva:06x}  {mnem:<8} {ops}")


# ------------------------------------------------------------ detector 2 ----
def shape(c: dict, i: int) -> tuple[str, tuple[str, ...]]:
    """(template, registers) - the instruction with regalloc factored out.

    Masked: relocated absolute addresses (`A`), branch/call targets (`L`), frame
    displacements (`S`).  Kept: immediates and non-frame displacements - a struct
    offset or a magic number is exactly the signal we are clustering on.
    """
    rva, raw, mnem, ops = c["insns"][i]
    if BRANCH.match(mnem):
        return (mnem + " L", ())
    ops = FRAME.sub("[S]", ops)
    out, regs = [], []

    def reg(m):
        name = m.group(0)
        if name not in regs:
            regs.append(name)
        return f"\x01{regs.index(name)}\x02"

    ops = REGS.sub(reg, ops)
    for tok in NUM.findall(ops):
        if is_address(c, rva, raw, int(tok, 16)):
            ops = ops.replace(tok, "A")
    return (mnem + " " + ops, tuple(regs))


def shingles(c: dict, n: int):
    """hash -> [start instruction index], over windows that stay inside one fn."""
    shapes = [shape(c, i) for i in range(len(c["insns"]))]
    buckets = defaultdict(list)
    for fr, (lo, hi) in c["bodies"].items():
        for s in range(lo, hi - n + 1):
            names, key = {}, []
            for k in range(s, s + n):
                tmpl, regs = shapes[k]
                for r in regs:
                    names.setdefault(r, len(names))
                key.append(re.sub(r"\x01(\d+)\x02",
                                  lambda m: f"r{names[regs[int(m.group(1))]]}",
                                  tmpl))
            buckets["\n".join(key)].append(s)
    return buckets, shapes


# Instructions that carry no design intent: the frame, the call ABI, the flow.
FILLER = {"push", "pop", "mov", "ret", "call", "jmp", "leave", "nop", "int3", "movzx",
          "movsx", "lea"}
# Real computation - the tell that a motif is a HELPER BODY, not glue.
COMPUTE = {"imul", "idiv", "div", "mul", "sar", "shr", "shl", "and", "or", "xor",
           "neg", "not", "cdq", "add", "sub", "inc", "dec", "adc", "sbb", "test",
           "cmp", "setz", "sete", "setne", "sbb", "rol", "ror"}
# Compiler-generated shapes that dominate any clone search on an /O2 /GX MSVC image.
BOILER = (
    ("fs:[0x0]", "SEH frame"),
    ("push -0x1", "SEH frame"),
    ("test byte ptr [S], 0x1", "scalar deleting destructor"),
    ("rep movs", "inline memcpy"),
    ("repne scas", "inline strlen"),
    ("rep stos", "inline memset"),
)


def score(key: str):
    """(interest, why) - how much a motif looks like a hand-written helper body."""
    lines = key.splitlines()
    for probe, why in BOILER:
        if any(probe in ln for ln in lines):
            return 0.0, why
    mnems = [ln.split(None, 1)[0] for ln in lines]
    compute = sum(1 for m in mnems if m in COMPUTE)
    filler = sum(1 for m in mnems if m in FILLER)
    if compute == 0:
        return 0.0, "no computation"
    # distinct non-address immediates are the strongest single signal
    imms = {int(t, 16) for ln in lines for t in NUM.findall(BRACKET.sub("", ln))}
    rare = sum(1 for v in imms if unusual(v))
    return compute / max(len(lines), 1) + 0.5 * rare - 0.2 * filler / len(lines), ""


def cmd_ngrams(c: dict, args):
    buckets, _ = shingles(c, args.n)
    cl = claims()
    rows = []
    for key, starts in buckets.items():
        fns = {owner_of(c, c["insns"][s][0])["rva"] for s in starts}
        if args.game_only:
            fns = {r for r in fns if (cl.get(r) or {}).get("unit")}
        if len(fns) < args.min_fns:
            continue
        interest, why = score(key)
        if interest <= 0 and not args.all_kinds:
            continue
        rows.append((interest * len(fns), len(fns), len(starts), key, sorted(fns)))
    rows.sort(key=lambda r: -r[0])
    print(f"# {args.n}-gram clusters in >= {args.min_fns} distinct functions, "
          f"ranked by interest x spread ({len(rows)} survive the boilerplate filter)")
    shown, seen = 0, []
    for rank, nf, ns, key, fns in rows:
        # positional dedup: a motif yields ~n overlapping windows over the same code
        sig = set(fns)
        if any(len(sig & prev) >= 0.8 * len(sig) for prev in seen):
            continue
        seen.append(sig)
        cid = hashlib.sha1(key.encode()).hexdigest()[:10]
        units = sorted({cl[r]["unit"] for r in fns if (cl.get(r) or {}).get("unit")})
        print(f"\n--- {cid}  rank {rank:.1f}  {nf} fns / {len(units)} units, "
              f"{ns} sites ---")
        print("    " + "\n    ".join(key.splitlines()))
        print("    @ " + " ".join(f"0x{r:06x}" for r in fns[:8]))
        print("    units: " + " ".join(units[:12]))
        shown += 1
        if shown >= args.limit:
            break


def cmd_show(c: dict, args):
    buckets, _ = shingles(c, args.n)
    for key, starts in buckets.items():
        if hashlib.sha1(key.encode()).hexdigest()[:10] != args.show:
            continue
        print(f"# cluster {args.show}: {len(starts)} sites")
        for s in starts:
            f = owner_of(c, c["insns"][s][0])
            print(f"\n=== 0x{c['insns'][s][0]:06x}  in fn 0x{f['rva']:06x} ===")
            for k in range(s, s + args.n):
                rva, _, mnem, ops = c["insns"][k]
                print(f"  0x{rva:06x}  {mnem:<8} {ops}")
        return
    print(f"no cluster {args.show} at n={args.n}", file=sys.stderr)


# --------------------------------------------------------------- cross-ref ---
_CLAIMS = None


def claims() -> dict:
    """rva -> {name, unit, lib} from the build's symbol map + the library map.

    A cluster is only worth fixing where the sites are OUR code; `lib` marks the
    CRT/MFC band, which is the detector's largest false-positive family.
    """
    global _CLAIMS
    if _CLAIMS is not None:
        return _CLAIMS
    import csv as _csv
    out = {}
    src = REPO / "build/gen/symbol_names.csv"
    if src.exists():
        with src.open(encoding="utf-8", newline="") as fh:
            for row in _csv.DictReader(fh):
                out[int(row["rva"], 0)] = {"name": row["name"], "unit": row["unit"],
                                           "lib": ""}
    lib = REPO / "config/retail/library_labels.csv"
    if lib.exists():
        with lib.open(encoding="utf-8", newline="") as fh:
            for row in _csv.DictReader(fh):
                rva = int(row["rva"], 0)
                if rva in out:
                    out[rva]["lib"] = row["lib"]
                else:
                    out[rva] = {"name": row["name"], "unit": "", "lib": row["lib"]}
    _CLAIMS = out
    return out


def describe(c: dict, rva: int) -> str:
    f = owner_of(c, rva)
    base = f["rva"] if f else rva
    row = claims().get(base)
    if not row:
        return f"0x{base:06x} (unclaimed)"
    tag = f"[{row['unit']}]" if row["unit"] else f"<{row['lib']}>"
    return f"0x{base:06x} {row['name']} {tag}"


def cmd_src(c: dict, args):
    """Name the src/ claim at each RVA, so a cluster maps to files to fix."""
    for tok in args.src:
        print(describe(c, int(tok, 0)))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--consts", action="store_true", help="rare-constant histogram")
    ap.add_argument("--ngrams", action="store_true", help="shingle clustering")
    ap.add_argument("--value", help="dump every use of one constant")
    ap.add_argument("--show", help="dump one --ngrams cluster id")
    ap.add_argument("--src", nargs="*", help="name the src/ claim at these RVAs")
    ap.add_argument("-n", type=int, default=8, help="shingle length (default 8)")
    ap.add_argument("--min-fns", type=int, default=3,
                    help="minimum distinct functions (default 3)")
    ap.add_argument("--limit", type=int, default=60)
    ap.add_argument("--all-kinds", action="store_true",
                    help="do not filter to unusual immediates / boilerplate motifs")
    ap.add_argument("--game-only", action="store_true", default=True,
                    help="count only sites with a src/ claim (default)")
    ap.add_argument("--with-library", dest="game_only", action="store_false",
                    help="include the CRT/MFC band")
    ap.add_argument("--refresh", action="store_true", help="rebuild the disasm cache")
    args = ap.parse_args(argv)

    c = corpus(refresh=args.refresh)
    if args.value:
        return cmd_value(c, args)
    if args.show:
        return cmd_show(c, args)
    if args.src is not None:
        return cmd_src(c, args)
    if args.ngrams:
        return cmd_ngrams(c, args)
    return cmd_consts(c, args)


if __name__ == "__main__":
    sys.exit(main() or 0)
