"""view_typedef - no `typedef RealClass ViewName;` aliases.

A class-to-class alias typedef is a relic of a folded view: a second name for a
class that was once modeled as its own type and later proven to BE the real class.
The alias lets stale code keep the old name; it should be deleted and the real
class used directly (single real type, everywhere). Reached 0 at introduction
(2026-07-22); FATAL so it can never regress.

Standard SDK / CRT / integer aliases (GUID, RECT, i32, uLong, ...) are NOT views
and are whitelisted. Template-instantiation and `typedef struct TAG NAME;` forms
are two-token-mismatched and never matched here.

    python -m gruntz.audit.view_typedef            # report
    python -m gruntz.audit.view_typedef --ratchet  # FATAL if any alias exists
"""
import argparse
import glob
import os
import re

TD = re.compile(r'^[ \t]*typedef[ \t]+([A-Za-z_]\w*)[ \t]+([A-Za-z_]\w*)[ \t]*;', re.M)
# aliases (RHS) that are standard names, not view relics
KEEP_RHS = {"GUID", "RECT", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
            "uLong", "uLongf", "Bytef", "gz_size_t", "size_t", "BOOL", "DWORD",
            "WORD", "BYTE", "UINT", "HANDLE"}
BUILTIN_LHS = {"int", "char", "short", "long", "unsigned", "signed", "float",
               "double", "void", "BOOL", "DWORD", "WORD", "BYTE", "UINT"}


def repo_root():
    p = os.path.abspath(__file__)
    for _ in range(4):
        p = os.path.dirname(p)
    return p


def relics(root):
    out = []
    for h in sorted(glob.glob(os.path.join(root, "include", "**", "*.h"), recursive=True)):
        rel = os.path.relpath(h, os.path.join(root, "include"))
        for m in TD.finditer(open(h, errors="replace").read()):
            real, alias = m.group(1), m.group(2)
            if real in BUILTIN_LHS or alias in KEEP_RHS:
                continue
            out.append((rel, real, alias))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ratchet", action="store_true")
    ap.add_argument("--root", default=None)
    args = ap.parse_args()
    root = args.root or repo_root()
    found = relics(root)

    if args.ratchet:
        if found:
            print(f"[view-typedef] FATAL: {len(found)} view-alias typedef(s) - a class "
                  f"aliased under a second name:")
            for rel, real, alias in found:
                print(f"   typedef {real} {alias};   ({rel})")
            print("   Delete the typedef and use the real class name; if it is a genuine "
                  "SDK/int alias, whitelist its RHS in view_typedef.py.")
            raise SystemExit(1)
        print("[view-typedef] OK - no view-alias typedefs")
        return

    print(f"== view-alias typedefs: {len(found)} ==")
    for rel, real, alias in found:
        print(f"   typedef {real} {alias};   ({rel})")


if __name__ == "__main__":
    main()
