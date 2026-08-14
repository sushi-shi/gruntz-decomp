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

**A jump table is not a branch.** llvm-objdump disassembles linearly, and a jump table
embedded in `.text` after an indirect `jmp` decodes as instructions - including `ja`/`je`,
which invent branches out of nowhere. It is ASYMMETRIC: on the base side each table entry
carries a DIR32 relocation and prints as a reloc row the parser skips, while the delinked
target packs raw addresses that disassemble as code. So the target reads MORE branches
than the base on every switch function - `_EngStr_RenderText` 6 against 9,
`CSpriteRef::Build` 1 against 3, `CPlay::LoadCursorSprites` 48 against 51, all of them
phantoms. `table_stop()` cuts at the last `ret` (MSVC5 parks the table at the end of the
COMDAT) whenever the function contains an indirect `jmp`, and `code_stop()` combines that
with `first_bad()`. Only a `first_bad` BEFORE the table is a real truncation - a switch
function is a clean result and must not be excluded from the sweep.

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
# set observed over every base+target obj in the tree; `loop*` never appears in MSVC5 /O2
# output and an unexpected spelling raises rather than being dropped silently.
# `jecxz` DOES appear (cl5 emits it ahead of an inline `rep` block, where a zero count
# must skip the string op). It is a real conditional branch, so it belongs in the
# sequence; it has no signed/unsigned twin, so a flip involving it lands in OTHER.
JCC = frozenset("je jne jl jle jg jge ja jae jb jbe js jns jo jno jp jnp jecxz".split())
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


# cl's two spellings for a block label inside a function's COMDAT.  `$L<n>` is the
# compiler-generated one (switch arms, if/else joins).  `$<name>$<n>` is a SOURCE
# label - the target of a `goto` - and it is NOT covered by a `$L` prefix test.
# Reading only `$L` truncated every function containing a named label at that label
# (CAniAdvanceCursor::Advance stopped 0x126 bytes early at `$loop_restart$32243`,
# hiding a whole else-arm and reading as "cl deleted the call").
LOCAL_LABEL = re.compile(r"^\$(?:L\d+|\w+\$\d+)$")


def is_local_label(name):
    """True for a cl block label that belongs to the ENCLOSING function's COMDAT."""
    return bool(LOCAL_LABEL.match(name))


def parse_objdump(out):
    """Parse llvm-objdump output into complete per-function instruction streams.

    MSVC emits `$L<n>` (and `$<goto-label>$<n>`) symbols for switch arms and other local
    basic blocks. llvm-objdump prints each one with the same header syntax as a function,
    but it is still part of the current function's COMDAT. Starting a new stream there
    made every switch body disappear after its indirect jump and falsely classified
    complete functions as missing bodies.
    """
    syms, cur, base = {}, None, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            name = m.group(1)
            if is_local_label(name) and cur is not None:
                continue
            cur, base = syms.setdefault(name, []), None
            continue
        if cur is None:
            continue
        m = INSN.match(ln)
        if not m:
            continue
        off, mn, op = int(m.group(1), 16), m.group(2), m.group(3).strip()
        if base is None:
            base = off
        if JUMPY.match(mn):
            t = TGT_OP.match(op)
            if t:
                op = hex(int(t.group(1), 16) - base) + op[t.end():]
        cur.append((off - base, mn, op))
    return syms


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
    parsed = parse_objdump(out)
    if symbol and _ends_at_indirect_jump(parsed.get(symbol)):
        # `--disassemble-symbols` emits ONLY that symbol, so the `$L<n>` switch-arm
        # symbols parse_objdump exists to re-merge are not in the output at all and
        # the stream stops at the indirect jump: ?Advance@CAniAdvanceCursor@@ reported
        # 6 branches / 3 rets against a real 59 / 3, which silently mis-ranks the
        # worklist. Pay for one whole-object decode in exactly that case.
        cmd = ["llvm-objdump", "-d", "--no-show-raw-insn", str(obj)]
        parsed = parse_objdump(subprocess.run(cmd, capture_output=True,
                                              text=True).stdout)
    return parsed


def _ends_at_indirect_jump(insns):
    """True when the stream's last real instruction is an indirect `jmp` - the
    signature of a symbol-scoped decode that stopped at a switch's jump table."""
    if not insns:
        return False
    for off, mn, ops in reversed(insns):
        if mn == "(bad)" or mn.startswith("<"):
            continue
        return mn.startswith("jmp") and "*" in ops
    return False


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


def table_stop(insns):
    """Offset where a switch's trailing JUMP TABLE begins, or None.

    `first_bad` only catches a table whose bytes happen to decode as `(bad)`. Plenty
    decode as perfectly plausible instructions instead - and among them, as CONDITIONAL
    BRANCHES, which then inflate the target side's branch count out of nowhere. The
    asymmetry is the delinker's: on the BASE side each 4-byte table entry carries a DIR32
    relocation, so llvm-objdump prints it as a reloc row this module's parser skips,
    while the delinked target packs raw absolute addresses that disassemble as code.
    `_EngStr_RenderText` read 6 branches against 9 and `CSpriteRef::Build` 1 against 3,
    all six of the extra "branches" being table bytes.

    MSVC5 parks the table at the END of the function's COMDAT, so everything past the
    last `ret` is data. Gated on the function actually containing an indirect `jmp` so a
    body that legitimately ends in a tail jump is never truncated.
    """
    if not any(mn.startswith("jmp") and "*" in op for _, mn, op in insns):
        return None
    last = None
    for off, mn, _ in insns:
        if mn.startswith("ret"):
            last = off
    if last is None or insns[-1][0] <= last:
        return None
    return last + 1


def code_stop(insns):
    """The earliest of `first_bad` and `table_stop` - where real code ends."""
    stops = [s for s in (first_bad(insns), table_stop(insns)) if s is not None]
    return min(stops) if stops else None


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


# A general-purpose register anywhere in an operand. `head_key` blanks these because
# the destination-content test below must survive REGISTER ALLOCATION, which differs
# between our build and retail on nearly every sub-100% function.
GPR = re.compile(r"%(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh])\b")


def head_key(mn, op):
    """The comparison key for the FIRST instruction of a branch destination.

    Registers are blanked; immediates and displacements are KEPT, and a jump/call
    operand is dropped entirely (it is an address, and addresses shift).

    **Why the raw text is not the key.** The GUARD-vs-ARM screen asks "do the two
    sides' destinations hold the same code?". Comparing raw operands answers a
    different question - "did cl colour the destination identically?" - and the
    answer to that is routinely NO on a function that is sub-100% in the first
    place. Measured cost of getting this wrong: `CGrunt::CommitNeighbor` @0x5b050
    is a genuine inverted guard (retail `v = m_170; if (v > 0x16) v = m_19c;`
    against our `if (m_170 > 0x16) { v = m_19c; ... }`, so retail also fires the
    move-config path when m_170 itself is 1). Both sides' destination is the same
    `test <reg>,<reg> / je` block - but retail holds the flag in ecx and we hold it
    in eax, so the raw-text test called it an arm selector and hid the bug.

    The bias is deliberate: an over-reported GUARD costs one reading, an
    under-reported one hides a behaviour bug. Immediates stay in the key because
    that is exactly what distinguishes two real arms - `?PolyIsConvexCW` stores
    `1` on one side and `2` on the other, and must stay an ARM."""
    if mn.startswith("j") or mn.startswith("call"):
        return (mn, "")
    return (mn, GPR.sub("%r", op))


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
      trunc   (base_offset|None, target_offset|None) - where the stream stopped decoding
              real CODE. A `table_stop` boundary is not a truncation: the code region
              before it is complete, so a switch function is a clean result and must not
              be excluded from the sweep the way an undecodable body is.
    """
    bstop, tstop = code_stop(bi), code_stop(ti)
    bb, tb = branches(bi, bstop), branches(ti, tstop)

    def bad_before(insns, stop):
        at = first_bad(insns)
        return at if at is not None and (stop is None or at < stop) else None

    res = {"kind": None, "rows": [], "nbr": len(bb), "nbr_t": len(tb),
           "rets": (rets(bi, bstop), rets(ti, tstop)),
           "trunc": (bad_before(bi, table_stop(bi)), bad_before(ti, table_stop(ti)))}
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
        # GUARD vs ARM SELECTOR - measured false positive of the =dest screen.
        # `sym_target` names a destination by BLOCK INDEX. When a branch selects between
        # the two ARMS of one construct (a ternary, an if/else), swapping the arms swaps
        # BOTH the polarity and which arm each index denotes, so index equality survives
        # while the behaviour is identical. `=dest` is therefore NECESSARY BUT NOT
        # SUFFICIENT: decisive for a GUARD (both sides branching to a shared
        # continuation/exit), not for an arm selector.
        # The discriminator is the destination's CONTENT: retail's
        #   je <armB> / push A / jmp <join> / armB: push B / join:
        # has two destinations that begin with DIFFERENT instructions. A guard's
        # destination is the same code on both sides. (Found on CTriggerMgr::
        # SetupTubeAnim @0x50a50, whose two arms push different string relocs.)
        #
        # `head_key` - NOT the raw text - is what "same content" means here: raw operands
        # make REGISTER ALLOCATION look like two different arms, which demoted a real
        # guard (`CGrunt::CommitNeighbor` #16, where retail holds the flag in ecx and we
        # held it in eax while both destinations were the same `test/je` block).
        #
        # VALIDATION HISTORY, because the first pass was not enough:
        #  * Known-verdict test (4 functions, 7 rows): the confirmed defects come back
        #    GUARD - RectContains #9, RectContainsGated #10, RectSegProbe #1/#4/#11 -
        #    and SetupTubeAnim #0/#10 come back ARM. No misclassification.
        #  * That sample MISSED the regalloc trap entirely: none of those four exhibit
        #    it. A tree-wide census then found 19 of 51 =arm rows differing only in the
        #    OPERAND, i.e. wrongly demoted. Small samples confirm; only a census refutes.
        # The bias is deliberate: an over-reported guard costs one reading, an
        # under-reported one hides a behaviour bug.
        at_b = {off: head_key(mn, op) for off, mn, op in bi}
        at_t = {off: head_key(mn, op) for off, mn, op in ti}
        guard = []
        for i, _, _ in flips:
            if bt_f[i] != tt_f[i]:
                continue
            bd, td = bb[i][2], tb[i][2]
            if bd is None or td is None:      # unresolvable target: keep the row
                guard.append(i)
                continue
            hb, ht = at_b.get(bd), at_t.get(td)
            if hb is None or ht is None or hb == ht:
                guard.append(i)               # same destination content => GUARD
        res["same_dest"] = guard
        res["arm_selector"] = [i for i, _, _ in flips
                               if bt_f[i] == tt_f[i] and i not in guard]
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
