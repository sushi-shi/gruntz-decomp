#!/usr/bin/env python3
"""permute.py - source-permutation hill-climber for the MSVC 5.0 /O2 codegen wall.

On /O2 TUs the match plateaus on block ordering / register coloring / instruction
scheduling that C++ source cannot directly pin. This applies SEMANTICS-PRESERVING
mutations to the real .cpp, recompiles each variant with the REAL MSVC 5.0 under wine
(the compiler handles the C++), scores the resulting COFF against the delinked target
with objdiff-cli, and keeps improvements.

WARNING: the operand-swap and additive-reassociation mutations are DISABLED - the
regex form was operator-precedence-BLIND (it swapped `a + b` inside `a + b*c`, crossing
the tighter `*` and computing a different value that still scored higher = banking WRONG
code). Only the two provably-safe mutations run today: independent-line reorder and
scalar decl-split. The swaps are being restored on a real clang AST (which knows each
operator's true operand sub-expressions). Unlike the classic pycparser-based
decomp-permuter, it never parses the language, so C++ (class /
reinterpret_cast / templates) is fine.

Ported from the HoMM2 sibling decomp (same pipeline: units.toml + cc_wrap + objdiff-cli).
Gruntz specifics: base profile is /O2, so expect INCREMENTAL gains (the optimizer
resists source steering; the core register-coloring residue is out of reach) rather
than guaranteed byte-identical completions - but block-order-shaped residuals (the
kind hand-fixed by for-loop/goto/while spellings) are exactly what this automates.
It will NOT fix a control-flow-shape mismatch or a wrong type/layout.

Usage (inside `nix develop .#build`, run from the repo/worktree root - paths are
resolved against the CWD's flake.nix, so cd into the tree FIRST; you must have run
`gruntz build` at least once so build/objdiff/target/<TU>.c.obj exists):

    python3 -m gruntz.match.permute <src.cpp> <TU> <mangled-sym> [iters]
  e.g.
    python3 -m gruntz.match.permute src/Gruntz/GameLevel.cpp gamelevel \\
        '?ProbeFootBlocked@CGameLevel@@AAEHHHH@Z' 400

Get <TU> from config/units.toml (the `unit` stem) and <mangled-sym> from the objdiff
row (`gruntz sema status`, or `objdiff-cli` output). Edits <src.cpp> IN PLACE,
leaving the best-scoring variant (prints start/FINAL scores); inspect with `git diff`
and rebuild/commit yourself. Interrupt-safe: the best-known variant (never worse than
the original) is written back on any exit.
"""
import subprocess, re, sys, json, os, random, tomllib, pathlib

_CWD = pathlib.Path.cwd()
# Anchor on the CWD's repo root (flake.nix), NOT the package location: run from a
# worktree, we must edit/score THAT tree, never main (PYTHONPATH can point at main).
ROOT = next((str(p) for p in [_CWD, *_CWD.parents] if (p / "flake.nix").exists()),
            os.environ.get("REPO") or str(_CWD))


def _usage():
    sys.exit("usage: python3 -m gruntz.match.permute <src.cpp> <TU> <mangled-sym> [iters]")


if len(sys.argv) < 4:
    _usage()
SRC   = sys.argv[1]                # e.g. src/Gruntz/GameLevel.cpp
TU    = sys.argv[2]                # e.g. gamelevel  (units.toml `unit` stem)
SYM   = sys.argv[3]                # mangled symbol
ITERS = int(sys.argv[4]) if len(sys.argv) > 4 else 400
OBJ   = f"build/objdiff/base/{TU}.obj"      # candidate (freshly recompiled)
TGT   = f"build/objdiff/target/{TU}.c.obj"  # delinked retail target
os.chdir(ROOT)


def _load_flags(tu):
    """Compile flags for a TU = its config/units.toml [flags] profile (+ any `extra`)."""
    with open("config/units.toml", "rb") as fh:
        cfg = tomllib.load(fh)
    profiles = cfg.get("flags", {})
    units = {u["unit"]: u for u in cfg.get("unit", []) if "unit" in u}
    default = profiles.get("base", ["/nologo", "/c", "/O2", "/MT"])
    u = units.get(tu)
    if u is None:
        return list(default)
    flags = list(profiles.get(u.get("flags", "base"), default))
    flags += list(u.get("extra", []))       # the per-TU /GR-style escape hatch
    return flags


FLAGS = _load_flags(TU)


def _sym_rva(sym):
    """RVA (int) of a mangled symbol from build/gen/symbol_names.csv, or None.
    Format: `rva,mangled,unit,size,kind`; MSVC mangled names carry no comma."""
    try:
        for line in open("build/gen/symbol_names.csv"):
            parts = line.rstrip("\n").split(",")
            if len(parts) >= 2 and parts[1] == sym:
                return int(parts[0], 16)
    except OSError:
        pass
    return None


def _target_span(text):
    """[start,end) char span of SYM's function body in `text` - from its RVA() marker to
    the NEXT RVA()/DATA() marker (source order). Mutating only this span keeps the search
    fast (no wasted whole-TU compiles on sibling edits) and CANNOT regress a sibling (a
    stacked phase-2 mutation could otherwise silently break a 100% neighbour, since score()
    only reads the target %). Falls back to the whole file if the marker isn't found."""
    rva = _sym_rva(SYM)
    if rva is None:
        return 0, len(text)
    i = text.find(f"RVA(0x{rva:08x}")       # markers are 8-hex zero-padded, at column 0
    if i < 0:
        return 0, len(text)
    nxt = [c for c in (text.find("\nRVA(0x", i + 8), text.find("\nDATA(0x", i + 8)) if c >= 0]
    return i, (min(nxt) if nxt else len(text))


def score(text):
    """Recompile <SRC>=text and return the objdiff match_percent for SYM (-1 on failure)."""
    with open(SRC, "w") as fh:
        fh.write(text)
    # cc_wrap adds the /I include set itself; we pass only the profile flags after --.
    r = subprocess.run(["python3", "-m", "gruntz.build.cc_wrap", "--out", OBJ, "--src", SRC, "--", *FLAGS],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return -1.0
    r = subprocess.run(["objdiff-cli", "diff", "-1", TGT, "-2", OBJ, SYM, "-o", "-", "--format", "json"],
                       capture_output=True, text=True)
    try:
        d = json.loads(r.stdout)
    except Exception:
        return -1.0
    # CRITICAL: objdiff-cli IGNORES the <symbol> arg for JSON output - it emits every
    # symbol in the TU as {left:{symbols:[...]}, right:{symbols:[...]}}. Taking a global
    # max match_percent (the naive walk) saturates at 100 the moment ANY sibling is 100%
    # (and every real Gruntz TU has 100%-matched siblings) - the search would go blind.
    # Pull OUR symbol's match_percent by exact mangled name (present on both sides,
    # identical value); -1.0 if the compile dropped it / the name is wrong.
    for side in ("left", "right"):
        for sym in (d.get(side) or {}).get("symbols", []):
            if sym.get("name") == SYM:
                return sym.get("match_percent", -1.0)
    return -1.0


OPD = r'(?:0x[0-9a-fA-F]+|\d+|[A-Za-z_]\w*(?:\[[A-Za-z0-9_ +*-]+\]|->\w+|\.\w+)*)'
# commutative ops: swapping operands preserves the computed value
COMM = ['+', '*', '==', '!=', '&', '|', '^']
ASSIGN = re.compile(r'^(\s*)([A-Za-z_]\w*) = (.+);\s*$')
# decl split: scalar types (incl. the gruntz i32/u32/... aliases) or any single-word
# pointer type - `IconEntry *` in the HoMM2 original was one domain hardcode; generalized.
_SCALAR = r'(?:unsigned |signed |const )*(?:int|short|char|long|float|double|bool|' \
          r'i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|usize|isize)\**'
DECL = re.compile(r'^(\s*)(' + _SCALAR + r'|[A-Za-z_]\w* ?\*) (\w+) = (.+);\s*$')


def idents(s):
    return set(re.findall(r'[A-Za-z_]\w*', s))


def gen_variants(full):
    """Value-preserving variants of `full`, mutating ONLY SYM's function span (spliced back
    into the surrounding file unchanged) - see _target_span for why scoping matters."""
    start, end = _target_span(full)
    prefix, span, suffix = full[:start], full[start:end], full[end:]
    return [prefix + v + suffix for v in _mutate(span)]


def _mutate(text):
    out = []
    # 1) commutative-operand swaps + 3) additive reassociation: DISABLED - they were
    #    operator-precedence-BLIND. OPD matches ONE primary, so in `a + b*c` the regex
    #    swapped `a + b`, crossing the tighter-binding `*` and computing a DIFFERENT value
    #    (e.g. `cells + width*y + x` -> `cells + y*x + width`) that still scored higher -
    #    banking WRONG code. A correct swap needs the real operand sub-expression extents,
    #    which only a parser gives; restored in the clang-AST rewrite. Until then only the
    #    two provably-safe, precedence-independent mutations below run.
    # 2) reorder adjacent independent scalar/global assignment lines (value-preserving)
    lines = text.split("\n")
    for i in range(len(lines) - 1):
        m1, m2 = ASSIGN.match(lines[i]), ASSIGN.match(lines[i + 1])
        if not (m1 and m2):
            continue
        # skip lines with side effects / memory writes / calls / increments
        if any(t in lines[i] + lines[i + 1] for t in ("*", "++", "--", "(", "[")):
            continue
        l1, r1 = m1.group(2), m1.group(3)
        l2, r2 = m2.group(2), m2.group(3)
        if l1 == l2:            # write-after-write to same lhs: order matters
            continue
        # independent iff neither lhs is used by the other statement
        if l1 in idents(r2) or l2 in idents(r1):
            continue
        nl = lines[:]
        nl[i], nl[i + 1] = lines[i + 1], lines[i]
        out.append("\n".join(nl))
    # (3) additive reassociation: DISABLED with (1) above - same precedence-blindness.
    # 4) declaration split: `TYPE V = E;` -> `TYPE V;` + `V = E;` (moves materialisation point)
    for i, ln in enumerate(lines):
        m = DECL.match(ln)
        if not m or any(t in ln for t in ("*=", "new ", "[", "(")):
            continue
        ind, ty, v, e = m.groups()
        nl = lines[:i] + [f"{ind}{ty} {v};", f"{ind}{v} = {e};"] + lines[i + 1:]
        out.append("\n".join(nl))
    return out


def main():
    best = open(SRC).read()
    orig = best
    if not os.path.exists(TGT):
        sys.exit(f"target obj missing: {TGT} - run `gruntz build` first (the delink step).")
    try:
        s0, s1 = _target_span(best)
        if (s0, s1) == (0, len(best)):
            print(f"WARNING: could not locate {SYM}'s RVA span - mutating the WHOLE file "
                  f"(slow, and a stacked mutation can regress a sibling)", flush=True)
        else:
            nl = best.count("\n", 0, s0) + 1, best.count("\n", 0, s1) + 1
            print(f"scoped to {SYM}: lines {nl[0]}..{nl[1]}", flush=True)
        bscore = score(best)
        print(f"start {bscore:.3f}", flush=True)
        if bscore < 0:
            sys.exit(f"baseline did not compile/score - fix {SRC} / check the symbol {SYM}.")
        improved = True
        seen = set()
        rounds = 0
        while improved and rounds < 12:
            improved = False
            rounds += 1
            variants = gen_variants(best)
            random.shuffle(variants)
            for i, v in enumerate(variants):
                if v in seen:
                    continue
                seen.add(v)
                s = score(v)
                if s > bscore + 1e-6:
                    print(f"  round {rounds} site {i}: {bscore:.3f} -> {s:.3f}", flush=True)
                    best, bscore = v, s
                    improved = True
                    break
        # phase 2: random multi-mutation walk to escape single-step local optima
        for attempt in range(ITERS):
            cur = best
            for _ in range(random.randint(2, 6)):
                vs = gen_variants(cur)
                if not vs:
                    break
                cur = random.choice(vs)
            if cur in seen:
                continue
            seen.add(cur)
            s = score(cur)
            if s > bscore + 1e-6:
                print(f"  walk {attempt}: {bscore:.3f} -> {s:.3f}", flush=True)
                best, bscore = cur, s
                # greedy re-climb from the new best
                again = True
                while again:
                    again = False
                    for v in gen_variants(best):
                        if v in seen:
                            continue
                        seen.add(v)
                        sv = score(v)
                        if sv > bscore + 1e-6:
                            best, bscore = v, sv
                            print(f"    reclimb: -> {bscore:.3f}", flush=True)
                            again = True
                            break
        print(f"FINAL {bscore:.3f} ({'improved' if best != orig else 'no change'})", flush=True)
    finally:
        # always leave the best-known variant on disk (never worse than the original),
        # so an interrupt mid-walk cannot strand SRC on a random trial candidate.
        with open(SRC, "w") as fh:
            fh.write(best)


if __name__ == "__main__":
    main()
