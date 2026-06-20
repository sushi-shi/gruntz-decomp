# `structure/` — the comprehension-layer remainder

`structure/` holds layout headers (structs/enums) that feed the struct generator
(`scripts/gruntz/build/ghidra_metadata_generate.py`, which reads `src/**/*.h` +
`structure/**/*.h`; **`src/` wins on overlap**). As a class is byte-matched and
authoritatively defined in `src/`, its `structure/` header becomes redundant and is
deleted — the layer shrinks toward empty.

The function-matching worklist that drove most of this is **done**: 22 headers were
cleared as their classes landed in `src/` (the CGrunt / triggers / hazards / sprites
/ states / statusbar / world-object / command / ambient-sound families, the
DX/Bute managers, and the MFC/WAP32 base headers). What remains are **9 type-only
headers** that still uniquely provide a struct or enum with no `src/` equivalent yet:

| header | uniquely provides |
|---|---|
| `enums.h` | 12 game enums |
| `managers/ddrawmgr_surface_family.h` | 19 structs (the `Unknown*` "HP" placeholder classes + MFC collections) |
| `game/cgruntzmgr.h` | `CDWordArray`, `Pair`, `UnknownClassArrays`, `UnknownClassInCGruntzMgr` |
| `formats/wwd_object.h` | `WwdObject`/`WwdObjectRecord`/`WwdRect` (+ 2 enums) |
| `formats/wwd.h` | `PidHeader`/`WwdPlaneHeader` (+ 2 enums) |
| `formats/rez.h` | `RezDirEntry` |
| `registry.h` | `GruntzRegistryConfig` |
| `utils/font.h` | `Font::Size` (nested struct; its 4 functions are already matched) |
| `utils/memory_pool.h` | `MemoryPool_Pair` |

## Deletability test (per header)
A header is deletable once **all** its structs/enums are authoritatively in `src/`:
run `gruntz structs` before/after — the header is deletable iff
`build/gen/structs.json` + `build/gen/enums.json` drop **nothing** (namespaced
`WAP32::X` duplicates of a `src/` `X` are acceptable drops). Then `git rm` the
header, re-run `gruntz structs` to confirm, and commit. (This is exactly how the 22
were cleared.)
