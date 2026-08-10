#!/usr/bin/env python3
"""Per-function MEMBER-STORE-OFFSET diff: base (our compile) vs target (delinked retail).

WHY
  objdiff's fuzzy % scores instruction-for-instruction similarity.  A ctor that
  writes the WRONG member - `mov [esi+0x420],edi` where retail writes
  `mov [esi+0x424],edi` - emits the SAME instruction, the same length, the same
  operand shape.  objdiff's masked view calls it a near-match (CGrunt::CGrunt
  still scored 90.39% with exactly that bug), the member it should have
  initialised keeps heap garbage, and a 100%-byte-exact function downstream
  dies on it.  No existing gate can see this.

  The signature is not "an instruction differs", it is
  "an OFFSET is present on ONE SIDE ONLY".

METHOD
  Disassemble each objdiff base obj and its delinked target obj, split at COFF
  symbol headers, and for every function extract the ORDERED SEQUENCE of
  member-store destinations.  A store is any instruction whose destination is a
  memory operand: `mov`/`fstp`/`and`/`or`/... `<width> ptr [reg (+ idx*s) (+ disp)]`.

  Register allocation differs between the two sides, so the register NAME can
  never be part of the key.  Instead a dataflow over the function's CFG re-roots
  each store onto the object it actually names:

      this  - entry ecx (__thiscall) and everything lea/mov-derived from it
      p0..  - the incoming stack parameters, seeded from the `ret N` argument
              byte count and tracked through the esp delta / a frame pointer
      -     - unrooted (a pointer loaded out of memory, a call result, ...)

  `lea edi,[esi+0x468]` followed by `mov [edi+4],eax` therefore scores as
  this+0x46c, the same key retail's folded `mov [esi+0x46c],eax` produces.
  Frame slots (esp/ebp-relative) are NEVER stores in this sense - their
  displacements track the frame size, which is exactly the false-positive
  source that made the call-count detector's `--slots` view unusable until it
  excluded them.  Spills and push/pop THROUGH those slots ARE tracked, so
  `mov [esp+8],esi` / `mov edi,[esp+8]` and `push esi` / `pop esi` keep the
  rooting alive.

  Compare the two sides as MULTISETS first: a key on one side only is the
  defect signature.  Only if the multisets agree is the ORDER compared - a pure
  permutation is scheduling noise and is reported separately and never chased.

SIX THINGS THAT MUST BE RIGHT, each of which produced phantom rows until fixed
  1. TOKENISATION.  llvm-objdump's byte column is a fixed width; a 10-byte
     encoding fills it exactly and leaves no space before the tab.  A
     whitespace regex then eats the mnemonic - and `c7 86 <disp32> <imm32>`,
     i.e. `mov dword ptr [reg+off], imm`, is exactly 10 bytes and is the single
     commonest member INITIALISER.  Split on the tabs.  (This alone hid retail's
     `m_blockedVoicePending = 1` in ??0CGrunt.)
  2. ebp IS A GENERAL REGISTER unless the function established a frame.  cl 5.0
     routinely parks an interior pointer there (CImage::BlitNorm copies a RECT
     via `mov [ebp+0..0xc]`, ebp = &info->m_dirty.m_rect).
  3. esp MUST BE PROPAGATED ALONG THE CFG, not linearly.  A function with two
     epilogues unwinds at the first `ret` and then reads the second epilogue's
     `pop`s as positive.  Calls must be modelled too: __stdcall/__thiscall
     callees pop their own arguments, __cdecl call sites clean with `add esp,N`
     or cl's `pop ecx` idiom.  Linear esp mis-keyed 2819 of 4327 pairs.
  4. THE ROOTING STATE MUST BE CFG-PROPAGATED TOO, merged by intersection at
     joins - otherwise the walk falls through an early-return epilogue and
     carries its `pop esi` into the block after it (CSBI_WellGoo::Setup: five
     phantom RETAIL-ONLY offsets).
  5. PARAMETER SLOTS ARE SEEDED FROM `ret N`, never a fixed guess.  Seeding ten
     of them made a spill slot in a 2-argument function read as `p4` and
     reported a phantom +0/+4 slip in zPTree::Insert.
  6. `add reg, imm` / `sub reg, imm` ADVANCES A ROOT, it does not destroy one.
     cl walks a member array either as `lea edi,[esi+N]` re-derived per step or
     by advancing the cursor in place, and it picks a different spelling on the
     two sides of the same loop.  Killing the root on `add` made retail's
     CGruntSpawnConfig::ClearSprites (`add ecx,0x8`) look like it stored nothing
     at all while ours (`lea eax,[ecx+8]`) stored two fields - a whole-function
     phantom.  Both spawn-config rows disappeared when `add`/`sub` of a constant
     started shifting the displacement instead.

COVERAGE (2026-08-10, 341 units)
  4327 function pairs; 2728 make no rooted member store at all (accessors, CRT,
  math, and functions that only write through loaded pointers - use
  --all-roots for those); 1496 identical sequences; 64 reorder-only; 39 with a
  one-sided offset.  1279 functions hit an esp-modelling conflict; for those the
  frame-slot rooting is DISOWNED wholesale (p-roots dropped, `this` kept) rather
  than guessed at.  The register dataflow is flow-sensitive but not
  path-sensitive: a root that two predecessors disagree on is dropped, so the
  tool under-reports rather than inventing.

USAGE
  python -m gruntz.audit.store_offsets                  # ranked worklist
  python -m gruntz.audit.store_offsets --unit grunt     # one unit
  python -m gruntz.audit.store_offsets --fn '??0CGrunt' # one function, detailed
  python -m gruntz.audit.store_offsets --all-roots      # include unrooted stores
  python -m gruntz.audit.store_offsets --reorder        # scheduling-noise list
  python -m gruntz.audit.store_offsets --tsv out.tsv    # machine readable
  python -m gruntz.audit.store_offsets --coverage       # what it could not read

ADJUDICATION (what a one-sided offset turned out to be, 2026-08-10 wave)
  (a) MEMBER SLIP / MISSING INIT - retail stores there, we do not (or we store
      to the neighbouring slot).  A pointer member is a latent crash.  FIX.
      Found: CTriggerMgr's four goo/resource timers were modelled as their LOW
      words only, so the Hi halves were never rearmed and the deadline compare
      was 32-bit where retail's is sub/sbb/cmp/cmp.
  (b) STORED ELSEWHERE - retail initialises the member in Activate/Reset
      instead; confirm that other site exists on our side, then note it.
  (c) INLINING ARTIFACT - one side inlines a callee that does the store (or
      out-of-lines it).  Confirm the callee, note it, do not "fix".
      Found: ??0CWarlord / ??0CInGameIcon `call CUserLogic::AttachToObject`
      where retail expands it, so all seven of its stores read as RETAIL-ONLY.
  (d) BRANCH MERGE - both sides assign the same member in every arm, but one
      side tail/head-merges the stores into one.  Semantically identical; the
      `[same-count: likely rooting asymmetry]` tag does NOT cover this, so check
      the arms before believing a one-store delta.
  (e) BULK STORE - a `rep stos`/memset covers the range on one side.  Flagged
      as `bulk` on the row; adjudicate by hand.
"""
import argparse
import collections
import json
import re
import subprocess
import sys
from pathlib import Path

OBJDUMP = "llvm-objdump"
REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

SYM_HEAD = re.compile(r"^[0-9a-f]{8} <(.+)>:$")
# llvm-objdump lays a row out as  "<addr>: <bytes><TAB><mnem><TAB><ops>", and the
# byte field is a FIXED WIDTH: a 10-byte encoding fills it exactly, leaving no
# space before the tab.  A whitespace-based regex then swallows the mnemonic into
# the byte run - and `c7 86 <disp32> <imm32>` (`mov dword ptr [reg+off], imm`) is
# precisely 10 bytes, i.e. the single commonest member INITIALISER in a ctor.
# Splitting on the tabs is the only correct tokenisation.
ROW = re.compile(r"^\s*([0-9a-f]+):\s")
MEMDST = re.compile(r"^(dword|word|byte|qword|tbyte|fword) ptr \[([^\]]+)\](?:, (.+))?$")

GPR = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")
SUB = {  # sub-register -> full register (a write to al still kills eax's rooting)
    "al": "eax", "ah": "eax", "ax": "eax", "bl": "ebx", "bh": "ebx", "bx": "ebx",
    "cl": "ecx", "ch": "ecx", "cx": "ecx", "dl": "edx", "dh": "edx", "dx": "edx",
    "si": "esi", "di": "edi", "bp": "ebp", "sp": "esp",
}

# instructions that do NOT write their first operand
NO_WRITE = {"cmp", "test", "push", "jmp", "call", "ret", "retn", "nop", "int3",
            "fld", "fild", "fcom", "fcomp", "fucom", "fucomp", "fstsw", "int",
            "hlt", "leave", "cld", "std", "prefetch"}
JCC = re.compile(r"^j[a-z]{1,3}$")

# store mnemonics: the destination is memory and the instruction modifies it
STORE_MNEM = {"mov", "fstp", "fst", "fistp", "fist", "fbstp", "and", "or", "xor",
              "add", "sub", "adc", "sbb", "inc", "dec", "neg", "not", "shl",
              "shr", "sar", "rol", "ror", "imul", "movsx", "movzx", "xchg",
              "btr", "bts", "btc"}
SETCC = re.compile(r"^set[a-z]{1,3}$")

CTOR_HINT = re.compile(r"^\?\?0|Init|Reset|Setup|Create|Construct|Alloc|New|Load|Clear|Zero")


def dump(obj: Path) -> str:
    return subprocess.run([OBJDUMP, "-d", "--x86-asm-syntax=intel", str(obj)],
                          capture_output=True, text=True).stdout


def parse_mem(inner: str):
    """'esi + 0x420' -> ('esi', None, 0x420);  'eax + ecx*4 + 0x10' -> ('eax','ecx*4',0x10)"""
    inner = inner.strip()
    if ":" in inner:                      # fs:[0x0] segment override
        return None
    base = idx = None
    disp = 0
    parts = re.split(r"\s*([+-])\s*", inner)     # keep the signs as tokens
    sign = 1
    first = True
    for tok in parts:
        if tok == "+":
            sign = 1
            continue
        if tok == "-":
            sign = -1
            continue
        tok = tok.strip()
        if not tok:
            continue
        if tok in GPR and first and base is None:
            base = tok
        elif "*" in tok:
            idx = tok
        elif tok in GPR:
            idx = tok
        else:
            try:
                disp += sign * int(tok, 0)
            except ValueError:
                return None
        first = False
    return base, idx, disp


class Tracker:
    """Linear re-rooting of pointer registers onto `this` / the stack params."""

    def __init__(self, nargs):
        # reg -> (root_tag, disp)
        self.reg = {"ecx": ("this", 0)}
        # normalized frame slot key -> (root_tag, disp).  Seeded with EXACTLY the
        # incoming parameters and no more: `ret 0x8` proves two stack args, so
        # slot +0x14 in that function is a LOCAL, not `p4`.  Over-seeding made
        # zPTree::Insert's spill slot read as a parameter and reported a phantom
        # +0/+4 slip in an otherwise instruction-identical function.
        self.slot = {4 * (i + 1): (f"p{i}", 0) for i in range(nargs)}
        self.esp = 0          # esp delta relative to function entry
        self.ebp_base = None  # esp delta that ebp captured, if a frame ptr exists
        self.lost = 0         # instructions whose effect we could not model
        self.drift = False    # esp delta was not 0 at a `ret` - rooting unreliable

    def kill(self, r):
        r = SUB.get(r, r)
        self.reg.pop(r, None)

    def slot_key(self, base, disp):
        if self.drift:
            return None       # esp modelling failed - no frame slot is trustworthy
        if base == "esp":
            return self.esp + disp
        if base == "ebp" and self.ebp_base is not None:
            return self.ebp_base + disp
        return None

    def resolve(self, base, disp):
        """memory operand -> (root, offset) or None

        `ebp` is a FRAME register only when the function actually established a
        frame (`push ebp; mov ebp,esp`).  Without /Oy- cl 5.0 hands ebp out as a
        general register, and it routinely holds an interior pointer - CImage::
        BlitNorm copies a RECT with `mov [ebp+0..0xc]` where ebp = &info->
        m_dirty.m_rect.  Excluding ebp unconditionally reported four phantom
        RETAIL-ONLY offsets in each of the three Blit* siblings.
        """
        if base == "esp" or (base == "ebp" and self.ebp_base is not None):
            return None
        v = self.reg.get(base)
        if v is None:
            return None
        return (v[0], v[1] + disp)

    def step(self, mnem, ops):
        # esp itself is owned by esp_deltas() (CFG-propagated); this only tracks
        # the register/slot rooting.
        # push/pop are just stores/loads at [esp]: a mid-function `push esi ...
        # pop esi` RESTORES esi's rooting, and dropping it there lost `this` for
        # the whole tail of CSBI_WellGoo::Setup (5 phantom RETAIL-ONLY offsets).
        if mnem == "push":
            r = ops.strip()
            k = self.esp - 4
            if self.drift:
                return
            v = self.reg.get(r) if r in GPR else None
            if v is not None:
                self.slot[k] = v
            else:
                self.slot.pop(k, None)
            return
        if mnem == "pop":
            r = ops.strip()
            v = None if self.drift else self.slot.pop(self.esp, None)
            if r in GPR:
                if v is not None:
                    self.reg[SUB.get(r, r)] = v
                else:
                    self.kill(r)
            return
        if mnem in ("sub", "add") and ops.startswith("esp,"):
            return
        if mnem == "leave":
            self.ebp_base = None
            self.kill("ebp")
            return
        if mnem == "call":
            for r in ("eax", "ecx", "edx"):
                self.reg.pop(r, None)
            return
        if JCC.match(mnem) or mnem in ("jmp", "ret", "retn", "nop", "int3"):
            return

        dst, _, src = ops.partition(", ")
        dst, src = dst.strip(), src.strip()

        # --- frame-pointer establishment ------------------------------------
        if mnem == "mov" and dst == "ebp" and src == "esp":
            self.ebp_base = self.esp
            return

        # --- store into a frame slot: remember what was spilled --------------
        m = MEMDST.match(ops)
        if m and mnem == "mov":
            inner = parse_mem(m.group(2))
            if inner:
                base, idx, disp = inner
                k = self.slot_key(base, disp) if idx is None else None
                if k is not None:
                    v = self.reg.get(src) if src in GPR else None
                    if v is not None and m.group(1) == "dword":
                        self.slot[k] = v
                    else:
                        self.slot.pop(k, None)
            return

        # --- cursor advance: `add edi,0x4` / `sub esi,0x10` KEEPS the rooting ---
        # cl walks a member array either as `lea edi,[esi+N]` + `[edi+k]` or by
        # advancing the base register in place.  The two spellings are the same
        # pointer, so an `add`/`sub` of a constant re-roots rather than kills -
        # without this the two sides of one loop read as disjoint offset sets.
        if mnem in ("add", "sub") and dst in GPR:
            v = self.reg.get(dst)
            if v is not None and re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", src or ""):
                self.reg[dst] = (v[0], v[1] + (int(src, 0) if mnem == "add" else -int(src, 0)))
                return
            self.kill(dst)
            return

        # --- reload / derive --------------------------------------------------
        if mnem in ("mov", "lea") and dst in GPR:
            if src in GPR:                                   # mov r, r
                v = self.reg.get(src)
                if v:
                    self.reg[dst] = v
                else:
                    self.reg.pop(dst, None)
                return
            sm = MEMDST.match(src) if mnem == "mov" else None
            inner = None
            if mnem == "lea":
                lm = re.match(r"^\[([^\]]+)\]$", src)
                if lm:
                    inner = parse_mem(lm.group(1))
            elif sm:
                inner = parse_mem(sm.group(2))
            if inner:
                base, idx, disp = inner
                if mnem == "lea":
                    v = self.reg.get(base) if idx is None else None
                    if v:
                        self.reg[dst] = (v[0], v[1] + disp)
                        return
                else:                                        # load
                    k = self.slot_key(base, disp) if idx is None else None
                    if k is not None and k in self.slot:
                        self.reg[dst] = self.slot[k]
                        return
            self.reg.pop(dst, None)
            return

        # --- everything else: conservatively drop the written register --------
        if mnem in NO_WRITE or JCC.match(mnem):
            return
        if mnem == "cdq":
            self.kill("edx")
            return
        if mnem in ("mul", "imul", "div", "idiv") and "," not in ops:
            self.kill("eax")
            self.kill("edx")
            return
        if dst in GPR or dst in SUB:
            self.kill(dst)
        elif dst and not dst.startswith(("dword", "word", "byte", "qword", "st")):
            self.lost += 1


def split_symbols(text: str):
    """-> [(symbol, [(raw, mnem, ops), ...])] in obj order."""
    out, cur = [], None
    for ln in text.splitlines():
        m = SYM_HEAD.match(ln.strip())
        if m:
            name = m.group(1)
            if name.startswith("$"):
                continue                     # switch arm / goto target of the fn above
            cur = []
            out.append((name, cur))
            continue
        if cur is None:
            continue
        r = ROW.match(ln)
        if not r:
            continue
        parts = ln.split("\t")
        if len(parts) < 2:
            continue
        raw = parts[0].split(":", 1)[1].strip()
        mnem = parts[1].strip()
        ops = parts[2].strip() if len(parts) > 2 else ""
        cur.append((int(r.group(1), 16), raw, mnem, ops))
    return out


BR_TGT = re.compile(r"^(0x[0-9a-f]+)")


def arg_bytes(insns):
    """`ret 0x8` -> 8 stack-argument bytes.  A bare `ret` (__cdecl) gives None."""
    for _a, raw, mnem, ops in insns:
        if mnem in ("ret", "retn"):
            ops = ops.strip()
            if not ops:
                return None
            try:
                return int(ops, 0)
            except ValueError:
                return None
    return None


def prologue_end(insns):
    """Index one past the last frame push.

    cl 5.0 emits every callee-saved `push` (and the /GX `push -1; push handler;
    push fs:[0]` triple) in one leading run, before any control flow.  So the
    first `call`/`jmp`/`jcc` ends the prologue: pushes above it belong to the
    FRAME, pushes below it are CALL ARGUMENTS that the callee (__stdcall /
    __thiscall) pops on return.  Without that split every argument push reads as
    a permanent frame growth and esp drifts by the argument bytes - which is what
    made 2566 of 4327 pairs unreadable.
    """
    for i, (_a, _raw, mnem, ops) in enumerate(insns):
        if mnem in ("call", "jmp", "ret", "retn") or JCC.match(mnem):
            return i
        if mnem != "push":
            continue
        o = ops.strip()
        if o in ("ebx", "esi", "edi", "ebp"):
            continue                                  # callee-saved save
        prev = insns[i - 1][2:] if i else ("", "")
        if o == "-0x1":
            continue                                  # /GX: push -1
        if i and insns[i - 1][2] == "push" and insns[i - 1][3].strip() == "-0x1":
            continue                                  # /GX: push <handler>
        if o == "eax" and prev[0] == "mov" and "fs:" in prev[1]:
            continue                                  # /GX: push fs:[0]
        return i                                      # an ARGUMENT push
    return len(insns)


def esp_deltas(insns):
    """Propagate the esp delta along the CFG -> [esp_in per instruction], drift.

    A LINEAR walk cannot do this: a function with two epilogues unwinds at the
    first `ret` and then reads the second epilogue's `pop`s as a POSITIVE delta,
    so every frame slot below it is mis-keyed.  Walking the edges keeps every
    basic block's depth right, and a block reached at two different depths is a
    real modelling failure - reported as `drift`, whereupon the function's
    frame-slot rooting is disowned wholesale rather than guessed at.
    """
    idx = {a: i for i, (a, _, _, _) in enumerate(insns)}
    pro = prologue_end(insns)
    espin = [None] * len(insns)
    drift = False
    work = [(0, 0, 0)]
    while work:
        i, e, args = work.pop()
        while 0 <= i < len(insns):
            if espin[i] is not None:
                if espin[i] != e:
                    drift = True
                break
            espin[i] = e
            _a, _raw, mnem, ops = insns[i]
            if mnem in ("ret", "retn"):
                if e != 0:
                    drift = True
                break
            nxt = insns[i + 1] if i + 1 < len(insns) else None
            if mnem == "push" or mnem == "pushf":
                e -= 4
                if i >= pro:
                    args += 4
            elif mnem in ("pop", "popf"):
                e += 4
            elif mnem in ("sub", "add") and ops.startswith("esp,"):
                try:
                    n = int(ops.split(",")[1].strip(), 0)
                    e += -n if mnem == "sub" else n
                except ValueError:
                    e = 0
            elif mnem == "call":
                # __cdecl cleans at the CALL SITE - either `add esp,N` or cl's
                # one-arg idiom `pop ecx` (and runs of it).  Those instructions
                # are walked normally, so only the part they do NOT cover is
                # attributed to the callee (__stdcall/__thiscall).
                clean, j = 0, i + 1
                while j < len(insns):
                    m2, o2 = insns[j][2], insns[j][3].strip()
                    if m2 == "add" and o2.startswith("esp,"):
                        try:
                            clean += int(o2.split(",")[1].strip(), 0)
                        except ValueError:
                            pass
                        break
                    if m2 == "pop" and o2 in ("ecx", "eax", "edx"):
                        clean += 4
                        j += 1
                        continue
                    break
                e += max(0, args - clean)
                args = 0
            elif mnem == "leave" or (mnem == "mov" and ops.startswith("esp,")):
                e = 0                       # epilogue unwind; only the tail follows
            if mnem == "jmp" or JCC.match(mnem):
                m = BR_TGT.match(ops)
                j = idx.get(int(m.group(1), 16)) if m else None
                if j is not None:
                    work.append((j, e, args))
                if mnem == "jmp":
                    break
            i += 1
    return espin, drift


def dataflow(insns, espin, drift, nargs):
    """Fixpoint of the rooting state over the CFG -> [in-state per instruction].

    A LINEAR walk is wrong for the same reason it was wrong for esp: it falls
    through an EARLY-RETURN epilogue and carries the epilogue's `pop esi` into
    the block after it, so `this` is lost for the rest of the function.  That
    silently produced five phantom RETAIL-ONLY offsets in CSBI_WellGoo::Setup.
    States are merged at joins by INTERSECTION (keep only what both predecessors
    agree on), which is the conservative direction: a root survives only where
    every path proves it.
    """
    n = len(insns)
    idx = {a: i for i, (a, _, _, _) in enumerate(insns)}
    IN = [None] * n
    init = ({"ecx": ("this", 0)},
            {} if drift else {4 * (i + 1): (f"p{i}", 0) for i in range(nargs)})
    work = [(0, init)]
    guard = 0
    while work and guard < 200000:
        i, st = work.pop()
        guard += 1
        if not (0 <= i < n) or espin[i] is None:
            continue
        if IN[i] is None:
            IN[i] = st
        else:
            old = IN[i]
            r = {k: v for k, v in old[0].items() if st[0].get(k) == v}
            sl = {k: v for k, v in old[1].items() if st[1].get(k) == v}
            if len(r) == len(old[0]) and len(sl) == len(old[1]):
                continue                       # nothing new to propagate
            IN[i] = (r, sl)
        tr = Tracker(0)
        tr.reg, tr.slot, tr.esp, tr.drift = dict(IN[i][0]), dict(IN[i][1]), espin[i], drift
        _a, _raw, mnem, ops = insns[i]
        tr.step(mnem, ops)
        out = (tr.reg, tr.slot)
        if mnem in ("ret", "retn"):
            continue
        if mnem == "jmp" or JCC.match(mnem):
            m = BR_TGT.match(ops)
            j = idx.get(int(m.group(1), 16)) if m else None
            if j is not None:
                work.append((j, out))
            if mnem == "jmp":
                continue
        work.append((i + 1, out))
    return IN


def stores(obj: Path, cdecl_slots=12):
    """symbol -> {'seq': [(root, off, width, idx, mnem, srckind)], 'bulk', 'lost', ...}"""
    res = {}
    for name, insns in split_symbols(dump(obj)):
        cur = res.setdefault(name, dict(seq=[], bulk=False, lost=0, n=0, pad=0,
                                        drift=False))
        n = arg_bytes(insns)
        # __cdecl leaves the arg count unknowable from the callee; seed a generous
        # window and let the drift check below disown it if esp tracking slipped.
        tr = Tracker(cdecl_slots if n is None else max(0, n // 4))
        espin, drift = esp_deltas(insns)
        tr.drift = drift
        nargs = cdecl_slots if n is None else max(0, n // 4)
        _scan(cur, tr, insns, espin, dataflow(insns, espin, drift, nargs))
    return res


def _scan(cur, tr, insns, espin, states):
    for i, (_a, raw, mnem, ops) in enumerate(insns):
        if espin[i] is None or states[i] is None:
            continue                        # unreachable (delinker tail, data)
        tr.esp = espin[i]
        tr.reg, tr.slot = states[i]
        cur["n"] += 1
        # COMDAT tail padding (00-byte fill, 0x90 nop runs) decodes as
        # `add byte ptr [eax], al` / a 0x90909090 displacement.  Both are junk
        # past the function's real end; drop them or they read as phantom stores.
        if mnem == "add" and ops == "byte ptr [eax], al" and raw.startswith("00 00"):
            cur["pad"] += 1
            continue
        if mnem.startswith("rep") or "stos" in mnem or "movs" in ops[:8]:
            cur["bulk"] = True
        # record BEFORE stepping (the store's address uses the pre-state)
        if (mnem in STORE_MNEM or SETCC.match(mnem)) and ops.startswith(
                ("dword ptr", "word ptr", "byte ptr", "qword ptr", "fword ptr", "tbyte ptr")):
            mm = MEMDST.match(ops)
            if mm:
                inner = parse_mem(mm.group(2))
                if inner:
                    base, idx, disp = inner
                    if abs(disp) > 0x100000:      # not a member offset - junk
                        cur["pad"] += 1
                        tr.step(mnem, ops)
                        continue
                    root = tr.resolve(base, disp) if base else None
                    src = mm.group(3) or ""
                    kind = "imm" if re.match(r"^-?0x[0-9a-f]+$|^-?\d+$", src.strip()) else "reg"
                    if root:
                        cur["seq"].append((root[0], root[1], mm.group(1),
                                           "idx" if idx else "", mnem, kind))
                    elif base not in ("esp", "ebp"):
                        cur["seq"].append(("-", disp, mm.group(1),
                                           "idx" if idx else "", mnem, kind))
    cur["lost"] = tr.lost
    cur["drift"] = tr.drift
    if tr.drift:
        # esp tracking slipped, so every frame-slot key is suspect; keep only the
        # `this` rooting (ecx-derived, independent of the stack).
        cur["seq"] = [e for e in cur["seq"] if e[0] in ("this", "-")]


def units(cfg: Path):
    d = json.loads(cfg.read_text())
    for u in d["units"]:
        yield u["name"], (cfg.parent / u["base_path"]).resolve(), \
                         (cfg.parent / u["target_path"]).resolve()


def pct_map(report: Path) -> dict:
    try:
        d = json.loads(report.read_text())
    except Exception:
        return {}
    out = {}
    for u in d.get("units", []):
        for f in (u.get("functions") or u.get("sections") or []):
            nm, m = f.get("name"), f.get("fuzzy_match_percent")
            if nm is not None and m is not None:
                out[nm] = m
    return out


def key_of(e, all_roots):
    root, off, width, idx, mnem, kind = e
    if not all_roots and root == "-":
        return None
    return (root, off, width, idx)


def fmt_key(k):
    root, off, width, idx = k
    o = f"+0x{off:x}" if off >= 0 else f"-0x{-off:x}"
    return f"{root}{o}{'[i]' if idx else ''}:{width[0]}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit", action="append")
    ap.add_argument("--fn", help="substring: dump both sides' sequences")
    ap.add_argument("--all-roots", action="store_true",
                    help="also compare stores through unrooted pointers")
    ap.add_argument("--reorder", action="store_true",
                    help="list multiset-equal / order-different functions (noise)")
    ap.add_argument("--coverage", action="store_true")
    ap.add_argument("--tsv")
    ap.add_argument("--min-pct", type=float, default=0.0)
    ap.add_argument("--project", default=str(REPO / "build/objdiff"))
    a = ap.parse_args()

    proj = Path(a.project)
    pcts = pct_map(proj / "report.json")

    n_units = n_units_read = n_pairs = n_skip_side = n_nostore = 0
    n_equal = n_reorder = n_diff = 0
    lost_fns = drift_fns = 0
    rows, reorders = [], []

    for unit, base, target in units(proj / "objdiff.json"):
        n_units += 1
        if a.unit and unit not in a.unit:
            continue
        if not base.is_file() or not target.is_file():
            n_skip_side += 1
            continue
        n_units_read += 1
        b, t = stores(base), stores(target)
        for name in sorted(set(b) & set(t)):
            n_pairs += 1
            if a.fn and a.fn not in name:
                continue
            bs = [key_of(e, a.all_roots) for e in b[name]["seq"]]
            ts = [key_of(e, a.all_roots) for e in t[name]["seq"]]
            bs = [k for k in bs if k]
            ts = [k for k in ts if k]
            if b[name]["lost"] or t[name]["lost"]:
                lost_fns += 1
            if b[name]["drift"] or t[name]["drift"]:
                drift_fns += 1
            if not bs and not ts:
                n_nostore += 1
                continue
            cb, ct = collections.Counter(bs), collections.Counter(ts)
            if a.fn:
                print(f"\n=== {unit} :: {name}   {pcts.get(name, -1):.2f}%"
                      f"{'  [bulk]' if b[name]['bulk'] or t[name]['bulk'] else ''}")
                print("  OURS  ", " ".join(fmt_key(k) for k in bs))
                print("  RETAIL", " ".join(fmt_key(k) for k in ts))
                if cb != ct:
                    for k, v in sorted((ct - cb).items()):
                        print(f"     RETAIL-ONLY {fmt_key(k)} x{v}")
                    for k, v in sorted((cb - ct).items()):
                        print(f"     OURS-ONLY   {fmt_key(k)} x{v}")
            if cb == ct:
                if bs == ts:
                    n_equal += 1
                else:
                    n_reorder += 1
                    reorders.append((unit, name, pcts.get(name)))
                continue
            n_diff += 1
            miss = ct - cb                       # retail stores, we do not
            extra = cb - ct                      # we store, retail does not
            p = pcts.get(name, -1.0)
            if p < a.min_pct:
                continue
            # Discriminator: the TOTAL store count including unrooted ones.  If
            # both sides make the same NUMBER of stores, a one-sided offset means
            # only that one side's base register could be named - the store does
            # exist on both (CAniAdvanceCursor::Deserialize).  A differing total is
            # a store that is genuinely present on one side only.
            nb, nt = len(b[name]["seq"]), len(t[name]["seq"])
            bulk = b[name]["bulk"] or t[name]["bulk"]
            ctor = bool(CTOR_HINT.search(name))
            nptr = sum(v for k, v in list(miss.items()) + list(extra.items())
                       if k[2] == "dword")
            rows.append(dict(unit=unit, name=name, pct=p, miss=miss, extra=extra,
                             bulk=bulk, ctor=ctor, nptr=nptr, nb=nb, nt=nt,
                             tot=sum(miss.values()) + sum(extra.values())))

    if a.coverage:
        print(f"units in project        : {n_units}")
        print(f"units with both objs    : {n_units_read}  (skipped {n_skip_side}: obj missing)")
        print(f"function pairs analysed : {n_pairs}")
        print(f"  no member store at all: {n_nostore}")
        print(f"  identical sequence    : {n_equal}")
        print(f"  reorder only (noise)  : {n_reorder}")
        print(f"  MULTISET DIFFERS      : {n_diff}")
        print(f"functions where the tracker gave up on >=1 insn : {lost_fns}")
        print(f"functions where esp drifted (params disowned)   : {drift_fns}")
        return 0

    if a.reorder:
        for unit, name, p in sorted(reorders, key=lambda r: -(r[2] or 0)):
            print(f"{(p if p is not None else -1):6.2f}  {unit}  {name}")
        print(f"\n{len(reorders)} reorder-only function(s) - scheduling noise, do not chase.")
        return 0

    if a.fn:
        return 0

    rows.sort(key=lambda r: (not r["ctor"], -r["nptr"], -r["tot"], -r["pct"]))
    hdr = f"{'%':>6} {'n':>3} {'k':>1}  unit / symbol"
    print(hdr)
    print("-" * 76)
    for r in rows:
        tag = "C" if r["ctor"] else " "
        same = " [same-count: likely rooting asymmetry]" if r["nb"] == r["nt"] else ""
        print(f"{r['pct']:6.2f} {r['tot']:>3} {tag}  {r['unit']}  {r['name']}"
              f"{'  [bulk]' if r['bulk'] else ''}{same}")
        for k, v in sorted(r["miss"].items(), key=lambda x: (x[0][0], x[0][1])):
            print(f"            RETAIL-ONLY  {fmt_key(k)}{f' x{v}' if v > 1 else ''}")
        for k, v in sorted(r["extra"].items(), key=lambda x: (x[0][0], x[0][1])):
            print(f"            OURS-ONLY    {fmt_key(k)}{f' x{v}' if v > 1 else ''}")
    print(f"\n{len(rows)} function(s) with a one-sided store offset "
          f"({n_reorder} reorder-only, not listed).")

    if a.tsv:
        with open(a.tsv, "w") as f:
            f.write("unit\tsymbol\tpct\tside\troot\toffset\twidth\tidx\tcount\tbulk\tctor\n")
            for r in rows:
                for side, c in (("retail", r["miss"]), ("ours", r["extra"])):
                    for k, v in sorted(c.items()):
                        f.write(f"{r['unit']}\t{r['name']}\t{r['pct']:.2f}\t{side}\t"
                                f"{k[0]}\t0x{k[1]:x}\t{k[2]}\t{k[3]}\t{v}\t"
                                f"{int(r['bulk'])}\t{int(r['ctor'])}\n")
        print(f"wrote {a.tsv}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
