# DirectDraw's emulation-only sentinel

The single site in
[`CDDrawFrontSurface::SetGeometry`](../../src/DDrawMgr/DDrawSurfacePair.cpp)
is:

```cpp
GUID* emulationOnly = reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY);
hr = deviceManager->CreateDevice(surfaceManager->m_hWnd, emulationOnly, w, h, bpp, mode);
```

The actual build SDK `dx/Include/ddraw.h` defines `DDCREATE_EMULATIONONLY`
as `0x00000002l` (line 193), but `DirectDrawCreate` accepts `GUID*` (line 159).
Our [CreateDevice](../../src/DDrawMgr/DirectDrawMgr.cpp) carries that argument
unchanged to the API when creating the device. It never dereferences address
2 as a GUID. If an existing shared device is reused, the creation call is
skipped.

## Contract and external example

[Microsoft explicitly documents](https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate)
this flag as an accepted value for the GUID-pointer argument. It selects
software emulation. This is an integer token transported through a pointer
slot, not an object placed at that address. Forming and transporting it
depends on the Win32 implementation; dereferencing it would still be invalid.

A real GitHub caller is
[Wine's DirectDraw test](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/ddraw/tests/ddraw1.c#L14995):

```cpp
hr = DirectDrawCreate((GUID *)DDCREATE_EMULATIONONLY, &ddraw, NULL);
```

That source demonstrates actual usage of the same sentinel. The Microsoft
contract supplies the reason it is valid at this boundary; the test's mere
existence is not a test result from this audit.

## Provenance and disposition

[8fab39f2d](https://github.com/sushi-shi/gruntz-decomp/commit/8fab39f2da45f97f92d8c77db66a9d52150d8188)
represented this token through an `AddrWord<GUID>` union.
[3b09d668f2](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0)
removed that invented union and spelled the API conversion directly. The
named cast did not introduce a new dereference or allocation.

Retain it. Neither a real GUID object nor null expresses this API option.
A future generic pointer-validation or copying wrapper must recognize the
sentinel instead of dereferencing it. Do not change the SDK declaration to
an integer merely to hide this documented boundary.
