# Match learnings — per-class reconstruction layout notes

> **Codegen idioms now live in `docs/patterns/`** (one file per MSVC5 /O2 idiom,
> indexed in [`patterns/INDEX.md`](patterns/INDEX.md)). The reusable "MSVC5 emits X
> when source is Y" lessons that used to accumulate here have graduated into that
> catalog — grep it (`grep cpp:switch patterns/INDEX.md`) when staring at a stuck diff.
>
> **This file holds only the per-class reconstruction LAYOUT notes**: member-offset
> maps for the big in-progress TUs (CPlay/CState, CGrunt, CNetMgr, the manager
> classes, the file/asset classes) and the "UNLOCKED → next target" pointers a
> continuing matcher needs. This is reconstruction DATA (offsets are load-bearing;
> names are placeholders), not idioms. Newest at top.

---

## CPlay / CState game-state hierarchy (units `cplay`, `gamemode`)

The `m_mode` polymorphic state RezMgr drives (RezMgr+0x2c). **CState** (RTTI
`.?AVCState@@`, vftable @0x5ea21c) derives from a WAP32 base @0xfa150; concrete
subclasses **CPlay** (vftable @0x5ea0bc), **CMenuState** (@0x5e9e84),
**CCreditsState** (@0x5e9c64), **CBootyState** (@0x5e9cec). Slot **+0x10 Update** is a
6-byte `mov eax,<id>; ret` STATE-ID query (CState=1, CPlay=3, CMenuState=5,
CCreditsState=8, CBootyState=0xa); slot **+0x14 Render** is the real per-frame
step+draw. `g_entityList @0x645574` (count@+4, elems@+8; each `e->vtbl[+0x10]` =
per-entity Update) is the per-frame entity set every Render iterates.

**CState LAYOUT (≥0x1a8 B):** ctor-written scalars +0x4/8/c/14/18/24/28/2c/38/3c=0,
+0x4c(byte)=0, +0x150/154/160/164/168/16c/178/17c/188..1a4=0,
**+0x170/174/180/184=0x40** (time/budget seeds). Render-path members: **+0x4**
owner/CGruntzMgr back-ptr (→+0x4→+0x4 = HWND), **+0xc** input/anim sub-object holder,
**+0x24** state discriminator (==5 → WM 0x8023 vs 0x8027), **+0x1b4** one-shot FX
latch, **+0x1c4** conditional-FX gate.

**CPlay LAYOUT (extends CState from +0x1a8):** inherited CState **+0x4** owner→CWorld
(`+0x54` world-draw, `+0x48` sound mgr, `+0x5c` 2nd layer, `+0x6c` frame-timer,
`+0x70` input, `+0x68`→`+0x230` substep gate, `+0xc` active-grunt sel), **+0xc** view
holder (`+0x4`→renderer-state {`+0x10`→`+0x2c` surface, `+0x14` view, `+0x18`
present-target}, `+0x8`/`+0xc` renderers, `+0x20` profiler timer, `+0x24`
draw-surface), **+0x150/+0x154** BeginFrameClear args. CPlay-own: **+0x1a8/+0x1ac**
StepInputA boot one-shot latches, **+0x1b0** half-selector, **+0x2dc** subsystem
(`+0x10c`, `+0x550/+0x554` ready-gate, `+0x574`), **+0x2e0/+0x2e4** marker sinks,
**+0x2f8** level-id (==0x66), **+0x30c** world-ready gate, **+0x310** RECT buffer,
**+0x320** overlay gate, **+0x328..+0x344** booty/ambient timers, **+0x348**
ambient-init latch, **+0x3f4** frame-marker obj, **+0x3f8..+0x408** ambient-cue
timer/toggle, **+0x40c** cue wParam / last-cueId latch, **+0x410** PlayCueAt per-cue
state obj (addr-taken), **+0x414** drew-flag, **+0x430..+0x46c** four scroll-region
timers, **+0x470/+0x474/+0x478/+0x47c** their gates, **+0x480** StepC view-mode
discriminator (0=idle/1=ModeA/2=ModeB), **+0x4a0..+0x4b0** snapshot timer+latch,
**+0x4e4** StepScroll scroll-offset SINK (writes +0x5c X / +0x60 Y), **+0x4ec** hard
early-out, **+0x4f4** win/lose banner, **+0x4f8** PRIMARY mode (nonzero=MAIN frame),
**+0x4fc** overlay-active, **+0x500** paused, **+0x510** countdown. CState-region (in
the OnRegion handlers): **+0x160/+0x164** (per-half value), **+0x168/+0x178** (per-half
ptr blocks, addr passed), **+0x188/+0x198** (per-half {x,y} edge feed). CDrawSurface
**+0x10/+0x14** scroll origin, **+0x5c→+0x40 .{x,y}** geom feed.

**CGameRegistry (@0x64556c) LAYOUT:** **+0x14** dev/has-window, **+0x30**→{`+0x8` map,
`+0x24` rect}, **+0x60** cue sink (the five-argument
`CGruntSpawnConfig::SpawnVoiceDriver` overload @0x11b7c0 via thunk 0x33b4,
ret 0x14),
**+0x68** OnRegion0 message-post sink, **+0x8c/+0x90** viewport, **+0x134** 1/2 mode
(==1 gate), **+0x13c/+0x140/+0x144/+0x148** view rect (minX/minY/maxX/maxY), **+0x15c**
level tree.

**CPlay vtable @0x5ea0bc (extends CState):** **+0x7c** BeginFrameClear (3 args), **+0x9c**
RenderSlow, **+0xa0** RenderFast (the UNSIGNED frame-rate gate selects between them); +
renderer vtable slots **+0x24** begin-scene / **+0x34** present (on `m_c->m_8`/`m_c->m_c`).

**Status:** CPlay::Render @0xc8cf0 (3092 B) is a faithful CARCASS (the per-frame heart;
3-way SEH dispatch, kept `wip` — see patterns/big-seh-fuzzy-desync.md). The four
OnRegion scroll one-shots @0xd8aa0/0xd8a00/0xd8b20/0xd8bc0 are BYTE-EXACT; StepC@0xd8d90,
StepScroll@0xd1ac0, StepInputA@0xd11e0, PlayCueAt@0xd1890 are faithful carcasses
(78-90%). GameMode unit 8/8 byte-exact (CState ctor/dtor + the five Update stubs).
**NEXT:** StepWorldB @0xd12b0 (725 B, world/camera step); below it the per-entity
update layer walked inside the world-draw blit (`m_4->m_54->Blit` @0x1a7d).

---

## CGruntzMgr / RezMgr manager (unit `rezmgr`)

The game manager (CGruntzMgr/RezMgr, leaked `C:\Proj\Gruntz`). InitializeGameManager
@0x80a20 = `operator new(0xa30)` + ctor @0x83030. The per-frame tick
`RezMgr::PerFrameTick @0x8b740` (vtable slot +0x10) is BYTE-EXACT.

**Manager LAYOUT (0xa30 B):** vftable @0 (VA 0x5e9b64); **+0x2c** m_mode (active
`CGameMode*`/CState; ctor zeroes it, a SetGameMode caller sets it); **+0xb0**
m_renderGate (render-suppress this frame); embedded objects @+0xc8 (CString),
@+0xd8 (0x14-byte class, vtbl 0x5ec2dc), **+0xec/+0xf0** CString m_pathA/m_pathB,
@+0x150 (0x238-byte sub-object); scalars +0x10=+0x14=1, +0x88=0x10, +0xb8=1, +0xcc=0x1e,
**+0xf4=1** (m_inGameDir), **+0xf8=0** (m_haveRez), **+0xfc=0** (m_haveMoviez),
+0x100=+0x104=+0x10c=+0x110=1, +0x138=3, the rest 0.

**Manager vtable @0x5e9b64 (incremental-link thunked):** +0x00 dtor (→0x83330), +0x04
(→0x83450), +0x08 (→0x855e0), +0x0c (→0x83300), **+0x10 PerFrameTick ✓MATCHED**, +0x14
(LAB_0x31e3), +0x2c FUN_0x13dd50, +0x30 FUN_0x13ddb0, +0x34 (→0x85560), **+0x38
UpdateClock @0x13ddc0** (also called directly by the tick), +0x3c (→0x85580), +0x50
(→0x85480), +0x54 (→0x82430), +0x58 (→0x9f840), +0x5c (→0x9f9a0). Wide (≥0x60).

**Frame-clock globals (file-scope, reloc-masked):** `0x653c70 g_now` / `0x653c74
g_frameDelta` (written by UpdateClock @0x13ddc0, also vtbl +0x38); `0x645580 g_lastNow` /
`0x645584 g_lastDelta` (clamped ≤0x64) / `0x645588 g_accumMs` / `0x64558c g_frameTicks` /
`0x645590..0x6455a0` five interval countdown timers (seeds 0x32/0x64/0xc8/0x190/0x1f4 ms).
Low-detail/front-end-class gate global `@0x6455d4`.

**RezMgr path/key builders:** MakeImageKey @0x13e5d0 BYTE-EXACT (ext dispatcher: strrchr
+ stricmp ladder `.BMP`@0x61a0e4/`.PCX`@0x61a0dc/`.PID`@0x61a0d4 → loaders LoadBmp@0x144110
/LoadPcx@0x145110/LoadPid@0x145cd0). MakeRezPath @0x91670 PLATEAU 91.87% (EH/CString
entropy): builds m_pathA "Gruntz.REZ" / m_pathB FEC, `GetGruntzDriveLetter`@0x8fa70,
`ReportError`@0x8dc60. **Manager ctor @0x83030 DEFERRED** (store-order-entropy carcass).

**RezMgr container node classes (THREE distinct layouts, do NOT merge):**
- `CRezItmBase` (16 B, ctor @0x13c4e0): vtbl@+0 (0x5ef768), parent@+0xc.
- `CRezItm : CRezItmBase` (0x24 B, ctor @0x13c540): vtbl@+0 (0x5ef788), +0x10=0, +0x14=0,
  +0x20=-1. **BYTE-EXACT 99.23%.**
- `CRezDir : CRezItmBase` (0x38 B, ctor @0x13c940): vtbl@+0 (0x5ef7a8), embedded
  child-collection two-vtbl @+0x10/+0x1c (0x5ef7c8), head@+0x14/tail@+0x18=0,
  +0x20/+0x24/+0x28/+0x34=0, +0x2c=RezMgr back-ptr (arg2), +0x30=1. ctor PLATEAU 78.45%
  (store-schedule entropy; do NOT model the collection as a member-with-ctor — that emitted
  an out-of-line ctor + EH frame → 15%).
- **Load's node** (@0x13a0f0, BYTE-EXACT 99.57%): +0x10 payload SIZE, +0x18 archive-source
  ptr, +0x48 loaded-buffer/already-loaded gate, +0x38 child collection. **OpenSub's node**
  (@0x13b0c0, DEFERRED): +0x1c child COUNT, +0x10 list-append, +0x40 gate, +0x64 name-buf,
  +0x54..+0x60 max-dims.
- `FindEntry` @0x13c080 BYTE-EXACT 99.74% (a filesystem STAT, not a binary search; tests
  the directory bit 0x4000 of the WIN32-find attribute dword; `this` never read).

---

## CGrunt entity (unit `grunt`)

**Animation resolvers** (5/5 logic+CFG+offsets byte-exact; Death 97.1%, Generic 94.7%,
Moving 92.0%, Idle 91.4%, Battlecry 89.6%): the `Resolve*Animation` __thiscall methods
@0x045100 Moving / @0x0455f0 Death / @0x0457b0 generic "_JOY" / @0x045960 Idle / @0x045b60
Battlecry build `"GRUNTZ_" + m_typeName + "_<CAT>"` and `CButeTree::Find` @0x16d190 on the
global anim tree @0x6bf620.

**CGrunt LAYOUT (resolvers + HUD creators):** **+0x10** HUD geometry source (→+0x5c/+0x60),
**+0x14** anim-set lookup holder (`->m_1c`=resolved node), **+0x30** old m_14->m_1c (saved
before Find), **+0x38** animation player (`SetAnim`@0x150540 ret4 / `SetAnimEx`@0x1504d0
ret8; `SetGeometry`@0x15c2d0 ret4 on sub-player at player+0x1a0; player+0x1b4=active-anim
descriptor, cached into +0x40 before geometry call), **+0x40** cached anim descriptor,
**+0x54** CString m_typeName, **+0x58[]** Idle geo sources / **+0x68[]** Battlecry geo / **+0x74**
generic geo / **+0x78** death geo / **+0x7c** moving geo, **+0x88/+0x8c/+0x90/+0x94**
Moving-only time/seed block (`m_90=(rand()%0x5dc1+0x1770)*10`, `m_88=g@0x645588`), **+0xa8**
resolve gate/dirty flag (Death latches =1, Moving does NOT), **+0xac** death-cue arg2.
Sprite-pointer slots: selected **+0x1b8**, toy **+0x1bc**, health **+0x1c4**, stamina
**+0x1c8**, toyTime **+0x1cc**, wingzTime **+0x1d0**, powerup **+0x1d4**; Add* args
**+0x1ec/+0x1f0**; stat/gate words **+0x238** (wingz), **+0x3ec** (health), **+0x3f0**
(stamina), **+0x3f4** (toyTime), **+0x3f8** (wingzTime). CGruntHud **+0x188** cue arg.

Category strings `_MOVING`@0x60d220/`_DEATH`@0x60d22c/`_JOY`@0x60d234/`_IDLE`@0x60d36c/
`_BATTLECRY`@0x60d374; prefix `GRUNTZ_`@0x60d28c; Find keys "B"/"C"/"E"/"A"/"F".

**HUD sprite-creator cluster** (7/7 logic byte-exact; @0x04d130..0x04d730
CreateHealth/Toy/Stamina/ToyTime/WingzTime/Powerup/Selected): each calls the global HUD
sprite FACTORY @0x1597b0 (`__thiscall ret 0x18`, via `(*0x64556c)->m_30->m_8`) with the
sprite class-NAME string, registers via `sprite->m_7c->m_18` Add* (@0x07f0d0 3-arg /
@0x07f920 2-arg / @0x080380 3-arg Powerup / @0x07e9c0 2-arg). Per-sprite m_60 subtractions:
Health/Toy −0x19, Stamina/ToyTime −0x20, WingzTime −0x26, Powerup/Selected −0.

---

## CNetMgr DirectPlay manager (unit `netmgr`)

4/4 byte-exact (OnMultiOptions@0x0badd0, OnMultiPause@0x0bad40, OnOutOfSync@0x0bae40,
ApplyCmdDelayDefaults@0x0b85a0). The higher-level state/message methods (no DirectPlay COM).

**CNetMgr LAYOUT:** **+0x4** m_4 (→ HWND via +0x4→+0x4→+0x4), **+0x1c** WM_COMMAND wParam
posted on resync, **+0x574** OnOutOfSync per-instance reentrancy guard, **+0x584** state
word cleared on handler entry, **+0x598** CString config value-name prefix, **+0x5a4/+0x5a8**
_CmdDelay/_Resend command-timing values. Globals: `0x648d08` (OnMultiOptions guard),
`0x648d04` (OnMultiPause guard), `0x648ce0` (shared flag cleared by all three). The
game-manager singleton @0x64556c holds a `RegistryHelper*` @+0x38. Multiplayer dispatcher
@0x4bc250 (`__stdcall`); command callbacks @0x4bda70/0x4bd850/0x4bddd0; HandleVersionCheck
@0xbd0b0, OnDropPlayer @0xbc110 byte-exact.

---

## Wap32 app/window classes (units `gameapp`, `gruntzapp`, `gamewnd`, `winmain`)

The WinMain→app→window→manager lifecycle is matched end to end.

**CGameApp LAYOUT (Wap32.h):** m_4/m_8 = `CGameResource*` (polymorphic, vtable slot0 =
scalar-deleting dtor; m_4 also exposes HWND@+0x4 + guard@+0xc; **m_8@+0x8 = the game
MANAGER**, freed by CloseResources, set by VirtualUnknownMethod02), m_c=HINSTANCE,
m_10=HACCEL, m_18 flag byte (bit1=system arrow cursor), **m_240/m_244** = run/active latch
pair (BOTH set ⇒ idle ticks), m_244/m_248(one-shot guard)/m_24c/m_250 error fields.
Embeds `GameInfo m_gameInfo@+0x14` (0x1d4 B: size@+0=0x1d4, windowClassFlags@+4 [an **int**,
not char], hInstance@+8, szCmdLine[0x80]@+0xc, szGameIdentifier[0x40]@+0x8c,
szWindowName[0x40]@+0xcc, szWindowClassName[0x80]@+0x14c, windowWidth@+0x1cc,
windowHeight@+0x1d0), `WNDCLASSA m_wc@+0x1e8`, `CREATESTRUCTA m_createStruct@+0x210`.

**CGameApp vtable (16 slots, tomalla order):** +0x8 Init (VirtualUnknownMethod03), +0x18
RunMessageLoop, +0x20 idle (VirtualUnknownMethod09, tail-calls `m_8->vtbl[+0x10]` =
PerFrameTick), +0x24 FreeGameManager, +0x2c InitializeAccelerators, +0x30 ShowError, +0x34
InitializeGameWindow, +0x38 InitializeGameManager, +0x3c InitializeDefaultWindowClass, +0x40
InitializeDefaultCreateStruct. CGruntzApp vtable @0x5e9ab4 (slots 0 dtor / +0x8 Init / +0x18
Run / +0x30 ShowError). Active-window singleton = file-scope `static CGameWnd*` @0x653c68
(distinct from the CGameApp instance counter @0x653c6c). Error buffer `g_errorText`@0x644ea0
(char[0x100]); ErrorDialogProc's HWND store @0x64557c; g_registryHelper instance @0x6295d8;
g_hInstance @0x651618; g_pApp @0x651600; FileVersion ints @0x651608..0x651614.

**CGameWnd LAYOUT:** HWND m_4@+0x04, owner m_8@+0x08, guard m_c@+0x0c. GameWindowProc
@0x13cff0 (860 B) and GameWindowProc dispatch tables @0x53d34c/0x53d360/0x53d374 SKIPPED.

---

## File / asset classes

**CFileIO** (unit `filestream`, 9/10 byte-exact; the MFC CFile work-alike, built **/O1**):
16 B, `: public CObject` — vtable@+0, **HANDLE m_handle@+0x04** (-1=closed), **int
m_open@+0x08**, **CString m_name@+0x0c**. Ctor @0x1befd7, dtor/Close @0x1bf121, Open @0x1bf200
(ret 0xc, nOpenFlags→Win32 translator), Read @0x1bf328 (ret 8), GetLength @0x1bf505,
adopt-handle ctor @0x1bf033, scalar-deleting dtor @0x1bf017. Two-phase vtable (base
0x5e8cb4, derived 0x5ed15c).

**Image classes (unit `image`, 4/4 byte-exact):** TWO distinct classes — `CImage` (owns
`LoadFromRez`@0x175a90, an ext dispatcher: `.BMP`/`.PCX`/`.RID`@0x624278/`.PID` →
siblings LoadBmp@0x175e40/LoadPcx@0x176190/LoadRid@0x176310/LoadPid@0x1766a0/
LoadDefault@0x1767d0, all ret 0xc) vs `CFileImage` (LoadBmp@0x144110/LoadPcx@0x145110
identical; LoadPid@0x145cd0 omits the len==0 guard + 4th decoder arg). Decoders (deferred):
DecodeBmp@0x143fc0/DecodePcx@0x144ee0/DecodePid@0x145b10. `.RID`@0x624278 = a 4th image ext.

**WWD (unit `wwdfile`):** header validators IsValidWwd@0x160530 / CheckHeader@0x160660
byte-exact (validate the 0x5F4 header; first u32 == sizeof ≤ 0x5F4). ReadPlane@0x15d8d0
99.19% (CPlane is 0x158 B). `WwdHeader 0x5F4`, `WwdPlaneHeader 0xA0`, `WwdObjectRecord 0x11C`,
`PidHeader 0x20` (src/Stub/types/). **CGameLevel::LoadWwd @0x15d280 CARCASS ~56%** (unit
`gamelevel`; a this→ebp / hdr→ebx reg-alloc divergence the source can't steer; vtable slot
0x38, pre-load Reset slot 0x44; m_header@+0xE0, m_flags@+8, m_checksum@+0xAC, plane CArray
@+0x34 [m_data@+0x38/m_size@+0x3c=count], image-set CArray @+0x48, main plane idx @+0x60).
`CArray::SetAtGrow @0x5b5822` (m_data@+0x4, m_size@+0x8). **NEXT:** ReadPlaneObjects @0x162af0
(2054 B). zlib DONE (51 fns byte-exact); `InflateMainBlock`@0x160790 plateaus ~88.7%.

**CMapMgr** (unit `mapmgr`, 7/7 byte-exact; vftable `??_7CMapMgr@@6B@` @0x5ea3b4): ≥0x60 B —
vftable@+0 (6 slots: slot0 Reset @0x09ec30, slots 1-5 @0x09f7f0/0x09f840/0x09f9a0/0x09eca0/
0x4853f0), m_4@+4 / m_8@+8 (heap ptrs Reset frees), +0xc/+0x10/+0x18/+0x1c=0, embedded
array @+0x30 (CMapArrayA: block@+0x04, stride 0x24, next@+0x14/prev@+0x18) and @+0x3c
(CMapArrayB: block@+0x00, stride 0x0c, data@+0/prev@+4/next@+8), +0x4c=0, **+0x50=-1**,
+0x58=0, **+0x5c=1**. Allocate@CMapArrayA @0x09e740 / @CMapArrayB @0x09e860 DEFERRED
(strength-reduction reg-alloc divergence ~64-70%).

---

## Config subsystem (units `registryhelper`, `advancedoptions`, `winapi`)

**Utils::RegistryHelper** (8/8 byte-exact): **+0x00** open/result gate, **+0x04** base HKEY
(Open), **+0x08/+0x0c/+0x10/+0x14** nested HKEY chain, **+0x18** deepest open HKEY (getters
operate on it; Close skips +0x14 when == +0x18), **+0x1c** char[0x100] (szKeyName2), **+0x11c**
char[0x100] (szLastKey); min ≥0x21c. Open @0x139210, Close @0x139330, InitializeLastKey
@0x139370, Set/GetValueString/Dword @0x1393b0/0x139460/0x1394a0/0x1395d0, GetRegistryKey
@0x139650 (__thiscall ret 0xc, ignores `this`). `0x1396f0/0x139710/0x1397a0` are a DIFFERENT
class (vtable@+0x1c). No GetValueBool exists.

**AdvancedOptions** (5/5 byte-exact): persists 5 flags under `HKLM\Software\Monolith
Productions\Gruntz\1.0`. Control-IDs IDC_DISABLE_VIDEO=0x46c..MOVIE=0x470, IDC_DEFAULTS=0x426,
IDOK=1/IDCANCEL=2; reg value names = the literal labels.

**Utils::WinAPI** (4/4 to byte-exact): FileExists @0x1189c0 (and 0x1fd70, byte-identical) =
`OpenFile(path,&of,OF_EXIST)!=-1`, returns int; ActiveWait @0x13dfe0 (timeGetTime busy-spin);
IsGruntzCDInAnyDrive @0x1fd50; GetGruntzDriveLetter @0x1ffe0 (97.9% reg-alloc plateau;
memoised static @0x62b25c; reads `HKLM\…\1.0` "CdRom Drive", marker `<L>:\GAME\GRUNTZ.EXE`,
sprintf @0x11f890). LegacyAreProcessesRunning @0x118ce0 LEFT OUT (501 B TOOLHELP32).

---

## Subsystem entry points still open (anchored callees)

The per-frame/game-logic frontier below CPlay::Render: StepWorldB @0xd12b0, the per-entity
update layer (`g_entityList @0x645574`, `e->vtbl[+0x10]`), the world-draw helpers (PushView
@0x15dc90, surface-flush @0x13e850, HUD-draw @0x163f40), the sound trio (PlaySound @0x138840 /
FindSound @0x138730 / StopSound @0x139030), the marker/guts pair @0x1170b0/@0xfe6b0. The
CGrunt animation player (SetAnim @0x150540 / SetAnimEx @0x1504d0 / SetGeometry @0x15c2d0) and
the on-screen cue sink @0x11b7c0 are anchored callees for the rest of the (huge) CGrunt TU.
The image-format decoders, WwdFile::ReadPlaneObjects, the RezMgr OpenSub walk, and the CMapMgr
Allocate free-list builders are the deferred entropy-class targets.
