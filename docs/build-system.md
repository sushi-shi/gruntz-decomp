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
        |       via the `cl` rule -> scripts/gruntz/core/cc_wrap.py -> `wine cl /c ... /Fo`
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
Each subcommand's implementation is **one module in `scripts/gruntz/sema/`**
(browse that directory; `sema/__init__.py` holds the inventory table), running
**in one process** over the `gruntz/core` library (`pe`/`symbols`/`report` —
the EXE + symbol db loaded once per process). Child processes only for true
externals (llvm-objdump, the clangd server, wine cl) — sema never spawns
python. Shared engines (`vtable_scan`/`vtable_hierarchy`, `exe_map`,
`clangd_query`, `match/status`) are imported and called in-process. rc
convention: **0 answered, 1 answered-NO** (diff differs / not a virtual / no
hit), **2 error**. **Batch mode**: `gruntz sema -` reads newline-delimited
sema command lines from stdin and answers each against ONE loaded Context (N
queries, 1 parse — use it for investigation loops). `symbol`/`def` were
retired (0 uses in 9,771 logged calls; the harness LSP covers them).
**Semantic questions go here — grep is lexical-only.**

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
gruntz sema disasm 0x0008c750 --branches --diff  # the branch sequence --diff's masking HIDES
gruntz sema xref 0x00080850          # who calls this fn (retail call/jmp graph)  [--callees --raw]
gruntz sema refs|hover F L [C]       # all-refs (USR-exact) / type at point (clangd)
gruntz sema rename F L [C] NEW [--dry-run]   # tree-wide, USR-keyed rename (clangd; matching-neutral)
gruntz sema rva 0x00080850           # address dossier: src claim + lib row + Ghidra fn + match %
gruntz sema class CImage             # vtable slots tagged new/override/inherited + hierarchy
gruntz sema match cplay | 0x..       # per-function/unit match % (from report.json)
gruntz sema disasm 0x00080850        # retail disasm + relocs (dump_target)
gruntz sema strings 0x00080850       # string set of a fn;  --find TEXT for the reverse lookup
```

**`disasm --branches`** prints the ordered conditional-branch sequence with every
target named by **branch index**, plus each side's `ret` count; with `--diff` it prints
only the differing rows, classified SIGNEDNESS / POLARITY / OTHER / TOPOLOGY (rc=1 if
they differ). This is the view `--diff` *structurally cannot* give you: `--diff` masks
address operands so reloc-bound targets do not show as spurious diffs, and that masking
also hides intra-function branch **displacements** — a `je` to a different basic block
prints `je <tgt>` on both sides and compares equal. Naming targets by branch index makes
a uniform displacement shift compare equal while a genuine retarget does not, so the
masking stays (unmasking it would put a `+`/`-` on every branch of every function whose
sizes differ upstream). The comparison lives in `gruntz.core.branches`;
`python -m gruntz.audit.jcc_sieve` is the same comparison swept over the whole tree.
When `--diff` or `--blocks --diff` has nothing to show but the function is not 100%,
they now print a one-line pointer to it.

**`python -m gruntz.audit.eh_frame`** is the same idea for the `/GX` exception frame.
`/GX` is on project-wide, so cl 5.0 emits the registration-record prologue (`push -1` /
`push <handler>` / `mov fs:[0],esp`) in a function **iff** that function owns something
whose destructor must run during an unwind — a source fact, not a codegen preference.
The sieve classifies both sides of every scoring function and reports the presence
disagreements in both directions, plus (`--states`) the wider set where both sides are
framed but store a different NUMBER of `mov [esp+N],<n>` unwind states. Each row is
tagged by cause: a retail-only ctor/dtor COMDAT call inside the guarded region means the
object is the SAME and only cl's inline cut differs (a wall —
`docs/patterns/new-site-eh-states-are-a-called-base-ctor.md`), while no call difference
means one side really does own an object the other's source never declared. `--calibrate`
measures both signals against the functions objdiff already scores at 100.00%, which are
byte-identical and must agree.

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
  command = python3 scripts/gruntz/core/cc_wrap.py --out $out --src $in -- $cflags
  description = cl $unit
```

`scripts/gruntz/core/cc_wrap.py` is the Linux->Wine bridge. For each TU it:

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

Pass `gruntz link --res path/to/GRUNTZ.RES` when producing a runnable candidate
that needs dialogs, strings, and icons. Matching links omit resources.

- `configure.py:emit_link_phase` emits the `link` rule; it runs
  `scripts/gruntz/build/link.py`, which feeds the obj list + flags through a
  **response file** (`@…objs.rsp`) — VC5 `link` has a short argv limit under wine.
- **Libraries are linked** (see § below): the retail set resolves everything but the
  reconstruction backlog, so the punch list is **1 unresolved external**. `/FORCE`
  stays on to survive that one. Layout study uses `/OPT:NOREF /OPT:NOICF` to keep
  every COMDAT in the map, and `/FIXED:NO` to emit the `.reloc` retail has (purely
  additive — `.text`/`.rdata`/`.data` come out byte-identical either way).
  `--no-libs` restores the historical objects-only probe.
- The deliverable is the **`.map`** (each function's link-assigned RVA + source
  object) plus `…link.log` and `…unresolved.txt`, the drive-to-linkable worklist.
- link.exe statically imports **`MSDIS100.DLL`** (VC5 disassembler, only used by
  `/dump /disasm`), which the toolchain omits, so it would not load under wine.
  `scripts/gruntz/build/msdis_stub.py` makes it resolvable (a real sourced DLL if
  present, else a generated export-only stub — link output is identical either way)
  and installs it into the wine prefix's 32-bit system dir.

#### The library set (and the two libs we have to synthesise)

The objs already carry most of it: `cl /MT` writes `-defaultlib:LIBCMT` +
`-defaultlib:OLDNAMES` into `.drectve`, and MFC's headers add `nafxcw kernel32
user32 gdi32 comdlg32 winspool advapi32 shell32 comctl32` — which is 8 of the 10
Win32 DLLs in retail's import table. link.py simply does **not** pass
`/NODEFAULTLIB`, so those fire exactly as they did for the devs. Three groups
declare themselves nowhere and are named explicitly:

| group | libs | why it is not declared |
|---|---|---|
| game-only Win32 | `version winmm` | used by the game, requested by no header |
| DirectX 6 | `ddraw dsound dinput dplayx` + static `dxguid` | the DX SDK ships no `#pragma comment(lib)` |
| RAD SDKs | `mss32 smackw32` | **we do not have those SDKs** |

`scripts/gruntz/build/import_lib.py` rebuilds the two missing RAD import libs into
`build/lib/` from **retail's own import table** (`gruntz.core.pe.PE.imports`), whose
stored names (`_AIL_startup@0`) are already decorated exactly as the original import
lib produced them. It does this by generating a throwaway **stub DLL** of
`__declspec(dllexport) __stdcall` functions with the matching argument-byte counts
and keeping link.exe's `/IMPLIB:` — `LIB /DEF:` cannot express it, because it derives
the public symbol by prefixing an underscore (`__imp___AIL_startup@0`, one too many)
or, if you drop the underscore in the .def, writes the wrong hint/name string.
The **hints** are reproduced too: a hint is the name's index in the vendor DLL's
sorted export-name table, and retail's import table stores the vendor's values
(`_AIL_release_sequence_handle@4` = 126 of the real Miles DLL's ~196 exports), so
the stub pads its export list with `__cdecl` filler names that sort strictly
between the real decorated names until every real export sits at exactly its
retail index. The fillers never reach the image — nothing references them, so no
member of theirs is ever pulled (measured: thunk order byte-identical with and
without them) — and `_verify_hints` re-reads the produced archive's `.idata$6`
and fails the synthesis on any mismatch.

Result: the candidate EXE's import table has **the same 16 DLLs in retail's exact
descriptor order, the same imported-name set per DLL — 456 names, none missing,
none extra (`PE.imports` on both) — and all 449 named imports carrying retail's
hint values**. Getting the DLL order exact required naming
`nafxcw`/`libcmt` *first*, since 306 of the 456 names are referenced only by MFC/CRT
members; see `docs/linker-flags.md` § Libraries. Still open: the order *within* each
DLL — a resolution-history artifact of the linker's undefined-symbol worklist, not
a link-line property; the mechanism and its bounded evidence are in
`docs/patterns/idata-thunk-order-is-resolution-history.md`.

This is the tool behind **`docs/link-order-investigation.md`**: the candidate map
cross-referenced with retail RVAs recovers the build order (intra-TU order =
source-definition order; cross-TU order = object link order). `gruntz link
--analyze` runs `scripts/gruntz/audit/link_order.py` to print that report.
Whole-binary byte-verification against retail is a later step (needs fuller
reconstruction + the matched link order).

The link also carries a **`.rsrc`**: there is no `rc.exe` in the toolchain, so
`scripts/gruntz/build/rescomp.py` IS the resource compiler — it parses
`src/Gruntz/Gruntz.rc` (tracked, genuine rc grammar: all 57 authorable resources —
STRINGTABLEs, DIALOG/DIALOGEX, ACCELERATORS, VERSIONINFO, MFC DLGINIT, 91.5% of the
payload) plus the 18 carried ICON/CURSOR art blobs (`config/retail/rsrc/data/`, the
8.5% that has no text form) and writes the Win32 `.RES` container itself; `link.exe`
(via its built-in `cvtres`) turns it into the section. All 75 resources come out
byte-identical; the only differing bytes in the whole 123,260-byte section are the 75
`OffsetToData` fields, shifted by the section placement. `rescomp check` — a
normal-tier build gate — recompiles the `.rc` and byte-compares every payload against
the retail image, so the "this text produces those bytes" claim is re-proven on every
gated build. Only the art is still carried retail bytes; the manifest's `provenance`
column says so per row.

**`docs/link-section-census.md`** classifies every byte of every remaining
section delta (`python -m gruntz.audit.section_census [--bss] [--reloc]`). The
headline: retail is an `/INCREMENTAL:YES` link (its `.CRT$XC*` tables sit at the
exact padded offsets ours do), the incremental linker pads command-line objects by
+20% and library members by 0, and that single fact accounts for the whole `.data`
excess — modelled with `--engine-lib`, `.data` goes from +67,456 over to 22,304
under.

## The target (delink) half

Unchanged in spirit, just orchestrated by ninja. The `delink` rule runs
`scripts/gruntz/build/delink.py`, which:

1. runs `scripts/gruntz/build/synth_pdb.py` (overlay `build/gen/symbol_names.csv` onto
   `config/retail/functions.tsv`, derive data targets from the PE HIGHLOW relocation
   directory, and emit a fabricated `gruntz_named.pdb`,
   bucket-shift 16 so un-named functions group into `seg_NNNN.cpp`),
2. runs `vostok-delinker` on `build/exe/GRUNTZ.EXE` (the stable retail copy)
   -> `build/delink/named/`,
3. collects the in-scope `<unit>.c.obj` into `build/objdiff/target/`.

`synth_pdb.py` (under `scripts/gruntz/build/`) is kept. The whole one-time local
setup runs in `gruntz init`: a stable retail copy at `build/exe/GRUNTZ.EXE`, the
Wine prefix, and clangd metadata. Ghidra is not a setup or build dependency. The
FID library labels are tracked
(`config/retail/functions_static_libs.tsv`, so they survive `git clean`); regenerate them with
`python -m gruntz.audit.fid_generate`.

The delink rule's declared outputs are the per-unit `build/objdiff/target/<unit>.c.obj`
(one command, multiple outputs); its inputs are the EXE,
`config/retail/functions.tsv`, and `build/gen/symbol_names.csv`.

### Ghidra enrichment metadata (apply.py inputs)

Beyond names, the Ghidra DB is enriched from generated, source-derived metadata so
nothing important lives only in the `.gpr` blob — it is all reproducible:

- `config/retail/functions.tsv`: the complete admitted function boundary model.
  `apply.py` pushes this model into a disposable viewer database; it never exports
  boundaries back into the build.
- `build/gen/functions.json` (`labels.py`): per-RVA **signatures** — class, return
  type, calling convention, and named parameters. Derived by joining the IR
  rva-map with `llvm-undname` (authoritative return/cc/class/param-types) and the
  clang AST (parameter *names*). `apply.py` applies these as typed Ghidra
  prototypes (+ a struct\* `this`).
- `build/gen/globals.json` (`labels.py`): per-RVA **declared global type** for each
  named global (the `DATA()`-bound declaration's C/C++ type and pylibclang
  `Type.get_size()` from the current TU). The same current-TU extent enrolls the
  datum in the delinker manifest; `structs.json` is not on this path. `apply.py`
  lays typed data at the address so the global decodes as its real type (e.g.
  `g_buteMgr : CButeMgr`) instead of raw bytes.
- `build/gen/locals.json` (`harvest_locals.py` + `codeview.py`): per-RVA **named
  local variables** for byte-exact functions only. Each src TU is compiled a second
  time with `cl /Z7` (codegen-neutral — the `.text` is byte-identical to the base
  obj, so the debug objs stay OUT of the matching graph, in `build/debug/`), and the
  old-format MSVC 5.0 CodeView (`.debug$S`) is read for frame-relative locals.
  `apply.py` injects them as named stack variables (they surface in the on-demand
  decompiler). The harvest runs before every `apply.py` (in `_ghidra_metadata_apply`).
- `build/gen/structs.json` + `build/gen/enums.json`
  (`ghidra_metadata_generate.py`): whole-tree Clang record-layout and enum indexes
  used by static layout/access audits. Headers enter through their real
  `#include`s. The optional Ghidra viewer consumes the same indexes.

### Name precedence (src wins) and the apply report

`apply.py` layers names in a fixed order so the outcome is deterministic and the
`src/` labels are the SOURCE OF TRUTH at every RVA they claim:

1. **FID library labels** (`config/retail/functions_static_libs.tsv`; HIGH/MED/AMBIG only — LOW
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

A re-run is a fixed point: the corrected boundary model does not change and names
do not flap (only a
small set of already-correct names is re-asserted, to the identical value). Every
layer's applied / skipped / conflict tally — including boundary fixes,
`src-claimed skipped`, and the byte-exact locals coverage (`locals.json sets` /
`reached`) — is written to
`build/ghidra-named/exports/enrichment_apply_report.txt`.

The Ghidra project is disposable generated state. Record a useful discovery directly
in `src/` or its evidence-backed tracked config; GUI-only names, comments, and local
edits are not a supported persistence channel and do not survive clean rebuilds.

### What triggers a re-delink (incremental label map)

The label map is built **per TU**: `gen_labels_one` runs `labels.py` on one
`src/<unit>.cpp` (Clang IR/AST plus pylibclang DATA extents) → a fragment
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
it ensures the wineserver is up (kept alive across builds, not killed each time)
and runs `ninja`, which builds the objs and `report.json` in-graph. With a warm
wineserver, the build/objdiff portion of an up-to-date `--fast` invocation is
subsecond; source-tree checks are intentionally not part of that path.

**Gate tiers.** The structural gate tail runs in one of three tiers (`--tier`,
default `normal`; `--fast`, `--normal`, and `--full` are convenience flags). The
tiers answer different questions:

| tier | ~wall | what runs | when |
| --- | --- | --- | --- |
| **fast** (`--fast`) | **0s of gates** | incremental build, objdiff, concise score only | every matcher iteration |
| **normal** (default/`--normal`) | ~6s of checks | fast + negative controls, annotation/order/uniqueness checks, text scoreboard, and cheap feedback | once per commit or hand-off |
| **full** (`--full`) | ~10–15 min warm; ~15–20 min if layouts are stale | normal + semantic scoreboard, whole-tree libclang scans, class layouts, vtables, view debt, and declared-only discovery | periodic/daily, or to create a work plan |

Fast deliberately runs **zero source gates**. Normal owns the checks that answer
“is this change structurally safe?” Full answers “what reconstruction debt remains?”
Full checks therefore continue after a finding so a single run searches the whole tree
instead of stopping at the first backlog item. Discovery scans write durable worklists
such as `build/gen/bare_constants.tsv` and `build/gen/enum_case_labels.tsv`.
Measured on 2026-08-06, an up-to-date normal build took 5.9s, including 1.73s
for the source-text cleanliness scoreboard. The slower caller/callee IR metric
now lives in `cleanliness-semantic-baseline.tsv` and runs only at full tier.
Full-only non-libclang checks were 0.4–6.0s each.
The two all-TU libclang scans (`enum_case_labels` and `bare_constants`) were roughly
5–7 minutes each, and a stale `structs.json` layout regeneration adds about 4.5 minutes.

The normal tier also re-proves
`config/retail/data.tsv` with
`python -m gruntz.audit.data_denominator --check`. The artifact partitions every
unenrolled initialized-data range into eligible or excluded bytes. Its roots are
game/compiler code and enrolled data; static data-to-data pointers propagate
visibility, while proven CRT/MFC/library functions are traversal boundaries.
Consequently library ownership alone never removes a byte from coverage. Directly
game-visible library data remains eligible, and every unclassified byte remains in
the denominator. Refresh the generated artifact with `--write` only after reviewing
the reachability/ownership delta; the scoreboard reports reconstructable coverage
from the checked artifact and falls back to gross coverage when it is absent or stale.

The full-tier linked-referent ratchet always rebuilds a dedicated
`GRUNTZ.integrity.EXE` with the derived retail object list. It must not overwrite
`GRUNTZ.candidate.EXE`: doing so makes Ninja accept an experimental layout as a fresh
canonical candidate and can refresh README from the resource-less archive link. The
gate generates `build/gen/data-integrity-link-order.txt` from the checked link-line
model, rebuilds the engine archives, and passes both explicitly to the linker before
counting wrong, ordering-only, and multiplicity-only referents. The archive step is
required: putting recovered library members directly on the command line can duplicate
definitions also supplied by an SDK archive (the dxguid GUIDs are the negative control)
and creates a link shape retail could not have produced.

**Build timing.** Every `gruntz build` records its wall-clock — printed as
`[gruntz] build timing: total Ns (ninja Xs, gates Ys) [tier]` and appended to
`build/gen/build_times.tsv` (gitignored, per-worktree; columns
`timestamp worktree mode ninja_s gates_s total_s`). It splits **ninja** (the
incremental recompile/delink — usually seconds) from **gates** (the tier above).

## Data-symbol normalization (before objdiff)

Between delink and report, a `normalize` ninja edge
(`scripts/gruntz/build/normalize_objs.py` → `canonicalize_data_symbols.py`) rewrites the
compiler-private data names (`$SG`/`$T`/`name$S<n>`) and same-function jump-table `DIR32`
labels of every base + target obj into a **content-addressed, disposable comparison copy**
under `build/objdiff/normalized/{base,target}/`. objdiff pairs those copies so identical
data/jump-tables match BY NAME across base and target. The transform is **matching-neutral**
(the real `base/`+`delink/` objs are untouched; a fail-closed reparse proves only symbol
names + authorized jump-table reloc fields moved and every resolved offset is unchanged) and
proven safe over all objs (exact-match count unchanged). See **`docs/data-attribution.md`**;
`gruntz data-audit` complements it with a retail data-byte attribution ledger.

The normalizer also attempts content names for emitted `_$E<n>` text helpers,
but that does **not** make the ordinal a stable binding. Retail delinking may
omit relocation records that exist in the recompiled object; the resulting
digest then retains different address bytes and cannot establish identity.
These helpers remain unlabelled and are recorded only in
`config/retail/compiler-generated-functions.tsv`.

### The EH funclet band (`gruntz.build.eh_band`)

cl 5.0 compiles every `/GX` function that owns a destructible object into two
pieces: the body, and a small EXECUTE COMDAT (`.text$x`) holding that function's
**unwind funclets** (`mov ecx,[ebp-X] ; jmp <dtor>`, one per unwind state) followed
by its **registration stub** (`mov eax,<FuncInfo> ; jmp __CxxFrameHandler`), which
the prologue pushes to build its `EXCEPTION_REGISTRATION`. The retail linker packed
every one of those COMDATs into one contiguous band at the end of `.text`
(RVA `0x1d7d00`..`0x1e3b55`).

No unit's contribution covered that band, so each prologue's `push` decomposed as an
**undefined `FUN_005exxxx` plus a nonzero addend**: the delinked object set did not
close over EH (a relink would fail on the unresolved externals) and objdiff could
only name-match the reference — the funclet bytes were never compared.

`scripts/gruntz/build/eh_band.py` derives each group from **retail data alone**: it
scans a claimed function's body for a `push imm32` landing on a `b8 …/e9 …` stub,
reads the `FuncInfo` that stub loads (magic `0x19930520`) and walks its unwind map
(and try-block map) for the funclet addresses. `synth_pdb.py` adds one record per
funclet plus one for the stub to the PDB, attributed to the OWNING unit, and
supersedes the inventory's finer per-funclet `eh` rows inside each group so the
delinker never sees overlapping records. Naming mirrors cl's own labels — anything
coarser is truncated at the base's next `$L` label and compares against the wrong
extent:

    __ehunwind$<owner>$<n>   the n-th unwind funclet, n in ADDRESS order (== state order)
    __ehreg$<owner>          the registration stub

`canonicalize_data_symbols.py` renames the base's compiler-numbered `$L<n>` labels to
the same names (the owner is the function containing the `push`, identical on both
sides), so the two sides co-name without either reading the other.

Result: 750 groups / 2284 unwind funclets / 30,672 B carved, **zero** funclet pushes
left on an undefined `FUN_`, every push decomposing as `__ehreg$<owner>+0`, and the
funclet bytes genuinely compared. `python -m gruntz.audit.eh_band` reports the
inventory, re-proves closure and the carve, and classes each group
(`unwind-identical` / `unwind-content-differs` / `unwind-count-differs`) — the last
two are real reconstruction defects the masked comparison used to hide.

These symbols are **scored but excluded from the reconstruction-target scoreboard**
(`gruntz.core.report.split_eh_band`): the function universe classifies the whole band
`eh` and the README's denominator never had them, so leaving them in objdiff's
aggregate would put them in the numerator alone. `gruntz status` prints their state on
its own line.

The stub's `mov eax,<FuncInfo>` still reads 97.5% on all 750 with byte-identical
code, but the reason is now one level deeper, and getting there needed a DELINKER
fix rather than a manifest workaround
(`nix/patches/vostok-data-hypothesis-must-contain.patch`).
`data_manifest::hypothesis_owner_and_addend_for_rva` ranked enrolled definitions by
`(!contains, distance, …)` but returned the best one even when NOTHING contained the
rva, with an unbounded addend — and both callers consult it BEFORE the
`--recover-data-relocs-from-pdb` fallback, so the guess beat the exact-address PDB
symbol. Measured: **1,020 of 21,730** enrolled-symbol data relocations decomposed past
their symbol's end, across 185 objects — `??_R4CGruntVoice@@6B@ + 0x10800` into a 0x14 B
RTTI locator (all 750 stubs), `_inflate_mask + 0x3db4` into 0x44 B (164 sites), and
negative addends where the nearest enrolled datum sits AFTER the target. Requiring
containment takes that to **0**, and the stub now names `DAT_<va>` at addend 0 — the
FuncInfo exactly.

Strict containment deliberately excludes one-past pointers: the byte at
`datum_rva + sizeof(datum)` is not part of the datum. When retail proves that an
individual relocation encoded such an expression, record its exact function,
target, relocation-field RVA, owner and addend in
`config/retail/reloc-aliases.tsv`. The delinker validates the owner/addend equation,
site membership and occurrence count before using it; the build graph makes the
manifest an input to re-delinking.

What is left is the FuncInfo's EXTENT, and it is the `.xdata$x` half
(`docs/referent-debt-ddrawmgr.tsv`, class `c`): the delinker sizes a PDB data symbol
to its next neighbour and gets **4 B** — just the `0x19930520` magic — where cl emits
`32 + 8*maxState`. Until the FuncInfo is enrolled in `delink_data_manifest.tsv` with
that provable extent, the two sides must NOT be co-named: renaming both to
`__ehfuncinfo$<owner>` was tried and `gruntz.audit.data_relocs` correctly failed it
750 times, because our 40-byte blob carries a `pUnwindMap` relocation at +8 that a
4-byte truncation cannot have. Co-naming a truncation is a claim the bytes do not
support; the enrollment comes first.

## Pairing (objdiff)

`build/objdiff/objdiff.json` (written by `configure.py:emit_objdiff`) pairs, per unit:

- base: `./normalized/base/<unit>.obj` (cl `/O2 /MT`, then data-name normalized)
- target: `./normalized/target/<unit>.c.obj` (delinked per `symbol_names.csv`, normalized)

Symbols are pre-named on both sides (cdecl `_<name>`), so objdiff pairs them
**by symbol name** with no `symbol_mappings` overlay. A unit whose target obj
does not exist yet is paired against an empty `dummy.obj` so it still lists at
0% (`build_base`/`build_target` are `false`: ninja, not objdiff, builds objs).

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, and a
   `flags` profile — `base` unless the TU needs `/GX`/`/O1`);
2. `#include "../rva.h"` and annotate **each** matched function with an `RVA()`
   macro (`include/rva.h`) directly above the definition, after the description. A
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

   The macros (`include/rva.h`, compiled to nothing under MSVC 5.0 — it predates
   `__attribute__` and C99 variadic macros, so each macro is FIXED-arity):
   - `RVA(addr, size)` — a matched function;
   - `DATA(addr)` — on an `extern` decl of a matched global (the DATA symbol it
     is referenced through);
   - `RVA_COMPGEN(<rva>, <size>, <mangled>)` — the macro (rva.h) for a
     deterministically named compiler-generated function with no source body
     (such as a `??_G` deleting dtor) that cannot hold an attribute. Volatile
     ordinal names such as `_$E<n>` are forbidden here and belong in
     `config/retail/compiler-generated-functions.tsv`.

   Its DATA analog is a **manifest**, `config/retail/data_compgen.tsv`
   (`rva`/`size`/`symbol`/`emitter`) — see "Compiler-generated DATA pins" below.

   `labels.py` reads `RVA` from **LLVM IR** (`@llvm.global.annotations`
   pairs the mangled symbol DIRECTLY with the annotation — no positional join;
   the old `SYMBOL()` name-override escape hatch is RETIRED — a clang-vs-VC5
   mangling mismatch is a modeling bug to fix in source);
   `DATA` from the clang AST (an `extern`'s annotation is dropped from IR). The
   label map regenerates from these annotations — never hand-edit the CSV. (The
   vendored zlib C TUs keep PRISTINE source — no labels in it; their rva→symbol
   map is the static `config/retail/functions_zlib.tsv (+ data_zlib.tsv)`, emitted directly. See
   `docs/zlib-matching.md`.)
3. `gruntz build` (configure -> compile -> labels -> delink -> objdiff).

### Compiler-generated DATA pins

`config/retail/data_compgen.tsv` is the DATA analog of `RVA_COMPGEN`.
It names a datum cl.exe emits from a definition that is already in the tree but
that neither source-side data device can reach:

| device | binds to | why it cannot reach this |
| :-- | :-- | :-- |
| `DATA(rva)` | an AST VarDecl in the MAIN file | a function-local static inside a **header** inline lives outside the main file, and `labels.collect_vars` is main-file-only |
| `DATA_COMPGEN(rva, value)` | a value expression at a **use site** | a `??_B` dynamic-init guard byte has **no source spelling at all** — cl assigns it a counter (`??_B?1??Fn@@YAHXZ@51`) |

(The `DATA_COMPGEN` *macro* is a different, last-resort device for use-site
literals the automatic identity oracles cannot reach — its rule and wiring live in
`docs/data-attribution.md` §3b-iii. This manifest covers the COMMON population
neither macro can express.)

Schema (gated): four tab-separated columns `rva` (zero-padded to 8 lowercase hex
digits) · `size` (unpadded lowercase hex) · `symbol` (verbatim, as `RVA_COMPGEN`
takes it) · `emitter` (the header + function whose definition makes cl emit it),
rows ascending by rva.

**Why a manifest and not a macro.** `RVA_COMPGEN` is a *source* pin because a
compiler-generated FUNCTION belongs to one COMDAT in one TU's contribution, and
its source position encodes that ownership (`compgen_order` ratchets it). These
data have **no owning TU**: cl emits each as a COFF **COMMON** — a tentative
definition — into every TU that instantiates the header inline, and the linker
merges the copies into one bss slot. Any source position would fabricate an owner.

**Why this is not the retired `DATA_SYMBOL`.** `DATA_SYMBOL` was a source
*declaration* that let a datum exist as a name-only pin **instead of** a real C++
definition. Here the definition is real (the `emitter` column names it) and the
only fact stated is the retail ADDRESS, which the compiler cannot know. Everything
else is re-derived at every build: `labels.compgen_data_tu` emits a row only for a
TU whose base obj actually has that symbol as a COMMON of exactly that size, and
`python -m gruntz.audit.compgen_data` (normal tier, FATAL) additionally checks the
spelling, that the pin reached `symbol_names.csv`, and — the ratchet —
**coverage**: every COMMON in any base obj must be pinned. An invented row binds
nothing and fails the gate.

Coverage matters because this class is invisible to every other signal: objdiff
masks relocations, so an unnamed COMMON costs 0%, and it **links perfectly well**
(`gruntz link` resolves all of them as `<common>`), so `link_defects` sees nothing
either. The only thing that reported them was `assert_relocs`, and it mis-read
COMMON as an unresolved external (fixed; see its `defined_syms`).

## Generated vs. tracked

Tracked: `config/units.toml`, the `src/` sources (incl. their `RVA()`/`DATA()`
annotation macros and `include/rva.h`), `configure.py`, and the whole `scripts/gruntz/`
package, one sub-package per role: the pipeline `{build,ghidra,init}/`, the
shared engine library `core/`, match scoring + integrity gates `match/`, the
cleanliness board + quality gates `cleanliness/`, the permuter climbers
`permute/`, the sema navigation surface `sema/`, and the one-shot campaign
audits `audit/`.

Generated (git-ignored): `build/gen/symbol_names.csv` (from `src/` `RVA()`),
`build/gen/functions.json` + `build/gen/locals.json` (optional viewer metadata),
`build.ninja`, `.ninja_log`/`.ninja_deps`, and everything under `build/` (base
objs, `/Z7` debug objs, delinked target objs, synth PDB, the clangd compdb, the
wine prefix, optional Ghidra viewer, objdiff project + report) — see the table below.

## The `build/` directory

ALL local, imperative state lives under `build/` (every subdir is **git-ignored**;
`gruntz clean` nukes the lot, `gruntz init` rebuilds it). The retail EXE and the
toolchain come from the flake; nothing here is tracked.

| subdir | what it is | generated by | role in the pipeline | size class |
|---|---|---|---|---|
| `gen/` | generated source metadata: `symbol_names.csv`, signatures, locals, record layouts, enums and globals | `gruntz labels` / `structs` | delink name map, static audit indexes, and optional viewer inputs | ~1 MB |
| `objdiff/` | the diff project: `base/<unit>.obj` (wine `cl` output), `target/<unit>.c.obj` (delinked), `objdiff.json` (pairing), `report.json` (the scored result) | the ninja `cl`+`delink` rules + `objdiff-cli report generate` | the actual base-vs-target comparison `gruntz status` reads | ~2 MB |
| `delink/` | `named/` — raw per-symbol COFF objects straight out of vostok-delinker, before they are collected into `objdiff/target/` | the ninja `delink` rule (`delink.py`) | intermediate delinker output | ~3 MB |
| `pdb/` | the synthesized fake PDB (`gruntz_named.pdb` + its `.yaml`) — Public + Procedure + Data symbols, no section contribs | `synth_pdb.py` (inside the `delink` rule) | feeds vostok-delinker (it needs symbol+length records, which the retail EXE lacks) | ~12 MB |
| `exe/` | `GRUNTZ.EXE` — a stable-named copy of `$GRUNTZ_EXE` | `gruntz init`/`build` (`_ensure_retail_copy`) | the delink input and optional Ghidra import target | ~2.5 MB |
| `clangd/` | the clangd compile DB (`compile_commands.json`) + the derived `func_fingerprints.tsv` cache | `gruntz clangd` (`init/clangd.py`) + `gruntz.match.fingerprints` | editor/LSP navigation; per-function regression fingerprints | ~4 MB |
| `debug/` | the `/Z7`-compiled `<unit>.obj`s (debug info embedded) used only to harvest local-variable names/types | the ninja debug-compile path (`harvest_locals`) | source of `gen/locals.json` (Ghidra enrich) | ~1.5 MB |
| `ghidra-named/` | optional disposable Ghidra viewer (`gruntz.{gpr,rep}`), populated from tracked/source data | explicit `gruntz ghidra-refresh` only | decompiler UI; never read by build/delink/status | ~42 MB |
| `wineprefix/` | the Wine prefix with the MSVC 5.0 toolchain registered | `gruntz init` (`init/toolchain.py`); `$WINEPREFIX` | hosts every `wine cl` invocation (the base-obj compiles) | ~561 MB |

(Also transient: `build/fid/` — scratch for `gruntz.audit.fid_generate`'s
library-label regen — and root-level `build.ninja`/`.ninja_*`.)

## Current status

zlib 1.0.4 plus a growing set of engine/Gruntz reconstructions build through ninja,
and the generated label map drives the delink. **Run `gruntz status` for the live
match %** (kept out of this doc so it can't go stale). Part of the exact-vs-fuzzy
gap is a reloc-naming difference — delinked relocs reference Ghidra `DAT_*/FUN_*`
while the base references the real symbols, not a code difference — and the rest
are functions still in progress.
