# Investigation: `src/Stub/UnmatchedLibrary.cpp` — are these 15 truly library?

Read-only disassembly of every entry in the `engine_unmatched_lib` split vs
`$GRUNTZ_EXE`, resolving each function's full call/import/string/data reference set
and cross-checking the FID matcher's raw output (`build/fid/`). Answers: *what is
each*, *why did FID miss it*, *is it truly library*.

## How "FID" works here (and how it misses)
`config/library_labels.csv` is produced by the custom masked-byte COFF matcher
(`gruntz.analysis.fid`), **not** Ghidra FID. It signs every *external* function
symbol in the VC5 `LIBCMT.LIB` / `NAFXCW.LIB` `.obj` members (reloc-masking
DIR32/REL32 fields) and matches them **only at Ghidra-carved function starts**,
needing ≥8 bytes / ≥6 fixed bytes. zlib is covered by a *separate* anchored pass
(`config/zlib_labels.csv`, delinked against `vendor/zlib-1.0.4/`), not by this
matcher. So a body is missed when it is (a) not a carved start (mid-body fragment),
(b) too small / too few fixed bytes after masking, (c) absent from the signed set
(compiler glue; zlib gap; static/local helper with no external symbol), or
(d) codegen-diverged from our lib build.

## Unifying fact: these are unreferenced orphans
**14 of 15 have zero call/jmp xrefs and zero address-taken** — orphan bodies the
linker pulled in but nothing calls (the whole zlib *deflate* compression path,
unused iostream overloads, an MFC accessor). That is why caller-tracing never
reaches them and FID is the only tool that could label them. The zlib *deflate*
path survives `/OPT:REF` because stock zlib is built **without** `/Gy`: `deflate.c`
is one non-COMDAT section, so the linker keeps it whole once anything in it is
referenced. (CRT/MFC objs *are* `/Gy`/COMDAT, so `/OPT:REF` can drop their unused
functions individually.)

### COMDAT / PDB notes
- The linked `GRUNTZ.EXE` has **no COMDAT** — it is an object-file
  (`IMAGE_SCN_LNK_COMDAT`) concept the linker consumes; the PE retains none, and
  `functions.csv`/`symbol_names.csv` have **0 duplicate RVAs** (nothing folded to
  observe image-side). The *library `.obj`s* in `build/fid/` do use COMDAT.
- **No PDB exists** (debug-directory RVA = 0; no CodeView/RSDS; none ever shipped).
  The only symbol↔address ground truth is the library `.obj` symbol tables (e.g.
  `winsig.obj` defines `___pxcptinfoptrs` → `0x130570`).

## Per-function verdict

| RVA | size | identity | lib? | why FID missed |
|---|---|---|---|---|
| 0x118a30 | 0xda | **engine heap-dump diag** — `ApiCallerStubs` HeapWalk wrapper + `__heapchk` + `sprintf` + OutputDebugString; "Checking/Walking/Finished walking heap.", "USED"/"FREE" | **NO — engine** | correctly no hit (not CRT/MFC/zlib). **Misfiled** → reclassified to `Unmatched.cpp` |
| 0x124848 | 0x4e | CRT **C++ EH cleanup** — `0xe06d7363`/`0x19930520`, `__getptd`, `_DestructExceptionObject` | yes (CRT) | mis-carved **fragment**: no prologue, 65 B uncarved gap before it → no lib symbol starts there |
| 0x130570 | 0x9 | CRT `__pxcptinfoptrs` (`call __getptd; add eax,0x54; ret`) | yes (CRT) | **FID does match it** (MEDIUM, `winsig.obj`) — tracked `library_labels.csv` is stale; regen labels it |
| 0x1861b0 | 0x21f | zlib **`deflateInit2_`** — version/stream_size check, `zalloc=&_zcalloc`, state alloc | yes (zlib) | zlib not in LIBCMT/NAFXCW; the `zlib_labels.csv` pass missed deflate.c entry points → **now labeled** |
| 0x1864d0 | 0x76 | zlib **`deflateReset`** — calls `__tr_init` | yes (zlib) | same → **now labeled** |
| 0x186620 | 0x2b6 | zlib **`deflate`** — `configuration_table[level].func` dispatch (`ff 14 85`) | yes (zlib) | same → **now labeled** |
| 0x18bf4d | 0xaf | CRT math **`log` error tail** — `__math_exit`/`__convertTOStoQNaN`/`__startOneArgErrorHandling`, "log" | yes (CRT) | error-path helper: likely local/static label (no external sym) → no signature |
| 0x18d4cd | 0xae | CRT math **`acos` error tail** | yes (CRT) | same as 0x18bf4d |
| 0x193080 | 0xb5 | iostream **ostream** body — `opfx`→`operator<<`×N→`osfx` (unreferenced) | likely CRT * | composite body matches no single signed overload (length/codegen) |
| 0x193140 | 0x1fa | iostream **istream** body — `ipfx`/`operator>>`/`putback`/`isdigit`/`eatwhite` + MT lock | likely CRT * | same — composite, no matching signed body |
| 0x1badee … 0x1baea8 | 0xc ×4 | compiler **`$E362/368/373/378` atexit thunks** (`push &$E; call _atexit; pop ecx; ret`) | glue | compiler-emitted per-TU, in no `.lib`; 12 B/4 fixed < threshold |
| 0x1bb315 | 0x6 | MFC accessor `mov eax,&AfxWndProc; ret` | yes (MFC) | 6 B, 2 fixed bytes « threshold |

\* moderate confidence: iostream-cluster, library-only call graph, unreferenced;
composite (not a single signed overload), so could in principle be a custom inserter.

## Actions taken in this change
1. **zlib deflate trio** (`0x1861b0`/`0x1864d0`/`0x186620`) → `config/zlib_labels.csv`
   (unit `deflate`); removed their stubs from `UnmatchedLibrary.cpp`. The `deflate`
   unit's 6 *local* functions already match, so these public entry points should
   match byte-exact.
2. **`0x118a30`** reclassified from library → engine reconstruction target
   (moved to `Unmatched.cpp`).
3. Left as correctly-classified library/compiler stubs (not reconstruction
   targets): the EH fragment, both math tails, both iostream bodies, the four `$E`
   atexit thunks, the AfxWndProc accessor, and `__pxcptinfoptrs` (the last is a
   stale-FID item — a `fid_generate` regen will label and drop it).
