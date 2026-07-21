# Survey of `src/Stub/Unmatched.cpp` (the `engine_unmatched` worklist)

Triage of every stub in `src/Stub/Unmatched.cpp` — the catch-all "real
reconstruction target with no stub yet" unit emitted by
`gruntz.analysis.gen_unmatched_stubs`. Goal: for each of the 273 stubs, say
**what it actually is** (via instruction shape + nearest matched RVA + its
reference set) and split them into *carve-outs* (not reconstruction targets),
*library/compiler* (skip per matcher policy), *easy decomps*, and the genuine
*game-body backlog*.

Snapshot: HEAD `83cf0a5`, 273 stubs. Method is reproducible — see
[§ Methodology](#methodology).

## Headline

| Bucket | Count | What it is | Action |
|---|---|---|---|
| **GLUE — IAT import thunks** | 19 | `jmp ds:[__imp_Api]` linker-synthesized import stubs | **carve out** (not reconstructable) |
| **GLUE — EH trampolines** | 3 | `pop eax;pop ecx;xchg [esp],eax;jmp eax` compiler catch-return thunk | **carve out** |
| **LIB / compiler bodies** | 46 | CRT iostream, MFC `CWnd`/`Afx*`, zlib, CRT math funclets, `$E*` atexit thunks | **skip** (use the lib; matcher policy) |
| **EASY — GetTypeTag** | 10 | 6-byte `mov eax,<id>; ret` per-class logic-type id virtual | **decomp** (deterministic) |
| **EASY — ret-addr** | 4 | `mov eax,<&global>; ret` accessor returning a pointer | decomp (needs the named global) |
| **EASY — setvtbl ctor** | 3 | `mov [ecx],<vtbl>; ret` base ctor body | decomp (needs the vtable) |
| **EASY — singleton tail-jmp** | 6 | `mov ecx,<&g_obj>; jmp Method` global-object forwarder | decomp (needs the global + callee) |
| **BODY — game** | 182 | genuine engine/game function bodies | reconstruction backlog |

So **22 stubs are pure linker/compiler glue that the generator's filter misses**,
and a further **46 are library code FID didn't tag** — i.e. **~25% of the
"unmatched worklist" is not a reconstruction target at all.** Only ~205 of the
273 are real targets, of which ~23 are trivial leaves.

This extends the existing "unmatched is mostly compiler/library" finding one
level deeper: the carve-outs that `gen_unmatched_stubs` *does* apply (leading ILT
run, `thunk_`/`Unwind@`-named, FID list) leave behind a second layer of glue and
untagged library that the same metric-honesty argument says should also be
excluded.

## GLUE — carve these out of the worklist (22)

These are produced by the linker/compiler from our code, not written in any
source TU; they can never go "exact" because no C++ lowers to them. They inflate
the worklist denominator exactly like the ILT jmp-table that the generator
*already* excludes — they just aren't caught because Ghidra names them `FUN_`
(not `thunk_`/`Unwind@`) and they sit at high scattered RVAs, not the leading run.

### IAT import thunks (19) — `ff 25` `jmp dword ptr ds:[__imp_*]`
```
0x132cca ImageList_Read   0x132cd0 ImageList_Write  0x177660 Ordinal_4(comctl)
0x18b780 VerQueryValueA   0x18b786 GetFileVersionInfoA  0x18b78c GetFileVersionInfoSizeA
0x18b98a RtlUnwind        0x18bcfc ReuseDDElParam   0x18bd02 UnpackDDElParam
0x18bebe ClosePrinter     0x18bec4 DocumentPropertiesA  0x18beca OpenPrinterA
0x18bed0 GetFileTitleA    0x18fa90 DirectInputCreateA   0x1915a8 dinput.Ordinal_1
0x1915ae DirectDrawCreate 0x1915b4 DirectDrawEnumerateA 0x1937c0 ddraw.Ordinal_1
0x1937c6 ddraw.Ordinal_2
```
Each is a 6-byte `jmp ds:[<IAT slot>]` — the linker's thunk for a non-`dllimport`
call to that API. Carve out with the same logic as the ILT run.

### EH catch-return trampolines (3)
```
0x11ee10  0x11ee20  0x11ee30   — pop eax; pop ecx; xchg [esp],eax; jmp eax
```
The MSVC compiler-emitted "return out of the catch funclet" thunk. Compiler glue,
same family as the `Unwind@*` funclets already excluded.

## LIB / compiler bodies — skip per matcher policy (46)

Real bodies, but in statically-linked **CRT / MFC / zlib**, not game code. The
matcher policy ([[matcher-policy-game-not-crt-no-park-huge]]) is *use the library,
don't hand-reconstruct it*. FID tagged most of the CRT/MFC, but these slipped its
signature set. They cluster cleanly:

- **CRT iostream** (`Crt/OStream.cpp` neighborhood, RVA `0x19xxxx`): `ostream`/
  `istream` `ipfx`/`opfx`/`osfx`/`sbumpc`, the `operator<<`/`operator>>` numeric
  formatters, critical-section-guarded stream ops. ~18 fns.
- **MFC** (RVA `0x1bxxxx`–`0x1dxxxx`): `CWnd` ctors, `AfxWndProc`, `AfxGetThread`/
  `AfxUnlockGlobals`, `CView`/`CInternetSession` teardown, and the `$E***`
  **atexit static-initializer thunks** (`mov ecx,&g_obj; call CWnd::CWnd; ... atexit`).
- **zlib** (in the `MenuItem2.cpp` cluster, RVA `0x186xxx`): `zcalloc`/`zcfree`,
  `_tr_init`, `__tr_stored_block`, the `"1.0.4"`/`"deflate"` version strings →
  these belong to the zlib unit (see `docs/zlib-matching.md`), not Gruntz.
- **CRT math funclets** (`0x18bf4d` `log`, `0x18d4cd` `acos`): `__math_exit`/
  `__convertTOStoQNaN`/`__startOneArgErrorHandling` IEEE error-path tails.
- **CRT runtime**: `0x124848`/`0x130570` (`__getptd`/`__abnormal_termination` SEH
  support), `0x118a30` (`__heapchk`/`HeapWalk` debug-heap dump).

These are best handled by *tagging into the FID/library list* (so the metric
excludes them) rather than reconstructing.

## EASY — decompilable leaves (23)

### GetTypeTag (10) — `mov eax,<id>; ret`, the per-class logic-type id

The exact archetype already matched at 100% for `CBehindCandy`/`CEyeCandy`/
`CExitTrigger`/`CDoNothing`/`CTileTriggerTransition` etc. (`i32 C::GetTypeTag()
{ return 0x3f0; }`). Per-class binary layout is
`[GetTypeTag 6B][scalar-dtor][plain-dtor 0x44]`, so each GetTypeTag's owning class
is the one whose dtor it precedes. Folding of the identical 0x44-byte dtors makes
a few ambiguous; the table marks confidence.

| RVA | id | owning class | confidence |
|---|---|---|---|
| `0x00fa40` | `0x3ef` | unnamed eyecandy leaf (dtor = `Boundary_00fb00`) | low — class unnamed |
| `0x010f00` | `0x429` | next-dtor class after `CFortressFlag::~` (`CTileTriggerSwitch`?) | med |
| `0x011bf0` | `0x428` | `CCursorSnapSprite`-adjacent (dtor = `Boundary_011c40`) | low |
| `0x012020` | `0x410` | **`CGruntStaminaSprite`** | **high — done** |
| `0x0120e0` | `0x411` | `CGruntWingzTimeSprite` or folded sibling | med (folding) |
| `0x0121a0` | `0x417` | `CGruntWingzTimeSprite` or folded sibling | med (folding) |
| `0x012cd0` | `0x41c` | **`CParticlez`** | **high — ready** |
| `0x012ff0` | `0x41d` | unnamed class (dtor = `Boundary_013040`) | low |
| `0x0132f0` | `0x425` | **`CPathHazard`** | **high — done** |
| `0x0133b0` | `0x426` | `CVoiceTrigger`? (dtor = `Boundary_013400`) | low |

**Matched in this branch (all verified 100% byte-exact):**
- `CGruntStaminaSprite::GetTypeTag` (`0x12020`=0x410)
- `CGruntToyTimeSprite::GetTypeTag` (`0x120e0`=0x411)
- `CGruntWingzTimeSprite::GetTypeTag` (`0x121a0`=0x417)
- `CPathHazard::GetTypeTag` (`0x132f0`=0x425)
- plus two sibling static `+0x3fN` timer accessors found in the same cluster:
  `CGruntStaminaSprite::GetStaminaTime` (`0x7fbb0`, +0x3f0) and
  `CGruntToyTimeSprite::GetToyTime` (`0x7fca0`, +0x3f4) — the `GetWingzTime` (+0x3f8)
  family.

The `[GetTypeTag 6B][scalar-dtor 0x1e][plain-dtor 0x44]` triple turned out to
attribute every one cleanly (the 0x1e scalar-dtor sits between, the 0x44 plain-dtor
names the class) — the earlier fold ambiguity was just from not seeing the scalar
dtor. Remaining GetTypeTags whose owning class is still an unnamed Boundary stub
(`0x3ef`,`0x428`,`0x429`,`0x41d`) await class identification; `0x41c` (CParticlez)
and `0x426` (CVoiceTrigger) are named but live in the dual-decl `UserLogic.cpp`.

### ret-addr (4) — `mov eax,<addr>; ret`
```
0x0183d0 → 0x5e8e98   0x1b9b8d → 0x6156f4   0x1bb315 → 0x5bb2e6 (AfxWndProc)   0x1bd989 → 0x5bd9ad
```
Accessors returning a pointer to a global/vtable/function. `0x1bb315`/`0x1bd989`
are MFC (`AfxWndProc`, a thunk addr) → library. `0x0183d0` (in `Dialogs.cpp`
cluster) and `0x1b9b8d` are game/engine — need the referent named as a typed
`DATA()` extern (never `(T*)0xADDR`).

### setvtbl ctors (3) — `mov [ecx],<vtbl>; ret`
```
0x11d100 → vtbl 0x5ec26c (scalar-deleting-dtor table)
0x11e8dc → g_severusWorkerDtorVtbl (0x5e8cb4)
0x1d4722 → vtbl 0x5ec26c
```
Bare base ctor that only installs the vptr. Decomp once the vtable's contents are
modeled (else the transitional `*(void**)this = &g_vtbl` masked-`DATA()` form).

### singleton tail-jmps (6) — `mov ecx,<&g_obj>; jmp Method`
```
0x03ac30 → CString (SetMenuItemBitmaps path, CDDrawSurfacePair nbr)
0x0f9710 → CString (NetSession nbr)
0x1155b0 → g_largeFont::<m>   0x115630 → g_mediumFont   0x1156b0 → g_smallFont   (EngStr fonts)
0x1d4c0c → AUX_DATA (MFC)
```
`g_*Font` trio are clean EngStr helpers (`mov ecx,&g_largeFont; jmp Font::<m>`).
Decomp needs the global declared + the tail-called method typed.

## BODY — game reconstruction backlog (182)

Grouped by nearest matched-class neighbor (link order ≈ source TU). High-value
clusters worth a dedicated matcher wave:

- **`ApplyRange`/projectile-type-registry** (`0x7fd0`–`0x8710`, 6 fns):
  `g_projActCache`/`g_buteTree`/`g_typeCounter` — the `[0x008240/0x03e300/0x0c60e0/
  0x0c78b0/0x10ebe0]` `s_actKeyA`+bute-tree register cluster recurs ~6× (the
  per-class `RegisterActs`/type-id allocator), a single cracked idiom reused.
- **`Dialogs.cpp` battlez/options** (`0xc2a20`–`0xc4c00`, ~12 fns): `g_64bd5c`
  channel/options dialog handlers, `SendMessageA`/`GetDlgItem` — self-contained.
- **`Io/FileStream.cpp`** (47 fns, `0xbe030`–`0xc0f10` + MFC tail): CObList list
  ops (`AddTail`/`RemoveTail`), net-session (`OnPlayerLeft`/`HandleControlMsg`),
  RezFree teardown — mixed engine; the low `0xbexxx`–`0xc0xxx` run is one TU.
- **`Grunt.cpp`** (`0x3c8f0`,`0x555e0`(0x12f8!),`0xf71c0`,`0xf8240`): big grunt
  AI/pathing bodies — final-sweep / leaf-first.
- **`CAttract`/`CFader`/`CDirectDrawMgr`/`Image`** clusters: title-sequence,
  fader, DDraw page-mgr, image-decode bodies.
- **`LogicWorkerHandlers`** (`0xaa0a0`/`0xaa1e0`/`0xaa460`, 3× 0xf1): near-identical
  `CMenuSparkle`/`CFrontCandy*` spawn handlers — one idiom, batch.
- **`TriggerMgr`** (`0x759e0`–`0x75a90`, 4 small fns): hit-test helpers.

Sizes span 6 B accessors to the 4856 B `0x555e0`. Per matcher doctrine, leaf-first
within a cluster; defer the >512 B bodies to the final sweep.

## Recommended tooling fix

Extend `gruntz.analysis.gen_unmatched_stubs` to also exclude the 22 GLUE entries
(a byte-pattern carve-out, run on each candidate's first bytes):
- `ff 25` (jmp `ds:[imm32]`) → IAT import thunk
- `58 59 87 04 24 ff e0` → EH catch-return trampoline

and to *tag the 46 library bodies into the FID list* (or a sibling
`compiler_labels.csv`) so the headline metric stops counting them. That removes
~68 non-targets (~25%) from the worklist, sharpening the real backlog to ~205.

## Methodology

All read-only against `$GRUNTZ_EXE` + `build/ghidra-enrich/exports/`:
1. Parse the 273 `RVA(rva,size)` stubs from `Unmatched.cpp`.
2. **Classify by first bytes** (`/tmp/classify_unmatched.py`): IAT-thunk,
   EH-tramp, ret-const, setvtbl, singleton-jmp, getter, body.
3. **Nearest matched RVA**: parse every `RVA()` in `src/` (excluding
   `Stub/`) → bisect each stub against the real-class anchors (link order ≈ source
   order ⇒ the neighbor TU names the cluster).
4. **Reference set**: resolve in-range `e8/e9` calls + `ff15` imports + DIR32
   reloc sites to symbol/string names → the call/data fingerprint that separates
   library from game and hints the class.
5. **GetTypeTag attribution**: dump the ordered function table around each
   (`/tmp/layout.py`) → confirm the `[GetTypeTag][scalar][plain-dtor]` per-class
   layout and read off the owning class from the next dtor.

Full per-function table: [`unmatched-survey-table.txt`](./unmatched-survey-table.txt).
