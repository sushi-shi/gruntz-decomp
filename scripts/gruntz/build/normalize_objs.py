#!/usr/bin/env python3
"""Batch-normalize base + target COFF objs into disposable objdiff comparison copies.

Thin ninja driver around `canonicalize_data_symbols.canonicalize_coff`: for every
manifest unit it rewrites the compiler-private data names (`$SG`/`$T`/`name$S<n>`)
and same-function jump-table `DIR32` labels of both the recompiled base obj and its
delinked target obj into a content-addressed, side-by-side view under
`build/objdiff/normalized/`. `objdiff.json` points at these copies; the real
`build/objdiff/base/` and `build/objdiff/target/` objects are never touched, so the
transform is matching-neutral (see canonicalize_data_symbols.py and the sibling
homm2 docs/data-symbol-normalization).

One ninja edge (keyed on the base objs + the delink stamp) drives the whole set;
per-object work is skipped when the normalized copy is already newer than its input
and the normalizer module, so a single-file edit only re-normalizes that one obj.
A `.symbols.tsv` sidecar is emitted next to each copy for auditing. The declared
output is a single stamp listing the processed set (mirroring the delink rule)."""
from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import canonicalize_data_symbols as canon  # noqa: E402

_MODULE_MTIME = max(
    Path(canon.__file__).stat().st_mtime,
    Path(__file__).stat().st_mtime,
)


def _stale(src: Path, out: Path) -> bool:
    if not out.exists():
        return True
    out_mtime = out.stat().st_mtime
    return out_mtime < src.stat().st_mtime or out_mtime < _MODULE_MTIME


def _normalize_one(src: Path, out_obj: Path, out_sidecar: Path) -> str:
    """Normalize src -> out_obj (+ sidecar) when stale. Return a state token."""
    if not _stale(src, out_obj) and not _stale(src, out_sidecar):
        return "skip"
    result = canon.canonicalize_coff(src.read_bytes())
    canon._atomic_write(out_obj, result.data)
    canon._atomic_write(out_sidecar, canon.sidecar_bytes(result.rows))
    return "wrote"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--base-dir", required=True, type=Path)
    ap.add_argument("--target-dir", required=True, type=Path)
    ap.add_argument("--out-dir", required=True, type=Path)
    ap.add_argument("--stamp", required=True, type=Path)
    ap.add_argument("--unit", action="append", default=[], dest="units")
    args = ap.parse_args(argv)

    base_out = args.out_dir / "base"
    target_out = args.out_dir / "target"
    wrote = skipped = base_n = target_n = 0
    processed = []
    for unit in sorted(args.units):
        base_src = args.base_dir / f"{unit}.obj"
        if base_src.exists():
            state = _normalize_one(
                base_src, base_out / f"{unit}.obj", base_out / f"{unit}.symbols.tsv")
            wrote += state == "wrote"
            skipped += state == "skip"
            base_n += 1
            processed.append(f"base/{unit}")
        target_src = args.target_dir / f"{unit}.c.obj"
        target_obj = target_out / f"{unit}.c.obj"
        target_sidecar = target_out / f"{unit}.symbols.tsv"
        if target_src.exists():
            state = _normalize_one(target_src, target_obj, target_sidecar)
            wrote += state == "wrote"
            skipped += state == "skip"
            target_n += 1
            processed.append(f"target/{unit}")
        else:
            # A unit that lost its delinked target (e.g. all names removed) must not
            # leave a stale normalized copy behind for objdiff to pair against.
            for stale in (target_obj, target_sidecar):
                if stale.exists():
                    stale.unlink()

    digest = hashlib.sha256("\n".join(processed).encode("utf-8")).hexdigest()
    canon._atomic_write(
        args.stamp,
        ("# normalized objdiff comparison copies\n"
         f"base_objects\t{base_n}\n"
         f"target_objects\t{target_n}\n"
         f"wrote\t{wrote}\n"
         f"skipped\t{skipped}\n"
         f"set_sha256\t{digest}\n").encode("utf-8"))
    print(f"[normalize] base={base_n} target={target_n} wrote={wrote} skipped={skipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
