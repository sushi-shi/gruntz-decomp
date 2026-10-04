# DirectPlay player long name

## Site and conflicting types

[CNetMgr::CreatePlayer](../../src/Net/NetMgr.cpp#L639) receives `const char* longName` under [our declaration](../../include/Net/NetMgr.h#L454). Its remaining conversion is:

```cpp
    name.lpszLongNameA = const_cast<char*>(longName);
```

The local `DPNAME` then goes to `IDirectPlay4A::CreatePlayer`. The actual Microsoft SDK at `build/toolchain-r2/dx/Include/dplay.h` declares `DPNAME::lpszLongNameA` as `LPSTR` (line 372), meaning `char*`, and the method accepts `LPDPNAME`. [Wine's corresponding header](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/include/dplay.h#L197-L203) confirms the same field shape. Our const pointee and the SDK's mutable pointee are the precise conflict; neither the SDK type nor constness of the containing record proves character nonmutation.

## Why our parameter is const

The SDK field declaration, with the surrounding structure omitted, is:

```cpp
LPSTR lpszLongNameA;
```

The engine never writes through this parameter. After the SDK call, [AddPlayer](../../src/Net/NetMgr.cpp#L589) sends it to [CNetPlayerNode::Initialize](../../src/Net/NetMgr.cpp#L1076), which copies it into a `CString`. Current [host](../../src/Gruntz/Multi.cpp#L1263), [join](../../src/Gruntz/Multi.cpp#L1292), and [local-player](../../src/Gruntz/Multi.cpp#L2933) callers pass an empty literal. Therefore our own code treats it as text input; the long-name field really receives a pointer to a literal, unlike the password case whose empty-string guard skips assignment.

Const was inferred during reconstruction, not recovered from original source or an original decorated NetMgr symbol. [028fe0e6b](https://github.com/sushi-shi/gruntz-decomp/commit/028fe0e6b277cfcb31365c9964220b04b11e3851) changed the integer placeholder to a const string. [756d31e10](https://github.com/sushi-shi/gruntz-decomp/commit/756d31e10cad9e523ce8703f8f05f9657bb8eb2c) recognized the four-word stack record as `DPNAME` and introduced the named cast at its long-name field. That commit's evidence was the record layout, not a Windows read-only guarantee. The [const-contract cleanup](https://github.com/sushi-shi/gruntz-decomp/commit/d943bc941da8cd346c3c0ee985e1e4564ea5091e) retained it and made the short-name boundary consistent.

## Audited GitHub examples and their limits

The [DirectX 6.1 dpchat sample](https://github.com/NickCis/directx-6-1-sdk-samples/blob/fcd9eac7bb7d85a331dabd4521d67891692b2255/dplay/src/dpchat/dialog.cpp#L375-L383) explicitly leaves the long name null. [Blitz's player creation](https://github.com/blitz-research/blitz3d_soloud/blob/f132a78c9f2da255905bf477cb845994324b865a/bbruntime/multiplay.cpp#L165-L179) also leaves it null after zeroing the record; its string cast concerns only the short name. Neither is evidence for casting a nonnull long name. Null in those examples is not evidence that replacing our empty literal with null preserves behavior.

[Wine's client test does supply a nonnull long name](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/tests/dplayx.c#L4947-L4988): it casts literals into both fields and passes that record to its `CreatePlayer` test wrapper with expected success and matching returned names. This is an actual analogous API caller, not a runtime result from this audit or a universal Windows nonmutation guarantee.

[Wine's player creation](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L1835-L1855) duplicates the name record. Its [name-copy implementation](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L2012-L2060) copies both name fields, and its [string helper](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L261-L301) reads the source into separate destination storage. This establishes that Wine implementation's handling, not a Microsoft Windows guarantee.

## Verdict and removal conditions

Retain this SDK conversion with the external nonmutation/retention assumption explicit. The engine's read-only use is supported; safety across Microsoft's implementation and providers is not proven by the inspected sources. Writing even the terminator of an empty literal would be invalid. No such write has been established by this audit.

Changing our parameter to `char*` merely makes literal callers less explicit about the same assumption. It requires authentic mutable-interface evidence and appropriate caller storage to count as a repair. A writable owned copy could remove the cast if mutation or retention is established and lifetime requirements are known, but would add allocation/copy behavior. Changing empty text to null needs separate semantic and retail evidence. A documented read-only/copying contract would strengthen the current choice without changing the SDK type mismatch. Keep the native SDK structure and COM signature intact.
