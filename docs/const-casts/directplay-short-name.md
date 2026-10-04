# DirectPlay player short name

## Site and conflicting types

[CNetMgr::CreatePlayer](../../src/Net/NetMgr.cpp#L639) and [its declaration](../../include/Net/NetMgr.h#L454) take `const char* shortName`. The exact SDK assignment is:

```cpp
    name.lpszShortNameA = const_cast<char*>(shortName);
```

The resulting `DPNAME name` is passed to `IDirectPlay4A::CreatePlayer`. In the actual Microsoft build SDK, `build/toolchain-r2/dx/Include/dplay.h`, `DPNAME::lpszShortNameA` is `LPSTR` (line 367), or `char*`; the method takes `LPDPNAME`. [Wine's compatible declaration](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/include/dplay.h#L187-L203) agrees about that mutable field. These are SDK types, not project declarations to change. Neither a mutable field nor a const pointer to the containing record establishes whether the API writes the characters.

## Why our parameter is const

The SDK field declaration, with the surrounding structure omitted, is:

```cpp
LPSTR lpszShortNameA;
```

Our wrapper only forwards the text to DirectPlay and then [AddPlayer](../../src/Net/NetMgr.cpp#L589). [CNetPlayerNode::Initialize](../../src/Net/NetMgr.cpp#L1076) assigns it to the node's owned `CString`. Callers supply a [literal](../../src/Gruntz/Multi.cpp#L1263) or [CString data](../../src/Gruntz/Multi.cpp#L2933); none requests an output buffer. CString temporaries remain alive through the call expression. This proves the engine's read-only use and its own copy, not the external API's copying or retention behavior.

This is our inferred input contract, not an original Gruntz signature. [028fe0e6b](https://github.com/sushi-shi/gruntz-decomp/commit/028fe0e6b277cfcb31365c9964220b04b11e3851) typed the joining wrapper's names as const strings but cast its short name when forwarding to the then-untyped `CreatePlayer` parameter. [b9fce95b2](https://github.com/sushi-shi/gruntz-decomp/commit/b9fce95b27297a9ed6ec3aa5e52c6c482aa10c25) changed that parameter from `void*` to `char*`, citing already-cast callers. That was reconstruction evidence, not original mangling evidence.

The [const-contract cleanup](https://github.com/sushi-shi/gruntz-decomp/commit/d943bc941da8cd346c3c0ee985e1e4564ea5091e), authored in local checkpoint `482063e860be1771632b181c717c249c21a22d2e`, made `CreatePlayer` accept const short names. It removed casts from the joining wrapper and game callers and introduced this single conversion at `DPNAME`. Compiled bodies and ordered references were unchanged after mapping the intentionally changed reconstructed symbols; that does not prove the external contract.

## Audited GitHub examples

The [DirectX 6.1 dpchat sample](https://github.com/NickCis/directx-6-1-sdk-samples/blob/fcd9eac7bb7d85a331dabd4521d67891692b2255/dplay/src/dpchat/dialog.cpp#L350-L383) accepts an already-mutable player-name pointer, stores it in `DPNAME`, and calls DirectPlay. It shows the SDK pattern but does not exercise const input.

[Blitz's player creation](https://github.com/blitz-research/blitz3d_soloud/blob/f132a78c9f2da255905bf477cb845994324b865a/bbruntime/multiplay.cpp#L165-L179) instead builds a local string, appends a terminator, casts its data pointer to the SDK field type, and invokes `CreatePlayer`. It is a real analogous constness workaround, not a correctness guarantee.

[Wine's player creation](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L1835-L1855) duplicates the names. Its [copy helper](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L2012-L2060) allocates separate name storage. This establishes the inspected Wine path's copying behavior, not Microsoft's implementation or all providers.

## Verdict and removal conditions

The two short-name assignments above illustrate the difference directly
(setup and calls omitted):

```cpp
// DirectX sample: already-mutable input.
dpName.lpszShortNameA = lpszPlayerName;

// Blitz: explicit removal of the string view's constness.
name.lpszShortNameA=(char*)t0.data();
```

Keep the conversion localized at the SDK boundary. There is no demonstrated engine mutation bug; Windows nonmutation and lifetime guarantees remain unverified here. A cast does not write, but a later write through this pointer could violate CString sharing or attempt to modify a literal.

Removing the cast by reverting our parameter to `char*` would push the mismatch back onto CString callers. It needs evidence of a truly mutable original interface and writable caller storage, not just a lower cast count. An owned writable copy is an alternative only after establishing necessary mutation/retention and the buffer lifetime; it adds behavior absent from the current reconstruction. An authoritative read-only/copying API contract would justify the current assumption, but the SDK's `LPSTR` would still require this conversion. Do not alter SDK layout or declarations.
