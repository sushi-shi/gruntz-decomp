# Plan: remove ComDefs.h (and the other reimplemented headers)

**Goal:** delete `include/ComDefs.h` entirely — every hand-rolled COM view either
migrates to its real SDK header, or (for engine-only interfaces) sources the
`STDMETHOD`/`HRESULT`/`GUID` macros from the real `<windows.h>` instead of our mirror.
Then `WaveFormatX.h` follows.

## Why ComDefs exists (the constraint to break)
ComDefs.h is a hand-rolled mirror of `<basetyps.h>`+`<wtypes.h>` (STDMETHOD/HRESULT/
GUID/IID/CLSID) so **lean TUs** (no `<windows.h>` — e.g. SoundDevice.cpp) can declare/
dispatch COM interfaces without pulling the OLE/RPC/windows chain, whose extra
declarations perturb MSVC5 regalloc on already-matched functions (ComDefs.h's own
caveat: −1.2% on CLightFxRender). Removing ComDefs therefore **forces windows.h into
~16 lean TUs** — the regression is the price, accepted per the user's mandate
(measure per-TU; DirectSound's lean SoundDevice.cpp turned out byte-neutral, so many
may be free).

## Reimplemented headers (inventory)
- **ComDefs.h** — COM macros + IID/CLSID/GUID types. ~27 .cpp see it (~16 lean).
- **WaveFormatX.h** — a `WAVEFORMATEX`-shaped struct (mmsystem.h mirror). 9 users.

## Current state (done)
- DirectSound → real `<dsound.h>` (8 files, lean SoundDevice.cpp pulled windows.h, neutral).
- DirectPlay lobby → real `<dplobby.h>` (+1).
- DirectInput → real `<dinput.h>` (m-7, +?).
- **ComDefs CLSID/REFCLSID/REFGUID stopgap** (commit dd74af9d): completes the wtypes.h
  `__IID_DEFINED__` mirror so ComDefs *coexists* with a real DX header in one TU (fixes
  the cguid.h/ole2.h CLSID starvation). Costs −1 (1860→1859); resolves when ComDefs dies.

## The interfaces, by category
**DX-SDK (migrate to the real SDK header — clean where slots are well-known):**
- `IDirectDraw2Z`→IDirectDraw2, `IDirectDrawSurfaceZ`→IDirectDrawSurface(3),
  `IDirectDrawPaletteZ`→IDirectDrawPalette  (`<ddraw.h>`; DirectDrawMgr.h + DDSurface.h)
- 3 local `IDirectDrawSurfaceZ` copies (GameMode.cpp / ChatBoxOwner.cpp / DrawDebugStats.cpp)
  → real IDirectDrawSurface (m-4's DC-dedup local views).
- `IDirectPlay4Z` (NetMgr.h) — **best-effort, does NOT migrate cleanly**: the code calls
  GetPlayerData2/EnumGroups/EnumGroupsCb/GetData5/GetData2/GetSessionDesc which don't match
  real IDirectPlay4's names/signatures (only QI/Release/GetMessageCount@17/Receive@25 align).
  Its DirectPlay method identities were never pinned. Treat as an ENGINE interface (below).

**ENGINE (no SDK header — keep hand-rolled, but source STDMETHOD from `<Win32.h>` not ComDefs):**
- `INetReleasable` (NetMgr.h; used by 5 lean Net TUs), `IDirectPlay4Z` (best-effort),
  `CImageSource`/`CImagePayload`/`CImageProvider` (ImageProbe.cpp), `CWorldDispatch`
  (already off ComDefs — GruntzMgr.cpp uses Mfc.h since the dplobby migration).

## Sequenced plan (ComDefs deleted LAST, per m-7)
1. **DirectDraw → `<ddraw.h>`** (unblocked by the CLSID stopgap). Same recipe as DirectSound:
   rename Z→real, drop the hand-rolled interface structs + DD* constants that match ddraw.h
   (KEEP any whose value differs — cf. the DSBSTATUS_LOOPING 0x02-vs-0x04 DX5/DX6 catch),
   real IID GUIDs from dxguid instead of the `IID_IDirectDraw2[16]` byte arrays, forced-SDK
   pointer casts, `<Win32.h>`+`<ddraw.h>` into the 6 lean surface TUs. Verify byte-exact.
2. **Engine interfaces off ComDefs:** give the lean Net TUs + ImageProbe.cpp `<Win32.h>`
   (windows.h) so INetReleasable/IDirectPlay4Z/CImage* get their STDMETHOD from the real
   `<basetyps.h>`; drop `#include <ComDefs.h>` from NetMgr.h + ImageProbe.cpp. Measure/accept
   the lean-TU regalloc hits.
3. **WaveFormatX.h → real `WAVEFORMATEX`** in the (now windows.h-bearing) Dsndmgr TUs; delete
   WaveFormatX.h.
4. **Delete ComDefs.h.** No includers remain → remove the file + the CLSID stopgap; the −1
   stopgap regression is superseded by the (measured) lean-TU windows.h cost.

## Gotchas (learned)
- `labels.py` (clang-cl label step) needs `vendor/directx6/Include` on its `/I` path, like
  cc_wrap.py (added). Any new vendored SDK dir with an `Include/` subdir needs the same.
- ninja does NOT recompile a TU when only a vendored/`ComDefs.h` header changes — delete
  `build/objdiff/base/*.obj` before rebuilding or the % is a stale-obj illusion.
- Constants differ across DX versions even when interfaces don't (DSBSTATUS_LOOPING) — always
  diff each hand-rolled DD*/DS* constant vs the vendored header before deleting it.
- A "COM interface" whose retail method identities were never pinned (IDirectPlay4Z) is NOT
  migratable to its real header — keep it hand-rolled, only reparent its macro source.
