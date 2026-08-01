"""gruntz.core.branches - the ordered branch sequence of a function, on both sides.

**Why this exists.** `gruntz sema disasm --diff` masks every address operand so that
reloc-bound call/jump targets do not show up as spurious diffs. That masking also hides
**intra-function conditional-branch displacements**: a `je` that goes to a different
basic block on the two sides is printed `je <tgt>` on both and compares EQUAL. So a
genuine divergence in control flow can render as "identical asm" while the function sits
below 100%.

`CDDrawSurfaceMgr::SetDimensions` 0x155f60 was parked at 99.88% with "identical asm";
the whole bug was `74 2e` vs `74 16` - retail's early-out skipped the level rebuild, we
ran it unconditionally on every no-op resize. See
docs/patterns/masked-diff-hides-branch-target.md.

**DO NOT "FIX" `--diff` BY UNMASKING DISPLACEMENTS.** The masking is right for what it
does: every function whose instruction sizes differ anywhere upstream would grow a
`-`/`+` on every single branch, which is exactly the noise the masking exists to remove.
The answer is a comparison that names branch targets SYMBOLICALLY - which is this module
- not one that stops masking. Two consumers share it so there is no logic to drift:

    gruntz sema disasm <rva> --branches [--diff]   # one function, interactively
    python -m gruntz.audit.jcc_sieve               # the whole tree, as a worklist

## What a hit is NOT (the same caveats both consumers print)

**Differing branch COUNTS are not this signal.** An extra or missing conditional jump is
a structural difference - a whole basic block we did not reconstruct, an `if` the
optimizer folded, an inlining decision. Those need a normal reconstruction pass, not a
one-line condition fix, and they swamp a listing.

**Constant displacement shifts are noise.** If our side is a few bytes longer upstream,
every subsequent branch target shifts by the same amount while the control flow is
identical. `sym_target` therefore names each target by the index of the first branch at
or after it, so a uniform shift compares equal. The residue this cannot see: two blocks
that both sit past the last branch (both normalize to "past the end"), so TOPOLOGY
under-reports at the epilogue.

**More than ~4 flips is not this signal either.** At that point the two functions are
differently shaped and the positional pairing is meaningless; a 30-branch function with
11 flips is telling you the reconstruction is wrong, not that a comparison is signed.

**A truncated stream is not a clean result.** llvm-objdump disassembles linearly, and a
jump table embedded in `.text` after an indirect `jmp` decodes as garbage. `first_bad()`
finds where that starts; `branches()` stops there so the list it returns is a reliable
PREFIX, and callers must say so rather than printing a short list that looks complete.

**A hit is a lead, not a verdict.** cl picks the branch polarity that suits its block
layout, so a POLARITY row can be pure codegen preference (see
docs/patterns/positive-gate-enables-shrink-wrap.md - count the `ret`s first, which is
what `rets()` is for). SIGNEDNESS is the half that is nearly always a real source bug.
"""
import re
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
BASE_DIR = REPO / "build" / "objdiff" / "base"
TGT_DIR = REPO / "build" / "objdiff" / "target"

# A WHITELIST, not `j` minus `jmp`: llvm-objdump spells the indirect jump `jmpl`, which
# a `(?!mp\b)` exclusion lets through (`\b` fails before the `l`) - that put unconditional
# jumps into the branch sequence and produced a `jne->jmpl` "flip". This is the complete
# set observed over every base+target obj in the tree; `jcxz`/`loop*` never appear in
# MSVC5 /O2 output and an unexpected spelling raises rather than being dropped silently.
JCC = frozenset("je jne jl jle jg jge ja jae jb jbe js jns jo jno jp jnp".split())
JUMPY = re.compile(r"^j[a-z]+$")
UNCOND = frozenset(("jmp", "jmpl", "jmpw"))

SYM_HDR = re.compile(r"^[0-9a-f]{8} <(.+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
TGT_OP = re.compile(r"^(0x[0-9a-f]+)\b")

# The signed/unsigned twins. x86 has two condition families over the same flags; picking
# the wrong one is a source-level type bug, not a codegen choice.
SIGNED_TWIN = {
    "jl": "jb", "jb": "jl",
    "jle": "jbe", "jbe": "jle",
    "jg": "ja", "ja": "jg",
    "jge": "jae", "jae": "jge",
}

# The inverse of each condition: same test, opposite sense.
INVERSE = {
    "je": "jne", "jne": "je",
    "jl": "jge", "jge": "jl", "jle": "jg", "jg": "jle",
    "jb": "jae", "jae": "jb", "jbe": "ja", "ja": "jbe",
    "js": "jns", "jns": "js",
    "jo": "jno", "jno": "jo",
    "jp": "jnp", "jnp": "jp",
}


def obj_paths(unit):
    """The (base, target) object pair objdiff compares for one unit.

    Note the asymmetric `.c`: the delinker names its output `<unit>.c.obj`."""
    return BASE_DIR / (unit + ".obj"), TGT_DIR / (unit + ".c.obj")


def decode(obj, symbol=None):
    """Linear disassembly of `obj`: {symbol: [(offset, mnemonic, operand)]}.

    Offsets are relative to each symbol's first instruction, so the two sides are
    directly comparable without knowing either one's placement.

    Without `symbol` this decodes the WHOLE object in one llvm-objdump - a unit has up to
    a few hundred functions and the per-symbol form costs a process each. Pass `symbol`
    for the interactive single-function case."""
    cmd = ["llvm-objdump", "-d", "--no-show-raw-insn", str(obj)]
    if symbol:
        cmd.append("--disassemble-symbols=" + symbol)
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    syms, cur, base = {}, None, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            cur, base = syms.setdefault(m.group(1), []), None
            continue
        if cur is None:
            continue
        m = INSN.match(ln)
        if not m:
            continue
        off, mn, op = int(m.group(1), 16), m.group(2), m.group(3).strip()
        if base is None:
            base = off
        # Rebase the branch TARGET too, not just the offset. Leaving the target absolute
        # while the offset is relative makes `sym_target` compare two different address
        # spaces - which silently turned every function into a TOPOLOGY hit (5 -> 298)
        # the first time this was refactored. Offsets and targets live in ONE space.
        if JUMPY.match(mn):
            t = TGT_OP.match(op)
            if t:
                op = hex(int(t.group(1), 16) - base) + op[t.end():]
        cur.append((off - base, mn, op))
    return syms


def first_bad(insns):
    """Offset where llvm-objdump stopped decoding real instructions, or None.

    A jump table embedded in `.text` after an indirect `jmp` is the usual cause: the
    table bytes decode as `(bad)` or as nonsense, and everything after that point is
    unreliable. Callers must SAY the list is partial - a silently short branch list on
    exactly the functions people most need is why the block view is not trusted."""
    for off, mn, _ in insns:
        if mn == "(bad)" or mn.startswith("<"):
            return off
    return None


def branches(insns, stop=None):
    """The ordered conditional-branch list: [(offset, mnemonic, target|None)].

    `stop` (see `first_bad`) truncates the walk so the result is a reliable prefix.
    A target that is not a plain address is kept as None so the sequence length stays
    honest."""
    out = []
    for off, mn, op in insns:
        if stop is not None and off >= stop:
            break
        if mn in JCC:
            m = TGT_OP.match(op)
            out.append((off, mn, int(m.group(1), 16) if m else None))
        elif JUMPY.match(mn) and mn not in UNCOND:
            raise SystemExit(
                "gruntz.core.branches: unclassified jump mnemonic %r - add it to JCC "
                "or UNCOND, do not let it fall through" % mn)
    return out


def rets(insns, stop=None):
    """How many `ret` the side has.

    docs/patterns/positive-gate-enables-shrink-wrap.md turns this count into the rewrite:
    base > target means an exit block is duplicated that retail tail-merges (write the
    positive gate / shared exit). base < target is the INVERSE direction and is a wall -
    cl5 tail-merges identical epilogues regardless of source position; read it as
    diagnosed, not actionable. Equal counts mean the gate spelling is already right and
    the polarity has another cause."""
    return sum(1 for off, mn, _ in insns
               if mn.startswith("ret") and (stop is None or off < stop))


def sym_target(brs, tgt):
    """Name a branch target symbolically: the index of the first branch at or after it.

    This is what makes a uniform displacement shift compare EQUAL - the thing the module
    docstring calls noise. `len(brs)` means "past the last branch"."""
    if tgt is None:
        return None
    for i, (off, _, _) in enumerate(brs):
        if off >= tgt:
            return i
    return len(brs)


def classify(a, b):
    """SIGNEDNESS / POLARITY / OTHER for one mnemonic pair (base, target)."""
    if SIGNED_TWIN.get(a) == b:
        return "SIGNEDNESS"
    if INVERSE.get(a) == b:
        return "POLARITY"
    return "OTHER"


def compare(bi, ti, max_flips=4):
    """Compare two instruction streams' branch sequences.

    Returns a dict both consumers render:
      status  'flips' | 'topology' | 'clean' | 'struct' | 'many-flips' | 'no-branches'
      kind    SIGNEDNESS / POLARITY / OTHER / TOPOLOGY (only for flips/topology)
      rows    [(index, base_mnemonic, target_mnemonic)] for flips, or
              [(index, base_block, target_block)] for topology
      nbr     branch count (both sides agree whenever status is not 'struct')
      rets    (base, target)
      trunc   (base_offset|None, target_offset|None) - the stream truncation points
    """
    bstop, tstop = first_bad(bi), first_bad(ti)
    bb, tb = branches(bi, bstop), branches(ti, tstop)
    res = {"kind": None, "rows": [], "nbr": len(bb), "nbr_t": len(tb),
           "rets": (rets(bi, bstop), rets(ti, tstop)), "trunc": (bstop, tstop)}
    if len(bb) != len(tb):
        res["status"] = "struct"
        return res
    if not bb:
        res["status"] = "no-branches"
        return res
    flips = [(i, x[1], y[1]) for i, (x, y) in enumerate(zip(bb, tb)) if x[1] != y[1]]
    if len(flips) > max_flips:
        res["status"] = "many-flips"
        return res
    if flips:
        kinds = set(classify(a, b) for _, a, b in flips)
        res["status"] = "flips"
        res["kind"] = ("SIGNEDNESS" if "SIGNEDNESS" in kinds else
                       "OTHER" if "OTHER" in kinds else "POLARITY")
        res["rows"] = flips
        res["je_jne_only"] = all({a, b} == {"je", "jne"} for _, a, b in flips)
        # THE SCREEN (measured 2026-08-01: 7 POLARITY rows opened, 1 was a real bug).
        # A mnemonic flip alone does not distinguish a wrong PREDICATE from mere block
        # LAYOUT, because reaching a `return` through a shared exit rather than a local
        # copy flips the mnemonic without changing behaviour. The targets decide:
        #   both sides land on the SAME symbolic destination -> the condition itself is
        #     inverted, i.e. a real predicate bug. OPEN IT.
        #   destinations of different KIND (short/local vs far/shared exit) -> layout.
        # `rets N->M` corroborates but does not decide - CBattlezMapConfig::RouteUnitTo
        # is four pure je/jne flips at EQUAL ret counts and is still layout.
        bt_f = [sym_target(bb, t) for _, _, t in bb]
        tt_f = [sym_target(tb, t) for _, _, t in tb]
        res["same_dest"] = [i for i, _, _ in flips if bt_f[i] == tt_f[i]]
        res["flip_dests"] = [(i, bt_f[i], tt_f[i]) for i, _, _ in flips]
        # The converse blind spot: a branch whose MNEMONIC matches on both sides but
        # whose TARGET differs is invisible once any flip exists, because the target
        # comparison below only runs on the all-mnemonics-equal path. Measured cost of
        # that gap: on CGrunt::PathScan the reported flip was the small half, while the
        # unreported same-mnemonic row (`je` on both sides, far exit in retail vs a short
        # local block in ours) was what actually explained the function. Report them.
        flipped = {i for i, _, _ in flips}
        res["moved_same_mnem"] = [(i, bt_f[i], tt_f[i])
                                  for i in range(len(bb))
                                  if i not in flipped and bt_f[i] != tt_f[i]]
        return res
    # Same mnemonics everywhere: the only thing left that a masked diff can hide is a
    # branch landing on a different block.
    bt = [sym_target(bb, t) for _, _, t in bb]
    tt = [sym_target(tb, t) for _, _, t in tb]
    moved = [(i, x, y) for i, (x, y) in enumerate(zip(bt, tt)) if x != y]
    if not moved:
        res["status"] = "clean"
    elif len(moved) > max_flips:
        res["status"] = "many-flips"
    else:
        res["status"] = "topology"
        res["kind"] = "TOPOLOGY"
        res["rows"] = moved
    return res
