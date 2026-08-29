# A source-proven typed union can be the necessary base despite an inline dip

tags: cpp:union cpp:pointer cpp:inline cpp:member cpp:operator | asm:call asm:jmp | topic:codegen-idiom topic:source-model topic:source-oracle
symptoms: a surviving same-lineage class proves a typed union, but applying only that member layer moves several exact callers onto worse `/Ob1` islands
confidence: 10/10

Equal layout and equal standalone code do not make two member models equivalent
to MSVC 5.0's front end. A typed union adds member-selection structure before C2
sees an inline helper. That structure participates in C1 cost and can move
expansion cutoffs throughout a translation unit even when the union is the real
source model.

## The complete Bute composition

The surviving NOLF `butemgr.h` defines a nested
`CButeMgr::CSymTabItem`: an adjacent enum selects one of several typed pointer
arms in `data`. It also assigns values through a `const CSymTabItem& operator=`
with `break` in every switch arm and one shared trailing return. This is direct
source evidence, not a union inferred from layout.

Gruntz retail proves revision differences within that layer: it has nine values
numbered 0 through 8, no later Null/Byte/Bool arms, and pointer-taking aggregate
constructors. Those differences were retained while the nested owner, typed
union, and assignment boundary were restored.

Applying only the typed union produced a genuine exploratory descent:

| caller | union-only base | complete source composition |
|---|---:|---:|
| `SetInt` | 86.2164 | **100.0000** |
| `SetDword` | 95.7508 | **100.0000** |
| `SetFloat` | 95.7508 | **100.0000** |
| `SetDouble` | 95.0255 | **100.0000** |
| `SetRect` | 93.4468 | **100.0000** |
| `SetPoint` | 94.9172 | **100.0000** |
| `SetVector` | 82.0889 | **100.0000** |
| `SetRange` | 93.4468 | **100.0000** |
| `SetString` | 95.2445 | **100.0000** |

Nesting the item under `CButeMgr` was codegen-flat for those callers. Replacing
the invented pointer-based `CopyValue` boundary with the surviving
const-reference `operator=` then made the entire family exact. `SetString` is
exact at 0x3fc bytes, 318 instructions, 33 calls, 41 branches, one return, and
54 ordered relocations.

The assignment at 0x172040 is also exact: 0x120 bytes, 104 instructions, one
call, two branches, eight returns, and 11 relocations. This is an important
negative control. The authored source has one shared return, but C2 duplicates
it into the same eight emitted epilogues previously misread as proof of
per-arm source returns. Primary bytes alone did not select the old `CopyValue`
spelling.

Finally, changing the six retail vtable identities from the superseded global
`CButeValue` specialization to nested `CButeMgr::CSymTabItem` fixed the sole
remaining constructor residue. Its instructions were already identical; only
the ordered relocation targets named the wrong semantic class.

## What this falsifies

The earlier higher-scoring `void* pValue` transcription was a local maximum,
not the retail-selected abstraction. Its tag-directed casts described behavior
but erased the source-proven type arms and assignment boundary. The union-only
dip was not evidence against the union; it was evidence that the surviving
layer had been applied incompletely.

Likewise, an anonymous union overlay only showed that direct-member expression
shape moved C1 cost. It could not decide semantic ownership. Once the complete
surviving family is composed, the humane typed model and retail bytes agree.

## Reverse-use rule

1. Require independent evidence for the union's semantic arms: surviving
   source, complete typed consumers, or equally strong object/type records.
2. Treat a one-layer score dip as a new base. Compare the missing call sites and
   compose the adjacent surviving helpers, overloads, and ownership boundaries.
3. Do not infer source returns from duplicated C2 epilogues when a shared-return
   spelling emits the same standalone body.
4. Audit vtable and relocation identities after changing a nested class or
   template specialization; masked instruction equality does not make the old
   referent name correct.
5. Reject revision-only enum arms and APIs independently rather than rejecting
   the whole authentic class layer.

The complete import and every retained divergence are recorded in the
`nolf-bute-*` rows of `config/lithtech_lineage.tsv`.

Related:
[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md),
[`address-of-temporary-reuses-ctor-return`](address-of-temporary-reuses-ctor-return.md),
[`void-star-is-the-fake-view-laundering-channel`](void-star-is-the-fake-view-laundering-channel.md).
