# Generated symbols do not prove a pointer-versus-reference API

tags: cpp:reference cpp:const cpp:temporary cpp:ctor | asm:mov asm:lea asm:ret | topic:source-oracle topic:source-model topic:evidence-discipline
symptoms: an old rejection cites exact mangling, but the names came from reconstructed source, a synthetic PDB, or a generated-data manifest
confidence: 10/10 for the controlled Bute family

An address-width argument or return is compatible with several C++ interfaces.
When original symbol information is absent, a generated name describes the
current reconstruction; it cannot independently establish the original API.
Likewise, an address passed from a stack temporary does not select pointer
syntax over binding to a const reference.

## Controlled complete-family test

The baseline was PR #79 checkpoint `4fdd3e7a7`. Its Bute vector/range family
already matched retail. Both the pinned public Bute header and NOLF's released
header instead preserve reference-returning getters, reference defaults and
const-reference setter/item-constructor inputs. NOLF also preserves the same
individually allocated value payloads; no object-bank transfer is needed.

The real-TU A/B changed those six methods and two inline constructors together,
plus their parser consumers. The parser now binds ordinary value temporaries
and copies returned references directly. Allocation, deletion, lookup bodies,
typed payload storage, statement order, public access and static initializers
are unchanged. Two constructor pins and four local-static/guard names were
updated to the newly emitted signatures, without aliases or extra storage.

| Body / RVA | Bytes | Instructions | Calls | Branches | Returns | Relocations |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| default-argument GetVector / 1741f0 | 4e | 34 | 3 | 3 | 2 | 4 |
| static-default GetVector / 174240 | e3 | 77 | 6 | 4 | 4 | 21 |
| SetVector / 174340 | 3e8 | 312 | 30 | 40 | 1 | 51 |
| vector item constructor / 174730 | 3c | 26 | 1 | 1 | 2 | 1 |
| default-argument GetRange / 174770 | 4e | 34 | 3 | 3 | 2 | 4 |
| static-default GetRange / 1747c0 | cf | 73 | 6 | 4 | 4 | 19 |
| SetRange / 1748a0 | 404 | 326 | 30 | 40 | 1 | 51 |
| range item constructor / 174cb0 | 49 | 30 | 1 | 1 | 2 | 1 |
| Statement / 170750 | a04 | 750 | 87 | 83 | 2 | 124 |

All nine remain **100%**: their normalized instruction bytes and ordered
relocations/addends are identical to retail and to the saved baseline after
mapping only the eight reviewed signature renames. SetVector and SetRange each
have nine identical unwind actions; Statement has thirteen. Raw referent audits
of both setters and the parser find no defects. The full pinned build passes
without fresh regressions. This restores source fidelity, not nine new matches.

The old pointer API is the negative control: it also emits those same bodies.
Therefore neither its exact score nor the names it generated could reject the
surviving reference layer. The complete source family supplies the positive
selection evidence that raw address passing cannot provide.

## Reverse-use rule

1. Trace a claimed symbol to its provenance: original object/export/debug
   record, or reconstructed/generated name. Do not promote the latter to an
   independent oracle.
2. Audit the complete declaration and caller family. A reference may alter
   temporary lifetimes even when its standalone implementation ABI is flat.
3. Preserve actual allocation, layout, default-value and null-handling evidence;
   source revision differences must be adjudicated independently.
4. Compare real bytes, ordered referents and EH after updating semantic labels.
   Do not create compatibility aliases to conceal a changed claim.
5. Keep a source-backed byte-flat correction. Exact machine code does not prove
   every inferred C++ identity inside the previous reconstruction.

Candidate status and retained source differences live in the canonical lineage
rows `bute-vector-range-reference-api`, `reassess-cavector` and
`reassess-carange`. Other pointer-taking aggregate APIs require their own audit;
this experiment does not settle them.
