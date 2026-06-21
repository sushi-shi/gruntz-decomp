# WinAPI and DirectX Caller Naming Plan

Generated from `build/exe/GRUNTZ.EXE` disassembly plus
`build/ghidra-enrich/exports/functions.csv`, `symbols.csv`, and
`build/gen/symbol_names.csv`.

The exhaustive extraction is in `docs/api-caller-name-plan.tsv`. It includes
every direct import caller found by this pass, plus DirectX wrapper callers that
call local DirectX import wrappers rather than the IAT directly.

## Counts

| Category | Status | Count |
|---|---:|---:|
| `winapi` | greenfield | 226 |
| `winapi` | tracked/already tried | 116 |
| `winapi` | named but not in baseline | 444 |
| `directx` | tracked/already tried | 8 |
| `directx-wrapper-caller` | greenfield | 4 |
| `directx-wrapper-caller` | tracked/already tried | 2 |
| `thirdparty` | greenfield | 19 |
| `thirdparty` | tracked/already tried | 1 |

`thirdparty` is separated so Miles/Smacker imports do not pollute the WinAPI
queue.

## DirectX

The imported DirectX functions are mostly hidden behind local wrapper functions.
Those wrappers are already tracked, so the useful greenfield work is their
callers and lower dependencies.

| RVA | Status | Current name | Proposed name | Evidence | Action |
|---:|---|---|---|---|---|
| `0x132ce0` | tracked | `DirectInputMgr2::Stub_132ce0` | `DirectInputMgr2::Create` | Calls `DirectInputCreateA`; normalized name already in `symbol_names.csv`. | Do not retry. |
| `0x136550` | tracked | `DirectSoundMgr::Stub_136550` | `DirectSoundMgr::Create` | Calls `DSOUND.#1` / `DirectSoundCreate`. | Do not retry. |
| `0x141dc0` | tracked | `CDirectDrawMgr::Stub_141dc0` | `CDirectDrawMgr::CreateDevice` | Calls `DirectDrawCreate` and `DirectDrawEnumerateA`; normalized name already in `symbol_names.csv`. | Do not retry. |
| `0x17c040` | tracked | `CDirectDrawMgr::Stub_17c040` | `CDDPageMgr::Init` | Calls `DirectDrawCreate`; normalized name already in `symbol_names.csv`. | Do not retry. |
| `0x08eca0` | tracked | `CGruntzMgr::InitializeLobbyConnectionSettings` | same | Calls DPLAYX ordinal 4 / `DirectPlayLobbyCreate`. | Do not retry. |
| `0x1780b0` | tracked | `CNetMgr::Stub_1780b0` | `CNetMgr::CreateDirectPlay` / `CNetMgr::OpenDirectPlay` | Calls DPLAYX ordinal 1 and then DirectPlay COM methods. | Do not retry yet; use as dependency context. |
| `0x178280` | tracked | `CNetMgr::Stub_178280` | `CNetMgr::EnumDirectPlayConnections` | Calls DPLAYX ordinal 2 with callback `0x1782d0`. | Do not retry yet; use as dependency context. |
| `0x1782d0` | tracked | `CNetMgr::Stub_1782d0` | `CNetMgr::CreateDirectPlayConnection` | Calls DPLAYX ordinal 1 and helper `0x178360`. | Do not retry yet; use as dependency context. |

Greenfield DirectX wrapper callers:

| RVA | Current name | Proposed name | Evidence | Priority |
|---:|---|---|---|---|
| `0x0b77a0` | `FUN_004b77a0` | `CNetMgr::OpenDirectPlaySession` / `CNetMgr::ConnectDirectPlay` | Calls `0x1780b0`; uses `CNetMgr+0x524`, registry/config strings, and network setup branches. | High when entering NetMgr connection flow. |
| `0x137720` | `FUN_00537720` | `DirectSoundMgr::CreateDefault` / `CreateDirectSoundMgr` | Thin two-argument wrapper around `DirectSoundMgr::Create(..., flags=0)`. | Good small leaf. |
| `0x1644a0` | `FUN_005644a0` | `CDDPageMgr::CreateDeviceAndPrimarySurface` | Stores width/height/depth, calls `CDirectDrawMgr::CreateDevice`, creates/attaches a page/surface. | High graphics bootstrap candidate. |
| `0x17c2a0` | `FUN_0057c2a0` | `CDDPageMgr::CreateHiddenWindowAndInit` | Creates an MFC window, then calls `CDDPageMgr::Init` at `0x17c040`. | High if continuing DirectDraw init. |

Tracked DirectX wrapper callers:

| RVA | Name | Evidence |
|---:|---|---|
| `0x083450` | `RezSync::Init` | Calls `DirectInputMgr2::Create`. |
| `0x0b78b0` | `CNetMgr::Stub_0b78b0` | Calls `CNetMgr::EnumDirectPlayConnections`. |

## WinAPI

The full WinAPI caller list is too broad for manual planning because it includes
static MFC and CRT. The matching-useful split is:

| Region | Greenfield count | Matching value |
|---|---:|---|
| Game/engine RVAs before `0x120000` | 161 | Mostly UI, collision/rect, dialog, game-state, and NetMgr helpers. |
| CRT/MFC-ish `0x120000`-`0x132fff` | 2 | Low priority; library/runtime, not game structure. |
| Media/graphics/sound/image `0x133000`-`0x17ffff` | 52 | High value for DSound/DDraw/Image/Smacker-facing work. |
| Late/game tail `0x180000`-`0x1affff` | 2 | Low/medium; isolated helpers. |
| MFC/runtime `>=0x1b0000` | 9 | Low priority unless replacing MFC labels. |

High-value greenfield WinAPI clusters:

| Cluster | RVAs | Proposed names / role | APIs |
|---|---|---|---|
| Frame clocks / pacing | `0x00cd00`, `0x00cd70`, `0x019f50`, `0x0861e0`, `0x0c7ec0`, `0x0da200`, `0x11b3b0`, `0x11b7c0`, `0x136e20`, `0x136fe0`, `0x137ac0`, `0x137e30`, `0x1380d0`, `0x147f30`, `0x147ff0`, `0x1480a0`, `0x15cbe0`, `0x17fe00` | `*::UpdateClock`, `*::GetElapsedTime`, or `*::ThrottleFrame` depending on owner. | `timeGetTime`, `GetTickCount` |
| Message posting / state transitions | `0x0143e0`, `0x014720`, `0x01f8a0`, `0x023090`, `0x039440`, `0x064540`, `0x08f340`, `0x08f480`, `0x08f530`, `0x090220`, `0x092710`, `0x092f00`, `0x0953f0`, `0x0cbaf0`, `0x0cdb10`, `0x0ceae0`, `0x0cfbd0`, `0x0d7220`, `0x0de590` | `PostCommand`, `PostGameMessage`, or state-specific `Request*` handlers. | `PostMessageA` |
| Dialog/control helpers | `0x036d00`, `0x036d50`, `0x036da0`, `0x036e10`, `0x036ec0`, `0x0371e0`, `0x037260`, `0x037ff0`, `0x038150`, `0x038220`, `0x03af90`, `0x03b1a0`, `0x03b310`, `0x090260`, `0x092ab0`, `0x09dff0`, `0x09e2d0`, `0x0bda00`, `0x0bdb90`, `0x0bdc00`, `0x0bdd60`, `0x0bdfe0`, `0x0be0a0`, `0x0be2f0`, `0x0be400`, `0x0be490`, `0x0be760`, `0x0e35f0`, `0x0e3a40`, `0x0e3b20`, `0x0e3be0`, `0x0e3e80`, `0x0e4850` | Dialog procs, option loaders/savers, timer-backed dialog updates. | `GetDlgItem`, `SendMessageA`, `EndDialog`, `SetTimer`, `KillTimer` |
| Rect/collision/UI geometry | `0x00c840`, `0x01a700`, `0x0267c0`, `0x02a570`, `0x02ab80`, `0x02ae00`, `0x02b340`, `0x02c140`, `0x02c690`, `0x02dfa0`, `0x02e3a0`, `0x031ca0`, `0x032060`, `0x032ce0`, `0x033520`, `0x04a9f0`, `0x04d800`, `0x057db0`, `0x075c60`, `0x077df0`, `0x078060`, `0x09ea60`, `0x0d00a0`, `0x0d0b30`, `0x0d5f90`, `0x0d8c60`, `0x0e6020`, `0x0ecc90`, `0x0ed9f0`, `0x0ee800`, `0x0f0e20`, `0x0f36a0`, `0x0f42f0`, `0x0f60f0`, `0x0fe460`, `0x0fe520`, `0x0fe600`, `0x107d00`, `0x115300`, `0x115930`, `0x115b30`, `0x13f460`, `0x1538c0`, `0x153b20`, `0x153d90`, `0x153ff0`, `0x154270`, `0x1544d0`, `0x154750`, `0x161e80`, `0x168080`, `0x179e70`, `0x1804a0`, `0x182ab0` | Use owner/context names such as `BuildRect`, `HitTest`, `IntersectBounds`, `CopyBounds`. | `SetRect`, `CopyRect`, `OffsetRect`, `IntersectRect`, `PtInRect` |
| GDI text/drawing | `0x021f20`, `0x022160`, `0x022360`, `0x022810`, `0x0396f0`, `0x039a60`, `0x164380`, `0x164420` | `Draw*Text`, `RenderLabel`, `DrawClippedText`. | `DrawTextA`, `SelectObject`, `SetTextColor`, `SetBkMode` |
| Resource loaders | `0x136a30`, `0x136ce0`, `0x144270`, `0x1479e0`, `0x1775f0` | `LoadResourceBlob`, `LoadBitmapResource`, or class-specific resource init. | `FindResourceA`, `LoadResource`, `LockResource` |
| Palette / DIB / image | `0x1485b0`, `0x174fe0`, `0x1750e0`, `0x1751f0`, `0x1752f0`, `0x1753f0`, `0x1757c0`, `0x175c90`, `0x176df0`, `0x177070`, `0x1770a0`, `0x1770e0`, `0x177160`, `0x17cd90` | `CImage::*Palette*`, `CImage::DecodeBmpHeader`, `CreatePaletteFromDC`, `DeletePalette`. | `CreateDIBSection`, `CreatePalette`, `GetSystemPaletteEntries`, `SelectPalette` |
| Window focus / app shell | `0x118930`, `0x1192d0`, `0x13d4c0`, `0x17c3f0`, `0x17c510`, `0x17c790` | `FocusGameWindow`, `IsWindowMinimized`, `DestroyGameWindow`, `ShowGameCursor`, `PumpMessages`. | `SetFocus`, `IsIconic`, `DestroyWindow`, `ShowCursor`, `PeekMessageA` |
| Process/config launch | `0x08f120`, `0x090860`, `0x0f8e20`, `0x1bf577`, `0x1d4a18` | `LaunchExternalProcess`, `ReadLaunchCommand`, library unload helpers. | `CreateProcessA`, `RegQueryValueA`, `FreeLibrary` |

## Matching Order

1. DirectX greenfield wrapper callers: `0x137720`, `0x1644a0`, `0x17c2a0`, then `0x0b77a0`.
2. WinAPI media/graphics helpers around `0x136a30`-`0x177160`, especially resource, palette, and DIB functions.
3. WinAPI app/window shell leaves: `0x17c790`, `0x17c3f0`, `0x17c510`, `0x13d4c0`.
4. Game/engine dialog and message-posting helpers only when their owning flow is being matched.
5. Rect/collision helpers in batches by caller tree; do not create isolated stubs for every `IntersectRect` leaf unless that subtree is active.
6. Skip CRT/MFC/runtime greenfield rows for now unless they block a project function.

## Notes

- `named-untracked` rows in the TSV are mostly MFC/CRT/library functions and
  already-recovered project names not present in `match_baseline.tsv`. Treat
  them as context, not immediate matching targets.
- `tracked` means either the raw Ghidra name or the normalized generated name is
  already present in `config/match_baseline.tsv`.
- COM vtable calls on DirectDraw/DirectSound/DirectPlay interfaces are not fully
  recoverable from direct import scanning alone. The DirectX list catches creator
  imports and wrapper callers; method-level naming still needs interface-field
  typing from the surrounding manager classes.
