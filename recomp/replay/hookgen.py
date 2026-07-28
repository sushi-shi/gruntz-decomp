#!/usr/bin/env python3
r"""hookgen.py - find the `call rel32` sites that reach a function.

The capture DLL patches CALL SITES rather than function prologues (no
instruction-length decoding, and the target's bytes stay pristine - see
capture.c). That needs the site list, which is a purely static question:

    e8 <rel32>   where site + 5 + rel32 == target
                 ... or == an ILT thunk (`e9 <rel32>`) that jumps to target

MSVC 5.0 with incremental linking routes almost every intra-image call through
a thunk in the 0x1000-0x9000 band, so the second case is the common one.

    python recomp/replay/hookgen.py 0x000f9280
    python recomp/replay/hookgen.py 0x000f9280 --cfg      # capture.cfg lines

Indirect calls (virtual dispatch, function pointers) are NOT found by this and
cannot be - they have no call site to patch. A target reached only indirectly
needs a prologue detour instead; the census in `--stats` tells you whether the
direct sites plausibly cover the call graph.
"""
import argparse
import struct
import sys
from pathlib import Path

REPO = next(p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists())
EXE = REPO / "build" / "exe" / "GRUNTZ.EXE"


class Image:
    def __init__(self, path):
        self.d = Path(path).read_bytes()
        lfa = struct.unpack_from("<I", self.d, 0x3C)[0]
        nsec = struct.unpack_from("<H", self.d, lfa + 6)[0]
        optsz = struct.unpack_from("<H", self.d, lfa + 20)[0]
        self.secs = []
        for i in range(nsec):
            o = lfa + 24 + optsz + i * 40
            name = self.d[o:o + 8].rstrip(b"\0").decode("latin1")
            va, rawsz, rawptr = struct.unpack_from("<III", self.d, o + 12)
            self.secs.append((name, va, rawsz, rawptr))
        assert self.secs, "no sections parsed"
        self.text = next(s for s in self.secs if s[0] == ".text")
        self._index()

    def _index(self):
        """One pass over .text: destination -> [call sites], and -> [jmp thunks].

        Byte-at-a-time in Python over 2 MB is slow enough to notice and this is
        called per target, so it is built once. Overlapping/misaligned matches
        are inherent to scanning x86 linearly; a false positive would have to be
        an `e8`/`e9` byte inside another instruction whose next four bytes
        happen to encode exactly this target, and it is filtered later by the
        capture DLL, which refuses to patch a site that is not `e8`."""
        _, va, sz, ptr = self.text
        blob = self.d[ptr:ptr + sz]
        self.calls = {}
        self.jmps = {}
        for i in range(len(blob) - 5):
            op = blob[i]
            if op != 0xE8 and op != 0xE9:
                continue
            dest = va + i + 5 + struct.unpack_from("<i", blob, i + 1)[0]
            (self.calls if op == 0xE8 else self.jmps).setdefault(dest, []).append(va + i)
        assert self.calls and self.jmps, "call/jmp index came out empty"

    def off(self, rva):
        for _, va, sz, p in self.secs:
            if va <= rva < va + sz:
                return p + (rva - va)
        return None

    def i32(self, rva):
        return struct.unpack_from("<i", self.d, self.off(rva))[0]

    def u8(self, rva):
        return self.d[self.off(rva)]


def sites_for(img, target):
    """(ILT thunks jumping to target, direct call sites reaching either)."""
    th = [t for t in img.jmps.get(target, []) if t < img.text[1] + 0x9000]
    sites = []
    for dest in [target] + th:
        sites.extend(img.calls.get(dest, []))
    return th, sorted(set(sites))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("target", nargs="+", help="target RVA(s), e.g. 0x000f9280")
    ap.add_argument("--exe", default=str(EXE))
    ap.add_argument("--cfg", action="store_true", help="emit capture.cfg site= lines")
    ap.add_argument("--probe", action="store_true", help="emit a probe= line per target")
    a = ap.parse_args()

    img = Image(a.exe)
    for t in a.target:
        target = int(t, 0)
        th, sites = sites_for(img, target)
        if a.probe:
            print("probe=" + ",".join("0x%08x" % x for x in [target] + sites))
        elif a.cfg:
            print("target=0x%08x" % target)
            for s in sites:
                print("site=0x%08x" % s)
        else:
            print("target 0x%08x: %d ILT thunk(s) %s, %d direct call site(s)"
                  % (target, len(th), [hex(x) for x in th], len(sites)))
            for s in sites:
                print("   site 0x%08x" % s)
        if not sites:
            print("   (no direct call sites - reached only indirectly?)",
                  file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
