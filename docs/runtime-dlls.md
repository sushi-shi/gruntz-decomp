# Gruntz runtime DLLs

The matching decompilation produces a rebuilt `GRUNTZ.EXE`. To **run** that EXE
under Wine it needs the same proprietary third-party DLLs the retail game shipped
(or LoadLibrary'd). This document records each one: what it is, where it comes
from, its hash/size, whether it is required vs optional, and - the key point -
that **none of them are needed at build/link time**.

Mechanism mirrors vostok's `vostok-libs`: a `gruntz-runtime` derivation
(a `runCommand`) assembles the DLLs into one out dir, exposed in the `.#build`
devShell as `$GRUNTZ_RUNTIME`. The DLLs that *do* ship on the retail CD are
pulled straight out of the Internet Archive CD image using the same
`/download/<id>/<iso>/<inner-path>` 302 trick as `gruntz-exe` (the inner path is
`%2F`-encoded; `fetchurl` follows the redirect).

Date: 2026-06-13. Item: `gruntz-pc`, `Gruntz.iso` (English retail v1.0).

---

## Build-time vs run-time (the framing that matters)

**NONE of the DLLs below are needed to build or link the rebuilt EXE.**

| DLL | how the EXE binds it | link-time dependency? |
|---|---|---|
| `MSS32.DLL` | load-time import (named imports `AIL_*`) | satisfied by an **import lib** we synthesise from the DLL (or take from the MSS SDK). No DLL needed at link; the `.lib` is enough. |
| `SMACKW32.DLL` | load-time import (`_Smack*@n`) | same - an import lib (from the DLL or SDK) satisfies the linker; the DLL itself is runtime-only. |
| `SFMAN32.DLL` | **`LoadLibraryA` at runtime** (resolves the `SFManager` export by name) | **none at all** - dynamic load, no import-table entry, no `.lib`. |
| `CTL3D32.DLL` | **`LoadLibraryA` at runtime** (pulled in by static MFC's `_AFX_CTL3D_*`) | **none at all** - dynamic load. |

The things that must **byte-match** the retail binary (the static multithreaded
CRT `LIBCMT.LIB`, static MFC 4.2 `NAFXCW.LIB`, and zlib) are linked in from the
**VC5 SP3 toolchain + zlib** (see `docs/toolchain-vc50-sp3.md` and
`docs/libraries-and-funcid.md`) - **NOT** from anything in this document. The
runtime DLLs never contribute bytes to the EXE.

So: `gruntz-runtime` is for *running* the result, not for the matching build.

---

## The DLLs

Verified against `GRUNTZ.EXE`'s own strings - the imported/LoadLibrary'd DLL
names are: `mss32.dll`, `smackw32.dll`, `SFMAN32.DLL` (+ the `SFManager`
LoadLibrary export), `CTL3D32.DLL`, plus the DirectX-6 imports
`DDRAW/DINPUT/DSOUND/DPLAYX/DSOUND` and the usual Win32 system DLLs.

| DLL | vendor / purpose | source | sha256 (SRI) / size | required vs optional | build-time needed? |
|---|---|---|---|---|---|
| **MSS32.DLL** | RAD Game Tools - **Miles Sound System v4.0g** (© 1991-98). Digital audio + XMI/MIDI + the `AIL_DLS_*` software wave synthesizer. | `gruntz-pc` / `Gruntz.iso` -> `GAME/MSS32.DLL` (loose file on CD) | `sha256-rM/BX6WSTF3cwhAl81r5COTn7XV2tmSrVNcTfkUyPnU=` / **269,312 B** | **Required** (no sound without it; an audio path the game always tries) | **NO** (import lib only) |
| **SMACKW32.DLL** | RAD Game Tools - **Smacker** video codec (© 1994-97). Intro/cutscene playback (`SmackOpen`/`SmackDoFrame`/`_SmackWait@4`...). | `gruntz-pc` / `Gruntz.iso` -> `GAME/SMACKW32.DLL` (loose file on CD) | `sha256-+bL9tevI5lnHrBMsIT/P0usFmhGVoSkSG7aMohaZ5eE=` / **96,256 B** | **Required-ish** (intro/cutscenes; game may limp without it but expects it) | **NO** (import lib only) |
| **SFMAN32.DLL** | RAD/Miles - **AIL SoundFont Manager** (the `SFManager` API; loads `Gruntz.SF2` / `Gruntz4.SF2` banks). The MSS-4.x SoundFont companion to mss32. | **NOT on the CD** - external source (see below). Currently a `fakeHash` placeholder. | *(unpinned)* | **Optional** - `LoadLibrary`'d; absence only disables SoundFont music, the EXE still links and runs | **NO** (LoadLibrary - no link dep) |
| **CTL3D32.DLL** | Microsoft - 3D dialog controls redistributable. Pulled in by static MFC (`_AFX_CTL3D_STATE`/`_AFX_CTL3D_THREAD`) and `LoadLibrary`'d. | MS redistributable / ships with old Windows (see below). Not packaged here. | *(not packaged)* | **Optional, low priority** - cosmetic dialog theming; on Wine the builtin `ctl3d32` typically satisfies it | **NO** (LoadLibrary - no link dep) |
| *(GRUNTZ.REZ)* | Monolith - packed asset archive (graphics/sound/levels). Not a DLL, but needed to actually *run* the game. | `gruntz-pc` / `Gruntz.iso` -> `DATA/GRUNTZ.REZ` | *(unpinned - `fakeHash`)* / **77,253,149 B (~74 MiB)** | **Required to run** (but huge; left unpinned to avoid the big fetch) | **NO** |

### How the hashes/sizes were obtained

`MSS32.DLL` and `SMACKW32.DLL` were **actually fetched** and pinned:

```
nix-prefetch-url --type sha256 --name MSS32.DLL \
  'https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FMSS32.DLL'
#   -> 0x9y692pw4ypajmn9dknfpnygr08z5dg698hqbf5sk4jlmgw3kxc
#   -> nix hash convert: sha256-rM/BX6WSTF3cwhAl81r5COTn7XV2tmSrVNcTfkUyPnU= (269,312 B)

nix-prefetch-url --type sha256 --name SMACKW32.DLL \
  'https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FSMACKW32.DLL'
#   -> sha256-+bL9tevI5lnHrBMsIT/P0usFmhGVoSkSG7aMohaZ5eE= (96,256 B)
```

(`--name` is required: without it `nix-prefetch-url` derives a store-path name
from the URL, and the `%2F` in the in-ISO path is an illegal store-path char.)

`gruntz-runtime` was built and confirmed to contain both DLLs byte-for-byte:
`nix build .#gruntz-runtime`.

`GRUNTZ.REZ`'s size came from the HTTP `Content-Length` of its in-ISO URL
(no download); its sha256 is left as `fakeHash` - pin it with
`nix-prefetch-url --name GRUNTZ.REZ <url>` when you want the assets.

---

## SFMAN32.DLL - the externally-sourced one

`GRUNTZ.EXE` `LoadLibraryA`s **`SFMAN32.DLL`** and resolves the **`SFManager`**
export to play its SoundFont banks (`Gruntz.SF2` / `Gruntz4.SF2`). This is the
**RAD/Miles AIL "SoundFont Manager"** - the SoundFont companion to the bundled
**MSS v4.0g** (`MSS32.DLL`, © 1991-98). It is **OPTIONAL**: because it is
LoadLibrary'd (not an import-table entry), its absence simply disables SoundFont
music and the EXE still links and runs.

### Confirmed absent from the retail CD

Every loose file on `Gruntz.iso` was checked, and the in-ISO probe confirms it:
fetching `…/Gruntz.iso/GAME%2FSFMAN32.DLL` returns a **0-byte body** (IA returns
an empty 200 for a non-existent inner path, vs the real 269,312 B for
`GAME%2FMSS32.DLL`). So SFMAN32 must be sourced externally.

### Search results (no clean standalone Miles `sfman32.dll` found)

- **archive.org full-text/file search for `sfman32`: 0 hits.** No item exposes a
  loose `sfman32.dll` (IA does not index files *inside* ISOs/zips, and no
  standalone item exists).
- **The freely-downloadable `sfman32.dll` is the WRONG vendor.** dll-files.com /
  dll4free / originaldll all host **Creative Technology Ltd**'s *"SoundFont Master
  Manager"* (versions `4.6.0.501` ~52 KB and `6.0.230.11` ~10 KB). That is the
  Creative/EMU SoundFont API - a different DLL that happens to share the name. It
  is **NOT** the RAD/Miles `SFManager` the game expects. **Do not use it.**
- **Period-correct Miles SDK on archive.org is too old / not loose files.** The
  only Miles items are `Lowe_MilesSoundSystemSDK{1,3}of3_1996.10.22` - MSS **3.x**
  (1996) floppy *disk images* (1.44 MB `.img`/`.zip`), pre-DLS-synth era,
  predating the 4.x SoundFont manager, and not extractable as a loose DLL via the
  in-ISO URL trick.
- **MSS v4 = February 1998** (RAD history / Wikipedia), matching the `4.0g`,
  © 1991-98 stamp in our `MSS32.DLL`. So the right donor is a **contemporaneous
  MSS-4.x SDK / game redistributable** that bundled `sfman32.dll`.

### Best candidate (not yet fetched/pinned)

- **`lithengine`** on archive.org - *"LithTech Jupiter"* SDK ISO (`LithEngine.iso`,
  ~636 MB), the Monolith engine SDK. Monolith made Gruntz and later LithTech, so
  this ISO very likely bundles the MSS redistributable including `sfman32.dll`.
  Blocker: IA does not enumerate inner files for this item and brute-forced
  in-ISO paths all returned 0 bytes, so confirming it needs a ~636 MB download.
- Failing that, any **other MSS-4.x-era game** (or the **MSS 4.x SDK**, if a copy
  surfaces) that ships `sfman32.dll` alongside an MSS-4.x `mss32.dll`.

### Current flake state

`gruntz-sfman32` is wired as a `fetchurl` with a **`pkgs.lib.fakeHash`**
placeholder and a `PLACEHOLDER-mss4-sdk` candidate URL. A
`gruntz-sfman32-available` flag (`sha256 != fakeHash`) keeps the placeholder out
of the build graph, so `gruntz-runtime` builds today with just MSS32 + SMACKW32.
To enable SoundFont music: locate a genuine Miles `sfman32.dll`, set
`gruntz-sfman32-src.{url,sha256}` in `flake.nix`, and it is copied into
`$GRUNTZ_RUNTIME` automatically.

---

## CTL3D32.DLL - low priority

`GRUNTZ.EXE` `LoadLibraryA`s `CTL3D32.DLL` because it statically links MFC 4.2,
whose `_AFX_CTL3D_STATE`/`_AFX_CTL3D_THREAD` machinery uses Microsoft's 3D
dialog-controls helper. It is **optional and cosmetic** (3D shading on old-style
dialogs) and creates **no build/link dependency** (dynamic load).

Where to get it (not packaged in the flake):

- It is a long-standing **freely redistributable** Microsoft DLL and ships with
  old Windows (`%WINDIR%\SYSTEM32\CTL3D32.DLL` on Win9x/NT4/2000-era installs) and
  in many MS SDKs/VC redists of the period.
- Under **Wine**, the builtin `ctl3d32` typically satisfies the `LoadLibrary`
  already, so no action is usually needed to run the rebuilt EXE.

Priority: low. Add a `gruntz-ctl3d32` fetchurl + wire into `gruntz-runtime` only
if a target machine lacks it and Wine's builtin proves insufficient.

---

## flake.nix additions (summary)

- `gruntz-mss32`, `gruntz-smackw32` - real, pinned `fetchurl`s from the in-ISO
  URLs.
- `gruntz-sfman32` - `fetchurl` with `fakeHash` placeholder (gated by
  `gruntz-sfman32-available`).
- `gruntz-rez` - `fetchurl` with `fakeHash` placeholder (large asset archive).
- `gruntz-runtime` - `runCommand` assembling `MSS32.DLL` + `SMACKW32.DLL` (and
  `SFMAN32.DLL` once pinned) into one out dir.
- All four added to `packages.${system}`.
- `.#build` devShell exports `GRUNTZ_RUNTIME="${gruntz-runtime}"`.

`nix-instantiate --parse flake.nix` passes; `nix build .#gruntz-runtime` builds;
`nix flake show` evaluates all outputs.
