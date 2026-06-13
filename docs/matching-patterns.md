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
