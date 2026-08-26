# The matching build system (manifest -> ninja -> objdiff)

The matching loop is a **native incremental ninja build** generated from a
single manifest. `gruntz build` is the one entry point; every edge in the graph
is also a verb of its own (see [`docs/tooling-map.md`](tooling-map.md)).

```
config/units.toml  (per-TU manifest: unit, source, flags profile)
        |  gruntz configure          (gruntz.graph.emit)
        v
build/build.ninja
        |  ninja
        +-- cl        src/<unit>.cpp -> build/objdiff/base/<unit>.obj   (wine cl 5.0)
        +-- compdb    units.toml     -> build/clangd/compile_commands.json
        +-- labels    source + base obj -> build/gen/claims/<unit>.tsv
        +-- model     claims x censuses/providers -> build/gen/bindings.tsv
        +-- delink    bindings -> synth PDB -> vostok-delinker
        |                       -> build/objdiff/target-new/<unit>.c.obj
        +-- normalize base + target objs -> the disposable comparison copies
        +-- project   the delinked directory -> compare-new/objdiff.json
        +-- report    objdiff-cli -> build/objdiff/compare-new/report.json
        +-- verify_fp the per-function source-fingerprint cache
        +-- verify_check  the MAX gate + the fast and normal gate tiers
        v
per-unit + roll-up match % (`gruntz verify status`)
```

Phony aliases stop the graph early: `ninja -f build/build.ninja base` (objects
only), `claims`, `target`, `compare`, `verify`. `all` is the default.
`candidate` is phase 2 and is never in `all`.

Everything runs inside `nix develop` — the one dev shell (`.#build` is a kept
alias of it), which exports `MSVC_DIR`, `DXSDK_DIR`, `WINEPREFIX`, and `ninja`
on PATH. The Wine prefix is created by `gruntz init`, which the shell runs on
entry (idempotent).

## Quick start

```sh
nix develop                     # the dev shell: analysis tools + MSVC 5.0 under Wine
gruntz init                     # once: the build wine prefix
gruntz build                    # cl -> labels -> model -> delink -> compare -> verify check
gruntz match                    # the same build, then the summary for the CHANGED units
```

Pass ninja arguments straight through: `gruntz build -j8 -v`, or name a phony
target: `gruntz build base`.

## Incrementality: what re-runs after an edit

Every producer writes **if-changed** and its edge carries `restat`, so an edit
that does not change a downstream input stops the cascade there.

- **Pure code edit** (no label change): `cl` recompiles one object and `labels`
  re-extracts one fragment. The fragment is byte-identical, `restat` stops
  `model`, and the delink and every target object are untouched. Net work: one
  base obj, one normalize pass, a fresh `report.json`.
- **Label change** (add/rename a function, move an `RVA()`): the fragment
  changes → `model` rewrites `bindings.tsv` → `delink` re-runs → that unit's
  `<unit>.c.obj` updates.

Two edges declare a **stamp** rather than their real outputs, because neither
set can be enumerated at configure time: `delink` writes one object per unit
that has a claim (declaring all ~311 would re-run the whole delink on every
build), and `normalize` writes a variable pair of copies per unit. Both drivers
are keyed on content upstream, so a stamp only moves when something real did.
`gruntz build --force-delink` drops the delink stamp when you want it anyway.

`gruntz build` records its wall clock in `build/gen/build_times.tsv`
(gitignored, per-worktree).

## The manifest: `config/units.toml` (single source of truth)

Per **translation unit**. Every `[[unit]]` names a `[flags]` profile
explicitly; a profile is the FULL flag set, there are no per-unit bolt-ons.

```toml
[build]
compiler = "msvc5.0"
platform  = "win32"

[flags]
c             = ["/nologo", "/c", "/O2", "/MT"]                        # vendor C (zlib)
cpp           = ["/nologo", "/c", "/O2", "/MT", "/GX"]                 # the engine libs
cpp-rtti      = ["/nologo", "/c", "/O2", "/MT", "/GX", "/GR"]          # the Gruntz project
cpp-rtti-noeh = ["/nologo", "/c", "/O2", "/MT", "/GR"]                 # movinglogic
cpp-noeh      = ["/nologo", "/c", "/O2", "/MT"]                        # bute's dyninit TUs

[[unit]]
unit   = "adler32"                  # stem; obj is <unit>.obj, target <unit>.c.obj
source = "vendor/zlib-1.0.4/adler32.c"
flags  = "c"
```

Every profile is **recovered from the retail bytes, not chosen**:

- `/O2` already forces function-level COMDAT packaging (no `/Gy` needed);
  deflate excludes packing below 4 while default `/Zp8` matches; `/GF` is off
  because retail literals use writable `.data` COMDATs. Calibrated against the
  exact witness panels — see [`docs/compiler-flags.md`](compiler-flags.md) and
  [`docs/zlib-matching.md`](zlib-matching.md).
- `/GR` is on for the Gruntz project only: retail's Gruntz vtables carry a
  Complete Object Locator at `[-4]`, the engine libs' do not.
- `cpp-rtti-noeh` exists because retail's `movinglogic` unit has zero EH frames
  while every sibling has them.
- `cpp-noeh` exists because Bute's two container globals have their ctor/dtor
  bodies INLINE in the `$E` dynamic-init helper, which cl 5.0 only does without
  `/GX` (`docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md`).

Per-unit rationale (why a TU exists, was split, or absorbed another) lives in
[`docs/tu-partition-brief.md`](tu-partition-brief.md); the compile-flag evidence
is in [`docs/compiler-flags.md`](compiler-flags.md).

## The `cl` rule (the wine compiler bridge)

```ninja
rule cl
  command = $py -m gruntz.tool.cl --out $out --src $in -- $cflags
```

`gruntz.tool.cl` is the Linux→Wine bridge. For each TU it resolves `CL.EXE`
under `$MSVC_DIR/bin` (case-insensitively), keeps a persistent `wineserver -p`
alive so `ninja -j` does not pay a cold wineserver start per object, translates
the paths with `winepath -w`, runs `wine cl.exe <flags> /Fo<obj.w> <src.w>`, and
treats **"the `.obj` exists"** as the success signal — Wine spews unrelated
driver noise and can return a non-cl exit code. Objects are written **with the
COFF timestamp stabilised and only if the content changed**, which is what makes
`gruntz match`'s "which units moved?" question answerable by hash.

## Labels → claims → the Model

`gruntz labels` (`gruntz.retail_labels.source`) reads one `src/<unit>.cpp` and
emits `build/gen/claims/<unit>.tsv`. The macros come from **LLVM IR**
(`@llvm.global.annotations` pairs the mangled symbol DIRECTLY with the
annotation — no positional join) plus the clang AST for `extern` declarations
IR drops, and every emitted name is authorized against the unit's own base
object: a name cl did not emit is DROPPED and reported, never claimed.

The extraction front end reads the VC5 source dialect, not modern C++
portability policy. Its shared clang flags demote two constructs VC5 accepts
but clang otherwise rejects: taking the address of a temporary and using a
signed `switch` with an unsigned `0x80000000`-range SDK HRESULT case macro.
The latter keeps `case DDERR_*`/`DIERR_*`/`DSERR_*` source intact instead of
requiring a cast around every label.

`gruntz model` is the one join. Claims (from `src/`) and provider tables (from
`config/retail/`) are resolved against the base censuses by channel precedence:

```
src > src_compgen > src_dyninit > src_data_compgen > functions_zlib/data_zlib
    > data_vtables > data_compgen > data_static_libs > functions_static_libs
```

The winner per rva is the binding; the losers are recorded as aliases. The
result is `build/gen/bindings.tsv` plus `build/gen/violations.tsv`, and
**violations must be 0** — an unadmitted claim, a size crossing, a duplicate
data name, or a keyword-spelled `RVA_DYNINIT` owner all land there. This is why
there is no separate channel/size/uniqueness audit any more: the invariant is
structural.

## The target (delink) half

`gruntz delink` (`gruntz.delink.run`):

1. `gruntz.delink.pdb_synth` builds `build/pdb/gruntz_named.pdb` from the Model
   — function records with the claim-resolved extent and a synthetic
   `c:\proj\<unit>.c` source file so the delinker emits one `<unit>.c.obj` per
   TU, plus data records for every relocation-target address renamed to the
   claimed source names, cl's own `??_C@` string-pool spellings, and the proven
   `__imp_` IAT decorations. An identity is always PROVIDED, never invented: a
   target no name reaches keeps a fence spelling that states the verdict —
   `DAT_<va>` when only library bands reference it, `UNPROVISIONED_<va>` (which
   the delinker refuses to emit) when a game band does.
2. `gruntz.delink.data_manifest` writes the data + section manifests, including
   the `class=common` re-proof: every COFF COMMON row in
   `config/retail/data_compgen.tsv` must be emitted by some base obj, and the
   owner is the earliest-arriving module in link order among those that emit it.
3. `vostok-delinker` runs over `build/exe/GRUNTZ.EXE` (the stable retail copy)
   into `build/delink/named/`;
4. the in-scope `<unit>.c.obj` are collected into `build/objdiff/target-new/`.
   The address-bucketed `seg_NNNN.cpp.obj` for the un-named `.text` remainder
   stay behind and are never collected.

### The EH funclet band

cl 5.0 compiles every `/GX` function that owns a destructible object into two
pieces: the body, and a small EXECUTE COMDAT (`.text$x`) holding that function's
**unwind funclets** (`mov ecx,[ebp-X] ; jmp <dtor>`, one per unwind state)
followed by its **registration stub** (`mov eax,<FuncInfo> ; jmp
__CxxFrameHandler`), which the prologue pushes to build its
`EXCEPTION_REGISTRATION`. The retail linker packed every one of those COMDATs
into one contiguous band at the end of `.text` (RVA `0x1d7d00`..`0x1e3b55`).

No unit's contribution covered that band, so each prologue's `push` decomposed
as an **undefined `FUN_005exxxx` plus a nonzero addend**: the delinked object
set did not close over EH, and objdiff could only name-match the reference —
the funclet bytes were never compared.

`gruntz.delink.eh_band` derives each group from **retail data alone**: it scans
a claimed function's body for a `push imm32` landing on a `b8 …/e9 …` stub,
reads the `FuncInfo` that stub loads (magic `0x19930520`) and walks its unwind
map (and try-block map) for the funclet addresses. `pdb_synth` adds one record
per funclet plus one for the stub, attributed to the OWNING unit, superseding
the census's finer per-funclet `eh` rows so the delinker never sees overlapping
records. Naming mirrors cl's own labels — anything coarser is truncated at the
base's next `$L` label and compares against the wrong extent:

    __ehunwind$<owner>$<n>   the n-th unwind funclet, n in ADDRESS order (== state order)
    __ehreg$<owner>          the registration stub
    __ehfuncinfo$<owner>     the 32-byte `_s_FuncInfo` record the stub loads
    __ehunwindmap$<owner>    the `8 * maxState` unwind map that follows it

Both data extents are PROVEN out of the record rather than assumed: the blob
enrolls only when its own `pUnwindMap` word points at `funcinfo + 32` and its
try-block / ip-to-state maps are empty. `gruntz.compare.canonicalize` renames
the base's compiler-numbered `$L<n>` labels to the same names (the owner is the
function containing the `push`, identical on both sides), so the two sides
co-name without either reading the other.

Result: 750 groups / 2284 unwind funclets / 30,672 B carved, **zero** funclet
pushes left on an undefined `FUN_`, every push decomposing as `__ehreg$<owner>+0`,
and the funclet bytes genuinely compared. These symbols are scored but excluded
from the reconstruction-target denominator (`gruntz.verify.universe` classifies
the whole band `eh`).

Getting the stub's `mov eax,<FuncInfo>` right needed a DELINKER fix rather than
a manifest workaround (`nix/patches/vostok-data-hypothesis-must-contain.patch`).
`data_manifest::hypothesis_owner_and_addend_for_rva` ranked enrolled definitions
by `(!contains, distance, …)` but returned the best one even when NOTHING
contained the rva, with an unbounded addend — and both callers consult it BEFORE
the `--recover-data-relocs-from-pdb` fallback, so the guess beat the
exact-address PDB symbol. Measured: **1,020 of 21,730** enrolled-symbol data
relocations decomposed past their symbol's end, across 185 objects —
`??_R4CGruntVoice@@6B@ + 0x10800` into a 0x14 B RTTI locator (all 750 stubs),
`_inflate_mask + 0x3db4` into 0x44 B (164 sites), and negative addends where the
nearest enrolled datum sits AFTER the target. Requiring containment takes that
to **0**.

Strict containment deliberately excludes one-past pointers: the byte at
`datum_rva + sizeof(datum)` is not part of the datum. When retail proves that an
individual relocation encoded such an expression, record its exact function,
target, relocation-field RVA, owner and addend in
`config/retail/reloc_referents.tsv`. The delinker validates the owner/addend
equation, site membership and occurrence count before using it, and the build
graph makes the manifest an input to re-delinking.

## Normalization and pairing (objdiff)

Between delink and report, the `normalize` edge
(`gruntz.compare.normalize` → `gruntz.compare.canonicalize`) rewrites the
compiler-private data names (`$SG`/`$T`/`name$S<n>`), resolves COFF weak
externals to their default (cl's `??_E<C>` vector-deleting-dtor slot →
`??_G<C>`), materializes COFF COMMONs into `.bss` exactly as the linker would,
and rewrites same-function jump-table `DIR32` labels — into a
**content-addressed, disposable comparison copy** under
`build/objdiff/normalized/{base,target}/`.

The transform is **matching-neutral**: the real `base/` and `delink/` objects
are untouched, and a fail-closed reparse proves that only symbol names and
authorized jump-table relocation fields moved while every resolved offset is
identical. So normalization can only sharpen objdiff, never inflate a false
match. See [`docs/data-attribution.md`](data-attribution.md).

`gruntz.compare.project` writes `compare-new/objdiff.json`. Symbols are
pre-named on both sides (cdecl `_<name>`), so objdiff pairs them **by symbol
name** with no `symbol_mappings` overlay, under **strict relocation scoring**
(target name/address AND the pointed-to data participate). Every manifest unit
gets a base entry; which units have a TARGET is read off the directory the
delinker wrote, never predicted — a unit with no target pairs against an empty
`dummy.obj` and lists at 0%. That distinction is load-bearing: predicting the
named set once left two data-only units pointing at the dummy while their real
target objs sat unopened, and objdiff scores an empty pairing 100.00% on every
measure with zero totals.

## Gates: `gruntz verify check --tier`

The graph's `verify_check` edge runs the MAX gate plus the **fast** and
**normal** tiers; `full` and `link` opt in. The full roster is in
[`docs/tooling-map.md`](tooling-map.md#the-verify-slice--scores-the-max-gate-and-every-ported-gate).

| tier | question it answers | when |
| --- | --- | --- |
| **fast** | is the source text within its committed ledgers? (board, cast ledger, vtable bans, enum devices, label style, include order) | every build |
| **normal** | is this change structurally safe? (name uniqueness, library overlap, TU order, data TU order, undefined closure) | every build |
| **full** | what reconstruction debt remains? (vtable tier, alloc-size sizeof oracle, reloc multisets, data relocs, caller/callee, the retail data-access map + the claim-side coverage census) | periodic, or to build a work plan |
| **link** | does it link, land where retail landed, and reach the same referents? | after `gruntz link` |

Two rules hold across all of them. A gate **returns findings and writes
nothing** — lowering a floor is always a separate manual verb (`gruntz verify
board --update`, `gruntz verify bank`). And every gate ships with its **negative
control**: `gruntz verify selftest` feeds each one a known violation and asserts
it FAILS, then asserts clean input passes. A gate nobody has seen fail is a
green light, not a check.

## Semantic navigation — `gruntz sema`

```sh
gruntz sema rva     0x00080850    # address dossier: winning binding, aliases, channel, match%
gruntz sema disasm  0x0008c750    # annotated retail i386 assembly  [--lite --blocks --switch]
gruntz sema dump    0x0008c750    # raw bytes + relocation targets + asm
gruntz sema xref    0x00080850    # callers, callees, referent sites
gruntz sema strings 0x00080850    # strings a function reaches; --find TEXT reverses it
gruntz sema vtable  0x001e8754    # a vtable's slots / who holds a function
gruntz sema class   CGrunt        # every vtable the class holds, slot by slot
gruntz sema map                   # the retail address-space map
gruntz sema match   cplay         # objdiff scores for a unit / function
```

sema is a **read-only consumer with four inputs and no policy of its own**: the
Model for identity, the retail image for bytes, the compare report for scores,
`config/units.toml` for the unit list. It writes nothing. Every module is also a
direct entry (`python3 -m gruntz.sema.xref 0x136180`), and `gruntz sema -` is
batch mode — newline-delimited view commands on stdin answered against ONE
loaded Model and image, so a 40-query investigation pays one parse instead of
forty. rc convention: **0 answered, 1 answered-NO, 2 error**.

**Doctrine: assembly only.** Nothing here decompiles; views annotate real
instruction bytes with Model labels, and a question the labels cannot answer is
reported as unanswered rather than guessed.

Two things sema deliberately does **not** do:

- **base-vs-target comparison.** That is the compare report's job, and
  classifying a divergence is `gruntz walls diagnose <fn>` — it reads the
  NORMALIZED pair (the exact evidence objdiff scored) and names the first
  divergence class: referent → inline/call-set → cfg → regalloc.
- **source navigation.** `gruntz lsp refs|hover|rename` is clangd-backed and
  USR-exact, so a same-named member of a different class is never touched.
  It needs `build/clangd/compile_commands.json`
  (`python3 -m gruntz.graph.compdb`, or just `gruntz build`).

clangd is a READER of this MSVC5 dialect: navigation is reliable, its
diagnostics are NOT build truth — the wine `cl` build and objdiff are.

## Formatting — the Rust-like house style

The reconstructed C++ is formatted with **clang-format** (from the Nix dev
shell) to read as close to Rust as the language allows: 4-space indent, 100-col
lines, attached braces *including on function definitions* (`int f() {`), `&`/`*`
bound to the type (`int* p`), a hanging-close (BlockIndent) wrap with function
*declaration* params one-per-line (call args and data arrays stay bin-packed, so
GUID/byte tables don't explode), and braces on every control body. The full
config — and the deliberate decompile-specific deviations — lives in the root
**`.clang-format`**.

Formatting is **whitespace-only ⇒ matching-neutral**: it never changes the COFF
bytes objdiff compares (the one parser-visible case, `> >` vs `>>` for MSVC 5.0,
is pinned by `Standard: c++03`).

**You normally never run it by hand.** A repo-tracked **pre-commit hook**
(`.githooks/pre-commit`) runs `clang-format` over staged `src/`+`include/` files
on each commit; the dev shell enables it on entry via `git config core.hooksPath
.githooks` (idempotent; shared across worktrees). Outside the Nix shell (no
`clang-format` on PATH) the hook skips with a notice rather than blocking the
commit.

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
outside the `src/`+`include/` roots the hook touches, and is independently
guarded by `vendor/.clang-format` (`DisableFormat: true`), so even an editor's
format-on-save leaves it alone.

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, and a
   `flags` profile);
2. `#include "rva.h"` and annotate **each** matched function with an `RVA()`
   macro directly above the definition, after the description. A real example
   from `src/Gruntz/SBI_RectOnly.cpp`:

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

   The macros live in `include/rva.h` and compile to nothing under MSVC 5.0 —
   it predates `__attribute__` and C99 variadic macros, so each macro is
   FIXED-arity:

   - `RVA(addr, size)` — a matched function;
   - `DATA(addr)` — on the definition of a matched global;
   - `RVA_COMPGEN(rva, size, mangled)` — a deterministically named
     compiler-generated function with no source body (such as a `??_G` deleting
     dtor) that cannot hold an attribute. Volatile ordinal names such as
     `_$E<n>` are FORBIDDEN here;
   - `RVA_DYNINIT(rva, size, owner)` — the `$E` dynamic-init helper, pinned at
     its OWNER (the owning datum's definition line) precisely because the `$E`
     ordinal is emission-order state, not identity;
   - `DATA_COMPGEN(rva, value)` — the last-resort use-site data pin; its rule
     and wiring are in [`docs/data-attribution.md`](data-attribution.md) §3b-iii.

   The vendored zlib C TUs keep PRISTINE source — no labels in it; their
   rva→symbol map is the static `config/retail/functions_zlib.tsv` (+
   `data_zlib.tsv`). See [`docs/zlib-matching.md`](zlib-matching.md).
3. `gruntz build`.

### Compiler-generated DATA with no source spelling

`config/retail/data_compgen.tsv` is a **manifest**, not a macro, and it covers
what neither source-side data device can reach:

| device | binds to | why it cannot reach this |
| :-- | :-- | :-- |
| `DATA(rva)` | a VarDecl in the MAIN file | a function-local static inside a **header** inline lives outside the main file, and extraction is main-file-only |
| `DATA_COMPGEN(rva, value)` | a value expression at a **use site** | a `??_B` dynamic-init guard byte has **no source spelling at all** — cl assigns it a counter (`??_B?1??Fn@@YAHXZ@51`) |

The manifest has two classes:

- **`class=common`** — the COFF COMMONs cl emits from a header inline's local
  static, plus the `??_B` guard beside them. cl emits a tentative definition
  into every TU that instantiates the inline and the linker merges the copies
  into one bss slot, so there is **no owning TU**: any source position would
  fabricate one. Only the retail address is stated; the `owner` column
  documents the emitting header inline. Everything else is re-proven every
  build — `gruntz delink` requires an emitting base obj for each row and errors
  on a row no base obj supplies.
- **`class=copy`** — a reviewed per-TU copy of a header static (the
  GruntDirStatics device), where the owner IS the emitting TU, decided by its
  `$E` static initializer's position. Several rows may share one rva, since each
  TU's copy folds onto the same retail byte.

**Why this is not the retired `DATA_SYMBOL`.** `DATA_SYMBOL` was a source
*declaration* that let a datum exist as a name-only pin **instead of** a real
C++ definition. Here the definition is real and the only fact stated is the
retail ADDRESS, which the compiler cannot know.

This class is invisible to every other signal — objdiff masks relocations so an
unnamed COMMON costs 0%, and it links perfectly well (`gruntz link` resolves
them all as `<common>`), so the link tier sees nothing either. That is why the
enrolment is a hard delink-time requirement rather than an advisory audit.

## Phase 2 — the candidate link (opt-in)

`gruntz link` (or `ninja candidate`) links every base `<unit>.obj` into
`build/exe/GRUNTZ.candidate.EXE` + `.map` with the genuine VC5 `link.exe`
(version **5.10.7303** — the linker that built retail GRUNTZ.EXE) under wine.
It is **out of the default target**, so a normal `gruntz build` is unaffected.

**There is no `/FORCE`, and it must never come back.** The tree links with zero
unresolved externals and zero duplicate symbols, so the link is an ORACLE.
`/FORCE` would re-swallow exactly the defects this phase exists to catch — an
unresolved extern (a fabricated name, a body homed nowhere) and an
LNK2005/LNK4006 duplicate. It also blocks `/INCREMENTAL` (LNK4075), and retail
IS an incremental link. A link failure here is a FINDING: read the LNK codes and
fix the source.

Layout study uses `/OPT:NOREF /OPT:NOICF` to keep every COMDAT in the map, and
`/FIXED:NO` to emit the `.reloc` retail has (purely additive). The obj list goes
through a **response file** — VC5 `link` has a short argv limit under wine.
`link.exe` statically imports **`MSDIS100.DLL`** (the VC5 disassembler, only
used by `/dump /disasm`); `gruntz.tool.wine` provisions it into the prefix so
the linker loads at all.

### The library set (and the two libs we have to synthesise)

The objects already carry most of it: `cl /MT` writes `-defaultlib:LIBCMT` +
`-defaultlib:OLDNAMES` into `.drectve`, and MFC's headers add `nafxcw kernel32
user32 gdi32 comdlg32 winspool advapi32 shell32 comctl32` — 8 of the 10 Win32
DLLs in retail's import table. We do **not** pass `/NODEFAULTLIB`, so those fire
exactly as they did for the devs. Three groups declare themselves nowhere and
are named explicitly:

| group | libs | why it is not declared |
|---|---|---|
| game-only Win32 | `version winmm` | used by the game, requested by no header |
| DirectX 6 | `ddraw dsound dinput dplayx` + static `dxguid` | the DX SDK ships no `#pragma comment(lib)` |
| RAD SDKs | `mss32 smackw32` | **we do not have those SDKs** |

`gruntz.graph.implib` rebuilds the two missing RAD import libs into `build/lib/`
from **retail's own import table**, which is ground truth: the stored names
(`_AIL_startup@0`) are already decorated exactly as the original import lib
produced them. It generates a throwaway **stub DLL** of `__declspec(dllexport)
__stdcall` functions with matching argument-byte counts and keeps link.exe's
`/IMPLIB:` — `LIB /DEF:` cannot express it, because it derives the public symbol
by prefixing an underscore (`__imp___AIL_startup@0`, one too many) or, with the
underscore dropped in the `.def`, writes the wrong hint/name string.

The **hints** are reproduced too: a hint is the name's index in the vendor DLL's
sorted export-name table, and retail's import table stores the vendor's values
(`_AIL_release_sequence_handle@4` = 126 of the real Miles DLL's ~196 exports),
which makes retail itself the evidence for the vendor DLL's name table. The stub
pads its export list with `__cdecl` filler names that sort strictly between the
real decorated names until every real export sits at exactly its retail index.
The fillers never reach the image — nothing references them, so no member of
theirs is ever pulled (measured: thunk order byte-identical with and without
them) — and the synthesis re-reads the produced archive's `.idata$6` and fails on
any mismatch.

Result: the candidate's import table has **the same 16 DLLs in retail's exact
descriptor order, the same imported-name set per DLL — 456 names, none missing,
none extra — and all 449 named imports carrying retail's hint values**. Getting
the DLL order exact required naming `nafxcw`/`libcmt` *first*, since 306 of the
456 names are referenced only by MFC/CRT members; see
[`docs/linker-flags.md`](linker-flags.md) § Libraries. Still open: the order
*within* each DLL — a resolution-history artifact of the linker's
undefined-symbol worklist, not a link-line property; the mechanism and its
bounded evidence are in
`docs/patterns/idata-thunk-order-is-resolution-history.md`.

The link carries a real **`.rsrc`**, compiled from source by the era resource
compiler: toolchain r3's RC.EXE 5.00.1472.1, driven by `gruntz tool rc` over
`src/Gruntz/Gruntz.rc` + the `.ico`/`.cur` files in `src/Gruntz/res/`. All 75
resources are source; nothing is extracted. `link.exe` (built-in `cvtres`) turns
the `.res` into the section, and `gruntz rsrc check` recompiles the `.rc` and
byte-compares every payload against the retail image, in both directions, so the
"this source produces those bytes" claim is re-proven on every gated run.

The `.map` is the deliverable that feeds
[`docs/link-order-investigation.md`](link-order-investigation.md): each
function's link-assigned RVA and source object, which cross-referenced with the
retail RVAs recovers the original build order (intra-TU order = source-definition
order; cross-TU order = object link order).

## Generated vs. tracked

**Tracked:** `config/units.toml`, `config/retail/*` (the censuses, providers and
retail-derived evidence), `config/match_baseline.tsv`, `config/cleanliness/*`,
the `src/` sources with their label macros, `include/rva.h`, and the whole
`scripts/gruntz/` package.

**Generated (git-ignored):** `build/build.ninja`, `.ninja_log`/`.ninja_deps`,
and everything under `build/`.

| subdir | what it is | generated by |
|---|---|---|
| `gen/` | `claims/<unit>.tsv`, `bindings.tsv` (the serialized Model), `violations.tsv`, the delink data manifests, the data-access map + coverage artifacts, the fingerprint cache | the `labels`/`model`/`delink` edges and the verify gates |
| `objdiff/` | `base/<unit>.obj` (wine `cl`), `target-new/<unit>.c.obj` (delinked), `normalized/` (the comparison copies), `compare-new/objdiff.json` + `report.json` | the `cl`/`delink`/`normalize`/`project`/`report` edges |
| `delink/` | `named/` — raw per-symbol COFF objects straight out of vostok-delinker | the `delink` edge |
| `pdb/` | the synthesized PDB (`gruntz_named.pdb` + `.yaml`) | `gruntz.delink.pdb_synth` |
| `exe/` | `GRUNTZ.EXE` (a stable-named copy of `$GRUNTZ_EXE`), plus the candidate EXE/map/logs | `gruntz init` / `gruntz link` |
| `clangd/` | `compile_commands.json` + the lowercase include mirrors | `gruntz.graph.compdb` |
| `lib/` | the synthesised `mss32`/`smackw32` import libs | `gruntz.graph.implib` |
| `ghidra/` | the viewer payload (`knowledge.json`) and the project | `gruntz ghidra export` / `build` |
| `wineprefix/` | the Wine prefix with the MSVC 5.0 toolchain registered | `gruntz init` |

## Current status

**Run `gruntz verify status` for the live match %** — it is kept out of this doc
so it cannot go stale, and the README's score block is the banked snapshot
(`gruntz verify bank` writes it; never hand-edit between the markers).
