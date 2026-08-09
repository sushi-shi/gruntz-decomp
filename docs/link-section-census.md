# The candidate link, section by section — a classified census

`ninja candidate` is a real link (344 objs + 19 libs, **0 unresolved externals, 0
duplicate symbols, no `/FORCE`**), so the candidate EXE's section table is an oracle.
Every `retail − candidate` byte below is in a named bucket; nothing is left as
"the sections differ by N".

Reproduce with `python -m gruntz.audit.section_census [--bss] [--reloc]`.
Measured 2026-08-09 at `f24f7ca4c` (function scoring 3,498/4,325 exact, 92.20% fuzzy).

## The table

Four link models. `candidate` is the default (`ninja candidate`); `englib` archives the
engine modules into static `.lib`s (`link --engine-lib`); `flat` is `/INCREMENTAL:NO`.

| virtual size | retail | candidate | englib | flat |
|---|---:|---:|---:|---:|
| `.text` | 1,987,179 | 1,952,230 | 1,929,569 | 1,458,210 |
| `.rdata` | 135,080 | 142,984 | 141,767 | 126,918 |
| `.data` | 763,820 | 831,276 | 741,516 | 691,028 |
| `.idata` | 15,169 | 15,169 | 15,169 | 0 |
| `.rsrc` | 123,260 | **123,260** | 123,260 | 0 |
| `.reloc` | 113,105 | 109,349 | 109,083 | 97,388 |
| — `.data` raw (initialized) | 136,192 | 137,216 | 137,216 | 111,104 |
| — `.data` zero-fill (`.bss`) | 627,628 | 694,060 | 604,300 | 579,924 |
| FILE | 2,511,872 | 2,482,176 | 2,458,112 | 1,795,584 |
| % of retail | 100.00% | **98.82%** | 97.86% | 71.48% |

## 0. Retail is an incremental link, and that dominates every delta

Three independent proofs: the `E9 rel32` ILT thunk band at the top of `.text`, the
separate padded `.idata`, and — decisively — **where retail's CRT init tables sit**:

| | `.CRT$XCA` | `$XCC` | `$XCL` | `$XCU` |
|---|---|---|---|---|
| retail `.data` + | 0x0000 | **0x0104** | **0x0210** | **0x0320** |
| ours, `/INCREMENTAL:YES` | 0x0000 | **0x0104** | **0x0210** | **0x0320** |
| ours, `/INCREMENTAL:NO` | 0x0000 | 0x0004 | 0x000c | 0x0018 |

Two `__xc_*` pointers do not need 260 bytes. That is growth room, and retail has it at
the identical offsets.

**But the linker pads only what a relink might have to REPLACE — objects named on the
LINK LINE, never a member pulled out of a `.lib`.** Measured over the same objects,
`.bss`, flat vs incremental:

```
107 command-line objects   570,724 -> 684,724   (+114,000, +19.97%)
 27 library members          9,408 ->   9,408   (      +0,   0.00%)
```

So a section delta is meaningless until both sides use the same library/loose split.

## 1. `.rsrc` — was −123,260, is now **0**

We emitted no resource section at all. The toolchain ships `cvtres.exe` 5.00.1668.1 and
`link.exe` takes a `.RES` directly, but **no `rc.exe`** — so `gruntz.build.rescomp`
writes the Win32 `.RES` container itself, and `configure.py` wires it into
`ninja candidate`.

`python -m gruntz.build.rescomp verify`:

* `.rsrc` vsize **123,260 = 123,260**, all **75/75** resources present, all 75 payloads
  **byte-identical**;
* every differing raw byte lies inside the **75**
  `IMAGE_RESOURCE_DATA_ENTRY.OffsetToData` dwords, each at an identical
  section-relative site and shifted by exactly the section-placement delta. **Zero
  unexplained bytes.** (The count of differing bytes moves with the delta as the
  earlier sections grow — 139 at +0x9000, 140 at +0xC000 — the invariant is the
  classification, re-provable any time with `rescomp verify`.)

### What is in it, and the authored-vs-copied line

| type | # | bytes | share | provenance |
|---|---:|---:|---:|---|
| STRING | 22 | 68,130 | 69.9% | authorable |
| DIALOG | 31 | 18,544 | 19.0% | authorable (14 `DIALOG`, 17 `DIALOGEX`) |
| ICON | 8 | 7,168 | 7.4% | **copied** |
| DLGINIT | 2 | 1,252 | 1.3% | authorable (MFC `RT_DLGINIT`, type 240) |
| VERSION | 1 | 1,056 | 1.1% | authorable |
| CURSOR | 3 | 924 | 0.9% | **copied** |
| ACCELERATOR | 1 | 248 | 0.3% | authorable |
| GROUP_ICON | 4 | 136 | 0.1% | **copied** |
| GROUP_CURSOR | 3 | 60 | 0.1% | **copied** |
| **total** | **75** | **97,518** | | **89,230 authorable (91.5%) / 8,288 copied (8.5%)** |

Plus 5,791 B of directory tables, name strings and data entries, and **19,951 B (16.2%)
of trailing zero fill** — which the generated section reproduces exactly.

*Authorable* means every byte is a deterministic function of `.rc` TEXT a person wrote:
dialog templates, string tables, the accelerator table, `VERSIONINFO`, the MFC
`DLGINIT` blobs. *Copied* is the icon and cursor image bits and the group directories
computed from them — art files we do not have.

**The authorable 91.5% IS source now (2026-08-09): `src/Gruntz/Gruntz.rc`.** It is
tracked, genuine rc.exe grammar — 22 STRINGTABLEs, 31 DIALOG/DIALOGEX templates, the
ACCELERATORS table, `VERSIONINFO` (fully decoded to `FILEVERSION`/`VALUE` statements),
and both `DLGINIT` raw-data streams — and `rescomp` (there is no `rc.exe`; rescomp is
the compiler) parses it and encodes the payloads. The 57 authorable blobs are **deleted
from the repo**; only the 18 art blobs remain in `config/retail/rsrc/data/`
(`provenance` column: `copied`). Two standing proofs:

* `rescomp rc --roundtrip` — retail payload → model → **`.rc` text → parse** → bytes:
  **57/57 re-encode BYTE-IDENTICAL through the text** (**89,230 of 89,230 B**).
* `rescomp check` — a normal-tier `gruntz build` gate: recompiles the tracked `.rc`
  and byte-compares every payload (compiled + carried art) against the retail image,
  statement order included. The source claim is re-proven on every gated build.

The art 8.5% can only ever be carried — the game's shipped `Gruntz.REZ` was checked
(2026-08-09) and contains no `.ico`/`.cur`/RT_ICON data at all; its `CURSOR`-named
entries are in-game PID sprite frames, a different pixel format. The Windows shell art
exists nowhere but the EXE itself.

Two facts the codecs recovered on the way: the DIALOGEX item header is 24 bytes, not 28
(`helpID`/`exStyle`/`style`/`x,y,cx,cy`/`id`), and MFC's `RT_DLGINIT` record header is
`WORD idc; WORD msg; DWORD len` with `msg` **WM_USER-relative** (`0x0403` =
`CB_ADDSTRING`). `VERSIONINFO` says the build is **`1, 0, 0, 76`**, "Gruntz.EXE",
"Copyright © 1998, Monolith Productions Inc.".

### Recovered structure: the payload order is the original `.rc` statement order

`rc.exe` emits resources in statement order and `cvtres`/`link` keep it (only the
*directory* is sorted), so retail's payload addresses read back the source file:

```
KEYBOARD ICON · JOYSTICK ICON · GRUNTZ ICON · STOPCROSS ICON
GRUNTZ ACCELERATORS · VERSIONINFO
31 dialogs (ERROR, DEBUG_POSITION, MESSAGE, BRAND, CUSTOM_WORLD, CONFIG_SETTINGS, …)
ARROW CURSOR · GRUNTZ_NO CURSOR · GRUNTZ CURSOR
DLGINIT 192 · DLGINIT 197
22 STRINGTABLE blocks (ids 32771…33063+)
```

and each `GROUP_ICON`'s image count (2/1/4/1) matches the `ICON`s immediately before it,
accounting for all 8. The six numeric dialogs (192, 194, 195, 197, 203, 205) are MFC
`IDD_` resources; 192 and 197 are the two that carry `DLGINIT`.

## 2. `.data` +67,456 — **not initialized data, and not "we emit more"**

Split by the only split that matters:

| bucket | candidate | englib |
|---|---:|---:|
| initialized (raw) | **+1,024** | +1,024 |
| `.bss` zero-fill | **+66,432** | **−23,328** |
| total | +67,456 | −22,304 |

**98.5% of the excess is `.bss`.** Initialized `.data` is +1,024 — one 0x200
`FileAlignment` block, so between 513 and 1,024 bytes of real content, on 136 KB.

And the `.bss` excess is the incremental-padding artifact, not data:

| | bytes |
|---|---:|
| our `.bss` content (measured by a `/INCREMENTAL:NO` relink) | 580,132 |
| growth padding on 107 command-line objects (+19.97%) | **+114,000** |
| = our `.bss` | 694,060 |
| retail's `.bss` | 627,628 |

**Largest single contributors to the raw +67,456**, both pure padding:

| object | flat | candidate | englib | slack after it in retail |
|---|---:|---:|---:|---|
| `imagepolyclip.obj` | 235,016 | 282,016 (**+47,000**) | 235,016 | **2 B** |
| `ddsurface.obj` | 197,176 | 236,616 (**+39,440**) | 197,176 | **0 B** |
| `fileimage.obj` | 6,276 | 7,508 (+1,232) | 6,152 | 1,024 B |
| `dircellmethods.obj` | 82,016 | 98,424 (+16,408) | 98,424 | ~20% (padded) |

Retail leaves 2 and 0 bytes after its two largest `.bss` contributions, so retail did
**not** pad them — they were **library members**, while `dircellmethods` (a
`src/Gruntz` TU, on retail's link line) carries the full ~20%. That is an independent
`.bss` corroboration of the standing *engine modules were static `.LIB`s* finding, and
`--engine-lib` reproduces retail's spans exactly (282,016 → 235,016; 236,616 → 197,176).

**So, modelled the way retail's build actually was, `.data` is not +67,456 over; it is
−22,304 UNDER, of which `.bss` is −23,328.** We are missing about 3.7% of retail's
uninitialized data — real, unreconstructed globals. That is the honest worklist; the
+67 KB was a link-configuration artifact.

### The 30 wrong-`const` declarations are not part of this

`python -m gruntz.build.data_manifest --report` names 30 literals whose `.rdata` copy
cannot pair with the `.data` literal they are pinned onto (VC5 pools string literals
without `/GF`, so a `const` static lands in `.rdata` and can never fold onto a `.data`
literal). Their **total payload is 409 bytes** (2 to 29 bytes each: `"rb"`,
`"NORMALGRUNT"`, `"GRUNTZ_NORMALGRUNT_IMPACTMM3"`, …). They push bytes from `.data`
*into* `.rdata` — the opposite direction from the `.data` excess — and at 409 B they
cannot be a material part of any of these deltas. They are an `.rdata` correctness
worklist, not a `.data` size one.

## 3. `.reloc` −4,097 — fully explained, and **not** a missing-relocation problem

First, the section is not the table. Both sides carry a zero tail:

| | section vsize | reloc directory | trailing fill | page blocks | HIGHLOW | `ABSOLUTE` pad entries |
|---|---:|---:|---:|---:|---:|---:|
| retail | 113,105 | 93,612 | 19,493 | 489 | 44,604 | 246 |
| candidate | 109,349 | 90,108 | 19,241 | 501 | 42,792 | 258 |

The **−3,504 B directory delta is exact arithmetic, residual zero**:

```
-3,504 = -1,812 fixups x 2 B  (-3,624)
       +    12 blocks  x 8 B  (   +96)
       +    12 extra 2-byte ABSOLUTE alignment entries (+24)
```

### Is the shortfall proportional to `.text`'s?

At face value **no**: `.text` is −1.76% by size but −4.82% by fixup count. Bucketed, the
disproportion is entirely outside our reconstruction:

| bucket | retail | candidate | delta |
|---|---:|---:|---:|
| `.text` fixups inside the 4,264 function bodies present on BOTH sides | 22,446 | 22,203 | **−243 (−1.08%)** |
| `.text` fixups outside them (statically-linked CRT/MFC + unattributed) | 9,745 | 8,437 | **−1,308 (−13.4%)** |
| `.rdata` | 10,675 | 10,754 | +79 |
| `.data` | 1,738 | 1,398 | −340 |

* Inside our own code the deficit is **−243 over 81 functions**, and **80 of the 81 are
  already scored non-exact** (42 at 80–95%, 33 below 80%, 5 at 95–100%) — every missing
  relocation is a function objdiff already flags. The one exception is not a defect:
  `??_GzPTree@@UAEPAXI@Z` scores 100% but carries **+2** fixups, because it is a COMDAT
  and the link's fold winner came from `buteglobals.obj` while the claim (and objdiff's
  scored copy) is `chatboxowner`'s. At −1.08% the deficit is *below* the code shortfall,
  so **we are not systematically emitting absolute addresses as literals.** The heaviest
  single unit is `butemgr` (−123 across seven ~1 KB `CButeMgr::Set*` methods sitting at
  90–95% fuzzy, each short 10–18 fixups) — a per-TU reconstruction gap already on the
  match worklist, not a global defect.
* The −1,308 outside is library code: the same `LIBCMT`/`NAFXCW` archives, but not
  the same member set (we pull 235,035 B of library `.text`; retail's differs).
* `.data` −340 is retail-relocated words we do not emit **at all**, i.e. pointer data
  not yet reconstructed. `python -m gruntz.audit.data_relocs` reports **0 WRONG** over
  8,809 adjudicated words, so nothing we *do* emit points anywhere retail's does not.

So: the `.reloc` shortfall is proportional-or-better within the reconstruction, and the
residue is library composition — there is no hidden family of un-relocated absolute
addresses.
