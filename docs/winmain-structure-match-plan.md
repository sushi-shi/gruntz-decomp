# WinMain Dependency Tree Plan

Goal: plan the top-down `WinMain` structure without matching code in this pass.
If a function already appears in `config/match_baseline.tsv`, treat it as tried
and do not schedule another direct attempt. Instead, descend into its unresolved
callees/blockers and create stubs for those lower nodes when they are not already
tracked.

Evidence used here:

- `src/Gruntz/WinMain.cpp`
- `config/match_baseline.tsv`
- `config/match-queue.md`
- `/home/sheep/Projects/gruntz/build/exe/GRUNTZ.EXE` thunk disassembly
- `/home/sheep/Projects/gruntz/build/ghidra-enrich/exports/functions.csv`

## Top-Down Stop Rules

- Stop at functions already in `match_baseline.tsv`; do not retry them.
- Stop at CRT/library labels, Win32 imports, and DirectX/DPlay import ordinals.
- Stop at already matched functions unless their direct lower deps are bare and
  useful for the high-level startup tree.
- Prefer repeated blocker thunks: they unlock more than one parent.

## Tree

### 0. `_WinMain@16` `0x11c860`

Status: tried and 100%; stop at this node.

Direct callees and what to do:

| Edge | Resolved target | Status | Plan |
|---|---:|---|---|
| `CheckExePath` thunk `0x002e6e` | `0x118ce0` `Utils::WinAPI::LegacyAreProcessesRunning` | tried | Do not retry; descend only to its blocker. |
| `StartupGate` thunk `0x002f59` | `0x01f9b0` `StartUpPrompt` | tried | Do not retry; descend only to blockers. |
| `SubstringMatch` `0x120090` | `_strstr` | LIBCMT | Do not stub. |
| `VersionScan` `0x120900` | `_sscanf` | LIBCMT | Do not stub. |
| `SettleDelay` | `0x13dfe0` `Utils::WinAPI::ActiveWait` | tried and 100% | Do not retry. |
| `AdvancedOptionsDialogProc` | `0x00afb0` | tried and 100% | Do not retry. |
| `CGruntzApp` construction/init/run/delete path | app/window/manager nodes below | mixed | Descend by tree below. |

### 1. Process/path gate branch

Parent: `CheckExePath` thunk -> `0x118ce0`
`Utils::WinAPI::LegacyAreProcessesRunning`.

Status: `0x118ce0` already tried as `?Stub_118ce0@WinAPI@Utils@@YAXXZ`.

Lower blocker:

| RVA | Current evidence | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x0024f5` | thunk to `0x118f60` | `thunk_LegacyFindModule` | No | `0x118f60` is already tracked as `LegacyFindModule` at 99.9%. Stop here. |

No new stubs from this branch unless a later export shows a non-ToolHelp helper
inside `0x118ce0`.

### 2. Startup/CD gate branch

Parent: `StartupGate` thunk -> `0x01f9b0` `StartUpPrompt`.

Status: `StartUpPrompt` already tried; do not retry directly.

Lower blockers:

| RVA | Resolved target | Status | Plan |
|---:|---|---|---|
| `0x002540` | `0x01fd50` `Utils::WinAPI::IsGruntzCDInAnyDrive` | tried and 100% | Stop. |
| `0x004282` | `0x1189c0` `Utils::WinAPI::FileExists` | tried and 100% | Stop. |

No new stubs from this branch. `StartUpPrompt` is blocked only by already
tracked helpers.

### 3. App/window lifecycle branch

Parents reached from `WinMain`:

- `CGruntzApp::VirtualUnknownMethod03`
- `CGameApp::VirtualUnknownMethod03`
- `CGameApp::VirtualUnknownMethod02`
- `CGameApp::RunMessageLoop`
- `CGameApp::GameWindowProc`
- `CGameWnd` create/destroy/quit handlers

Status: all are already tried; many are 100%. Do not retry these in this plan.

Lower blockers and stop points:

| RVA | Current evidence | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x13dbc0` | queued `CGameApp::InitializeGameManager`, blocked by `0x13dd10` | `CGameApp::InitializeGameManager` | No | Parent already tried; blocker `0x13dd10` is tracked. |
| `0x13dd10` | `WAP32::CGameMgr::CGameMgr` | base manager ctor | No | Already tracked and 100%; stop. |
| `0x13de70` | `WAP32::CGameMgr::InitTimeFields` | frame-clock field init | No | Already tracked and 100%; stop. |
| `0x13dea0` | `WAP32::CGameMgr::UnknownMethodInitializeTimeGlobal` | frame-clock global init | No | Already tracked and 100%; stop. |
| `0x13cff0` | `CGameApp::GameWindowProc` | WndProc dispatcher | No | Already tried; do not schedule another pass. |

No new app/window stubs are needed to go lower from `WinMain`; this branch is
already represented.

### 4. `CGruntzApp` delete/lifetime branch

Parent: `delete g_pApp` from `WinMain`.

Status: `CGruntzApp::~CGruntzApp` already tried and 100%; deleting destructor
label `0x112820` already tried. Do not retry.

Lower blocker:

| RVA | Resolved target | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x002e0f` | thunk to `0x110570` | `LoadSwitchDownSprite` | No | Not part of app lifetime structure; this is a downstream asset/status-bar function. Skip for this tree. |

### 5. Game-manager creation branch

Parent: `CGruntzApp::InitializeGameManager` creates `CGruntzMgr`.

Status: parent is tried and 100%; `CGruntzMgr::CGruntzMgr` at `0x083030` is
already tried as a stub. Do not retry the constructor.

Lower blocker:

| RVA | Current evidence | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x13dd10` | `WAP32::CGameMgr::CGameMgr` | base manager ctor | No | Already tracked and 100%; stop. |

No new constructor deps from current queue data. If the constructor body is
expanded later, use its direct callee list to seed lower stubs, not the ctor
itself.

### 6. Game-manager first-run branch

Parent: `CGruntzMgr::InitializeLobbyConnectionSettings` at `0x08eca0`.

Status: already tried; do not retry.

Lower blockers:

| RVA | Current evidence | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x177660` | import thunk `Ordinal_4` | `DirectPlayLobbyCreate` | No | DPLAYX import; do not match. |
| `0x1776a0` | `CNetMgr::ReportError` | DirectPlay diagnostic formatter | No | Already tracked and 100%; stop. |

Optional later DPlay leaves, not immediate `WinMain` structure:

| RVA | Current evidence | Possible name | Stub? | Notes |
|---:|---|---|---|---|
| `0x178360` | bare `FUN_00578360`, blocker for DPLAY ordinal #1 wrapper | `DirectPlayCreateWrapper` or `CreateDirectPlaySession` | Yes, later | Only if continuing into DirectPlay manager setup. |
| `0x178430` | bare `FUN_00578430`, blocker for DPLAY ordinal #2 wrapper | `DirectPlayEnumerateWrapper` or `EnumDirectPlayConnections` | Yes, later | Same scope as above. |

### 7. Game-manager shutdown branch

Parent: `CGruntzMgr::~CGruntzMgr` -> `CGruntzMgr::UnknownClose`.

Status: both are already tried; do not retry them directly.

Important resolved stop points from `UnknownClose`:

| Edge | Resolved target | Status | Plan |
|---|---:|---|---|
| `0x0043c2` | `0x0855e0` `CGruntzMgr::UnknownClose` | tried | Do not retry; descend. |
| `0x004282` | `0x1189c0` `FileExists` | tried and 100% | Stop. |
| `0x004214` | `0x08fa70` `CGruntzMgr::GetGruntzDriveLetter` | tried and 100% | Stop. |
| `0x13cdb0` | `WAP32::CGameMgr::UnknownClose` | tried and 100% | Stop. |
| `0x139330` / `0x139460` | `RegistryHelper` methods | tried and 100% | Stop. |

New lower stubs worth creating from `UnknownClose` direct calls:

| RVA | Current evidence | Possible name | Stub? | Why it belongs |
|---:|---|---|---|---|
| `0x085c50` | bare `FUN_00485c50`, called while freeing manager field `+0x68` | `CGruntzMgrField68::Close` or `DestroyField68Resource` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x085df0` | bare `FUN_00485df0`, called while freeing manager field `+0x60` | `CGruntzMgrField60::Close` or `DestroyField60Resource` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x085f40` | bare `FUN_00485f40`, called while freeing manager field `+0x5c` | `CGruntzMgrField5C::Close` or `DestroyField5CResource` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x09dc80` | bare `FUN_0049dc80`, called while freeing manager field `+0x78` | `CGruntzMgrField78::Close` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x0e2290` | bare `FUN_004e2290`, called while freeing manager field `+0x74` | `CGruntzMgrField74::Close` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x137900` | bare `FUN_00537900`, called from manager sound field `+0x48` | `CGruntzSoundZ::Shutdown` or `CGruntzSoundZ::StopAndFlush` | Yes | Direct child of manager shutdown; likely sound subsystem. |
| `0x14c9f0` | bare `FUN_0054c9f0`, called while freeing manager field `+0x50` | `CGruntzMgrField50::Close` | Yes | Direct child of manager shutdown, not yet tracked. |
| `0x17c8e0` | bare `FUN_0057c8e0`, called while freeing manager field `+0x40` | `CGruntzMgrField40::Close` | Yes | Direct child of manager shutdown, not yet tracked. |

Already-tried lower shutdown calls to skip:

- `0x085d10` `CGruntzMapMgr::~CGruntzMapMgr`
- `0x1158f0` `FreeFontsMemory`
- `0x154f50` `CDDrawWorkerRegistry::VirtualMethodUnknown2C`
- ambient/sound/status-bar stubs already listed in `match_baseline.tsv`

## Stub Creation Queue

Create stubs only for the new lower leaves, in this order:

1. `0x137900` `CGruntzSoundZ::Shutdown` / `StopAndFlush`
2. `0x085c50` `CGruntzMgrField68::Close`
3. `0x085df0` `CGruntzMgrField60::Close`
4. `0x085f40` `CGruntzMgrField5C::Close`
5. `0x09dc80` `CGruntzMgrField78::Close`
6. `0x0e2290` `CGruntzMgrField74::Close`
7. `0x14c9f0` `CGruntzMgrField50::Close`
8. `0x17c8e0` `CGruntzMgrField40::Close`

Defer these until the tree intentionally enters DirectPlay internals:

9. `0x178360` `DirectPlayCreateWrapper` / `CreateDirectPlaySession`
10. `0x178430` `DirectPlayEnumerateWrapper` / `EnumDirectPlayConnections`

Do not create new stubs for:

- `0x002e6e`, `0x002f59`, `0x002540`, `0x0024f5`, `0x004214`,
  `0x004282`, `0x0043c2`: these are thunks to already tracked functions.
- `_strstr`, `_sscanf`, Win32 imports, DPLAYX ordinals.
- any function already present in `config/match_baseline.tsv`.

## Next Data Needed

Before implementing the stub queue, resolve each proposed `CGruntzMgrFieldXX`
owner from RTTI/vtable stores or constructor stores. Until then, keep names
temporary and field-offset based so we do not bake wrong class names into
headers.
