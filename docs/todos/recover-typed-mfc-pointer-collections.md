# Recover typed MFC pointer collections

Several class members are currently modeled as raw `CPtrArray` or `CPtrList`
objects even though every known consumer treats the collection as holding one
stable pointer type. This exposes storage details throughout the owning class:
callers repeat casts around `GetAt()` and `GetData()`, directly select between
`Add()` and `InsertAt()`, and duplicate element-lifetime policy.

The leading source-shape hypothesis is an MFC typed adapter such as
`CTypedPtrArray<CPtrArray, T*>`, or an evidence-backed owner-specific collection
with inline typed operations. These types can have the same object layout as the
raw MFC base while changing caller evaluation order, inline boundaries, and
method selection. Layout compatibility alone is therefore not enough to choose
between them.

## Initial example: `CStatusBarMgr::m_rewardQueue`

The field is currently a raw `CPtrArray`, but its complete observed lifecycle is
for `Coord*` values allocated from `g_coordPool`:

- teardown and reset recycle every `Coord*` and clear the array;
- `QueuePickupReward` inserts by the coordinate's score field;
- `StartChipMachineCycle` consumes and recycles the first value;
- serialization writes each eight-byte `Coord` value;
- deserialization allocates `Coord` values, reads them, and stores them back;
- the status-bar query reads the current value as a `Coord`/integer pair.

Retail also constrains the insertion API. The append path expands
`CPtrArray::Add` to a `CPtrArray::SetAtGrow` call, while an interior insertion
calls `CPtrArray::InsertAt`. Do not replace both paths with unconditional
`InsertAt`, even though inserting at `GetSize()` is semantically sufficient.
Likewise, do not introduce a convenience wrapper unless its inlined callers
preserve these resolved call identities.

## Follow-up audit

1. Derive an inventory of raw MFC pointer collections and group every use by
   owner, element type, allocation source, teardown path, and serialization.
2. Check contemporary Monolith source and the pinned surviving-source lineage
   for `CTypedPtrArray`, `CTypedPtrList`, and owner-specific inline adapters.
3. For each candidate, prove complete layout and API compatibility, then test
   the whole declaration/use family in its real translation units.
4. Compare resolved call targets, ordered relocations, CFG, constructors,
   destructors, and all affected historical-MAX gates. A cast reduction or
   byte-flat type swap is not by itself proof of the original type.
5. Adopt only complete evidence-backed families; leave unresolved fields raw
   rather than inventing a local wrapper around one call site.

This work is intentionally separate from control-flow cleanup. A local goto can
be removed when its current `CPtrArray` call topology is proven, without settling
the collection's eventual source-level type.
