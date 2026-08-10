#!/usr/bin/env python3
"""Decode a shipped Gruntz.sav / SlotN.sav statically - the save subsystem's ground truth.

WHY
  When a save/load defect is reported, the FILES are evidence and reading them
  settles in seconds what a percentage argument cannot settle at all.  This
  decoder answered three questions on 2026-08-10 that inference had got wrong:

    * Gruntz.sav's index is well formed, its checksum matches ComputeAll(), and
      the used slots carry the right names and SlotN.sav paths - so "the save
      does not appear" is NOT a corrupt or unwritten index.
    * SlotN.sav ends with a complete 0x3843a-byte preview BMP (320x240x24), so
      ChainForward -> SaveScreenshot -> SaveFile RAN TO COMPLETION and that save
      SUCCEEDED - refuting "SnapshotChildren returned 0".
    * The CTriggerMgr grid record parses field-for-field with our reader, and
      the cell the ClearGridRange crash faulted on holds a genuine LOGIC_GRUNT
      that IS present in the object table - so the crash is an unchained grunt,
      not a type confusion.

FORMATS
  Gruntz.sav = the 0xa1c bytes starting at CSaveGame+0x8 (m_header[4],
  m_maxLevel, m_curLevel, m_magic, ...) followed by SaveSlot m_slots[10].  Each
  0x100-byte slot is obfuscated by CSaveGame::Encode as `buf[i] ^= i`, and
  m_header[2] is the sum over all slots of `sum(byte[i] * i)` on the DECODED
  bytes (CSaveGame::ComputeAll / ::Verify).

  SlotN.sav = CSnapshotHeader (0x120) + the SERIAL_SNAPSHOT_BEGIN payload +
  m_childCount x WwdSnapshot (0xa0) + the phase stream, with the preview BMP
  appended by SaveScreenshot.  The object table is located by its stride: 0xa0
  records whose +8 is a known LoadableClassId and whose +0x14 is a printable
  worker name.  The CTriggerMgr grid record is located by its own invariant -
  60 object ids followed by m_rowCount[4] where rowCount[r] equals the number
  of non-zero ids in row r.

USAGE
  python -m gruntz.audit.savegame_dump <dir-or-file> ...
"""

import argparse
import re
import struct
import sys
from collections import Counter
from pathlib import Path

SLOT = 0x100
NSLOT = 10
INDEX_HDR = 0xA1C
REC = 0xA0
SNAP_HDR = 0x120
PREVIEW_BYTES = 0x3843A
CLASSIDS = {5: "WWDOBJA", 6: "WWDOBJC", 0x10: "CALLBACKOBJ", 0x16: "WWDOBJF", 0x1B: "WWDOBJB"}
LOGIC_GRUNT = 0x3E8


def cstr(b):
    z = b.find(b"\0")
    return b[:z if z >= 0 else len(b)].decode("latin1")


def dump_index(path, d):
    print("%s  %d B  (index; expect %d)" % (path, len(d), INDEX_HDR + NSLOT * SLOT))
    h = struct.unpack_from("<4i", d, 0)
    maxl, curl, magic = struct.unpack_from("<3I", d, 0x10)
    print("  m_header=[%d, %d, 0x%08x, %d]  m_maxLevel=%d m_curLevel=%d m_magic=0x%x"
          % (h[0], h[1], h[2] & 0xFFFFFFFF, h[3], maxl, curl, magic))
    total = 0
    for i in range(NSLOT):
        raw = bytearray(d[INDEX_HDR + i * SLOT: INDEX_HDR + (i + 1) * SLOT])
        acc = 0
        for j in range(SLOT):
            raw[j] ^= j
            acc += raw[j] * j
        total += acc
        ty, lvl, cnt, act, chk = struct.unpack_from("<5i", raw, 0)
        lo, hi = struct.unpack_from("<2i", raw, 0xF8)
        print("  slot %d %s type=0x%x lvlId=%d count=%d active=%d chk=0x%08x"
              % (i, "USED" if ty & 1 else "----", ty, lvl, cnt, act, chk & 0xFFFFFFFF))
        print("         name=%-22r path=%-14r levelName=%r custom=%d won=%d"
              % (cstr(raw[0x14:0x34]), cstr(raw[0x35:0x75]),
                 cstr(raw[0x75:0xF8]), lo, hi))
    ok = (total & 0xFFFFFFFF) == (h[2] & 0xFFFFFFFF)
    print("  checksum sum=0x%08x header[2]=0x%08x  %s"
          % (total & 0xFFFFFFFF, h[2] & 0xFFFFFFFF, "MATCH" if ok else "MISMATCH"))


def dump_slot(path, d):
    ver, mon, day, yr = struct.unpack_from("<4i", d, 0)
    nchild, idctr = struct.unpack_from("<2I", d, 0x110)
    print("%s  %d B  (snapshot)" % (path, len(d)))
    print("  header: v%d %04d-%02d-%02d name=%r childCount=%d objIdCounter=%d"
          % (ver, yr, mon, day, cstr(d[0x10:0x110]), nchild, idctr))

    bm = d.find(b"BM", 0x1000)
    if bm >= 0 and len(d) - bm == PREVIEW_BYTES:
        size, off = struct.unpack_from("<I", d, bm + 2)[0], struct.unpack_from("<I", d, bm + 10)[0]
        w, hgt, _pl, bpp = struct.unpack_from("<iiHH", d, bm + 14 + 4)
        print("  preview: COMPLETE at 0x%x, %d B (=SAVE_PREVIEW_BYTES), %dx%dx%d, bfSize=%d "
              "bfOffBits=0x%x -> SaveScreenshot ran to completion"
              % (bm, len(d) - bm, w, hgt, bpp, size, off))
    else:
        print("  preview: ABSENT or truncated (expected %d trailing bytes) -> "
              "ChainForward/SaveScreenshot did not finish" % PREVIEW_BYTES)

    def plausible(off):
        for k in range(min(nchild, 40)):
            o = off + k * REC
            if o + REC > len(d):
                return False
            if struct.unpack_from("<i", d, o + 8)[0] not in CLASSIDS:
                return False
            nm = cstr(d[o + 0x14:o + 0x94])
            if not nm or not re.fullmatch(r"[A-Za-z0-9_ ]+", nm):
                return False
        return True

    base = next((o for o in range(SNAP_HDR, 0x900) if plausible(o)), None)
    if base is None:
        print("  !! object table not found")
        return
    objs, kinds = {}, Counter()
    for k in range(nchild):
        o = base + k * REC
        mid, oid, cid, stid, ltid = struct.unpack_from("<5i", d, o)
        nm = cstr(d[o + 0x14:o + 0x94])
        x, y, sk = struct.unpack_from("<3i", d, o + 0x94)
        objs[oid] = (nm, ltid, cid, x, y)
        kinds[nm] += 1
    end = base + nchild * REC
    print("  object table @0x%x  %d records  workers: %s"
          % (base, nchild, kinds.most_common(10)))

    for off in range(end, len(d) - 300, 4):
        ids = struct.unpack_from("<60i", d, off)
        nz = [v for v in ids if v]
        if not (1 <= len(nz) <= 60) or any(v not in objs for v in nz):
            continue
        rc = struct.unpack_from("<4i", d, off + 240)
        per = [sum(1 for v in ids[r * 15:(r + 1) * 15] if v) for r in range(4)]
        if list(rc) != per:
            continue
        print("  CTriggerMgr grid @0x%x  rowCount=%s" % (off, per))
        for r in range(4):
            for c, v in enumerate(ids[r * 15:(r + 1) * 15]):
                if not v:
                    continue
                nm, ltid, cid, x, y = objs[v]
                warn = "" if ltid == LOGIC_GRUNT else \
                    "   <-- logicTypeId 0x%x is NOT LOGIC_GRUNT" % ltid
                print("    [%d][%2d] id=%-5d %-20s logicType=0x%03x %-11s pos=(%d,%d)%s"
                      % (r, c, v, nm, ltid, CLASSIDS.get(cid, cid), x, y, warn))
        return
    print("  !! grid record not found")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="+", help="Gruntz.sav / SlotN.sav / a game directory")
    args = ap.parse_args(argv)
    files = []
    for p in map(Path, args.paths):
        files.extend(sorted(p.glob("*.sav")) if p.is_dir() else [p])
    for f in files:
        d = f.read_bytes()
        if len(d) == INDEX_HDR + NSLOT * SLOT:
            dump_index(str(f), d)
        else:
            dump_slot(str(f), d)
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
