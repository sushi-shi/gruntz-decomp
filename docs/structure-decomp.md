# Eliminating `structure/` — leaf-first decomp worklist

Goal: byte-match every class still living in `structure/` (the comprehension
layer) so its header becomes redundant and can be deleted — eventually emptying
`structure/`. `src/` wins over `structure/` in the struct generator
(`ghidra_metadata_generate.py`), so a header is deletable once **all** its classes
are authoritatively defined in `src/`.

## Per-header method
1. Pick the next header (leaf-first = fewest remaining labeled functions / smallest bytes).
2. Match its classes' labeled functions to byte-exact in a new/existing `src/` TU
   using the **`RVA(addr, size)`** macro (`src/rva.h`; see `docs/build-system.md`
   "Add a translation unit" / `docs/match-learnings.md`).
3. Verify coverage empirically: `gruntz structs` before/after — the header is
   deletable iff removing it drops **no** struct from `build/gen/structs.json`
   (namespaced `WAP32::X` dups of a `src/` `X` are acceptable drops).
4. `git rm structure/<header>`; re-run `gruntz structs` to confirm; commit.

## Worklist (leaf-first; `remaining labeled fns / total · remaining bytes · #classes`)
Closest to clearable first:
- `game/mapmgr.h`        — 1/3 · 167B · 2cls  (CMapMgr nearly done; CGruntzMapMgr remains)
- `game/dialogs.h`       — 2/6 · — · (4 matched; remain: CCheckpointDlg, CMultiHelpDlg)
- `game/userlogic.h`     — 2/2 · 480B · CUserBase/CUserLogic
- `managers/directsoundmgr.h` — 2 · 967B
- `managers/directinputmgr2.h` — 2 · 1644B
- `game/misc.h`          — 3/3 · 1086B · CDoNothing/CDoNothingNormal/CSingleFrameMessage
- `managers/cdirectdrawmgr.h` — 3 · 3254B
- `game/commands.h`      — 5/5 · 132B · CGruntzCommand/Single/Multi  (NOTE: 2 trivial 7B
                            vptr-ctors + 1 scalar-del dtor are easy; @0x24220/@0x24360 are
                            43B `new(0x14)`+static-guard+tail-call — singleton/factory shape, not plain ctors)
- `game/cgruntzwnd.h`    — 5/5 · 212B · CGruntzWnd (extends matched CGameWnd)
- `game/ambient_sound.h` — 5/5 · 673B · CAmbientSound/CAmbientPosSound/CRandomAmbientSound
                            (dtors @0xb6a0/b7b0/b850/b960/bb60 are RTTI-vptr leaves)
- `game/states.h`        — 7/8 · 5106B · 11cls (CAttract/CDemo/CHelpState/CMulti/CMultiBootyState/CSplashState…)
- `game/statusbar.h`     — 7/8 · 13348B · 14 SBI classes
- `game/projectiles.h`   — 8/8 · 4584B · CBoomerang/CExplosion/CParticlez/CProjectile/CTimeBomb
- `game/grunt.h`         — 8/20 · 10353B (CGrunt mostly matched; remain CGruntCreationPoint/Puddle/StartingPoint/Voice)
- `game/sprites.h`       — 10/10 · 2761B · 9 sprite classes
- `game/eyecandy.h`      — 11/11 · 4601B · 11 classes
- `game/cgruntzmgr.h`    — 13/14 · 3548B · CGruntzMgr/CPtrArray/CDWordArray/UnknownClassArrays…
- `game/world_objects.h` — 15/15 · 7901B · CWarlord/CWormhole/CTeleporter/CFortressFlag…
- `game/hazards.h`       — 17/17 · 10434B · 12 hazard classes
- `game/triggers.h`      — 28/28 · 6562B · 23 trigger classes
- `managers/ddrawmgr_surface_family.h` — 91/91 · 10219B · 24cls (the Unknown* "Harry Potter" placeholder classes + MFC collections)

## Type-only headers (NO functions to match)
These define structs/enums/formats, not code — they can't be "decomp'd", only
removed if their TYPES become defined in `src/`:
`enums.h`, `formats/rez.h`, `formats/wwd.h`, `formats/wwd_object.h`,
`managers/butemgr.h`, `registry.h`, `utils/memory_pool.h`, `wap32/cwapobj.h`,
`wap32/zruntime.h`, `mfc_runtime.h`. `utils/font.h` — its 4 functions are matched
but it's kept for the `Font::Size` nested struct (no `src/` equivalent).

(Regenerate the remaining/total counts any time with the analysis in the campaign
log; matched = present in `build/gen/symbol_names.csv`.)
