"""gruntz.walls.diagnose - classify one wall from the normalized pair.

    gruntz walls diagnose <rva|mangled|CClass::Member> [--asm]

The ladder (CLAUDE.md): the FIRST divergence class decides the wall.

  referent   masked instruction bytes identical, only relocation TARGETS
             differ - an identity/aliasing question, not codegen. Lever:
             the reloc-sequence diff below; fix the claim, not the source.
  inline     the call-target multisets differ - a callee was expanded on
             one side and called on the other (or a call-set member is
             missing). Lever: `gruntz walls inline-model --gap` quantifies
             the budget deficit; docs/patterns/ob1-budget-cutoff-*,
             inline-visibility-splits-call-and-expansion.md.
  cfg        call sets agree but branch/return counts differ - control-flow
             reconstruction (arm shape, tail merge, loop form). Levers:
             docs/patterns/ tail/cross-jump/do-while family.
  regalloc   same calls, same branch skeleton, different bytes - register
             allocation / scheduling / instruction selection. Lever:
             docs/relevations/cl5-callcrossing-ebx-first-by-use-schedule.md;
             a TU-state probe (docs/patterns/tu-state-probe-family-*) as a
             disposable A/B only.

Inputs, read-only: the Model (locate the function), the compare out-dir's
NORMALIZED base/target objs (delink.coffx topology), tool.objdump for the
instruction skeleton. No retail-image reads here: the pair IS the evidence
objdiff scored.
"""

from __future__ import annotations

import re

from gruntz.core.paths import BUILD
from gruntz.delink.coffx import Obj

NORM = BUILD / "objdiff/compare-new"

_CALL = re.compile(r"\b(?:call)\s")
_RET = re.compile(r"\bret\b")
_JCC = re.compile(r"\bj(?:mp|e|ne|z|nz|a|ae|b|be|g|ge|l|le|s|ns|o|no|p|np)\b")


def _find_function(obj: Obj, name: str):
    """(payload, {offset: (target, addend)}, size) of `name`'s section slice.
    The slice runs from the symbol to the next defined symbol in its section."""
    for secnum in range(1, obj.nsec + 1):
        members = obj.section_members(secnum)
        value = next((v for v, n, _s in members if n == name), None)
        if value is None:
            continue
        payload = obj.section_payload(secnum)
        relocs = obj.typed_relocations(secnum)
        starts = sorted(v for v, _n, _s in members)
        end = next((s for s in starts if s > value), len(payload))
        rel = {off - value: tgt for off, tgt in relocs.items()
               if value <= off < end}
        body = payload[value:end]
        # trim alignment padding to the next symbol (int3 / nop fill) so a
        # pad-length difference never reads as a codegen divergence
        body = body.rstrip(b"\xcc").rstrip(b"\x90")
        return body, rel, len(body)
    return None, None, 0


def _jump_table_bytes(rel: dict, own: str) -> set[int]:
    """Offsets covered by this function's own switch jump table.

    A DIR32 reloc whose referent is the function ITSELF is a table entry, not
    a call: the table is DATA embedded in .text, and objdump decodes it as
    instructions - some entry bytes decode as `call`, which used to inflate
    the call count and read as a false INLINE/CALL-SET wall on switch-heavy
    functions (CGrunt::StepCompassMove reported 24 vs 22 calls; both are 22)."""
    covered: set[int] = set()
    for off, (tgt, _addend) in rel.items():
        if tgt == own:
            covered.update(range(off, off + 4))
    return covered


def _skeleton(payload: bytes, rel: dict, vma: int = 0, data: set[int] = ()):
    """(masked bytes, calls-in-order, n_branches, n_returns, n_insns)."""
    from gruntz.tool import objdump
    masked = bytearray(payload)
    for off in rel:
        masked[off:off + 4] = b"\0\0\0\0"
    text = objdump.disassemble(payload, vma=vma)
    calls = branches = rets = insns = 0
    for line in text.splitlines():
        if ":\t" not in line:
            continue
        try:
            if int(line.split(":\t", 1)[0].strip(), 16) in data:
                continue
        except ValueError:
            pass
        insns += 1
        body = line.split("\t", 2)[-1]
        if _CALL.search(body):
            calls += 1
        elif _JCC.search(body):
            branches += 1
        elif _RET.search(body):
            rets += 1
    return bytes(masked), calls, branches, rets, insns, text


def _referents(rel: dict) -> list[str]:
    def spell(t):
        tgt, addend = t
        return f"{tgt}+0x{addend:x}" if addend else tgt
    return [spell(rel[o]) for o in sorted(rel)]


def _locate(token: str):
    """The claimed function a token names: a hex rva, the mangled name, or the
    readable `CClass::Member` spelling every other view accepts."""
    from gruntz.model import resolve
    from gruntz.sema.index import short_name
    model = resolve()
    named = [f for f in model.functions if f.name]
    if token.lower().startswith("0x"):
        try:
            rva = int(token, 16)
        except ValueError:
            return None, f"{token!r} is not a hex rva"
        b = next((f for f in named if f.rva == rva), None)
        if b is not None:
            return b, ""
        return None, (f"no CLAIMED function starts at {token} "
                      f"(`gruntz sema rva {token}` says what is there)")
    hits = [f for f in named if f.name == token] \
        or [f for f in named if short_name(f.name) == token]
    if len(hits) == 1:
        return hits[0], ""
    if not hits:
        return None, (f"no claimed function is named {token!r} (give the hex "
                      f"rva, the mangled name, or `CClass::Member`; "
                      f"`gruntz sema map find {token}` searches the Model)")
    where = ", ".join(f"0x{h.rva:06x} [{h.unit}]" for h in hits[:6])
    return None, f"{token!r} names {len(hits)} claimed functions: {where}"


def diagnose(token: str, show_asm: bool = False) -> int:
    b, why = _locate(token)
    if b is None:
        print(f"[diagnose] {why}")
        return 2
    base_p = NORM / "base" / f"{b.unit}.obj"
    tgt_p = next((p for p in (NORM / "target" / f"{b.unit}.c.obj",
                              NORM / "target" / f"{b.unit}.obj") if p.is_file()),
                 None)
    if not base_p.is_file() or tgt_p is None:
        print(f"[diagnose] normalized pair missing for {b.unit} - run "
              f"`gruntz compare` first")
        return 2

    from gruntz.tool import ToolError
    sides = {}
    for tag, path in (("base", base_p), ("target", tgt_p)):
        payload, rel, size = _find_function(Obj(path), b.name)
        if payload is None:
            print(f"[diagnose] {tag} obj does not define {b.name}")
            return 2
        try:
            table = _jump_table_bytes(rel, b.name)
            sides[tag] = (payload, rel, size,
                          *_skeleton(payload, rel, data=table))
        except ToolError as e:
            print(f"[diagnose] {e}")
            return 2

    (bp, brel, bsz, bmask, bcall, bbr, bret, bins, basm) = sides["base"]
    (tp, trel, tsz, tmask, tcall, tbr, tret, tins, tasm) = sides["target"]
    bref, tref = _referents(brel), _referents(trel)

    print(f"[diagnose] {b.name}  [{b.unit}]  rva 0x{b.rva:06x}")
    print(f"  base:   {bsz:#x} B, {bins} insns, {bcall} calls, "
          f"{bbr} branches, {bret} rets, {len(brel)} relocs")
    print(f"  target: {tsz:#x} B, {tins} insns, {tcall} calls, "
          f"{tbr} branches, {tret} rets, {len(trel)} relocs")

    if bmask == tmask and bref != tref:
        wall = "referent"
        print("  class: REFERENT - masked bytes identical; the relocation "
              "TARGETS differ:")
        for i, (x, y) in enumerate(zip(bref, tref)):
            if x != y:
                print(f"    reloc[{i}]: base {x}  !=  target {y}")
    elif sorted(n for n, _a in _call_targets(brel, basm)) != \
            sorted(n for n, _a in _call_targets(trel, tasm)):
        wall = "inline"
        bs = sorted(n for n, _a in _call_targets(brel, basm))
        ts = sorted(n for n, _a in _call_targets(trel, tasm))
        print("  class: INLINE/CALL-SET - the call-target multisets differ:")
        for n in ts:
            if n not in bs:
                print(f"    target calls, base expanded/lacks: {n}")
        for n in bs:
            if n not in ts:
                print(f"    base calls, target expanded/lacks:  {n}")
        print("  lever: gruntz walls inline-model --gap (budget deficit per "
              "starved site)")
    elif (bbr, bret) != (tbr, tret):
        wall = "cfg"
        print(f"  class: CFG - branch/return skeleton differs "
              f"(base {bbr}/{bret}, target {tbr}/{tret}); a structural "
              f"reconstruction question (arm shape, tail merge, loop form)")
    elif bmask != tmask:
        wall = "regalloc"
        first = next((i for i, (x, y) in enumerate(zip(bmask, tmask))
                      if x != y), min(len(bmask), len(tmask)))
        print(f"  class: REGALLOC/SCHEDULING - same calls and skeleton, "
              f"bytes first differ at +{first:#x}; instruction selection, "
              f"lifetime or allocation "
              f"(docs/relevations/cl5-callcrossing-ebx-*)")
    else:
        wall = "none"
        print("  class: NONE - the normalized pair is identical; the score "
              "gap is outside this function (pairing, data, or unit-level)")

    if wall in ("cfg", "regalloc", "inline"):
        _duplicate_tail_probe(basm, tasm)

    if show_asm and wall != "none":
        print("  --- base ---")
        print("\n".join("  " + ln for ln in basm.splitlines()[:60]))
        print("  --- target ---")
        print("\n".join("  " + ln for ln in tasm.splitlines()[:60]))
    return 0


def _insn_text(asm: str) -> list[str]:
    """One normalized mnemonic+operand string per instruction, in order.

    Intra-function branch displacements survive here on purpose: two arms that
    jump to DIFFERENT continuations must not read as identical runs."""
    out = []
    for line in asm.splitlines():
        if ":\t" not in line:
            continue
        out.append(" ".join(line.split("\t", 2)[-1].split()))
    return out


def _repeat_runs(insns: list[str], minlen: int = 4):
    """Maximal repeated instruction runs, each classified SUFFIX vs PREFIX.

    Only a converging SUFFIX is foldable at all (the unconditional suffix
    cross-jump is /Os-gated and off in our /O2 build; what merges under /O2 is
    value-based factoring - wall-reasons-layout.md), so the distinction
    decides whether a duplicated run could ever have been merged:
      suffix - every copy leaves via the same terminator (ret, or a jmp to one
               target); the copies converge, so the pass COULD merge them.
      prefix - the copies diverge after the run; no merge pass would fold them,
               and the duplication is a CFG reconstruction difference.
    Returns [(length, n_copies, kind, last_insn)] sorted longest-first."""
    index: dict[str, list[int]] = {}
    for i, t in enumerate(insns):
        index.setdefault(t, []).append(i)
    # text of a maximal run -> the distinct start positions it occurs at
    found: dict[tuple[str, ...], set[int]] = {}
    for positions in index.values():
        if len(positions) < 2:
            continue
        for a in range(len(positions)):
            for b in range(a + 1, len(positions)):
                i, j = positions[a], positions[b]
                # extend backwards to the run's true start, then forwards
                s = 0
                while (i - s - 1 >= 0 and j - s - 1 > i - s - 1
                       and insns[i - s - 1] == insns[j - s - 1]):
                    s += 1
                i0, j0 = i - s, j - s
                n = 0
                while (j0 + n < len(insns)
                       and insns[i0 + n] == insns[j0 + n]):
                    n += 1
                if n < minlen:
                    continue
                found.setdefault(tuple(insns[i0:i0 + n]),
                                 set()).update((i0, j0))
    out = []
    for text, starts in found.items():
        n = len(text)
        last = text[-1]
        terminal = bool(_RET.search(last)) or last.startswith("jmp")
        # a run is a foldable SUFFIX when every copy leaves the same way:
        # a terminator, or an identical following instruction
        after = {insns[s + n] if s + n < len(insns) else None for s in starts}
        kind = "suffix" if (terminal or len(after) == 1) else "prefix"
        out.append((n, len(starts), kind, last))
    out.sort(key=lambda r: (-r[0], -r[1]))
    return out


MIN_RUN = 10


def _duplicate_tail_probe(basm: str, tasm: str) -> None:
    """List the LONG repeated runs on each side, classified suffix vs prefix.

    Short runs repeat by chance in any large body (a 4-insn window recurs
    hundreds of times), so only runs of MIN_RUN+ instructions are evidence.
    The routing question is per-RUN, not per-count: a long SUFFIX duplicated
    on one side only is a blocked cross-jump; long PREFIXES that diverge are
    never foldable and mean the CFG differs. This prints the evidence and
    names the rule; it does not pretend to adjudicate the function."""
    b = _repeat_runs(_insn_text(basm), MIN_RUN)
    t = _repeat_runs(_insn_text(tasm), MIN_RUN)
    if not b and not t:
        return

    def show(tag, runs):
        if not runs:
            print(f"  {tag}: no repeated run >= {MIN_RUN} insns")
            return
        for n, c, kind, last in runs[:4]:
            print(f"  {tag}: {c}x {n}-insn {kind} run, ends `{last}`")

    show("base  ", b)
    show("target", t)
    bs = [r for r in b if r[2] == "suffix"]
    ts = [r for r in t if r[2] == "suffix"]
    if [(n, c, k) for n, c, k, _l in b] == [(n, c, k) for n, c, k, _l in t]:
        print("    -> SYMMETRIC: both sides duplicate the same runs, so the "
              "duplication is retail's own shape (a no-IL tail or a per-arm "
              "scope it really had), not a defect - do not chase it.")
    elif bs and not ts:
        print("    -> only BASE duplicates a long converging SUFFIX. The "
              "unconditional suffix cross-jump is /Os-gated and OFF in our "
              "/O2 build, so what merges here is value-based factoring: look "
              "for a join at the suffix head, a per-arm destructible local, "
              "or arm VALUES that differ where retail's agree. "
              "docs/relevations/wall-reasons-layout.md")
    elif ts and not bs:
        print("    -> only TARGET duplicates a long suffix: retail's arms "
              "carried something ours factored away (a per-arm scope is the "
              "usual one).")
    elif (b or t) and not bs and not ts:
        print("    -> the long repeats are PREFIXES that diverge; no merge "
              "pass folds those, so a duplication difference here is a CFG "
              "reconstruction question, not a placement coin.")


def _call_targets(rel: dict, asm: str) -> list[tuple[str, int]]:
    """Reloc referents that sit inside a call instruction's operand."""
    call_offs = set()
    for line in asm.splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        if _CALL.search(rest.split("\t")[-1]):
            try:
                call_offs.add(int(addr.strip(), 16))
            except ValueError:
                pass
    out = []
    for off in sorted(rel):
        # a REL32 call operand starts 1 byte after the opcode
        if (off - 1) in call_offs or (off - 2) in call_offs:
            out.append(rel[off])
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="gruntz walls diagnose", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("token", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--asm", action="store_true",
                    help="print both sides' disassembly (first 60 lines)")
    a = ap.parse_args(argv)
    return diagnose(a.token, a.asm)


if __name__ == "__main__":
    raise SystemExit(main())
