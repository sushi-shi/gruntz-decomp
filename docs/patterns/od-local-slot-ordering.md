# /Od `[ebp-N]` local slots are ordered by the local's NAME (hash), not by declaration position — SOLVED

**Tags:** cpp:local | asm:mov | topic:codegen-idiom topic:regalloc
**Confidence:** 9/10

## Symptom

A function retail compiled **unoptimized** (`/Od`, full `push ebp; mov ebp,esp;
sub esp,N` frame, every local through memory) recompiles with the **exact same
instruction stream** — same mnemonics, same operands, same order, same frame size
— yet objdiff shows ~85% *byte* match (~99.5% *fuzzy*, because fuzzy scores
mnemonic+operand shape, not the displacement immediate). The **only** difference
is the `[ebp-N]` displacement on every local access: the same slots, permuted
across the same locals.

## Why (the mechanism — measured, not guessed)

MSVC 5.0 `/Od` walks its **local symbol table** to assign slots, and that table
is keyed by the identifier, so:

1. **The slot order is a function of the local NAMES.** Renaming one local
   reshuffles the layout; renaming it within its hash bucket changes nothing
   (measured: `sp`→`asp` was a total no-op, `sp`→`zsp` moved five slots).
2. **Declaration order only breaks ties INSIDE one bucket**, and there the
   **later-declared local takes the earlier (higher, closer to ebp) slot.**
3. Declaration position across buckets, scope nesting (function scope vs a
   nested block vs a `for`-init), and `/Oi` are all irrelevant — measured no-ops.
   (The older reading of this pattern — "retail's lexical scoping is
   unobservable, therefore a wall" — was wrong; scope has nothing to do with it.)

## How to solve it (mechanical, ~200 compiles, no guessing)

The oracle is the compiler; iterate on ONE unit — `ninja
build/objdiff/base/<unit>.obj` is ~1.6 s.

1. **Read retail's role→slot map** off `gruntz sema disasm <rva> --lite`.
   Identify each local by what is stored into it: the `$0x0` inits in order, the
   `mov eax,[ebp+0xc]` parameter copy, the `and ecx,0x3f` run mask, …
2. **Probe your identity name set** — ONE compile gives the full relative order of
   all N names at once (read the same role→slot map out of your own obj).
3. **Rank candidate names**: per role, substitute one candidate at a time (all
   other names fixed) and record the position it inserts at among the other N-1.
   Cost = the sum of the pools; the result is the gap each candidate falls in.
4. **Pick a chain**: choose one *semantically apt* name per role whose gaps are
   non-decreasing in the required slot order, and emit the declaration list with
   each tie-group **reversed** (rule 2). Verify with one compile.

Worked, byte-exact: `CDDSurface::DecodeByteRun1Plane` (0x145270) and `DecodeByteRun3Planes`
(0x1453f0), both 99.5 → **100 EXACT**. Retail's layout for both is
`sp -4, hold -8, tok -c, y -10, len -14, dstp -18, k -1c, cols -20 (, base -24)`
and the name set that reproduces it is exactly those identifiers, declared
`sp, y, tok, hold, k, dstp, len, cols, base`.

**The names are ours to choose** (retail's are unrecoverable), so this is not a
fit-the-metric hack: pick names that are correct for the role AND land in the
right bucket. Do not settle for semantically wrong names — the candidate pools
are large enough that a sensible chain exists (both solutions above use only
plain `sp/hold/tok/y/len/dstp/k/cols/base`).

`CDDSurface::DecodeRun8` (0x140aa0) and `DecodeRun24` (0x140c50) closed the same way
(99.50/99.54 -> **100 EXACT**), and they show the one gotcha: **the hash order depends on
how MANY locals are in the table**, so a solved name set does NOT carry over between
functions. DecodeByteRun1Plane's eight names produce a different order once DecodeRun8's three
extra locals join them - each function must be ranked from scratch. Their solutions are
`sp/hold/tok/w/pbits/y/runx/dstp/kj/height/nleft` and
`inp/rest/pm/ln/nrow/cnt/dst/k/cols` respectively.

Two practical notes from those two:
- **Read the required permutation off the disassembly, don't identify roles by hand.** The
  instruction STREAM is identical between base and target, so zipping the two
  `[ebp-N]` displacement sequences gives the base-slot -> target-slot map directly, and
  the number of positions that already agree is a fine-grained search score (73 and 184
  positions respectively - far better signal than a boolean).
- **Beautify afterwards.** Solve first with a wide pool, then try nicer names one role at
  a time keeping only those that preserve a perfect score. That turned DecodeRun24's
  `sn/ru/dp/kx` into `inp/cnt/dst/k`. Two of its locals (`pm`, `ln`) have no descriptive
  name in the right bucket - an 11x11 pair sweep found none - so the comment names them.

## Distinguish from

- [`stack-slot-coalesce-frame-4b`](stack-slot-coalesce-frame-4b.md) — a
  *frame-size* difference under `/O2`; here the frame size is identical.
- **`/O2` frame layout is NOT name-driven.** Measured on `CPlay::DrawDebugStats`
  (0x0cf770): 20 different names for one local moved nothing, and neither did
  reordering or re-scoping the declarations. A `/O2` frame-offset difference is a
  real *local-set* difference — a wrong array size, or one local too many. (There
  it was both: retail's buffer is 0x200 not 0x1f0, and retail has no separate
  out-`RECT` — it hands `GetRect` the sprintf scratch buffer.)
- [`o2-optimizer-bailout-framed`](o2-optimizer-bailout-framed.md) — that is about
  `/O2` *emitting* an unexpected framed body; here the body is genuinely `/Od`.
