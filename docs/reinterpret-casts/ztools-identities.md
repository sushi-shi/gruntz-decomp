# ZTools integer/pointer boundaries: four retained expressions

These sites use a pointer sentinel, preserve an allocation address in an integer result, and compare address identities. They have different contracts. None grants permission to dereference an arbitrary integer-derived pointer.

## 1. No-scratch sentinel

[ZVec.h](../../include/ZTools/ZVec.h):

```cpp
ZVEC_NO_SCRATCH_ADDRESS = 1
return reinterpret_cast<char*>(ZVEC_NO_SCRATCH_ADDRESS);
```

The enumeration value becomes a `char*`, then converts to the `_zdvec` constructor's `void* overflow` parameter in [ZDArray.h](../../include/ZTools/ZDArray.h). It represents a non-null sentinel, not allocated character storage. [The base constructor](../../src/Bute/TypeKeyColl.cpp) stores it in `ovf` and skips allocating an overflow object when non-null. Its destructor frees `vec`, not this sentinel. There is no object lifetime or alignment claim at address one.

The [surviving zDArray constructor](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L1646-L1665) passes integer one as a `void*` in the same position. This is a genuine family witness. The current `char*` helper result is a reconstruction spelling, not the exact source type in that witness. The conversion depends on the target's integer/pointer mapping. In an allocation/range failure, `get` can return `ovf` after error handling; if handling returns and `operator[]` dereferences it, the sentinel is not a valid `T`. This is an inherited exceptional-path hazard, not proof of a normal successful-path fault. Replacing it with `NULL` would change allocation behavior.

[0335a53651](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) introduced the named helper using an `AddrWord<char>` union. [3b09d668f2](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) replaced that union overlay with this explicit cast during prelude/alias cleanup. The cast made the existing boundary visible; it did not create the sentinel design. Retained.

## 2. Allocated address returned as integer status

[_zdvec::realloc](../../src/Bute/TypeKeyColl.cpp):

```cpp
vec = p;
return reinterpret_cast<i32>(p);
```

`p` is the successful `char*` returned by the C allocator's `realloc`; `i32` is `int`. Both are 32-bit in the VC5 target. The object owns the buffer through `vec`, and the normal lifetime later frees it. The integer result does not transfer ownership. [The sourced declaration and caller](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L1540-L1554) use an integer success result; the header does not contain the allocation implementation.

The [lineage ledger](../../config/lithtech_lineage.tsv), entries `reassess-zdvec-realloc-return`, `reassess-zdvec-normalized-success` and `reassess-zdvec-pointer-api-inference`, records retail successful returns containing allocation-address bits, failure returning zero, and consumers testing zero. It expressly does not claim to have recovered the original implementation/cast spelling. Pointer-to-integer conversion is implementation-defined within a sufficiently wide integer type; a 64-bit pointer does not fit this fixed-width result. The target's nonzero address mapping is part of the status contract. Returning literal one would change the recorded retail result even if known callers test only truthiness.

[0335a53651](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) changed the earlier reconstructed pointer-returning `GrowTo` to the sourced integer API and introduced explicit casts in both success arms. [2efb0f2c0d](https://github.com/sushi-shi/gruntz-decomp/commit/2efb0f2c0d2ac47bd35ada9d652ce043c45b3752) later shared the success suffix, leaving the single occurrence now present. This is an ABI-width dependency, not a pointee aliasing operation. Retained.

## 3–4. Both operands of the signed address difference

[zErrHandler::srch](../../src/Bute/TypeKeyColl.cpp):

```cpp
cmp = reinterpret_cast<long>(dl[slot].object) - reinterpret_cast<long>(o);
```

These are **two distinct casts**, one for the stored `void*` identity and one for the queried `void*`. `cmp` and both converted operands are signed `long`, 32-bit under VC5. [Error.h](../../include/ZTools/Error.h) owns the fixed `_dhandler` table; `set_ef` inserts/removes identities, while `srch` uses the sign of the difference for binary-search direction. Neither pointer is dereferenced here, so there is no pointee alignment or aliasing requirement. Identity lifetime still matters to callers registering/removing handlers, but the search owns no pointed-to object.

The conversions' target mapping is distinct from the subtraction: signed subtraction can overflow, which is undefined behavior in C++. If all converted addresses occupy the usual nonnegative 31-bit domain, their pairwise difference fits; this audit has not proved that invariant for every environment/caller. Matching a wrapped machine subtraction does not establish portable C++ arithmetic. Converting to unsigned arithmetic or replacing the difference with relational comparisons requires an explicit choice of ordering at boundary values; neither is a source-neutral cast deletion.

[85a64f0f](https://github.com/sushi-shi/gruntz-decomp/commit/85a64f0fffbd5b182a02d51d2e6bc0bbd3b0306f) introduced the older integer-key search under `CVariantSlot::Find`. [0335a53651](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) restored pointer identity fields and the `zErrHandler::srch` owner, making both address conversions explicit. The [source witness](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L70-L120) declares the pointer-based search but not its body. The ledger's `reassess-zerrhandler-srch-debug-locals` additionally attributes signed `long cmp` to a related Debug library and records a retail instruction comparison. That is stronger than an arbitrary invented integer type, but does not prove absence of overflow or the exact original cast spelling. Both casts retained; the arithmetic risk remains explicit.

## Common limits

The [C++ conversion rules](https://eel.is/c++draft/expr.reinterpret.cast) distinguish conversions from subsequent access; the [expression rules](https://eel.is/c++draft/expr.pre) separately govern unrepresentable arithmetic results. These four expressions were audited without executing allocation-failure paths or changing source. Source lineage explains why these boundaries exist, not universal safety. No external source excerpts are reproduced on this page.
