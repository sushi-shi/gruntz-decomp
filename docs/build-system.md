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
`build.ninja`, `.ninja_log`/`.ninja_deps`, and everything under `build/` (base
objs, delinked target objs, synth PDB, the clangd compdb, the wine prefix, the
Ghidra DB + exports, objdiff project + report) — see the table below.

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
