# Source-Derived Debug Symbols for the Fake PDB

## Current Flow

Source-derived information reaches Ghidra and the delinker through two mostly
separate paths:

- `scripts/gruntz/build/ghidra_metadata_generate.py` reads `src/` and
  `structure/` with clang and emits `build/gen/structs.json` and
  `build/gen/enums.json`. `scripts/gruntz/ghidra/apply.py` applies those to the
  Ghidra database.
- `scripts/gruntz/build/labels.py` reads `src/` `@address`/`@data`
  annotations, joins them to base `.obj` symbols, and emits
  `build/gen/symbol_names.csv`.
- `scripts/gruntz/build/synth_pdb.py` uses Ghidra `functions.csv`/`symbols.csv`
  plus `symbol_names.csv` to synthesize a PDB for `vostok-delinker`. That PDB
  is intentionally sparse today: function/data symbols plus line records for
  file bucketing.

There is no current path for function-scoped debug symbols such as parameters,
stack locals, or optimized register locals.

## Probe

`src/Io/FileStream.cpp` was compiled twice:

```text
/nologo /c /O1 /MT /GX
/nologo /c /O1 /MT /GX /Z7
```

The `/Z7` object adds `.debug$S`, `.debug$F`, and `.debug$T` sections. LLVM sees
the old CodeView signature (`Magic: 0x2`) but does not parse this VC5 format
through `llvm-readobj --codeview`; `cvdump` is not available in the environment.

The useful records are old CodeView ST records:

- `S_GPROC32_ST` (`0x100b`) for function scopes.
- `S_REGISTER_ST` (`0x1001`) for register-resident locals.
- `S_BPREL32_ST` (`0x1006`) for frame/base-pointer-relative args and locals.
- `S_END` (`0x0006`) for scope close.

Decoded examples from `CFileIO::Open`:

```text
REG  this          reg=0x12 type=0x101c
BP   lpszFileName  off=+8   type=0x470
BP   nOpenFlags    off=+12  type=0x75
BP   pError        off=+16  type=0x403
BP   dwAccess      off=+12  type=0x22
BP   sa            off=-12  type=0x1038
BP   szPath        off=-272 type=0x1035
```

The function COMDAT bytes were identical between no-debug and `/Z7` objects for
the probe:

```text
functions compared: 12
mismatches: 0
```

## Roadmap

### 1. Stop using `engine_labels.csv` as an authority

`engine_labels.csv` should be an optional extension/helper file for future
manual mapping, not a driver of canonical names, namespaces, prototypes, or
this-types. If present, it may still seed candidate Function objects and
advisory comments for unknown code; nothing in the canonical source-derived
pipeline should depend on the file existing.

Authoritative inputs should be:

- `src/` annotations and definitions for matched source-owned functions;
- generated compiler symbols from the source objects;
- generated clang layouts from `src/` and `structure/`;
- explicit exported review data from Ghidra when the user intentionally maps
  something in the database.

### 2. Reliably apply `this` and argument types for `src/` functions

For functions written in `src/`, the binding should be based on the compiled
source symbol, not `engine_labels.csv`.

Concrete tasks:

- Parse `build/gen/symbol_names.csv` function rows and demangle or parse MSVC
  mangled names to recover class, method kind, calling convention, return type,
  and parameter types where possible.
- Resolve recovered class names to generated `/Gruntz/<Class>` structures from
  `structs.json`.
- Apply explicit function signatures in Ghidra, including `this: Class *` for
  matched member functions.

### 3. Parse exact-match debug symbols and apply locals

The `/Z7` object debug records are trustworthy for the target only after the
function is byte-exact in the current objdiff report.

Concrete tasks:

- Add a `/Z7` debug-object build path, preferably separate from the primary base
  objects until `/Z7` has been validated across all flag families.
- Implement a narrow old-CodeView parser for VC5 `.debug$S` and `.debug$T`.
- Join parsed debug records by compiled symbol to `symbol_names.csv`.
- Join the result to `build/objdiff/report.json` and keep only current 100%
  exact matches.
- Feed the exact-match subset into the existing generated-target-PDB path where
  practical, or into an equivalent `synth_pdb.py`-owned sidecar emitted from the
  same data.

### 4. Export user Ghidra edits back to source-controlled data

The Ghidra database should become a rebuildable cache, not the source of truth.
When the user maps functions, structs, globals, locals, comments, or other
metadata in Ghidra, those edits should be exportable into tracked CSV/JSONC files
and then re-applied on a fresh database.
