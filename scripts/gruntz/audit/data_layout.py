#!/usr/bin/env python3
"""gruntz.audit.data_layout - predict / invert MSVC 5.0 per-TU data layout.

The algorithm was reverse-engineered from the toolchain binaries themselves
(c1xx.dll front end + c2.exe back end) and probe-verified; the full write-up
lives in docs/compiler-data-layout.md. Summary:

  .data / .rdata (initialized):
      emitted in DECLARATION order (the front end streams initializer records
      at the point of definition; c2 assigns offsets as they arrive).
  .bss (uninitialized, C++ TU):
      emitted by c1xx's end-of-TU walk over the global scope HASH TABLE:
          h = 0; for c in name: h = h*4 + (h>>4) + c        (uint32)
          check16 = (h >> 16) ^ (h & 0xFFFF)                 [c1xx 0x1040e132]
          bucket  = check16 & 0x3FF                          [1024 buckets,
                                                              insert 0x1040b2b6]
      walk = buckets ascending, chains LIFO (reverse first-declaration order)
      [walk fn c1xx 0x10403ea6, vtable slot 9/10 of the scope-table class].
      The hashed NAME is the plain identifier for file-scope symbols, and the
      C++ DECORATED name for function-local statics (?x@?1??fn@@...@4HA) and
      class statics (?x@CFoo@@2HA) - the front end interns those eagerly.
  alignment (c2-side, per section, evolves in emission order):
      scalars:  double/i64 -> 8, everything else (char, short, int, float,
                pointers) -> 4  (yes, chars occupy 4-aligned slots)
      arrays/aggregates: size > 8 -> 8
                         size < 4 -> 4
                         4 <= size <= 8 -> the section's current RATCHET:
                             8 if some earlier object in this section got 8,
                             else 4   (c2 reuses the section-align ratchet)
      no trailing pad after the last object.

Forward mode (predict):
    python -m gruntz.audit.data_layout predict decls.tsv
  decls.tsv columns (tab-separated, in DECLARATION order):
    name  kind  size  init
      name : source identifier, or the decorated name for local/class statics
      kind : scalar|double|array   (double also covers __int64/long long)
      size : bytes
      init : none|data|rdata   (none -> .bss; data -> .data; rdata -> .rdata)
  An `extern` declaration that precedes the definition may be given as its own
  row with kind=extern (it pins the chain slot); the later definition row is
  then matched by name.

Reverse mode (infer, homm2-style):
    python -m gruntz.audit.data_layout infer observed.tsv
  observed.tsv columns: offset name size?  (ascending .bss offsets; name '?'
  for unknown slots). Emits:
    - hash-consistency check of the named symbols (flags misattributions),
    - for '?' slots: the [lo,hi] bucket window the occupant's name must hash
      into (candidate-name sieve),
    - the relative-declaration-order constraints recoverable from LIFO ties
      (same-bucket neighbours are in reverse declaration order).
"""

import sys
import argparse


def name_hash(name: str) -> int:
    """c1xx identifier hash: h = h*4 + (h>>4) + c, 32-bit."""
    h = 0
    for ch in name:
        h = (h * 4 + (h >> 4) + ord(ch)) & 0xFFFFFFFF
    return h


def check16(name: str) -> int:
    h = name_hash(name)
    return ((h >> 16) ^ h) & 0xFFFF


GLOBAL_BUCKETS_MASK = 0x3FF  # 1024 buckets in the global scope table


def bucket(name: str) -> int:
    return check16(name) & GLOBAL_BUCKETS_MASK


def obj_align(kind: str, size: int, ratchet: int) -> int:
    """c2 per-object alignment. ratchet = section's max align so far (4 or 8)."""
    if kind == "double":
        return 8
    if kind == "scalar":
        return 4
    # array / aggregate
    if size > 8:
        return 8
    if size < 4:
        return 4
    return ratchet


def layout_stream(decls):
    """Place a list of (name, kind, size) in order; returns (rows, secalign)."""
    off = 0
    ratchet = 4
    rows = []
    for name, kind, size in decls:
        a = obj_align(kind, size, ratchet)
        off = (off + a - 1) & ~(a - 1)
        rows.append((off, name, size, a))
        off += size
        ratchet = max(ratchet, a)
    return rows, ratchet


def predict(decl_rows):
    """decl_rows: list of dicts with name/kind/size/init in declaration order."""
    # chain slot is pinned by FIRST declaration (extern decl counts)
    first_index = {}
    defs = []
    for i, d in enumerate(decl_rows):
        nm = d["name"]
        if nm not in first_index:
            first_index[nm] = i
        if d["kind"] != "extern":
            defs.append(d)

    out = {}
    # .data / .rdata: declaration order
    for sec, initkind in ((".data", "data"), (".rdata", "rdata")):
        seq = [(d["name"], d["kind"], d["size"]) for d in defs if d["init"] == initkind]
        out[sec] = layout_stream(seq)
    # .bss: bucket walk, LIFO chains by first-declaration index
    bss = [d for d in defs if d["init"] == "none"]
    walk = sorted(bss, key=lambda d: (bucket(d["name"]), -first_index[d["name"]]))
    out[".bss"] = layout_stream([(d["name"], d["kind"], d["size"]) for d in walk])
    return out


def cmd_predict(args):
    rows = []
    for ln in open(args.tsv):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        name, kind, size, init = ln.split("\t")
        rows.append({"name": name, "kind": kind, "size": int(size, 0), "init": init})
    out = predict(rows)
    for sec in (".data", ".rdata", ".bss"):
        placed, secalign = out[sec]
        if not placed:
            continue
        print(f"== {sec}  (section align {secalign})")
        for off, name, size, a in placed:
            b = f" bucket {bucket(name):#5x}" if sec == ".bss" else ""
            print(f"  {off:#06x} +{size:<5d} align {a}{b}  {name}")


def cmd_infer(args):
    rows = []
    for ln in open(args.tsv):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        parts = ln.split("\t")
        off = int(parts[0], 0)
        name = parts[1]
        size = int(parts[2], 0) if len(parts) > 2 else None
        rows.append([off, name, size])
    rows.sort(key=lambda r: r[0])

    named = [(off, nm) for off, nm, _ in rows if nm != "?"]
    print("== hash-consistency of named symbols (ascending offsets)")
    ok = True
    prev_b = -1
    prev_nm = None
    decl_edges = []
    for off, nm in named:
        b = bucket(nm)
        flag = ""
        if b < prev_b:
            flag = "  <-- OUT OF WALK ORDER (misattributed name, or not this TU's .bss)"
            ok = False
        if b == prev_b:
            decl_edges.append((nm, prev_nm))  # same bucket: nm declared BEFORE prev
            flag = f"  (collides with previous -> '{nm}' was declared BEFORE '{prev_nm}')"
        print(f"  {off:#06x} bucket {b:#5x}  {nm}{flag}")
        prev_b, prev_nm = b, nm
    print("  =>", "CONSISTENT" if ok else "INCONSISTENT")

    unknowns = [(i, r) for i, r in enumerate(rows) if r[1] == "?"]
    if unknowns:
        print("== bucket windows for unknown slots")
        for i, r in unknowns:
            lo = 0
            hi = GLOBAL_BUCKETS_MASK
            for j in range(i - 1, -1, -1):
                if rows[j][1] != "?":
                    lo = bucket(rows[j][1])
                    break
            for j in range(i + 1, len(rows)):
                if rows[j][1] != "?":
                    hi = bucket(rows[j][1])
                    break
            print(f"  {r[0]:#06x}: name must hash into bucket [{lo:#x}, {hi:#x}]")
    if decl_edges:
        print("== declaration-order facts recovered from LIFO ties")
        for a, b in decl_edges:
            print(f"  '{a}' declared before '{b}'")


def cmd_hash(args):
    for nm in args.names:
        print(f"{nm}\th={name_hash(nm):#010x}\tcheck16={check16(nm):#06x}\tbucket={bucket(nm):#05x}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p1 = sub.add_parser("predict", help="declaration list -> predicted section layouts")
    p1.add_argument("tsv")
    p1.set_defaults(fn=cmd_predict)
    p2 = sub.add_parser("infer", help="observed slots -> naming/decl-order constraints")
    p2.add_argument("tsv")
    p2.set_defaults(fn=cmd_infer)
    p3 = sub.add_parser("hash", help="print h/check16/bucket for names")
    p3.add_argument("names", nargs="+")
    p3.set_defaults(fn=cmd_hash)
    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
