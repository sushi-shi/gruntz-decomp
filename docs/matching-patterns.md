# Matching patterns — MSVC 5.0 SP3 codegen idioms (x86, `/O2 /MT`)

A reference for the binary-matcher (agent or human): assembly ↔ source patterns
that recur in `GRUNTZ.EXE`, so you can *read* the disassembly and *write source
that reproduces the exact codegen*. This is the compiler's behaviour, not the
game's logic.

Toolchain this documents: **MSVC 5.0 SP3** (`cl` 11.00, `_MSC_VER 1100`),
linker 5.10.7303, **`/O2 /MT`** (the locked global flags), x86, `__cdecl` for
free functions / `__thiscall` for members. See `docs/zlib-matching.md` for how
the flags were calibrated and `docs/build-system.md` for the match loop.

Each entry is tagged:
- **[VERIFIED]** — confirmed by an actual byte-exact match in this repo (the
  proving function is cited). Trust these.
- **[HEURISTIC]** — a plausible rule worth *trying*, not yet confirmed for VC5.
  Test it, then promote to VERIFIED (with the proving function) or refute it.

When you confirm or discover a pattern, add it here and cite the function.

---

## [REFUTED → QUALIFIED] Stack-local slot order is **name-dependent but NOT alphabetical**; and at `/O2` it does **not** depend on names at all

**Original claim (refuted).** "Stack-resident locals get their `[ebp-X]` offsets
in *lexicographic* order of their identifier names." **This is false for MSVC
5.0 SP3.** Tested empirically (see evidence below). What's actually true:

- **`/Od`:** the slot order **does change when you rename locals** (so names *do*
  influence layout — declaration order is NOT the key either), but the order is
  **not lexicographic**. It is a **name-dependent hash/bucket order** that you
  cannot predict by alphabetizing. Two name sets that are identical in
  declaration position produce *different* `[ebp-X]` assignments, and a name set
  that is already in alphabetical order (`aaa,bbb,ccc,ddd`) does **not** get
  alphabetical slots (see test t7: `aaa` landed at the *farthest* slot, not the
  closest). So: renaming perturbs `/Od` layout, but alphabetizing won't
  reproduce a target's order.
- **`/O2` (our match flags):** renaming stack-resident locals has **zero effect**
  on codegen — the `.text` is **byte-identical** before/after a rename (only the
  symbol/string table grows for longer names). Slot layout at `/O2` is driven by
  the optimizer's allocation/scheduling, **independent of identifier names**.
  And as noted, most scalars enregister/coalesce at `/O2` and get **no slot at
  all**. **Practical relevance to our matching: essentially nil.** Don't rename
  locals hoping to shift `/O2` stack offsets — it won't move them. To change a
  `/O2` stack slot you must change something the optimizer sees: the local's
  **type/size**, whether its **address escapes**, its **live range**, or the
  **count/order of operations** on it.

**So how *do* you match a `/O2` stack offset?** Match the *shape*, not the name:
reproduce the same set of genuinely-stack-resident objects (same types/sizes,
same address-escaping, same live ranges). The compiler then assigns the same
offsets deterministically regardless of what you call them. Renaming is a no-op.

**Evidence (MSVC 5.0 SP3, `cl /c /MT`, scratch in `build/asm-experiments/`).**
All probes had 4 `int[4]` locals in fixed declaration positions `pos0..pos3`,
each address-taken (passed to `sink`) and live (stored to `out`), so all four
stay stack-resident. Only the **names** were permuted across runs. Slot index 0
= closest to `ebp` (`-0x10`), 3 = farthest (`-0x40`).

`/Od` — slot assignment vs. names (decl position fixed, names permuted):

| run | pos0 | pos1 | pos2 | pos3 | closest→farthest (`-0x10`→`-0x40`) | alphabetical? |
|-----|------|------|------|------|------------------------------------|---------------|
| t1  | `zulu@-40` | `alpha@-10` | `mike@-30` | `bravo@-20` | alpha,bravo,mike,zulu | coincidentally yes |
| t2  | `delta@-20` | `yankee@-10` | `charlie@-30` | `oscar@-40` | yankee,delta,charlie,oscar | **no** |
| t3  | `sierra@-40` | `golf@-30` | `tango@-20` | `alpha@-10` | alpha,tango,golf,sierra | **no** |
| t7  | `aaa@-40` | `bbb@-10` | `ccc@-20` | `ddd@-30` | bbb,ccc,ddd,aaa | **no** (names already sorted; slots are not) |

t1's "alphabetical" result is a coincidence; t2/t3/t7 break it. Renaming clearly
**reshuffles** the offsets (so not declaration order), but along a hash order, not
A→Z. Even the loop counter `i` moved slot when only the *array* names changed
(t5 `i@-0x24` vs renamed-twin t6 `i@-0x4`), confirming a whole-symbol-table
re-hash.

Other positional rules tested against the same data and **also refuted** (decl
position → slot, slot 0 = closest `-0x10`, 3 = farthest `-0x40`):

| run | pos0 | pos1 | pos2 | pos3 |
|-----|------|------|------|------|
| t1  | slot3 | slot0 | slot2 | slot1 |
| t2  | slot1 | slot0 | slot2 | slot3 |
| t3  | slot3 | slot2 | slot1 | slot0 |
| t7  | slot3 | slot0 | slot1 | slot2 |

- *"first-declared goes to the farthest slot, rest ordered"* — fails: t2's
  pos0 lands in slot 1, not the farthest (pos3 does).
- *"reverse declaration order"* — would need pos0→slot3,…,pos3→slot0 every
  time; t1/t2/t7 violate it.
- *"the rest (pos1..3) are in declaration order among themselves"* — fails on
  t1 (slots 0,2,1, not monotonic).

There is **no positional rule**; the slot is a function of the **name** through a
hash. The mapping is **deterministic** (same source → same offsets) but **not**
computable by hand from the names.

**Build order is irrelevant.** Recompiling the same TUs in a different sequence
(reversed/interleaved, sharing one persistent `wineserver`) produces
**byte-identical** `.text` — the layout depends only on the source TU, not on
compile order or prior-invocation state. (Verified: every probe rebuilt in a
shuffled order matched its original `.text` exactly.)

`/O2` — same probes, renamed twin: `.text` **byte-identical** (0xb0 bytes both),
diff empty. Renaming changed only the COFF symbol/string table, never a single
code byte or stack offset. (Test t5 vs t6.) Likewise t1↔t2↔t3 at `/O2` were
byte-identical to each other.

Mixed sizes (`double zulu; char alpha[3]; int mike; double bravo`, `/Od`, t4):
layout is **not** size/alignment-bucketed — the `int` (`mike@-0x10`) sits between
the two 8-byte doubles (`bravo@-0xc`, `zulu@-0x18`); each symbol just consumes
its natural size at its hash-ordered position.

**Bottom line for matchers.** Do **not** try to reproduce a target's stack-slot
order by renaming/alphabetizing locals — it doesn't work for VC5 (no-op at `/O2`;
unpredictable hash shuffle at `/Od`). Match offsets by matching the locals'
*types, sizes, address-escaping and live ranges* instead.

*(Refuted by `build/asm-experiments/t1`–`t7`, this experiment. No in-repo
byte-match function cites this because the rule is not actionable.)*

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
