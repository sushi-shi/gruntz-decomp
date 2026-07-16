# The matching build system (manifest -> ninja -> objdiff)

The base/recompile side of the matching loop is a **native incremental ninja
build** generated from a single manifest. It replaces the old
recompile-everything front-end (`rebuild.py`, now removed) and folds the objdiff
project generation into `configure.py`. `gruntz build` is the one entry point.

```
config/units.toml  (per-TU manifest: unit, source, flags profile)
        |  python3 configure.py
        v
build.ninja  +  build/objdiff/objdiff.json
        |  ninja
        +-- PHASE 1 (compile): each unit -> build/objdiff/base/<unit>.obj
        |       via the `cl` rule -> scripts/gruntz/build/cc_wrap.py -> `wine cl /c ... /Fo`
        +-- TARGET (delink): scripts/gruntz/build/delink.py
        |       synth_pdb.py -> vostok-delinker -> build/objdiff/target/<unit>.c.obj
        v
objdiff-cli report generate -p build/objdiff -o build/objdiff/report.json
        |
        v
per-unit + roll-up match % (`gruntz build` prints this; it is now a thin
wrapper around configure.py + ninja + objdiff)
```

Everything runs inside `nix develop` — the one dev shell (`.#build` is a kept
alias of it), which exports `MSVC_DIR`, `DXSDK_DIR`, `WINEPREFIX`, and `ninja`
on PATH. The Wine prefix + toolchain env are set up by `gruntz init`, which the
shell runs on entry (idempotent).

## Quick start

```sh
nix develop --command python3 scripts/gruntz/init/toolchain.py   # once
nix develop --command python3 configure.py                 # manifest -> build.ninja
nix develop --command ninja                                # build (incremental)
nix develop --command gruntz build           # all of the above + match summary
```

``gruntz build`` is the one-command front door (configure -> ninja ->
objdiff report -> summary). Pass ninja args after `--`, e.g.
`gruntz build -- -j8`.

## Semantic navigation — `gruntz sema`

Source/target navigation for matchers & classifiers lives under one discoverable
group (`gruntz sema -h` is self-teaching — one usage example per subcommand).
Every subcommand is a **thin delegation** to an existing `gruntz.analysis` /
`gruntz.match` module (each still runnable as `python -m gruntz.<...>`); nothing
here re-implements analysis. **Semantic questions go here — grep is lexical-only.**

Every `gruntz sema` invocation is appended to **`build/gruntz_sema.log`** (usage
analysis → tool improvements). Metadata first, the shell-quoted command after the
`: ` so it copies straight out:
`[2026-07-04][19:33:36][0]: gruntz sema xref 0x00080850 --raw` (fields: date,
time, return code).

```sh
gruntz sema disasm 0x0008c750        # TARGET (retail) disasm+relocs; --target explicit
gruntz sema disasm 0x0008c750 --base # BASE: your compiled fn from build/objdiff/base/<unit>.obj
gruntz sema disasm 0x0008c750 --rich # BASE asm interleaved with /Z7 source lines (implies --base)
gruntz sema disasm 0x0008c750 --rich --lite  # same, but bare asm (drops the addr/byte columns)
gruntz sema disasm 0x0008c750 --diff # base-vs-target asm diff (addresses masked; rc=1 if differs)
gruntz sema disasm 0x0008c750 --lite # asm only - no addresses/bytes/reloc blocks
gruntz sema xref 0x00080850          # who calls this fn (retail call/jmp graph)  [--callees --raw]
gruntz sema symbol CGruntzApp        # fuzzy workspace-symbol search (clangd)
gruntz sema def|refs|hover F L [C]   # go-to-def / all-refs (USR-exact) / type at point (clangd)
gruntz sema rename F L [C] NEW [--dry-run]   # tree-wide, USR-keyed rename (clangd; matching-neutral)
gruntz sema rva 0x00080850           # address dossier: src claim + lib row + Ghidra fn + match %
gruntz sema class CImage             # vtable slots tagged new/override/inherited + hierarchy
gruntz sema match cplay | 0x..       # per-function/unit match % (from report.json)
gruntz sema disasm 0x00080850        # retail disasm + relocs (dump_target)
gruntz sema strings 0x00080850       # string set of a fn;  --find TEXT for the reverse lookup
```

**`disasm --rich`** interleaves the BASE disassembly with the C++ source lines it
came from, so you can see which statements survive `/O2` and which instruction(s)
each produced (source line flush-left, its instructions indented — a homm2-style
source↔asm view). It is BASE-only (retail carries no line info, so `--rich` implies
`--base` and rejects `--target`/`--diff`) and composes with `--lite` (source lines +
bare asm). The line data comes from a `/Z7` debug object of the same TU
(`build/debug/<unit>.obj`, the codegen-neutral CodeView build `harvest_locals.py`
also uses) — built on demand and cached on source mtime. MSVC 5.0 does **not** emit
modern C13 line tables; it uses classic COFF line numbers (`.text` section-header
`PointerToLinenumbers` + 6-byte `IMAGE_LINENUMBER` records) whose stored value is
relative to the function's `.bf` begin line, so `codeview.parse_lines()` recovers
`source_line = bf_line + stored`. Functions from vendored (non-`src/`) TUs, or when
the `/Z7` compile is unavailable, degrade to bare asm.

`xref`/`class`/`disasm`/`strings`/`rva` read the retail EXE + generated exports
(no clangd needed); the clangd-backed ones (`symbol`/`def`/`refs`/
`hover`/`rename`) need `build/clangd/compile_commands.json` (`gruntz clangd`) and
warm on first use (`rename` waits for the background index so cross-TU edits are
complete). The harness LSP tool covers def/refs/hover/symbol/calls but **not**
rename — that is why `sema rename` exists.

The harness **LSP** tool is powered by the official clangd plugin; a user enables
it once (agent-side install is not possible):

```
/plugin install clangd@claude-plugins-official
```

It reads the root `.clangd` (`CompilationDatabase: build/clangd`), so bare
`clangd` resolves the generated compile DB from the repo root with no extra flags
(verify: `clangd --check=src/<any>.cpp` prints `Loaded compilation database from
.../build/clangd/compile_commands.json`).

## Formatting — the Rust-like house style

The reconstructed C++ is auto-formatted with **clang-format** (from the Nix dev
shell) to read as close to Rust as the language allows: 4-space indent, 100-col
lines, attached braces *including on function definitions* (`int f() {`), `&`/`*`
bound to the type (`int* p`), a hanging-close (BlockIndent) wrap with function
*declaration* params one-per-line (call args and data arrays stay bin-packed, so
GUID/byte tables don't explode), and braces on every control body. The full
config — and the deliberate decompile-specific deviations — lives in the root
**`.clang-format`**.

```sh
gruntz format          # rewrite src/ + include/ in place (~0.3s for the tree)
gruntz format --check  # CI gate: no writes, non-zero exit if anything drifts
```

Formatting is **whitespace-only ⇒ matching-neutral**: it never changes the COFF
bytes objdiff compares (the one parser-visible case, `> >` vs `>>` for MSVC 5.0,
is pinned by `Standard: c++03`).

**You normally never run it by hand.** A repo-tracked **pre-commit hook**
(`.githooks/pre-commit`) formats staged `src/`+`include/` files automatically on
each commit; the dev shell enables it on entry via
`git config core.hooksPath .githooks` (idempotent; shared across worktrees).
Outside the Nix shell (no `clang-format` on PATH) the hook skips with a notice
rather than blocking the commit.

Two deliberate deviations from pure rustfmt, because this is a decompile:
- **Comment text is never reflowed** (`ReflowComments: Never`) — the ASCII
  "carcass" diagrams and `// +0xNN` field-offset tables map source to
  disassembly and must not be rewrapped. Trailing comments *are* column-aligned
  (`AlignTrailingComments: Always`) so those offset columns stay tidy after the
  surrounding code is reflowed.
- **Includes are never reordered** (`SortIncludes: Never`). Include order here is
  hand-tuned and interleaved with explanatory comments.

**Vendored code is never formatted.** `vendor/` (e.g. `vendor/zlib-1.0.4/`) must
stay byte-for-byte as shipped — it is part of the matching surface. It sits
outside the `src/`+`include/` roots that `gruntz format` and the hook touch, and
is independently guarded by `vendor/.clang-format` (`DisableFormat: true`), so
even an editor's format-on-save leaves it alone.

## The manifest: `config/units.toml` (single source of truth)

Per **translation unit** (per-TU). This is the counterpart to
`build/gen/symbol_names.csv`, which is per-**function** (`rva,name,unit`). Every
`unit` in the manifest must line up with a `unit` value used in
`symbol_names.csv` so the recompiled base obj pairs with the delinked target obj
of the same name.

```toml
[build]
compiler = "msvc5.0"                # -> objdiff scratch
platform = "win32"

[flags]                             # named flag profiles (full flag sets)
base = ["/nologo", "/c", "/O2", "/MT"]         # the locked global default
eh   = ["/nologo", "/c", "/O2", "/MT", "/GX"]  # + C++ exception-handling frame
mfc  = ["/nologo", "/c", "/O1", "/MT", "/GX"]  # MFC-derived /O1 (favor size) + /GX

[[unit]]
unit   = "adler32"                  # stem; obj is <unit>.obj, target <unit>.c.obj
source = "vendor/zlib-1.0.4/adler32.c"
flags  = "base"                     # required: names a [flags] profile
```

`configure.py` validates the manifest (required `unit`/`source`/`flags`, unique
units, a known `flags` profile) and resolves each unit's flags from the named
`[flags]` profile.

### The locked `base` flags

`cl /c /O2 /MT` (cdecl). Calibrated against the zlib TUs (see
`docs/zlib-matching.md`):

- `/O2` already forces function-level COMDAT packaging (so no `/Gy` needed).
- default struct packing is `/Zp8`, which matches (so no `/Zp` override).
- `/GF` has no observable effect on these TUs (so it is left off).

### Flag profiles (`[flags]`)

There is no implicit global flag set: every `[[unit]]` names a `[flags]` profile
explicitly (`flags = "base"`). The profiles are the full flag sets. Most TUs use
`base`; the only deviations so far are a C++ exception-handling frame (`eh` =
`base` + `/GX`) and, for MFC-derived code, optimizing for size (`mfc` = `/O1` +
`/GX`). Add a new profile to `[flags]` when a future TU needs a combination not
yet covered. (`base`, the first profile, doubles as `build.ninja`'s `$cflags`
default — a generation detail; the manifest still names it on every unit.)

## The `cl` rule (wine compiler wrapper)

`configure.py` emits, verbatim:

```ninja
rule cl
  command = python3 scripts/gruntz/build/cc_wrap.py --out $out --src $in -- $cflags
  description = cl $unit
```

`scripts/gruntz/build/cc_wrap.py` is the Linux->Wine bridge. For each TU it:

1. resolves `CL.EXE` under `$MSVC_DIR/bin` (case-insensitive) and checks `wine`,
2. keeps a persistent `wineserver -p` alive (so `ninja -j` parallelism does not
   pay a cold wineserver start per object),
3. translates the source + output paths with `winepath -w`,
4. runs `wine cl.exe <flags> /Fo<obj.w> <src.w>` in the obj's directory,
5. treats **"the `.obj` exists"** as the success signal (Wine spews unrelated
   driver/EGL noise and can return a non-cl exit code), exiting non-zero so
   ninja sees a real failure otherwise.

`/Fo<obj>` makes cl write the object exactly where ninja declared its output;
that path choice does not affect the emitted `.text` (objdiff compares function
code, not the COFF header timestamp, which varies run to run).

## The two graph phases

### Phase 1 — compile -> base `.obj` (IMPLEMENTED)

Each manifest unit's `source` compiles to `build/objdiff/base/<unit>.obj` via
the `cl` rule. This is the base side fed to objdiff. Native incremental: edit a
source (or its `RVA()`/`DATA()` annotations), or add a unit, and ninja rebuilds
only what changed (the label map regenerates from `src/`).

### Phase 2 — link -> candidate `.EXE` (IMPLEMENTED, opt-in)

`gruntz link` (or `ninja candidate`) links every base `<unit>.obj` into
`build/exe/GRUNTZ.candidate.EXE` + `.map` using the genuine VC5 `link.exe`
(version **5.10.7303** — the linker that built retail GRUNTZ.EXE) under wine. It
is **opt-in** (not in the default `all` target) so a normal `gruntz build` is
unaffected.

- `configure.py:emit_link_phase` emits the `link` rule; it runs
  `scripts/gruntz/build/link.py`, which feeds the obj list + flags through a
  **response file** (`@…objs.rsp`) — VC5 `link` has a short argv limit under wine.
- The reconstruction is **partial**, so link.py passes **`/FORCE`** and the EXE is
  **not runnable**. Layout study uses `/OPT:NOREF /OPT:NOICF` to keep every COMDAT
  in the map. The deliverable is the **`.map`** (each function's link-assigned RVA
  + source object).
- link.exe statically imports **`MSDIS100.DLL`** (VC5 disassembler, only used by
  `/dump /disasm`), which the toolchain omits, so it would not load under wine.
  `scripts/gruntz/build/msdis_stub.py` makes it resolvable (a real sourced DLL if
  present, else a generated export-only stub — link output is identical either way)
  and installs it into the wine prefix's 32-bit system dir.

This is the tool behind **`docs/link-order-investigation.md`**: the candidate map
cross-referenced with retail RVAs recovers the build order (intra-TU order =
source-definition order; cross-TU order = object link order). `gruntz link
--analyze` runs `scripts/gruntz/analysis/link_order.py` to print that report.
Whole-binary byte-verification against retail is a later step (needs fuller
reconstruction + the matched link order).

## The target (delink) half

Unchanged in spirit, just orchestrated by ninja. The `delink` rule runs
`scripts/gruntz/build/delink.py`, which:

1. runs `scripts/gruntz/build/synth_pdb.py` (overlay `build/gen/symbol_names.csv` onto the
   Ghidra `functions.csv`/`symbols.csv` -> a fabricated `gruntz_named.pdb`,
   bucket-shift 16 so un-named functions group into `seg_NNNN.cpp`),
2. runs `vostok-delinker` on `build/exe/GRUNTZ.EXE` (the stable retail copy)
   -> `build/delink/named/`,
3. collects the in-scope `<unit>.c.obj` into `build/objdiff/target/`.

`synth_pdb.py` (under `scripts/gruntz/build/`) is kept. The whole one-time local
setup runs in `gruntz init`: a stable retail copy at `build/exe/GRUNTZ.EXE`, and
the heavy Ghidra DB build + `functions.csv`/`symbols.csv` export (import +
auto-analyze GRUNTZ.EXE, then `apply.py`/`export.py`). The FID library labels are
tracked
(`config/library_labels.csv`, so they survive `git clean`); regenerate them with
`python -m gruntz.analysis.fid_generate`.

The delink rule's declared outputs are the per-unit `build/objdiff/target/<unit>.c.obj`
(one command, multiple outputs); its inputs are the EXE + the two Ghidra CSVs +
`build/gen/symbol_names.csv`.

### Ghidra enrichment metadata (apply.py inputs)

Beyond names, the Ghidra DB is enriched from generated, source-derived metadata so
nothing important lives only in the `.gpr` blob — it is all reproducible:

- `build/gen/functions.json` (`labels.py`): per-RVA **signatures** — class, return
  type, calling convention, and named parameters. Derived by joining the IR
  rva-map with `llvm-undname` (authoritative return/cc/class/param-types) and the
  clang AST (parameter *names*). `apply.py` applies these as typed Ghidra
  prototypes (+ a struct\* `this`).
- `build/gen/globals.json` (`labels.py`): per-RVA **declared global type** for each
  named global (the `DATA()`-bound `extern`'s C/C++ type). `apply.py` lays typed
  data at the address so the global decodes as its real type (e.g. `g_buteMgr :
  CButeMgr`) instead of raw bytes.
- `build/gen/locals.json` (`harvest_locals.py` + `codeview.py`): per-RVA **named
  local variables** for byte-exact functions only. Each src TU is compiled a second
  time with `cl /Z7` (codegen-neutral — the `.text` is byte-identical to the base
  obj, so the debug objs stay OUT of the matching graph, in `build/debug/`), and the
  old-format MSVC 5.0 CodeView (`.debug$S`) is read for frame-relative locals.
  `apply.py` injects them as named stack variables (they surface in the on-demand
  decompiler). The harvest runs before every `apply.py` (in `_ghidra_metadata_apply`).
- `build/gen/structs.json` + `build/gen/enums.json` (`ghidra_metadata_generate.py`):
  clang record layouts / enums over `src/` + `src/Stub/types/`, defined in the DTM;
  each struct is applied as the `this` type on its class's methods.

### Name precedence (src wins) and the apply report

`apply.py` layers names in a fixed order so the outcome is deterministic and the
`src/` labels are the SOURCE OF TRUTH at every RVA they claim:

1. **FID library labels** (`config/library_labels.csv`; HIGH/MED/AMBIG only — LOW
   rows are deliberately skipped as noise) name only the RVAs `src/` does **not**
   claim. A FID row at an `src`-claimed RVA is skipped (counted as `src-claimed
   skipped`): FID's AMBIG collisions — `??0CMetaFileDC@@` at a real ctor,
   `??_G__non_rtti_object@@` at a real scalar-deleting dtor, `??1CFile@@` at
   `CFileIO`'s dtor, `?GetStatus@CFile@@` at a global — must never win (before this
   was enforced at the source layer, FID applied first and layer 3 only *partially*
   undid it: a `CFileIO::~CFile` Frankenstein leaf, or a global wrongly nested as
   `CFile::GetFileTimeInfo`).
2. **`symbol_names.csv`** (from `src/` `RVA()`/`DATA()`) names + demangles every RVA
   `src/` claims — so `src` beats both FID and Ghidra's own analysis/demangler.
3. **`functions.json`** overlays the readable leaf + class namespace + typed
   prototype; **`locals.json`** adds stack-local names for byte-exact functions only.
4. **`user_annotations.json`** (human edits, `USER_DEFINED`) is re-applied LAST and
   wins over everything. The FID and `symbol_names` layers additionally skip any RVA
   already carrying a `USER_DEFINED` name (`human skipped`), so a human rename is
   never demoted or clobbered mid-run — even a deliberately mangled-looking one.

A re-run is a fixed point: the DB does not change and names do not flap (only a
small set of already-correct names is re-asserted, to the identical value). Every
layer's applied / skipped / conflict tally — incl. `src-claimed skipped`, `human
skipped`, and the byte-exact locals coverage (`locals.json sets` / `reached` /
`human slots preserved`) — is written to
`build/ghidra-named/exports/enrichment_apply_report.txt`.

### Round-trip: capturing human edits (`gruntz capture`)

The generated enrichment is the forward direction (`src/` → Ghidra). The back
direction persists HUMAN work so it survives a clean rebuild (which wipes the
`.gpr`). After renaming functions, writing comments, or naming/typing stack locals
in the Ghidra GUI (and saving), run **`gruntz capture`**: `export_user.py` extracts
exactly those human edits into the TRACKED **`config/user_annotations.json`**, which
`apply.py` re-applies LAST on every refresh/init.

Human vs generated is told apart by provenance: `apply.py` applies everything it
generates as `SourceType.ANALYSIS` and human edits use `SourceType.USER_DEFINED`
(the GUI's default), so a `USER_DEFINED` function symbol or stack variable is a human
edit. Comments have no `SourceType`, so `apply.py` snapshots the generated comments
to `build/gen/applied_comments.json` and the capture takes the set difference. The
generated sections skip a function/slot already owned by a `USER_DEFINED` name, so a
refresh never clobbers an un-captured edit. Commit `config/user_annotations.json` to
share/persist the edits.

### What triggers a re-delink (incremental label map)

The label map is built **per TU**: `gen_labels_one` runs `labels.py` on one
`src/<unit>.cpp` (the expensive clang-IR pass) → a fragment
`build/gen/labels/<unit>.csv`; a cheap `merge_labels` concatenates the fragments
→ `build/gen/symbol_names.csv` (re-applying the cross-TU duplicate-RVA guard +
DATA dedup). Both write **only when the content changed** (leaving the mtime
untouched otherwise) and both edges carry `restat`, so an edit that does not
change the labels stops the cascade right there.

The full chain a single `gruntz build` runs after a `src/` edit:

```
edit src/<unit>.cpp
  └─ gruntz build → ninja (build.ninja self-regenerates via its generator edge):
       cl             rule  cc_wrap.py  → build/objdiff/base/<unit>.obj   (recompile via wine cl)
       gen_labels_one rule  labels.py   → build/gen/labels/<unit>.csv     (ONLY this unit's IR)
       merge_labels   rule  labels.py   → build/gen/symbol_names.csv      (concat + dup-guard; restat)
       delink         rule  delink.py   ← symbol_names.csv (restat: re-runs only if it changed)
                            synth_pdb.py + vostok-delinker → build/objdiff/target/<unit>.c.obj
       report         rule  objdiff-cli → build/objdiff/report.json       (objs changed)
```

Two regimes:

- **Pure code edit** (no `RVA()`/symbol change): only `cl` + `gen_labels_one`
  run. The fragment is byte-identical → write-if-changed leaves its mtime →
  `restat` stops `merge_labels`, so the label map, the delink, and every target
  obj are untouched. Net work: one recompiled base obj + a fresh `report.json`.
- **Symbol change** (add/rename a function, change an `RVA()`): the unit's
  fragment changes → `merge_labels` rewrites `symbol_names.csv` → the delink
  re-runs and the unit's `<unit>.c.obj` updates.

This keeps a single-TU rebuild ~1s instead of re-emitting clang IR for all ~23
TUs and re-delinking the whole EXE on every edit. `gruntz build` itself is thin:
it ensures the wineserver is up (kept alive across builds, not killed each time),
runs `ninja` (which builds the objs AND `report.json` in-graph), and runs the
non-fatal feedback tail (README score block + regression check) **only when
`report.json` actually moved** — a no-op build returns in ~0.15s.

**Build timing.** Every `gruntz build` invocation records its wall-clock — printed
as `[gruntz] build timing: total Ns (ninja Xs, gates Ys) [mode]` and appended to
`build/gen/build_times.tsv` (gitignored, per-worktree; columns
`timestamp worktree mode ninja_s gates_s total_s`). `mode` is `noop` (nothing
rebuilt), `fast` (`--fast`, ninja + summary only), or `full` (ninja + the whole
gate tail). It splits the two costs that matter: **ninja** (the incremental
recompile/delink — usually seconds) vs **gates** (the structural-invariant tail:
`verify_*`, `structs` regen, `class_sizes`, the `vtable_*` audits, `view_debt` — the
dominant cost of a full build, which is exactly why matchers iterate with `--fast`
and pay the gate tail once before committing). Pool the per-worktree TSVs to compare
how long worker builds take.

## Pairing (objdiff)

`build/objdiff/objdiff.json` (written by `configure.py:emit_objdiff`) pairs, per unit:

- base: `./base/<unit>.obj` (cl `/O2 /MT vendor/zlib-1.0.4/<unit>.c`)
- target: `./target/<unit>.c.obj` (delinked, named per `symbol_names.csv`)

Symbols are pre-named on both sides (cdecl `_<name>`), so objdiff pairs them
**by symbol name** with no `symbol_mappings` overlay. A unit whose target obj
does not exist yet is paired against an empty `dummy.obj` so it still lists at
0% (`build_base`/`build_target` are `false`: ninja, not objdiff, builds objs).

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, and a
   `flags` profile — `base` unless the TU needs `/GX`/`/O1`);
2. `#include "../rva.h"` and annotate **each** matched function with an `RVA()`
   macro (`src/rva.h`) directly above the definition, after the description. A
   real example from `src/Gruntz/SBI_RectOnly.cpp`:

   ```cpp
   // ---------------------------------------------------------------------------
   // CSBI_RectOnly::CSBI_RectOnly()
   // Inlines the CStatusBarItem base ctor (the dead m_8=0 store is elided),
   // stores its own vptr, then sets m_8 = 1.
   RVA(0x101fa0, 0x1b)   // retail .text RVA (VA = 0x400000 + rva), byte size
   CSBI_RectOnly::CSBI_RectOnly()
   {
       m_8 = 1;
   }
   ```

   The macros (`src/rva.h`, compiled to nothing under MSVC 5.0 — it predates
   `__attribute__` and C99 variadic macros, so each macro is FIXED-arity):
   - `RVA(addr, size)` / `RVAU(addr)` — a matched function (sized / unsized);
   - `SYMBOL(mangled)` — an explicit mangled-name override for the rare case
     clang's MS mangling differs from VC5's;
   - `DATA(addr)` — on an `extern` decl of a matched global (the DATA symbol it
     is referenced through);
   - `// @rva-symbol: <mangled> <rva> [<size>]` — a comment for a thunk with no
     source body (a `??_G` deleting dtor) that can't hold an attribute.

   `labels.py` reads `RVA`/`SYMBOL` from **LLVM IR** (`@llvm.global.annotations`
   pairs the mangled symbol DIRECTLY with the annotation — no positional join);
   `DATA` from the clang AST (an `extern`'s annotation is dropped from IR). The
   label map regenerates from these annotations — never hand-edit the CSV. (The
   vendored zlib C TUs keep PRISTINE source — no labels in it; their rva→symbol
   map is the static `config/zlib_labels.csv`, emitted directly. See
   `docs/zlib-matching.md`.)
3. `gruntz build` (configure -> compile -> labels -> delink -> objdiff).

## Generated vs. tracked

Tracked: `config/units.toml`, the `src/` sources (incl. their `RVA()`/`DATA()`
annotation macros and `src/rva.h`), `configure.py`, and the whole `scripts/gruntz/`
package (the pipeline `{build,ghidra,init}/`, the match tooling `match/`, and the
analysis tools `analysis/`).

Generated (git-ignored): `build/gen/symbol_names.csv` (from `src/` `RVA()`),
`build/gen/functions.json` + `build/gen/locals.json` (Ghidra enrichment metadata),
`build.ninja`, `.ninja_log`/`.ninja_deps`, and everything under `build/` (base
objs, `/Z7` debug objs, delinked target objs, synth PDB, the clangd compdb, the
wine prefix, the Ghidra DB + exports, objdiff project + report) — see the table below.

## The `build/` directory

ALL local, imperative state lives under `build/` (every subdir is **git-ignored**;
`gruntz clean` nukes the lot, `gruntz init` rebuilds it). The retail EXE and the
toolchain come from the flake; nothing here is tracked.

| subdir | what it is | generated by | role in the pipeline | size class |
|---|---|---|---|---|
| `gen/` | the generated metadata: `symbol_names.csv` (mangled name ↔ RVA, from `src/` `RVA()`/`DATA()` via LLVM IR) plus `functions.json`/`locals.json`/`structs.json`/`enums.json`/`globals.json`/`applied_comments.json` (clang record layouts + harvested locals fed to Ghidra enrich) | `gruntz labels` / `structs` (the ninja `gen_labels` rule + `ghidra_metadata_generate.py`) | the delink's name map + Ghidra-enrich inputs | ~1 MB |
| `objdiff/` | the diff project: `base/<unit>.obj` (wine `cl` output), `target/<unit>.c.obj` (delinked), `objdiff.json` (pairing), `report.json` (the scored result) | the ninja `cl`+`delink` rules + `objdiff-cli report generate` | the actual base-vs-target comparison `gruntz status` reads | ~2 MB |
| `delink/` | `named/` — raw per-symbol COFF objects straight out of vostok-delinker, before they are collected into `objdiff/target/` | the ninja `delink` rule (`delink.py`) | intermediate delinker output | ~3 MB |
| `pdb/` | the synthesized fake PDB (`gruntz_named.pdb` + its `.yaml`) — Public + Procedure + Data symbols, no section contribs | `synth_pdb.py` (inside the `delink` rule) | feeds vostok-delinker (it needs symbol+length records, which the retail EXE lacks) | ~12 MB |
| `exe/` | `GRUNTZ.EXE` — a stable-named copy of `$GRUNTZ_EXE` | `gruntz init`/`build` (`_ensure_retail_copy`) | the delink input **and** the Ghidra import target | ~2.5 MB |
| `clangd/` | the clangd compile DB (`compile_commands.json`) + the derived `func_fingerprints.tsv` cache | `gruntz clangd` (`init/clangd.py`) + `gruntz.match.fingerprints` | editor/LSP navigation; per-function regression fingerprints | ~4 MB |
| `debug/` | the `/Z7`-compiled `<unit>.obj`s (debug info embedded) used only to harvest local-variable names/types | the ninja debug-compile path (`harvest_locals`) | source of `gen/locals.json` (Ghidra enrich) | ~1.5 MB |
| `ghidra-named/` | the named Ghidra project (`gruntz.{gpr,rep}`): GRUNTZ.EXE imported, auto-analyzed, RTTI/FLIRT/leaked-name labeled, then enriched | `gruntz init` (PyGhidra import+analyze) + `ghidra-refresh` | the comprehension DB; source of the function/symbol exports | ~42 MB |
| `ghidra-enrich/` | `exports/functions.csv` + `symbols.csv` re-dumped from the enriched DB | `apply.py`/`export.py` under PyGhidra (`ghidra-refresh`) | the two CSVs the delink consumes (function boundaries + names) | ~2 MB |
| `wineprefix/` | the Wine prefix with the MSVC 5.0 toolchain registered | `gruntz init` (`init/toolchain.py`); `$WINEPREFIX` | hosts every `wine cl` invocation (the base-obj compiles) | ~561 MB |

(Also transient: `build/fid/` — scratch for `gruntz.analysis.fid_generate`'s
library-label regen — and root-level `build.ninja`/`.ninja_*`.)

## Current status

zlib 1.0.4 plus a growing set of engine/Gruntz reconstructions build through ninja,
and the generated label map drives the delink. **Run `gruntz status` for the live
match %** (kept out of this doc so it can't go stale). Part of the exact-vs-fuzzy
gap is a reloc-naming difference — delinked relocs reference Ghidra `DAT_*/FUN_*`
while the base references the real symbols, not a code difference — and the rest
are functions still in progress.
