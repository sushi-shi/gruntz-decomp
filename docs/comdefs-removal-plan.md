# Plan: remove ComDefs.h (and the other reimplemented headers)

> **STATUS (DONE — ComDefs.h deleted).** DirectDraw migrated to real `<ddraw.h>`;
> NetMgr.h's last `<ComDefs.h>` replaced with real `<wtypes.h>`+`<basetyps.h>` (HRESULT +
> STDMETHOD - lighter than `<objbase.h>`'s OLE/RPC chain, same match result); **`include/
> ComDefs.h` deleted** (CLSID stopgap gone with it). Overall exact **1859→1859 (net
> neutral)**: DirectDraw +1, NetMgr −1 (CNetSession2::Verify 100→89.5% - the real COM
> header's declarations perturb that Net TU's regalloc; this is exactly why ComDefs.h
> existed, and the accepted cost of deleting it). Fuzzy −0.01%; the rest are small
> fuzzy%-only drops in already-unmatched functions in TUs that newly pulled windows.h via
> ddraw.h. Two findings changed the plan:
> - **`WaveFormatX.h` KEPT PERMANENTLY (Step 3 CANCELLED-FOR-CAUSE).** Probe-verified:
>   the SDK `WAVEFORMATEX` is **0x12** — `#pragma pack(1)` in EVERY SDK era (mmsystem.h
>   wraps it in pshpack1) — but Monolith's own struct is **0x14** (the last `u16` padded
>   to a 4-byte multiple, unpacked). The DirectSound TUs byte-match at 0x14, so `WaveFormatX`
>   is NOT a scaffolding mirror of `WAVEFORMATEX`; it is the faithful reconstruction of the
>   devs' OWN unpacked wave-format struct. Migrating 0x14→0x12 would shrink every containing
>   layout and break matches. It stays forever. (Evidence: `static_assert(sizeof(WAVEFORMATEX)
>   ==0x12)` + `static_assert(sizeof(WaveFormatX)==0x14)` both pass, wine-cl + clang.)
> - **`m_caps` IS a real `DDCAPS_DX6` (0x17c) — the retail dwSize is CORRECT.** Probe-verified:
>   `sizeof(DDCAPS)==sizeof(DDCAPS_DX6)==0x17c` (the vendored DX6 ddraw.h defaults
>   DIRECTDRAW_VERSION=0x0600, so `DDCAPS` aliases `DDCAPS_DX6`). The retail's hardcoded
>   dwSize=0x17c is therefore `sizeof(DDCAPS)`, NOT a stale/mistaken constant — the earlier
>   "0x13c / larger than DDCAPS" note was WRONG: it read 0x13c off the MSVC5 toolchain's
>   OLDER shadow `DDRAW.H` (the include-order bug below). `m_caps`/`m_helCaps` stay raw
>   `i32[0x5f]` only because the widely-included header must not pull `<ddraw.h>`; the .cpp
>   accesses them through the real `DDCAPS` type (`((LPDDCAPS)m_caps)->dwSize/->dwCaps`,
>   byte-proven codegen-neutral). Real DX types are used everywhere their size is
>   version-stable (RECT/POINT/DDBLTFX/DDSURFACEDESC/DDCOLORKEY/PALETTEENTRY locals).
> - **Lean shared headers stay forward-decl.** `DDSurface.h`/`DirectDrawMgr.h` are
>   included by both MFC and non-MFC TUs; an MFC TU forbids raw windows.h, so the header
>   can't pull ddraw.h — each dispatching `.cpp` includes it instead (PCH is the future fix).
> - **Cold-init bug FIXED (commit "init/clangd: put vendor/directx6/Include on clang's /I
>   path").** Root cause: `clangd.py`'s vendor loop is one-dir-deep, so it added
>   `/I vendor/directx6` but MISSED `vendor/directx6/Include` where the DX6 headers live; the
>   clang AST-dump (ghidra_metadata_generate) + clangd editor tooling — which read that compdb
>   — then resolved `<dsound.h>`/`<ddraw.h>` to the MSVC5 toolchain's OLDER shadow copies (via
>   `/imsvc`), which lack `DSBLOCK_ENTIREBUFFER` and mis-size `DDCAPS`. So a cold `gruntz init`
>   failed at the AST-dump on DirectSoundMgr.cpp. Wine-cl was always fine (cc_wrap prepends the
>   vendored DX). The fix adds `vendor/directx6/Include` on the user `/I` path (searched BEFORE
>   the `/imsvc` system dirs), matching cc_wrap + labels.py — one compdb fix aligns every
>   clang-side consumer. Verified: `gruntz structs` passes; the preprocessor opens the vendored
>   dsound.h. This ALSO disproved the "0x13c DDCAPS" note above (that number was the toolchain
>   shadow's older DDCAPS).


**Goal:** delete `include/ComDefs.h` entirely — every hand-rolled COM view either
migrates to its real SDK header, or (for engine-only interfaces) sources the
`STDMETHOD`/`HRESULT`/`GUID` macros from the real `<windows.h>` instead of our mirror.
(`WaveFormatX.h` does NOT follow — it is a permanent-keep, see Step 3.)

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
3. **WaveFormatX.h → CANCELLED-FOR-CAUSE (permanent-keep, do NOT delete).** The original plan
   was to migrate to real `WAVEFORMATEX` and delete WaveFormatX.h. Probe-verified this is
   WRONG: the SDK `WAVEFORMATEX` is 0x12 (`pack(1)` in every era via pshpack1.h) while
   Monolith's `WaveFormatX` is 0x14 (the trailing `u16 cbSize` padded to a 4-byte multiple,
   unpacked). The DirectSound TUs byte-match at 0x14, so WaveFormatX models the devs' OWN
   struct — it is the faithful reconstruction, not a scaffolding mirror of WAVEFORMATEX.
   Swapping to the packed 0x12 type would shrink every containing layout and break matches.
   WaveFormatX.h stays permanently; it is decoupled from ComDefs.h entirely.
4. **Delete ComDefs.h.** No includers remain → remove the file + the CLSID stopgap; the −1
   stopgap regression is superseded by the (measured) lean-TU windows.h cost.

## Gotchas (learned)
- Every clang-side tool needs `vendor/directx6/Include` on its `/I` path, like cc_wrap.py:
  `labels.py` has it directly (VENDOR_INCS); `clangd.py` (the compdb generator that the
  `structs`/AST-dump step + clangd editor tooling + tidy/caller/rename audits all consume)
  now adds it too (its vendor loop is one-dir-deep and had missed the `Include/` subdir). Any
  new vendored SDK dir with an `Include/` subdir needs the same, on `/I` (user, searched before
  the `/imsvc` system dirs) so the vendored header wins over the toolchain's shadow.
- ninja does NOT recompile a TU when only a vendored/`ComDefs.h` header changes — delete
  `build/objdiff/base/*.obj` before rebuilding or the % is a stale-obj illusion.
- Constants differ across DX versions even when interfaces don't (DSBSTATUS_LOOPING) — always
  diff each hand-rolled DD*/DS* constant vs the vendored header before deleting it.
- A "COM interface" whose retail method identities were never pinned (IDirectPlay4Z) is NOT
  migratable to its real header — keep it hand-rolled, only reparent its macro source.
