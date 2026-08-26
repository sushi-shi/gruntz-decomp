# Retail `cl.exe` flag audit — MSVC 5.0 SP3

## Result

The effective compile profiles in `config/units.toml` remain correct:

```text
C                    /nologo /c /O2 /MT
C++                  /nologo /c /O2 /MT /GX
C++ with RTTI        /nologo /c /O2 /MT /GX /GR
no-EH exceptions     remove /GX from the affected TU profile
```

No extra global code-generation flag is justified. In particular, retail was
not built with `/O1`, `/Os`, `/Od`, `/Ob0`, `/Ob2`, `/Oa`, `/Ow`, `/Op`,
`/G6`, `/GF`, `/Ge`, `/Gh`, `/EHa`, `/QIfdiv`, `/Gr`, `/Gz`, or the full-general
member-pointer model `/vmg`.

That is an **effective-output** conclusion, not a complete recovery of the
literal Developer Studio command line. Options such as `/W3`, `/FD`, `/Zi`,
`/Z7`, `/Gm`, `/Gi`, `/GA`, `/GD`, `/QI0f`, and some preprocessor definitions
can disappear before the executable or be neutral on every exact witness. They
must stay classified as unresolved even when they were common in period project
files.

This audit was measured on 2026-08-26 with the pinned `cl.exe` 11.00.7022. Every
retail comparison used a TU that was 100% exact before the flag changed; strict
relocation scoring remained enabled.

## Exact witness panels

The panels deliberately cover C and C++, small and large functions, MFC/RTTI,
exception state, FP code, strings/data, and large stack frames:

| unit | exact baseline | useful surface |
|---|---:|---|
| `directinputmgr2` | 56/56 functions, 7,911 code B, 1,620 data B | ABI, strings, intrinsics, EH |
| `grunthealthsprite` | 18/18, 1,667 code B, 780 data B | FP, RTTI, EH, C1 state |
| `directdrawmgr` | 116/116 | large C++/MFC TU |
| `deflate` | 17/17, 4,754 code B | C, packing, hot loops |
| `savegamedialogs` | 18/18, 3,791 code B | RTTI/EH, `__chkstk` witness |
| `ddpalette` | 31/31, 5,180 code B | C++ engine TU |
| `bracketvalue` | 1/1, 210 code B | small control |

The disposable objects and reports live under
`build/compiler-flag-audit/`. They are evidence-run artifacts, not source or
generated state to commit.

## Optimization and emitted-code choices

| question | measured result | conclusion |
|---|---|---|
| `/O2` versus `/Ox` | both exact on `directinputmgr2`; `/O1` 40.73%, `/Os` 42.16%, `/Od` 14.91% | fast optimized bundle proven; literal `/O2` is the period/default spelling |
| inline level | `/Ob1` exact; `/Ob0` 84.72%, `/Ob2` 92.73% | effective `/Ob1`, which VC5 `/O2` implies |
| global/intrinsic/frame passes | `/Og-`, `/Oi-`, and `/Oy-` each regress the exact unit | `/Og`, `/Oi`, `/Oy` are effective |
| speed preference | `/Ot` exact; `/Os` strongly wrong | effective `/Ot`; explicit spelling is unobservable |
| alias assumptions | `/Oa` and `/Ow` regress | both are off |
| FP consistency | `/Op` changes two exact `grunthealthsprite` functions; `/Op-` remains exact | `/Op` is off |
| function COMDATs | `/Gy-` is byte-neutral under `/O2`; the objects still emit per-function COMDATs | packaging is effective, explicit `/Gy` is not proven |
| string pooling | `/Gf` is neutral; `/GF` moves literal COMDATs to `.rdata` and loses retail data matches | `/O2`'s writable-`.data` `/Gf` behavior is retail; `/GF` is off |

`/Op` needed an FP-bearing witness: it was neutral on `directinputmgr2` but
regressed `grunthealthsprite` to 16/18 exact functions. This is why a neutral
single-unit result is never treated as proof that an option was present.

## CPU, layout, and ABI choices

| question | measured result | conclusion |
|---|---|---|
| processor target | `/G3`, `/G4`, `/G5`, `/GB`, and default are identical across `grunthealthsprite`, `directdrawmgr`, `deflate`, and `savegamedialogs`; `/G6` regresses all four | `/G6` excluded; the other literal spellings are indistinguishable |
| default calling convention | `/Gr` makes `directinputmgr2` 80.78%; `/Gz` makes it 80.86% | `/Gd`/cdecl proven for unannotated free functions |
| struct packing | `/Zp1` and `/Zp2` both change `deflate`; `/Zp4`, `/Zp8`, `/Zp16`, and default are exact across the ABI panel | only packing **at least 4** is proven; default `/Zp8` is the least-assumption setting, not a recovered literal flag |
| member pointers | `/vmg` regresses RTTI data/code; `/vmb`, `/vms`, `/vmm`, and `/vmv` are neutral on the panel | full-general model excluded; keep default `/vmb`; the narrower explicit assumptions are not witnessed |
| `vtordisp` | `/vd0` and `/vd1` are neutral | unresolved because the panel has no discriminating virtual-inheritance use |
| plain `char` | `/J` is neutral across nine char-heavy exact TUs, but changes a signed-char synthetic control | unresolved; no retail signed-plain-char witness yet |

The older statement that deflate “pins `/Zp8`” was too strong. It pins the
layout reached by every setting from 4 through 16; the default remains correct
because no override is required.

## Runtime, exceptions, and RTTI

- `/MT` is proven at the linked-image level: retail statically contains the
  multithreaded CRT/MFC and has no `MSVCRT.DLL` import. `/ML` happens to be
  code/data-exact on `directinputmgr2`, but its `.drectve` selects the wrong
  runtime and therefore is not a retail candidate. `/MD` also changes one large
  exact function; `/MTd` changes five.
- `/GX-` regresses `directinputmgr2`, `grunthealthsprite`, `ddpalette`, and
  `savegamedialogs`. `/EHs` and `/EHs /EHc` are exact on the EH panel, while
  `/EHa` regresses three C++ units. Retail uses synchronous C++ EH, not
  asynchronous SEH translation. VC5's own help describes `/GX` as the `/EHsc`
  spelling.
- `/GR-` leaves the `grunthealthsprite` code exact but drops its matched RTTI
  data from 780 B to 44 B. `/GR` is therefore required for the game RTTI
  profile. The engine and the two documented no-EH exceptions retain their
  separate profiles; this is not evidence for one solution-wide `/GR` switch.

## Stack probes, instrumentation, and Pentium FDIV

- `/Ge` and `/Gs0` add probes to frames that retail leaves alone and regress the
  exact panels. `/Gs4096` and `/Gs8192` are neutral on the selected small frames;
  `/Gs999999` suppresses the retail `__chkstk` call in `savegamedialogs`.
  Retail has an unprobed 0x340-byte frame and probed application frames at
  0x1124, 0x1674, 0x384d4, and 0x86ac. That interval is fully consistent with
  VC5's default 0x1000 threshold; no custom `/Gs` value is justified.
- `/Gh` inserts `__penter` and is excluded. The linked CRT's own profiling or
  CPU-test routines are not evidence that application TUs used `/Gh`.
- Default and `/QIfdiv-` emit raw x87 divide instructions. `/QIfdiv` adds an
  `__adjust_fdiv` check and an `__adj_fdiv_m64` call around a synthetic divide.
  Retail application code contains direct unwrapped `fdiv`/`fdivr`, so
  `/QIfdiv` is off. The statically linked CRT still contains the Pentium FDIV
  test/fix routines; those library members do not imply the compiler switch.
- `/QI0f` and `/QI0f-`, `/GA`, and `/GD` were byte-neutral on every exact
  witness. They remain unresolved rather than “verified off.”

## Developer Studio flags and precompiled headers

A plausible VC5 Release project line is broader than the effective profiles:

```text
/nologo /MT /W3 /GX /O2
/D WIN32 /D NDEBUG /D _WINDOWS
/YX /FD /c
```

`/GR` would be added for the game project/TUs that emit retail RTTI. `_MBCS`
was also common in generated ANSI projects, but is a separate question.
Format-5 Developer Studio projects preserve this Release-line family in a
[contemporary project listing](https://tpc.org/results/fdr/tpcc/cpq_DL760-8P-900_fdr.pdf).
Microsoft's VC5 readme explains that the IDE adds `/FD` for dependency tracking
([KB Q165687 mirror](https://ftp.zx.net.nz/pub/mirror/ftp.microsoft.com/MISC/KB/en-us/165/687.HTM)),
and its AppWizard documentation says generated MFC projects use a `StdAfx.h`
precompiled header ([KB Q173483 mirror](https://ftp.zx.net.nz/pub/archive/ftp.microsoft.com/MISC/KB/en-us/173/483.HTM)).

Those are priors, not retail proof:

- `/W3` is diagnostics-only and was exact on the panel.
- `/FD` produced its dependency database and left all 68 panel functions exact.
- `/D WIN32`, `/D NDEBUG`, and `/D _WINDOWS` were individually exact on
  `grunthealthsprite`; their absence from emitted bytes does not establish that
  the original project omitted them.
- `/D _MBCS` changed only `CGruntHealthSprite::HealthUpdate`, from 100% to
  95.072464%, without changing its size, calls, or CFG. The combined Release
  define bundle produced the same 180-byte function as `_MBCS` alone. Therefore
  `_MBCS` is not safe to add to the current exact TU merely because it was a
  common IDE default.
- `/Zi`, `/Z7`, `/Zd`, `/Gi`, `/Gm`, and `/Zl` are neutral on the scored
  code/data surfaces. Retail has no useful debug directory/PDB path, but the
  linked image cannot by itself distinguish debug records discarded by LINK.

### PCH boundary A/B

VC5 precompiled headers are not byte-neutral state caches:

| TU and boundary | result |
|---|---:|
| `directinputmgr2`, no PCH | 100% |
| `directinputmgr2`, explicit `/Yc` + `/Yu` at `DirectInputMgr2.h` | 100% |
| `directinputmgr2`, automatic `/YX` | 100% |
| `grunthealthsprite`, no PCH | 100% |
| `grunthealthsprite`, explicit `/Yc` + `/Yu` at `GruntHealthSprite.h` | 99.46793%, 17/18 |
| `grunthealthsprite`, automatic `/YX` | 99.46793%, 17/18 |
| either TU, explicit boundary at the first minimal `rva.h` include | 100% |

Creator and user compiles agree. The only changed function is the 180-byte
`HealthUpdate`, at 95.072464%; the PCH and `_MBCS` versions of that function are
byte-identical. The changed stream has the same calls, branches, returns, and
relocations as retail, but rotates the address calculation and the virtual-call
receiver registers. This is a controlled VC5 front-end-state effect, documented
as [`patterns/pch-boundary-is-a-tu-state-input.md`](patterns/pch-boundary-is-a-tu-state-input.md).

Consequences:

1. Do not add `/YX`, `/Yc`, `/Yu`, or `_MBCS` globally to imitate an IDE
   template. They break a current exact witness.
2. The original project may still have used a PCH with a different original
   include prefix or explicit boundary. Recover that only from corroborating
   source/project evidence plus exact-TU A/Bs.
3. A PCH boundary can be tested as a disposable TU-state diagnostic, but it is
   not a humane wall-closing source lever and must not be retained solely for a
   score.

### Executable PCH forensics

The direct PCH metadata is gone. The generated `.pch` files contain the
`VCPCH0` signature, boundary/header names, include/source paths, and PDB path;
none of those strings or `/Y*` switches occurs in the compiled objects or
retail executable. `GRUNTZ.EXE` has no COFF symbol table, debug directory, or
file overlay, and VC5's C/C++ objects add no Rich-header `@comp.id` record.
There is therefore no literal PCH flag, filename, or boundary to extract.

An indirect trace does survive: the PCH boundary can change the order in which
VC5 emits deferred COMDATs, and LINK 5.10 preserves that order in the final
`.text` and `.rdata`. Sweeping every direct include in the two exact TUs found:

- `directinputmgr2`: no PCH and an `rva.h`-only boundary have 53 retail-order
  inversions over 45 common functions and **zero** over 13 common `.rdata`
  contributions. A boundary at `DirectInputMgr2.h` or any usable later include
  has 198 and 8 respectively, despite remaining 56/56 byte-exact.
- `grunthealthsprite`: no PCH and the `rva.h` boundary remain 18/18 exact;
  every boundary from `GruntHealthSprite.h` through the last usable project
  include reaches the same 17/18 `HealthUpdate` island and is excluded under
  the current reconstructed prefix.

This lets us recover boundary **constraints**, not the original PCH itself.
Use only unique, selected retail contributions: header-inline/RTTI COMDATs kept
from another TU can place a symbol at that other object's address and create a
false boundary fingerprint. Full evidence and the creator-versus-consumer
caveat are in
[`patterns/pch-boundary-is-a-tu-state-input.md`](patterns/pch-boundary-is-a-tu-state-input.md).

## Working classification

**Proven effective:** `/O2`'s `/Ob1 /Og /Oi /Ot /Oy /Gf` behavior, `/MT`,
`/Gd`, synchronous `/GX` on the relevant C++ profiles, `/GR` on RTTI profiles,
packing at least 4, default-compatible stack probing.

**Proven absent:** `/O1`, `/Os`, `/Od`, `/Ob0`, `/Ob2`, `/Oa`, `/Ow`, `/Op`,
`/G6`, `/GF`, `/Ge`, `/Gh`, `/EHa`, `/QIfdiv`, `/Gr`, `/Gz`, `/vmg`, `/Zp1`,
and `/Zp2`.

**Literal spelling unresolved:** `/O2` versus code-equivalent `/Ox`, explicit
`/Ob1`/`/Ot`/`/Gy`/`/Gf`, `/G3`/`/G4`/`/G5`/`/GB`, `/Zp4`/`/Zp8`/`/Zp16`,
`/J`, `/QI0f`, `/GA`, `/GD`, `/vd*`, the narrow `/vm*` assumptions, warning and
debug/dependency switches, Release preprocessor definitions, and the original
PCH boundary.
