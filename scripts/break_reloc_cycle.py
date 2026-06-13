#!/usr/bin/env python3
"""Break base-reloc pointer cycles that make vostok-delinker recurse forever.

vostok-delinker resolves every .rdata/.data absolute relocation to the nearest
named symbol and, in `ObjectFile::add_relocation_at`, recursively follows the
pointer chain: a Constant/Static reloc whose *target* address is itself another
relocation source is chased via `relocs_rva.get(&target_rva)`. The delinker has
no visited-set guard, so a cyclic pointer chain (in particular a self-pointer
where location == target) recurses infinitely and overflows the stack.

GRUNTZ.EXE contains exactly one such node: a self-referential .data pointer at
RVA 0x2169e0 (Ghidra: `PTR_LOOP_006169e0`, VA 0x6169e0 -> 0x6169e0). Every
reported "cycle" funnels into this single self-loop.

This tool writes a *working copy* of the EXE in which each detected cycle node's
4-byte pointer is redirected to the nearest preceding .data address that is NOT
itself a relocation source, so the delinker's recursion terminates after one
hop. It touches only data pointer words (never code), reports every edit, and
leaves the original EXE untouched. The base-reloc table is unchanged, so the
delinker still discovers and rewrites the exact same set of relocations; only
the dword *value* at the cyclic node differs (one data word per cycle).

This is a delinker-recursion workaround, not a semantic change to the game; the
right long-term fix is a visited-set guard in the delinker's recursion.
"""

import argparse
import shutil
import struct
import sys


def parse_pe(data):
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe_off:pe_off + 4] == b"PE\0\0"
    num_sec = struct.unpack_from("<H", data, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    opt_off = pe_off + 24
    image_base = struct.unpack_from("<I", data, opt_off + 28)[0]
    size_of_image = struct.unpack_from("<I", data, opt_off + 56)[0]
    sec_off = opt_off + opt_size
    secs = {}
    for i in range(num_sec):
        b = sec_off + i * 40
        name = data[b:b + 8].rstrip(b"\0").decode()
        vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, b + 8)
        secs[name] = dict(vaddr=vaddr, vsize=vsize, rawptr=rawptr, rawsize=rawsize)
    return image_base, size_of_image, secs


def map_image(data, size_of_image, secs):
    mapped = bytearray(size_of_image)
    for s in secs.values():
        chunk = data[s["rawptr"]:s["rawptr"] + s["rawsize"]]
        mapped[s["vaddr"]:s["vaddr"] + len(chunk)] = chunk
    return mapped


def read_relocs(data, image_base, mapped, secs):
    """Return {reloc_rva: target_rva} for all HIGHLOW base relocations."""
    r = secs[".reloc"]
    rd = data[r["rawptr"]:r["rawptr"] + r["rawsize"]]
    relocs = {}
    pos = 0
    while pos + 8 <= len(rd):
        page_rva, block_size = struct.unpack_from("<II", rd, pos)
        if block_size < 8:
            break
        nent = (block_size - 8) // 2
        for i in range(nent):
            e = struct.unpack_from("<H", rd, pos + 8 + i * 2)[0]
            if (e >> 12) != 3:  # IMAGE_REL_BASED_HIGHLOW
                continue
            rrva = page_rva + (e & 0xFFF)
            relocs[rrva] = struct.unpack_from("<I", mapped, rrva)[0] - image_base
        pos += block_size
    return relocs


def section_of(rva, secs):
    for name in (".text", ".rdata", ".data"):
        s = secs[name]
        if s["vaddr"] <= rva < s["vaddr"] + s["vsize"]:
            return name
    return None


def find_cycle_nodes(relocs, secs):
    """Detect cycles in the delinker's recursion graph.

    Edge N -> target exists only when target lands in .rdata/.data (the
    Constant/Static recursion). Each node has at most one out-edge, so cycles
    are simple to find by walking.
    """
    def out_edge(n):
        t = relocs.get(n)
        if t is None:
            return None
        return t if section_of(t, secs) in (".rdata", ".data") else None

    done = set()
    cycle_nodes = set()
    for start in relocs:
        if start in done:
            continue
        path, seen, cur = [], set(), start
        while cur is not None and cur not in done and cur not in seen:
            seen.add(cur)
            path.append(cur)
            cur = out_edge(cur)
        if cur in seen:  # closed a loop
            idx = path.index(cur)
            cycle_nodes.update(path[idx:])
        done.update(path)
    return cycle_nodes


def file_offset_of(rva, secs):
    for s in secs.values():
        if s["vaddr"] <= rva < s["vaddr"] + s["rawsize"]:
            return s["rawptr"] + (rva - s["vaddr"])
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--in-exe", required=True)
    ap.add_argument("--out-exe", required=True)
    args = ap.parse_args()

    with open(args.in_exe, "rb") as f:
        data = f.read()
    image_base, size_of_image, secs = parse_pe(data)
    mapped = map_image(data, size_of_image, secs)
    relocs = read_relocs(data, image_base, mapped, secs)
    reloc_locs = set(relocs)
    cycle_nodes = find_cycle_nodes(relocs, secs)

    print("[break_reloc_cycle] HIGHLOW relocs: %d ; cycle nodes: %d"
          % (len(relocs), len(cycle_nodes)), file=sys.stderr)
    if not cycle_nodes:
        print("[break_reloc_cycle] no cycles; copying EXE unchanged", file=sys.stderr)

    shutil.copyfile(args.in_exe, args.out_exe)
    edits = []
    with open(args.out_exe, "r+b") as f:
        for node in sorted(cycle_nodes):
            data_base = secs[".data"]["vaddr"]
            redirect = None
            for cand in range(node - 4, data_base, -4):
                if cand not in reloc_locs:
                    redirect = cand
                    break
            if redirect is None:
                raise RuntimeError("no safe redirect target for node 0x%x" % node)
            off = file_offset_of(node, secs)
            new_va = redirect + image_base
            f.seek(off)
            f.write(struct.pack("<I", new_va))
            edits.append((node, off, relocs[node] + image_base, new_va))

    for node, off, old_va, new_va in edits:
        print("[break_reloc_cycle] node RVA 0x%x (file 0x%x): VA 0x%x -> 0x%x"
              % (node, off, old_va, new_va), file=sys.stderr)
    print("[break_reloc_cycle] wrote %s" % args.out_exe, file=sys.stderr)


if __name__ == "__main__":
    main()
