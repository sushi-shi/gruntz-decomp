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
  `CGameApp/CGameMgr/CGameWnd/CGruntzMgr/CNetMgr` field+vtable layouts and its
  `@address`/`@offset`/`@vftable`/`@bug`/`@todo` annotation conventions.
  All ported layouts carry an attribution comment. **Note**: tomalla's `@address`
  values are for build **1.0.1.77**, NOT our target v1.0.0.76 — treat them as
  *approximate* / re-verify before use in the v1.0 matching pipeline.

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
  wap32/      engine substrate: CGameApp/CGameMgr/CGameWnd + the z-runtime
              (zDArray, zPtrColl, zPTree, zErrHandling, _zvec/_zdvec, CWapObj/CWapX)
  managers/   one header per leaked manager TU:
              CDirectDrawMgr (DDRAWMGR.CPP), DirectSoundMgr (DSNDMGR.CPP),
              DirectInputMgr2 (DinMgr2.cpp), CNetMgr (NetMgr.cpp),
              RezSync/CRezDir (REZ/VRZ loader), ButeMgr (attributez.txt config),
              ddrawmgr_surface_family.h (HYPOTHESIZED DDrawMgr surface/page family)
  game/       the Gruntz game layer (C:\Proj\Gruntz):
              CGruntzMgr, CGrunt, the CUserLogic dispatch system, the trigger
              family, and the long tail of game classes grouped by theme.
  utils/      reconstructed util classes (tomalla-derived, @approx 1.0.1.77):
              Utils::RegistryHelper, Utils::MemoryPool<T>, Font (+ .fnt format)
  formats/    version-independent on-disk data formats (shared editor<->game):
              wwd_object.h (WWD world-object record + flag enums),
              rez.h (REZ/VRZ "RezMgr" archive directory entry)
  enums.h     game taxonomy enums (GruntType, Tool, Toy, …, Statez, LaunchMode)
              + LaunchModeCode, Resolution, Commands, GruntzVolumeAttenuation
  registry.h  the registry value-names as a config struct
  INDEX.md    table of every one of the 231 RTTI classes
```

(See also `docs/editor-notes.md` for the editor-only MFC classes that are
deliberately NOT modeled here, and the editor<->game data-model evidence.)

## Annotation conventions (reused from tomalla)

| Tag | Meaning |
|---|---|
| `//@address: 004xxxxx` | absolute VA of a function / static (tomalla = 1.0.1.77) |
| `//@offset: NN` | byte offset of a field within its object (hex) |
| `//@vftable: NN` | slot offset within the vtable (hex) |
| `//@size: NN` | total object size in bytes (hex) |
| `//@bug` | a faithfully-reproduced original bug |
| `//@todo` | unknown / not-yet-recovered; do not trust |
| `//@rtti` | the verbatim mangled RTTI name (added by us, for traceability) |
| `/* unknown */` | type/value we do not actually know |

Each stubbed class keeps its **verbatim RTTI mangled name** in a comment
(`// .?AVCXxx@@`) so it is always traceable back to the binary.

## Confidence policy

**Only assert offsets/types we actually know.** Everything else is `@todo` or
`/* unknown */`. We never invent an offset or a field type. The honest tiers are:

1. **Field + vtable layout** (high confidence, ported from tomalla, marked):
   `CGameApp`, `CGameMgr`, `CGameWnd`, `CGruntzMgr`, `CGruntzApp`, `CGruntzWnd`,
   `CNetMgr` (partial), `Utils::RegistryHelper`, `Utils::MemoryPool<T>`, `Font`,
   the `UnknownClassArrays` / `UnknownClassInCGruntzMgr` nested types, and the
   `ddrawmgr_surface_family.h` hierarchy (offsets/sizes/inheritance high
   confidence; class IDENTITY = HYPOTHESIS, names = tomalla placeholders).
   Also the version-independent on-disk formats `WwdObject` (field SET high
   confidence, byte layout @todo) and `RezDirEntry` (field set, layout @todo).
2. **Field *order* known, types guessed** (`CGrunt`, from the debug-dump string —
   every member is `@todo`-typed with the raw token kept in a comment).
3. **Name-only** (the bulk): we know the class exists and (often) its rough role
   from naming + strings, but no layout. Represented as forward-declared / empty
   `class CXxx { /* .?AVCXxx@@ */ };`.

`INDEX.md` records, per class, exactly which tier it sits in.

## Coding style

Clean, consistent **C++98-ish** (MSVC 5.0 target — no C++11 in the real build).
Headers are include-guarded. The WAP32 engine base lives in `namespace WAP32`
(matching tomalla); game classes are global, MFC-style `C`-prefixed.
