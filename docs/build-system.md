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

Everything runs inside `nix develop .#build` (exports `MSVC_DIR`, `DXSDK_DIR`,
`WINEPREFIX`, and now `ninja` on PATH). Initialise the Wine prefix once with
`scripts/gruntz/init/toolchain.py` if it has not been set up.

## Quick start

```sh
nix develop .#build --command python3 scripts/gruntz/init/toolchain.py   # once
nix develop .#build --command python3 configure.py                 # manifest -> build.ninja
nix develop .#build --command ninja                                # build (incremental)
nix develop .#build --command gruntz build           # all of the above + match summary
```

``gruntz build`` is the one-command front door (configure -> ninja ->
objdiff report -> summary). Pass ninja args after `--`, e.g.
`gruntz build -- -j8`.

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

### Phase 2 — link -> candidate `.EXE` (DEFERRED)

**Not implemented.** Whole-binary verification (link every base `.obj` into a
candidate `GRUNTZ.EXE` and byte-compare against the retail binary) is a marked
placeholder in `configure.py:emit_link_phase` and a `# === PHASE 2 (DEFERRED)
===` comment block at the bottom of the generated `build.ninja`. When
implemented it will add:

- a `link` rule running `wine link.exe` through a **response file** (`@objs.rsp`)
  — VC5's `link` has a short command-line limit under wine, so the obj list +
  flags must go through a response file, not argv;
- link flags pinned by matching: `/OPT:REF` `/OPT:ICF` plus the exact link
  **order** (COMDAT order, not source-definition order — see
  `docs/zlib-matching.md`);
- inputs = every base `<unit>.obj`; output = `build/exe/GRUNTZ.candidate.EXE`
  plus a verify step diffing it against the retail EXE.

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
`scripts/analysis/fid_generate.py`.

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
- `build/gen/locals.json` (`harvest_locals.py` + `codeview.py`): per-RVA **named
  local variables** for byte-exact functions only. Each src TU is compiled a second
  time with `cl /Z7` (codegen-neutral — the `.text` is byte-identical to the base
  obj, so the debug objs stay OUT of the matching graph, in `build/debug/`), and the
  old-format MSVC 5.0 CodeView (`.debug$S`) is read for frame-relative locals.
  `apply.py` injects them as named stack variables (they surface in the on-demand
  decompiler). The harvest runs before every `apply.py` (in `_ghidra_metadata_apply`).

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

### What triggers a re-delink

The delink is keyed on **`build/gen/symbol_names.csv`** (the EXE + Ghidra CSVs only
change at `gruntz init`). So in the inner loop, editing a `src/` `RVA()` macro is
what re-delinks: ninja regenerates the label map, which *is* the delink's input.
The full chain a single `gruntz build` runs:

```
edit src/<unit>.cpp  (add / change an  RVA(0x<rva>, 0x<size>))
  └─ gruntz build → configure.py (build.ninja) → ninja:
       cl         rule  cc_wrap.py   → build/objdiff/base/<unit>.obj   (recompile via wine cl)
       gen_labels rule  labels.py    → build/gen/symbol_names.csv      (rva → name → unit)
                                        ↑ a src RVA() changed ⇒ the label map regenerates (via LLVM IR)
       delink     rule  delink.py    ← symbol_names.csv is a declared input, so it re-runs:
                        synth_pdb.py    functions.csv + symbols.csv + symbol_names.csv
                                          → build/pdb/gruntz_named.pdb
                        vostok-delinker  GRUNTZ.EXE + the PDB → build/delink/named/
                        collect          → build/objdiff/target/<unit>.c.obj
       objdiff-cli report generate    → build/objdiff/report.json
```

In practice the delink re-runs on **every** `src/` edit: `gen_labels` rewrites
`symbol_names.csv` unconditionally (fresh mtime) and the delink rule has no
`restat` guard, so even a body-only tweak (identical labels) re-delinks. That's
left as-is on purpose — the delink is one command emitting all units' objs in a
single cheap pass (~one synth_pdb + one vostok-delinker over the EXE), the whole
loop is fast, and a write-if-different + `restat` skip isn't worth the complexity.

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
annotation macros and `src/rva.h`), `configure.py`, and the pipeline package
`scripts/gruntz/{build,ghidra,init}/`.

Generated (git-ignored): `build/gen/symbol_names.csv` (from `src/` `RVA()`),
`build/gen/functions.json` + `build/gen/locals.json` (Ghidra enrichment metadata),
`build.ninja`, `.ninja_log`/`.ninja_deps`, and everything under `build/` (base
objs, `/Z7` debug objs, delinked target objs, synth PDB, the clangd compdb, the
wine prefix, the Ghidra DB + exports, objdiff project + report).

## Current status

zlib 1.0.4 plus a growing set of engine/Gruntz reconstructions build through ninja,
and the generated label map drives the delink. **Run `gruntz status` for the live
match %** (kept out of this doc so it can't go stale). Part of the exact-vs-fuzzy
gap is a reloc-naming difference — delinked relocs reference Ghidra `DAT_*/FUN_*`
while the base references the real symbols, not a code difference — and the rest
are functions still in progress.
