# A typed teardown adapter can contain an authored explicit destructor call

tags: cpp:template cpp:destructor cpp:callback cpp:cast | asm:jmp asm:call | topic:source-oracle topic:compiler-artifact topic:cfg
symptoms: cleanup code contains an explicit destructor call and a type-erasing function-pointer cast, while a collection owns destruction and deallocation as separate operations
confidence: 10/10

The surviving LithTech `ztools.h` defines `zSymTab<T>` over an untyped pointer
collection. Its callback is real authored source, not a transcription of a
compiler-generated deleting destructor:

```cpp
template<class T> void zSymTab<T>::dtf(T* p) {
    p->~T();
}
```

The template constructor erases `void (*)(T*)` to the collection's
`void (*)(void*)` callback type. The collection invokes that callback and then
deallocates the storage. Consequently both the explicit destructor expression
and the ABI cast are load-bearing source layers. The compiler-artifact verifier
admits exactly this path/type/count signature; an unreviewed destructor call or
an unexplained cast still fails its respective gate.

Gruntz retail also proves a revision difference inside the authentic layer. For
the nested `zSymTab<zSymTab<CButeMgr::CSymTabItem>>` adapter at 0x174de0:

| spelling | VC5 result |
|---|---|
| `p->~T()` | 11 bytes, vtable load, indirect scalar-deleting-destructor call, `ret` |
| `p->T::~T()` | 9 bytes, direct tail jump to the inner-table destructor |

The surviving NOLF Release object independently emits the first, virtual-call
form. Gruntz retail is the second form exactly, so the typed template transfers
but its destructor qualification does not. The inner `CButeMgr::CSymTabItem` adapter at
0x174df0 remains byte-exact because that type has no virtual destructor choice.

## Reverse use

1. Determine whether destruction and deallocation are separate collection
   responsibilities. If not, an explicit destructor call remains suspicious.
2. Recover the callback's typed source signature before its ABI erasure.
3. For a polymorphic `T`, compare `p->~T()` with `p->T::~T()`: VC5 can lower the
   former through the deleting-destructor vtable slot and the latter as a direct
   destructor call.
4. Keep the type erasure at the one callback boundary, mark its reason, and
   retain the negative gates for every new destructor/cast site.

The source adoption and the one revision-specific spelling are recorded in
`config/lithtech_lineage.tsv`.
