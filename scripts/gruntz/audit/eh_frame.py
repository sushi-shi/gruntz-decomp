#!/usr/bin/env python3
"""eh_frame.py - sieve the tree for /GX EXCEPTION-FRAME PRESENCE mismatches.

`/GX` is on project-wide (proven from retail's own EH tables: 741 functions across
186 TUs).  cl 5.0 therefore emits the frame in a function **if and only if** that
function owns an object whose destructor must run during an unwind - a by-value class
local, a temporary, or a `new T` whose ctor can throw.  The frame is not a codegen
preference and no optimizer setting turns it on or off per function, so a
presence DISAGREEMENT between base and target is a hard SOURCE fact:

  * **TARGET_ONLY** - retail has the frame, we do not.
  * **BASE_ONLY** - we have the frame, retail does not.

A mismatch has THREE causes needing completely different work, so every row is
tagged (see `cause()`).  Only the last is what the frame naively suggests:

  * **INLINE_CUT** - one side CALLS a ctor/dtor COMDAT the other never calls.  Same
    object, different inline cut: an out-of-line ctor can throw and so takes an
    unwind state, an inlined one cannot.  cl 5.0 picks the cut PER `new`-SITE, so no
    declaration form expresses it - a census entry, not a worklist item.
    docs/patterns/new-site-eh-states-are-a-called-base-ctor.md.
  * **EXIT_MERGE** - the SAME ctor/dtor, a different NUMBER of call sites.  Not an
    object difference at all: cl gives every `if (...) return 0;` its own exit block
    but collapses them all when a `||`/`&&` guard sends them to a common
    destination, and each surviving dtor copy carries its own state store.  The
    lever and the wider sweep are `gruntz.audit.exit_merge_sieve` +
    docs/patterns/goto-fail-shares-one-exit-block.md.
  * **MISSING_OBJECT** / **EXTRA_OBJECT** - no ctor/dtor difference of either kind
    explains it, so one side really does own a destructible object the other's
    source never declared: a by-value `CString` where the other wrote `LPCSTR`, a
    by-value `CRect`/`CPoint`/MFC collection, a stack helper whose dtor releases
    something.

Measured 2026-08-08 over the 3 presence + 44 state rows: 23 INLINE_CUT, 9
EXIT_MERGE, 9 MISSING_OBJECT, 6 EXTRA_OBJECT.  Reading the frame as "a destructible
object" without the split would have sent a lane after 32 phantoms.

## What is detected

The registration-record prologue, as a SEQUENCE, never a single instruction:

    mov  eax, fs:[0]        ; cl 5.0 schedules this load first or third
    push -1                 ; initial unwind state
    push <handler>          ; __ehhandler$... (a DIR32 reloc slot on the base side)
    push eax
    mov  fs:[0], esp

`mov fs:[0], esp` is the load-bearing one - nothing else in cl's output stores ESP
into the TIB's exception-list head - but on its own it would also fire on a hand-
written SEH prologue in a delinked jump table's misdecoded bytes, so a hit needs the
`push -1` and the `fs:[0]` load in the same prologue window too.

## The EH-STATE store count, and why it is the useful half

Between construction and destruction cl keeps an unwind-state index in the frame -
`mov [ebp-4], <n>` in an EBP-framed function, `mov [esp+N], <n>` in the frameless
form cl 5.0 prefers at /O2 (`CStatusBarMgr::BuildStatusBarTabs` uses `[esp+0x38]`).
The number of DISTINCT states is roughly the number of destructible objects whose
lifetimes overlap a call, so `states` on a TARGET_ONLY row tells you how many objects
you are missing, and the offsets of the FIRST and LAST state store bracket their
combined lifetime exactly - which is the handoff when a row will not crack.

`--states` is the secondary sieve and reaches much further than presence does:
functions where BOTH sides carry a frame but the state COUNTS differ.  Same
evidence, same two causes, one object's worth of resolution.

## Calibration

`--calibrate` measures both signals against the functions objdiff already scores at
100.00%: they are byte-identical, so they must agree by construction and any
disagreement is a detector bug.  Measured 2026-08-08: presence 0 of 3455, state
count 0 of the 510 EH-framed among them.

    python -m gruntz.audit.eh_frame                  # presence mismatches, biggest first
    python -m gruntz.audit.eh_frame --states --detail # the state-count sieve
    python -m gruntz.audit.eh_frame --calibrate      # false-positive rate on 100% fns
    python -m gruntz.audit.eh_frame --direction target   # only the retail side
    python -m gruntz.audit.eh_frame --unit gruntzmgr
    python -m gruntz.audit.eh_frame --rva 0x000ffde0 --detail   # one function, verbose
    python -m gruntz.audit.eh_frame --tsv out.tsv
"""
import argparse
import csv
import json
import re
import sys
from collections import Counter
from pathlib import Path

from gruntz.audit.insn_count import streams, trim, truncated
from gruntz.core.report import Report, fn_fuzzy

REPO = Path(__file__).resolve().parents[3]
OBJDIFF = REPO / "build" / "objdiff" / "objdiff.json"
SYMBOLS = REPO / "build" / "gen" / "symbol_names.csv"

# how many leading instructions may hold the registration prologue.  cl 5.0 puts it
# first, but a `mov esi,ecx` / argument load can slot in front of the pushes.
PROLOGUE_WINDOW = 14

FS_STORE = re.compile(r"^%esp,\s*%fs:0x?0$")
FS_LOAD = re.compile(r"^%fs:0x?0,\s*%e\w\w$")
FS_RESTORE = re.compile(r"^[^,]+,\s*%fs:0x?0$")
PUSH_M1 = re.compile(r"^\$-0x1$")
# `movl $<imm>, <disp>(%esp)` / `movl $<imm>, -0x4(%ebp)`
IMM_STORE = re.compile(r"^\$(-?0x[0-9a-f]+),\s*(-?0x[0-9a-f]+)\((%esp|%ebp)\)$")


def unit_objs():
    """{unit: (base_obj, target_obj)} straight out of objdiff's own config.

    NOT `gruntz.core.branches.obj_paths`: objdiff scores the *normalized* copies,
    and `build/objdiff/{base,target}` hold a superset/subset that disagrees with
    them for some units (`sbi_rectonly` has no un-normalized base obj at all)."""
    if not OBJDIFF.is_file():
        return {}
    cfg = json.loads(OBJDIFF.read_text())
    root = OBJDIFF.parent
    out = {}
    for u in cfg.get("units", []):
        b = (root / u["base_path"]).resolve()
        t = (root / u["target_path"]).resolve()
        if b.is_file() and t.is_file():
            out[u["name"]] = (b, t)
    return out


def rva_index():
    """{mangled name: rva} from the generated symbol table."""
    out = {}
    if SYMBOLS.is_file():
        for r in csv.DictReader(SYMBOLS.open()):
            if r.get("kind") == "func":
                out.setdefault(r["name"], r["rva"])
    return out


def has_eh(insns):
    """Does the registration-record prologue appear, as a whole sequence?

    Requires all three of the `mov fs:[0], esp` store, a `push -1`, and the
    `mov <reg>, fs:[0]` load, inside the prologue window."""
    head = insns[:PROLOGUE_WINDOW]
    store = any(mn.startswith("mov") and FS_STORE.match(op) for _, mn, op in head)
    if not store:
        return False
    load = any(mn.startswith("mov") and FS_LOAD.match(op) for _, mn, op in head)
    push = any(mn.startswith("push") and PUSH_M1.match(op) for _, mn, op in head)
    return load and push


def has_unwind(insns):
    """Is there a matching `mov <x>, fs:[0]` teardown after the prologue?

    A function that sets the frame up and never tears it down is a decode error,
    not a function; used only to flag a row as suspect."""
    return any(mn.startswith("mov") and FS_RESTORE.match(op) and not FS_STORE.match(op)
               for _, mn, op in insns[PROLOGUE_WINDOW:])


SUB_ESP = re.compile(r"^\$(0x[0-9a-f]+),\s*%esp$")
PUSH_REG = re.compile(r"^%e\w\w$")


def uncomment(op):
    """llvm-objdump's trailing `# imm = 0xFFFFFFFF` annotation, removed.

    It is appended to exactly the large immediates, so a naive `endswith(slot)`
    silently drops every `movl $0xffffffff, <slot>` - i.e. precisely the
    leave-the-region state stores, which manufactured a fleet of +-1 rows."""
    return op.split("#", 1)[0].strip()


def normalize(insns):
    """One instruction stream with every operand field uncommented."""
    return [(o, m, uncomment(p)) for o, m, p in insns]


def framed(insns):
    """`push ebp; mov esp,ebp` in the prologue - cl's EBP frame form."""
    head = insns[:PROLOGUE_WINDOW + 8]
    for i, (_, mn, op) in enumerate(head):
        if mn.startswith("push") and op == "%ebp" and i + 1 < len(head):
            nxt = head[i + 1]
            if nxt[1].startswith("mov") and nxt[2] == "%esp, %ebp":
                return True
    return False


ESP_MEM = re.compile(r"^(-?0x[0-9a-f]+)\(%esp\)$")
REG = re.compile(r"^%[a-z]{2,3}$")
IMM = re.compile(r"^\$-?0x[0-9a-f]+$")


SAVED = ("%ebx", "%ebp", "%esi", "%edi")


def prologue_slot(insns):
    """`sub K + 4*P + 8` off the prologue - the state's displacement at frame base.

    Only genuine callee-save pushes count (an argument push would over-shoot), and
    the scan stops at the first control transfer."""
    k, saves, seen = 0, 0, False
    for _, mn, op in insns[:PROLOGUE_WINDOW + 10]:
        if mn.startswith("mov") and FS_STORE.match(op):
            seen = True
            continue
        if not seen:
            continue
        if mn.startswith("sub") and SUB_ESP.match(op):
            k += int(SUB_ESP.match(op).group(1), 16)
        elif mn.startswith("push") and op in SAVED:
            saves += 1
        elif mn.startswith(("j", "call", "ret", "loop")):
            break
    return k + 4 * saves + 8


def teardown_slots(insns):
    """`D + 8 + 4*pops` for every unwind teardown, i.e. the state's BODY displacement.

    An unwind path ends `mov <D>(%esp), %reg` / `mov %reg, fs:[0]`, and that `D` is
    the registration record's FIRST dword by construction - so it pins the state at
    whatever ESP depth the teardown runs at.  That anchor is needed because the
    prologue formula cannot see a SHRINK-WRAPPED callee-save
    (`CFontConfig::MeasureLabel` saves ESI inside the first `if`, putting the state
    at `[esp+0x2c]` where the prologue says `[esp+0x28]`).

    But the teardown's own depth is not the body's: cl freely interleaves the
    epilogue's `pop`s with the load, and every pop that has ALREADY run makes `D`
    name a shallower address.  `BuildLevelTitleString` pops ESI and EDI before the
    load on our side and after it in retail - identical code, `0x384dc` against
    `0x384e4` - which reads as a two-state difference if the pops are ignored.  So
    count the pops between the start of the epilogue block and the load, and add
    them back."""
    out = set()
    for i, (_, mn, op) in enumerate(insns):
        if not (mn.startswith("mov") and FS_RESTORE.match(op) and not FS_STORE.match(op)):
            continue
        reg = op.split(",")[0].strip()
        # walk back to the top of this epilogue block, remembering where the load of
        # `prev` was and how many pops preceded it
        m, pops, load = None, 0, None
        for j in range(i - 1, max(-1, i - 24), -1):
            _, mj, oj = insns[j]
            if mj.startswith(("call", "j", "ret", "loop")):
                break
            if mj.startswith("pop"):
                pops += 1 if load is not None else 0
                continue
            if load is None and mj.startswith("mov") and \
                    oj.rsplit(",", 1)[-1].strip() == reg:
                m = ESP_MEM.match(oj.split(",")[0].strip())
                load = j
                if m is None:
                    break
        if load is not None and m is not None:
            d = int(m.group(1), 16) + 8 + 4 * pops
            if d >= 0:
                out.add(d)
    return out


# how many dwords of pushed arguments a state store may sit behind.  The frame's
# state field is the TOP of the registration record: above it are only the return
# address and the incoming arguments, never a local - so accepting `slot + 4*k` can
# collide with a parameter write, not with a local, and only for k >= 3.
ARG_DEPTH = 8


def eh_states(insns):
    """(slot, [(offset, state|None), ...]) - the unwind-state stores of an EH function.

    **This is a heuristic lead, not the exact signal the frame's PRESENCE is.**  The
    frameless state lives at `record+8`, so the displacement naming it moves with
    ESP, and neither anchor is sufficient alone: the prologue formula misses a
    shrink-wrapped save, while a teardown fires part-way through the epilogue's pops.
    Exact ESP tracking is not available - a `__thiscall`/`__stdcall` callee pops its
    own arguments, so the walk would need every callee's `ret N`, which is not in
    this object.

    So: take the union of both anchors, pick the one actually written most (frequency
    over a two-element principled candidate set is safe where a scan over every frame
    slot is not - that version picked a loop counter over the real slot in
    `BuildStatusBarTabs`), and then ALSO accept the same field one argument-push
    region deeper.  cl schedules a state store freely with respect to a call's
    pushes, and it does so ASYMMETRICALLY: `CGruntCreationPoint`'s state 2 is
    `movb $0x2,[esp+0x28]` at frame base in retail and `movb %bl,[esp+0x30]` behind
    a push for us, identical code that read as a missing object.  Only an immediate
    counts at the deeper displacements, since those addresses are incoming
    parameters at frame base rather than locals.

    Stores are counted SOURCE-INDEPENDENTLY at the slot itself - any width, any
    source.  Reading only dword immediates under-counts twice over: `SaveTga` sets
    state 0 with `mov %esi,<slot>` where retail writes `movl $0x0,<slot>`, and
    `MeasureLabel` writes states 1 and 2 as `movb`.  `None` is a register store, so
    the VALUE set is a lower bound while the COUNT is exact.

    Validated by hand on `BuildStatusBarTabs` (4), `SaveTga` (4/4),
    `MeasureLabel` (3/3), `CAniCycle` (5/5), `BuildLevelTitleString` (3/3) and
    `CGruntCreationPoint` (5/5)."""
    if framed(insns):
        cands = {"-0x4(%ebp)": {"-0x4(%ebp)"}}
    else:
        # each candidate is scored INDEPENDENTLY over its own depth window.  A
        # shared displacement can belong to two candidates (`0x18` is `0xc + 12`
        # and `0x10 + 8`), and assigning it to just one of them by dict order
        # picked the wrong slot in `CBattlezDlg::ReadPlayerName`.
        cands = {"0x%x(%%esp)" % d:
                 {"0x%x(%%esp)" % (d + 4 * k) for k in range(1, ARG_DEPTH + 1)}
                 for d in ({prologue_slot(insns)} | teardown_slots(insns))}
    hits = {c: [] for c in cands}
    for off, mn, op in insns:
        op = uncomment(op)
        if not mn.startswith("mov") or "," not in op:
            continue
        src, dst = op.rsplit(",", 1)[0].strip(), op.rsplit(",", 1)[-1].strip()
        # a SIB destination (`movb $0x5a,(%ecx,%edx,1)`) puts a comma inside the
        # operand, so the naive split leaves junk in `src`
        imm = int(src[1:], 16) if IMM.match(src) else None
        val = None if imm is None else (-1 if imm in (0xFFFFFFFF, 0xFF) else imm)
        for c, deep in cands.items():
            if dst == c:
                if imm is not None:
                    hits[c].append((off, val))
                elif REG.match(src):
                    hits[c].append((off, None))
            elif dst in deep and (imm is not None or mn == "movb"):
                # an immediate, or a BYTE-wide register store.  A pushed argument
                # is always a dword, so `movb %bl,<deeper>` cannot be one - and
                # that is exactly how our side writes CGruntCreationPoint's
                # state 2.
                if imm is not None and not (-1 <= imm <= 0x40 or imm in (0xFFFFFFFF, 0xFF)):
                    continue
                hits[c].append((off, val))
    if not any(hits.values()):
        return (sorted(cands)[-1] if cands else "-"), []
    slot = max(hits, key=lambda d: (len(hits[d]), d))
    return slot, sorted(hits[slot])


SYM_HDR = re.compile(r"^([0-9a-f]{8}) <(.+)>:$")
REL32 = re.compile(r"^\s+([0-9a-f]{8}):\s+IMAGE_REL_I386_REL32\s+(\S+)\s*$")
# cl's mangled ctor/dtor spellings: ??0 ctor, ??1 dtor, ??_D vbase-dtor helper,
# ??_E vector-deleting dtor, ??_G scalar-deleting dtor, ??_H/??_I vector ctor iterators.
CTOR_DTOR = re.compile(r"^\?\?(?:[01]|_[DEGHI])")


def rel32_calls(obj):
    """{symbol: [(offset, callee)]} - every REL32 (call/jmp) relocation, per function.

    Local `$L`/`$name$n` block labels are folded into the enclosing COMDAT, same rule
    as `gruntz.core.branches`: `--disassemble-symbols` stops at the first switch-arm
    label, which on a jump-table function hides most of the body's calls."""
    import subprocess

    from gruntz.core.branches import is_local_label
    out = subprocess.run(["llvm-objdump", "-d", "-r", "--no-show-raw-insn", str(obj)],
                         capture_output=True, text=True).stdout
    syms, cur = {}, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            name = m.group(2)
            if is_local_label(name) and cur is not None:
                continue
            cur = syms.setdefault(name, [])
            continue
        if cur is None:
            continue
        m = REL32.match(ln)
        if m:
            cur.append((int(m.group(1), 16), m.group(2)))
    return syms


def ctor_delta(base_calls, tgt_calls):
    """(retail-only, ours-only, same-callee-more-sites) ctor/dtor COMDAT callees."""
    b = Counter(n for _, n in base_calls if CTOR_DTOR.match(n))
    t = Counter(n for _, n in tgt_calls if CTOR_DTOR.match(n))
    shared = {n for n in set(b) & set(t) if b[n] != t[n]}
    return (sorted(set(t) - set(b)), sorted(set(b) - set(t)),
            sorted("%s x%d/%d" % (n, b[n], t[n]) for n in shared))


def cause(verdict, delta, only_t, only_b, resited):
    """WHY the two sides disagree about the frame - three mechanisms, different work.

      INLINE_CUT     - one side CALLS a ctor/dtor COMDAT the other never calls at
        all.  An out-of-line ctor can throw, so each such site takes an unwind state
        and the function gets a frame.  SAME object, different inline cut - and cl
        5.0 picks the cut depth PER `new`-SITE, so no declaration form expresses it
        and `#pragma inline_depth` is a fitted artifact that must not enter the tree.
        See docs/patterns/new-site-eh-states-are-a-called-base-ctor.md.
      EXIT_MERGE     - the SAME ctor/dtor, called a different NUMBER of times.  Not
        an object difference at all: cl gives every `if (...) return 0;` its own
        exit block but collapses all of them when a `||`/`&&` guard sends them to a
        common destination, and each surviving copy of the dtor carries its own
        state store.  `CGruntSpawnConfig::SpawnVoiceDriver` calls `??1CString` twice
        for us and EIGHT times in retail off one `&&` guard.  The lever, and the
        wider sweep, are `gruntz.audit.exit_merge_sieve` +
        docs/patterns/goto-fail-shares-one-exit-block.md.
      MISSING_OBJECT - no ctor/dtor difference of either kind explains the frame, so
        retail owns a destructible object our source never declared: a by-value
        `CString` where we wrote `LPCSTR`, a by-value `CRect`/`CPoint`, a stack
        helper whose dtor releases something.
      EXTRA_OBJECT   - the mirror: we invented one, or spelled as a by-value
        temporary something retail kept as a pointer or reference.
    """
    retail_side = verdict == "TARGET_ONLY" or (verdict == "BOTH" and delta > 0)
    if only_t or only_b:
        return "INLINE_CUT"
    if resited:
        return "EXIT_MERGE"
    return "MISSING_OBJECT" if retail_side else "EXTRA_OBJECT"


def classify(base_ins, tgt_ins):
    be, te = has_eh(base_ins), has_eh(tgt_ins)
    if be and not te:
        return "BASE_ONLY"
    if te and not be:
        return "TARGET_ONLY"
    return "BOTH" if be else "NEITHER"


def scan(units=None, quiet=False):
    """One row per scoring function objdiff pairs, with both sides classified."""
    objs = unit_objs()
    rep = Report()
    rvas = rva_index()
    rows = []
    for u in rep.units:
        name = u.get("name")
        if units and name not in units:
            continue
        pair = objs.get(name)
        if not pair:
            continue
        try:
            bs, ts = streams(pair[0]), streams(pair[1])
            bc, tc = rel32_calls(pair[0]), rel32_calls(pair[1])
        except Exception as exc:  # a malformed obj must not kill the sweep
            if not quiet:
                print("skip %s: %s" % (name, exc), file=sys.stderr)
            continue
        for fn in u.get("functions") or []:
            sym = fn.get("name")
            b, t = bs.get(sym), ts.get(sym)
            if b is None or t is None:
                continue
            bi, ti = normalize(trim(*b)), normalize(trim(*t))
            if not bi or not ti:
                continue
            bslot, bst = eh_states(bi) if has_eh(bi) else ("", [])
            tslot, tst = eh_states(ti) if has_eh(ti) else ("", [])
            src, slot, states = ((ti, tslot, tst) if has_eh(ti) else (bi, bslot, bst))
            only_t, only_b, resited = ctor_delta(bc.get(sym, []), tc.get(sym, []))
            verdict = classify(bi, ti)
            why = cause(verdict, len(tst) - len(bst), only_t, only_b, resited)
            rows.append(dict(
                unit=name, name=sym, rva=rvas.get(sym, ""),
                fuzzy=fn_fuzzy(fn), size=int(fn.get("size") or 0),
                verdict=verdict, cause=why,
                extra_ctors=only_t, our_ctors=only_b, resited=resited,
                trunc=truncated(*b) or truncated(*t),
                base_insn=len(bi), tgt_insn=len(ti),
                slot=slot, states=sorted({s for _, s in states if s is not None}),
                base_states=len(bst), tgt_states=len(tst),
                base_vals=sorted({s for _, s in bst if s is not None}),
                tgt_vals=sorted({s for _, s in tst if s is not None}),
                first=states[0][0] if states else None,
                last=states[-1][0] if states else None,
                unwind=has_unwind(src),
            ))
    return rows


def calibrate(rows):
    """Both signals, measured on the functions objdiff already scores at 100.00%.

    Those are byte-identical, so presence AND state count must agree; a disagreement
    there is a detector bug, not a finding.  The two rates are reported separately
    because the state count is the weaker of the two - it has to locate cl's frame
    slot, and `state_slot()` reads a frameless function's displacement off the
    prologue, which an unusual prologue can defeat."""
    exact = [r for r in rows if r["fuzzy"] >= 100.0]
    bad = [r for r in exact if r["verdict"] in ("BASE_ONLY", "TARGET_ONLY")]
    eh = [r for r in exact if r["verdict"] == "BOTH"]
    sbad = [r for r in eh if r["base_states"] != r["tgt_states"]]
    return exact, bad, eh, sbad


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append", help="restrict to a unit (repeatable)")
    ap.add_argument("--direction", choices=("target", "base", "both"), default="both",
                    help="which mismatch side to list (default both)")
    ap.add_argument("--states", action="store_true",
                    help="secondary sieve: BOTH rows whose unwind-STATE COUNTS disagree")
    ap.add_argument("--min", type=float, default=0.0, help="low fuzzy bound")
    ap.add_argument("--max", type=float, default=100.0, help="high fuzzy bound")
    ap.add_argument("--calibrate", action="store_true",
                    help="report the false-positive rate against 100%%-exact functions")
    ap.add_argument("--rva", help="detail one function by rva")
    ap.add_argument("--detail", action="store_true", help="print the state-store bracket")
    ap.add_argument("--tsv", help="write the full table here")
    a = ap.parse_args(argv)

    if not OBJDIFF.is_file():
        sys.exit("no %s - run `gruntz build` first" % OBJDIFF)
    rows = scan(set(a.unit) if a.unit else None)
    if not rows:
        sys.exit("no scoring functions found - run `gruntz build` first")

    exact, bad, eh_exact, sbad = calibrate(rows)
    tally = Counter(r["verdict"] for r in rows)
    print("%d scoring functions in %d units" % (len(rows), len({r['unit'] for r in rows})))
    for k in ("BOTH", "NEITHER", "TARGET_ONLY", "BASE_ONLY"):
        print("  %-12s %4d" % (k, tally.get(k, 0)))
        if k == "TARGET_ONLY":
            sub = Counter(r["cause"] for r in rows if r["verdict"] == k)
            for c in ("INLINE_CUT", "EXIT_MERGE", "MISSING_OBJECT"):
                if sub.get(c):
                    print("    %-10s %4d" % (c, sub[c]))
    print("calibration on the %d functions objdiff scores at 100.00%%:" % len(exact))
    print("  presence   %d disagree  (%.2f%% false-positive rate)"
          % (len(bad), 100.0 * len(bad) / max(1, len(exact))))
    print("  state cnt  %d of %d EH-framed disagree  (%.2f%%)"
          % (len(sbad), len(eh_exact), 100.0 * len(sbad) / max(1, len(eh_exact))))

    if a.calibrate:
        for r in sorted(bad, key=lambda r: -r["size"]):
            print("  FP-presence %-11s %-9s %-22s %s"
                  % (r["verdict"], r["rva"], r["unit"], r["name"]))
        for r in sorted(sbad, key=lambda r: -r["size"]):
            print("  FP-states   base=%-3d tgt=%-3d %-9s %-22s %s"
                  % (r["base_states"], r["tgt_states"], r["rva"], r["unit"], r["name"]))
        return 0

    if a.tsv:
        with open(a.tsv, "w", newline="") as fh:
            w = csv.writer(fh, delimiter="\t")
            w.writerow("verdict cause rva unit fuzzy size base_states tgt_states states "
                       "slot first last trunc extra_ctors name".split())
            for r in sorted(rows, key=lambda r: (r["verdict"], -r["size"])):
                w.writerow([r["verdict"], r["cause"], r["rva"], r["unit"],
                            "%.2f" % r["fuzzy"], r["size"],
                            r["base_states"], r["tgt_states"],
                            ",".join(str(s) for s in r["states"]), r["slot"],
                            "" if r["first"] is None else "0x%x" % r["first"],
                            "" if r["last"] is None else "0x%x" % r["last"],
                            "Y" if r["trunc"] else "",
                            " ".join(r["extra_ctors"] + r["resited"]),
                            r["name"]])
        print("wrote %s" % a.tsv)

    if a.states:
        d = [r for r in rows if r["verdict"] == "BOTH"
             and r["base_states"] != r["tgt_states"]
             and a.min <= r["fuzzy"] <= a.max]
        d.sort(key=lambda r: -abs(r["tgt_states"] - r["base_states"]))
        print("\n%d BOTH row(s) with differing unwind-state counts" % len(d))
        for r in d:
            print("%+3d states  %5d B  %-9s %-22s %6.2f%%  base=%d tgt=%d  %-14s %s"
                  % (r["tgt_states"] - r["base_states"], r["size"], r["rva"], r["unit"],
                     r["fuzzy"], r["base_states"], r["tgt_states"], r["cause"], r["name"]))
            if a.detail and (r["extra_ctors"] or r["resited"]):
                print("            ctor/dtor call delta: %s"
                      % " ".join(r["extra_ctors"] + r["resited"]))
        return 0

    want = {"target": ("TARGET_ONLY",), "base": ("BASE_ONLY",),
            "both": ("TARGET_ONLY", "BASE_ONLY")}[a.direction]
    hits = [r for r in rows if r["verdict"] in want
            and a.min <= r["fuzzy"] <= a.max
            and (not a.rva or r["rva"] == a.rva)]
    hits.sort(key=lambda r: (r["verdict"], -r["size"]))
    print()
    for r in hits:
        print("%-11s %-14s %5d B  %-9s %-22s %6.2f%%  states=%-10s %s"
              % (r["verdict"], r["cause"], r["size"], r["rva"], r["unit"], r["fuzzy"],
                 ",".join(str(s) for s in r["states"]) or "-", r["name"]))
        if a.detail and r["first"] is not None:
            print("            unwind-state slot %s, lifetime bracket 0x%x..0x%x%s"
                  % (r["slot"], r["first"], r["last"],
                     "" if r["unwind"] else "  [NO TEARDOWN - suspect decode]"))
        if a.detail and (r["extra_ctors"] or r["resited"]):
            print("            ctor/dtor call delta: %s"
                  % " ".join(r["extra_ctors"] + r["resited"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
