# Non-virtual header-inline members crater callers — MSVC5 INLINES them (NOT a delinker bug)

**CORRECTION (measured 2026-07-10).** An earlier version of this note blamed the
delinker's "per-unit target packing" (a distant rva mis-packing a unit's `.text`
block). **That mechanism is false.** Reproduced end-to-end in a throwaway worktree:

- **The delinker is innocent.** Re-attributing `CBrickzGrid::ComputeCellFlags`
  (0x77790) brickz→rockbreakparticles in `symbol_names.csv` and re-delinking *with no
  base recompile* — the exact "far rva into a distant unit" the old note blamed — left
  `BuildRockBreakParticles` at **80.82 → 80.74%** (unchanged), `ApplyMove` 70.13 → 70.09.
  `object_files.rs` groups functions by the PDB line-program file and `append_section_data`s
  them in iteration order, pairing **by symbol name** — there is NO rva-contiguous block to
  disrupt.
- **The crater is MSVC5 inlining, on the base side.** Making `ComputeCellFlags` an
  `inline` header member and rebuilding: base `BuildRockBreakParticles` **doubled
  1008 → 2096 B**, the switch constants (`0x4002008`/`0x1bf40000`) are now folded **inside**
  it, and **0 calls** to `ComputeCellFlags` remain. MSVC5 `/O2` inlined the 893-byte member
  into its caller. Retail does the opposite — the delinked target carries `U ComputeCellFlags`
  (undefined extern) = retail **calls** it. Inlined-caller vs called-in-retail ⇒ 0%.

**Root cause.** A non-virtual member that is `inline`-visible in a header gets **inlined
into every caller that odr-uses it** by MSVC5 `/O2`. Retail emitted these as standalone
out-of-line functions (reached via incremental-link thunks), so retail's callers CALL them.
Inlining them into the caller diverges the caller's whole body from retail.

**Why virtuals are exempt.** A virtual inline member is dispatched `call *(%eax)` through the
vtable; MSVC5 does not devirtualize, so it is **never inlined at the call site**. That is why
the ~61 working header-inline cases (`GetTypeTag`, small accessors) are safe — they are
virtuals and/or genuinely tiny functions that retail *also* inlined.

**No lever rescues it.** `__declspec(noinline)` is **rejected by MSVC5** (`C2485:
'noinline' : unrecognized extended attribute`). `#pragma auto_inline(off)` and
`#pragma inline_depth(0)` on the DEFINITION have **no effect** — inline-expansion pragmas
gate the *call site* (the caller's TU), not the callee. The tricks that do disable inlining
(`__try`/`alloca`/recursion/taking the address) change the function's OWN codegen and break
its match.

**Resolution.** Keep non-virtual, multiply-called members **out-of-line in their owning
class's `.cpp`** — that is faithful (retail calls them) and is already the state for
`ComputeCellFlags` (in `Brickz.cpp`). **Do not move them to headers** (SPLIT_PLAN category D
is unfaithful for them). The delinker needs no change; there is no COMDAT / emitter-unit fix
to build. Both spellings are NOT byte-identical (the old note's claim) — the header spelling
inlines into callers; only out-of-line matches.

**Recognizing the case.** A scattered singleton reached via an incremental-link thunk with
multiple caller TUs is an OUT-OF-LINE function, not a header-inline member. Leave it in the
owner's `.cpp`.
