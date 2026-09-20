# Recover MFC pointer-collection API boundaries

Several class members are currently modeled as raw `CPtrArray` or `CPtrList`
objects even though every known consumer treats the collection as holding one
stable pointer type. This exposes storage details throughout the owning class:
callers repeat casts around `GetAt()` and `GetData()`, directly select between
`Add()` and `InsertAt()`, and duplicate element-lifetime policy.

This is a repository-wide API-recovery problem, not just a missing cast helper.
Before adding a local inline helper, determine whether a raw-looking sequence is
already one MFC collection operation, an expansion through an MFC typed adapter,
or an owner-specific collection method. Reimplementing such an operation at
every call site loses the original abstraction boundary even when the resulting
machine code is currently equivalent.

The main source-shape hypotheses are:

- an existing `CPtrArray` or `CPtrList` member operation whose full VC5-era
  contract has not yet been modeled;
- an MFC typed adapter such as `CTypedPtrArray<CPtrArray, T*>` or
  `CTypedPtrList<CPtrList, T*>`;
- an evidence-backed owner-specific collection with typed inline operations;
- deliberate direct access to the MFC base for bulk serialization or storage
  management.

These alternatives can have the same object layout while changing caller
evaluation order, inline boundaries, method selection, and ownership placement.
Layout compatibility or the disappearance of casts is therefore not enough to
choose between them. Conversely, do not wrap every `GetData()`, `SetSize()`, or
`RemoveAt()` mechanically: some direct uses may be the authentic interface.

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
   owner, element type, allocation source, teardown path, indexing pattern,
   insertion/removal policy, and serialization.
2. Audit the complete VC5 MFC collection API before introducing helpers. Mark
   sequences that may already be a standard member operation, especially
   append-versus-insert, grow-and-store, clear, head removal, and typed element
   access.
3. Check contemporary Monolith source and the pinned surviving-source lineage
   for `CTypedPtrArray`, `CTypedPtrList`, and owner-specific inline adapters.
4. Recover each candidate as a complete declaration/use family. A method
   hypothesis must explain all callers rather than only the first convenient
   sequence found in one function.
5. Prove layout and API compatibility, then test the family in its real
   translation units. Compare resolved call targets, ordered relocations, CFG,
   constructors, destructors, and all affected historical-MAX gates.
6. Adopt only complete evidence-backed families. A cast reduction or byte-flat
   type swap is not proof of the original type; leave unresolved fields raw
   rather than inventing a per-call-site wrapper.

This work is intentionally separate from control-flow cleanup. A local goto can
be removed when its current `CPtrArray` call topology is proven, without settling
the collection's eventual source-level type.
