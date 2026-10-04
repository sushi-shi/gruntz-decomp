# `zBitVec::body`: const accessor returning mutable storage

[All sites](README.md). This page covers the single cast in
[`include/ZTools/BitVec.h`](../../include/ZTools/BitVec.h#L55).

## Our code

```cpp
u32* body() const {
    return static_cast<u32>(m_capacity) > BITARRAY_WORD_BITS ? m_words
                                                             : const_cast<u32*>(&m_inline);
}
```

The storage is an anonymous union of `u32* m_words` and `u32 m_inline`.
Inside a const member function, `&m_inline` has type `const u32*`, but the
function promises `u32*`. The cast removes that qualification for the inline
case. The heap case needs no cast: making the pointer member const does not
make the separately allocated words const.

## Declarations and source provenance

No Windows SDK declaration requires this signature. It comes from the related
ZTools library API. The surviving NOLF header declares a const `zBitSet::body`
that returns a mutable unsigned-word pointer. Its inline-storage branch casts
the address of its pointer-sized storage word. Our reconstruction expresses
that storage as a typed union instead of repeating the original storage cast.

The GitHub witness is
[`ztools.h` at `dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54`](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L368-L380).
Its Git blob is `8f548108894313b5b62d5cb6b17e6f078128cbd1`, matching the
`hs-bitset-body` row in the [lineage ledger](../../config/lithtech_lineage.tsv).
This is direct family source evidence, not merely an unrelated project's
coding style. It does not prove every declaration of the Gruntz revision.

Our accessor and named cast originated in checkpoint `9f75d37ce498d65bc2a81058b9fdf9dc9720ddb9`,
which restored the sourced helper. Public blame reaches the containing
[PR #79 commit](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f).
The qualifier was adopted with the helper; it was not recovered from a game symbol.

## Current use and safety

The two current private calls are in the string constructor in
[`TypeKeyColl.cpp`](../../src/Bute/TypeKeyColl.cpp#L297), while the object is
being initialized. They use mutable construction storage. Mutation of ordinary
members during construction is permitted; the accessor is not currently used
to mutate an already-constructed const object. The cast itself does not write.

The accessor nevertheless exposes a mutable pointer from a const-qualified
method. That API would be unsafe if a future call used it to modify a truly
const object's inline word. Source provenance is not a general safety proof.

## What could remove it?

A non-const private accessor would suffice for the current constructor calls.
Const and non-const overloads could instead distinguish read and write access.
Both would intentionally depart from the surviving helper's declared contract;
neither is forced by an SDK boundary. Reconsider this site if its callers or
the source-lineage decision change, rather than treating the cast as inherently necessary.
