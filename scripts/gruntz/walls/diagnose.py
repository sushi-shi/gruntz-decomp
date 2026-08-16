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


def _skeleton(payload: bytes, rel: dict, vma: int = 0):
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
            sides[tag] = (payload, rel, size, *_skeleton(payload, rel))
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

    if show_asm and wall != "none":
        print("  --- base ---")
        print("\n".join("  " + ln for ln in basm.splitlines()[:60]))
        print("  --- target ---")
        print("\n".join("  " + ln for ln in tasm.splitlines()[:60]))
    return 0


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
