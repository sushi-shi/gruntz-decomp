# Gruntz — C++ "structure" scaffold (RE-derived)

This directory is a **reverse-engineering work product**: an organized set of C++
header/skeleton files capturing *everything we have recovered so far* about the
class model, field layouts, enums, and source-tree map of **Gruntz** (Monolith
Productions, 1999; WAP32 engine; built with MSVC 5.0).

It is **NOT** copyrighted binary content. Every file here is our own scaffolding,
derived from:
- **RTTI**: the 231 mangled `.?A[VU]…@@` `type_info` names that survive in the
  retail `GRUNTZ.EXE` (`81c7f648…`, v1.0.0.76). These give class *names* and a few
  template instantiations for free. RTTI lives in `.data` (0x608000+) on MSVC5.
- **Leaked source paths**: 12 `C:\Proj\…` `.cpp/.h` paths in assert strings (see
  `GRUNTZ.srcpaths.txt`) → the module/file boundaries.
- **Mined strings**: `STRINGS_ANALYSIS.md` — the registry value-names, the game
  taxonomies (grunt types, Toolz, Toyz, Warlordz, Powerupz, tints, directions,
  states), the `CGrunt` debug-dump field order, CLI launch tokens, asset paths,
  manager-class↔TU map.
- **tomalla** (`refs/tomalla-gruntz/`): an existing partial RE source-recreation
  (targets the *patched* 1.0.1.77 build). We **harvest** its reconstructed
  `CGameMgr/CGruntzMgr/CGruntzWnd/CNetMgr` field+vtable layouts. All ported layouts
  carry an attribution comment. **Note**: tomalla's `@address` values are for build
  **1.0.1.77**, NOT our target v1.0.0.76 — treat them as *approximate* / re-verify
  before use in the v1.0 matching pipeline.

These headers are now **compilable placeholder C++** (the same style as `src/**`):
real bases + `virtual`, `int`/`void*`/`char[]` placeholder members, and explicit
`char _padNN[K]` for unknown gaps. There are **no `@offset`/`@size`/`@vftable`
annotations** — a class's *compilable declaration is its layout*, and
`scripts/ghidra_metadata_generate.py` derives every field offset by running clang
`-fdump-record-layouts` over `src/**` ∪ `structure/**` (with `src/` winning any
overlap). Each header parses standalone under the MSVC-5.0 clang target
(`--target=i686-pc-windows-msvc -fms-extensions -fms-compatibility-version=1100`).

## What this is for

A coherent set of stubs that an editor / clangd / the future matching-decompilation
work can build on. Classes (even empty), known field layouts, enums, the source-tree
map, function names + addresses where we have them. Many small themed headers rather
than one giant file.

## Source-tree mirror

The leaked tree is `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,Gruntz,incs}`. We mirror
it as:

```
structure/
  wap32/      engine substrate: CGameMgr + the z-runtime
              (zDArray, zPtrColl, zPTree, zErrHandling, _zvec/_zdvec, CWapObj/CWapX)
  managers/   one header per leaked manager TU:
              CDirectDrawMgr (DDRAWMGR.CPP), DirectSoundMgr (DSNDMGR.CPP),
              DirectInputMgr2 (DinMgr2.cpp), CNetMgr (NetMgr.cpp),
              ButeMgr (attributez.txt config),
              ddrawmgr_surface_family.h (HYPOTHESIZED DDrawMgr surface/page family)
  game/       the Gruntz game layer (C:\Proj\Gruntz):
              CGruntzMgr, CGruntzWnd, CGrunt, the CUserLogic dispatch system, the
              trigger family, and the long tail of game classes grouped by theme.
  utils/      reconstructed util classes (tomalla-derived, @approx 1.0.1.77):
              Utils::MemoryPool<T>, Font (+ .fnt format)
  formats/    version-independent on-disk data formats (shared editor<->game):
              wwd_object.h (WWD world-object record + flag enums),
              wwd.h (WWD plane / PID image headers), rez.h (RezDirEntry)
  enums.h     game taxonomy enums (GruntType, Tool, Toy, …, Statez, LaunchMode)
              + LaunchModeCode, Resolution, Commands, GruntzVolumeAttenuation
  registry.h  the registry value-names as a config struct
  INDEX.md    table of every one of the 231 RTTI classes
```

**Graduate-on-match:** once a class is byte-matched it moves into `src/<Module>/`
and its `structure/` header is deleted. Already graduated (no longer here):
`CGameApp`/`CGameWnd`/`GameInfo` (→ `src/Wap32/Wap32.h`), `CGruntzApp`
(→ `src/Gruntz/GruntzApp.cpp`), `Utils::RegistryHelper` (→ `src/Utils/`),
`CRezDir`/`CRezItm`/`CRezItmBase` (→ `src/Rez/RezMgr.h`), `WwdHeader`
(→ `src/Wwd/WwdFile.h`), and the `CStatusBarItem` / `CTileTriggerLogic` stubs
(→ `src/Gruntz/`). `ghidra_metadata_generate.py` reads those from `src/`.

(See also `docs/editor-notes.md` for the editor-only MFC classes that are
deliberately NOT modeled here, and the editor<->game data-model evidence.)

## Annotation conventions

Layout carries **no annotation** — the compilable declaration *is* the layout
(clang derives every field offset). Only these comment tags remain:

| Tag | Meaning |
|---|---|
| `//@address: 004xxxxx` | absolute VA of a function / static (tomalla = 1.0.1.77) |
| `//@rtti` | the verbatim mangled RTTI name (added by us, for traceability) |
| `//@approx tomalla 1.0.1.77` | layout ported from tomalla's patched-build reading |

`@offset`/`@size`/`@vftable`/`@todo`/`/* unknown */` are **gone**: a field at a
known offset is a real placeholder member (`int`/`void*`/its real type), an unknown
gap is an explicit `char _padNN[K]`, and an object's size is `sizeof`. Each stubbed
class still keeps its **verbatim RTTI mangled name** in a comment
(`// .?AVCXxx@@`) so it is always traceable back to the binary.

## Confidence policy

**Only encode offsets/types we actually know.** An unknown slot is a right-sized
placeholder; an unknown gap is padding; nothing wider/more-aligned than the
evidence supports is used. The honest tiers are:

1. **Field + vtable layout** (high confidence, ported from tomalla, marked
   `@approx`): `CGameMgr`, `CGruntzMgr`, `CGruntzWnd`, `CNetMgr` (vptr only),
   `Utils::MemoryPool<T>`, `Font`, the `UnknownClassArrays` /
   `UnknownClassInCGruntzMgr` nested types, and the `ddrawmgr_surface_family.h`
   hierarchy (offsets/sizes/inheritance high confidence; class IDENTITY =
   HYPOTHESIS, names = tomalla placeholders). Also the version-independent on-disk
   formats `WwdObjectRecord` (PINNED 0x11C), `WwdPlaneHeader`, `PidHeader`, and the
   field-set-known `WwdObject` / `RezDirEntry` (flat sequential reference views,
   NOT byte-layout sources).
2. **Field *order* known, types/offsets NOT** (`CGrunt`, from the debug-dump
   string): left as a name-only stub with the recovered token roster in a comment
   — no fake sequential layout is invented.
3. **Name-only** (the bulk): we know the class exists and (often) its rough role
   from naming + strings, but no layout. Represented as empty
   `class CXxx { /* .?AVCXxx@@ */ };`.

`INDEX.md` records, per class, the scaffold file and knowledge tier.

## Coding style

Clean, consistent **C++98-ish** (MSVC 5.0 target — no C++11 in the real build).
Headers are include-guarded. The WAP32 engine base lives in `namespace WAP32`
(matching tomalla); game classes are global, MFC-style `C`-prefixed.
