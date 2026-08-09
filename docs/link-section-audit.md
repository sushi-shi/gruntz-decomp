# Link section audit — why the candidate `.text` and `.rdata` differ from retail

`ninja candidate` (`gruntz link`) produces a real link: 344 objs + 19 libraries,
**0 unresolved externals, 0 duplicate symbols, no `/FORCE`**. The image is 2,358,272 B
against retail's 2,511,872 B. This document accounts for the two sections that hold
code and read-only data. `.data` / `.rsrc` / `.reloc` are audited elsewhere.

Reproduce everything here with

    python -m gruntz.audit.link_sections [--selftest] [--thunks] [--gaps N]

## Method: partition both images the same way

A section total tells you nothing on its own, so both images are cut into the same
regions and each region is measured with a detector that needs **no symbol table** —
retail and candidate go through identical code.

`.text` has four regions, in link order:

| region | detector |
|---|---|
| incremental-link thunk band | the leading run of `E9 rel32` thunks with `0xCC`/`0x90` filler |
| plain `.text` | everything up to the MFC AFX groups |
| `.text$AFX_AUX` … `.text$AFX_TERM` | MFC's `alloc_text`-segmented code |
| `.text$x` | the `/GX` unwind funclets |

`.rdata` has three groups: `.rdata` (const data, vftables, FP/string pools),
`.rdata$r` (RTTI `??_R1/R2/R3/R4`), `.xdata$x` (EH tables — every record opens
`0x19930520`).

The candidate's boundaries come straight from the link `.map`. Retail's are **derived**
and then checked against retail's own data (`--selftest`):

* the `.rdata` group detector is run on the *candidate* and must reproduce the
  candidate `.map` exactly — it does, for both boundaries;
* the retail AFX block start is the retail address of the candidate's first
  `.text$AFX_AUX` member (via FID). Walking the candidate's group sizes from there,
  **9 of 9** group starts land on the first FID-identified MFC function of the next
  group and/or on the far edge of an uncarved hole in `config/retail/functions.tsv`.

## `.text`: −34,949

| bucket | retail | candidate | delta |
|---|---:|---:|---:|
| ILT incremental-link thunk band | 26,992 | 45,760 | **+18,768** |
| plain `.text` | 1,748,322 | 1,694,758 | **−53,564** |
| `.text$AFX_*` (MFC) | 153,124 | 153,124 | **0** |
| pad `AFX` → `$x` | 10 | 6 | −4 |
| `.text$x` (EH funclets) | 58,731 | 58,582 | −149 |
| **total** | **1,987,179** | **1,952,230** | **−34,949** |

### The largest single contributor is the thunk band, and it is a link-line fact

Retail emits **2,695** `E9` thunks; we emit **4,575**. Each costs 10 B (5 code + 5
filler), so `1,880 × 10 = 18,800` — the whole `+18,768`. `/INCREMENTAL:YES` creates one
thunk per function defined in an object **named on the link line**; functions arriving
from a `.LIB` get none. Our 4,575 is exactly the number of our publics in plain `.text`,
so all 344 objs are on the command line. Retail's 2,695 says a large part of the game
came in as static libraries — the `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr}` modules
(see `docs/tu-partition-brief.md`). Because the band is *bigger* than retail's, the real
code shortfall is not 34,949 but **53,749**.

### …and the thunk band names exactly which modules were static libraries

`--thunks` decodes every `E9 rel32` in retail's band. The 2,695 targets span
`0x007970..0x11c860` and the boundary is **hard**:

* src claims at or below `0x11c860`: 2,525, of which **2,520 are thunked** (the 5
  exceptions are `static`-linkage callbacks such as `_BattlezMapComboEditProc@16`);
* src claims above `0x11c860`: 1,811, of which **0 are thunked**.

So retail's `.text` is `[thunk band][command-line .obj code][static .LIB code][MFC AFX]
[.text$x]`, and the module split falls straight out:

| module | in a command-line `.obj` | in a static `.LIB` |
|---|---:|---:|
| Gruntz | 2,360 | 223 |
| Io | 38 | 0 |
| Net | 78 | 56 |
| Wap32 | 16 | 41 |
| Image | 9 | 122 |
| Utils | 7 | 18 |
| Wwd | 4 | 210 |
| Rez | 4 | 72 |
| DDrawMgr | 2 | 572 |
| Bute | 1 | 190 |
| Dsndmgr | 1 | 152 |
| zlib / DinMgr2 / Font / Crypto | 0 | 63 / 59 / 21 / 17 |

This is the direct measurement behind `docs/tu-partition-brief.md`'s leaked-path guess:
**DDrawMgr, Dsndmgr, DinMgr2, Wwd, Bute, Image, Rez, Font, Crypto and zlib were static
libraries; the Gruntz project and Io were on the link line; Net and Wap32 were split.**
The 223 Gruntz claims sitting above the cut are an attribution signal worth chasing —
either the unit is mis-homed or the function is a COMDAT the linker grouped with the
library block.

### MFC's AFX-segmented code is identical

153,124 B on both sides, and the `0xCC` filler inside the block matches to 33 bytes
(60,184 vs 60,217). A byte-compare of the two blocks agrees on 99,837 / 153,124 bytes;
every disagreement is a relocated operand or a rel32 displacement, plus a **member
reordering**: `AFX_AUX`, `AFX_COL2`, `AFX_CORE4` and `AFX_TERM` have every paired
function at the *identical* block offset (0 order inversions), while `COL1`, `CORE3`,
`INIT` and parts of `CORE1`/`CORE2` hold the same members in a different order. Same
objects, same sizes, different intra-group order — a link-order artifact, not content.

### `.text$x` matches to 0.25%

58,731 vs 58,582 (−149). Corroborated from the other side: retail has **972** EH
records, we have **975**. `/GX` coverage is right.

### Inside plain `.text`: −53,564

| | retail | candidate | delta |
|---|---:|---:|---:|
| `0xCC` incremental-link filler | 387,433 | 376,445 | −10,988 |
| non-filler content | 1,360,889 | 1,318,313 | **−42,576** |

Both images are ~22% filler; this is what an incremental link looks like. Measured per
function on our side: the linker leaves a multiple of 16 after each contribution, 16 B
most often, 88 B mean.

The content delta breaks down as:

| sub-bucket | delta | how it is measured |
|---|---:|---|
| the 4,265 claimed functions with a known retail extent | **−5,730** | our COMDAT section size vs `roundup(retail extent, 16)`; **3,975 are byte-for-byte the same length** |
| bodies we emit that retail has no claim for (174 fns) | +7,224 | COFF, exact |
| claimed but retail extent unrecorded (71 fns, mostly zlib statics) | +17,472 | COFF, exact |
| import thunks (`FF 25`) | +36 | byte scan; 447 vs 453 thunks |
| retail content in holes Ghidra never carved | −102,920 | see below |

cl emits **one COMDAT `.text` section per function** (every `.text*` section in our 344
objs holds exactly 0 or 1 function symbol), so a function's compiled size is its
section's raw size — exact, with no symbol-offset differencing needed.

### There are no missing bodies at the claimed-function level

Of retail's 4,336 src-claimed functions, **28 have no candidate public and 27 of those
are zlib statics**; the one real absentee is 134 B in `netmgr`. Every function we claim
is emitted, and 3,975 of them are exactly retail's length.

### What *is* missing is code that was never claimed

250 holes in retail's plain `.text` carry **102,920 non-filler bytes** that no carved
function, FID label or compiler-helper row accounts for. The largest:

| retail RVA | hole | non-filler | what it is |
|---|---:|---:|---|
| `0x0006f16f` | 26,737 | **21,070** | unreconstructed `CTriggerMgr` code between `GooWellMgr.cpp` and `TriggerMgrHitTest.cpp`; real x86 with ~0x2a8 stack frames |
| `0x0018fa96` | 6,930 | 6,902 | `dinput.lib` (`dilib1/2/4.obj` carry 6,904 B of `.text`) |
| `0x0008a04a` | 5,878 | 1,557 | |
| `0x000f5483` | 3,181 | 1,499 | |

`--gaps N` prints this worklist. It is the honest "functions retail has that we have not
reconstructed" list, and `0x6f16f` alone is 40% of the true code shortfall.

### Library code is not a source of the delta

Retail's FID-labeled library code in plain `.text` (excluding zlib, which is ours) is
111,839 B — a lower bound, FID coverage being partial. Our library region is
`0x180df0..0x1a9ee6` = 168,182 B, of which 135,375 B is content; the archives say the
pulled members carry `libcmt` 96,721 + `libcimt` 22,539 + `dinput` 6,904 + `nafxcw`
non-AFX ≈ 8,031 = 134,195 B. The two agree, so the CRT/MFC/DX pull is the same.

## `.rdata`: +7,904 → +7,312

| bucket | retail | candidate | delta |
|---|---:|---:|---:|
| `.rdata` (const, vftables, pools) | 44,832 | 50,560 | **+5,728** |
| `.rdata$r` (RTTI) | 23,472 | 23,472 | **0** (was +536) |
| `.xdata$x` (EH tables) | 66,776 | 68,360 | +1,584 |
| **total** | **135,080** | **142,392** | **+7,312** |

The `.rdata` excess is, in order: **4,368 B of DirectX-SDK GUID drift (76%)**, ~450 B of
wrong-`const` string duplicates (8%), 1,584 B of EH-table slack, and ~900 B unattributed.
The RTTI group was +536 and is now exactly 0.

Retail's boundaries are `.rdata$r` at `0x1f1f20` and `.xdata$x` at `0x1f7ad0`; both are
preceded by zero padding and open with the expected structure (a `??_R1` base-class
descriptor, and `19930520 00000001 ...`), and the three spans sum to exactly 135,080.

### `.rdata$r` was +536; it is now exactly 0

Retail's `.data` holds **231** RTTI type descriptors, ours held **237**. The six extras
were the whole `CFader` family, and retail's own vtables refute them: `??_7CFaderFlat@@6B@`
(`0x1f07f8`), `CFaderRadial` (`0x1f0810`), `CFaderSine` (`0x1f0848`), `CFaderLight`
(`0x1f0870`) and `CFaderShape` (`0x1f0890`) have **no Complete Object Locator at `[-4]`** —
that word holds FP-pool constants (`0x3f800000` = 1.0f, `0x3fc90fd0` ≈ π/2, `0x40000000`
= 2.0f), where a class with RTTI (`CUserLogic`, `CToyPeek`) has a pointer into `.rdata$r`.

`FaderEffects.cpp` was therefore compiled **without `/GR`**. Its `units.toml` flags were
`cpp-rtti`; changed to `cpp`. Result: `.rdata$r` 24,008 → 23,472, **exactly retail**;
RTTI classes 237 → 231, **exactly retail**; `.data` −224 B; `.reloc` −92 B. Cost: none —
function scoring is unchanged at 3498/4325 exact, 92.20% fuzzy.

### `.rdata` proper: +5,728, of which 4,368 is DirectX SDK drift

Every GUID is a 16-byte constant, so each of the 379 `dxguid.lib` GUIDs our link keeps
can be looked up **by value** in retail's whole image: **273 are absent**, i.e. 4,368 B
of `.rdata` retail simply does not contain. The absentees are all DX6-era interfaces
(`IDirectDrawFactory2`, `IDirectDrawOptSurface`, the VideoPort / MotionComp / D3DCallbacks2
families). `dxguid.lib` keeps its GUIDs in one archive member, so pulling any pulls all —
and our DX6 SDK member simply has more of them than the SDK retail built against. This is
toolchain drift, not a source defect; it accounts for 76% of the `.rdata` excess.

The next-largest known contributor is small: **30 named `static const char[]` that we put
in `.rdata` where retail has the same string once, pooled, in `.data`** — the rows
`gruntz.build.data_manifest --report` withholds as *"our rdata copy cannot be the data
literal `??_C@…` it is pinned onto"* (`GRUNTZ_`, `LightFx`, `SingleAnimation`,
`BABYWALKERGRUNT`, `GRUNTZ_NORMALGRUNT_IMPACTMM3`, …). Decoded, they are ~409 B of string
plus 4-byte alignment, so **~450 B — about 8% of the excess, not the cause of it.** They
are still real duplicates and worth dropping the `const` on; that work belongs to the
`.data` lane, which owns the literals they collide with.

That leaves ~900 B (1.8% of the group) unattributed — within vtable/pool sizing noise.

### `.xdata$x`: +1,584 on 975 vs 972 records

Decoding every `__ehfuncinfo`:

| | retail | candidate |
|---|---:|---:|
| records | 972 | 975 |
| Σ `maxState` (unwind-map entries, 8 B each) | 2,666 | 2,702 |
| Σ `nTryBlocks` (20 B each) | **19** | **19** |

The try-block count is *identical* — the whole game has 19 `try` blocks and we have all
of them. The +1,584 is 3 extra records (+84 B of header), 36 extra unwind states (+288 B),
and ~1,200 B of inter-contribution padding. Consistent with the `.text$x` funclet delta of
−149 and with `/GX` being on project-wide.

## Section start addresses

Nothing but `.text`'s size moves `.rdata`. In both images
`rdata_rva == 0x1000 + roundup(text_vsize, 0x1000)` exactly: retail
`0x1000 + roundup(1,987,179) = 0x1e7000`, candidate
`0x1000 + roundup(1,952,230) = 0x1de000`. The 36 KB by which our `.rdata` starts early is
purely the `.text` shortfall; no other cause.

## Under-modelled data: the trap this audit is built to catch

objdiff only compares the bytes a datum **claims**, so modelling `struct { float x, y; }`
as a bare `float x` scores 100% while the trailing bytes are never looked at — and the
link is where it shows, as a short section. Sweeping the 904 pinned retail `.rdata`
addresses for unclaimed slack before the next pin finds 30,078 B, but almost all of it is
either genuine zero padding (retail's `.rdata` carries incremental-link zero filler
between contributions — e.g. 44 zero bytes between `??_7CBattlezDlg@@6B@` and the next
vtable) or library data we have not pinned at all (the MFC message-map block at
`0x1eb068`, 13,628 B; the CRT export-name pool after `?g_dot@@3PBDB`, 3,386 B).

`--undersized N` prints the ranked list (149 of 909 pinned addresses, 29,506 B). Most of
it is library data we never pinned — the MFC message-map block at `0x1eb068` (13,628 B),
the CRT export-name pool after `?g_dot@@3PBDB` (3,386 B), MFC/CRT `.rdata` behind
`??_7CMenuItem2@@6B@` (5,612 B, which is simply the unpinned tail of the group). The rows
that matter are the ones where **the slack is more of the same datum**.

### Worked example: `g_multiBootyGeom` is a third of the real table

`?g_multiBootyGeom@@3PAY03$$CBUCoord@@A` (`bootystateactivate`, `0x1e9078`) is declared
`Coord[N][4]` and claims `0x100` = 32 `Coord`. objdiff compares those 256 bytes, finds
them perfect, and stops. Retail keeps going for another **560 bytes** — right up to the
next pin, `?g_secretChars@@3PBDB` at `0x1e93a8` — and it is plainly more of the same:

    0x1e9078  190 437  306 437  422 437  538 437     <- what we model: X in {190,306,422,538}
    ...
    0x1e9178  200 415  284 465  316 415  400 465     <- 48 Coord, X in {200,284,316,400,
    0x1e9198  432 415  516 465  548 415  632 465        432,516,548,632} - a WIDER layout
    ...                                                 (6 Y levels x 8 columns)
    0x1e92f8   50  87  390 115  166  87  506 115     <- 8 Coord, a different shape
    0x1e9338   45 155  175 215   50 198  180 258     <- 28 Coord, a third shape
    ...
    0x1e93a8  ?g_secretChars@@3PBDB

Three more geometry tables (384 + 64 + 112 = 560 B) that no TU declares. This is the
canonical form of the trap: **we chose the size, so the metric agrees with us.** Only the
link notices, because the section comes out short. `?g_sineOne@@3MB` (`0x1f0864`, claim 4,
slack 32 opening `0x40000000` = 2.0f) is a small instance of the same thing.

## What this changes

* `.rdata$r` and `.text$AFX_*` are now **exact-by-construction buckets** — if either moves,
  something real broke, and `--selftest` says whether the boundary derivation still holds.
* The `.text` shortfall is **not** missing bodies and **not** library drift. It is, in order:
  the ILT band being 1,880 thunks too long (a link-line/static-library question, +18,768),
  ~103 KB of retail code in never-carved holes that we have not reconstructed, and −5,730 B
  of residue across the 827 non-exact functions.
