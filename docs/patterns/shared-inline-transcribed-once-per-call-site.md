# A small header inline was transcribed once per call site — find them with `inline_clones`
tags: cpp:inline cpp:global cpp:call | asm:imul asm:lea asm:cmp | topic:identity topic:codegen-idiom
symptoms: the same 8-20 instruction block appears verbatim in many unrelated functions; an odd
immediate (`0x269ec3`, `0x939`) recurs at unrelated RVAs; N near-identical `static inline`
helpers with different local names in N different `.cpp`s; one string literal referenced from
dozens of TUs
confidence: 10/10

Whoever reconstructs a TU in isolation sees a compiler EXPANSION, not the source that produced
it, so a small header inline gets transcribed once per call site. It is a systematic defect, not
a one-off: `GetRandomNumber` was 14 open-coded copies of the guard-and-LCG across 11 TUs, and the
act-name find-or-create was 61 open-coded copies across 34.

`python -m gruntz.audit.inline_clones` is the sweep. Two detectors, because they fail differently.

## `--consts` — repeated unusual immediates

Histograms every numeric operand in retail `.text` that is NOT an address. The `.reloc` table is
the oracle: a value sitting at an `IMAGE_REL_BASED_HIGHLOW` site is a pointer, never a constant.
Bracketed numbers are memory displacements (struct offsets — everywhere, no identity) and are
dropped, EXCEPT when large: cl strength-reduces `x*K + C` into `lea r,[r + s*r + C]`, so the
addend of a magic multiply hides in a displacement field. That is where `GetRandomNumber`'s
`0x269ec3` (= 2531011L) lives.

    python -m gruntz.audit.inline_clones --consts --min-fns 4
    python -m gruntz.audit.inline_clones --value 0x269ec3     # every site, named

Cheap and high precision, but it finds two different things and you must tell them apart:

| the constant sits in | what it is |
|---|---|
| an arithmetic chain (`lea`/`imul`/`shl`) | a helper BODY — the real hit |
| `push` / `cmp` / `test` against one value | a shared MAGIC NUMBER — a missing enum, not an inline |

`0x939` (61 sites, 15 functions) is the second kind: it is `BRICKZ_BLOCKED_MASK`, already a named
enum. So is `0x366` (a sound id), `0xcf84f`, `0x504358`. Blind to any helper without a
distinctive constant.

## `--ngrams` — normalized instruction-sequence clustering

Every instruction is reduced to a shape token: registers alpha-renamed by first appearance INSIDE
the window (so regalloc permutation does not split a cluster), frame displacements → `[S]`,
relocated addresses → `A`, branch targets → `L`. Immediates and non-frame displacements are KEPT
— a struct offset and a magic number are exactly the discriminators. Windows of `-n` tokens are
hashed; a hash occurring in several unrelated functions is a candidate inlined body.

    python -m gruntz.audit.inline_clones --ngrams -n 8 --min-fns 5
    python -m gruntz.audit.inline_clones --show <cluster-id> -n 8

n=8 is the useful default: shorter drowns in glue, longer misses helpers whose two expansions
schedule differently. Clusters are ranked by interest x spread, where interest counts real
computation and zeroes out four boilerplate families (SEH frames, `` `scalar deleting destructor' ``,
`rep movs`/`repne scas`/`rep stos`). Without that filter the entire top of the list is
compiler-generated. Sites are attributed through `build/gen/symbol_names.csv` +
`config/retail/functions_static_libs.tsv`, and `--game-only` (the default) drops the CRT/MFC band, which
is the largest remaining false-positive family.

## The confirming evidence, and it is decisive

**Count the addresses of the string literals.** With `/Gf` (implied by `/O2`; `/GF` is off) a
literal written at 60 call sites in 60 TUs would occupy 60 addresses. A literal inside a shared
header inline is emitted as a COMDAT and the linker folds every copy to one. So:

    "A" -> 1 distinct address: 0x60a454
    "B" -> 1 distinct address: 0x60d1bc
    ... all nineteen act keys, one address each, referenced from every registrar

One address per key across a 2.5 MB image, from dozens of TUs, is not something 60 hand-written
copies can produce. That turned "these blocks look alike" into proof.

## Cross-checking against `src/`

The binary detector finds the population; the source-side dual finds what we did with it. Cluster
`.cpp` inline DEFINITIONS by normalized body — a body appearing in >1 file is a per-TU copy of
something that belongs in a header. That is how the `ActNameSlots`/`ActNameLookup`/
`ConstructGrownSlots`/`TypeLookup` family (16 copies) and the four `CellFlagsAt` spellings were
found. Watch for the same function under a LOCAL ALIAS (`TypeLookup`, `ProjTypeLookup`,
`R3Lookup`, `KSlimeLookup` are all one function) — a name-keyed grep misses those; a body-keyed
one does not.

## False-positive profile

- compiler/CRT boilerplate — filtered by `score()`, but the filter is a blocklist, so new
  families will surface at the top until they are added
- MFC inline accessors (`GetAt`, `GetNext`, `CArray::SetSize`'s `max(4, n/8)` grow) — real
  inlines, but library; nothing to fix
- a helper we ALREADY share (`zDArray::ResolveEntry`, `LOGIC_WORKER_PUMP`, `CMapMgr::CellFlagsAt`)
  — a true positive for the detector needing no fix. These are the calibration points; a run that
  does not surface them is mis-tuned
- genuine per-site duplication the devs wrote by hand — a cluster confined to ONE unit is usually
  this, so rank by UNIT spread, not site count

## Two traps when you fix one

1. **Where one expansion's result feeds several later reads, hold it in a local.** Turning each
   read into its own call re-runs the body and silently changes behaviour (the RNG advances the
   seed).
2. **An `RVA()` label binds to the NEXT function in the file.** Inserting a helper directly under
   one silently rebinds it and the function drops out of scoring entirely rather than regressing
   — the overall percentage barely moves, so it reads as noise. After any insertion or deletion
   near an `RVA()`, confirm the function still appears in `report.json`.

And see [inline-expanded-twice-costs-a-register](inline-expanded-twice-costs-a-register.md):
the shared definition frequently has to be a MACRO, not an inline function.
