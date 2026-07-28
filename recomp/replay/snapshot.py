#!/usr/bin/env python3
r"""snapshot.py - read the record/replay snapshot format from Python.

The format is deliberately dumb (see snapshot.h): a fixed header, a fixed-size
region table, then raw blobs. So this is `struct.unpack` and nothing else - no
parser, no schema, no dependency on the C side beyond the field order.

    python recomp/replay/snapshot.py build/replay/entry.snap
    python recomp/replay/snapshot.py build/replay/entry.snap --regions
    python recomp/replay/snapshot.py build/replay/entry.snap --at 0x01a5610c --len 16
    python recomp/replay/snapshot.py build/replay/entry.snap --diff build/replay/exit.snap
"""
import argparse
import struct
import sys
from pathlib import Path

HDR = "<8s17I10I64s"
HDR_FIELDS = ("magic version flags phase hdr_size region_size n_regions region_off "
              "total_size image_base target_rva hit_index entry_esp ret_addr "
              "stack_base stack_end tick pad0").split()
REG = "<10I40s"
REG_FIELDS = ("base size protect alloc_base alloc_protect state type klass rflags "
              "blob_off").split()
REGS = "eax ecx edx ebx esp ebp esi edi eflags eip".split()

KLASS = {1: "game-image", 2: "other-image", 3: "private", 4: "mapped", 5: "hook-stack"}
RF_BYTES, RF_WRITABLE = 1, 2


class Snapshot:
    def __init__(self, path):
        self.path = Path(path)
        self.d = self.path.read_bytes()
        vals = struct.unpack_from(HDR, self.d, 0)
        self.h = dict(zip(HDR_FIELDS, vals[:len(HDR_FIELDS)]))
        if self.h["magic"] != b"GRZSNAP\x01":
            raise SystemExit("%s: not a snapshot (magic %r)" % (path, self.h["magic"]))
        self.regs = dict(zip(REGS, vals[len(HDR_FIELDS):len(HDR_FIELDS) + 10]))
        self.name = vals[-1].split(b"\0")[0].decode("latin1")
        assert self.h["hdr_size"] == struct.calcsize(HDR), (
            "header is %d bytes, this reader expects %d"
            % (self.h["hdr_size"], struct.calcsize(HDR)))
        assert self.h["region_size"] == struct.calcsize(REG), "region size mismatch"
        self.regions = []
        for i in range(self.h["n_regions"]):
            o = self.h["region_off"] + i * self.h["region_size"]
            v = struct.unpack_from(REG, self.d, o)
            r = dict(zip(REG_FIELDS, v[:10]))
            r["module"] = v[10].split(b"\0")[0].decode("latin1")
            self.regions.append(r)
        assert self.regions, "region table came out empty"

    def bytes_at(self, addr, n):
        """The recorded bytes at a VIRTUAL address, or None."""
        for r in self.regions:
            if r["base"] <= addr < r["base"] + r["size"] and (r["rflags"] & RF_BYTES):
                off = r["blob_off"] + (addr - r["base"])
                return self.d[off:off + min(n, r["base"] + r["size"] - addr)]
        return None

    def region_of(self, addr):
        for r in self.regions:
            if r["base"] <= addr < r["base"] + r["size"]:
                return r
        return None

    def summary(self):
        tot = sum(r["size"] for r in self.regions if r["rflags"] & RF_BYTES)
        return ("%s\n  target %s rva=%08x image=%08x hit=%d phase=%d flags=%d\n"
                "  %d regions, %d bytes of content, file %d bytes\n"
                "  entry_esp=%08x ret=%08x stack=%08x..%08x\n"
                "  regs %s" % (
                    self.path, self.name, self.h["target_rva"], self.h["image_base"],
                    self.h["hit_index"], self.h["phase"], self.h["flags"],
                    len(self.regions), tot, len(self.d),
                    self.h["entry_esp"], self.h["ret_addr"], self.h["stack_base"],
                    self.h["stack_end"],
                    " ".join("%s=%08x" % (k, self.regs[k]) for k in REGS)))


def diff(a, b, limit=64):
    """Byte differences between two snapshots, region by region, in address order."""
    out = []
    by_base = {(r["base"], r["size"]): r for r in b.regions}
    for r in a.regions:
        if not (r["rflags"] & RF_BYTES):
            continue
        o = by_base.get((r["base"], r["size"]))
        if o is None or not (o["rflags"] & RF_BYTES):
            out.append((r["base"], None, None, "region absent from %s" % b.path.name))
            continue
        x = a.d[r["blob_off"]:r["blob_off"] + r["size"]]
        y = b.d[o["blob_off"]:o["blob_off"] + r["size"]]
        if x == y:
            continue
        for i in range(r["size"]):
            if x[i] != y[i]:
                out.append((r["base"] + i, x[i], y[i],
                            "%s %s" % (KLASS.get(r["klass"], "?"), r["module"])))
                if len(out) >= limit:
                    return out
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("snap")
    ap.add_argument("--regions", action="store_true")
    ap.add_argument("--at", type=lambda s: int(s, 0))
    ap.add_argument("--len", type=lambda s: int(s, 0), default=32)
    ap.add_argument("--diff", help="second snapshot to diff against")
    ap.add_argument("--limit", type=int, default=64)
    a = ap.parse_args()

    s = Snapshot(a.snap)
    print(s.summary())

    if a.regions:
        print("\n     base     size  prot klass        W B module")
        for r in s.regions:
            print("%08x %8x %5x %-12s %d %d %s"
                  % (r["base"], r["size"], r["protect"], KLASS.get(r["klass"], "?"),
                     1 if r["rflags"] & RF_WRITABLE else 0,
                     1 if r["rflags"] & RF_BYTES else 0, r["module"]))

    if a.at is not None:
        b = s.bytes_at(a.at, a.len)
        r = s.region_of(a.at)
        print("\n%08x in %s" % (a.at, ("%08x+%x %s" % (r["base"], r["size"],
                                                       KLASS.get(r["klass"], "?")))
                                if r else "NO RECORDED REGION"))
        if b:
            for i in range(0, len(b), 16):
                print("  %08x  %s" % (a.at + i, " ".join("%02x" % c for c in b[i:i + 16])))

    if a.diff:
        o = Snapshot(a.diff)
        d = diff(s, o, a.limit)
        print("\ndiff %s -> %s: %d difference(s)%s"
              % (s.path.name, o.path.name, len(d),
                 " (capped)" if len(d) >= a.limit else ""))
        for addr, x, y, note in d:
            if x is None:
                print("  %08x  %s" % (addr, note))
            else:
                print("  %08x  %02x -> %02x   [%s]" % (addr, x, y, note))
    return 0


if __name__ == "__main__":
    sys.exit(main())
