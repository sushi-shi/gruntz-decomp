# A typed union can preserve C2 bytes while changing C1 inline decisions

tags: cpp:union cpp:pointer cpp:inline cpp:member | asm:call asm:jmp | topic:codegen-idiom topic:source-model
symptoms: standalone inline helpers remain exact after replacing a tagged `void*` with a typed union, but many callers simultaneously cross different /Ob1 cutoffs
confidence: 10/10

Equal layout and equal standalone code do not make two member models equivalent
to MSVC 5.0's front end. A typed union adds member-selection structure before C2
sees the helper. That structure participates in the C1 cost and can move /Ob1
expansion cutoffs throughout a translation unit even when C2 emits identical
bytes for each out-of-line helper.

## The controlled Bute case

`CButeValue` is a tagged heterogeneous heap holder: `type` selects the allocated
object type for every construction, access, copy, and deletion. Its earlier
model was the eight-byte pair:

```cpp
ButeType type;
void* pValue;
```

A cleanup replaced `pValue` with a `ButeValuePayload` union of nine typed
pointers. The ABI and offsets stayed unchanged. Several standalone functions
also remained exact, which made the union look harmless. It was not harmless to
their inline callers:

| caller | typed union | direct `void*` member |
|---|---:|---:|
| `SetInt` | 86.2164 | **100.0000** |
| `SetDword` | 95.7508 | **100.0000** |
| `SetFloat` | 95.7508 | **100.0000** |
| `SetDouble` | 95.0255 | **100.0000** |
| `SetRect` | 93.4468 | **100.0000** |
| `SetPoint` | 94.9172 | **100.0000** |
| `SetVector` | 82.0889 | **100.0000** |
| `SetRange` | 93.4468 | **100.0000** |
| `SetString` | 77.0314 | **81.7868** |

The module moved from 102/126 to 110/126 exact. `CButeValue::CopyValue`,
`CButeValue::~CButeValue`, `ButeGroup_Apply`, and `ButeValueTeardown` are exact
with the direct member and tag-selected `static_cast<T*>` operations.

An anonymous overlay separated storage layout from expression shape: retaining
the typed union as a second view while spelling the inline operations through a
direct `pValue` member recovered `SetString` 81.7868 and the exact setter family.
A named `payload.m_value` union arm instead sent `SetString` to 61.85. Removing
the diagnostic overlay and retaining only `pValue` preserved the recovered
family. The one current exact-count difference between overlay and final model
is an already-banked-100 caller perturbed by the removed declarations; it is TU
state, not evidence for two overlapping semantic views.

## Why the `void*` is real here

This is not permission to launder an invented view through `void*`. The storage
itself is heterogeneous, and the adjacent tag is consulted at every operation.
Each allocation stores a different real type into the same slot, and each read
or delete recovers exactly the type selected by that tag. There is no single
concrete pointee type that could replace the boundary.

The typed union had no independent retail oracle; it merely made those casts
less visible in the reconstruction. The broad exact-family recovery selects the
single erased storage member and demonstrates that the union was an invented
abstraction, not a recovered one.

## Reverse-use rule

When a header-inline family has exact standalone helpers but widespread
inline/call-set residue, audit any recently introduced union or wrapper in the
callee's expression tree. Test the original direct member spelling as one
source-shaped A/B and measure the whole caller family. Retain it only when the
storage semantics independently justify it; never add `void*` as a generic
inline-budget lever.

Related:
[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md),
[`void-star-is-the-fake-view-laundering-channel`](void-star-is-the-fake-view-laundering-channel.md),
[`rep-movs-count-is-merge-sensitive`](rep-movs-count-is-merge-sensitive.md).
