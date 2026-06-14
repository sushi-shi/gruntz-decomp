# The matching build system (manifest -> ninja -> objdiff)

The base/recompile side of the matching loop is a **native incremental ninja
build** generated from a single manifest. It replaces the old
recompile-everything front-end (`rebuild.py`, now removed) and folds the objdiff
project generation into `configure.py`. `gruntz build` is the one entry point.

```
config/units.toml  (per-TU manifest: unit, source, status, optional cflags)
        |  python3 configure.py
        v
build.ninja  +  compile_commands.json  +  build/objdiff/objdiff.json
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
cflags   = ["/nologo", "/c", "/O2", "/MT"]   # the locked global flags
compiler = "msvc5.0"                          # -> objdiff scratch
platform = "win32"

[[unit]]
unit   = "adler32"                  # stem; obj is <unit>.obj, target <unit>.c.obj
source = "vendor/zlib-1.0.4/adler32.c"
status = "matched"                  # "matched" | "wip"
# cflags = [...]                    # OPTIONAL per-unit override (defaults to [build].cflags)
```

`configure.py` validates the manifest (required `unit`/`source`, unique units),
defaults `status` to `wip` and `cflags` to the global locked set.

### Locked global flags

`cl /c /O2 /MT` (cdecl). Calibrated against the zlib TUs (see
`docs/zlib-matching.md`):

- `/O2` already forces function-level COMDAT packaging (so no `/Gy` needed).
- default struct packing is `/Zp8`, which matches (so no `/Zp` override).
- `/GF` has no observable effect on these TUs (so it is left off).

The per-unit `cflags` override mechanism exists for **future** TUs that turn out
to need different flags; today every unit inherits the global set.

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
source (or its `// @address:` annotations), or add a unit, and ninja rebuilds
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
2. runs `vostok-delinker` on the **break_reloc_cycle'd**
   `build/exe/GRUNTZ.delinkable.EXE` -> `build/delink/named/`,
3. collects the in-scope `<unit>.c.obj` into `build/objdiff/target/`.

`synth_pdb.py` and `reloc.py` (under `scripts/gruntz/build/`) are kept; the EXE
prep (`reloc.py` -> `GRUNTZ.delinkable.EXE`) and the Ghidra `functions.csv`/
`symbols.csv` export are one-time setup steps **upstream** of this build (not yet
wired into `gruntz`; see `gruntz ghidra-refresh`).

The delink rule's declared outputs are the 9 `build/objdiff/target/<unit>.c.obj`
(one command, multiple outputs); its inputs are the EXE + the two Ghidra CSVs +
`build/gen/symbol_names.csv`, so changing the names map re-delinks.

## Pairing (objdiff)

`build/objdiff/objdiff.json` (written by `configure.py:emit_objdiff`) pairs, per unit:

- base: `./base/<unit>.obj` (cl `/O2 /MT vendor/zlib-1.0.4/<unit>.c`)
- target: `./target/<unit>.c.obj` (delinked, named per `symbol_names.csv`)

Symbols are pre-named on both sides (cdecl `_<name>`), so objdiff pairs them
**by symbol name** with no `symbol_mappings` overlay. A unit whose target obj
does not exist yet is paired against an empty `dummy.obj` so it still lists at
0% (`build_base`/`build_target` are `false`: ninja, not objdiff, builds objs).

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, `status`);
2. annotate each matched function in `src/` with `// @address: 0x<rva>` (the
   label map regenerates from these — no hand-edited CSV);
3. `gruntz build` (configure -> compile -> labels -> delink -> objdiff).

## Generated vs. tracked

Tracked: `config/units.toml`, the `src/` sources (incl. their `// @address:`
annotations), `configure.py`, and the pipeline package
`scripts/gruntz/{build,ghidra,init}/`.

Generated (git-ignored): `build/gen/symbol_names.csv` (from `src/` `@address`),
`build.ninja`, `compile_commands.json`, `.ninja_log`/`.ninja_deps`, and everything
under `build/` (base objs, delinked target objs, synth PDB, the wine prefix,
objdiff project + report).

## Current status

zlib 1.0.4 plus a growing set of engine/Gruntz reconstructions build through ninja,
and the generated label map drives the delink. **Run `gruntz status` for the live
match %** (kept out of this doc so it can't go stale). Part of the exact-vs-fuzzy
gap is a reloc-naming difference — delinked relocs reference Ghidra `DAT_*/FUN_*`
while the base references the real symbols, not a code difference — and the rest
are functions still in progress.
