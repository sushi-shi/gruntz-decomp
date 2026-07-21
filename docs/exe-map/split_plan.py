#!/usr/bin/env python3
"""Actionable split/move worklist from the misplacement analysis -> SPLIT_PLAN.md.

Classifies each flagged unit (destructors removed, src/Stub/ excluded):
  SPLIT-X  cross-module: >=2 substantial blocks (>=4 fns) in DIFFERENT .text regions
           (e.g. game + engine) -> a genuinely multi-TU class; split the file to match.
  SPLIT-S  same-module: >=2 substantial blocks in ONE region -> two TUs OR scatter; review.
  MOVE     a lone (<=2-fn) block far from the file's main block, with a strong dominant
           home in another file -> move the function(s) there.
Benign COMDAT-pooled members (Serialize, boundary Forward-thunks) are excluded from MOVEs.
Run under `nix develop` (needs exe_map). Reuses flag_outliers' region/cluster helpers."""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import flag_outliers as fo   # named_regions, is_dtor, clusters, GAP, WIN
import gruntz.core.exe_map as em

HERE = os.path.dirname(os.path.abspath(__file__))
BIG = 4          # a "substantial" block (real TU chunk, not stray)
FAR = 0x8000     # a MOVE outlier must be this far from its main block
HOME_MIN = 3     # ... and land amid >=3 methods of one other file
CODE_REGIONS = {"game", "engine"}   # module regions (vs library/EH/thunks)


def benign(name):
    # special members (ctor ??0, deleting-dtor/vtable ??_*), Serialize, and boundary
    # Forward-thunks are COMDAT-pooled, so "far from home" is expected, not a misplace.
    # Regular methods don't pool (proven), so those are the trustworthy MOVE candidates.
    return ("Serialize" in name or "Forward" in name or name.startswith(("??0", "??_")))


VIRT = set("EFMNUV")   # MSVC access chars marking a VIRTUAL member fn (E/F priv, M/N prot, U/V pub)


def classof(name):
    if name.startswith(("??0", "??1")):
        return name[3:].split("@@")[0].split("@")[0]
    if name.startswith(("??_G", "??_E")):
        return name[4:].split("@@")[0].split("@")[0]
    if name.startswith("?"):
        p = name[1:].split("@")
        return p[1] if len(p) > 1 else ""
    return ""


def is_virtual(name):
    i = name.find("@@")               # the char right after the qualified name is access+virtual
    return i >= 0 and i + 2 < len(name) and name[i + 2] in VIRT


def placeholder_cls(c):
    return not c or bool(re.search(r"[0-9a-f]{4,}", c))   # discovered/placeholder class name


def main():
    import bisect
    funcs, meta = em.load()
    funcs = [r for r in funcs if not r["source"].startswith("src/Stub/")]
    meth = [r for r in funcs if not fo.is_dtor(r["name"])]
    owned = [r for r in meth if r["category"] == "unit" and r["source"]]
    gstarts = sorted(owned, key=lambda r: r["rva"])
    grva = [r["rva"] for r in gstarts]
    regions = fo.named_regions(funcs, meta["text_lo"], meta["text_hi"])

    def region_of(rva):
        for lo, hi, fam in regions:
            if lo <= rva < hi:
                return fam
        return "unknown"

    def territory(lo, hi, exclude):
        i, j = bisect.bisect_left(grva, lo - fo.WIN), bisect.bisect_right(grva, hi + fo.WIN)
        t = {}
        for r in gstarts[i:j]:
            if r["source"] != exclude:
                t[r["source"]] = t.get(r["source"], 0) + 1
        return sorted(t.items(), key=lambda kv: -kv[1])

    # class -> methods, and class -> {src files it lives in}. A real class that is
    # entirely in ONE file is correctly grouped: a method of it scattered in RVA is a
    # header-inline (deduped COMDAT copy), NOT a misplacement. Only a class spread over
    # >1 file, or an unknown/placeholder/free function, is a genuine MOVE candidate.
    by_class, class_files = {}, {}
    for r in owned:
        c = classof(r["name"])
        if c and not placeholder_cls(c):
            by_class.setdefault(c, []).append(r)
            class_files.setdefault(c, set()).add(r["source"])

    by_file = {}
    for r in owned:
        by_file.setdefault(r["source"], []).append(r)

    splits_x, splits_s, moves = [], [], []
    for src, recs in by_file.items():
        cls = fo.clusters(recs)
        big = [c for c in cls if len(c) >= BIG]
        if len(big) >= 2:
            blocks = []
            for c in sorted(big, key=lambda c: c[0]["rva"]):
                lo = c[0]["rva"]
                blocks.append({"region": region_of(lo), "lo": lo, "n": len(c),
                               "fns": [x["name"] for x in c]})
            regs = {b["region"] for b in blocks if b["region"] in CODE_REGIONS}
            entry = {"file": src, "blocks": blocks, "nblk": len(big)}
            (splits_x if len(regs) >= 2 else splits_s).append(entry)
        # lone outliers -> moves
        main_c = max(cls, key=lambda c: (len(c), sum(x["size"] for x in c)))
        for c in cls:
            if c is main_c or len(c) > 2:
                continue
            clo, chi = c[0]["rva"], c[-1]["rva"] + c[-1]["size"]
            mlo, mhi = main_c[0]["rva"], main_c[-1]["rva"] + main_c[-1]["size"]
            dist = clo - mhi if clo > mhi else mlo - chi
            if dist < FAR:
                continue
            terr = territory(clo, chi, src)
            if not terr or terr[0][1] < HOME_MIN:
                continue
            for x in c:
                if benign(x["name"]) or is_virtual(x["name"]):   # virtual = header-inline
                    continue
                cn = classof(x["name"])
                # a scattered method of a class that is otherwise all in ONE file is a
                # header-inline (handled below), NOT a move. What's left is a function whose
                # class we can't pin: an unknown/placeholder-class or free function in a
                # reconstruction bucket -> the RVA neighbour is a re-home HINT, low confidence.
                if cn and not placeholder_cls(cn) and len(class_files.get(cn, ())) <= 1:
                    continue
                moves.append({"file": src, "name": x["name"], "rva": x["rva"],
                              "dist": dist, "home": terr[0][0], "home_n": terr[0][1]})

    # ---- HEADER-INLINE: any method scattered from its own CLASS body, when the class
    # is otherwise all in ONE file (correctly grouped). The scattered RVA is the linker's
    # deduped copy of an inline/template/static member defined in a header -> reconstruct
    # it INLINE in the class header, not moved to any .cpp. (No size gate: WrapCoord 172B,
    # _zvec::GrowTo 267B and the WwdFile statics are inline too, just larger.) ----
    inline = []
    for c, ms in by_class.items():
        if len(ms) < 2 or len(class_files.get(c, ())) != 1:   # multi-file class -> SPLIT/MOVE, not here
            continue
        cl = fo.clusters(ms)
        if sum(1 for x in cl if len(x) >= BIG) >= 2:   # a genuine two-TU class -> SPLIT, not inline
            continue
        mc = max(cl, key=lambda x: (len(x), sum(y["size"] for y in x)))
        mlo, mhi = mc[0]["rva"], mc[-1]["rva"] + mc[-1]["size"]
        for m in ms:
            if m in mc:
                continue
            d = m["rva"] - mhi if m["rva"] > mhi else mlo - m["rva"]
            if d < fo.GAP:
                continue
            inline.append({"cls": c, "name": m["name"], "size": m["size"], "dist": d,
                           "virtual": is_virtual(m["name"]),
                           "file": m["source"].rsplit("/", 1)[-1]})
    inline.sort(key=lambda x: (not x["virtual"], -x["dist"]))
    from collections import Counter
    inline_by_cls = Counter(x["cls"] for x in inline)

    splits_x.sort(key=lambda e: -sum(b["n"] for b in e["blocks"]))
    splits_s.sort(key=lambda e: -sum(b["n"] for b in e["blocks"]))
    moves.sort(key=lambda m: -m["dist"])

    def base(p):
        return p.rsplit("/", 1)[-1]

    def stem(p):
        return base(p)[:-4] if base(p).endswith(".cpp") else base(p)

    L = ["# Split / move worklist — from the exe-map misplacement analysis",
         "",
         "Generated by `split_plan.py` (destructors removed, `src/Stub/` excluded). Four fix",
         "categories, most-confident first:",
         "",
         "- **A. SPLIT cross-module** — one class compiled in two module TUs (`CNetMgr`); split the file.",
         "- **B. SPLIT same-module** — two+ TU blocks in one region; review, then split.",
         "- **C. RE-HOME HINTS** — unknown-class / bucket / namespace functions where the RVA",
         "  neighbour is a low-confidence home hint (named classes are already filed correctly).",
         "- **D. HEADER-INLINE** — a method scattered as a deduped COMDAT copy of an inline/",
         "  template/static header member; it belongs *inline in the header*, NOT moved to a `.cpp`.",
         "",
         "Regenerate: `python docs/exe-map/split_plan.py`.", ""]

    L += [f"## A. SPLIT — cross-module ({len(splits_x)}) · highest confidence",
          "",
          "Blocks sit in **different `.text` regions** (game vs engine) — a single class whose",
          "methods were compiled in two modules (the `CNetMgr` pattern). Split the file so each",
          "block becomes its own unit at its true region.", ""]
    for e in splits_x:
        f = stem(e["file"])
        L.append(f"### `{e['file']}` → {len(e['blocks'])} units")
        for b in e["blocks"]:
            suffix = {"game": "Game", "engine": "Engine"}.get(b["region"], b["region"].title())
            samp = ", ".join(n[:40] for n in b["fns"][:4]) + (" …" if b["n"] > 4 else "")
            L.append(f"- **{b['region']}** block @ `0x{b['lo']:06x}` ({b['n']} fns) "
                     f"→ `{f}{suffix}.cpp`  ·  {samp}")
        L.append("")

    L += [f"## B. SPLIT — same-module ({len(splits_s)}) · review",
          "",
          "Two+ substantial blocks in the *same* region — likely two TUs of one module, but could",
          "be COMDAT scatter or a mis-group. Confirm before splitting.", ""]
    for e in splits_s[:15]:
        blk = "  ".join(f"{b['n']}@0x{b['lo']:06x}({b['region']})" for b in e["blocks"])
        L.append(f"- `{e['file']}` — {e['nblk']} blocks: {blk}")
    L.append("")

    L += [f"## C. RE-HOME HINTS — unknown-class / bucket functions ({len(moves)}) · low confidence",
          "",
          "What's left after A/B/D. **Named classes are (verified) filed correctly** — e.g. all 12",
          "`CPlaneRender` methods are in `WwdFile.cpp`; the scattered ones were just header-inlines",
          "(category D). So these rows are functions whose *class we can't pin*: placeholder-named",
          "bucket functions (`CObj_bdd0`, `Obj38120`, `C213a0`) and namespace utilities",
          "(`WinAPI::Utils::…`). The neighbour is a **re-home hint** to apply as naming/coverage",
          "improves — **not** a confident semantic move. Do not apply blindly.", "",
          "| function | currently in | → hint | distance |",
          "|---|---|---|---|"]
    for m in moves[:30]:
        L.append(f"| `{m['name'][:48]}` | {base(m['file'])} | **{base(m['home'])}** "
                 f"({m['home_n']} fns) | {em._fmt_size(m['dist'])} |")
    if len(moves) > 30:
        L.append(f"\n_(+{len(moves)-30} more — see the generator output.)_")
    L += ["",
          f"## D. HEADER-INLINE — reconstruct in the header ({len(inline)}) · not a move",
          "",
          "Small or virtual member functions sitting **scattered from their own class body**.",
          "They were defined **inline in a header**: MSVC still emits one out-of-line COMDAT copy",
          "(a virtual needs a vtable address; a small one the inliner sometimes declines at a call",
          "site) and the linker dedups it to a single spot — so the scattered RVA is a **dedup",
          "artifact, not a home**. Fix = define them as inline members in the class's **header**,",
          "not in any `.cpp`; the compiler reproduces the same COMDAT + dedup for free.", "",
          "Top classes by inline-scattered method count:", ""]
    for c, n in inline_by_cls.most_common(20):
        L.append(f"- `{c}` — {n}")
    L += ["", "Examples (virtual first):", "",
          "| function | class | size | virtual |", "|---|---|---:|:--:|"]
    for x in inline[:25]:
        L.append(f"| `{x['name'][:44]}` | {x['cls']} | {x['size']} B | {'✓' if x['virtual'] else ''} |")
    L += ["", "## How to execute a split (NetMgr worked example)", "",
          "1. **Keep the class declaration in one shared header** (`NetMgr.h`) — it is NOT split",
          "   (ODR: every TU sees one identical class). Only the *method definitions* move.",
          "2. **Create the new `.cpp`** (e.g. `src/Net/NetMgrEngine.cpp`) and move the engine-block",
          "   `CNetMgr` method bodies into it; the game bodies stay in `NetMgr.cpp` (or a",
          "   `NetMgrGame.cpp`).",
          "3. **Register the new unit** in `config/units.toml` (`[[unit]]` + `source` + `flags`) and",
          "   move that block's `RVA()` rows into the new file.",
          "4. **Rebuild + verify.** Each unit now packs into its own contiguous target `.text` block",
          "   at its true region, so per-unit match% should *rise* — a unit whose RVA()s span two",
          "   distant blocks can't pack (see the delinker-packing note). The split is matching-",
          "   neutral-to-positive; confirm per-unit % before/after.", ""]

    open(os.path.join(HERE, "SPLIT_PLAN.md"), "w").write("\n".join(L))
    print(f"SPLIT_PLAN.md: {len(splits_x)} cross-module splits, {len(splits_s)} same-module "
          f"splits, {len(moves)} moves, {len(inline)} header-inline")
    print("\nTop cross-module splits:")
    for e in splits_x[:8]:
        blk = " + ".join(f"{b['n']}@{b['region']}" for b in e["blocks"])
        print(f"  {stem(e['file']):<22} {blk}")
    print("\nTop moves:")
    for m in moves[:8]:
        print(f"  {m['name'][:42]:<42} {base(m['file']):<18} -> {base(m['home'])}")


if __name__ == "__main__":
    main()
