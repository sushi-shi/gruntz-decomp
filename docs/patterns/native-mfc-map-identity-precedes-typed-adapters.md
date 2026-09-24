# Native MFC identity precedes typed adapters

tags: cpp:mfc cpp:template cpp:constructor cpp:cast | asm:mov asm:call | topic:mis-model topic:eh
symptoms: same-size typed collection, missing derived-vptr store, key/value lookup alias

The pinned VC5 `AFXTEMPL.H` derives its `CTypedPtrArray`, `CTypedPtrList` and
`CTypedPtrMap` from native MFC containers. Equal sizes and matching base-method
calls do not prove equal concrete types. Real `/O2 /MT` construction controls
for both pointer/object arrays, both pointer/object lists, and both pointer/string
maps emit derived-vtable references absent from native construction. The positive
and negative controls live in `scripts/test_container_headers.py`.

## Complete constructor evidence

The status-bar reward queue is a useful reverse-use witness. Every observed
payload is a pooled `Coord*`, but retail `CMulti::LoadGameAssetNamespaces`
initializes `CStatusBarMgr +0x530` with `CPtrArray::CPtrArray` at 0xb59e7 and
does not replace its vptr. The eight tab lists at +0x2c use the native constructor
adapter and destructor in the vector-construction call at 0xb58f3. A typed
derived constructor cannot be substituted merely because it calls the same base.
Teardown and the reward queue's serialization, insertion and consumption confirm
element ownership independently of container identity.

The same distinction applies to the map family. Retail constructs the two
`CDDrawChildGroup` ID maps at 0x1559aa/+0x2c and 0x1559b9/+0x48, calling
`CMapPtrToPtr::CMapPtrToPtr` at 0x1b85b1. The sound and animation registry maps
at 0x155b7f and 0x155bd0 call `CMapStringToPtr::CMapStringToPtr` at 0x1b8247.
None is followed by a derived map vptr store. The outer owner's vptr belongs to
a different subobject and cannot satisfy that requirement.

The older six-real-TU control at `6cdccb254` independently tested the complete
SDK map family. `KEY=CString` adds a copy/destructor lifetime to `RemoveKey`,
which takes `KEY` by value; `Lookup` instead takes `BASE_ARG_KEY`. Reference-key
arguments fail VC5's `KEY&` iteration declaration, while `LPCTSTR` cannot accept
the existing `CString&` output sink. That historical control is not a current
caller-exactness certification. The current SDK and raw retail constructor
checks independently corroborate its identity conclusion.

## Preserve the native output seam

The real map API writes through `void*&`. Its typed SDK forwarder uses a reference
cast; the equivalent named conversion is `reinterpret_cast<void*&>(out)`.
`MapOutRef<T>` merely wrote one union arm and read another to conceal that seam.
Removing the union preserves the existing typed output home, caller reset and
inline helper boundary. The unused concrete `CObject` overload of `MapLookupById`
duplicates the template and has no emitting or referencing object. No class,
constructor, or additional storage is needed for these free forwarders.

Native identity excludes the SDK derived class; it does not establish the original
names of reconstructed free helpers or exclude an unknown composition wrapper.
Absent surviving declarations or another independent source witness, do not
invent a typed owner solely to eliminate casts.

## Distinguish the stored key from the stored value

The pinned SDK `mfc-src/MAP_SP.CPP` has separate `Lookup` and `LookupKey` methods.
Both call `GetAssocAt` and both are 34 bytes in retail. At +0x16, the first loads
the association's value from +12; the second loads its key from +8. Accordingly,
0x1b8438 is `Lookup(LPCTSTR, void*&)` and 0x1b845a is
`LookupKey(LPCTSTR, LPCTSTR&)`, not a second alias for `Lookup`.

The integration control checks the consumed Model's names against those actual
retail operands, with swapped-field negative checks. This prevents a label-only
test from passing while the normal build still consumes a duplicate identity.
