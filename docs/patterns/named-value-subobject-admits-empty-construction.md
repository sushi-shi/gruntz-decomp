# A named value subobject can admit constructors without changing storage

tags: cpp:aggregate cpp:constructor cpp:inline cpp:scope | asm:call asm:fstp | topic:source-shape topic:identity

A compiler rejection involving a nontrivial member of an anonymous aggregate is
evidence about that declaration, not proof that the value type lacked a
constructor. Check the complete containing owner before excluding the sourced
constructor family.

The controlled VC5 `/O2 /MT` experiment starts at `3230f59c4`. Naming the existing
motion block `CGruntCellRec::Motion` and adding only an empty `DoubleVector2`
default constructor changes none of the 396 scored body/reference records in
the four rebuilt translation units: Spotlight, Grunt, GruntDataRecord and
SerialObjectFactory. This is a composition control, not attribution to either
declaration in isolation.

The important positive controls are the production and original cell callbacks:
the 27-byte constructor still constructs five four-byte CStrings, and the
16-byte destructor still tears them down. All five ordered fixups agree. The
two serializers still transfer the motion block at cell offset 72, length 32.
Pinned header checks additionally enforce the 104-byte cell, 32-byte nested
block, and direction/step ordering. An empty default constructor must not be
silently replaced with zero initialization.

The complete arithmetic experiment must then be judged separately. Adding the
sourced scalar constructor and value-returning arithmetic with implicit copies
can introduce integer DWORD-half copies and FP homes; composing the existing
member `Init` restores floating-point destination stores. Thus a legal,
layout-preserving declaration does not by itself prove the correct caller's
copy, result-lifetime or assignment boundary. Conversely, a caller mismatch does
not invalidate the independently checked owner declaration.

Reverse-use procedure:

1. Establish the real containing object and whole-subobject uses. Do not invent
   a global class or inheritance layer merely to satisfy the compiler.
2. Test the minimum named-owner/noninitializing-constructor composition with
   caller bodies fixed.
3. Check layout, actual array callbacks, serialization and cleanup referents.
4. Compose the complete used value API and its callers. Compare from the first
   divergence, including raw stores and result lifetimes, not just score.

`Motion` is an inferred owner-local name, not recovered original spelling.
Canonical decisions and individually recheckable alternatives are
`reassess-vector2-cell-nested-motion`,
`reassess-vector2-copy-sum-difference-composition`, and the `reassess-vector2-*`
rows in the lineage ledger. This does not close the broad vector reassessment.
Regression controls are in `test_grunt_cell_vector_consumers.py` and
`test_activate_vector_helpers.py`; the serializer checks are local protocol
checks, not a general proof of every serializer branch or external EH caller.
