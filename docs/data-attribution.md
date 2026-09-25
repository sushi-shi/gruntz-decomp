# Data attribution

Retail bytes and ordered relocation targets are the authority. Source claims,
delinked objects, and generated PDBs are models; matching a synthesized data
section is not independent proof of original storage or ownership.

## Identity and source pins

Declare real storage once in its evidence-backed owner. `DATA(rva)` records
semantic identity; it does not force a linker address. Use the actual type,
initializer, storage duration, and linkage. An interior address belongs to the
owning object, not a second overlapping global.

Compiler-generated identities should come from the automatic oracles:

- String literals: content plus retail reference evidence.
- FP pool constants: payload and corroborated ordered relocation sites.
- Dynamic initialization: `RVA_DYNINIT(rva, size, owner)` on the owning datum,
  not an `RVA_COMPGEN` pin on a volatile `_$E<n>` ordinal.
- An ambiguous literal or an FP slot without a corroborated referrer may need
  `DATA_COMPGEN(rva, value)`. This is a last-resort use-site pin, admitted only
  when the claiming TU emits the payload and retail has those bytes.

See [include/rva.h](../include/rva.h) and the [label extractor](../scripts/gruntz/retail_labels/source.py)
for the actual annotation contract. Removing a pin requires a real rebuild and
inspection of referent identity; a superficially unchanged score is insufficient.

## Header statics and COMMONs

[config/retail/data_compgen.tsv](../config/retail/data_compgen.tsv) is distinct
from the `DATA_COMPGEN` macro:

- `class=common` describes header-inline local statics and their compiler guard
  bytes. No unique source TU owns the shared storage. Base-object COMMON tables
  must corroborate emission; a row with no emitting object is an error.
- `class=copy` identifies per-TU copies of header statics. The emitting TU is
  part of the identity.

Do not manufacture a guard global or source alias to make the model bind.
Linkage and all emitters decide whether state is shared or per-TU.

## Generated manifests and comparison copies

The [data manifest builder](../scripts/gruntz/delink/data_manifest.py) writes
`build/gen/delink_data_manifest.tsv` and
`build/gen/delink_data_section_manifest.tsv`. It joins model identities,
retail storage evidence, and base-object section topology. Its module header
defines both schemas; do not maintain another schema copy here.

Shared COMDATs can legitimately contribute copies in multiple target objects.
Vtable/RTTI enrollment must follow emitted definitions and associations, not
assume every leader symbol is external and at offset zero. Ambiguous identity,
overlap, or unsupported storage evidence must not be resolved by guessing.

The [normalizer](../scripts/gruntz/compare/normalize.py) and
[canonicalizer](../scripts/gruntz/compare/canonicalize.py) create disposable
comparison copies. They handle compiler-private names, COMMONs, and authorized
relocation normalization; original objects remain available for auditing.
Content equality alone does not make distinct semantic data interchangeable.

## Checks

Run inside `nix develop`, after `gruntz build`:

```sh
gruntz verify data-relocs
gruntz verify data-access
gruntz verify data-coverage
gruntz verify library-data-refs
```

Consult each command's help and [gate registration](../scripts/gruntz/verify/tiers.py)
for its current scope. Final placement and linked references additionally need
the [candidate-image checks](image-diff.md).

Do not infer original .data/.bss membership from delinked section percentages.
Retail's file-alignment tail can leave storage classification ambiguous.
Do not fill final-image gaps with fake arrays, infer extents from adjacency
without qualification, or split one object into overlapping definitions.

Historical experiments and score tables were removed from this document.
Use `git show b27b05deb:docs/data-attribution.md` for provenance, not current
tool instructions.
