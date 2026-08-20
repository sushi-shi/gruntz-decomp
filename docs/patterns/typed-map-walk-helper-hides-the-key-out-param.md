# A map-walk helper that hides an out-param drops retail's zero-init of it

**Tags:** cpp:local cpp:template cpp:method | asm:mov asm:xor | topic:codegen-idiom

## Symptom

A `CMapPtrToPtr::GetNextAssoc` / `CMapStringToPtr::GetNextAssoc` loop is one
`mov` short of retail per iteration, retail materialises a zero in a callee-saved
register and compares against it (`cmp ebx,esi`) where we emit `test ebx,ebx`,
and the loop's preheader carries a `jmp` into the body that we do not have.
`walls diagnose` reports CFG (one extra branch) or REGALLOC; `walls semdiff`
shows no exclusive key at all, only `mov base N target N+1`.

```asm
; retail: BOTH out-params are re-zeroed every iteration
xor  edx,edx                       ; the shared zero, hoisted
...
mov  DWORD PTR [esp+0x20],edx      ; rKey   = NULL
mov  DWORD PTR [esp+0x18],edx      ; rValue = NULL
call CMapPtrToPtr::GetNextAssoc
; base: only the value slot is zeroed
mov  DWORD PTR $0x0,0x24(%esp)
```

## Mechanism

`GetNextAssoc(POSITION&, void*& rKey, void*& rValue)` has TWO sinks. A typed
convenience wrapper that only exposes the value —

```cpp
template<class T> inline void MapGetNextValue(CMapPtrToPtr& map, POSITION& pos, T*& out) {
    AddrWord<void> ignoredKey;                 // never initialised
    MapOutRef<T> dst; dst.m_asTyped = &out;
    map.GetNextAssoc(pos, ignoredKey.m_addr, *dst.m_asVoid);
}
```

— leaves the key slot uninitialised, so cl emits one store where retail emits
two. The second store is not decoration: with two zero stores per iteration cl
hoists the constant into a register, and that register then serves every other
`== NULL` test in the function, which is why a one-store difference moves the
whole body's instruction selection.

Retail's order is **key first, then value**, so the key must be a named local
declared BEFORE the value at the call site — a wrapper cannot produce it, since
the caller's `T* val = NULL;` is already in the IL by the time the wrapper's body
is inlined.

## Fix

Spell both sinks at the call site and use the wrapper that takes the key:

```cpp
// NO
CWwdGameObject* val = NULL;
MapGetNextValue(m_registeredGameObjectsById, pos, val);

// YES - retail zeroes rKey then rValue
void* key = NULL;
CWwdGameObject* val = NULL;
MapGetNext(m_registeredGameObjectsById, pos, key, val);
```

`void*` here is the SDK's own key type for `CMapPtrToPtr`, not an unmodelled
pointer.

STEERABLE. Measured 2026-08-21 on `CDDrawChildGroup`'s five map walks:
`CountActive` 97.47 -> **100.00 EXACT**, `ForEachDispatch` 86.82 -> **100.00
EXACT**, `ForEachProbe` 92.94 -> **100.00 EXACT**, `ForEachSerialize` 97.92 ->
**100.00 EXACT**, `PruneOrphans` 90.95 -> 93.75 (its residue is a separate
`Lookup`-result materialisation). `MapGetNextValue` had no other user and was
removed.

## Bounds

Only for the `CMapPtrToPtr` overload, whose key is a bare `void*`. The
`CMapStringToPtr` walks already declare `CString key;`, which has a constructor,
so their key slot is initialised and the defect cannot occur there.
