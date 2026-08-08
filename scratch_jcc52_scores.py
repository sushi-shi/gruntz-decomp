import json, sys, os

REPO = os.environ.get("JCC52_REPO", "/home/sheep/Projects/gruntz/.claude/worktrees/matcher-52")
r = json.load(open(REPO + "/build/objdiff/report.json"))
units = {u["name"]: u for u in r["units"]}
want = sys.argv[1:] or [
    "tiletriggercontainer",
    "tileswitchlogic",
    "triggermgr",
    "ddsurface",
    "fontconfig",
    "symtab",
    "shadetablecache",
    "menustate",
]
for un in want:
    u = units.get(un)
    if not u:
        print("MISSING", un)
        continue
    print("====", un, round(u["measures"]["fuzzy_match_percent"], 3))
    for f in u["functions"]:
        p = f.get("fuzzy_match_percent", 0.0)
        print("   %7.3f  %s" % (p, f.get("name")))
