# What the shipped game data says

Every string of reconstruction value in `Gruntz.REZ` / `GRUNTZ.VRZ`, and — as
importantly — the resource classes that were swept and found to contain
**nothing**, so the next reader does not repeat the sweep.

Companion to [`rez-v1.md`](rez-v1.md) (the container), [`wwd-v1.md`](wwd-v1.md)
and [`ani-v1.md`](ani-v1.md) (the record layouts). Those documents say what the
bytes *are*; this one says what the bytes *name*, and cross-checks every
identifier against `src/` + `include/` with a verdict.

Corpus: retail `Gruntz.REZ` (21 303 resources) and `GRUNTZ.VRZ` (1 517), plus
`GRUNTDEM.REZ` (10 553) and `GRUNTDEM.VRZ` (308) as the control. A string in one
archive and not the other is itself evidence, and it produced two of the
findings below.

## Reproducing it

```sh
cd tools && cargo build --release
REZ=/path/to/Gruntz.REZ
./target/release/rezls    "$REZ" grep '' > names.txt      # the whole name table
./target/release/rezpack  unpack "$REZ" ../build/rez-tree # every payload
./target/release/butez    "$REZ" dump ../build/rez-txt    # + Blowfish decrypt
./target/release/butez    "$REZ" cheatz                   # the [CheatN] table

# the two sieves this document is built on
the WWD object walker (retired) ../build/rez-tree --tsv objects.tsv
the asset-key harvester (retired)  --names names.txt --src .
the asset-key harvester (retired)  --names names.txt \
        --keys <(cut -f22,23 objects.tsv | tail -n +2 | tr '\t' '\n' | sort -u)
```

`wwd_objects` walks the object records rather than regexing the inflated block,
so each string is bound to its level, plane, object index and **field**.
`asset_keys` resolves registry keys — from `src/` literals or from a WWD column
— against the archive's name table.

---

## 1. Resource-class verdicts

| Class | Count | Verdict |
|---|---|---|
| **TXT** | 4 | **Rich.** Credits (incl. a source listing), the attribute/cheat database, a version stamp. [§2](#2-the-credits-file), [§5](#5-the-cheat-table) |
| **BAT** | 1 | **Rich.** An art-pipeline script; Monolith's own asset-source directory layout. [§4](#4-the-bat-an-art-pipeline-script) |
| **WWD** | 54 | **Rich.** Level/author/date headers plus 27 110 object records with four strings each. [§3](#3-the-wwd-object-corpus) |
| **ANI** | 660 | **Moderate.** 281 sound-cue tokens (all resolve) and 32 stale on-disk names. [§6](#6-ani-cues-and-the-stale-name-field) |
| **VRZ WAV** | 1 517 | **Moderate.** A `VOICES\<CATEGORY>\<LINE>` taxonomy naming 21 game events and 38 grunt types. [§7](#7-the-voice-archive) |
| **PID** | 19 953 | **NOTHING.** All 19 953 scanned. Every printable run is pixel or palette data; the two runs that occur 12 120 times each are bytes of the shared grunt palette. No header, trailer, or tool stamp. |
| **PCX** | 35 | **NOTHING.** All 35 headers are `manufacturer=10 version=5 encoding=1 bpp=8`, 3 planes (34) or 1 (1); the 54-byte reserved field is zero in every file. No creator stamp. |
| **PAL** | 36 | **NOTHING.** Raw colour tables. |
| **WAV** | 523 | **NOTHING.** 521 are `fmt`+`data`, 2 are `fmt`+`fact`+`data`. **No `LIST`/`INFO` chunk in any file** — no artist, software or comment. (513 are 22 050 Hz 8-bit mono.) |
| **XMI** | 37 | **NOTHING.** Only the Miles IFF chunk ids `XDIR`/`INFO`/`XMID`/`TIMB`/`FORM`. No title or composer field. |

The four media classes are **ruled out**, exhaustively rather than by sampling:
the scan read all 20 548 files.

---

## 2. The credits file

`STATEZ\CREDITZ\CREDITZ`, 9 639 bytes, plaintext (not Blowfish). 190 lines of
real credits, then a long joke section, and it is the joke section that carries
the evidence.

### 2a. Retail source-file names — TU-partition evidence

The tail of the file is a paste of a real multiplayer desync log. Two lines are
**source paths with line numbers**, and four more file names appear in the joke
list:

| Name | Where | Cross-check |
|---|---|---|
| `C:\Proj\Gruntz\Grunt_State.cpp`, line **922** | desync log, 12 occurrences | **new** — not in the EXE's string table |
| `C:\Proj\Gruntz\Grunt_Combat.cpp`, line **411** | desync log | **new** |
| `booty.cpp` | joke list | **new** |
| `compconai.cpp` | joke list | **new** — "computer/companion AI"? |
| `statusbar.cpp` | joke list | **new** |
| `nakedchix.cpp` | joke list | **new** (a joke, but it is a filename-shaped joke) |

`GRUNTZ.EXE`'s own string table leaks only **nine** `.cpp` paths, exactly one of
them under `C:\Proj\Gruntz\` (`GruntzMgr.cpp`). These six are additional members
of that directory and none of them appears anywhere in `docs/` today. Our tree
splits the same code across many small units — `GruntCombat.cpp`,
`GruntStateStep.cpp` / `GruntStateRec.cpp`, `StatusBarMgr.cpp` and siblings,
`BootyStateActivate.cpp` and siblings — so the credits are naming the *original*
compilands those units were carved out of. Recorded here as evidence for
[`../tu-partition-brief.md`](../tu-partition-brief.md); this document does not
claim a mapping.

### 2b. Two function names

`MoveGruntAroundObstacle()` and `ActuallyRemoveGrunt()`. Neither string appears
in `src/`, `include/` or `config/`. Both are plausible retail identifiers for
functions we have reconstructed under invented names; binding them is open work.

### 2c. The `GetRandomNumber` listing

```c
CODE:
int GetRandomNumber()
{
   static long holdrand = timeGetTime();
   return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

OUTPUT:
10,10,10,10,10,10,10,10,...
```

Monolith printing their own bug: the function-local static initialises **once**,
so with the MSVC LCG constants the sequence is fine — but the joke is the
`OUTPUT` line, i.e. in the multiplayer build every client re-seeded and
converged. Relevant to us twice over: the constants `214013`/`2531011` are the
CRT LCG, and the `static long x = <call>` shape is the function-local-static
guard signature the campaign already tracks.

### 2d. The desync log — retail's own field names

```
[crc] [time=182622] [p=0] [g=0] [health=95] [x=2064] [y=880] [dir=7] [stm=100]
[ttl=0] [tool=15] [toy=0] [sel=1] [lck=0] [iic=1] [qat=0] [qax=0] [ia=0]
[iad=1] [iid=0] [ait=0] [locked=0] [rnd=22341]
```

and the per-object transition line

```
C:\Proj\Gruntz\Grunt_State.cpp, 922
[1,9] [s=DEATH] [gp=2128,848] [tg=0] [r=4628]
```

Twenty-two abbreviated field names for the per-grunt CRC record, plus five state
names (`IDLE`, `ATTACK`, `ATTACKIDLE`, `MOVINGTOY`, `DEATH`) and the
`[player,grunt]` pair convention. **None of these format strings survive in
retail** — grep of the EXE finds no `[crc]`, no `Grunt_State.cpp` — so they are
`#ifdef`-ed debug output, and the credits file is the only place they exist.
`src/Gruntz/BuildGruntzCrcInfo.cpp` is the reconstructed descendant.

### 2e. Joke-list items that are game entities

`ButeMgr` (confirms the class name), `MPKELLY` and `MPWARPSTONEZ` (cheat codes —
[§5](#5-the-cheat-table)), `wa-wa-wa-wa-wa-wa` (the `MPWAWAWAWAWAWA` cheat,
[§5](#5-the-cheat-table)), "Ring-based radii", "The token ring combat system",
"The warpstone win condition", "Gruntzketball", "Difficulty adjustmentz",
"Auto attacking" / "Auto defending", "The level timer and the stopwatchez".

### 2f. Names

Full team list, 60+ people. Load-bearing for us: **Kevin Lambert** (lead
programmer, game design), **Brian L. Goble** ("Game Engine and Toolz" — the WAP32
/ `zlith` substrate), Toby Gladwell, Brian Long, Scott H. Pultz, Jeremy Blackman.
Both Lambert and Goble have cheat codes named after them ([§5](#5-the-cheat-table)),
and Goble's `"Brian L. Goble is a programming God..."` is a live string in the EXE.

---

## 3. The WWD object corpus

`the WWD object walker (retired) walks all 54 files: **27 110 object
records**, every one landing exactly on the next record boundary. Each record's
four packed strings are `name` / `logic` / `image_set` / `animation`.

### 3a. `logic` — 34 distinct, every one a registered worker

| count | logic | | count | logic | | count | logic |
|---:|---|---|---:|---|---|---:|---|
| 6537 | TileTrigger | | 764 | GuardPoint | | 90 | CheckpointTrigger |
| 5697 | EyeCandy | | 682 | TileSecretTrigger | | 84 | GruntPuddle |
| 3403 | BehindCandyAni | | 579 | FortressFlag | | 83 | Teleporter |
| 1792 | Brickz | | 561 | GruntStartingPoint | | 77 | KitchenSlime |
| 1241 | InGameIcon | | 469 | RollingBall | | 46 | GiantRock |
| 1195 | TileTriggerSwitch | | 400 | WayPoint | | 34 | UFO |
| 942 | StaticHazard | | 322 | GruntCreationPoint | | 32 | SecretTeleporterTrigger |
| 904 | CoveredPowerup | | 236 | DoNothing | | 32 | SecretLevelTrigger |
| | | | 212 | **GlobalAmbientSound** | | 32 | RainCloud |
| | | | 212 | BehindCandy | | 29 | ToobSpikez |
| | | | 148 | InGameText | | 28 | ObjectDropper |
| | | | 126 | VoiceTrigger | | 8 | EyeCandyAni |
| | | | 105 | ExitTrigger | | 8 | SpotLight |

Three-way cross-check against `RegisterGameObjectLogicTypes` @0x0000a3b0 (73 dispatchers)
and `LogicTypeId` (67 enumerators):

* **All 34 WWD logic names have registered dispatchers.** No missing entity.
* **39 registered dispatchers are never used by a shipped WWD** — their objects are created
  at runtime (`Grunt`, `Projectile`, every `Grunt*Sprite`, `Explosion`, …).
* **Seven registered dispatchers have no `LogicTypeId`**: the four sound dispatchers
  (below), `GruntVoice`, `DemoMover`, `DemoSign`.
* **One `LogicTypeId` has no registered dispatcher**: `LOGIC_SINGLEFRAMEMESSAGE` (0x3eb).
  `.?AVCSingleFrameMessage@@` is in retail's RTTI, so the class exists and is
  instantiated without a logic-type registration.
* Two `LogicTypeId` values in the 0x3e8..0x42c band are unattributed: **0x3f9**
  and **0x40e**. The ids are *not* assigned in registration order (checked), so
  registration position does not name them.

### 3b. `GlobalAmbientSound` — VERDICT: not a missing class, and not a misnaming

An earlier regex census flagged `GlobalAmbientSound` (218 hits by regex, **212**
by a real record walk) as the one logic name with neither a `C<name>` class nor a
`LOGIC_<NAME>` enumerator. It is neither missing nor misnamed:

```cpp
// src/Gruntz/GameObjectLogicTypes.cpp, RVA 0x0000a3b0
ctx->m_logicRegistry->RegisterLogicType(DispatchGlobalAmbientSoundLogic, "GlobalAmbientSound", 4);
ctx->m_logicRegistry->RegisterLogicType(DispatchAmbientSoundLogic,       "AmbientSound",       1);
ctx->m_logicRegistry->RegisterLogicType(DispatchAmbientPosSoundLogic,    "AmbientPosSound",    0);
ctx->m_logicRegistry->RegisterLogicType(DispatchSpotAmbientSoundLogic,   "SpotAmbientSound",   0);
```

The WWD `logic` field names a **registered dispatcher**, not a class, and the four sound
keys use four dispatch entry points over two classes. `DispatchGlobalAmbientSoundLogic`
@0x0000c810 is a dispatch alias:

```cpp
i32 DispatchGlobalAmbientSoundLogic(CGameObject* obj) {
    g_posSoundReq = 1;
    return DispatchAmbientSoundLogic(obj);
}
```

and `DispatchAmbientSoundLogic` @0x0000c840 distinguishes the two by comparing the
record's own dispatch pointer, setting object flag `0x2` for the global variant:

```cpp
if (aux->m_notify == DispatchGlobalAmbientSoundLogic) { obj->m_flags |= 2; }
else                                           { obj->m_flags &= ~2; }
```

Both build a **`CAmbientSound`**; `AmbientPosSound`/`SpotAmbientSound` build a
`CAmbientPosSound`. They carry no `LogicTypeId` because that enum is the
serialisation id of `CUserLogic` subclasses and `CAmbientSound : CUserBase` is
not one. All four factories are already reconstructed at 100 % in unit
`worldsoundset`. **No class is missing; nothing is misnamed; no action.**

The premise that failed was "one logic name ⇒ one `C<name>` class". It holds for
32 of 34 and is simply not the engine's rule.

Corpus profile of the 212 records, which pins one more field: `object_type`,
`hit_type`, `flags_dynamic`, `flags_user`, `score`, `points`, `powerup`,
`smarts`, `health`, `z` are **0 in all 212**; the only varying user slot is
`damage`, over `{0, 40, 45, 50, 60, 65, 70, 75, 80, 85, 90, 100}`. It reaches
`CAmbientSound::m_volumeScale` via
`CreateAmbientFromSound(sound, 0x64, &rc, obj->m_damage, 0)`, and both
`SetLevel` @0xc200 and `Recompute` @0xbf10 apply it as
`if (m_volumeScale > 0) v = (v * m_volumeScale) / 100`. So for a `GlobalAmbientSound`
object the `damage` slot **is a volume percentage** — another arm of the
`+0x114` union documented in [`../domain/README.md`](../domain/README.md).

### 3c. `image_set` and `animation` — 30 266 of 30 267 references resolve

The engine installs REZ subtrees into named registries with `_` as the
separator: `InstallTree(GAME\IMAGEZ, "GAME", "_")` in
`CState::LoadGameAssetNamespaces` @0xf9ea0, and
`LoadFromTree(AREA<n>\{IMAGEZ,SOUNDZ,ANIZ}, "LEVEL", "_")` in `PlayAssetLoad.cpp`.
So `GAME_WAPWORLDONLY_TRIGGER` **is** `GAME\IMAGEZ\WAPWORLDONLY\TRIGGER`, and
every key is checkable against the archive.

| field | references | resolve | registry |
|---|---:|---:|---|
| `image_set` | 27 110 | **27 110** | `<NS>\IMAGEZ` |
| `animation` | 3 157 | 3 156 | 2 945 `<NS>\ANIZ`, 211 `<NS>\SOUNDZ` |

`animation` resolving in **two** registries is not sloppiness: retail calls both
`SetAnimationByName(s, 0)` and `SetSoundCueByName(s)` on it, and the
`GlobalAmbientSound` objects use it for a `<NS>\SOUNDZ\AMBIENT\<X>` WAV while
water/candy objects use it for an ANI. `wwd-v1.md` calls the field "sound"; the
third-party spec calls it "animation". Both are half right — it is a
**registry key that may name either**.

**The one dangling reference in the whole corpus:**

```
AREA6\WORLDZ\LEVEL24  ("Gruntz - Level Set 21")  object #192, id 14631, at (12,49)
  logic=GlobalAmbientSound  damage(volume)=50  animation=LEVEL_AMBIENT_AREA6LOOP
```

The resource is `AREA6\SOUNDZ\AMBIENT\`**`AMBIENT6LOOP`**. A designer typed
`AREA6LOOP`. That level's background ambient loop never plays in retail. It is
the *only* unresolved asset key across 30 267 references.

### 3d. `name` — 14 objects, and the field is real

The fourth string has never been looked at. It is present on exactly **14 of
27 110** objects, all in `AREA2\WORLDZ\LEVEL7`, all `TileTrigger`, all reading
`WEENIE_SWITCH` — level-designer jargon ("weenie" = a landmark that draws the
player), i.e. an editor annotation.

It is nonetheless a live field, not padding: `ReadPlaneObjects` @0x162af0 stores
it into the object at **+0xdc** (`lea ecx,[ebx+0xdc]; call <CString::operator=>`
in the target disassembly), which is `CWwdSpriteObject::m_name`. The only reader
in the tree is a release-dead `TRACE` in `CDDrawChildGroup::DeserializeObjects`
@0x15b0e0. So: **read, stored, and consumed only by debug output**.

### 3e. The level table

`author` is `"Monolith Productions Inc."` and `rezFile` is
`C:\PROJ\GRUNTZ\GRUNTZ.REZ` in all 54. `launchApp` is not uniform, and that is
where the two interesting rows are:

| file | header `levelName` | `created` | `tileDirectory` | `launchApp` |
|---|---|---|---|---|
| `AREA1\WORLDZ\LEVEL1..4` | Gruntz - Level Set 1 | Nov 10 1998 | `\AREA1\TILEZ` | `C:\PROJ\GRUNTZ\GRUNTZ.EXE` (LEVEL4: **`…\DEBUG\GRUNTZ.EXE`**) |
| `AREA1\WORLDZ\TRAINING1..4` | Gruntz - Level Set 1 | Oct 23–28 1998 | `\AREA1\TILEZ` | `C:\PROJ\GRUNTZ\GRUNTZ.EXE` |
| **`AREA1\WORLDZ\LEVEL101`** | Gruntz - Level Set 1 | Nov 10 1998 | `\AREA1\TILEZ` | **`C:\GAMES\GRUNTZ\GRUNTZ.EXE`** |
| `AREA<n>\WORLDZ\LEVEL<m>` | Gruntz - Level Set {5,9,13,17,21,25,29} | Nov/Dec 1998 | `\AREA<n>\TILEZ` | `C:\PROJ\GRUNTZ\GRUNTZ.EXE` |
| `GAME\{BATTLEZ,MULTI}\<name>` | Gruntz - Battlez {1,5,9,13,17,21,25,29} | Oct–Nov 1998 | `\AREA<n>\TILEZ` | `C:\PROJ\GRUNTZ\GRUNTZ.EXE` |

* `AREA1\WORLDZ\LEVEL4` was last saved by an editor launched from the project's
  **`DEBUG\`** output directory — corroborating an MSVC Debug configuration.
* **`AREA1\WORLDZ\LEVEL101`** is the odd one out twice: it is the only level
  outside the `LEVEL1..32` / `TRAINING1..4` scheme, and the only one authored on
  a machine with the game at `C:\GAMES` rather than `C:\PROJ`. Its 163 objects
  are a real, complete level (57 `TileTrigger`, 12 `RollingBall`, 11
  `GruntStartingPoint`, 8 `TileTriggerSwitch`, 1 `ExitTrigger`).
  `GruntzCommandId.h` carries **`CMD_DEBUG_WARP_LEVEL101 … LEVEL132`** — a
  32-slot bonus-level bank of which exactly one level shipped, and the demo ships
  none. Reachable only through the debug warp command.
* The `GAME\BATTLEZ` / `GAME\MULTI` pairs are byte-identical twins by name and
  their `tileDirectory` is what binds them to an area, since their path carries
  no `AREA<n>`.

---

## 4. The `BAT` — an art-pipeline script

`GRUNTZ\IMAGEZ\DELETEIDLES`, 30 277 bytes, CRLF, 2 174 lines, **1 636 `del`
commands**. It is a shipped-by-accident build script that trims frames out of
the artists' source tree:

```
cd BombGrunt
cd northeast
cd idle
del frame004.*
…
```

so the art source layout was `<root>\<Type>Grunt\<direction>\<animation>\frameNNN.*`,
with the REZ's `GRUNTZ\IMAGEZ` as `<root>` — the BAT's own resource path.

It names **24 grunt types in Monolith's own CamelCase**, which is the C++
identifier casing, not the archive's uppercase:

```
BombGrunt  BoomerangGrunt  BrickGrunt  ClubGrunt  GauntletzGrunt  GlovezGrunt
GooberGrunt  GravityBootzGrunt  GunHatGrunt  NerfGunGrunt  NormalGrunt
RockGrunt  ShieldGrunt  ShovelGrunt  SpringGrunt  SpyGrunt  SwordGrunt
TimeBombGrunt  ToobGrunt  ToobWaterGrunt  WandGrunt  WarpStoneGrunt
WelderGrunt  WingzGrunt
```

(`GravityBootz`, `GunHat`, `NerfGun`, `TimeBomb`, `ToobWater`, `WarpStone` —
note the internal capitals the uppercase archive paths destroy.) Only the four
diagonals get trimmed (`northeast`/`northwest`/`southeast`/`southwest`), and only
`idle`.

Each type keeps a contiguous window and deletes the rest, which recovers the
shipped idle sub-animation per type: `SwordGrunt` keeps 1–5, `NormalGrunt` keeps
6–8, `GunHatGrunt` keeps 8–13, `GauntletzGrunt` keeps **none** (all 12 deleted),
`ShovelGrunt` deletes 5–31.

The BAT corroborates two dead references in our own source:
`GruntEntranceMove.cpp` names `GRUNTZ_WINGZGRUNT_IDLE4` and `..._IDLE5`, and the
archive has only `GRUNTZ\ANIZ\WINGZGRUNT\IDLE1..3`. Idle content was cut, and the
EXE still asks for some of it.

---

## 5. The cheat table

Two tables, and the interesting parts are what is *missing* from each.

### 5a. `[CheatN]` in `GAME\ATTRIBUTEZ` (Blowfish, key `"1212C"` truncated to 4 bytes)

`NumCheatz=70`, but only **69 sections are present**. The gap is `[Cheat24]`,
and the neighbours identify it:

| N | code | value | comment |
|---|---|---|---|
| 23 | `MPCOPPERFIELD` | 33015 = 0x80f7 | `// Wandz` |
| **24** | **absent** | **(0x80f8)** | **—** |
| 25 | `MPVASFLAM` | 33017 = 0x80f9 | `// Welderz` |

The tool cheats run in alphabetical order and `WarpStonez` sits exactly between
`Wandz` and `Welderz`; 0x80f8 is the one unused value in the run; and
`GruntzCommandId.h` already carries **`CHEAT_GIVE_WARPSTONE = 0x80f8`**, so the
command is implemented in the EXE. The credits' joke list names
**`MPWARPSTONEZ`**. `[Cheat24]` was `MPWARPSTONEZ` and it was deleted before ship,
leaving an implemented-but-unreachable command.

`MPKELLY`, also named in the credits, is in neither table. Cut entirely.

**Cross-check:** all 69 shipped cheat `Value`s already have a `GruntzCommandId`
enumerator. Zero gaps.

### 5b. The 19 built-in codes compiled into `CheatMgr.cpp`

Stored obfuscated as plaintext **+ 0x3d**, because `CCheatMgr::CheckCode`
@0x23090 uppercases the typed string and adds 0x3d before the map lookup. Decoded
here for the first time; the statics were `s_cheat_<hex>` placeholders and have
been renamed accordingly:

| code | command | note |
|---|---|---|
| `MPFPS` | `CHEAT_FRAME_RATE_DISPLAY` | |
| `MPPOS` | `CHEAT_WORLD_POSITION_DISPLAY` | |
| `MPOBJECTS` | `CHEAT_OBJECT_COUNT_DISPLAY` | |
| `MPNOINFO` | `CHEAT_DEBUG_FLAG20` | |
| `MPSTOPWATCH` | `CHEAT_ELAPSED_TIME_DISPLAY` | |
| `MPBUILD` | `CHEAT_DEBUG_FLAG400` | shows `"Alpha Version, Build %i, …"` |
| `MPHOLOGRAM` | `CHEAT_KEVIN_LAMBERT` | `"My name is Kevin Lambert. You typed in my cheat code. Prepare to die."` |
| `MPLAMBERT`, `MPLAMBERTIAN`, `MPCHOP` | `CHEAT_KEVIN_LAMBERT_ALT` | |
| `MPGOBLE`, `MPSCORPIO` | `CHEAT_PROGRAMMING_GOD` | `"Brian L. Goble is a programming God..."` |
| `MPLITH`, `MPLOGO`, `MPMONOLITH`, `MONOLITH` | `CHEAT_MONOLITH` | `MONOLITH` is the one code with no `MP` prefix |
| `MPDEVHEADS` | `CHEAT_NO_OP` | looks up `GAME_DEVHEADS` — **no such resource**, dead |
| `MPWILDWACKY` | **0x80be** | had **no enumerator**; added as `CHEAT_WILD_WACKY` |
| `MPWAWAWAWAWAWA` | `CHEAT_WAWA` | see below |

`MPSCORPIO` next to `MPGOBLE` on the same command is a person's star sign or
handle; `MPCHOP` next to the Lambert codes likewise.

### 5c. `MPWAWAWAWAWAWA` — a dead reference proven by the demo

The credits joke list contains the line `wa-wa-wa-wa-wa-wa`. The cheat
`MPWAWAWAWAWAWA` posts `CHEAT_WAWA`, whose handler
(`GruntzMgrCmd.cpp`, `HandleCommand` @0x862f0) plays sound-registry key
`"GAME_WAWA"` and prints `"WA WA WA WA WA WA!"`.

`GAME\SOUNDZ\WAWA` is **present in `GRUNTDEM.REZ` (22 745 B) and absent from
retail `Gruntz.REZ`**. So the cheat is live in retail and its sound was removed —
and, usefully for us, the demo proves our reconstructed string literal
`"GAME_WAWA"` is the right one rather than a mis-transcription.

---

## 6. ANI cues and the stale `name` field

* **281 distinct sound-cue tokens across 660 files, and all 281 resolve** to a
  real WAV under `<NS>\SOUNDZ\…`. Zero dangling cues. (986 tokens counted with
  multiplicity in `ani-v1.md`; 651 occurrences of the 281 distinct names in
  retail alone.)
* The token namespace is a third registry view: `GRUNTZ_<TYPE>GRUNT_<CUE>`,
  `GAME_<CUE>`, `LEVEL_<CUE>`.
* **The on-disk `name` field is authoring residue, not a name.** 628 of 660 files
  leave it empty. Of the 32 that do not, **29 contain the literal string
  `GRUNTZ_IMAGEZ_NORMALGRUNT_NORTH_WALK`** regardless of which grunt or animation
  the file actually is (`CLUBGRUNT_DEATH`, `GUNHATGRUNT_IDLE1`, …) — an editor
  default nobody updated, and it names an *image set*, not an animation. The
  other three are `GAME_IMAGES_SPARKLE` (×2) and `GAME_IMAGES_CURSOR`.
* `GAME_IMAGES_…` and `GAME_MENUS_SELECT` (referenced from `Multi.cpp`) both use
  the English plural where the shipped tree uses `IMAGEZ` / `MENU`. Pre-branding
  fossils: the "z" convention was applied to the asset tree after some of this
  content was authored.

---

## 7. The voice archive

`GRUNTZ.VRZ` is the same REZ v1 container holding 1 517 WAVs under
`VOICES\<CATEGORY>\<LINE>`. The category set is a list of the game events that
have voice:

| category | retail | demo | | category | retail | demo |
|---|---:|---:|---|---|---:|---:|
| GRUNTZ | 974 | 145 | | ENTRANCEZ | 25 | 3 |
| PICKUPZ | 125 | 31 | | EXITZ | 14 | 3 |
| DEATHZ | 86 | 18 | | GIVETOY | 12 | 0 |
| MEGAPHONEZ | 67 | 6 | | CURSOR | 12 | 10 |
| ACKNOWLEDGE | 56 | 55 | | ENEMYSELECT | 11 | 9 |
| AREATRIGGER | 35 | 0 | | DAMAGETILE | 10 | 8 |
| DEATH{KING,NAPOLEAN,PATTON,VIKING} | 14 each | 0 | | ENEMYDETECT | 10 | 7 |
| CHECKPOINT | 8 | 6 | | SECRETSWITCH | 8 | 4 |
| SECRETSPOT | 7 | 2 | | BOOTY | 1 | 1 |

`DEATHKING` / `DEATHNAPOLEAN` / `DEATHPATTON` / `DEATHVIKING` name the **four
Warlordz** (`CWarlord`; `GAME_FORTRESSFLAGZ_KING` and `..._NAPOLEAN` also appear
as WWD image sets). `VOICES\GRUNTZ` has 38 subcategories — the 24 from the BAT
plus the toy grunts and **`HAREKRISHNAGRUNT`**, **`REAPERGRUNT`** (the
Conversion and Death-Touch forms, already modelled as `GRUNT_HAREKRISHNA` /
`GRUNT_REAPER` in `PickupType.h`) and `WARLORDZGRUNTMP` / `WARLORDZGRUNTSP`.

---

## 8. Keys our source names that the archive does not have

`the asset-key harvester (retired) --src .` harvests 538 registry-key literals
from `src/` + `include/` and resolves them: **494 resolve, 44 do not.** Running
it against the demo separates the three kinds of failure.

**Proven removed** — resolves in `GRUNTDEM.REZ`, absent from retail:

| key | demo resource | site |
|---|---|---|
| `GAME_WAWA` | `GAME\SOUNDZ\WAWA` | `GruntzMgrCmd.cpp` (`CHEAT_WAWA`) |
| `STATEZ_PREVIEW` | `STATEZ\PREVIEW` | `LevelPreview.cpp` |

(The demo's `STATEZ` has `PREVIEW` and no `CREDITZ`; retail's has `CREDITZ` and
no `PREVIEW`.)

**Cut content** — in neither archive, so the art was dropped and the code kept:

* `GAME_TREASURE_{CHALICES,CROSSES,GECKOS,SCEPTERS}_{RED,GREEN,BLUE,PURPLE}` —
  **16 keys** from `GruntzMgr.cpp`. Nothing named `CHALIC`/`SCEPTER`/`GECKO`/
  `CROSSES` exists in either archive; only `GAME\SOUNDZ\TREASURE` survives. A
  whole Booty-screen treasure set.
* `GAME_ACTIONAREA_BLUE`, `GAME_ACTIONAREA_RED` (`ActionArea.cpp`) — no
  `GAME\IMAGEZ\ACTIONAREA` at all; only `GAME\ANIZ\ACTIONAREA`.
* `GRUNTZ_WINGZGRUNT_IDLE4`, `..._IDLE5` (`GruntEntranceMove.cpp`) — `IDLE1..3`
  exist. See [§4](#4-the-bat-an-art-pipeline-script).
* `GAME_STATUSBAR_TABZ_{GAMETAB_WARP, RESOURCETAB_CONVEYORTOP,
  RESOURCETAB_CONVEYORBOTTOM, RESOURCETAB_MACHINEFOREGROUND, STATZTAB_SMALL}` —
  the parent directories exist, these leaves do not.
* `GAME_DEVHEADS` (the `MPDEVHEADS` cheat), `GAME_ICONFLASH`, `GAME_ATTACK`,
  `GRUNTZ_WINGZGRUNT_PROJECTILELOOP`, `STATEZ_SPLASH`, `STATEZ_MULTI`,
  `GAME_MENUS_SELECT`.

**Not registry keys at all** — the resolver's own false positives, listed so the
next run does not re-investigate them: `GAME_DELETE`, `GAME_INFO`, `GAME_KEY`,
`GAME_LOAD`, `GAME_SAVE`, `GAME_SAVEMSG`, `GAME_OVERWRITE` (dialog/bute keys) and
the runtime-concatenated prefixes `GAME_INGAMEICONZ_`, `GRUNTZ_PICKUPS_`,
`GRUNTZ_NORMALGRUNT_`.

---

## 9. Identifiers cross-checked, with verdicts

| Identifier family | Source | Verdict |
|---|---|---|
| 34 WWD `logic` names | WWD records | **All 34** have registered dispatchers in `RegisterGameObjectLogicTypes` @0xa3b0. Confirms our logic-type table exactly. |
| `GlobalAmbientSound` | WWD, 212 records | **Resolved.** A dispatch alias over `CAmbientSound`, not a class. No action. [§3b](#3b-globalambientsound--verdict-not-a-missing-class-and-not-a-misnaming) |
| 186 `image_set` keys | WWD records | **All resolve** to real `<NS>\IMAGEZ` paths. |
| 29 `animation` keys | WWD records | 28 resolve; `LEVEL_AMBIENT_AREA6LOOP` is a shipped typo. |
| 56 `GAME_INGAMEICONZ_*` icons | WWD records | **All** map to a `PICKUP_*` enumerator. `GAME_INGAMEICONZ_SECRET{W,A,R,P}` ↔ `PICKUP_{W,A,R,P}` = 0x5a..0x5d, in that order — the letters spell **WARP** and the enum already has them contiguous and in the right sequence. `PICKUP_MEGAPHONE` = `MEGAPHONEZ`; `PICKUP_WARPSTONE` covers `WARPSTONEZ1..4`. Confirms `PickupType.h`. |
| 281 ANI cue tokens | ANI records | **All resolve.** Zero dangling. |
| 69 cheat `Value`s | `ATTRIBUTEZ` | **All** have a `GruntzCommandId` enumerator. |
| 19 built-in cheat codes | `CheatMgr.cpp` | Decoded; **18 map to a named command, 1 (`MPWILDWACKY`, 0x80be) had no enumerator** — added. |
| `[Cheat24]` | `ATTRIBUTEZ` gap | Reconstructed as `MPWARPSTONEZ` = 0x80f8. [§5a](#5a-cheatn-in-gameattributez-blowfish-key-1212c-truncated-to-4-bytes) |
| 24 CamelCase grunt types | `DELETEIDLES.BAT` | All present in the tree; the BAT recovers their **internal capitalisation**. |
| 38 voice subcategories | VRZ | All present; `HAREKRISHNAGRUNT`/`REAPERGRUNT` confirm `GRUNT_HAREKRISHNA`/`GRUNT_REAPER`. |
| 6 retail `.cpp` names | credits | **None** in `src/`, `include/`, `config/` or `docs/`. New TU-partition evidence. [§2a](#2a-retail-source-file-names--tu-partition-evidence) |
| `MoveGruntAroundObstacle()`, `ActuallyRemoveGrunt()` | credits | **Not in the tree.** Unbound retail function names. |
| 44 unresolved `src/` asset keys | `asset_keys` | 2 proven removed, ~28 cut content, ~14 resolver false positives. [§8](#8-keys-our-source-names-that-the-archive-does-not-have) |

## 10. Open

* `LogicTypeId` 0x3f9 and 0x40e are unattributed.
* `LOGIC_SINGLEFRAMEMESSAGE` (0x3eb) has a class and an id but no dispatcher
  registration — where is `CSingleFrameMessage` created?
* `MoveGruntAroundObstacle` and `ActuallyRemoveGrunt` are unbound.
* `CHEAT_WILD_WACKY` (0x80be) has no reconstructed handler.
* `AREA1\WORLDZ\LEVEL101` is unreachable except by debug warp; whether the
  release build reaches `CMD_DEBUG_WARP_LEVEL101` at all is unchecked.
* The two WAVs carrying a `fact` chunk despite `wFormatTag == 1` are
  unexplained (harmless).
