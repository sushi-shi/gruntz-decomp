# GRUNTZ.EXE — Linked Libraries & Function-Identification Plan

Decompilation/preservation RE notes for a binary the project owns.

**Target:** `GRUNTZ.EXE` (`$GRUNTZ_EXE`, flake-fetched) — Gruntz (1999, Monolith Productions),
PE32 x86 GUI, 2,511,872 bytes, 6 sections.
**Toolchain:** MSVC 5.0 (PE `MajorLinker 5 / MinorLinker 10` = LINK 5.10;
Rich header @comp.id reports C/C++ compiler build 8034 + cvtres 1668 — the VC5
signature). `_MSC_VER` for this toolchain is 1100.
**Linkage:** static CRT + static MFC 4.2 + statically-linked zlib 1.0.4; the rest
is the WAP32/"zlith" engine and Gruntz game code.
**Siblings (same toolchain, for cross-matching):** `CLAW.EXE` and `MEDIEVAL.EXE`
(also flake-fetched).

Section map (`objdump -h`):

| Idx | Name    | Size (hex) | Size (dec) | VMA        | Notes |
|-----|---------|-----------:|-----------:|------------|-------|
| 0   | .text   | 0x1e526b   | 1,987,179  | 0x00401000 | ~1.9 MB of code — the matching surface |
| 1   | .rdata  | 0x20fa8    |   135,080  | 0x005e7000 | |
| 2   | .data   | 0x21400    |   136,192  | 0x00608000 | RTTI `.?AV…` type_info names live here (MSVC5 quirk) |
| 3   | .idata  | 0x3b41     |    15,169  | 0x006c3000 | import table |
| 4   | .rsrc   | 0x1e17c    |   123,260  | 0x006c7000 | resources |
| 5   | .reloc  | 0x1b9d1    |   112,593  | 0x006e6000 | base relocs |

The whole point of this document: **~1.9 MB of `.text` currently looks like "game
code" but a large fraction is unmodified library code (CRT, MFC, zlib).** Identify
and label those bodies with function-ID tooling so hand-matching is reserved for the
actual Gruntz/WAP32 logic.

---

## 1. Static libraries linked into `.text` (the FID prize)

All three below are compiled INTO the binary — there are NO corresponding DLL
imports for them (confirmed: no `msvcrt.dll`, no `mfc42.dll`, no `zlib.dll` in the
import table). They are the highest-value identification targets.

### 1.1 MSVC 5.0 C runtime — STATIC, multithreaded (LIBCMT.LIB) — CONFIRMED

Evidence (`GRUNTZ.strings.all.txt`):

- Full CRT error-message table present: `R6002 - floating point not loaded`,
  `R6008 - not enough space for arguments`, `R6009 - not enough space for
  environment`, `R6016 - not enough space for thread data`, `R6017 - unexpected
  multithread lock error`, `R6018 - unexpected heap error`, `R6019 - unable to open
  console device`, `R6024 - not enough space for _onexit/atexit table`, `R6025 -
  pure virtual function call`, `R6026/R6027` (stdio/lowio init), `R6028 - unable to
  initialize heap`, `abnormal program termination`, `runtime error`,
  `Microsoft Visual C++ Runtime Library`, `Runtime Error!`, `<program name unknown>`.
- Math errno tables: `DOMAIN error`, `SING error`, `TLOSS error`, transcendental
  names (`atan acos asin tanh cosh sinh log10 pow`) → the `_matherr`/float CRT is in.
- Heap-walk diagnostics: `Heap return value: _HEAPOK/_HEAPEMPTY/_HEAPBADPTR/
  _HEAPBADNODE/_HEAPBADBEGIN` → `_heapchk`/`_heapwalk` family present.
- Locale/MB tables present (`SunMonTue…`, `JanFebMar…`, big country/locale list,
  `GetThreadLocale`, `MultiByteToWideChar`).
- **Variant determination:** `R6016 - not enough space for thread data` and
  `R6017 - unexpected multithread lock error` are emitted only by the *multithreaded*
  CRT (mtdll). Static MFC (the `…42s` flavor, below) additionally *requires* the
  static multithreaded CRT. ⇒ The CRT is **`LIBCMT.LIB`** (static multithreaded),
  release build, NOT `LIBC.LIB` (single-threaded) and NOT `MSVCRT.LIB`
  (DLL import — ruled out, no msvcrt.dll import).

So: **MSVC 5.0 static multithreaded CRT = `LIBCMT.LIB`.**

### 1.2 MFC — STATIC, version 4.2 (the `42s` flavor) → NAFXCW.LIB — CONFIRMED

Evidence:

- Window-class registration strings carry the MFC version+linkage suffix:
  `AfxWnd42s`, `AfxControlBar42s`, `AfxMDIFrame42s`, `AfxFrameOrView42s`,
  `AfxOleControl42s`, plus `AfxOldWndProc`, `Afx:%x:%x:%x:%x:%x`, `Afx:%x:%x`.
  The **`42`** = MFC 4.2x; the trailing **`s`** = **static** link (the DLL flavor
  would be `AfxWnd42d`/`AfxWnd42` w/o `s` and would import `mfc42.dll` — it does not).
- RTTI / `type_info` names (in `.data`, MSVC5 quirk) for MFC internals:
  `.?AVCObject@@`, `.?AVCWnd@@`, `.?AVCException@@`, `.?AVCStringList@@`,
  `.?AVCStringArray@@`, `.?AVCMemFile@@`, `.?AV_AFX_THREAD_STATE@@`,
  `.?AVAFX_MODULE_STATE@@`, `.?AVAFX_MODULE_THREAD_STATE@@`,
  `.?AV_AFX_BASE_MODULE_STATE@@`, `.?AV_AFX_WIN_STATE@@`,
  `.?AV_AFX_CTL3D_STATE@@`, `.?AV_AFX_CTL3D_THREAD@@`, `.?AVCObjectDropper@@`, etc.
- The full documented MFC class inventory (CWinApp/CWinThread/CCmdTarget/CWnd/
  CFrameWnd/CDialog/CView/CDC/CGdiObject/CArray family/CString family/CFile family/
  CException family …) is present per `strings-analysis.md` §1(b).
- `_AFX_CTL3D_STATE` + the literal `CTL3D32.DLL` string: static MFC 4.2 calls
  `Ctl3dRegister` via a runtime `LoadLibrary("CTL3D32.DLL")` (NOT a static import).
  This is an MFC artifact, not separate game code, and explains why `CTL3D32.DLL`
  appears as a string but not in `.idata`.

MFC version nuance: VC++ 5.0 shipped MFC **4.21** (the build the `42` libs
represent); the `42` token is shared with VC4.2/VC6, so the *exact* `.lib` must come
from a **VC5** install to byte-match (see §3). So: **static release MFC =
`NAFXCW.LIB`** (debug would be `NAFXCWD.LIB` + the `…42sd` strings — not seen).

### 1.3 zlib — STATIC, version 1.0.4 — CONFIRMED (high-value quick win)

Evidence — the complete zlib 1.0.4 string set is present, contiguous:

```
1.0.4
 deflate 1.0.4 Copyright 1995-1996 Jean-loup Gailly
 inflate 1.0.4 Copyright 1995-1996 Mark Adler
need dictionary / incorrect data check / incorrect header check /
invalid window size / unknown compression method
invalid bit length repeat / too many length or distance symbols /
invalid stored block lengths / invalid block type
incompatible version / buffer error / insufficient memory / data error /
stream error / file error / stream end          (the z_errmsg[] table)
incomplete dynamic bit lengths tree / oversubscribed dynamic bit lengths tree /
incomplete literal/length tree / oversubscribed literal/length tree /
invalid distance code / invalid literal/length code   (inflate tree builder)
```

This is unambiguously **zlib 1.0.4** (released 1996-07-24). Both the `deflate`
*and* `inflate` banners are present, plus the inftrees error set, so the binary
links the full deflate+inflate code path (the WWD/REZ loader uses inflate; deflate
is likely pulled in by the same `.lib`/`.obj` set). Adler is `adler32` (banner names
Mark Adler). This maps to the leaked `C:\Proj` engine: the WAP32 resource/WWD layer
calls into this zlib.

### 1.4 C++ iostreams / STL — STATIC (CRT-bundled, low value)

- RTTI for the old MSVC iostreams hierarchy: `.?AVios@@`, `.?AViostream@@`,
  `.?AVstreambuf@@`, `.?AVstrstreambuf@@`, `.?AVfstream@@`, and (per
  strings-analysis) `istream/ostream/istream_withassign/ostream_withassign/
  istrstream/ostrstream/strstream/filebuf/ifstream/ofstream`.
- This is the pre-standard MSVC5 iostreams that ships *inside* the same CRT libs
  (`LIBCMT.LIB`), so a generated CRT FID db covers it too — no separate acquisition.
- No third-party STL (no STLport / no `std::basic_string` mangling) — VC5 predates a
  usable shipped STL; the engine uses its own `z*` containers (see cross-game §4).

### 1.5 Other static libs / glue

- **No static Miles/Smacker code** — both are DLL imports (`mss32.dll`,
  `smackw32.dll`), so only thin thunks + the import stubs are in `.text`. The string
  `_SmackOpen@12` is an import-thunk symbol, not a statically-linked body.
- **SFMAN32** (`SFMAN32.DLL`, SoundFont manager): dynamically `LoadLibrary`'d (string
  present, not in `.idata`) — the `SFManager` class is *game* glue around it.
- No separate math lib — the float/math CRT is part of `LIBCMT.LIB` (§1.1).

---

## 2. Dynamic imports (thunks only — low value, listed for completeness)

From the PE import table (`objdump -p … | grep 'DLL Name'`). These contribute only
import stubs/thunks to `.text`; the bodies live in the OS/3rd-party DLLs and are NOT
matching targets.

| DLL            | Vendor / SDK                  | Purpose |
|----------------|-------------------------------|---------|
| KERNEL32.dll   | Microsoft Win32               | core kernel/process/heap/file |
| USER32.dll     | Microsoft Win32               | windows, messages, input |
| GDI32.dll      | Microsoft Win32               | GDI drawing |
| ADVAPI32.dll   | Microsoft Win32               | registry (HKLM Monolith\Gruntz) |
| COMCTL32.dll   | Microsoft Win32               | common controls |
| comdlg32.dll   | Microsoft Win32               | common dialogs |
| SHELL32.dll    | Microsoft Win32               | shell APIs |
| VERSION.dll    | Microsoft Win32               | version-resource queries |
| WINSPOOL.DRV   | Microsoft Win32               | print spooler (MFC pulls it) |
| WINMM.dll      | Microsoft Win32 multimedia    | timers / waveOut / joystick |
| DDRAW.dll      | Microsoft DirectX             | DirectDraw (DDrawMgr) |
| DINPUT.dll     | Microsoft DirectX             | DirectInput (DinMgr2) |
| DSOUND.dll     | Microsoft DirectX             | DirectSound (DsndMgr) |
| DPLAYX.dll     | Microsoft DirectX             | DirectPlay (NetMgr) |
| mss32.dll      | RAD Game Tools — Miles Sound System | audio mixing / MP3 / SoundFonts |
| smackw32.dll   | RAD Game Tools — Smacker      | FMV / cutscene playback |

Loaded dynamically (string only, not in `.idata`): **CTL3D32.DLL** (via static MFC),
**SFMAN32.DLL** (SoundFont manager). 23 named imports total across the table.

Sibling cross-check (`objdump -p`):
- **CLAW.EXE** adds `WSOCK32.dll` + `WININET.dll`, lacks `DINPUT.dll`.
- **MEDIEVAL.EXE** adds `WININET.dll`; otherwise the same DX/Miles/Smacker set.
- Same DirectX + Miles + Smacker + MFC stack across all three ⇒ same engine/toolchain.

---

## 3. Can we get the libs (to build signatures)?

### 3.1 MSVC 5.0 static CRT + MFC — ships WITH the VC5 toolchain we're packaging

Once the VC++ 5.0 toolchain tarball is in hand, the exact `.lib`s are present
(typically `…\VC\lib\`). The relevant files:

| File          | What it is                                   | Needed for GRUNTZ? |
|---------------|----------------------------------------------|--------------------|
| `LIBCMT.LIB`  | static **multithreaded** CRT (release)       | **YES** — §1.1 (also covers iostreams §1.4) |
| `LIBCMTD.LIB` | static MT CRT, debug                         | no (release build) |
| `LIBC.LIB`    | static single-threaded CRT                   | no (MT build) |
| `MSVCRT.LIB`  | import lib for msvcrt.dll                     | no (not DLL-linked) |
| `NAFXCW.LIB`  | static **MFC 4.2 release** (the `…42s` lib)   | **YES** — §1.2 |
| `NAFXCWD.LIB` | static MFC 4.2 debug                          | no |
| `MFC42.LIB`   | import lib for mfc42.dll                       | no (not DLL-linked) |

So the two `.lib`s we actually need to generate FID from are **`LIBCMT.LIB`** and
**`NAFXCW.LIB`** from a *VC5* install. **Caveat (must-confirm):** MFC `42`/CRT bodies
shifted between VC4.2 → VC5 (4.21) → VC6; for the highest hit-rate the libs MUST come
from the **VC++ 5.0** SP that built Gruntz. A VC6 lib will partially match but miss
on inlining/codegen differences. (Gruntz copyright is 1998; VC5 + its service packs
SP1–SP3 are the candidates; if first-pass hit-rate is low, try the other VC5 SP libs.)

### 3.2 zlib 1.0.4 — freely available, exact version archived

- Source tarball **`zlib-1.0.4.tar.gz`** (1996-07-24, 84 KB) is hosted at
  **https://zlib.net/fossils/** (the official fossils archive; confirmed present
  alongside 0.71 … 1.3.x). Also mirrored in many old source trees
  (e.g. MIT `source-8.1/third/ssh/zlib-1.0.4/`).
- **Build-exactness for FID/FLIRT:** zlib has no prebuilt MSVC5 `.lib`, so we must
  *recompile* `zlib-1.0.4` with the SAME VC5 `cl` + the SAME optimization flags
  (release `/O2`-ish, `/MT` to match LIBCMT) to byte-match the bodies. FID hashes
  instructions with operand-masking so it tolerates relocation/address differences,
  but it is sensitive to *codegen* (instruction selection, inlining, register
  allocation). Practical approach: build zlib 1.0.4 a few ways under VC5
  (`/O1` vs `/O2`, `/MT`) and FID each into a small db; whichever variant hits is the
  one Monolith used. zlib 1.0.4 is tiny (~12 source files), so this is cheap.
- Alternative that needs NO rebuild: cross-binary match the zlib bodies between
  CLAW/MEDIEVAL/GRUNTZ (§4) and label by the banner strings — zlib's functions are
  small and string-anchored (`inflate_blocks`, `inflate_codes`, `inftrees`).

### 3.3 Miles / Smacker SDKs — irrelevant for matching

They are DLLs (RAD Game Tools). No static bodies in `.text` to match; only import
thunks. SDK headers (mss32.h / smack.h) would help *name the imported APIs*, not
match code. Low priority; obtain only for naming the import stubs if desired.

---

## 4. Function-ID tooling — the matching plan

### 4.1 Ghidra FunctionID (FidDb) — ORIGINAL PLAN (superseded; see note)

> **Implemented differently.** The library labels were ultimately produced by a
> custom **masked-byte COFF-signature matcher** (`scripts/gruntz/audit/fid/`, driven by
> `python -m gruntz.audit.fid_generate`) — NOT Ghidra FID. Its output is the tracked
> `config/library_labels.csv`. The Ghidra-FID route below is kept for context: it
> explains why no stock MSVC-5.0 fidb exists and what a signature db must capture.

> **Single-claim invariant (FATAL build gate) — TOTAL over the full generated symbol
> set.** Every retail RVA is claimed by EXACTLY ONE side: a **src reconstruction** (a
> GAME body/global) **xor** a `config/library_labels.csv` row (a carved LIBRARY body,
> excluded from the match denominator). Claiming the same RVA in both is a
> **double-claim on the same bytes**. `python -m gruntz.match.verify_library_overlap`
> (wired fatally into `gruntz build`, no allowlist) enforces
> `src-claims ∩ library_labels.csv = ∅` over the **FULL generated symbol set**
> (`build/gen/symbol_names.csv`), not just the `RVA()` macros the first cut
> parsed. A src claim is ANY of:
>
> - **rva-macro** — `RVA(0x.., 0x..)` (a reconstructed body);
> - **rva-symbol** — `RVA_COMPGEN(<rva>, <size>, <mangled>)` (a self-contained fn label,
>   e.g. a `??_G` scalar-deleting-dtor thunk the compiler synthesizes for a
>   polymorphic game class);
> - **data** — `DATA(0x..)` / `// @data-symbol:` (a named game global or `??_7` vtable).
>
> The RVA()-macro-only cut saw 0 overlap while the full set overlapped at 45 rows
> (P0d). The gate reads the generated set (regenerated by the `gruntz build` labels
> edge before the gate runs) and falls back to a src parse when that file is
> stale/absent, so it never passes vacuously.
>
> The FID matcher is a fuzzy signature matcher, so it emits false positives (a real
> function has ONE address, yet e.g. `??_G__non_rtti_object` recurred at ~189 RVAs,
> `__fpclear` at ~45, `__inc` at ~989); when a genuine game body/global is
> reconstructed on such an RVA, **prune the false CSV row** (the src is right).
> Conversely, if a hand-copy of Microsoft's code slips into `src/`, **carve it to the
> CSV** (the CSV is right) and call the real routine via `<Mfc.h>`. P0's reconciliation
> carved 23 HIGH-confidence NAFXCW bodies (CFile family, CWinApp profile/registry API,
> AfxFullPath / CFile::GetStatus/SetStatus, AfxGetInProcServer /
> _AfxGetMouseScrollLines) and pruned 42 FID false rows; **P0d** widened the gate to the
> full symbol set and pruned the last 3 (`??_GCGruntzWnd`, `??_GSoundDevice` — both
> 100% EXACT thunks mislabeled `??_G__non_rtti_object` — and the `g_buteTreeArg`
> global mislabeled `__fpclear`).
>
> **The one deliberate coexistence — vendored library.** zlib 1.0.4 is compiled from
> real vendored source (`vendor/zlib-1.0.4/*.c`, named for the delinker via
> `config/zlib_labels.csv`) **and** FID-identifies as library, so its ~42 functions
> sit in BOTH the generated set and `library_labels.csv`. `status.py` resolves this
> with **"claimed wins"** (a carve-out that is also reconstructed counts as a real
> target) and the names AGREE (`_deflate_stored == _deflate_stored`). This is a THIRD
> category — vendored library, source held — not a defect. The gate excludes it by
> **source** (the `config/zlib_labels.csv` vendored table), NOT by an RVA allowlist,
> so the xor invariant reads: src reconstruction **xor** library carve-out **xor**
> vendored-library body.

**Does Ghidra ship a usable FID db for MSVC 5.0? NO.** Ghidra's bundled FID
databases (in `ghidra-data/FunctionID`, auto-installed) are:
`vsOlder_x86.fidb`, `vsOlder_x64.fidb`, `vs2012_x86/x64`, `vs2015_x86/x64`,
`vs2017_x86/x64`, `vs2019_x86/x64`. The oldest, **`vsOlder_x86.fidb`, targets ~VS6
(MSVC 6.0) and newer — it does NOT cover MSVC 5.0 / MFC 4.2.** It may produce a few
incidental hits on shared CRT helpers but should not be relied on. **We must generate
our own FidDb from the VC5 libs.**

**Workflow to generate a FidDb from a `.lib` (concrete):**

1. Import the `.lib` as a Ghidra *program*. Ghidra's importer accepts MS COFF
   archives directly: `File ▸ Import File ▸ LIBCMT.LIB` (and again for `NAFXCW.LIB`,
   and the zlib `.lib`/objs once rebuilt). Each archive member becomes a function set.
   Do this for **each** lib into its own program (or a folder of programs).
2. Run **auto-analysis** on the imported program(s) so functions are created/named
   from the COFF symbol table (this gives FID the names to attach).
3. Create an empty FidDb: `Tools ▸ Function ID ▸ Create new empty FidDb…`
   (e.g. `vc5_x86.fidb`).
4. **Populate** it: `Tools ▸ Function ID ▸ Populate FidDb from programs…` — point at
   the imported `.lib` programs, set Library Family/Version/Variant
   (e.g. family "MSVC", version "5.0", variant "Release-MT"). FID computes the
   full-hash + specific-hash for each function body.
5. **Attach** the new FidDb to the project and (re-)run the **Function ID analyzer**
   on `GRUNTZ.EXE`. Matched functions get auto-named/commented with the library
   symbol. Use `Tools ▸ Function ID ▸ Choose active FidDbs…` to toggle.
6. Tuning: FID has a minimum-instruction threshold (default ~ a handful) so tiny
   stubs are skipped to avoid false positives; if MFC/CRT thunks don't match, lower
   it. Use the **FunctionID Debug** plugin to inspect why a function did/didn't match
   (hash mismatch ⇒ wrong lib build, see §3.1 caveat).

Reference workflow + scripts for vintage MSVC libs:
`github.com/mantrainfosec/SRE-FID_Generation_Windows`,
`github.com/moralrecordings/ghidra-fidb-dos-win16`, and the community fidb repo
`github.com/threatrack/ghidra-fidb-repo`. Build doc: `ghidra-data/FunctionID/
building_fid.txt`.

### 4.2 IDA FLIRT — N/A (we have no IDA), documented for completeness

If we had IDA: `pcf` (COFF) / `plib` would turn `LIBCMT.LIB` / `NAFXCW.LIB` /
`zlib.lib` into `.pat` pattern files, then `sigmake` → `.sig`, dropped in
`IDA/sig/pc/` and applied via `File ▸ Load file ▸ FLIRT signature`. MFC/CRT FLIRT
sigs for VC5 also circulate publicly. **We do not have IDA → use Ghidra FID (§4.1).**

### 4.3 Lumina — N/A

Lumina is an IDA-only (Hex-Rays) cloud function-metadata service. Not applicable
without IDA. No equivalent is used here.

### 4.4 Cross-game matching — the route for the ENGINE code (no public FID exists)

FID covers CRT/MFC/zlib (3rd-party libs). It does **NOT** cover the WAP32/"zlith"
engine or the Gruntz game code — no public signature db exists for those. But:

- CLAW.EXE, MEDIEVAL.EXE and GRUNTZ.EXE are the **same engine built with the same
  VC5 toolchain** (confirmed by shared DX/Miles/Smacker import stack + shared
  `C:\Proj\…` module layout + the lowercase-`z` WAP32 RTTI: `zPtrColl`, `zPTree`,
  `zDArray`, `zErrHandling`, `CWapObj`, `CWapX`, see strings-analysis §1(a)).
- So the WAP32 managers (DDrawMgr, DinMgr2, DsndMgr, NetMgr, RezSync, ButeMgr) and
  the `z*` container/runtime library are **near-identical bodies across the three
  binaries**. This functions as its own "signature set":
  1. Name the engine functions once in whichever binary is easiest (Claw has the
     most public RE prior art; Gruntz has the richest leaked source paths + RTTI).
  2. **Transfer names by cross-binary diff** — objdiff/BSim (Ghidra's built-in
     structural-similarity match) or a Diaphora-style diff — to propagate labels to
     the other two. Ghidra's **BSim** is the in-house tool: generate a BSim signature
     db from the named binary, query the others, accept high-confidence matches.
  3. zlib bodies also cross-match here (§3.2 alternative) — anchor with the banner
     strings, confirm by BSim across the three EXEs.
- **Net:** FID strips the 3rd-party library floor (CRT/MFC/zlib); cross-game BSim
  strips the shared-engine floor; what remains to hand-match is the genuinely
  Gruntz-specific game logic (CGruntzMgr, the grunt/toy/logic object model, the
  trigger/switch family, states, dialogs).

---

## 5. Recommendation — prioritized quick-win list

Goal: shrink the ~1.9 MB `.text` hand-matching surface as fast as possible. Order by
(value × ease), noting the toolchain-libs blocker.

**Rough composition of `.text` (estimate — to be measured once FID runs):** of the
~1.9 MB, a substantial chunk (plausibly a third to a half) is CRT+MFC+zlib library
code that is identical to stock libs. Static MFC 4.2 release alone is large (tens to
~150 KB+ of bodies depending on which classes are referenced); the static MT CRT adds
tens of KB; zlib is small (~15–25 KB) but trivially identifiable and string-anchored.
These are exactly what FID/cross-match should remove from the hand-matching budget.

Step order:

1. **zlib 1.0.4 — DO FIRST (no blocker, highest ease).**
   - Download `zlib-1.0.4.tar.gz` from zlib.net/fossils. Recompile under VC5
     (`/MT`, release) → FidDb; OR just label the ~dozen zlib functions directly via
     the banner strings + cross-game BSim. Tiny, string-anchored, instant confidence.
   - Removes a small but unambiguous block; clears the WWD/REZ loader's decompressor
     from the "unknown" pile.

2. **Static CRT (`LIBCMT.LIB`) via generated FID — needs VC5 toolchain libs.**
   - Blocker: requires the VC5 `.lib`s (we're packaging the toolchain anyway → free
     once that lands). Import `LIBCMT.LIB` → FID db → attach → analyze.
   - Big win: removes startup/CRT/heap/printf/locale/math/iostreams bodies — the
     classic "looks like game code but isn't" noise. (Already string-anchored by the
     `R60xx`/heap/locale tables, so spot-verification is easy.)

3. **Static MFC 4.2 (`NAFXCW.LIB`) via generated FID — needs VC5 toolchain libs.**
   - Same blocker (VC5 install). Largest single removable block (the whole CWnd/
     CDialog/CDocument/CArray/CString machinery). Use the *VC5* `NAFXCW.LIB`
     specifically; if hit-rate is low, retry with the other VC5 SP's lib (§3.1
     caveat). Verify with the `…42s` Afx strings + the MFC RTTI names already mapped.

4. **Cross-game BSim for the WAP32 engine — parallelizable, no toolchain dep.**
   - After (or alongside) FID, run Ghidra BSim across CLAW/MEDIEVAL/GRUNTZ to label
     the shared `z*` runtime + DDraw/Din/Dsnd/Net/Rez/Bute managers. This is the only
     route for engine code (no public FID). Names transfer all three ways.

5. **Hand-match the remainder = the real prize.**
   - What's left after 1–4 is Gruntz-specific game logic: `CGruntzMgr`, the grunt/
     toy/powerup/logic object model, the large trigger/switch family, the state
     machine, multiplayer dialogs. This is where human RE effort should concentrate.

**Blockers / unconfirmed flags:**

- **BLOCKER:** Steps 2 & 3 need the **VC++ 5.0 `.lib` files** (the toolchain tarball).
  Until then, only zlib (step 1) and cross-game BSim (step 4) can proceed.
- **UNCONFIRMED (codegen exactness):** which VC5 service pack (SP1/SP2/SP3) built
  Gruntz — affects FID hit-rate for LIBCMT/NAFXCW. Try the SP that matches the Rich
  header build first; fall back to other SP libs if hashes miss.
- **UNCONFIRMED (precise `.text` library fraction):** the third-to-half estimate is
  inference from section sizes + the lib set; the real number comes out of the first
  FID + BSim pass. Re-measure then.
- **CONFIRMED:** CRT static (no msvcrt.dll), MFC 4.2 static release (`…42s`,
  NAFXCW), zlib 1.0.4, MSVC 5.0 toolchain (LINK 5.10 / Rich VC5), the full DLL import
  list, and that Ghidra ships NO MSVC-5.0 FID db (oldest = `vsOlder` ≈ VS6).

---

### Sources

- Ghidra FunctionID docs & bundled-db list:
  https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/FunctionID/src/main/doc/fid.xml ,
  https://github.com/NationalSecurityAgency/ghidra-data/tree/master/FunctionID
- FID generation workflow refs:
  https://github.com/mantrainfosec/SRE-FID_Generation_Windows ,
  https://github.com/moralrecordings/ghidra-fidb-dos-win16 ,
  https://github.com/threatrack/ghidra-fidb-repo
- zlib 1.0.4 archive: https://zlib.net/fossils/ (zlib-1.0.4.tar.gz, 1996-07-24)
- VC++ 5.0 / MFC 4.2 libs: https://jeffpar.github.io/kbarchive/kb/165/Q165685/ ,
  https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features
- Local: `objdump -p GRUNTZ.EXE`, `GRUNTZ.strings.all.txt`, `docs/strings-analysis.md`
