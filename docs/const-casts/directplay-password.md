# DirectPlay session password

## Site and conflicting types

[CNetMgr::CreateSession](../../src/Net/NetMgr.cpp#L448) receives `const char* password` in [our declaration](../../include/Net/NetMgr.h#L461). The remaining cast is:

```cpp
    if (password != NULL && *password != 0) {
        buf.lpszPasswordA = const_cast<char*>(password);
    }

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->Open(&buf, DPOPEN_CREATE);
```

The actual Microsoft SDK used for this build, `build/toolchain-r2/dx/Include/dplay.h`, declares `DPSESSIONDESC2::lpszPasswordA` as `LPSTR` (line 247), meaning `char*`, and `Open` takes `LPDPSESSIONDESC2`. Thus both the record and its password pointer are mutable at the SDK type boundary. A const record pointer would not make the pointed-to characters const. [Wine's independently maintained header has the same field type](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/include/dplay.h#L247-L251); it is corroboration, not the Microsoft header used to compile Gruntz.

## Why our parameter is const

The SDK field declaration, with the surrounding structure omitted, is:

```cpp
LPSTR lpszPasswordA;
```

Our wrapper checks the password and passes it to DirectPlay without writing through it. That supports an input-string contract for our implementation. It does **not** establish Microsoft's mutation or retention contract. The sole current game caller, [CreateHostSessionAndPlayer](../../src/Gruntz/Multi.cpp#L1257), supplies an empty literal, so the condition skips this assignment. That caller provides no runtime evidence about a nonempty password.

The qualifier is reconstructed, not recovered from original Gruntz source or an original mangled NetMgr symbol. [028fe0e6b](https://github.com/sushi-shi/gruntz-decomp/commit/028fe0e6b277cfcb31365c9964220b04b11e3851) replaced an integer placeholder with a const string parameter. Its message described string-type recovery, without supplying a Windows nonmutation guarantee. [6e4349d0f](https://github.com/sushi-shi/gruntz-decomp/commit/6e4349d0f9094a49c9ff9c670bff9a1a5abb9088) then recognized the session descriptor from its size and field offsets and introduced the named cast at the mutable password field. This was not merely a later conversion of a C-style cast. The [const-contract cleanup](https://github.com/sushi-shi/gruntz-decomp/commit/d943bc941da8cd346c3c0ee985e1e4564ea5091e) retained this SDK boundary.

## Audited GitHub evidence

[Wine's client tests](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/tests/dplayx.c#L3545-L3549) cast a nonempty password literal into the descriptor and call `Open` with `DPOPEN_CREATE`. This supplies a real analogous caller, but the source alone is not a test result on a particular Windows version or provider; the expected success is marked `todo_wine` there. No such test was run for this audit.

[Wine's session creation path](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L3770-L3783) calls its session-descriptor setter. That setter [duplicates the supplied descriptor](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L4492-L4499), including [copying the password into newly allocated storage](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/dplay.c#L4564-L4592). This is concrete ownership evidence for that Wine implementation, not proof about Microsoft's DirectPlay or every service provider.

The [archived DirectX 6.1 dpchat sample](https://github.com/NickCis/directx-6-1-sdk-samples/blob/fcd9eac7bb7d85a331dabd4521d67891692b2255/dplay/src/dpchat/dialog.cpp#L350-L371) zeroes its descriptor and sets a session name, but leaves the password null. It demonstrates the descriptor/API usage, **not** a nonempty password cast or its safety.

## Verdict and removal conditions

Retain the cast at this SDK boundary, with the external contract unresolved. No engine write was found. Casting alone does not modify storage; a subsequent write to originally const storage would be invalid. The SDK field type alone does not prove a write occurs, and the inspected examples do not prove it cannot occur on Windows.

Evidence of the original wrapper's mutable parameter and callers supplying writable storage could justify changing our parameter to `char*` and removing the cast. Alternatively, a verified need for mutation or longer retention would require an owned writable buffer with an established lifetime. Such copying would change allocation and ownership behavior and is not a neutral cleanup. A documented nonmutation/copying guarantee would strengthen the current boundary, but would not remove the C++ conversion required by `LPSTR`. Do not rewrite the SDK structure or COM signature to erase the mismatch.
