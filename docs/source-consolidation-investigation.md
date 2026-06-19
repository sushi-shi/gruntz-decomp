# Investigation: collapsing the matching "sources of truth" into `src/`

**Goal (user):** stop hand-maintaining the matched-label registries and the
`structure/` layout scaffold as separate artifacts. Make `src/` the single
source of truth, so that generating labels and applying them to the Ghidra DB
(→ fake PDB) is mechanical and cannot drift.

This doc maps what exists today, where it duplicates, and a concrete plan to get
to "annotate the source, regenerate everything."

> **Where the code lives now (post-reorg).** The pipeline is driven by one CLI,
> **`gruntz.py`** (subcommands `build`, `ghidra-refresh`, `init`, `labels`,
> `structs`, `status`, `todo`). The steps were moved into a package after the
> prose below was written; map the conceptual names used here to:
>
> | prose name | actual path |
> |---|---|
> | `gen_labels.py` | `scripts/gruntz/build/labels.py` |
> | `ghidra_metadata_generate.py` | `scripts/gruntz/build/ghidra_metadata_generate.py` |
> | `synth_pdb.py` / `delink_target.py` / `cc_wrap.py` | `scripts/gruntz/build/{synth_pdb,delink,cc_wrap}.py` |
> | `apply_ghidra_enrichment.py` | `scripts/gruntz/ghidra/apply.py` (+ `export.py`) |
> | `annotate_addresses.py` | one-shot `@address` migrator — **removed** after it seeded all `src/` annotations (the annotations live in `src/`) |
> | `configure.py` | repo root (generates `build.ninja`) |
>
> And **`build/gen/symbol_names.csv` no longer exists** — it is GENERATED at
> `build/gen/symbol_names.csv` by `gruntz labels`. Where the prose says
> "`build/gen/symbol_names.csv`", read the generated file.
>
> **The label JOIN has since changed (LLVM-IR migration).** The prose below
> describes the original mechanism: a `// @address:` comment + a POSITIONAL join
> (the comment binds to the nearest clang-AST definition below it by line). That
> positional join was fragile (a header inline def could steal a nearby address).
> Today matched code/data carries its address as a clang `annotate` ATTRIBUTE
> (`RVA()`/`DATA()`/`SYMBOL()` macros in `src/rva.h`), and `labels.py` reads
> functions from **LLVM IR** (`@llvm.global.annotations` pairs the mangled symbol
> DIRECTLY with the annotation — no join). `DATA` still uses the AST VarDecl
> (clang drops an `extern`'s annotation from IR). The `llvm-nm` authority check
> below is unchanged. The vendored zlib C TUs keep PRISTINE source; their
> rva→symbol map lives in `config/zlib_labels.csv` (emitted directly — their
> static/K&R functions drop from IR when unused). See
> `docs/build-system.md` ("Adding a unit") and `scripts/gruntz/build/labels.py`.

---

## 1. The data that drives matching today (6 artifacts)

| # | Artifact | Shape | Role | Hand-maintained? |
|---|----------|-------|------|------------------|
| 1 | `build/gen/symbol_names.csv` | `rva,name,unit` (100 rows) | THE byte-matched map. `synth_pdb.py` overlays it onto Ghidra's `functions.csv` → names the delinked `<unit>.c.obj` so objdiff pairs it to the base `<unit>.obj`. | **Yes** |
| 2 | `config/units.toml` | per-TU `unit,source,flags` profile (23 units) | Build manifest. `configure.py` → `build.ninja` + objdiff project. | Yes (legit) |
| 3 | `src/` | `@stub` metadata plus empty function bodies for unresolved labels | Comprehension/attribution + match-queue fuel. Harvested from tomalla + RTTI; `apply_ghidra_enrichment.py` applies names/protos to the Ghidra DB; `gen_match_queue.py` ranks it. | Partly (harvested + appended) |
| 4 | `structure/*.h` | 39 C++ headers, `@offset`/`@vftable`/`@size`/`@rtti` annotations | Engine-wide layout/enum scaffold (231 RTTI classes; tomalla-ported + hypotheses). Comprehension only — **not compiled**. | **Yes** |
| 5 | `scripts/gruntz/ghidra/apply.py` (`STRUCTS`/`ENUMS`, ~300 lines) | Python literals of struct fields + enums | Defines structs/enums in the Ghidra DTM and applies them as `this`-types. | **Yes** |
| 6 | `src/**/*.{cpp,h}` | the actual matched C++ (19 files, 23 TUs) | Compiled by `cl` → base `<unit>.obj`. RVAs, symbols, sizes, match-% all live in **prose comments**; field layouts live in **prose comments** in the `.h`. **Zero machine-readable annotations.** | Yes (the real work) |

`units.toml` (#2) is **not** duplication — it carries genuine per-TU build state
(the load-bearing per-TU flag selection — e.g. FileStream's `/O1` vs the global
`/O2`, via the `[flags]` profile it names). It stays. Everything else overlaps.

---

## 2. The duplication, measured

- **#1 ↔ #3 ↔ #6:** 46 of the 100 `symbol_names.csv` RVAs also appear in
  `src/Stub/`, and *all* of them are also described in prose at the top
  of their `src/` file. So a matched function's `(rva, name, class, prototype)`
  lives in **three** places.
- **#4 ↔ #5:** `apply_ghidra_enrichment.py` hardcodes **56 struct entries** that
  re-encode the layouts in the **39** `structure/*.h` files. Its own docstring
  says it is "reproducible from … `structure/`" — but it is a hand-copy, not a
  read.
- **#4 ↔ #6:** graduated classes are in both trees. `structure/` headers exist
  for at least `cgameapp`, `cgamewnd`, `cgruntzapp`, `registry_helper`,
  `rezsync` — all now also reconstructed in `src/`.

### Exhibit A — `RegistryHelper` has 3 layouts, and they disagree

| Source | Field naming | 0x1c field |
|---|---|---|
| `structure/utils/registry_helper.h` | `m_isInitialized,m_hKey,m_hKey2..6` | `char[256] m_szParam2` (+ `m_szParam4` @0x11c) |
| `src/Utils/RegistryHelper.h` | `m_0,m_4,m_8..m_18` (placeholders) | `char[0x100]` buffer (single) |
| `apply_ghidra_enrichment.py:239` | copy of structure/ names | `char[256] m_szParam2` |

The `src/` reading (proven against the matched code) and the tomalla scaffold
disagree on both names and field count. Three copies → three opinions. This is
the failure mode the consolidation must kill.

---

## 3. End-state: one annotated source, two flows

Legend:  `(H)` hand-authored    `(T)` tool/generated

### Flow 1 - building the fake PDB (target side -> objdiff)

```
  (H) src/ (+ @address comments)
       |
       +-- clang -ast-dump=json -fparse-all-comments --+
       |      (-> @address  +  candidate MS mangledName) |
       | cl /O2 /MT                                      v
       v                                        gen_labels.py:
  (T) base/<unit>.obj --llvm-nm (symbol set)-->  clang mangledName
       |                                          INTERSECT nm(base obj)
       | (also objdiff's                          member  = exact cl symbol
       |  recompiled side,                         miss    -> @symbol (??_G)
       |  bottom)                                       |
       |   (H) units.toml (unit <- path) ---------------+
       |                                                |
       |                                                v
       |                          (T) build/gen/symbol_names.csv
       |                              (rva -> mangled-name, unit)
       |                                                |
       |   (H) GRUNTZ.EXE (section bounds) --+          |
       |   (T) Ghidra functions.csv (rva,sz)-+          |
       |   (T) Ghidra symbols.csv  (data) ---+          |
       |                                     v          v
       |                              synth_pdb.py (yaml2pdb + DBI patch)
       |                                              |
       |                                              v
       |                                       (T) fake PDB
       |                                              |
       |                                              v  + GRUNTZ.EXE .text bytes
       |                                       vostok-delinker
       |                                              |
       |                                              v
       |                                  (T) target/<unit>.c.obj
       |                                              |
       +-------------------------> objdiff <----------+
                                      |  parses COFF (symbols+sections+relocs);
                                      |  pair by name; no debug info / no /Z7
                                      v
                                 match % report   (the arbiter)
```

### Flow 2 - improving Ghidra (comprehension side)

```
  (H) src/**                 (H) structure/**     [compilable placeholder C++]
       |                          |
       | - @address (rva)         | - class/method decls (names, params)
       | - class layouts          | - struct decls (fields, bases, padding)
       +------------+-------------+
                    v
        clang  (parse-only, --target=i686-pc-windows-msvc)
          - -ast-dump=json         -> names, params, calling conv
          - -fdump-record-layouts  -> every field offset   [no @offset]
                    |
                    v
        ghidra_metadata_generate.py / gen_ghidra.py
                    |
       +------------+------------+
       v            v            v
  functions.json  structs.json  enums.json     (rva <- @address)
  name,class,     name,size,    name,
  params,cc       fields+off    members
       |            |            |
       +------------+------------+
                    |
                    v
  (H) src/Stub/ --> apply.py (ghidra_metadata_apply)   <-- (T) library_labels.csv
      (shrinking backlog)   [headless PyGhidra, idempotent]   (FID/FLIRT)
                    |
                    v
        (T) enriched Ghidra DB
            names - namespaces - typed args - struct this-types - enums
                    |
                    | re-export
                    v
        functions.csv / symbols.csv  -->  feeds Flow 1 (better inventory)
```

Both flows read the same `src/`: Flow 1 derives the *mangled* name from the base
obj, Flow 2 derives the *readable* identity + layout from clang. Flow 2's enriched
DB re-exports the `functions.csv`/`symbols.csv` that Flow 1 consumes; Flow 1's
objdiff `%` is the arbiter that says whether an `@address`/size is right.

Key properties:
- `build/gen/symbol_names.csv` stops being hand-written. It becomes
  `build/gen/symbol_names.csv`, regenerated on every `configure.py`/`ninja` run.
  The mangled `name` is **read from the base `<unit>.obj`** (the exact `cl`
  output objdiff pairs on), never hand-typed — so a name mismatch is
  *structurally impossible* (today a CSV typo silently yields a 0% unit).
- **No offset annotations anywhere.** A struct's layout is its compilable
  declaration; `clang -fdump-record-layouts` (MS-ABI target) reports every field
  offset. `apply_ghidra_enrichment.py` stops embedding layouts and consumes the
  generated JSON, so Ghidra structs and the `src/`/`structure/` headers can never
  disagree.
- `@address` is the **only** manual per-function annotation (the retail RVA lives
  in no object); `@symbol` survives only as an optional override for
  compiler-generated thunks.
- `structure/` is converted to the same compilable placeholder style and shrinks
  as classes graduate into `src/` (see §5).

---

## 4. Proposed annotation scheme

The only machine-read annotation is `@address` on functions. Layout carries no
annotation at all — the compilable struct declaration *is* the layout. Above each
matched function:

```cpp
// @address: 0x1bf200            ; .text RVA in GRUNTZ.EXE (VA = 0x400000+rva)
// @status:  matched             ; optional: plateau-78
int CFileIO::Open(LPCSTR, UINT, void*) { ... }
```

### Why no `@symbol` — derive it from the base obj

The mangled name (`?Open@CFileIO@@QAEHPBDIPAX@Z`) is not hand-written. objdiff
pairs base ↔ target **by symbol name**, and the exact name `cl` emits already
exists in the base `build/objdiff/base/<unit>.obj` we compile every build. So
`gen_labels.py` *derives* it. The only fact a human supplies is `@address` (the
retail RVA — present in no object); `unit` comes from the file path via
`units.toml`. What remains is the **join**: which obj symbol owns a given
`@address`.

**The join — clang `mangledName` ∩ `llvm-nm`(base obj). [VALIDATED]**
One `clang -ast-dump=json -fparse-all-comments --target=i686-pc-windows-msvc`
pass yields, per defined function, both its `@address` (the attached comment) and
a candidate MS-ABI `mangledName`. `gen_labels.py` accepts that name **iff it is a
member of `llvm-nm`(base obj)** — the set of exact `cl` symbols. A member is, by
construction, the unique symbol for that function (mangled names encode the full
signature, so no collision is possible); a miss can only *fail to match*, never
mis-pair. So the obj stays the authority and clang is just an exact-string
candidate generator — no demangling, no typedef normalization, no overload
reasoning.

Spike (clang 19, this repo): for `int CFileIO::Open(LPCSTR, UINT, void*)` clang
emitted **`?Open@CFileIO@@QAEHPBDIPAX@Z`** — byte-identical to the real VC5
symbol in `symbol_names.csv` — and `-fdump-record-layouts-complete` placed
`m_handle`@4 / `m_open`@8 / vptr@0 exactly. clang's MS-ABI mangling/layout are
*reliable but not contractually VC5*, which is why the `nm` membership check
gates every emitted name.

**`/Z7` is not needed** and is dropped. It would only have served as a fallback
join (symbol↔line) for clang misses, but clang covers ordinary functions and the
sole residue (`??_G` thunks) is handled by `@symbol`. objdiff itself never needed
`/Z7`: it parses the base obj as plain COFF (symbol table + COMDAT sections +
relocations), so removing `/Z7` leaves its input byte-identical. (If a future TU
exposes a real clang↔VC5 divergence, the `/Z7` line-map via `cvdump` is the
documented escape hatch — see §8 — but it is not built now.)

### The exceptions — destructors and compiler-generated symbols

The clang-19 spike over `FileStream.cpp` matched **8/10** real symbols exactly
(all 6 methods + both ctors). Two cases need help, both handled by `gen_labels`:

- **Destructor (clang blind spot).** clang's AST `mangledName` for `~CFileIO`
  reports the *`vbase dtor`* variant (`??_DCFileIO@@QAEXXZ`), not the real
  `??1CFileIO@@UAE@XZ`. So the clang candidate *misses* the obj — and `gen_labels`
  resolves it via the **undname-identity fallback**: among the base obj's symbols,
  the one demangling to the plain `Class::~Class` is `??1` (excluding the
  deleting-dtor thunks). Verified logic; needs a base obj to exercise.
- **Source-less thunks.** `??_G`/`??_E` (scalar/vector deleting dtors), `??_7`
  (vtables), EH funclets have *no* source declaration at all (`??_GCFileIO@@…` is
  already a matched row). They take an explicit `@symbol:` in their own comment
  block (no definition required) — verified: a standalone
  `// @address: …` + `// @symbol: ??_G…` emits the row correctly.

`@symbol` thus stays an *optional* override for this minority; the destructor is
auto-resolved when a base obj is present. `gen_labels` reproduced all 10 real
`filestream` rows in the end-to-end test (dtor + `??_G` via `@symbol`).

### Field offsets — no annotation; the declaration is the layout

There is **no `@offset`**. A compiled struct already accounts for every byte 0..
`size` (it has to, or `this->m_handle` wouldn't emit `[ecx+4]` and the code
wouldn't byte-match), so `clang -fdump-record-layouts` (target
`i686-pc-windows-msvc`) reports every field offset exactly. The established
`src/` style already does this:

```cpp
class CFileIO : public CObject {
    HANDLE m_handle;   // clang: +0x4
    int    m_open;     // clang: +0x8
};
// unknown gaps → explicit padding, exactly as src/ does today:
char _pad10c[0x40];    // GameInfo, src/Wap32/Wap32.h
```

Rules that keep clang's layout equal to retail:
- Placeholder for an unknown 4-byte slot is `int`/`void*`; an unknown blob is
  `char[N]`. **Never** a wider/8-aligned type (`double`, `__int64`) you haven't
  confirmed — that can bump struct alignment and shift offsets.
- Declare the same bases + `virtual` as retail (vptr lands at +0, base subobject
  first under MS ABI) — `src/Rez/RezMgr.h` already does (`virtual ~CRezItmBase()`).
- A wrong-sized placeholder is self-catching: it throws off `sizeof` and the next
  field's offset, which breaks the byte-match. `ghidra_metadata_generate.py` also asserts
  clang `sizeof` == the known object size (`operator new(0x38)` etc.).

For placeholder structs (only `int`/`void*`/`char[]` + single inheritance +
vptr) clang's MS-ABI record layout is unambiguous and reliable — the edge cases
(bitfields, virtual/empty bases, 8-aligned members) are exactly what the
placeholder discipline avoids.

---

## 5. Where `structure/` goes — graduate-on-match (CHOSEN)

`structure/` is engine-WIDE (231 classes; most are name-only or `@todo`
hypotheses) while `src/` is only the ~20 matched/in-progress TUs. A blunt
`git mv structure/* src/` would dump hundreds of non-compiling stubs into the
build tree.

**Decision: graduate-on-match, and convert `structure/` to compilable headers.**
`structure/` headers are rewritten in the same compilable placeholder style as
`src/` (real bases + `virtual`, `int`/`void*`/`char[]` members, explicit padding;
**no `@offset`/`@todo` prose layout**). They are *not* added to `units.toml` (not
in the matching build) — they only need to **parse + lay out under clang**, so
`ghidra_metadata_generate.py` can read their offsets via `-fdump-record-layouts` exactly like
`src/`. This makes `structure/` and `src/` one uniform grammar and one extractor.

`structure/` still serves as the comprehension source for classes **not yet
matched**. The moment a class is byte-matched, its (already compilable) layout
graduates into `src/<Module>/*.h`, joins the build, and its `structure/` header
is **deleted**. `structure/` monotonically shrinks; once empty, the directory
disappears. End-state: everything in `src/`, reached incrementally, build tree
never holding non-compiling stubs.

A class we know only partially (a field at `+0x80`, contents of `0x10..0x80`
unknown) is still expressed compilably: `char _gap[0x70]; int field80; …`. The
padding size *is* the offset arithmetic — accepted as the cost of "no `@offset`."
Because these are comprehension-only (not byte-matched), a miscount only
mislabels a Ghidra field, never breaks a match, and the `sizeof` gate catches
gross errors.

Regardless of when classes graduate, **`apply_ghidra_enrichment.py`'s
`STRUCTS`/`ENUMS` literals are deleted** and replaced by a read of generated
JSON. `ghidra_metadata_generate.py` reads the union of `src/**/*.h` (matched, authoritative)
and the converted `structure/*.h` (unmatched comprehension), with `src/` winning
any overlap. That removes the third `RegistryHelper`-style copy.

`src/Stub/` (#3) stays as the **backlog/staging** file for functions not
yet in `src/` (it legitimately describes code that has no source body yet). As a
function graduates into annotated `src/`, its row leaves `src/Stub/`.
The Ghidra apply consumes `@stub` metadata from real source files plus the remaining unresolved `src/Stub/` tree.

---

## 6. Migration plan (ranked, each step independently shippable)

1. **`ghidra_metadata_generate.py` (clang layouts → JSON), de-Python `apply_ghidra_enrichment.py`.**
   Convert the 56 `STRUCTS`/`ENUMS` literals into compilable placeholder headers
   (the matched ones already exist in `src/`; the rest become converted
   `structure/` headers), read field offsets via `clang -fdump-record-layouts`,
   emit JSON, and have the enrichment script consume it. Kills the worst
   triplication (#4↔#5) with no change to the matching build. *Highest value.*
2. **Adopt `@address` in `src/` + `gen_labels.py`.** Annotate the 19 `src/` files
   (RVA only); generate `build/gen/symbol_names.csv` by joining `@address` to the
   base-obj symbols (`/Z7` line map, or `llvm-undname` fallback); point
   `configure.py`/`delink_target.py` at the generated file; delete
   `build/gen/symbol_names.csv`. Regression gate: objdiff totals unchanged.
3. **Convert `structure/` to compilable placeholder headers (no `@offset`).**
   Rewrite the tomalla `@offset`/`@todo` prose into real declarations + padding so
   clang lays them out; `ghidra_metadata_generate.py` then reads `src/` ∪ `structure/`
   uniformly (`src/` wins overlaps), resolving `RegistryHelper`-style
   disagreements in favor of the compiled `src/` reading.
4. **Graduate matched classes out of `structure/`.** When a class is in `src/`,
   delete its `structure/` header. `ghidra_metadata_generate.py` prefers `src/`, so this only
   removes dead copies; `structure/` shrinks toward empty.
5. **Trim `src/Stub/` to backlog-only** (drop rows now covered by
   annotated `src/`); make the Ghidra apply union the two sources.

After step 2 the user's headline ask is met: regenerate labels from `src/` and
feed the fake-PDB pipeline with one command, no hand-edited CSV. Steps 1+3
deliver the no-`@offset` Ghidra enrichment.

---

## 7. Decisions (resolved)

- **Fold shape:** graduate-on-match (§5). `structure/` stays as the unmatched-
  class comprehension source and shrinks to nothing as classes graduate into
  `src/`. No `src/_engine/` subtree.
- **Layout source:** **no `@offset` anywhere.** A struct's compilable placeholder
  declaration *is* its layout; `clang -fdump-record-layouts` derives offsets.
  `structure/` is converted to the same compilable style. `@address` is the only
  manual layout-adjacent annotation; the only struct annotation is none.
- **Sequencing:** implementation underway (this turn). The doc + flow diagrams
  are the spec; scripts/source land incrementally per §6.
- **`@symbol` source:** the mangled name is **derived from the base `<unit>.obj`**,
  not hand-written (§4). `@address` is the only per-function annotation; `unit`
  comes from the path. The join is **clang `mangledName` ∩ `llvm-nm`(base obj)**
  (VALIDATED — clang reproduced the real VC5 symbol). `@symbol:` survives only as
  an *optional* override for compiler-generated thunks (`??_G`). **`/Z7` dropped**
  (objdiff parses COFF natively; clang covers the join); `cvdump` line-map kept
  only as a documented escape hatch if a future TU shows a clang↔VC5 divergence.

---

## 8. Tooling / flake prerequisites

The generators only read; they add no *build* deps. The one new dependency is the
`clang` driver, now central to both flows:
- **Already provided:** `pkgs.llvm` → `llvm-nm` / `llvm-undname` / `llvm-readobj`
  / `llvm-pdbutil`. `llvm-nm` is the authority set for the label join.
- **Added — `clang` driver (`pkgs.clang`, done):** the unified front-end.
  `-ast-dump=json -fparse-all-comments --target=i686-pc-windows-msvc` →
  `@address` + candidate `mangledName` + prototypes; `-fdump-record-layouts-complete`
  → field offsets. `pkgs.clang-tools` ships clangd but **not** the driver, hence
  `pkgs.clang`. Parse-only — a reader, like clangd. (If the cc-wrapper's default
  include paths pollute the MSVC parse, switch to `clang-unwrapped` + `-resource-dir`.)
- **Dropped — `/Z7`:** not added to the base-compile rule. objdiff parses the
  base obj as plain COFF; the label join is clang∩nm. `/Z7` + a CV4 reader
  (`cvdump`, not in nixpkgs) remain a *documented escape hatch* only — built solely
  if a future TU exhibits a real clang↔VC5 mangling divergence.
- **Build flag:** add `/Z7` to the base-compile rule (`scripts/gruntz/build/cc_wrap.py` /
  `configure.py`). Codegen-neutral per `docs/linker-flags.md`, so the matching
  obj is unaffected; only the debug stream (consumed by Join A) appears.

---

## 9. What still needs manual maintenance

After consolidation the hand-maintained surface collapses to genuine RE findings
plus two files that shrink to nothing.

**Irreducible — real reverse-engineering facts, not derivable from anything:**

| What | Where | Why it can't be generated |
|---|---|---|
| **`@address` per function** | comment in `src/` | The retail RVA is in no object (the EXE ships no debug). It *is* the RE finding. One line per matched function. |
| **The `src/` code + class layouts** | `src/**/*.{cpp,h}` | The matched bodies *and* the placeholder fields + `char[]` padding that now carry layout. The product, not overhead. |
| **Per-unit flags / status** | `config/units.toml` | The `/O1`-vs-`/O2` choice (e.g. FileStream needs `/O1`) is itself an RE finding per TU. |
| **Retail function boundaries / sizes** | the Ghidra DB (curated) | `byte_size` drives the carve; Ghidra proposes it but a human fixes mis-split/merged boundaries. Semi-manual analysis. |

**Transitional — shrinks monotonically to empty:**

| What | Where | Empties when |
|---|---|---|
| Comprehension class layouts | `structure/**` (now compilable) | every class has graduated into `src/` |
| Unsourced-function backlog | `src/Stub/` | every function has a `src/` body |

**Occasional / optional:**
- **`@symbol`** override — only for compiler-generated thunks (`??_G`/`??_E`/`??_7`)
  with no source declaration. A handful.
- **`@status`** tags (`matched`/`plateau-NN`) — informational only.

**No longer touched (these were the drift sources):**
- `build/gen/symbol_names.csv` -> generated `build/gen/symbol_names.csv`
- `apply_ghidra_enrichment.py` `STRUCTS`/`ENUMS` (~300 lines) -> generated JSON
- field offsets -> clang `-fdump-record-layouts`; **no `@offset`**
- mangled names -> read from the base obj; **never hand-typed**
- matched-function rows in `src/Stub/` -> derived from `src/`
- `config/library_labels.csv` -> tracked FID output (regen: `scripts/analysis/fid_generate.py`)

Steady state: per *matched function*, ~one `@address` line (plus the code); per
*class*, zero offset bookkeeping — the declaration carries it.

---

## 10. Verification (real toolchain, this session)

Built and run inside the flake (`nix develop` / `.#build`). Generators live in
`scripts/gruntz/build/ghidra_metadata_generate.py` + `scripts/gruntz/build/labels.py`; clang reached via
`$GRUNTZ_CLANG` (the **unwrapped** clang — the cc-wrapper injects host flags that
break `--target=i686-pc-windows-msvc`).

- **`ghidra_metadata_generate.py`** on real `src/Io/FileStream.cpp` + `src/Rez/RezMgr.cpp`:
  `CFileIO` = `size 0x10, m_handle@4, m_open@8, m_name@12`; `CRezItm`/`CRezDir`/
  `CRezItmBase` = `0x24/0x68/0x10` — matching the header-documented layouts.
- **`gen_labels.py`** against the **genuine `cl` `filestream.obj`** (wine MSVC 5.0,
  built via `setup-toolchain.py --smoke` + `ninja`): reproduces **all 10**
  `filestream` rows byte-identically to the hand-written `build/gen/symbol_names.csv`.
  Methods + ctors via clang `mangledName` ∩ `nm`; the destructor via the canonical
  `??1…@XZ` fallback; `??_GCFileIO@@UAEPAXI@Z` via an `@symbol` anchor.
- **`cl` obj symbol types** confirm the design: every `cl` function is `nm` type
  `T`; the vtable `??_7CFileIO@@6B@` is `R` (correctly excluded as data); and the
  real obj carries **none** of clang's stand-in artifacts (`@4HA` guards,
  `?dtor$5@`/`___ehhandler$` funclets) — so the label set is clean.

Three bugs were found and fixed by running in-env (not visible on paper):
1. wrapped clang breaks the cross-target parse → use `clang-unwrapped`.
2. `@symbol` override leaked across comment blocks → block-scoped scanner.
3. `nm` data symbols + clang funclets made the dtor fallback ambiguous →
   code-symbol filter + canonical-mangled `??1` match.

**All 23 TUs annotated + the label pipeline fully wired.**
`scripts/annotate_addresses.py` inserted 90 `@address` comments (FileStream's 10
were already hand-placed); the only row needing an `@symbol` anchor was
filestream's `??_G` thunk. **Round-trip gate passed:** regenerating
`build/gen/symbol_names.csv` from the in-src `@address` annotations + the base
objs is **byte-identical to all 100 rows** of the original `build/gen/symbol_names.csv`.
`configure.py` now emits a `gen_labels` ninja rule (`src @address` + base objs →
`build/gen/symbol_names.csv`); the delink consumes that, not the hand CSV.

**Full `ninja` + objdiff pipeline — verified with generated labels.**
``gruntz build`` (`configure → gen_labels → cl base objs → delink → objdiff`)
ran clean over all 23 units: **57/100 functions exact, 99.18% fuzzy**, with
`filestream 6/10` matching main's baseline. Because the generated CSV is
byte-identical to the hand CSV, objdiff totals are unchanged by construction — and
the live run confirms it. Wine prefix initialised at `build/wineprefix`.

**Part 2 (Ghidra enrichment).** `apply_ghidra_enrichment.py` is refactored to
read `build/gen/structs.json` + `enums.json` (from `ghidra_metadata_generate.py`) and
`build/gen/symbol_names.csv`, *preferring* generated definitions and falling back
to its hardcoded `STRUCTS`/`ENUMS` only for comprehension classes whose
`structure/` header is not yet compilable — the graduate-on-match path; the
fallback shrinks to zero as `structure/` is converted (§5). Running the headless
apply was **not** executed this session (it would mutate the main checkout's
Ghidra DB; needs an isolated project copy). Converting the 39 `structure/` headers
to the compilable placeholder style is the remaining incremental work — the
mechanism is identical to the already-verified `src/` headers `ghidra_metadata_generate.py`
reads today.

**Full fake-PDB loop — verified end-to-end.** Reusing the Ghidra exports +
break-reloc'd EXE from the main checkout, delinking the retail EXE with the
**generated** `build/gen/symbol_names.csv` produced a `target/filestream.c.obj`
that is **byte-identical** to the one main produced from the hand-written
`build/gen/symbol_names.csv`:

```
gen_labels (src @address + cl obj) -> build/gen/symbol_names.csv
  -> synth_pdb -> fake PDB -> vostok-delinker -> target/filestream.c.obj
  == main/build/objdiff/target/filestream.c.obj   (cmp: identical)
```

Since objdiff pairs base ↔ target, an identical target obj means an identical
match result (main's baseline: `filestream` 6/10 exact, 98.29% fuzzy). So the
generated labels are a proven drop-in for the hand CSV across the whole Part-1
pipeline. (The Ghidra *enrichment* side — Part 2, `apply_ghidra_enrichment.py`
reading generated JSON — is the remaining piece; the struct/enum JSON is already
generated by `ghidra_metadata_generate.py`.)
