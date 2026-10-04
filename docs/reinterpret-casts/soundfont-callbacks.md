# SoundFont: the cast concealed the wrong interface version

The starting census contained **two expressions** in this family. One was removed and the export binding was corrected. The current code has **two conversions**: the exported data table and a retained alias of the global pointer storage for the SDK output parameter. It no longer calls a three-argument function through a fabricated two-argument prototype.

## The mistaken model

The previous [SoundFont initialization](../../src/Gruntz/SoundFontPath.cpp#L37) used:

```cpp
(reinterpret_cast<SfGetLoadedBankPathname2>(g_sfDevice->SF_GetLoadedBankPathname))(
    g_sfDeviceId,
    &g_sfMidiLocation
);
```

The source field was the 1.01 SDK's `LRESULT (*)(SFDEVINDEX, PSFMIDILOCATION, PSFBUFFEROBJECT)`. The fabricated destination was `LRESULT (*)(SFDEVINDEX, PSFMIDILOCATION)`, with the default `__cdecl` convention. These are incompatible function types. Calling through that converted type would not be valid C++ if the pointee really implemented the declared three-argument function. Caller cleanup and two 32-bit stack arguments do not cure the mismatch.

The earlier export cast was:

```cpp
reinterpret_cast<SfManagerFactory*>(GetProcAddress(g_sfDll, "SFManager"))
```

`SfManagerFactory` was itself `int (__cdecl*)(int, SFMANL101API**)`: the destination was a pointer to a function-pointer object. The extra indirection was not itself the discovery of a bad direct function call. `SFManager` is exported **data**, a `SFMANAGER` table containing `SF_QueryInterface`. The error was substituting an invented callback type and the wrong requested-interface table for those SDK types.

## Evidence and correction

[SFManager_SelectBestDevice](../../src/Gruntz/SFSelectDevice.cpp#L78) requests interface `0x00010000`. The existing [Creative header](../../vendor/sfman-1.01/SFMAN.H) defines that as `ID_SFMANL100API`, distinct from `ID_SFMANL101API` (`0x00010001`), but omitted the 1.0 table declaration.

The [pinned Creative/E-mu header preserved by kX](https://github.com/kxproject/kx-audio-driver/blob/19d1e856533e475b702121c42fd9463855808947/h/sfman/SFMAN.H#L438-L481) supplies the missing table: 1.0 has no pathname-query slot. Its callback at offset `0x34` is `SF_ClearLoadedBank`, taking exactly the two supplied arguments. The [matching implementation](https://github.com/kxproject/kx-audio-driver/blob/19d1e856533e475b702121c42fd9463855808947/kxsfman/sfman32.cpp#L505-L551) independently constructs that 1.0 table, selects it for the requested ID, and exports the manager as data. This is direct API-family evidence, not an analogy or proof that the kX DLL itself shipped with Gruntz.

The project now includes the source-attested [SFMANL100API supplement](../../vendor/sfman-1.00/SFMAN100.H), with [extraction provenance](../../vendor/sfman-1.00/README.md), and declares `g_sfDevice` as `SFMANL100API*`. The bank loop directly calls:

```cpp
g_sfDevice->SF_ClearLoadedBank(g_sfDeviceId, &g_sfMidiLocation);
```

Retail's two pushes, `call [ecx+0x34]`, and caller cleanup now describe a correctly typed operation. Clearing banks 1 through 127 before loading another bank or closing the device also agrees with the caller lifecycle. No missing output-buffer argument needs to be invented. This corrects the old claim that retail intentionally under-called the pathname function.

## Final conversions: individual sites

| Site | Exact expression | Source → destination and contract |
| --- | --- | --- |
| [Load exported manager](../../src/Gruntz/SFSelectDevice.cpp#L87) | `reinterpret_cast<SFMANAGER*>(GetProcAddress(g_sfDll, "SFManager"))` | SDK `FARPROC` return → pointer to the real exported data object |
| [Write returned interface](../../src/Gruntz/SFSelectDevice.cpp#L95) | `reinterpret_cast<PDWORD>(&g_sfDevice)` | `SFMANL100API**` → SDK `DWORD*` output view of the global pointer slot |

The actual VC5 `SFMANAGER::SF_QueryInterface` declaration returns `LRESULT` and takes `(INTERFACEID, PDWORD)`. The code calls that correctly typed member with `ID_SFMANL100API`, exposing the global `g_sfDevice` pointer slot through `PDWORD`. The SDK writes the returned table address directly to that global before the caller checks the result. The earlier repair used a real local `DWORD`, checked success, then decoded and assigned its value. That removed the alias but changed the output storage and assignment order. The direct-global form is retained to recover retail's call/storage shape; it is not a claim that the alias is safe ISO C++. The newer kX witness uses a pointer-sized integer output for newer targets; the historical SDK still specifies a 32-bit `DWORD`.

The first conversion is a Windows loader boundary: [GetProcAddress](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress) can locate exported variables despite its generic function-pointer-shaped return type. The resulting object must really be the aligned manager table, and its lifetime is the loaded module's lifetime. This is a platform-supported conversion, not an ISO guarantee for arbitrary function/object pointers. The second conversion exposes a live, writable, suitably aligned global pointer object as `DWORD*`. Win32 gives the pointer and integer equal widths, but their types are distinct: a write through `DWORD*` is incompatible typed access to the pointer object. The cast itself does not perform the write or validate it. This retained storage alias is therefore a known language-level hazard, not merely an implementation-defined integer-to-pointer conversion. It is also unsuitable for a 64-bit process with the old 32-bit `DWORD` interface. On success the SDK supplies a live table owned by the loaded DLL; callers must not use it after `FreeLibrary`. The current device close path clears readiness and avoids later guarded calls, but the global pointers themselves are not ownership-bearing smart pointers.

## How the error entered

[8d6f9bd097](https://github.com/sushi-shi/gruntz-decomp/commit/8d6f9bd0971489fd230e87800a9af9f058e49ade) recovered the vendor-header material. [a4f9eaa390](https://github.com/sushi-shi/gruntz-decomp/commit/a4f9eaa3907a98034befd95bf64489acc0d6d958) then removed a reconstructed SDK shadow, declared the returned device as `SFMANL101API`, and moved the invented factory/two-argument typedefs into `Dsndmgr/SfManager.h`. Its stated rationale was preserving a supposedly genuine retail under-call while using the real 1.01 header. That rationale overlooked the requested 1.0 interface. [edb4501b5c](https://github.com/sushi-shi/gruntz-decomp/commit/edb4501b5c74ea60f20120b64babbbf075d28019) later rehomed the initializer; it was not the source of the version assumption.

Both invented typedefs remain removed, and the correct 1.0 table plus direct `SF_ClearLoadedBank` call remain in place. The current output-slot cast restores the old global-write behavior through the real SDK signature; it does not restore the invented factory callback or the incorrect 1.01 table. A safe alternative remains a real `DWORD` output followed by address decoding, with an explicit decision about assignment on failed queries. That alternative is not currently retained. Isolating this cast in a wrapper would not cure its incompatible typed access. Any future correction must preserve the actual manager data object, requested interface version, callback signatures and DLL lifetime.
