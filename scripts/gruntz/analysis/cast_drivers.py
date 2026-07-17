"""cast_drivers - rank the offset-cast DRIVERS, not the files.

    python -m gruntz.analysis.cast_drivers

`(char*)x + N` is banned: the fix is the named member `&x->m_field`. But the fix is
per-DRIVER, and casts-per-file is the wrong ranking - it scatters one driver across
many files and hides it. The scroll-block driver was found this way instead: 14 casts
off ONE base (`&g_scrollAccum`) turned out to be 8 SEPARATELY-REFERENCED globals, six
of which already had names and DATA() homes. Naming them was byte-neutral and deleted
10 casts from one header edit.

So the signal is: a base with MANY DISTINCT constant offsets. That shape means "this is
one object being read at its fields", and the fix is to name those fields once.

Counts on the same comment/literal-stripped text `cleanliness` counts, so the totals
reconcile with the metric exactly.

READ THE OUTPUT WITH CARE - two traps, both hit for real:
  * a TEXTUAL base is not a driver. `this` tops the list at 36 sites / 21 offsets, but
    it is 21 DIFFERENT classes in 21 files - not one driver. Likewise a local named
    `rec`/`p`/`cur` is a different type per file. Only a base that names ONE object
    (a global like `g_gameReg`, or a member chain like `g_gameReg->m_world`) is a real
    driver; for the rest, scope USR-exact before believing the row.
  * many offsets already RESOLVE to a declared member. Chase the chain before modelling
    anything: `g_gameReg->m_world + 0x24` is CDDrawSurfaceMgr::m_level, already declared;
    `+0x5c` off that is CGameLevel::m_mainPlane, also already declared. The cast survives
    only because the LAST hop (CLevelPlane) is forward-declared and never defined.
"""
import collections
import pathlib
import re

from gruntz.match import cleanliness as C

CAST = re.compile(r"\((?:const )?char ?\*\)")
OPERAND = re.compile(r"\s*&?\s*(?:\*+\s*)?[\w:]+(?:\s*(?:->|\.|::)\s*[\w:]+|\s*\[[^\]]*\]|\s*\((?:[^()]|\([^()]*\))*\))*")
CONST = re.compile(r"^([+-])\s*(0x[0-9a-fA-F]+|\d+)\b(?!\s*[*/])")

rows = []  # (file, base, sign, value, line)
for base_dir in C.ROOTS:
    for p in sorted(pathlib.Path(base_dir).rglob("*")):
        if p.suffix not in C.EXTS or not p.is_file():
            continue
        t = C._strip(p.read_text(errors="replace"))
        for m in CAST.finditer(t):
            ls = t.rfind("\n", 0, m.start()) + 1
            le = t.find("\n", m.end())
            le = len(t) if le < 0 else le
            after = t[m.end():le]
            om = OPERAND.match(after)
            operand = om.group(0).strip() if om else ""
            rest = (after[len(om.group(0)):] if om else after).lstrip()
            cm = CONST.match(rest)
            if not cm:
                continue
            val = int(cm.group(2), 16) if cm.group(2).startswith("0x") else int(cm.group(2))
            if cm.group(1) == "-":
                val = -val
            rows.append((str(p), operand, val, t[ls:le].strip()))

print(f"A1 sites (constant offset-casts): {len(rows)}\n")

# group by (file-scope) base expression: the driver is a base with MANY distinct offsets
by_base = collections.defaultdict(list)
for f, b, v, line in rows:
    by_base[b].append((f, v, line))

print("=== bases ranked by DISTINCT constant offsets (the concentrated-driver signal) ===")
print(f"{'base expression':<26} {'sites':>5} {'distinct':>8} {'files':>5}  offsets")
print("-" * 96)
ranked = sorted(by_base.items(),
                key=lambda kv: (-len({v for _, v, _ in kv[1]}), -len(kv[1])))
for b, hits in ranked[:22]:
    offs = sorted({v for _, v, _ in hits})
    files = {f for f, _, _ in hits}
    show = " ".join(("-" if o < 0 else "") + hex(abs(o)) for o in offs[:11])
    if len(offs) > 11:
        show += f" ... (+{len(offs)-11})"
    print(f"{b[:26]:<26} {len(hits):>5} {len(offs):>8} {len(files):>5}  {show}")
