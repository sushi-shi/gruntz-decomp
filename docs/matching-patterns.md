# Matching patterns — MSVC 5.0 SP3 codegen idioms (x86, `/O2 /MT`)

A reference for the binary-matcher (agent or human): assembly ↔ source patterns
that recur in `GRUNTZ.EXE`, so you can *read* the disassembly and *write source
that reproduces the exact codegen*. This is the compiler's behaviour, not the
game's logic.

Toolchain this documents: **MSVC 5.0 SP3** (`cl` 11.00, `_MSC_VER 1100`),
linker 5.10.7303, **`/O2 /MT`** (the locked global flags), x86, `__cdecl` for
free functions / `__thiscall` for members. See `docs/zlib-matching.md` for how
the (compile) flags were calibrated, `docs/build-system.md` for the match loop,
and `docs/linker-flags.md` for the deferred whole-binary link-reproduction flags.

Each entry is tagged:
- **[VERIFIED]** — confirmed by an actual byte-exact match in this repo (the
  proving function is cited). Trust these.
- **[HEURISTIC]** — a plausible rule worth *trying*, not yet confirmed for VC5.
  Test it, then promote to VERIFIED (with the proving function) or refute it.

When you confirm or discover a pattern, add it here and cite the function.

---

## [VERIFIED-isle] Compiler entropy — set expectations FIRST

This is the **single most important thing to internalize** before you match a
single function. Read it before you treat any near-miss as a bug.

The phenomenon (verbatim, from isledecomp/reccmp practice on this MSVC era):
> "changes to the code base, for instance in a header, can pseudo-randomly
> affect the code generation of functions in compilation units that include
> this header, even if completely unrelated."

Concretely:
- **Triggers** are tiny: adding an *unused* inline function or even just an
  `enum` declaration to a header can flip the codegen of an unrelated function
  in any TU that includes it.
- **Scope** ~5% of functions are susceptible at any time. They fluctuate up and
  down as you edit — "whack-a-mole": fixing function A can regress function B
  that was green, and vice-versa.
- It behaves "**like it uses the file itself as the random seed**" —
  deterministic per exact source state, but *any* edit to a file (or an included
  header) may re-roll how an unrelated function compiles. Even **unoptimized**
  code is susceptible (it is not a `/O2`-only effect).
- It **poisons regression detection**: when a previously-green function goes red
  after an unrelated edit, you often can't tell a *real* bug from MSVC simply
  picking a different (equally valid) encoding.
- isle **plateaued at ~98% instruction-accuracy**; full 100% across a whole
  binary is considered unlikely "without reverse-engineering the compiler."

**Calibrate accordingly: a high-90s plateau is success, not failure.** Do not
read the last few percent as "the match is wrong." (See `orchestration.md` §
"Entropy expectation" for how this folds into progress tracking — the objdiff
number on a near-green function is *advisory*, not a verdict.)

Working theory: the old MSVC backend's symbol-table hash/bucket ordering plus its
register/temporary allocation are sensitive to the **set and order of symbols
visible in a TU** — not just the symbols a function actually *uses*. This is the
**same family of symbol-table sensitivity** as this project's reverse-engineered
`/Od` 16-bucket name-hash (the `/Od` symbol bucketing we reconstructed): both are
the compiler's symbol table leaking into output. (At `/O2` the stack-local
ordering is *name-independent* — see § "Match by shape" below and
`orchestration.md` § 6 — but the *symbol-set* sensitivity that drives entropy is
real at every opt level.)

**Mitigations [HEURISTIC]** (none confirmed to fully eliminate it for VC5):
1. **Minimize header churn** — `#include` only what a TU needs; every extra
   declaration is a potential re-roll.
2. **Match the include order *and* the symbol SET**, not merely the symbols a
   function uses — the visible set is what the compiler hashes.
3. Keep **declaration/definition order in headers stable** once a TU is green.
4. When a previously-green function **regresses after an unrelated edit**,
   suspect entropy first: revert or re-scope the edit rather than rewriting the
   victim function.
5. **Don't sacrifice a correct function to chase another.** If A and B can't both
   be green, annotate the green-enough one and move on — don't ping-pong.
6. **Per-TU isolation**: group functions into their original translation units so
   an edit's blast radius stays inside one TU (this is also why we dispatch
   matchers per-TU — `orchestration.md` § 2).

## [VERIFIED+HEURISTIC] Service-pack sensitivity

`c2.dll` (the MSVC back-end codegen) changes between toolchain versions: **VC5
RTM vs VC5 SP3 vs VC6** emit different instruction selection/scheduling for the
*same* source. We **target VC5 SP3** [VERIFIED — calibrated; see
`docs/toolchain-vc50-sp3.md` and `zlib-matching.md`]. So when a logically
identical function refuses to match, suspect, **in this order**:
1. a **real source difference** (wrong logic/types/offsets);
2. **entropy** (the section above) — an unrelated edit re-rolled it;
3. **wrong SP/version** of the toolchain [HEURISTIC — least likely here, since
   the global toolchain is already pinned to SP3].

---

## [VERIFIED] Frameless prologue ⇒ `/O2`

`/O2` (== `/Ox`) emits **no** `push ebp; mov ebp,esp` frame for leaf/simple
functions — it addresses locals/args off `esp` (or keeps them in registers).
`/O1`, `/Os`, `/Od` emit the `ebp` frame. So a frameless function is a `/O2`
tell, and a function that *does* set up `ebp` was likely **not** `/O2` (or needs
a frame for `alloca`/EH/large frames). *Proven: `adler32` @ 0x1882d0 — frameless
at `/O2`, framed at `/O1`/`/Os`/`/Od`.*

## [VERIFIED] Unsigned `%` by a constant is **not** strength-reduced

`x % 65521` compiles to a literal divisor: `mov reg, 0xFFF1; … div reg` — MSVC 5
does **not** turn an unsigned modulo-by-constant into the magic-number multiply
that later compilers use. So a real `div` with a literal in the code = a genuine
`%`/`/` in the source; don't try to "optimize" it in your source. *Proven:
`adler32` (`s1 % 65521`, `s2 % 65521`).*

## [VERIFIED] `__thiscall` constructor shape `??0Class@@QAE@XZ`

A constructor takes `this` in **ECX** and **returns `this` in EAX** (look for
`mov eax, ecx` near the end / the value flowing to EAX before `ret`). The
mangled name is `??0<Class>@@QAE@XZ` (`QAE` = public `__thiscall`). A
ctor-shaped function that writes through `[ecx]` but does **not** return `this`
is **not** this standard form — it's a helper or a different emission; don't try
to match it as a `??0…`. *Proven: the 5 ctors in `src/`; counter-example: the
`CUserBase`-family ctors that don't return `this` (deferred).*

## [VERIFIED] vtable-pointer store

A constructor (of a polymorphic class) stores the class vftable into `[this+0]`:
`mov dword ptr [ecx], offset ??_7<Class>@@6B@` — an absolute **DIR32**
relocation against the `??_7…@@6B@` vftable symbol. In source this is just the
implicit vptr init MSVC emits for a class with virtuals; you get it for free by
declaring the right `virtual` members so the vtable layout matches. *Proven: all
5 ctors (e.g. `CGameWnd` vftable 0x5ea344, `CGameApp` 0x5e9b0c).*

## [VERIFIED] Member stores *before* the vptr store ⇒ **inline** base ctor

Order of writes in a constructor tells you whether the base-class constructor
was inlined:
- vptr stored **first**, then members → single class (or out-of-line base).
- some members zeroed **before** the vptr store → the **base-class ctor was
  inlined** into the derived ctor. MSVC 5 inlines a base ctor **only if it is
  defined inline** (in the header); an out-of-line base ctor is a `call`.

So to reproduce a "members-then-vptr" schedule, define the base constructor
**inline**. *Proven: `CSBI_RectOnly : CStatusBarItem` @ 0x101fa0 — base fields
zeroed (base's redundant store dead-eliminated), then `+8 = 1`.*

## [VERIFIED] The optimizer reorders field stores — mirror it in source

`/O2` may schedule member stores out of declaration/offset order (e.g. the
`+0x10` store emitted **before** `+0x0c`). To byte-match, **reorder the member
initializers in your source** to match the emitted schedule rather than the
struct layout. *Proven: `CGameApp::CGameApp` @ 0x13d590 — source initializes
`m_10` before `m_c`.*

## [VERIFIED] Bulk zero of a contiguous region ⇒ `rep stosd`

A large contiguous run of zeroed members (an embedded array/buffer) compiles to
`xor eax,eax; mov ecx,<dwords>; lea edi,[this+off]; rep stosd`, not individual
stores. Model it in source as an array member (or `memset`) of the right size at
the right offset. *Proven: `CTileTriggerLogic` @ 0x1107f0 — 24-dword (96-byte)
array at `+0x3c` via `rep stosl`.*

---

## Matching-pipeline gotcha (scoring artifact, not a code diff)

When objdiff reports ~99.5% **fuzzy** (not 100% **exact**) on a function whose
bytes you've confirmed identical, suspect the **delinker reloc-typing gap**, not
a real mismatch: the delinked *target* emits a vftable/global reference as
`IMAGE_REL_I386_REL32` against a Ghidra label (`vftable` / `DAT_…`), while `cl`
emits `IMAGE_REL_I386_DIR32` against the mangled symbol (`??_7Class@@6B@`). The
**code bytes match**; only the relocation's type/target-name differ across
sides, so objdiff won't score it "exact". **Confirm by direct byte-compare**
(reloc-covered slots masked) before chasing a phantom diff. (Fix on the backlog:
type absolute `.text→data` stores as DIR32 + name the vftable RVA by its
`??_7…@@6B@` in `synth_pdb`.) This hits essentially every constructor, so expect
it.

---

## [HEURISTIC] Codegen patterns → source

Reference idioms (from isledecomp/reccmp practice for this MSVC era) for *reading*
disassembly and *writing* source that reproduces it. These are **not yet
confirmed for VC5/Gruntz** — try them, then promote with a proving function or
refute. They **complement** the [VERIFIED] entries above; where one overlaps a
verified entry it is cross-linked, not restated.

- **Stack frame & FPO.** A frameless (`esp`-relative / register-resident) function
  is the `/O2` tell — already [VERIFIED], see § "Frameless prologue ⇒ `/O2`". A
  function that *does* set up `push ebp; mov ebp,esp` either isn't `/O2` or needs
  the frame (`alloca`, EH, very large frame). Match the frame *presence*, not just
  the body.
- **Calling conventions** (the global default is [VERIFIED] `/Gd` `__cdecl` for
  free functions, `__thiscall` for members — do not change this globally; fix
  convention mismatches once, globally, before chasing per-function diffs):
  - `__thiscall` — `this` in **ECX**; member ctors additionally return `this` in
    EAX ([VERIFIED] shape `??0Class@@QAE@XZ`, see § "`__thiscall` constructor
    shape").
  - `__cdecl` — caller cleans the stack; callee ends with plain `ret`.
  - `__stdcall` — callee cleans; ends with `ret N` (N = arg bytes). Win32 imports
    are `__stdcall` (see `runtime-dlls.md`).
  - `__fastcall` — first two integer args in **ECX/EDX**, rest on the stack.
- **Struct return.** Small structs returned in **EAX:EDX**; large structs via a
  **hidden return-slot pointer** the caller passes (look for a caller-supplied
  buffer address and a callee that writes through it then returns it).
- **`switch`.** Dense label sets → a **jump table in `.rdata`** reached by
  `jmp [table + eax*4]` with a preceding range check (`cmp`/`ja` to the default).
  Sparse sets → a **compare chain** (`cmp`/`je` ladder). Match the source
  `switch`'s case density to the form you see.
- **x87 FP (no SSE this era).** All floating point is x87 stack (`fld`/`fadd`/
  `fstp`…). `double` vs `float` literals matter: `1.0` (8-byte `.rdata` const)
  vs `1.0f` (4-byte) select different `fld` operands. int↔float conversions go
  **through memory** (`fild`/`fistp` with a stack temp), not a register move.
- **String/const pooling.** String and numeric constants land in `.rdata`. For
  zlib we [VERIFIED] that `/GF` pooling has **no effect** (no duplicate literals
  to fold — see `zlib-matching.md`); for engine TUs with repeated literals,
  `/GF`-style folding *could* matter — verify per TU before assuming.
- **Inlining (`/O2` is aggressive).** A `call` you expected but **don't** see ⇒
  the callee was inlined ⇒ mark that source function `inline` (or define it in the
  header). Conversely a base-ctor `call` you *do* see means the base ctor is
  out-of-line — [VERIFIED] in § "Member stores *before* the vptr store ⇒ inline
  base ctor".
- **Inter-function padding.** Gaps between functions are `0xCC` (`int3`), not
  `nop`; the body ends at the last real instruction. (Trailing `nop` alignment
  *can* appear too — see the adler32 note in `zlib-matching.md` — but don't count
  pad bytes as body.)

See also the [VERIFIED] entries that already cover specific schedules: store
reorder (§ "The optimizer reorders field stores"), bulk zero → `rep stosd`
(§ "Bulk zero…"), and unsigned `%` not strength-reduced (§ "Unsigned `%`…").

## [HEURISTIC] Common mismatch checklist — "close but not green"

When a function is *almost* matched, walk this list (reference, unconfirmed for
VC5). Most are source-level choices the compiler faithfully encodes — so the fix
is usually in **your source**, not in flags. (First rule out the two non-source
causes: the reloc-typing **scoring artifact** above, and **entropy** — § top.)

- **Signed vs unsigned.** `jae`/`jb` & `shr` (unsigned) vs `jge`/`jl` & `sar`
  (signed); unsigned div-by-power-of-2 → `shr`, signed → `sar`+rounding fixup.
  Fix the *type*, not the asm.
- **`++i` vs `i++`** — can differ when the value is used.
- **Loop form** — `for`/`while`/`do…while`. A `do…while` **drops the pre-test**
  (no initial `cmp`/`jmp` to the condition); if the asm tests *after* the body,
  write `do…while`.
- **Loop direction** — counting **down to zero** (`dec`/`jnz`) is common and
  cheaper than counting up; match the direction you see.
- **Local order/count.** At `/O2` stack slots are **name-INDEPENDENT** [VERIFIED —
  see § "Match by shape" / `orchestration.md` § 6]; do **not** rename to move a
  slot. The `/O2` levers are **declaration order**, types, sizes, address-escaping
  and live-ranges — reorder *declarations* (never rename) to move slots.
- **Short-circuit / operand order** in `&&`/`||` — swapping operands reorders the
  tests and branches.
- **Comparison form** — `if (x)` vs `if (x != 0)` vs `if (x > 0)` can pick
  different `test`/`cmp`/`setcc`.
- **Ternary vs if/else** — may select `cmov`-free branchy vs branchless-ish code;
  try the other form.
- **Intrinsic vs library `memcpy`/`memset`.** Under `/O2`, `/Oi` is on, so these
  often inline to **`rep movs`/`rep stos`** ([VERIFIED] bulk-zero → `rep stosd`).
  A literal **`call _memcpy`** in the target ⇒ the intrinsic was *off* for that
  call (or it's a non-inlinable size) — reproduce the call, don't force `rep`.
- **Constant folding / strength reduction.** Write the *readable* expression
  (`x * 2`, `x / 8`) and let the compiler reduce it — but note [VERIFIED] unsigned
  `% const` is **NOT** strength-reduced by VC5 (keep the literal `div`; §
  "Unsigned `%`…").
- **`__declspec`** — `naked` (no prologue/epilogue), `dllexport`, `noreturn` all
  change the frame/CFG; match them if the target shows the effect.
- **Calling-convention default mismatch** — if *every* call site is off, you have
  a global convention error; fix it globally first ([VERIFIED] default `__cdecl`).
- **Assertions.** `assert()` expands to a `__FILE__`/line/expr string + a call.
  Keep `assert()`s *only if* retail shipped them — **VERIFY** before deciding:
  this is a release build but carries leftover debug/profiler strings, so check
  whether `assert` `__FILE__`/line strings are actually present
  (`docs/strings-analysis.md`) before adding or stripping them. (`/DNDEBUG`
  strips `assert()`.)
- **`bool` vs `int`** — `bool` results use `movzx`/`setcc` (1-byte); `int` uses
  full-width. Pick the type that produces the width you see.

---

## Navigating while you match

Use the LSP tool to read the reconstructed tree without an editor:

```
python3 scripts/clangd_query.py def   <file> <line> [col]   # go to definition
python3 scripts/clangd_query.py refs  <file> <line> [col]   # all references
python3 scripts/clangd_query.py hover <file> <line> [col]   # type at point
python3 scripts/clangd_query.py symbol <fuzzy-name>         # workspace symbols
```

clang is a **reader** of this MSVC5 dialect — navigation is reliable, but the
wine `cl` build + objdiff are the only verdict on correctness.
