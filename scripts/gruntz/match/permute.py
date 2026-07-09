#!/usr/bin/env python3
"""permute.py - source-permutation hill-climber for the MSVC 5.0 /O2 codegen wall.

On /O2 TUs the match plateaus on block ordering / register coloring / instruction
scheduling that C++ source cannot directly pin. This applies SEMANTICS-PRESERVING
mutations to the real .cpp, recompiles each variant with the REAL MSVC 5.0 under wine
(the compiler handles the C++), scores the resulting COFF against the delinked target
with objdiff-cli, and keeps improvements.

Mutations are PRECEDENCE-CORRECT because they come from a real clang AST (libclang),
not regex: for a commutative binary operator the two OPERAND SUB-EXPRESSIONS are read
from the AST (so the RHS of `a + b*c` is the whole `b*c`), and swapping them is
value-equivalent (C leaves operand evaluation order unspecified). The earlier regex
form was operator-precedence-blind and banked WRONG code; that is why this is AST-based.
Plus two parse-free safe mutations: scalar decl-split and independent-line reorder.

All source text is handled as BYTES: libclang reports BYTE offsets, and a single
non-ASCII byte (a `§` in a comment) is enough to drift char offsets and splice the
wrong bytes. Bytes keep our offsets identical to clang's.

Usage (inside `nix develop .#build`, from the repo/worktree root; run `gruntz build`
first so build/objdiff/{target,report} + build/clangd/compile_commands.json exist):
    python3 -m gruntz.match.permute <src.cpp> <unit> <mangled-sym> [iters]
  e.g.
    python3 -m gruntz.match.permute src/Gruntz/GameLevel.cpp gamelevel \\
        '?ProbeFootBlocked@CGameLevel@@AAEHPAUCGameObject@@H@Z' 400

Edits <src.cpp> IN PLACE, leaving the best-scoring variant; inspect with `git diff` and
rebuild/commit yourself. Interrupt-safe: the best-known variant (never worse than the
original) is written back on any exit.
"""
import subprocess, re, sys, json, os, random, tomllib, pathlib, shlex
import clang.cindex as cx

_CWD = pathlib.Path.cwd()
# Anchor on the CWD's repo root (flake.nix), NOT the package location: run from a
# worktree, we must edit/score THAT tree, never main (PYTHONPATH can point at main).
ROOT = next((str(p) for p in [_CWD, *_CWD.parents] if (p / "flake.nix").exists()),
            os.environ.get("REPO") or str(_CWD))

if len(sys.argv) < 4:
    sys.exit("usage: python3 -m gruntz.match.permute <src.cpp> <unit> <mangled-sym> [iters]")
SRC   = sys.argv[1]                # e.g. src/Gruntz/GameLevel.cpp
TU    = sys.argv[2]                # e.g. gamelevel  (units.toml `unit` stem)
SYM   = sys.argv[3]                # mangled symbol
ITERS = int(sys.argv[4]) if len(sys.argv) > 4 else 400
OBJ   = f"build/objdiff/base/{TU}.obj"      # candidate (freshly recompiled)
TGT   = f"build/objdiff/target/{TU}.c.obj"  # delinked retail target
os.chdir(ROOT)
_SRC_REAL = os.path.realpath(SRC)


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
    flags += list(u.get("extra", []))
    return flags


FLAGS = _load_flags(TU)


def _clang_args():
    """libclang parse args for SRC = the unit's clang-cl command from
    build/clangd/compile_commands.json, forced into clang-cl mode (--driver-mode=cl) so
    the /imsvc MSVC includes resolve (default clang mode misreads the /-flags)."""
    cc = json.load(open("build/clangd/compile_commands.json"))
    for e in cc:
        if os.path.realpath(e.get("file", "")) == _SRC_REAL:
            args = e.get("arguments") or shlex.split(e["command"])
            base = os.path.basename(SRC)
            return ["--driver-mode=cl"] + [a for a in args[1:] if not a.endswith(base)]
    sys.exit(f"no compile_commands.json entry for {SRC} - run `gruntz build` first.")


CLANG_ARGS = _clang_args()


def _sym_rva(sym):
    """RVA (int) of a mangled symbol from build/gen/symbol_names.csv, or None."""
    try:
        for line in open("build/gen/symbol_names.csv"):
            parts = line.rstrip("\n").split(",")
            if len(parts) >= 2 and parts[1] == sym:
                return int(parts[0], 16)
    except OSError:
        pass
    return None


def _target_span(text):
    """[start,end) BYTE span of SYM's function body in `text` (bytes) - its RVA() marker
    to the NEXT RVA()/DATA() marker. Whole file if the marker isn't found."""
    rva = _sym_rva(SYM)
    if rva is None:
        return 0, len(text)
    i = text.find(("RVA(0x%08x" % rva).encode())      # markers are 8-hex zero-padded, col 0
    if i < 0:
        return 0, len(text)
    nxt = [c for c in (text.find(b"\nRVA(0x", i + 8), text.find(b"\nDATA(0x", i + 8)) if c >= 0]
    return i, (min(nxt) if nxt else len(text))


def score(text):
    """Recompile <SRC>=text (bytes) and return the objdiff match_percent for SYM (-1 on fail)."""
    with open(SRC, "wb") as fh:
        fh.write(text)
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
    # objdiff-cli IGNORES the <symbol> arg for JSON and emits every symbol; a global max
    # saturates at 100 the instant any sibling is 100% - pull OUR symbol's % by name.
    for side in ("left", "right"):
        for s in (d.get(side) or {}).get("symbols", []):
            if s.get("name") == SYM:
                return s.get("match_percent", -1.0)
    return -1.0


# commutative binary operators (bytes): with REAL AST operand extents, swapping the two
# operand sub-expressions is value-equivalent. Excludes && || (short-circuit) and the
# non-commutative ops.
_COMM_OPS = {b"+", b"*", b"==", b"!=", b"&", b"|", b"^"}
# decl-split applies only to POD scalars/pointers (copy-init == default+assign for those):
_SCALAR = r"(?:unsigned |signed |const )*(?:int|short|char|long|float|double|bool|" \
          r"i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|usize|isize)\**"
DECL = re.compile(r"^(\s*)(" + _SCALAR + r"|[A-Za-z_]\w* ?\*) (\w+) = (.+);\s*$")
ASSIGN = re.compile(r"^(\s*)([A-Za-z_]\w*) = (.+);\s*$")


def _ast_swap_edits(text):
    """Precedence-correct commutative-operand swaps in SYM's span, as (start,end,repl_bytes)
    edits. Operand extents come from the clang AST, so swapping is value-preserving by
    construction. `text` and offsets are BYTES (== clang's byte offsets)."""
    s0, s1 = _target_span(text)
    with open(SRC, "wb") as fh:
        fh.write(text)
    tu = cx.Index.create().parse(SRC, args=CLANG_ARGS)
    edits, seen = [], set()
    for n in tu.cursor.walk_preorder():
        if n.kind != cx.CursorKind.BINARY_OPERATOR:
            continue
        f = n.location.file
        if f is None or os.path.realpath(f.name) != _SRC_REAL:   # skip header nodes
            continue
        kids = list(n.get_children())
        if len(kids) != 2:
            continue
        lb, le = kids[0].extent.start.offset, kids[0].extent.end.offset
        rb, re_ = kids[1].extent.start.offset, kids[1].extent.end.offset
        if not (s0 <= lb < le <= rb < re_ <= s1):   # ordered + inside the target span
            continue
        op = text[le:rb].strip()
        if op not in _COMM_OPS or b"\n" in text[lb:re_]:
            continue
        lhs, rhs = text[lb:le], text[rb:re_]
        if lhs == rhs or (lb, re_) in seen:
            continue
        seen.add((lb, re_))
        edits.append((lb, re_, rhs + text[le:rb] + lhs))   # keep the mid operator/spacing
    return edits


def _text_edits(text):
    """The two parse-free, provably-safe mutations as (start,end,repl_bytes) edits within
    SYM's span: scalar decl-split (`T v = e;` -> `T v; v = e;`) and independent-line
    reorder. `text`/offsets are BYTES."""
    s0, s1 = _target_span(text)
    edits = []
    lines = text.split(b"\n")
    starts, off = [], 0
    for ln in lines:
        starts.append(off)
        off += len(ln) + 1
    for i, ln in enumerate(lines):
        st = starts[i]
        if not (s0 <= st < s1):
            continue
        try:
            s = ln.decode("utf-8")
        except UnicodeDecodeError:
            continue
        end = st + len(ln)
        m = DECL.match(s)
        if m and not any(t in s for t in ("*=", "new ", "[", "(")):
            ind, ty, v, e = m.groups()
            edits.append((st, end, f"{ind}{ty} {v};\n{ind}{v} = {e};".encode()))
        if i + 1 < len(lines):
            try:
                s2 = lines[i + 1].decode("utf-8")
            except UnicodeDecodeError:
                continue
            m1, m2 = ASSIGN.match(s), ASSIGN.match(s2)
            if m1 and m2 and not any(t in s + s2 for t in ("*", "++", "--", "(", "[")):
                l1, r1 = m1.group(2), m1.group(3)
                l2, r2 = m2.group(2), m2.group(3)
                ids1 = set(re.findall(r"[A-Za-z_]\w*", r1))
                ids2 = set(re.findall(r"[A-Za-z_]\w*", r2))
                if l1 != l2 and l1 not in ids2 and l2 not in ids1:
                    end2 = starts[i + 1] + len(lines[i + 1])
                    edits.append((st, end2, (s2 + "\n" + s).encode()))
    return edits


def _edits(text):
    return _ast_swap_edits(text) + _text_edits(text)


def _apply(text, edits):
    """Apply non-overlapping (start,end,repl) BYTE edits right-to-left (offsets stay valid)."""
    for st, end, repl in sorted(edits, key=lambda e: -e[0]):
        text = text[:st] + repl + text[end:]
    return text


def _pick_disjoint(edits, k):
    """A random subset of up to k edits with pairwise non-overlapping ranges."""
    pool = edits[:]
    random.shuffle(pool)
    chosen, used = [], []
    for st, end, repl in pool:
        if len(chosen) >= k:
            break
        if all(end <= u0 or st >= u1 for u0, u1 in used):
            chosen.append((st, end, repl))
            used.append((st, end))
    return chosen


def main():
    best = open(SRC, "rb").read()
    orig = best
    if not os.path.exists(TGT):
        sys.exit(f"target obj missing: {TGT} - run `gruntz build` first (the delink step).")
    try:
        s0, s1 = _target_span(best)
        if (s0, s1) == (0, len(best)):
            print(f"WARNING: could not locate {SYM}'s RVA span - mutating the WHOLE file", flush=True)
        else:
            print(f"scoped to {SYM}: lines {best.count(chr(10).encode(), 0, s0) + 1}"
                  f"..{best.count(chr(10).encode(), 0, s1) + 1}", flush=True)
        bscore = score(best)
        print(f"start {bscore:.3f}", flush=True)
        if bscore < 0:
            sys.exit(f"baseline did not compile/score - fix {SRC} / check the symbol {SYM}.")
        seen = set()
        edits = _edits(best)                              # one parse per `best`
        improved, rounds = True, 0
        while improved and rounds < 12:                   # phase 1: greedy single-edit
            improved = False
            rounds += 1
            random.shuffle(edits)
            for e in edits:
                cand = _apply(best, [e])
                if cand == best or cand in seen:
                    continue
                seen.add(cand)
                s = score(cand)
                if s > bscore + 1e-6:
                    print(f"  round {rounds}: {bscore:.3f} -> {s:.3f}", flush=True)
                    best, bscore, improved = cand, s, True
                    edits = _edits(best)
                    break
        for attempt in range(ITERS):                      # phase 2: disjoint multi-edit walk
            cand = _apply(best, _pick_disjoint(edits, random.randint(2, 6)))
            if cand == best or cand in seen:
                continue
            seen.add(cand)
            s = score(cand)
            if s > bscore + 1e-6:
                print(f"  walk {attempt}: {bscore:.3f} -> {s:.3f}", flush=True)
                best, bscore = cand, s
                edits = _edits(best)
                again = True                              # greedy re-climb from the new best
                while again:
                    again = False
                    for e in edits:
                        c = _apply(best, [e])
                        if c == best or c in seen:
                            continue
                        seen.add(c)
                        sv = score(c)
                        if sv > bscore + 1e-6:
                            best, bscore = c, sv
                            edits = _edits(best)
                            print(f"    reclimb: -> {bscore:.3f}", flush=True)
                            again = True
                            break
        print(f"FINAL {bscore:.3f} ({'improved' if best != orig else 'no change'})", flush=True)
    finally:
        with open(SRC, "wb") as fh:                       # always leave the best-known variant
            fh.write(best)


if __name__ == "__main__":
    main()
