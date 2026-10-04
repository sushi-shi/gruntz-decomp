# FreeNodePool: recovering the owner of a payload

There is one retained cast in [FreeNodePool.h](../../include/Utils/FreeNodePool.h):

```cpp
Node* NodeOf(void* payload) {
    return reinterpret_cast<Node*>(static_cast<char*>(payload) - m_linkOffset);
}
```

`payload` is intended to point to a live `Node::m_value` subobject, not arbitrary storage. `static_cast<char*>` provides byte-address arithmetic; the final cast interprets the adjusted address as its enclosing node. `m_linkOffset` is a signed `i32` runtime offset.

## Allocation, ownership and caller contract

The owning template declares:

```cpp
struct Node {
    Node* m_next;
    T m_value;
};
```

`Init` allocates `new Node[count]` and links the nodes. `Pop`/`PopCopy` return `&m_freeHead->m_value`; `Push` calls `NodeOf` and writes the node's free-list link. The pool's destructor deletes the original node array. Recycling neither separately allocates/deletes the payload nor starts a fresh `T` lifetime.

The current concrete pool is [g_coordPool](../../include/Gruntz/CoordPool.h), `FreeNodePool<Coord>`, defined in [BootyStateActivate.cpp](../../src/Gruntz/BootyStateActivate.cpp). [GruntzMgr.cpp](../../src/Gruntz/GruntzMgr.cpp) initializes it with count `0x4e20` and offset `4`. In the target's node layout, the initial pointer occupies four bytes and `Coord` follows it. Thus the recorded offset has an actual owner/layout explanation. The template does not derive or validate the offset for arbitrary `T`.

Correct use requires the exact payload address of a still-live node from this pool, the correct offset, no duplicate recycling, and an initialized live backing array. Subtracting the wrong offset or accepting a foreign/stale pointer can produce a misaligned or unrelated address; the subsequent `node->m_next` store then corrupts memory. No such bad call was established in this audit.

## Language limits and comparison

This is a conventional implementation-dependent `container_of` operation. Casting an address does not by itself create a `Node`. Even with a known enclosing object, byte arithmetic backwards from a member subobject is not established as universally portable merely because character access may inspect object representation. Standard pointer-interconvertibility does not make every non-first member interchangeable with its parent. The comment “Language-forced” should be understood as a description of the chosen layout technique, not a proof that C++ requires this exact implementation or guarantees every use.

Boost.Intrusive's [commit-pinned parent-from-member implementation](https://github.com/boostorg/intrusive/blob/b089da5af88981d6e87392680b0b68fd30be0b12/include/boost/intrusive/detail/parent_from_member.hpp#L35-L103) performs the same general owner recovery, with explicit compiler/ABI branches and member-offset handling. This is a genuine comparison of the mechanism, **not** evidence of Gruntz's original source or proof that Boost's implementation can be transplanted unchanged into VC5. No external implementation is quoted.

This cast has no function-pointer mismatch or direct pointer-to-integer narrowing. Alignment follows only if the original node and exact offset are recovered. A redesign could return/store node handles, keep a separate payload-to-node map, or use another proven intrusive representation; those alter interfaces/storage and are not cosmetic cast removal. No safe source-equivalent replacement has been established here, so the cast is retained with its preconditions recorded.

## Provenance

| Commit | Change and reason |
| --- | --- |
| [1c4b1239](https://github.com/sushi-shi/gruntz-decomp/commit/1c4b12396b0b47880e6eca672d7e3dd40f7e3b80) | Added typed `CoordPoolNode* NodeOf(void*)` in `include/Gruntz/FreeNodePool.h`, centralizing the repeated payload-minus-runtime-offset operation. The initial version used reinterpret casts for both pointer conversions. |
| [1a8ed0b0](https://github.com/sushi-shi/gruntz-decomp/commit/1a8ed0b093fa34fb77298086891755eb2b60672a) | Recovered `Init` with an actual node-array allocation, supplying a concrete allocation/lifetime owner for the pointer recovery. |
| [0335a536](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) | Generalized to `FreeNodePool<T>`, moved to `include/Utils`, and retained the container recovery while using `static_cast<char*>` for the ordinary `void*` conversion. |

Current blame at the generic header is therefore not the initial introduction. The documented rationale is typed owner recovery and shared code, not independent original source attestation. Generic `Init`/`Pop` also have count/free-list preconditions; the observed positive initialization does not establish correctness for arbitrary counts. No pool code was changed or runtime-tested in this audit.
