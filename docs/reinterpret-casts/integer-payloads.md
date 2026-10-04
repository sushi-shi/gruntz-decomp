# Integer payloads and array cursors

This page accounts for **seven starting expressions**. Five expressions remain: three integer/pointer payload conversions and two cheat-row cursor conversions. The two flag-cursor conversions were removed by using the actual array endpoint.

## Action IDs in pointer-valued slots: two retained sites

[ActRegistry.h](../../include/Gruntz/ActRegistry.h) contains:

| Site | Exact expression | Source → destination |
| --- | --- | --- |
| `ActFindId` | `reinterpret_cast<i32>(g_buteTree.lookup(key))` | `i32*` slot value → `i32` ID |
| `ActInsertId` | `reinterpret_cast<i32*>(id)` | `i32` ID → non-owning `i32*` slot value |

`i32` is `int` in [Ints.h](../../include/Ints.h). [zSymTab<T>](../../include/ZTools/PTree.h#L157) exposes `T* lookup(const char*)` and `T* add(const char*, T*)`; it does not offer an integer-valued specialization. [The global instance](../../src/Bute/ButeGlobals.cpp#L28) is `zSymTab<i32>` constructed with `zPtrColl::PASSIVE`.

[ACT_NAME_ID](../../include/Gruntz/ActNameRegistry.h#L14) queries an action name, treats zero as absence, and stores the allocated action ID. The associated action-handler array and string collection use that same ID. The returned pointer representation is never used here as the address of an integer object. There is therefore no alignment or pointee-lifetime requirement at these two expressions; dereferencing such a value, or enabling ownership-based teardown, would violate the contract.

The conversions rely on VC5's 32-bit integer/pointer representation. They are implementation-defined encodings, not portable guarantees that an arbitrary integer survives every integer→pointer→integer sequence. They are also not ordinary casts between related class pointers. A modern integer-valued container could remove both, but replacing this source-attested pointer container would change its reconstructed API and instantiation.

[0335a53651](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f) introduced these centralized expressions while recovering `zSymTab<i32>` and replacing the old Bute-tree facade. Its relevant reason was actual template/container ownership, not making integer IDs into real allocated integers. The [surviving Monolith zSymTab source at a pinned commit](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h) establishes the typed-pointer container API. It does not independently establish this particular Gruntz table's integer-ID policy; registration and consumption supply that evidence.

## Serialization forwarding: one retained, unresolved type boundary

[GameSerializationCallback](../../src/Gruntz/SerialObjectFactory.cpp#L104) takes `void* payload`, then forwards non-creation modes through:

```cpp
return g_gameReg->SerializeGameState(archive, mode, typeId, reinterpret_cast<i32>(payload))
       != 0;
```

[WorldSerializationCallback](../../include/DDrawMgr/DDrawSurfaceMgr.h#L50) is a `__cdecl` callback with a final `void*` parameter. The destination [CGruntzMgr::SerializeGameState](../../include/Gruntz/GruntzMgr.h#L275) instead takes final `i32`; its [body](../../src/Gruntz/GruntzMgr.cpp#L3630) forwards that word to players, triggers, play state, commands, tile grid, scroll state and statistics.

The world owns the callback protocol. [SaveSnapshot](../../src/DDrawMgr/DDrawSurfaceMgr.cpp#L245) supplies null in save phases. [LoadSnapshot](../../src/DDrawMgr/DDrawSurfaceMgr.cpp#L284) supplies the address of its live local snapshot header during load phases. Object-creation phases supply a result-pointer slot and return from the factory arm before reaching this integer forwarding expression. Thus the callback payload is demonstrably not always an integer ID.

At this expression there is no dereference, alignment change or expired object: a live pointer is encoded into an equal-width integer on the Win32 target. Its numeric representation is implementation-defined and would truncate on a typical 64-bit build retaining `i32`. The integer value carries neither ownership nor lifetime extension. More importantly, **the integer callee parameter is an internal reconstruction choice, not an SDK requirement**. Any operational comment describing the callback itself as integer-valued is too strong. The retained expression is presently required by the reconstructed downstream signatures; a coherent `void*` migration would require tracing the full forwarding family and its callers, not changing this cast alone.

[0545c46c09](https://github.com/sushi-shi/gruntz-decomp/commit/0545c46c099dbb8860c0802d3a83516fa7b88c7c) corrected the callback's fifth word to a pointer. [d6cd75bfcb](https://github.com/sushi-shi/gruntz-decomp/commit/d6cd75bfcb9aee09f99185a8de2b9e123e2dcf12) reconstructed the full body, including forwarding through an `AddrWord` union. [3b09d668f2](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) removed that union and exposed the current pointer-to-integer conversion. The last change exposed an existing protocol mismatch; it did not prove the integer parameter authentic.

A concrete [Microsoft WordPad callback example](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpad.cpp#L279-L305) passes a live local's address through `LPARAM` to `EnumWindows`, then decodes it in the callback. This demonstrates a real integer message slot with explicit callback lifetime. It is an analogue only: unlike Windows' fixed `LPARAM` API, Gruntz's serializer signatures are under reconstruction and may be correctable.

## Booty cursor conversions: two retained, two removed

[BootyStateActivate.cpp](../../src/Gruntz/BootyStateActivate.cpp) contains the cheat-row expressions below; the flag-position expressions have been removed:

| Starting site | Original expression | Source → destination |
| --- | --- | --- |
| `CBootyState::LoadGameAssetNamespaces`, end marker | `reinterpret_cast<i32>(g_bootyCheatMessages[24].m_description + sizeof(BootyCheatMessage))` | `char*` → signed `int` |
| Same function, loop condition | `reinterpret_cast<i32>(p)` | `char*` → signed `int` |
| `CMultiBootyState::LoadGameAssetNamespaces`, current flag position | `reinterpret_cast<i32>(flagPos)` | `const Coord*` → signed `int` |
| Same condition, purported flag end | `reinterpret_cast<i32>(g_bootyTabPos)` | different `const Coord[4]` array decayed to pointer → signed `int` |

### Cheat rows

[BootyCheatMessage](../../include/Gruntz/BootyMessages.h#L9) has a 32-byte encoded-code member and a 128-byte description member; the actual owner is a 25-element record array. The current end expression adds a full 160-byte record size to the final description member. That goes beyond the member array's one-past position. Earlier in the same loop, `p - 0x20` reaches a sibling member and `p += 0xa0` crosses between different description arrays. Casting the resulting address to an integer cannot make the preceding out-of-bounds pointer arithmetic valid. This is a real source-model defect even when its x86 instructions matched retail.

A repair would use a `BootyCheatMessage*` row iterator, copy into its two actual members, and stop at `g_bootyCheatMessages + 25`. Those are valid record-array operations, but this rewrite is not in the current source because it changed the matched byte shape. The current signed-address comparison forces the integer casts; neither the SDK nor the record model requires that comparison. Oversized resource strings remain a separate risk in the unbounded string copies.

[ad10628cd7](https://github.com/sushi-shi/gruntz-decomp/commit/ad10628cd75e7c7a81044097971a551c5e515a14) correctly unified a split table into 25 records but formed the problematic beyond-member endpoint to reproduce a retail address. [3b09d668f2](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) replaced `AddrWord` carriers with the two explicit integer conversions. Later [d912316f58](https://github.com/sushi-shi/gruntz-decomp/commit/d912316f587991908868c1eba495a2a0fab77e20) renamed the record and fields. The naming change was not the origin of the pointer arithmetic.

### Flag positions

The second loop iterates the four entries of `g_bootyFlagPos`, but compared its integer address against a separate `g_bootyTabPos` array. Those arrays happen to be adjacent in the retail data map. They are distinct C++ objects, and `DATA` annotations do not force that ordering in a linked candidate. A later/reordered table could let the loop advance beyond its own array; an earlier table could stop it prematurely. Signed address order is not a substitute for the actual element count.

The corrected condition is `flagPos != g_bootyFlagPos + 4`, keeping traversal within its own array. No aggregate combining unrelated arrays was invented. [1dabef28f6](https://github.com/sushi-shi/gruntz-decomp/commit/1dabef28f6a1d0aee8653cb7d036b3a01a9f0bdc) used `AddrWord` cursor/end carriers; [3b09d668f2](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) made their signed-address comparison explicit. That commit's machine-comparison rationale did not establish portable array-boundary validity.

The flag repair removes layout-dependent traversal. The cheat-row traversal remains a known source-model defect; equal widths do not make it safe. The signed retail branch is evidence about generated instructions, not proof of the original source expression. No surviving original Booty loop source was found to settle its spelling. As an independent array-traversal example, [Microsoft WordPad’s pinned LoadAbbrevStrings](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpad.cpp#L417-L420) indexes records using their element count and accesses each record’s declared fields. It is an actual application pattern, not evidence of original Booty code.
