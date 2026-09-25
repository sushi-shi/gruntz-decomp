# Recover typed access to MFC pointer collections

Many class members are raw MFC pointer collections (`CPtrList`, `CPtrArray`,
`CObList`, `CObArray`, `CMapStringToPtr`, `CMapPtrToPtr`) although every
consumer treats each one as holding a single pointer type. Callers repeat casts
around `GetAt()`/`GetData()`/`GetNext()`, choose between `Add` and `InsertAt`
by hand, and duplicate element-lifetime policy. The goal is to recover the
source layer the original developers wrote around each collection.

## Settled

- The containers stay native MFC. Retail constructs native containers with no
  derived-vptr store, while `CTypedPtrArray`/`CTypedPtrList`/`CTypedPtrMap`
  would add one. Verified for the status-bar reward queue (`CStatusBarMgr`
  +0x530, `CPtrArray::CPtrArray` at 0xb59e7), the status-bar tab lists (vector
  construction at 0xb58f3), the `CDDrawChildGroup` ID maps (`CMapPtrToPtr` at
  0x1b85b1) and the sound/animation registry maps (`CMapStringToPtr` at
  0x1b8247). History: `git show 4e7b6ee0b^:docs/todos/recover-typed-mfc-pointer-collections.md`.
- Keep retail's resolved MFC calls. The reward queue's append path is
  `CPtrArray::Add` (expanding to `SetAtGrow`) and its interior insertion is
  `CPtrArray::InsertAt`; a helper must not merge them.

## Open

For each collection, decide which of these the source was:

- a standard MFC member operation not yet modeled;
- an owner-specific typed inline accessor or operation (the holista skill);
- deliberate raw access (bulk serialization, storage management).

Method, per collection family:

1. Inventory the owner, element type, allocation source, teardown, indexing,
   insertion/removal policy, and serialization of every use
   (`scripts/audit-template-models.py` emits `mfc-pointer-members.json`).
2. Check surviving Monolith source (`config/lithtech_lineage.tsv`,
   `~/Projects/monolith-sources`) for the owner's own accessors.
3. Adopt a helper only when it explains every caller, applies at every site,
   and keeps every exact function exact. Leave a field raw rather than invent
   a per-site wrapper; a byte-flat cast reduction alone is not proof.

Scale at restoration: 54 raw collection members, no typed adapters, 288 casts
around element access in 61 files.
