"""gruntz.sema.match - `gruntz sema match`: per-function/unit match summary
read from report.json. Takes a unit name (whole-unit table) or an 0x RVA a src
fn claims (that one function's row).

Engine: gruntz.match.status (also runnable as `python -m gruntz.match.status`).
"""
import sys

from gruntz.sema._common import GEN_NAMES, call_main, units


def run(args) -> None:
    t = args.target
    if t in {u["unit"] for u in units()}:
        sys.exit(call_main("gruntz.match.status", ["status", "--unit", t]))
    grep = t
    if t.lower().startswith("0x") and GEN_NAMES.exists():
        import csv
        want = int(t, 16)
        grep = None
        for r in csv.reader(GEN_NAMES.open()):
            if r and r[0].lower().startswith("0x"):
                try:
                    if int(r[0], 16) == want:
                        grep = r[1]
                        break
                except ValueError:
                    pass
        if grep is None:
            # answered-NO, not an error (rc convention: 1 = valid query, no hit)
            print(f"no src function claims RVA {t} (nothing to score) - try a unit name",
                  file=sys.stderr)
            sys.exit(1)
    sys.exit(call_main("gruntz.match.status", ["status", "--grep", grep]))


if __name__ == "__main__":
    # These modules are `gruntz sema <cmd>` implementations: the CLI owns argparse
    # and calls run(args), so `python -m` would import-and-exit silently (rc 0, no
    # output) and read as "identical"/"no findings". Fail loudly instead.
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...` (or `gruntz sema -` for a batch on stdin)."
             % (__name__, __name__.rsplit(".", 1)[-1]))
