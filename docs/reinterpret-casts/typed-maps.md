# Typed map outputs and integer object IDs

This page accounts for eight original cast expressions: **five output-reference casts removed**, plus **three integer-key casts retained**. The distinction matters: an opaque key is never dereferenced; an output reference actually writes a pointer object through another pointer type.

## Five removed output-reference puns

The original [MapTyped.h](../../include/Utils/MapTyped.h) expressions were:

| Wrapper | Original expression | Status |
| --- | --- | --- |
| `MapLookup(CMapStringToPtr&, LPCTSTR, T*&)` | `map.Lookup(key, reinterpret_cast<void*&>(out))` | Fixed |
| `MapLookup(CMapPtrToPtr&, void*, T*&)` | `map.Lookup(key, reinterpret_cast<void*&>(out))` | Fixed |
| `MapGetNext(CMapStringToPtr&, POSITION&, K&, T*&)` | `map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out))` | Fixed |
| `MapGetNext(CMapPtrToPtr&, POSITION&, K&, T*&)` | `map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out))` | Fixed |
| `MapLookupById(CMapPtrToPtr&, i32, T*&)` | `map.Lookup(reinterpret_cast<void*>(id), reinterpret_cast<void*&>(out))` | Output cast fixed; key cast retained |

The parameter `out` designates a caller-owned pointer object of type `T*`. Casting its reference to `void*&` does not convert that pointer's value: it asks MFC to access the **same pointer object** as `void*`. For non-void `T`, equal size/alignment under VC5 does not establish type accessibility. A successful write through the punned reference is not justified by standard C++ aliasing rules. No extra object is created by the cast. See [type accessibility](https://eel.is/c++draft/basic.lval) and [reference reinterpretation](https://eel.is/c++draft/expr.reinterpret.cast#11).

The current lookup implementation uses real output storage:

```cpp
void* value;
BOOL found = map.Lookup(key, value);
if (found) {
    out = static_cast<T*>(value);
}
return found;
```

Both lookup overloads use that form; the ID overload uses the retained integer key conversion for its first argument. Failure leaves `out` untouched and returns the original `BOOL`, rather than overwriting it with null or normalizing the result. Both iteration wrappers now use:

```cpp
void* value;
map.GetNextAssoc(pos, key, value);
out = static_cast<T*>(value);
```

Valid iteration always writes a value. The temporary ends after the call; it neither owns nor extends the pointed-to object's lifetime. A successful conversion still requires that the map's erased value represents the intended `T` object/address. It does not perform RTTI checking or a multiple-inheritance adjustment from an unrelated base pointer. This fix addresses output-slot aliasing, not corrupt maps or stale pointees.

## SDK and GitHub comparison

The actual VC5 SDK `msvc/include/AFXCOLL.H` declares `CMapPtrToPtr::Lookup(void*, void*&)` at line 1015 and `GetNextAssoc(POSITION&, void*&, void*&)` at line 1030. Its string-map siblings also return values through `void*&`; string keys use `LPCTSTR`/`CString`, not integer addresses. `POSITION` is MFC's iteration token. The wrappers do not dereference or allocate the key and do not assume ownership of returned values.

VC5 `AFXTEMPL.H:1621–1649` implements `CTypedPtrMap` using reference casts too. A [commit-pinned bundled MFC implementation in WinSCP](https://github.com/mirror/winscp/blob/22fdfe5692ce9cf209807026d63a9d2be2279a72/libs/mfc/include/afxtempl.h#L1678-L1709) independently shows this same vendor-wrapper pattern. This is a source mirror of the implementation, not an original Gruntz source witness. It explains the historical idiom; vendor usage does not resolve standard aliasing safety. Using `CTypedPtrMap` alone would hide, rather than remove, that boundary. No external code is quoted here.

## Three retained integer keys, separately enumerated

1. [MapTyped.h](../../include/Utils/MapTyped.h), `MapLookupById`: `reinterpret_cast<void*>(id)`, from an `i32` ID to opaque map key.
2. [WwdObjMgrInline.h](../../include/Wwd/WwdObjMgrInline.h), `WwdKey`: `reinterpret_cast<void*>(o->GetObjectId())`, from the object's `i32` identity to the same key domain.
3. [WwdGameObject.cpp](../../src/Wwd/WwdGameObject.cpp), post-load serialization: `reinterpret_cast<void*>(node)`, where `i32 node = m_carrierId`.

The misleading local name `node` in item 3 is **not a node pointer**. Presave writes the carrier's `GetObjectId()` to `m_carrierId`; postload looks it up and assigns the returned `CWwdGameObject*` to `m_carrier`. [ChildGroup](../../include/DDrawMgr/DDrawChildGroup.h) owns `CMapPtrToPtr m_registeredGameObjectsById`; [registration macros](../../include/Wwd/WwdObjMgrMacros.h) and [WwdObjMgr.cpp](../../src/Wwd/WwdObjMgr.cpp) insert/remove using `WwdKey`. Registration, lookup and removal therefore share the integer-ID encoding. The pointer-shaped key is not the object's actual allocation address and must never be dereferenced.

These conversions depend on VC5's 32-bit integer/pointer representation and MFC's opaque-key behavior. They do not create a pointee or impose pointee alignment. Do not assume an arbitrary platform has the same integer/pointer mapping, null encoding, or key hash. The new output temporary is independent of that retained target-specific key representation.

## Blame and introducing diffs

| Commit | What it established |
| --- | --- |
| [a1a891c0](https://github.com/sushi-shi/gruntz-decomp/commit/a1a891c089a95c5de61e81d8990a8acf60a947dc) | Introduced four typed output wrappers with `reinterpret_cast<void*&>`, centralizing existing call-site puns. “Language-forced” overstated necessity: a correctly typed temporary is possible. |
| [865b7131](https://github.com/sushi-shi/gruntz-decomp/commit/865b713159904a7c840b9f099ed9ad36fb86759d) | Added the generic `MapLookupById<T>` form with both key and output casts. |
| [a4b9e6b9](https://github.com/sushi-shi/gruntz-decomp/commit/a4b9e6b95b1093007d384d655b95862a20bad4c8) | Replaced visible casts with `MapOutRef`/`AddrWord` unions to preserve destination addresses/codegen. Its claim of “no pun” was not a language-safety proof; reading another union member did not fix the output access type. |
| [0335a536](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) | Removed `MapOutRef` and restored five direct output casts during template recovery. |
| [e17fa3f0](https://github.com/sushi-shi/gruntz-decomp/commit/e17fa3f05bbc61568b3264a8b524f9a5de172521) | Collected object-ID conversion into `WwdKey`, initially via `AddrWord`. |
| [f4e8a7eb](https://github.com/sushi-shi/gruntz-decomp/commit/f4e8a7ebcf5ba18a929c02017e0e9c702af2a2a6) | Earliest serializer reconstruction already saved an ID and looked it up at postload, although raw offsets and map typing were still incomplete. |
| [3b09d668](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) | Removed `AddrWord` at all three retained key boundaries. Later names/getters did not change the ID domain. |

The five genuine output temporaries are the targeted correction from this audit. Compiler verification belongs to the combined cleanup build; no byte-neutrality is assumed. No runtime game test was performed.
