# ZTools typed callbacks: two incompatible-call casts removed

The two former casts in [PTree.h](../../include/ZTools/PTree.h) implement a surviving library design, but calling incompatible function types is not defined C++. Both have been removed with correctly typed adapters. This is an intentional safety repair to the inherited design, not a claim that the replacement was the original source.

## Every occurrence and its call chain

| Original occurrence | Removed expression | Source type → destination type |
| --- | --- | --- |
| `zSymTab<T>` constructor | `reinterpret_cast<dtorf_t>(dtf)` | `void (__cdecl*)(T*)` → `void (__cdecl*)(void*)` |
| `zSymTab<T>::traverse` | `reinterpret_cast<stvf_t>(fn)` | `void (__cdecl*)(const char*, T*, void*)` → `void (__cdecl*)(const char*, void*, void*)` |

The former declarations and bodies were:

```cpp
typedef void(__cdecl* dtorf_t)(void*);
typedef void(__cdecl* stvf_t)(const char*, void*, void*);

zSymTab(cleanup_behaviour cleanup = ACTIVE)
    : zPTree(reinterpret_cast<dtorf_t>(dtf), cleanup) {}

void traverse(void(__cdecl* fn)(const char*, T*, void*),
              void* supplementary = NULL) {
    zPTree::_trav(reinterpret_cast<stvf_t>(fn), supplementary, NULL);
}
static void dtf(T* p) { p->T::~T(); }
```

The destructor callback is stored in `zPtrColl::m_dtor`. [zPTree::cleanup](../../src/Bute/TypeKeyColl.cpp) invokes `destroy(n->m_body)` when active cleanup is enabled; `destroy` calls that pointer. The callback explicitly destroys the typed object; it does not itself deallocate storage. Cleanup then deallocates the erased body and the node. That subsequent erased deletion has its own allocation/type contract; this function-pointer repair does not validate it.

The traversal callback reaches [ButeTree.cpp](../../src/Bute/ButeTree.cpp), where `_trav` calls `fn(node->m_symbol, node->m_body, supplementary)`. Previously this was an actual call through the converted type, not storage followed by conversion back. [ButeMgr.h](../../include/Bute/ButeMgr.h) instantiates item and nested tag tables. [ButeMgr.cpp](../../src/Bute/ButeMgr.cpp) creates these objects and traverses them through `AuxTabItemsSave` and `NewTabsSave`. The passive action-ID table in [ButeGlobals.cpp](../../src/Bute/ButeGlobals.cpp) does not establish that every other table is passive.

## Language and ABI assessment

The [function-pointer conversion rule](https://eel.is/c++draft/expr.reinterpret.cast) permits changing pointer types; the [call rule](https://eel.is/c++draft/expr.call#6) separately requires a compatible function type at invocation. Substituting `void*` for `T*` in a parameter does not make those function types compatible. Executing the former indirect calls through those incompatible types was undefined behavior for the non-void instantiations. Shared `__cdecl`, identical pointer widths and matching x86 argument slots explain why the target ABI could execute them as intended; they are not a language-level proof of safety.

There is no integer truncation here and the conversion allocates no object. A live, correctly aligned `T` must still underlie each active typed payload. The callback cannot repair a stale pointer, wrong table element type, double destruction, or invalid deallocation. No such runtime failure is asserted by this source-only audit.

The current teardown adapter actually accepts `void*` and converts the value inside its body:

```cpp
static void dtf(void* value) {
    T* p = static_cast<T*>(value);
    p->T::~T();
}
```

The constructor passes `dtf` without a cast. Traversal preserves its typed public callback parameter and constructs a private, stack-owned `TraversalContext` containing that callback plus the original supplementary pointer. It passes `TraverseTypedValue` and the context's address to `_trav`:

```cpp
static void __cdecl TraverseTypedValue(const char* key, void* value, void* opaque) {
    TraversalContext* context = static_cast<TraversalContext*>(opaque);
    context->function(key, static_cast<T*>(value), context->supplementary);
}
```

The trampoline has exactly `stvf_t`'s type and calls the original function through its original typed pointer. `_trav` completes synchronously and does not retain the supplementary pointer; its recursive calls remain within the context's lifetime. Nested/reentrant traversals receive independent stack contexts. The added nested context type changes no `zSymTab` object layout. The three destructor callback symbol annotations were updated for their actual `void*` parameter. Compiler/referent verification is part of the combined cleanup build; no byte-neutrality is assumed for this intentional repair.

## Source witness and Git provenance

The independently obtained [NOLF ZTools header](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L1205-L1274), Git blob `8f548108894313b5b62d5cb6b17e6f078128cbd1`, contains the same typed destructor and traversal erasure family. This is surviving related library source, not an unrelated safe analogy. Its old `stvf_t` declaration omits an explicit return type, whereas our reconstruction explicitly uses `void`; that difference must not be silently represented as a verbatim type match. The witness proves the family and casts existed, not that they are portable or that every Gruntz spelling is recovered uniquely. No external source is quoted here.

| Reconstruction commit | Introduction or later change | Reason versus proof |
| --- | --- | --- |
| [029b2bae04](https://github.com/sushi-shi/gruntz-decomp/commit/029b2bae0440f67c9b7a2773b53b50c8556037ba) | Replaced the former concrete Bute node wrapper with `zSymTab<T>` and introduced the teardown erasure. | Stated original-source-family recovery; the surviving header corroborates that family. |
| [7122837f96](https://github.com/sushi-shi/gruntz-decomp/commit/7122837f96de13054ab69b49be51f02fe62a0817) | Introduced the current callback typedefs and typed traversal cast; retained teardown through `dtorf_t`. | Restored the library API. Its “PROVEN” comment concerns source/ABI lineage, not absence of C++ UB. |
| [0335a53651](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) | Consolidated shared template ownership under ZTools. | The present header path is not the cast's original introduction; the earlier diffs are under `include/Bute/PTreeNode.h`. |

[The lineage ledger](../../config/lithtech_lineage.tsv) records the historical `nolf-zsymtab-typed-api`, qualified-destructor adaptation and definition-placement question. Those source-recovery records do not waive language rules or describe this safety adapter as original source. Two original cast expressions are accounted for and removed.
