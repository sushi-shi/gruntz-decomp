# The matching build system (manifest -> ninja -> objdiff)

The base/recompile side of the matching loop is a **native incremental ninja
build** generated from a single manifest. It replaces the old
`scripts/rebuild.py` recompile-everything loop (and folds in the standalone
`scripts/generate_objdiff_config.py`).

```
config/units.toml  (per-TU manifest: unit, source, status, optional cflags)
        |  python3 configure.py
        v
build.ninja  +  compile_commands.json  +  build/objdiff/objdiff.json
        |  ninja
        +-- PHASE 1 (compile): each unit -> build/objdiff/base/<unit>.obj
        |       via the `cl` rule -> scripts/cc_wrap.py -> `wine cl /c ... /Fo`
        +-- TARGET (delink): scripts/delink_target.py
        |       synth_pdb.py -> vostok-delinker -> build/objdiff/target/<unit>.c.obj
        v
objdiff-cli report generate -p build/objdiff -o build/objdiff/report.json
        |
        v
per-unit + roll-up match % (scripts/rebuild.py prints this; it is now a thin
wrapper around configure.py + ninja + objdiff)
```

Everything runs inside `nix develop .#build` (exports `MSVC_DIR`, `DXSDK_DIR`,
`WINEPREFIX`, and now `ninja` on PATH). Initialise the Wine prefix once with
`scripts/setup-toolchain.py` if it has not been set up.

## Quick start

```sh
nix develop .#build --command python3 scripts/setup-toolchain.py   # once
nix develop .#build --command python3 configure.py                 # manifest -> build.ninja
nix develop .#build --command ninja                                # build (incremental)
nix develop .#build --command python3 scripts/rebuild.py           # all of the above + match summary
```

`scripts/rebuild.py` is the one-command front door (configure -> ninja ->
objdiff report -> summary). Pass ninja args after `--`, e.g.
`python3 scripts/rebuild.py -- -j8`.

## The manifest: `config/units.toml` (single source of truth)

Per **translation unit** (per-TU). This is the counterpart to
`config/symbol_names.csv`, which is per-**function** (`rva,name,unit`). Every
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
  command = python3 scripts/cc_wrap.py --out $out --src $in -- $cflags
  description = cl $unit
```

`scripts/cc_wrap.py` is the Linux->Wine bridge. For each TU it:

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
source, change `config/symbol_names.csv`, or add a unit, and ninja rebuilds only
what changed.

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
`scripts/delink_target.py`, which:

1. runs `scripts/synth_pdb.py` (overlay `config/symbol_names.csv` onto the
   Ghidra `functions.csv`/`symbols.csv` -> a fabricated `gruntz_named.pdb`,
   bucket-shift 16 so un-named functions group into `seg_NNNN.cpp`),
2. runs `vostok-delinker` on the **break_reloc_cycle'd**
   `build/exe/GRUNTZ.delinkable.EXE` -> `build/delink/named/`,
3. collects the in-scope `<unit>.c.obj` into `build/objdiff/target/`.

`scripts/synth_pdb.py` and `scripts/break_reloc_cycle.py` are kept; the EXE prep
(`break_reloc_cycle.py` -> `GRUNTZ.delinkable.EXE`) is a one-time setup step
upstream of this build.

The delink rule's declared outputs are the 9 `build/objdiff/target/<unit>.c.obj`
(one command, multiple outputs); its inputs are the EXE + the two Ghidra CSVs +
`config/symbol_names.csv`, so changing the names map re-delinks.

## Pairing (objdiff)

`build/objdiff/objdiff.json` (written by `configure.py:emit_objdiff`, absorbed
from the old `generate_objdiff_config.py`) pairs, per unit:

- base: `./base/<unit>.obj` (cl `/O2 /MT vendor/zlib-1.0.4/<unit>.c`)
- target: `./target/<unit>.c.obj` (delinked, named per `symbol_names.csv`)

Symbols are pre-named on both sides (cdecl `_<name>`), so objdiff pairs them
**by symbol name** with no `symbol_mappings` overlay. A unit whose target obj
does not exist yet is paired against an empty `dummy.obj` so it still lists at
0% (`build_base`/`build_target` are `false`: ninja, not objdiff, builds objs).

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, `status`);
2. add its functions (`rva,name,unit`) to `config/symbol_names.csv`;
3. `python3 configure.py` then `ninja` (or just `python3 scripts/rebuild.py`).

## Generated vs. tracked

Tracked: `config/units.toml`, `config/symbol_names.csv`, `configure.py`,
`scripts/{cc_wrap,delink_target,synth_pdb,break_reloc_cycle,rebuild,ninja_syntax}.py`.

Generated (git-ignored): `build.ninja`, `compile_commands.json`,
`.ninja_log`/`.ninja_deps`, and everything under `build/` (base objs, delinked
target objs, synth PDB, objdiff project + report).

## Current status

The 9 matched zlib 1.0.4 TUs (`adler32 deflate trees inftrees infblock infcodes
inffast infutil zutil`) build through ninja and reproduce the prior match: the
42 named functions pair against the delinked target with adler32/zutil/infutil/
deflate at 100% exact code, and trees/inftrees/infcodes/infblock at lower
*exact* % but ~99-100% *fuzzy* (a reloc-naming gap — the delinked target's
relocs reference Ghidra `DAT_*/FUN_*` names while the base references the real
zlib symbols — not a code difference). Roll-up: 30/42 functions exact, 99.52%
fuzzy across the 9 named units.
