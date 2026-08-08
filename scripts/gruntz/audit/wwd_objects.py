"""Walk every object record of every shipped WWD and emit the four strings.

`docs/formats/wwd-v1.md` proves the layout; this is the traversal that turns it
into data. Each object record is a 0x11c-byte fixed part followed by four
packed, length-prefixed strings -- ``name`` / ``logic`` / ``image_set`` /
``animation`` -- whose byte counts live at record +0x04..+0x10
(``CDDrawWorkerHost::ReadPlaneObjects`` @0x162af0).

The point of a real walk rather than a regex over the inflated block is that a
regex cannot say WHICH of the four fields a string is, nor which level, plane
or object it belongs to. Those bindings are what cross-check our class names.

Usage::

    python -m gruntz.audit.wwd_objects <dir-or-file>... [--tsv out.tsv]

Directories are searched recursively for ``*.wwd`` / ``*.WWD``, so either a
directory of extracted resources or a whole ``rezpack unpack`` tree works.
"""

from __future__ import annotations

import argparse
import pathlib
import struct
import sys
import zlib
from typing import Iterator, NamedTuple

HEADER_SIZE = 0x5F4
PLANE_HEADER_SIZE = 0xA0
OBJECT_FIXED_SIZE = 0x11C

# File-header offsets (docs/formats/wwd-v1.md "Main header").
H_FLAGS = 0x008
H_LEVEL_NAME = 0x010
H_AUTHOR = 0x050
H_CREATED = 0x090
H_REZ_FILE = 0x0D0
H_TILE_DIR = 0x1D0
H_NUM_PLANES = 0x2DC
H_PLANES_OFFSET = 0x2E0
H_MAIN_BLOCK_LEN = 0x2E8
H_LAUNCH_APP = 0x2F4
H_IMAGE_DIR0 = 0x374

# Plane-header offsets ("Plane header").
P_FLAGS = 0x08
P_NAME = 0x10
P_OBJECTS_COUNT = 0x80
P_OBJECTS_OFFSET = 0x8C

# Object-record offsets ("Object records").
O_ID = 0x00
O_STRLENS = 0x04  # four u32: name, logic, image_set, animation
O_X, O_Y, O_Z = 0x14, 0x18, 0x1C
O_GRID_INDEX = 0x20
O_FLAGS_DYNAMIC = 0x28
O_FLAGS_DRAW = 0x2C
O_FLAGS_USER = 0x30
O_USER_VALUES = 0x34  # six i32: score points powerup damage smarts health
O_OBJECT_TYPE = 0x10C
O_HIT_TYPE = 0x110
O_MOVE_RES_X, O_MOVE_RES_Y = 0x114, 0x118


class Obj(NamedTuple):
    level: str
    plane: str
    index: int
    obj_id: int
    x: int
    y: int
    z: int
    object_type: int
    hit_type: int
    flags_dynamic: int
    flags_draw: int
    flags_user: int
    user_values: tuple
    name: str
    logic: str
    image_set: str
    animation: str


def _cstr(buf: bytes, off: int, cap: int) -> str:
    raw = buf[off : off + cap]
    end = raw.find(b"\0")
    if end >= 0:
        raw = raw[:end]
    return raw.decode("latin-1")


def inflate(raw: bytes) -> bytes:
    """`header || main block`, the layout WwdFile_InflateMainBlock @0x160790 builds."""
    (header_size,) = struct.unpack_from("<I", raw, 0)
    (flags,) = struct.unpack_from("<I", raw, H_FLAGS)
    if not flags & 2:
        return raw
    body = zlib.decompress(raw[header_size:])
    return raw[:header_size] + body


def walk(path: pathlib.Path) -> Iterator[Obj]:
    raw = path.read_bytes()
    img = inflate(raw)
    level = _cstr(img, H_LEVEL_NAME, 0x40)
    (num_planes,) = struct.unpack_from("<I", img, H_NUM_PLANES)
    (planes_off,) = struct.unpack_from("<I", img, H_PLANES_OFFSET)
    for p in range(num_planes):
        ph = planes_off + p * PLANE_HEADER_SIZE
        plane = _cstr(img, ph + P_NAME, 0x40)
        (count,) = struct.unpack_from("<I", img, ph + P_OBJECTS_COUNT)
        (off,) = struct.unpack_from("<I", img, ph + P_OBJECTS_OFFSET)
        cur = off
        for i in range(count):
            rec = img[cur : cur + OBJECT_FIXED_SIZE]
            lens = struct.unpack_from("<4I", rec, O_STRLENS)
            s = cur + OBJECT_FIXED_SIZE
            strs = []
            for n in lens:
                strs.append(img[s : s + n].split(b"\0")[0].decode("latin-1"))
                s += n
            yield Obj(
                level=level,
                plane=plane,
                index=i,
                obj_id=struct.unpack_from("<i", rec, O_ID)[0],
                x=struct.unpack_from("<i", rec, O_X)[0],
                y=struct.unpack_from("<i", rec, O_Y)[0],
                z=struct.unpack_from("<i", rec, O_Z)[0],
                object_type=struct.unpack_from("<i", rec, O_OBJECT_TYPE)[0],
                hit_type=struct.unpack_from("<i", rec, O_HIT_TYPE)[0],
                flags_dynamic=struct.unpack_from("<I", rec, O_FLAGS_DYNAMIC)[0],
                flags_draw=struct.unpack_from("<I", rec, O_FLAGS_DRAW)[0],
                flags_user=struct.unpack_from("<I", rec, O_FLAGS_USER)[0],
                user_values=struct.unpack_from("<6i", rec, O_USER_VALUES),
                name=strs[0],
                logic=strs[1],
                image_set=strs[2],
                animation=strs[3],
            )
            cur = s


def header_strings(path: pathlib.Path) -> dict:
    img = inflate(path.read_bytes())
    return {
        "level_name": _cstr(img, H_LEVEL_NAME, 0x40),
        "author": _cstr(img, H_AUTHOR, 0x40),
        "created": _cstr(img, H_CREATED, 0x40),
        "rez_file": _cstr(img, H_REZ_FILE, 0x100),
        "tile_directory": _cstr(img, H_TILE_DIR, 0x80),
        "launch_app": _cstr(img, H_LAUNCH_APP, 0x80),
        "image_directory": _cstr(img, H_IMAGE_DIR0, 0x80),
    }


def collect(paths) -> list:
    out = []
    for p in paths:
        if p.is_dir():
            out.extend(q for q in p.rglob("*") if q.is_file() and q.suffix.lower() == ".wwd")
        elif p.is_file():
            out.append(p)
    # A rezpack tree repeats leaf names across areas; key by the REZ-ish path.
    return sorted(set(out))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("path", type=pathlib.Path, nargs="+")
    ap.add_argument("--tsv", type=pathlib.Path)
    args = ap.parse_args(argv)

    files = collect(args.path)
    if not files:
        print("[wwd_objects] no .wwd files found", file=sys.stderr)
        return 1

    rows = []
    for f in files:
        for o in walk(f):
            rows.append((f.stem, o))

    out = args.tsv.open("w") if args.tsv else sys.stdout
    cols = (
        "file\tlevel\tplane\tindex\tid\tx\ty\tz\tobject_type\thit_type"
        "\tflags_dynamic\tflags_draw\tflags_user\tscore\tpoints\tpowerup"
        "\tdamage\tsmarts\thealth\tname\tlogic\timage_set\tanimation"
    )
    print(cols, file=out)
    for stem, o in rows:
        print(
            "\t".join(
                str(v)
                for v in (
                    stem,
                    o.level,
                    o.plane,
                    o.index,
                    o.obj_id,
                    o.x,
                    o.y,
                    o.z,
                    o.object_type,
                    o.hit_type,
                    o.flags_dynamic,
                    o.flags_draw,
                    o.flags_user,
                    *o.user_values,
                    o.name,
                    o.logic,
                    o.image_set,
                    o.animation,
                )
            ),
            file=out,
        )
    if args.tsv:
        out.close()
        print(f"[wwd_objects] {len(rows)} records -> {args.tsv}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
