# Gruntz (1999) — Reverse-Engineering Survey & Plan

**Goal:** compile every viable avenue for reverse-engineering / decompiling *Gruntz*
(Monolith Productions, 1999) and rank them by expected payoff, to feed an existing
decompilation harness.

**Status of this document:** combines (a) a completed multi-source web research pass
(adversarially fact-checked) with (b) **direct ground-truth PE analysis** of the real
retail `GRUNTZ.EXE` pulled from the Internet Archive CD image. Items marked
**[MEASURED]** were observed directly in the binary on this machine; items marked
**[RESEARCH]** come from the web pass; **[INFERRED]** are reasoned conclusions.

---

## 0. TL;DR — the three highest-payoff avenues

1. **Reuse the engine-sibling RE work.** Gruntz, *Claw* (1997), and *Get Medieval*
   (1998) all run on the **same engine — WAP32 (Windows Animation Package 32)** by
   Monolith co-founder Brian Goble. *Claw* already has a from-scratch open-source
   reimplementation (**OpenClaw**) plus standalone format libraries (**libwap**,
   **libwap32**) covering the exact asset formats Gruntz uses. This is free, directly
   transferable engine knowledge.
2. **Lean on RTTI + the leaked source-tree map.** The retail EXE is a
   statically-linked MSVC/MFC C++ build that **retains 231 RTTI mangled class names**
   (the whole object model + inheritance) **and leaks original source file paths**
   (`C:\Proj\DDrawMgr\…`, `C:\Proj\Gruntz\GruntzMgr.cpp`, …). That is an enormous
   head-start: named classes and module boundaries before a single function is read.
3. **FLIRT/Lumina against the static CRT + MFC.** No `MSVCRT.DLL` / `MFC42.DLL`
   imports → both are statically linked → a large fraction of the binary is
   identifiable library code, not game code. Signature-matching strips that away fast.

A nascent Gruntz-specific source-recreation project (`tomalla5120x/gruntz`) already
exists and can be extended rather than started from zero.

---

## 1. Engine identification  [RESEARCH, high confidence]

- **Engine: WAP32 — "Windows Animation Package 32"**, written by **Brian L. Goble**
  (Monolith co-founder / VP Engineering). His own words: *"I eventually rewrote WAP
  from scratch for Windows 95 and DirectX, called it WAP32, and it's the engine we
  used for Claw, Get Medieval, and Gruntz!"*
- Shared, by the **same author**, across three 2D titles:
  **Claw (1997) → Get Medieval (1998) → Gruntz (1999)** (Gruntz was the last).
  WAP32 is the DirectX/Win95 rewrite of Goble's earlier 16-bit WAP sprite engine
  (first in *The Adventures of MicroMan*, 1993).
- In-game cheat *"Brian L. Goble is a programming God"* corroborates authorship.
- **Distinct from LithTech** — Monolith's *3D* engine is a separate codebase; its SDK
  does **not** overlap WAP32. Do not chase LithTech leaks for this.

**Corroborated [MEASURED] in the binary:** RTTI exposes a `z`-prefixed runtime
namespace (`zErrHandling`, `_zvec`, `_zdvec`, `zDArray<…>`) — this is the WAP32 base
library statically linked into the game.

### Asset formats  [RESEARCH, high confidence; container [MEASURED] on the CD]
- `GRUNTZ.REZ` — primary asset archive (Monolith REZ format); `GRUNTZ.VRZ` companion.
- `.WWD` — *Wap World Document* level/world files (zlib/deflate-compressed).
- `.PID` — sprites/images (header: width/height/offset + palette index).
- `.PAL` — palettes (raw RGB triplets, no header).
- `.ANI` — animations. `.XMI` — music (XMI→MIDI via the Corsix `EVNT` conversion).
- WWD is a shared family with per-game variation (Claw uses "double" tiles w/ sub-areas;
  Gruntz tiles are all "single" type) — why cross-game editors support Gruntz only
  "to some extent."

---

## 2. Ground-truth PE analysis of retail `GRUNTZ.EXE`  [MEASURED]

Pulled directly from the Internet Archive CD image (see §6 for the download trick).

```
File:    GAME/GRUNTZ.EXE   (English retail, v1.0 unpatched)
Size:    2,511,872 bytes
MD5:     81c7f648db99501bed6e1ee71e66e4a0
SHA1:    54b7833410aa3c8f6b431edc20ee8d135e7e4886
Format:  PE32 / pei-i386, Windows GUI, 6 sections, ImageBase 0x00400000
```

| Property | Value | Why it matters for RE |
|---|---|---|
| **Linker version** | **5.10** | Visual C++ **5.0**-era toolchain (link 5.10; VC6 = 6.00). Separate `.idata` section corroborates VC5. |
| **Rich header** | **present** | Exact `cl.exe`/`link.exe` build IDs (`@comp.id`) recoverable → pins precise compiler build & object provenance. |
| **Relocations** | **present** (`HAS_RELOC`, `.reloc` ≈ 113 KB) | Full base-reloc table → can rebase freely; not stripped. |
| **Debug flag** | `HAS_DEBUG` set | Debug directory present — check for a CodeView/`.pdb` path record (no `.pdb` string seen in strings, but the directory entry is worth a structured dump). |
| **C runtime** | **statically linked** (no `MSVCRT.DLL` import) | Big surface for FLIRT/Lumina CRT signatures. |
| **MFC** | **statically linked** (no `MFC42.DLL` import; `CArray`/`C…` RTTI present) | MFC FLIRT sigs + known MFC class layouts apply; subtract MFC noise from game code. |
| **RTTI** | **present — 231 mangled class names** | Whole class hierarchy recoverable (IDA Class Informer / Ghidra RTTI analyzer). |
| **Sections** | `.text .rdata .data .idata .rsrc .reloc` | Classic MSVC release layout. |

**Imports (subsystem stack):** `DDRAW`, `DINPUT`, `DSOUND`, `DPLAYX` (DirectPlay →
multiplayer), `WINMM`, `mss32` (Miles Sound System / RAD), `smackw32` (Smacker video /
RAD), plus `COMCTL32`, `comdlg32`, `SHELL32`, `ADVAPI32`, `VERSION`, `WINSPOOL`.

### 2.1 Leaked original source-tree map  [MEASURED — high value]
Assert/error strings leak the original build paths, revealing Monolith's **modular
"manager" architecture** (each a separately-built static lib, then linked into the game):

```
C:\Proj\DDrawMgr\DDRAWMGR.CPP   ddrawmgr.h  DIRPAL.CPP  DIRSURF.CPP   ← DirectDraw (gfx/palette/surface)
C:\Proj\DinMgr2\DinMgr2.cpp     InputDevice.cpp                        ← DirectInput
C:\Proj\Dsndmgr\DSNDMGR.CPP     DSndMgSR.cpp                           ← DirectSound
C:\Proj\NetMgr\NetMgr.cpp                                              ← DirectPlay networking
C:\Proj\incs\ddrawmgr.h         netmgr.h                               ← shared headers
C:\Proj\Gruntz\GruntzMgr.cpp                                           ← the game (CGruntzMgr)
```
The `DDrawMgr / DinMgr2 / Dsndmgr / NetMgr` managers are reusable subsystems → almost
certainly the **same code present in Claw and Get Medieval**. This gives natural
decompilation module boundaries *and* cross-game diffing targets.

### 2.2 Object model (sample of 231 RTTI classes)  [MEASURED]
197 `C`-prefixed game/MFC-style classes, 5 `z`-prefixed runtime classes, plus templates
(`zDArray<…>`, MFC `CArray<…>`). Examples of the game object hierarchy:
`CGrunt`, `CGruntPuddle`, `CTeleporter`, `CBoomerang`, `CExplosion`, `CRollingBall`,
`CTimeBomb`, `CActionArea`, `CUserLogic`/`CUserBase`, `CAmbientSound`/`CRandomAmbientSound`,
`CEyeCandy`/`CFrontCandy`/`CBehindCandy`, `CGruntCreationPoint`, `CCheckpointTrigger`,
`CSpotLight`, `CWapX`.

### 2.3 Other observed strings  [MEASURED]
- Version resource: `Copyright 1998, Monolith Productions Inc.`, `ProductName: Gruntz`,
  website `http://www.gruntzgoo.com`.
- **`Gruntz Pre-Release`** + an unlock-key flow ("Enter your key below to unlock Gruntz",
  "contact Monolith via email GruntzInfo@lith.com … username and access code"). Implies a
  **key-locked pre-release/preview build** existed — a build worth hunting (§5/§9-E).
- Music uses SoundFont2: `C:\MUSIC\Gruntz.SF2`, `Gruntz4.SF2`; debug log `c:\gruntz.log`.

---

## 3. Existing projects to leverage  [RESEARCH, high confidence]

### Engine-sibling reimplementations / format libs (transferable code)
- **OpenClaw** — `github.com/pjasicek/OpenClaw`. From-scratch C++ reimplementation of
  *Captain Claw*; reads original `CLAW.REZ`. Bundles **`libwap`** with per-format parsers:
  `WwdFile.cpp`, `PidFile.cpp`, `AniFile.cpp`, `PalFile.cpp`, `XmiFile.cpp`,
  `RezArchiveFile.cpp`. Paths are hardcoded to `CLAW/…`, so a Gruntz loader needs
  path/tile-type adjustments — but the *parsing logic is the engine-shared part*.
- **libwap32** — `github.com/cubuspl42/libwap32`. C/C++ library explicitly for the WAP32
  formats of **Claw, Gruntz, and Get Medieval**. Full WWD read (`src/wwd_read.cpp`, zlib)
  + write (`src/wwd_write.cpp`), PID header (`include/wap32/pid.h`).

### Gruntz-specific
- **`tomalla5120x/gruntz`** — in-progress, from-scratch **source-code recreation** of
  Gruntz targeting the **patched English build 1.0.1.77**. Aims for near 1:1 functional
  equivalence with the original assembly (keeps original bugs), uses MFC `C`-prefix class
  names (`CGameApp`, `CGruntzWnd`, `CGruntzMgr`), built w/ MSVS 2019. Already implements
  the CD-presence check, in-binary resource reading, and the advanced options dialog.
  Dormant since ~July 2020 → **extend this rather than start fresh.**

### Modding tools / editors (format validators + asset round-trip)
- **WapMap** (`github.com/Zax37/WapMap`) — WAP32 level editor (orig. kijanek6, 2010–13);
  opens Gruntz/Get Medieval WWD "to some extent."
- **PIDStudio** (`github.com/Zax37/PIDStudio`) — PID graphics editor (Claw/Gruntz), v1.0.0 2024.
- **Gruntz Level Editor (GLE / "GruntzEd")** (~470 KB) — the level tool, on GooRoo's
  forum / Nexus Mods / ModDB. *Itself a WAP32 binary worth analyzing for format internals.*
- PID format was originally RE'd by **kijanek6** from `PCX2PID.EXE` inside `CLAW.REZ`.

### Communities (likely holders of un-indexed knowledge/files)
- **GooRoo's Gruntz Forum** — `gooroosgruntz.proboards.com` (active modding hub).
- **Data Shenanigans** blog — `datashenanigans.pl` (WWD spec, modding history; primary-ish).
- **The Cutting Room Floor** — `tcrf.net/Gruntz`. **Nexus Mods** — `nexusmods.com/gruntz`.
- `rudissaar/gruntz-installer-creator` (GitHub) — packaging helper.

---

## 4. Symbols / PDB / debug-info status  [RESEARCH + MEASURED]

- **No PDB, MAP, or symbol package known shipped or leaked** for Gruntz, Claw, or Get
  Medieval (absence of evidence, not proof — modding circles may hold private files; see
  §10 open questions). The `tomalla` author worked from **RTTI + strings**, not a PDB.
- **But the practical recovery surface is excellent** [MEASURED]:
  - 231 RTTI mangled class names → class names + inheritance for free.
  - Leaked `.cpp/.h` source paths → file/module names + project layout.
  - Static CRT + static MFC → high FLIRT/Lumina library-ID rate.
  - Rich header → exact compiler build provenance.
  - `HAS_DEBUG` directory entry → dump it for any residual CodeView record.
- **LithTech SDK is irrelevant** (separate 3D codebase).

---

## 5. Available builds / versions  [RESEARCH + MEASURED]

| Build | Source | Notes |
|---|---|---|
| **English retail v1.0** | `archive.org/details/gruntz-pc` → `Gruntz.iso` (610 MB) | **EXE analyzed here**; in-ISO files directly fetchable (§6). |
| English retail (BIN/CUE+sub) | `archive.org/details/gruntziso` | `IMAGE.img` 700 MB; full Redbook preservation. |
| **English RIP (92 MB)** | myabandonware / oldgamesdownload | CD audio/video stripped → smallest source of the real `GRUNTZ.EXE`. |
| LGU Repack (96 MB) | myabandonware | repack by Bladez1992. |
| **Polish** | `archive.org/details/gruntz-pl`, `gruntz-pl_202411`; myabandonware disc image (569 MB) | localized build → diff vs EN. |
| Czech magazine (full game) | `archive.org/details/czgamestar15cd` | Gamestar 15 (2000-02), CD2 = full Gruntz. |
| Polish magazine (Apr 1999) | `archive.org/details/Gruntz_PC_World_Komputer_Extra_4-99_Poland` | likely demo/preview — check for older/pre-release build. |
| **Patch v1.01** | `ladyofthecake.com/gruntz/Grnt_101.zip` (verified direct, 297 KB) | **[MEASURED]** = an **RTPatch Professional** (Pocket Soft) binary-diff that patches `GRUNTZ.EXE` → brings v1.0 to **1.0.1.77**. The patcher's own toolchain (linker 2.x, `.bss`/`.idata`) is RTPatch's, NOT Monolith's — ignore it for compiler fingerprinting. |
| **Demo** (`GRUNTDEM.EXE`/`ROMDEMO.EXE`) | on the retail CD under `PREVIEWS\DEMOS\`; also standalone | full game EXE, small — good light RE target. |
| Pre-release / key-locked build | **unconfirmed** — implied by "Gruntz Pre-Release" strings in retail EXE | worth hunting in communities (§9-E). |

**Toolchain (measured):** MSVC **5.0**-era (linker 5.10), MFC + CRT static, RTTI on,
relocs present. The `tomalla` "1.0.1.77 / VS2019" note refers to the *recreation's* build,
not the original toolchain.

---

## 6. Technique: fetch individual files from the CD without the full ISO  [MEASURED — works]

Internet Archive serves files **inside** the ISO. List and pull single files directly:

```sh
# list everything inside the disc image
curl -fsSL "https://archive.org/download/gruntz-pc/Gruntz.iso/" | grep -oE '[^"]+\.(EXE|REZ|DLL)'

# pull just the game EXE (note %2F-encoded path separators, -L follows the 302)
curl -fsSL -o GRUNTZ.EXE "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FGRUNTZ.EXE"
```

Notable files seen inside `gruntz-pc/Gruntz.iso`:
`GAME/GRUNTZ.EXE`, `DATA/GRUNTZ.REZ`, `GAME/MSS32.DLL`, `GAME/SMACKW32.DLL`,
`AUTORUN.EXE`, `CDTEST.EXE`, `PREVIEW.EXE`, and engine-sibling demos
**`PREVIEWS/DEMOS/CLAWDINS.EXE`** (Claw), **`PREVIEWS/DEMOS/GMDEMINS.EXE`** (Get Medieval),
`PREVIEWS/DEMOS/ROMDEMO.EXE`. (The Claw + Get Medieval demos shipping on the Gruntz disc
is itself corroboration of the shared-engine family.)

---

## 7. Prioritized avenues (ranked by expected payoff)

| # | Avenue | Effort | Payoff | Why |
|---|---|---|---|---|
| **A** | Load EXE → apply **RTTI analyzer** (Class Informer / Ghidra RTTI) + name funcs from leaked source paths | low | **very high** | Instant 231 named classes + module map; the single biggest force-multiplier. |
| **B** | **FLIRT/Lumina** against static CRT + **MFC (VC5)** sigs | low | **very high** | Removes most non-game code; isolates the ~game logic worth reading. |
| **C** | Port **OpenClaw `libwap` / `libwap32`** to read `GRUNTZ.REZ` + `.WWD/.PID/.ANI/.PAL/.XMI` | med | high | Assets decoded without RE'ing the loader from scratch; enables asset-driven validation. |
| **D** | **Cross-game diff** the `DDrawMgr/DinMgr2/Dsndmgr/NetMgr` managers vs Claw & Get Medieval binaries | med | high | Shared engine code → matching functions across 3 binaries triangulates names/behavior. |
| **E** | **Extend `tomalla5120x/gruntz`** recreation (target 1.0.1.77) | med→high | high | Don't start from zero; existing scaffolding + class names. |
| **F** | Decode **Rich header** → exact `cl.exe` build → load matching library/PDB sigs | low | med | Sharpens B; provenance of each object module. |
| **G** | Recover **GLE / GruntzEd** editor internals (it speaks WWD/PID natively) | med | med | Editor reveals authoritative format semantics + validation. |
| **H** | Mine **communities** (GooRoo, Data Shenanigans) for private symbols/maps/SDK fragments | low | med (variable) | Possible un-indexed gold; cheap to ask. |
| **I** | Hunt the **pre-release/key-locked build** & demos for extra symbols/asserts | low→med | med | Pre-release builds often retain more debug strings/asserts. |
| **J** | Structured **debug-directory dump** for any residual CodeView/PDB path | low | low→med | Cheap check; `HAS_DEBUG` is set. |

---

## 8. Concrete next steps (for the decomp harness)

1. **Binaries already on disk** at `binaries/`:
   - `retail_en/GRUNTZ.EXE` (v1.0, MD5 `81c7f648…`) — primary target.
   - `patch_101/Grnt_101.exe` (RTPatch v1.01 diff) — apply to v1.0 to get 1.0.1.77 if matching `tomalla`'s target.
2. **Pull the engine siblings** for cross-diffing (avenue D): grab `CLAW.EXE` /
   `GETMED.EXE` (or their demo EXEs from the Gruntz CD `PREVIEWS\DEMOS\`).
3. **Pull `GRUNTZ.REZ`** (`…/Gruntz.iso/DATA%2FGRUNTZ.REZ`) and the **GLE editor**; wire
   up `libwap32` for asset round-trip.
4. **Load `GRUNTZ.EXE` into the harness** → RTTI pass (A) → FLIRT/MFC pass (B) →
   rename from leaked `.cpp` paths → begin per-manager module recovery.
5. **Decode the Rich header** (F) to select exact VC5 CRT/MFC signature sets.
6. **Clone** `OpenClaw`, `libwap32`, `tomalla5120x/gruntz`, `WapMap`, `PIDStudio` for
   reference; **post in GooRoo's forum** asking about private symbols/maps/editor source (H).

---

## 9. Open questions  [from research; some now answered by §2]
- ~~Compiler version / relocations / RTTI?~~ → **Answered [MEASURED]:** VC5-era (link 5.10),
  relocs present, RTTI present, CRT+MFC static.
- Exact `cl.exe` build numbers (Rich header decode) and resulting FLIRT hit-rate — TBD.
- How far does Gruntz's WAP32 diverge from Claw at the engine level (format/opcode deltas)?
  Can `libwap` fork with only path/tile-type changes, or are deeper edits needed?
- Any **private** symbol/map files, internal SDK fragments, or **GLE editor source** in the
  Gruntz modding community not surfaced by public search?
- Does a **key-locked pre-release / preview build** survive anywhere? (strings imply it existed.)

---

## 10. Reference list

**Repos / code**
- OpenClaw — https://github.com/pjasicek/OpenClaw
- libwap32 — https://github.com/cubuspl42/libwap32
- Gruntz source recreation — https://github.com/tomalla5120x/gruntz
- WapMap (level editor) — https://github.com/Zax37/WapMap
- PIDStudio (PID editor) — https://github.com/Zax37/PIDStudio
- gruntz-installer-creator — https://github.com/rudissaar/gruntz-installer-creator

**Format / engine docs**
- WWD spec — http://datashenanigans.pl/2020/06/gruntz-wwd-specification/
- Modding history (REZ/PID/PCX2PID origins) — http://datashenanigans.pl/2017/12/gruntz-the-brief-history-of-modding/
- Engine author quote / lore — https://blood-wiki.org/index.php/Gruntz
- MobyGames WAP32 engine group — https://www.mobygames.com/game/7096/gruntz/
- TCRF — https://tcrf.net/Gruntz

**Builds / downloads**
- IA English ISO — https://archive.org/details/gruntz-pc
- IA English BIN/CUE — https://archive.org/details/gruntziso
- IA Polish — https://archive.org/details/gruntz-pl
- IA Czech magazine (full game) — https://archive.org/details/czgamestar15cd
- myabandonware (RIP/ISO/repack/PL/patch/editor) — https://www.myabandonware.com/game/gruntz-a4w
- oldgamesdownload — https://oldgamesdownload.com/gruntz/
- v1.01 patch (direct) — http://www.ladyofthecake.com/gruntz/Grnt_101.zip

**Communities**
- GooRoo's Gruntz Forum — https://gooroosgruntz.proboards.com
- Nexus Mods (Gruntz) — https://www.nexusmods.com/gruntz
- ModDB (Gruntz / GruntzEd)

**Reliability caveats:** engine + format facts rest on primary code (OpenClaw, libwap32,
tomalla) + the author's own quote → high confidence. "No PDB leaked" is absence-of-evidence,
not proof. AI-content-farm domains were excluded during research. Exact compiler build,
FLIRT hit-rate, and engine-divergence vs Claw remain to be measured in the harness.

---

## 11. Engine cross-diff corpus — three binaries, MEASURED

I pulled the retail binaries for all three WAP32 games and triaged them identically.
The shared engine is now confirmed **at the binary level** (not just from documentation).

```
GRUNTZ.EXE  (EN v1.0)   2,511,872 B  md5 81c7f648db99501bed6e1ee71e66e4a0  binaries/retail_en/
CLAW.EXE    (EN v1.2)   1,369,600 B  md5 878f9c8651ea6c35827258da7845e927  binaries/claw_retail/
MEDIEVAL.EXE(GM, portbl)1,116,672 B  md5 8829ec5b1a565ac75ac24a88501be816  binaries/getmed_retail/
```

| Property | GRUNTZ | CLAW | MEDIEVAL |
|---|---|---|---|
| Linker (opt. hdr) | **5.10 (VC++ 5.0)** | **5.10** | **5.10** |
| Rich `cvtres` build | **1668** | **1668** | **1668** |
| Rich key | `0x32022d89` | `0x000c0d88` | `0x000c0d88` |
| Extra Rich compiler module | linker build 8034 (12 obj) | — | — |
| Sections | `.text .rdata .data .idata .rsrc .reloc` | `.text .rdata .data .rsrc` | `.text .rdata .data .rsrc` |
| Relocations | **present** | stripped | stripped |
| CRT / MFC | static / static | static / static | static / static |
| RTTI class count | 231 | 231 | 206 |
| Shared base classes | `CGameApp CGameMgr CGameWnd CWapX CWapObj CUserLogic zDArray<…>` | same | same |
| Per-game manager (leaked path) | `C:\Proj\Gruntz\GruntzMgr.cpp` | (engine mgrs only) | `C:\Proj\Gq\GqMgr.cpp` |
| Shared manager tree | `DDrawMgr DinMgr2 Dsndmgr NetMgr` + `incs\` | `DDrawMgr DSndMgr NetMgr` + `incs\` | `DDrawMgr DinMgr2 Dsndmgr NetMgr` + `incs\` |
| Middleware imports | DDRAW DINPUT DSOUND DPLAYX mss32 smackw32 | DDRAW DSOUND DPLAYX mss32 smackw32 + WININET WSOCK32 | (WAP32 stack) |

**Conclusions [MEASURED]:**
- **Identical engine framework** across all three: same `CGameApp/CGameMgr/CGameWnd/CWapX/CUserLogic`
  base classes and the same `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,incs}` source tree. Each
  game is just a `C<codename>Mgr : CGameMgr` (`CGruntzMgr`, `CGqMgr` for Get Medieval / codename "Gq").
- **Same toolchain**: VC++ 5.0 linker (5.10) + `cvtres` build 1668 in all three.
- **Claw and Get Medieval share the same Rich key** (`0x000c0d88`) → built in the same toolset
  session; **Gruntz is a later/different build** (distinct key, plus an extra linker record (build 8034). So Gruntz = the most-evolved snapshot of the engine.
- Claw/GM have **relocations stripped** and merged `.idata`; **Gruntz keeps a full `.reloc`**.
- This makes **3-way function/class diffing** highly effective: a routine appearing in all three
  is engine; one only in Gruntz is game logic. Claw additionally has **OpenClaw** as a semantic
  oracle (see §12). Recommended primary anchor pair: GRUNTZ ↔ CLAW (richest RTTI overlap, 231/231).

### Rich header raw records (for the harness' comp-id DB to resolve precisely)
```
GRUNTZ : 0x00131f62 (prodid 0x13, build 8034, count 12) | 0x00000000 (count 1057) | 0x00060684 (cvtres, build 1668, count 1)
CLAW   : 0x00000000 (count 1145) | 0x00060684 (cvtres, build 1668, count 1)
MEDIEVAL: 0x00000000 (count 1108) | 0x00060684 (cvtres, build 1668, count 1)
```
Optional-header linker 5.10 is authoritative (VC++ 5.0). `tomalla` independently concluded
"VC6 or earlier" from EH magic numbers — consistent. (Decoder used: `perl /tmp/rich.pl <exe>`.)

**TOOLCHAIN PINNED -- VC++ 5.0 SP3 [corrected]:** `cvtres` build **1668** is an exact, unique match for **Visual C++ 5.0 (VS97) Service Pack 3** (`cvtres 5.00.1668`) per the dishather/richprint comp.id database. prodID `0x13` build **8034** is the **linker** (Linker512), NOT a C/C++ compiler module -- VC5's `cl 11.00` emitted no comp.id, so there is no compiler record in the Rich header. Target tools: **cl 11.00.x, link 5.10.7303, cvtres 5.00.1668**; the exact `cl` build is not in the binary, so take it from the matched SP3 toolchain and validate by byte-diff. `detect-it-easy`/`diec` (in nixpkgs) confirms the linker (`Microsoft Linker(5.12.8034)`); ignore its `Compiler 11.00-13.10` line (a heuristic collision with a modern VS2022 build stamp).

---

## 12. Engine-RE landscape — PDBs, decomp vs reimplementation (RESEARCH, verified)

### Q: Do Claw / Get Medieval (or any WAP32 game) have leaked PDBs/symbols?
**No.** No PDB, `.MAP`, `.DBG`, CodeView package, or symbol-bearing/unstripped EXE was found
for **any** WAP32 game, across every reachable primary source (tomalla repo + forum thread,
captainclaw.net "The Claw Recluse", TCRF, GitHub). The entire RE community works **without**
symbols — `tomalla` reverses off the stripped retail binary using RTTI + MFC source + MSDN +
diagnostic strings (his `@address`/`@offset` annotation system). *Caveat:* xentax/zenhax and the
Claw Discord/Clawmania were not fully reachable in search — a small residual chance of an obscure
leak there, but nothing on the open web. **The recovery surface is RTTI + leaked paths + FLIRT,
not a symbol file.**

### Q: Is there compilable decompiled/reimplemented source we can build?
There is **no matching decompilation** (original-source-that-relinks-to-the-binary) for any of
the three. What exists:

| Project | Game | Type | Lang / build | Status | Compiles? | Needs assets |
|---|---|---|---|---|---|---|
| **pjasicek/OpenClaw** | Claw | **reimplementation** (clean-room) | C++ / CMake / SDL2+Box2D | dormant (~2022) | yes | CLAW.REZ |
| Zax37/ClawJS | Claw | reimplementation | TypeScript/Node | dormant 2020 | — | yes |
| jakub-trzebiatowski/clawx | Claw | OpenGL renderer only | C++ | partial | — | yes |
| CrazyHook (jakub-trzebiatowski) | Claw | unofficial patch (not RE) | — | shipped | n/a | game |
| **tomalla5120x/gruntz** | Gruntz | **RE source recreation** (1:1 functional) | C++/MFC / VS2019 | dormant 2022 | yes (bootstrap only) | game |
| Dizgruntled/Dizgruntled | Gruntz | reimplementation/remake | TypeScript / Vite | active-ish 2023 | yes | yes |
| n1ght4ngel19/gruntz-unityverse | Gruntz | Unity remake | C#/Unity | — | — | yes |
| **Get Medieval** | — | **nothing** (no reimpl, no decomp) | — | — | — | — |

- **Best compilable engine reimplementation = OpenClaw** (Claw). Clean-room C++, builds with
  CMake+SDL2, runs original `CLAW.REZ`. Useful as a **behavioral oracle** for the shared WAP32
  engine, *not* as Gruntz source.
- **Get Medieval has no engine RE project** — only file-format tooling (libwap32, WapMap support).
- For a true **Gruntz decompilation**, there is no shortcut codebase: `tomalla` is the only RE-source
  effort and it's partial (see assessment below).

### Asset/format & tooling layer (engine internals beyond formats)
- **jakub-trzebiatowski/libwap32** (canonical; `cubuspl42` 301-redirects here) — C/C++ REZ/WWD/PID/
  PAL/ANI/XMI lib for Claw+Gruntz+GM; includes `wwd2html`. Dormant (2016).
- **Zax37/WapMap** — leading WWD level editor; **actively maintained (commits into 2026)**.
- **Zax37/PIDStudio** — PID editor. **Zax37/WAP64** — undocumented 64-bit WAP experiment (unclear).
- **fizary/wap32-viewer** — browser REZ explorer (TS).
- **No public IDA `.idb` / Ghidra `.gpr`** database found for any WAP32 game. Deepest *runtime*
  internals (class hierarchies, vtables) live in the **tomalla repo + GooRoo's forum thread**.

### `tomalla5120x/gruntz` — assessment (you said the author suggests restarting; agreed, but harvest it)
~8k LOC, last commit 2022-09-27, targets **patched build 1.0.1.77** (`md5 199d4613e4587e1d720623dc11569e4d`
— note: that's the *patched* EXE; our `binaries/retail_en/GRUNTZ.EXE` is unpatched v1.0). What's
actually there: the MFC app/window/manager **bootstrap** — `WAP32::CGameApp/CGameMgr/CGameWnd`
skeletons (with reconstructed vtable slots + field offsets), `CGruntzApp/CGruntzMgr/CGruntzWnd`,
`CNetMgr` (DirectPlay lobby), registry/winapi/memory-pool utils, extracted resources (.rc/icons),
and the advanced-options dialog. The gameplay layer (the 197 `C*` game classes — `CGrunt`,
`CTeleporter`, …) is essentially **untouched** (mostly `UnknownVirtualMethodN`/`fieldUnknownXXX`
placeholders, many `@todo`).
**Verdict: restart the codebase, but harvest three things as a seed —**
1. the **methodology + annotation conventions** (`@address @offset @vftable @bug @legacy @todo`),
2. the **reconstructed WAP32 base-class layouts** (`CGameMgr` etc. — vtable + offsets), and
3. the **`@address` map** (source ↔ binary addresses for build 1.0.1.77) as a partial symbol seed.
Cloned to `refs/tomalla-gruntz/` for reference.

---

## 13. Binary & reference inventory on disk (`binaries/`, `refs/`)
```
binaries/retail_en/GRUNTZ.EXE     Gruntz EN v1.0   (PRIMARY decomp target)
binaries/patch_101/Grnt_101.exe   v1.01 RTPatch diff (apply -> 1.0.1.77, tomalla's target)
binaries/claw_retail/CLAW.EXE     Claw EN v1.2     (cross-diff anchor; OpenClaw oracle available)
binaries/getmed_retail/MEDIEVAL.EXE Get Medieval    (cross-diff 3rd point)
binaries/getmed_demo/gmdemins.exe Get Medieval demo installer (InstallShield; needs unshield)
refs/tomalla-gruntz/              cloned RE-recreation repo (reference/seed)
```
**Editors still to pull when wanted** (each a WAP32 binary, may aid format/engine RE):
`GMEDIT.EXE` (Get Medieval editor, in the same GM portable 7z), GruntzEd/GLE (GooRoo/ModDB),
Claw's `CLAWADV.EXE`. **Assets:** `GRUNTZ.REZ` (77 MB) + `MEDIEVAL.REZ` for libwap32 wiring.

**Internet Archive in-container trick (works for ISO/ZIP/7z):** append the inner path to the
archive URL, `%2F`-encode separators, follow the 302 — e.g.
`curl -L https://archive.org/download/full-and-portable-get-medieval/Get%20Medieval%20%5BFull%20and%20Portable%5D.7z/Get%20Medieval%2FMEDIEVAL.EXE`.
(Nested containers — e.g. a `.bin` inside a `.zip` — are NOT traversed; the GM-Europe item is such a case.)

**Engine-RE archive.org slugs:** Claw retail `captain_claw_cd` (v1.2 EN), Claw demo `CaptainClaw`,
Get Medieval EU `GetMedievalEurope` (BIN/CUE-in-zip), GM portable `full-and-portable-get-medieval`,
GM demo `gmdemins`. **Communities:** GooRoo's forum (active hub), Gruntz Discord (backup invite
`discord.gg/SefJt3cu`), captainclaw.net, datashenanigans.pl.

---

## 14. Matching-decomp pipeline: delink → fake-PDB → objdiff (the chosen method)

Target workflow (per user): split the EXE into per-symbol COFF objects, then iterate hand-written
C++ until each function byte-matches the original, verified with **objdiff**.

```
GRUNTZ.EXE (.reloc present!)
  │
  ├─[Ghidra] auto-analyze + apply RTTI (Class Informer) + FLIRT (static CRT/MFC) + import leaked
  │          C:\Proj\ paths/names  →  comprehensive function-start inventory (names + lengths)
  │
  ├─[fake PDB] wandel/pdbgen (Ghidra-native)  →  Gruntz.pdb (Public + Procedure symbols, types)
  │            ⚠ first patch a CodeView/RSDS debug-dir entry into the PE (GUID = the PDB's);
  │              GRUNTZ.EXE currently has NO debug directory (rva0/size0).
  │
  ├─[delink] vostok-delinker --exe-path GRUNTZ.EXE --pdb-path Gruntz.pdb \
  │            --engine-path 'c:\proj\' --output-path target/ [--write-symbol-map map.txt]
  │          →  COFF .obj per symbol group ("target" objects for objdiff)
  │
  └─[match]  write C++ → compile with the ORIGINAL MSVC 5.0 (under wine) → "base" .obj
             objdiff (x86/COFF)  target.obj ⟷ base.obj  → per-function % match; iterate to 100%
             (engine functions cross-validated 3-way vs CLAW/MEDIEVAL + against OpenClaw semantics)
```

### Why this is feasible for Gruntz specifically (verified against the delinker source)
- **objdiff supports x86 with COFF objects** — exactly our toolchain (MSVC x86). ✓
- **vostok-delinker** (Rust, `flake.nix`, actively maintained — pushed 2026-06-13): parses the EXE as
  `PeFile32` (32-bit ✓), **requires `.text`/`.rdata`/`.data`** (Gruntz ✓) and **hard-requires a
  `.reloc` section** — it recovers relocations by walking the PE base-reloc table (HIGHLOW), reading
  each absolute target and rewriting it to *nearest-named-symbol + offset*.
  - ⇒ **GRUNTZ.EXE (has `.reloc`) is delinkable; CLAW.EXE / MEDIEVAL.EXE (reloc stripped) make the
    tool `bail!`.** So **Gruntz is the only delink target**; the siblings stay as read-only
    cross-diff/oracle references (§11–12).
- **PDB needs (from `pdb_symbols.rs`):** global **Public** symbols (mangled name + `function` flag +
  section:offset), per-module **Procedure** symbols (these carry function **lengths**), **Data**
  symbols (statics/constants), the **string table**. **It does NOT use section contributions** →
  pdbgen's symbol-only PDB is structurally adequate (pdbgen's missing section-contribs are a non-issue).

### Requirements / gaps to close (in priority order)
1. **Comprehensive function coverage.** `relocs.rs` does `.expect("all function relocs to be named")`
   — it PANICS if an absolute `.text` reloc target isn't a known function start. ⇒ the fake PDB must
   name a function at/under every absolute-reloc target. Seed coverage from: Ghidra auto-analysis,
   the **`.reloc` targets themselves** (vtable/function-pointer entries point at function starts),
   RTTI vtables, and FLIRT. Plan a coverage-audit pass before delinking.
2. **Procedure symbols with correct lengths.** Function sizes come from `S_PROC` records in module
   streams (used to slice each function's bytes + a stub-rename heuristic). Confirm pdbgen emits
   procedures-with-lengths (not just length-less publics); if not, post-process or use the
   `llvm-pdbutil yaml2pdb` route (we have `llvm-pdbutil`) to synthesize a PDB with proper `S_GPROC32`
   lengths from Ghidra's function table.
3. **Patch a CodeView debug entry into GRUNTZ.EXE** (pdbgen prerequisite) — add an
   `IMAGE_DEBUG_DIRECTORY` of type CODEVIEW (RSDS GUID = generated PDB's). Small scripted PE edit.
4. **Exact original compiler for the recompile half.** To reach byte-identical codegen, compile with
   the same **MSVC 5.0** that built it (optional-header linker 5.10; Rich shows a linker module build **8034** + cvtres 1668 (= VC5 SP3)). Source that exact `cl.exe`/`link.exe` and run under **wine**. This is the
   long pole — without the matching compiler, objdiff can confirm *logic* but not 100% byte-match.
5. **Minor delinker forking** likely: `--engine-path` scoping is Survarium-shaped (set to `c:\proj\`),
   and a couple of hardcoded stub byte-patterns are harmless. Fork is trivial (Rust); upstream is live.

### Local tooling status (this machine)
`ghidra` ✓ · `llvm-pdbutil` ✓ (fallback fake-PDB synth + inspection) · `wine`/`wine64` ✓ (run MSVC) ·
`nix`/`nix-shell` ✓ (build the delinker via its flake; pull `cargo`/`clang`/`cv2pdb`/`cmake`/`objdiff`).
Missing but nix-available: `cargo`/`rustc`, `cv2pdb`, `clang`, `cmake`, `objdiff` (CLI + GUI).
Fake-PDB tool choice: **pdbgen** (Ghidra-native). *FakePDB is IDA-only in practice (Ghidra = TODO)* —
not usable here unless an IDA DB is introduced.

### Concrete next actions for this pipeline
- [ ] `nix`-build vostok-delinker from its flake; dry-run `--help` to confirm CLI.
- [ ] Install pdbgen into Ghidra; confirm its PDB emits Procedure symbols w/ lengths + `function` flag.
- [ ] Script the CodeView debug-dir patch for GRUNTZ.EXE.
- [ ] Ghidra headless analyze GRUNTZ.EXE (+RTTI +FLIRT +leaked names); export function coverage; audit
      against `.reloc` `.text` targets to satisfy the delinker's "all named" assert.
- [ ] First delink → inspect target `.obj`s in objdiff; stand up an objdiff project (target vs base).
- [ ] Source the exact MSVC 5.0 (VC5 SP3 toolchain) for wine; pick a small leaf function (e.g. a `z`-lib
      routine) as the first match target.

---

## 15. Build environment — reusing the `vostok` (X-Ray 2.0) harness via a Nix flake

The reference harness is **`srp-survarium/vostok`** (private; readable via this machine's `gh` token,
account `sushi-shi`). Branch **`feature/agentic-matching-loop-2`** is a live, Claude-driven *agentic
matching loop* for the Vostok/X-Ray 2.0 engine: delinker → objdiff → `match.db` → README score
(~51% fuzzy / 34% exact at clone time). Cloned to `refs/vostok/` (gitignored). Its whole stack is
Nix-provided, and the same mechanism transfers to Gruntz — only the compiler payload differs.

### What we reuse verbatim
- **`vostok-delinker`** — engine-agnostic (`PeFile32` + PDB); used unchanged.
- **objdiff / objdiff-cli** — upstream prebuilt Linux binaries, autoPatchelf'd (objdiff does **x86 +
  COFF** = our case). Derivations lifted from vostok's flake (v3.7.1, same hashes).
- **The flake `vostok-toolchain` pattern** — `fetchurl` a prebuilt compiler tarball from a GitHub
  release, unpack to `msvc/`+`winsdk/`+`dxsdk/`+`ninja/`, export via env vars, run under
  **wine-staging**, configure PATH/INCLUDE/LIB in the Wine registry (`setup-toolchain.py`).
- **The agentic-loop scripts** as templates: `scripts/{rebuild,generate_delink,generate_objdiff_config,
  match_db,match_score}.py`, `docs/binary_matching/*` (esp. `match_db_design.md`, `agentic_loop.md`,
  `MATCHING.md`), and the `.claude/` orchestration.

### What we adapt (VS2008 → MSVC 5.0)
- Gruntz is **MSVC 5.0** (PE linker 5.10; Rich linker build **8034**, cvtres 1668 (= VC5 SP3)), not VS2008. So the
  VS2008-specific machinery does **not** apply: no SP1 `PATCH=`, no `crtassem.h` manifest overlay, and
  likely no `mspdbsrv`/C1902 saga.
- **VC++ 5.0 media is InstallShield/CAB**, not an MSI admin-install → package by `7z`-extracting
  `cl.exe`/`c1.exe`/`c2.exe`/`link.exe` + `mspdb*.dll` + `include/` + `lib/` (no `msiexec /a`).
- Add a **DirectX 6 SDK** (`ddraw`/`dinput`/`dsound`/`dplay` headers + import libs) and the
  **static MFC** + **Miles (`mss32`)/Smacker (`smackw32`)** import libs Gruntz links (vostok's
  `vostok-libs` analog).
- The PDB is **synthesised** (Ghidra + pdbgen, §14) — Survarium *shipped* `survarium.pdb`; Gruntz did
  not, which is the one place our pipeline does more work than theirs.

### `flake.nix` (written, parses OK)
Two shells, inputs pinned to vostok's locked revs (so the delinker `cargoHash` reproduces):
- **`nix develop`** (default) — **works today, no MSVC**: `vostok-delinker`, `objdiff`/`objdiff-cli`,
  `ghidra`, `llvm-pdbutil`, python/rg/file/xxd/jq/clang-tools. Covers analysis + **target-side delink**
  + objdiff. `GRUNTZ_EXE` points at the IA-fetched binary (`gruntz-exe`, pinned hash).
- **`nix develop .#build`** — adds `gruntz-toolchain` (MSVC 5.0) under `wineWow64Packages.staging` and
  exports `MSVC_DIR`/`DXSDK_DIR`/`NINJA_DIR`/`WINEPREFIX`. For the **base/recompile** side.

### The one remaining step to "get MSVC"
Produce **`gruntz-toolchain-vc50.tar.xz`** once and publish it (a `gruntz-build-env` release), then fill
its `url`+`sha256` in the flake (`nix build .#gruntz-toolchain` prints the real hash). Recipe (adapt
`scripts/create-toolchain-release.py`): source VC++ 5.0 **SP3** (the cvtres-1668 pin)
from archive.org/WinWorld → `7z` extract the x86 `bin`/`include`/`lib` → add DX6 SDK + ninja.exe +
static-MFC/Miles/Smacker libs → reproducible `tar -cJf`. Until then the default shell already supports
everything except the recompile half.

---

## 16. Regional releases & the two difference axes  [RESEARCH + MEASURED]

**Bottom line: localization is a pure data swap — the EXE is the *same binary* across languages.**
The only EXE differences are along the *version* axis (v1.0 vs the v1.01 patch), not the language axis.

### Release matrix

| Language / edition | Publisher | Official? | Year | Version | archive.org id | EXE vs EN |
|---|---|---|---|---|---|---|
| English (NA) | Monolith / GT Interactive | yes | 1999 | v1.0 (→1.01) | `gruntz-pc`, `gruntziso` | baseline (`81c7f648…`) |
| English (RealArcade) | Monolith / RealArcade | yes | ~1999+ | "1.10" (pre-patched) | — | = EN v1.01 |
| French | Microids | yes | 1999 | v1.0 | `Gruntz` (CloneCD img) | = EN |
| **Italian** | Microids | yes | 1999 | v1.0 | `gruntz_201912` (`Gruntz.iso`) | **= EN (byte-verified)** |
| German | CDV Software | yes | 2000 | v1.0 | (boxed; not on IA) | = EN |
| Spanish | (unconfirmed) | yes | ~1999 | v1.0 | (not located) | = EN (inferred) |
| Polish | (PL distrib. / cover-discs) | yes | 1999 | v1.0 | `gruntz-pl`, `gruntz-pl_202411` | = EN (inferred) |
| Czech | Gamestar 15 cover-disc (CD2) | yes | 1999/2000 | v1.0 | `czgamestar15cd` | = EN (inferred) |
| **Russian** | **Russobit-M / GFI** | yes | **2003** | v1.0-based | old-games.ru id 757 (not IA) | likely = EN base + RU assets (unverified) |

No Hungarian release found; **no pirate (Fargus/7Wolf/etc.) Russian** found — the lone Russian is the
official Russobit-M *«Месть карапузиков»* ("Revenge of the Gruntz"), a **full** localization (Russian
UI **and** voice), 2003.

### Evidence
- **MEASURED:** the Italian retail `GRUNTZ.EXE` (`gruntz_201912/Gruntz.iso → GAME/GRUNTZ.EXE`) is
  **byte-for-byte identical** to the English retail EXE (MD5 `81c7f648db99501bed6e1ee71e66e4a0`,
  2,511,872 B). Saved at `binaries/italian/GRUNTZ.EXE`.
- **RESEARCH (decisive):** the official v1.01 patch readme states it is *"English language only"* and
  warns *"applying this patch will change German, Spanish, etc. into English text"* — which only works
  if every locale runs the same engine + data format. A German retail buyer on GooRoo's forum applied
  the English v1.01 patch successfully (version → 1.01, voices intact). The version string lives in a
  **data file** (`GAME\VERSION\VERSION.TXT` inside `GRUNTZ.REZ`), not the EXE; the patch payload
  (`Gruntz.ZZZ`) is corrected `.WWD` level data.

### The two axes (what this means for the decomp)
- **Language axis → EXE identical.** Translation is entirely in `GRUNTZ.REZ` (+ localized voice assets,
  e.g. the Russian dub). One executable serves EN/FR/IT/DE/ES/PL/CZ. ⇒ **match once, covers every
  language.**
- **Version axis → EXE differs (slightly).** v1.0 (retail, what we hold: `81c7f648…`) vs **v1.0.1.77**
  (the v1.01 RTPatch binary-diff of `GRUNTZ.EXE` + corrected levels; RealArcade ships it pre-applied as
  "1.10"). The `tomalla` recreation targets **1.0.1.77**.
- **DECISION (locked): target v1.0** — `binaries/retail_en/GRUNTZ.EXE`, MD5 `81c7f648…`, the clean
  unpatched build that every localized retail shipped. Rationale: it's what all languages share, and it
  **definitely has `.reloc`** (the delinker's hard requirement), so the pipeline isn't blocked on the
  patched build. Revisit later if we want to align with tomalla's 1.0.1.77 `@address` map.
- `.reloc` in v1.0.1.77 left **unverified** (RTPatch GUI applier no-ops on a loose/checksum-mismatched
  source; not worth fighting). Reasoned yes — same MSVC 5.0 build line; a maintenance patch won't flip
  `/FIXED` or strip the 113 KB `.reloc`. Moot under the v1.0 decision.
- The Russian 2003 repack is the only edition whose EXE byte-identity to EN is unverified — byte-check
  it only if we ever target the Russian build specifically.

---

## 17. String-mining findings — decomp scaffolding  [MEASURED]

Full strings dumped (`GRUNTZ.strings.{ascii,utf16,all}.txt`) and mined into
`binaries/retail_en/STRINGS_ANALYSIS.md`. Highlights that shape the decomp:

- **Precise version: `1.0.0.76`** (VERSION resource) — i.e. our `81c7f648…` *is* the retail v1.0; the
  v1.01 patch takes it to `1.0.1.77`.
- **RTTI lives in `.data` (0x608000+), not `.rdata`.** Note for the Ghidra RTTI pass and the delinker
  (its `.data` symbol handling must cover the RTTI/vftable region).
- **Manager class ↔ TU map** (joins the leaked `.cpp` paths to their primary class):
  `DDRAWMGR.CPP→CDirectDrawMgr` (+`DIRPAL`/`DIRSURF`), `DSNDMGR.CPP→DirectSoundMgr` (+`DSndMgSR`,
  `SFManager`/`SFMAN32` SoundFont), `DinMgr2.cpp→DirectInputMgr2` (+`InputDevice`),
  `NetMgr.cpp→CNetMgr` (DirectPlay), `GruntzMgr.cpp→CGruntzMgr`. Also `RezSync`/`CRezDir` (REZ/VRZ
  loader) and **`ButeMgr`** (config via `attributez.txt`/`dwrects.txt` — a Monolith class reused from
  the LithTech lineage). Extra imports seen: `CTL3D32`, `COMCTL32`, `SFMAN32`.
- **`CGrunt` field layout leaked** verbatim via a debug-dump format string
  (`[p][g][health][x][y][dir][stm][ttl][tool][toy]…`) — a direct struct-layout seed.
- **Puzzle-logic system:** `CUserLogic`/`CUserBase` + a large `CTile*Trigger*Logic` family; the
  `zDArray<int (CUserLogic::*)()>` template is the **logic dispatch table** (array of member-fn ptrs).
- **Game taxonomy** (drives data tables): 36 grunt types, 22 Toolz, 10 Toyz, 4 Warlordz, 5 Powerupz,
  ~17 color tints, 8 directions, `STATEZ_*` states.
- **Command-line modes (full set):** `PLAY MULTI DEMO ATTRACT SELECT EDIT HOST JOIN LOAD: LOADGAME
  NOLOGO NOMOVIES LOBBYLAUNCH QUICKSTART` — note **`EDIT`** (the level editor is likely a mode of
  GRUNTZ.EXE itself, not just the separate GLE) and `HOST/JOIN/LOBBYLAUNCH` (DirectPlay MP).
- **Asset pipeline:** `Gruntz.REZ` + `GRUNTZ.VRZ` (sorted REZ dir), CD-probe `%c:\{GAME,DATA,MOVIEZ,
  MUSIC}\…`, `WORLDZ\LEVEL%i` / `TRAINING%i`, `*.WWD` worlds, `*.FEC` front-end config, `*.fnt` fonts,
  `Gruntz.SF2`/`Gruntz4.SF2`, `VERSION.TXT`, `CHEATZ.TXT`. Assert reporter format: `%s, line %i: %s (%i) - %s`.
  Full `DDERR_/DIERR_/DSERR_/DPERR_` stringify tables embedded.

### Leftover / non-shipping code compiled into RETAIL (account for these in the decomp)
- **Pre-release unlock-key + expiry + registration-ping** path is still present (string IDs 0x800c–f,
  "Enter your key below to unlock Gruntz", `Unlock`/`UnlockFile`, URL ping
  `Name=%s&Type=%i&Location=%s&Version=%lu&Checksum=%lu`).
- **Internal-build markers:** "Gruntz Pre-Release", "Monolith internal review copy. Do not distribute.
  We know who you are.", "Alpha Version, Build %i".
- **Debug overlay/profiler + cheat engine:** `Fps=`, `Delta/Update/Draw`, `DEBUG_GRUNTTYPE/JUMPLEVEL/
  POSITION/SETSKILL`, `DPRINTF`; `Cheatz`/`NumCheatz`, WARP letterz, gruntzgoo.com "make no commentz",
  "Brian L. Goble is a programming God", Kevin Lambert cheat.
- Window class `GruntzClass`; Smacker "Smacker Video Window"; DirectPlay GUIDs are binary (no plaintext).
