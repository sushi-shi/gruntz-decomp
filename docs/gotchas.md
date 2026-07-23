# Gotchas — measurement, build, and matching traps

Hard-won traps that cost real time. Grouped by area. The deeper codegen idioms live in
`docs/patterns/`; this file is the fast index of the *surprises*.

## Measuring match % (objdiff)

- **`objdiff --diff` MASKS large immediates as `<addr>`.** A wrong magic constant (a
  reciprocal divisor `0x88888889`=/30, `0x2aaaaaab`=/12, `0x55555556`=/3; an address; a
  flag word) is INVISIBLE in the plain diff and shows only as a downstream `sar`/`shr`/`lea`
  shift. **Always verify constants with `--base`** (the raw disasm). Multiple exact matches
  hid behind this (`/9`-vs-`/30`, FP `fild` operands).
- **`objdiff-cli report generate` returns STALE per-fn %** — it does not re-run the
  normalize / content-address step, so it can report a function unchanged when it actually
  moved (76%→85% shown as 76%). **Only `gruntz build --fast` gives accurate per-function
  fuzzy%.** Never trust a bare `report generate` number.
- **A normalization refresh can expose fake `_$E<n>` matches.** The suffix is a
  compiler emission ordinal, and retail helpers may lack the relocation records
  needed to hash them like the base object. A broad loss of exact tiny
  static-init helpers after refresh is evidence that old ordinal pins or stale
  normalized copies inflated the report; keep the real static objects, remove
  the pins, and preserve their observed rows in
  `config/compiler-generated-functions.tsv`. See
  `docs/patterns/volatile-compiler-ordinal-refresh-dip.md`.
- **Inline `.text` jump tables defeat objdiff alignment.** A function with one or two inline
  jump tables can't be aligned across the table region, so its *current* % measures
  alignment luck, not byte-correctness — **a byte-BETTER reconstruction can show a LOWER
  current %** (keep it; MAX-fuzzy preserves the best; see structure-over-current-%).
- **Delinker duplicate-symbol → false 0%.** A function reached via an ILT thunk that also
  contains inline jump tables whose self-target relocs reference its own symbol gets emitted
  TWICE in the delinked obj (real `.text` def + a size-0 UNDEFINED external); objdiff pairs
  the size-0 copy → hard **0%** despite a byte-correct body. Undercounts exact tree-wide.
  Detect: delinked objs with a size-0 undefined symbol that also has a `.text` def. This is a
  DELINKER/attribution fix, not a source fix (e.g. `MorphByTool`, `SetActionCode`).

## Build / worktree state (pool worktrees carry stale build state across resets)

- **`build/` stale after a unit was REMOVED from `units.toml`** → `vostok-delinker` fails
  `delink_data_section_manifest.tsv: storage does not match candidate section` (on
  `reghelpers.c`). ninja never GCs orphaned outputs. Fix (build/ is gitignored): delete
  `build/gen/labels/<stem>.{csv,functions.json,globals.json}` + `build/objdiff/{base,target,normalized}/<stem>.obj`
  for every `<stem>` NOT in `config/units.toml`, then `rm -f build/objdiff/.delink.stamp`, rebuild.
- **`build.ninja` stale after a header was DELETED** → `ninja: error: <hdr> missing and no
  known rule to make it`. The manifest's `configure` edge fires only on `config/units.toml`/
  `configure.py` mtime changes, so a source-only tree change doesn't regenerate it. Fix:
  `rm -f build.ninja .ninja_deps .ninja_log .ninja_lock` (ROOT ninja state ONLY, NOT `build/`
  — keeps the expensive delink/Ghidra caches). A reset worktree may need BOTH this and the GC above.
- **`GRUNTZ_SKIP_INIT=1`** before `nix develop -c <cmd>` skips the slow shell-entry `gruntz init`
  warmup — use it for quick one-off commands in a warm worktree.
- **Wineserver leak** — subagent builds leak `wineserver` processes; `pkill -9 -f wineserver`
  stale ones (they slow later builds). Don't chain build+format+status (can exceed timeouts).
- **`clang-format not on PATH — skipping format`** on commit is HARMLESS (formatting is
  whitespace-only / matching-neutral; it runs inside `nix develop`).

## Permuter / walls (see `docs/patterns/` + the `permute` skill)

- **`permute.py` (operand-order/reassoc/decl-split) cannot move regalloc.** MSVC5
  canonicalizes `ptr+i == i+ptr`, so operand swaps are no-ops on SIB walls.
- **`match_variants --state-trials` targets cross-function compiler state, not arbitrary
  source bugs.** It perturbs the *preceding* TU content, so use it when a source-identical
  later function moves after TU composition changes. Do not assume independent COMDATs mean
  independent codegen: adding the real preceding `BlitIntoDesc` changed two `ShadeRect`
  loop schedules, including mask/shift order and `ax` vs `di` partial-register selection.
  Conversely, four previously tested intrinsic wall families did not move even at 1024
  variants. Use a controlled predecessor A/B test to classify the residue before spending
  a large state search; see
  `docs/patterns/preceding-function-state-recolors-later-comdat.md`.
- **A 95%+ "regalloc wall" is often a MISLABELED CORRECTNESS BUG** the diff masks. Audit
  before believing the `@early-stop`. This is the real yield lever (see the playbook below).

## The mislabeled-bug audit playbook (the actual % yield on near-100% functions)

Each recurred and banked exact/near-exact matches. Grep-able signatures:

- **Signedness** — retail `jb/jbe/ja/jae` where you emit `jl/jle/jg/jge` (usually a loop
  guard/unsigned compare → cast the guard to `u32`). **FP variant:** retail zero-extends an
  `i32` time/delta field before `fild` = `static_cast<double>(static_cast<u32>(x))` (`fild
  qword` vs your `fild dword`).
- **Wrong magic constant** — reciprocal divisor/multiplier hidden by objdiff's `<addr>` mask;
  verify `--base`.
- **Block-ordering / inline-vs-out-of-line path** (~+70pp, recurs a lot) — retail places one
  switch arm's body out-of-line where your source inlines it (or vice-versa); reorder to match.
- **`do-while` vs `while`** read loop; **`memcpy` vs `memmove`**.
- **Shared `goto fail` epilogue** — retail merges all gate-failures to ONE `return 0`.
- **Inlined static helper** — mark a static helper `inline` to reproduce retail's inlined loop
  (and it lifts every caller in that TU).
- **Missed CSE** — cl reloads `this->m_x` before each store (aliasing-conservative); hoist it
  into a local, matching retail's cached register.
- **Branch polarity** — retail makes the failure/shorter arm the fall-through.
- **`__cdecl` alias vs real `__thiscall` method** — a call modeled as a free `__cdecl` alias
  (wrong `this`, spurious CString ctor) that's really `obj->Method()` NRV `__thiscall`. Delete
  the alias extern, call the real method.
- **Duplicate compare** — a type-discriminator chain listing the same target twice → cl
  CSE-folds a `cmp` retail keeps distinct.
- **List-walk order** — retail's inlined `GetNext` advances first (`cur=node; node=node->m_next;
  use cur`) vs your process-then-advance.

## Reconstruction targeting

- **Aim by UNIT-%, not function-%.** Mid-% functions in an ALREADY-reconstructed unit (high
  unit-%, e.g. ~90-95%) are genuine register/frame walls with no yield — a prior wave already
  fixed their shape. The shape-bug vein lives in LOW-unit-% units (`userlogic` 0.2%,
  triggermgrgrid 28%, gruntspawnconfig 41%, …) and `@stub` bodies (`rg '@stub' src`; each stub
  = net +1 exact).

## Codegen / modeling

- **`new X` inlines the ctor; placement `new (raw) X` does NOT** (MSVC5 /O2). To reproduce
  retail's inlined vptr-stamp + member init, use `new X`, not placement-new on raw storage.
- **Header butterfly** — adding a member (or an include) to a HOT header perturbs /O2 regalloc
  in that header's consumers; a "byte-neutral" fold across a 40-consumer header is NOT
  byte-neutral (it can net +/- a few exact via the ripple — measure). Enum-vs-int is neutral
  ONLY if the TU already includes the enum header (adding the include perturbs regalloc).
- **MSVC5 has no ICF** — two source functions with identical bodies are two functions. If a
  retail function shows up under two names at the SAME RVA, that's the fake-view symptom (the
  same body reconstructed on a fake view and on the real class); recover the one real identity.

## Cleanliness tooling (see `docs/cleanliness-metrics.md`, `docs/cast-metric-policy.md`)

- **`gruntz.cleanliness.board`** computes `config/cleanliness-baseline.tsv` (23 metrics,
  comment/string-stripped) and prints them with a delta each `gruntz build` (the report is
  authoritative; the doc's numbers are a snapshot). `--update` blesses a new baseline.
- Most metrics are at **0** (casts, placeholder classes/vtables/views, `)this`/`)m_`/`(char*)`
  casts, offset-cast macros — all DONE). Remaining actionable: **m_&lt;hex&gt; fields (~8.5k),
  Method/Stub/FUN/Gap (~245 unreconstructed stubs), `void* m_` members (18),
  `reinterpret_cast<class*>(m_)` (1), cpp extern decls (490)**.
- The one-shot cast CONVERTERS that drove those to 0 (`cast_ptr_to_named`, `cast_to_static`,
  `cast_str_to_named`, `cast_drivers`) are ARCHIVED in `scripts/archive/`.
- Live gates run every `gruntz build` (fail the build): `vtable_slot_binding`,
  `vtable_coverage`, `vtable_virtuality`, `vtable_bans`, `class_vtables`, `class_sizes`,
  `view_debt`, `high_water` (MAX-fuzzy), `verify_stubs`, `verify_unique_names`,
  `verify_library_overlap`, `gate_selftest`, `tu_order_check`. **Caveat:** a cleanliness
  regex can silently rot vs actual naming — a green `0` is a claim to re-verify against a fresh
  identifier enumeration, not proof.
