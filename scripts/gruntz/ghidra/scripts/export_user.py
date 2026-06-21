# -*- coding: utf-8 -*-
# export_user.py - capture HUMAN edits from the Ghidra DB into git (round-trip).
#
#   The back-direction of the enrichment: a human browsing build/ghidra-named in
#   Ghidra renames functions, writes comments, and names/types stack locals. This
#   script extracts exactly those human edits and writes them to the TRACKED file
#   config/user_annotations.json, so `apply.py` re-applies them on every refresh
#   and they survive a clean rebuild (which wipes the .gpr).
#
#   Human vs generated is told apart by provenance:
#     * function names / stack locals - the human GUI uses SourceType.USER_DEFINED;
#       apply.py applies everything it generates as SourceType.ANALYSIS, so a
#       USER_DEFINED symbol/variable is a human edit.
#     * comments - apply.py's own plates start with "[LABEL ..."; anything else is
#       a human comment.
#
#   Run as a GhidraScript under PyGhidra via `gruntz capture` (read-only; it only
#   writes the JSON, never mutates the program).
#@category Gruntz
import json
import os
from ghidra.program.model.symbol import SourceType
from ghidra.program.model.listing import CommentType

US = SourceType.USER_DEFINED
IMAGE_BASE = 0x400000
ROOT = os.environ.get("GRUNTZ_DIR", "/home/sheep/Projects/gruntz")
OUT = ROOT + "/config/user_annotations.json"

prog = currentProgram
fm = prog.getFunctionManager()
listing = prog.getListing()

_DEFAULT_LOCAL = ("local_", "uStack", "iStack", "auStack", "aiStack", "in_", "unaff_")


def rva(addr):
    return addr.getOffset() - IMAGE_BASE


# 1. FUNCTION NAMES the human set (USER_DEFINED primary symbol; skip Ghidra's FUN_)
functions = []
for fn in fm.getFunctions(True):
    sym = fn.getSymbol()
    if sym is not None and sym.getSource() == US:
        nm = str(fn.getName())
        if nm and not nm.startswith("FUN_"):
            functions.append({"rva": "0x%06x" % rva(fn.getEntryPoint()), "name": nm})

# 2. COMMENTS the human wrote = every comment NOT in the generated baseline that
#    apply.py snapshotted (build/gen/applied_comments.json). Comments have no
#    SourceType, so this diff is how human vs Ghidra/apply.py/demangler is told
#    apart. Without the baseline we don't guess (avoid dumping thousands of
#    Ghidra-generated plates) - capture no comments and warn.
_KINDS = [("plate", CommentType.PLATE), ("pre", CommentType.PRE),
          ("post", CommentType.POST), ("eol", CommentType.EOL),
          ("repeatable", CommentType.REPEATABLE)]
APPLIED_COMMENTS = ROOT + "/build/gen/applied_comments.json"
_baseline = None
if os.path.exists(APPLIED_COMMENTS):
    try:
        with open(APPLIED_COMMENTS) as _f:
            _baseline = set((c["rva"], c["kind"], c["text"]) for c in json.load(_f))
    except Exception:
        _baseline = None

comments = []
if _baseline is None:
    print("[export_user] WARN no build/gen/applied_comments.json baseline "
          "(run a ghidra-refresh first) - skipping comment capture")
else:
    it = listing.getCommentAddressIterator(prog.getMemory(), True)
    while it.hasNext():
        addr = it.next()
        if not addr.isMemoryAddress():
            continue
        r = "0x%06x" % rva(addr)
        for kind, ct in _KINDS:
            txt = listing.getComment(ct, addr)
            if txt and (r, kind, txt) not in _baseline:
                comments.append({"rva": r, "kind": kind, "text": txt})

# 3. STACK LOCALS the human named (USER_DEFINED local variables; skip Ghidra autos)
locals_out = []
for fn in fm.getFunctions(True):
    frva = rva(fn.getEntryPoint())
    for v in fn.getLocalVariables():
        if not v.isStackVariable() or v.getSource() != US:
            continue
        nm = str(v.getName())
        if not nm or nm.startswith(_DEFAULT_LOCAL):
            continue
        dt = v.getDataType()
        tname = str(dt.getName()) if dt is not None else ""
        if tname.startswith("undefined"):
            tname = ""    # let apply.py fall back; don't pin a placeholder type
        locals_out.append({"rva": "0x%06x" % frva, "offset": v.getStackOffset(),
                           "name": nm, "type": tname})

functions.sort(key=lambda d: d["rva"])
comments.sort(key=lambda d: (d["rva"], d["kind"]))
locals_out.sort(key=lambda d: (d["rva"], d["offset"]))
data = {"functions": functions, "comments": comments, "locals": locals_out}

_dir = os.path.dirname(OUT)
if _dir and not os.path.isdir(_dir):
    os.makedirs(_dir)
with open(OUT, "w") as f:
    json.dump(data, f, indent=1)
    f.write("\n")
print("[export_user] captured %d name(s), %d comment(s), %d local(s) -> %s"
      % (len(functions), len(comments), len(locals_out), OUT))
