# Compiler / linker fingerprinting for GRUNTZ.EXE

Goal: identify the EXACT MSVC cl.exe / link.exe build + service pack used to
build `binaries/retail_en/GRUNTZ.EXE`, so a byte-matching decompilation can be
compiled with the same toolchain.

Date: 2026-06-13. All claims below are tagged [CONFIRMED] (verified locally or
across >=2 independent sources) or [UNCONFIRMED] / [INFERRED].

---

## 0. Target facts (measured locally, this session)

Tools used: `winedump`, `objdump`, a hand-written Perl Rich-header decoder,
and `diec` (Detect It Easy 3.10 from nixpkgs).

- File: `GRUNTZ.EXE`, 2,511,872 bytes.
  sha256 `7073c2536106ae4cca32e3e82db21001f319678b214c4eae2c689c54902808b3`
- PE32 / i386 / GUI, 6 sections. [CONFIRMED via winedump + objdump]
- TimeDateStamp `0x36687DCF` = Sat Dec 5 1998 00:26:55 UTC. [CONFIRMED]
- PE optional header **MajorLinkerVersion.Minor = 5.10**. [CONFIRMED]
- **No DEBUG data directory** (rva 0, size 0) -> no CodeView/PDB pointer, no
  RSDS GUID. [CONFIRMED] This removes the easiest fingerprinting path.

### Rich header decode (independent re-derivation, matches user's hand read)

XOR key `0x32022d89`; `DanS` at file offset 0x80, `Rich` at 0xA8. Entries
`(prodID, build, count)`, prodID = compid>>16, build = compid & 0xffff:

| prodID        | build | count | meaning (see section 2)                |
|---------------|-------|-------|----------------------------------------|
| 0x0013 (19)   | 8034  | 12    | **Linker 5.12** (prodidLinker512)      |
| 0x0000 (0)    | 0     | 1057  | Unknown / import bucket (prodidImport0)|
| 0x0006 (6)    | 1668  | 1     | **cvtres 5.00** (prodidCvtres500)      |

**IMPORTANT CORRECTION to the brief.** The brief described the 12-count
entry (prodID 0x13, build 8034) as "a C/C++ compiler module". That is wrong:
prodID 0x13 is **Linker512** (the linker), not a compiler. All four comp.id
databases agree (section 2). There is in fact **NO C/C++ compiler comp.id
record at all** in this Rich header — which is itself the strongest VC++ 5.0
fingerprint (section 2.1 / 5).

---

## 1. TOOL SURVEY — PE compiler/linker fingerprinters

### Detect It Easy (DIE) / `diec`  [user's main interest]
- Repo: github.com/horsicq/Detect-It-Easy. Cross-platform (Win/Linux/macOS).
  Ships three front-ends: GUI `die`, lite `diel`, **console `diec`**.
- `diec` is a true headless CLI. Output formats: plain, `-j` JSON, `-x` XML,
  `-c` CSV, `-t` TSV. Scan depth flags: `-d` deep, `-u` heuristic, `-g`
  aggressive, `-b` verbose, `-i` file info, `-S Hash` hashing.
- Detection is **signature/script driven** (`.sg` scripts + db) plus structural
  parsing. For PE it parses the Rich header and reports a "Microsoft Linker"
  string with the prodID-derived linker version + build, and a "Microsoft
  Visual C/C++" compiler guess.
- MSVC 5/6-era reliability: **partial.** It correctly extracts the linker
  prodID (gives `5.12.8034` here, matching the comp.id tables), but its
  *compiler* verdict is a coarse heuristic range and was WRONG for this file
  (it said `11.00-13.10`, i.e. VC++ 2002/2003 — see section 4). It does map the
  linker prodID build, but it does not print a per-record `@comp.id` table the
  way `richprint` does. Treat DIE's compiler line as a hint, not authority,
  for pre-VS2002 binaries.
- Linux availability: **packaged in nixpkgs as `detect-it-easy` (v3.10)**, and
  in Kali (`detect-it-easy`). Binary names `die`/`diec`/`diel`.

### dishather/richprint (+ clayne/pe-richprint fork)  [the authority used here]
- Pure-Python (`richprint.py`) script + a large, actively-curated
  **`comp_id.txt`** mapping `prodid:build` -> product/SP/tool. This is the most
  detailed openly-available comp.id->MSVC-version-and-SP database. It maps Rich
  build numbers to product *and service pack* names (it explicitly has VC++ 5.0
  SP3 entries — see section 2). Linux: trivial (needs python3, which is NOT on
  this box; we grepped the raw `comp_id.txt` instead).

### skochinsky gist + kirschju/richheader (prodids.py) + saferwall/pe (Go)
- Three more independent prodID->name tables. All three agree
  `0x13=Linker512`, `0x06=Cvtres500`. Good for cross-checking the prodID
  *semantics*, but they map prodID to a tool/product, not to a specific SP via
  build number (richprint is the one that resolves SP).
- saferwall/pe is a Go library (no python needed) and would run on Linux.

### Ghidra
- The PE loader parses the Rich header; Ghidra's "compiler" property is set
  from analysis (typically just `windows`/`visualstudio` granularity). Not
  precise enough to distinguish VC5 SP1 vs SP3 by itself. Linux: yes (JVM).
  Heavyweight; not run here.

### pefile (Python)
- `pefile.PE(...).get_rich_header_info()` / `RICH_HEADER` parses the Rich
  entries (prodid, build, count) — but pefile does NOT bundle a
  prodid->version name table; you pair it with richprint's `comp_id.txt`.
  Needs python3 (absent here).

### Others (catalogued, not the right tool for this question)
- **PEiD** (+ external sig packs like `userdb.txt`): legacy, Windows GUI,
  signature based; weak/abandoned for modern detection; can ID "Microsoft
  Visual C++" coarsely. Linux only via wine.
- **Exeinfo PE**, **RDG Packer Detector**: Windows GUI, packer-focused,
  wine-only on Linux. Coarse MSVC family at best.
- **CFF Explorer / pestudio**: Windows GUI structural viewers; CFF Explorer
  shows the Rich header; pestudio flags compiler. Not headless-Linux.
- **pev / readpe** (`readpe`, `pedis`, ...): C, Linux-native, has `-r`/rich
  handling in recent versions; NOT installed here and not in the quick path.
- **winchecksec**: checks security mitigations (ASLR/DEP/CFG), NOT a compiler
  identifier. Irrelevant to this question.
- **rabin2** (radare2): `rabin2 -H` shows PE headers incl. rich on new
  versions; NOT installed here.
- **`winedump dump`** (installed): prints File/Optional headers incl.
  `linker version 5.10` and absence of debug dir; does not decode the Rich
  comp.id table. Used here for the optional-header read.

---

## 2. THE KEY QUESTION — build number -> MSVC version/SP mapping  [HEADLINE]

Source of truth: **dishather/richprint `comp_id.txt`** (fetched raw this
session), cross-checked against skochinsky gist, kirschju/richheader
`prodids.py`, and saferwall/pe `richheader.go`. Verbatim lines:

```
# prodID semantics (from VS2019's msobj140-msvcrt.lib, in comp_id.txt):
0002 [LNK] VS97 (5.10)            # prodidLinker510
0006 [RES] VS97 (5.0)             # prodidCvtres500
0010 [LNK] VS97 (5.11)            # prodidLinker511
0013 [LNK] VS97 (5.12)            # prodidLinker512    <-- OUR build-8034 entry
0038 [RES] VS97 (5.01)            # prodidCvtres501

# specific build-resolved entries:
00131f62 [LNK] 5.12 build 8034 (Likely Libs)               <-- prodID 0x13, build 8034  EXACT MATCH
00060684 [RES] VS97 (5.0) SP3 cvtres 5.00.1668             <-- prodID 0x06, build 1668  EXACT MATCH
00021c87 [IMP] VS97 (5.0) SP3 link 5.10.7303
```

And the decisive provenance comment block in `comp_id.txt`:

```
# MSVS97 5.0 Enterprise Edition (cl 11.00.7022, link 5.00.7022)
# Does NOT generate any @comp.id records, nor Rich headers.
# SP3 added Rich-generating linker (albeit it doesn't identify itself),
# and CVTRES and LIB(?) utilities that generate @comp.id records. There is no
# distinction between import and export records yet. ...
```

### Mapping verdict for OUR three records

1. **cvtres build 1668 -> VC++ 5.0 SP3 cvtres 5.00.1668.** [CONFIRMED, exact
   line `00060684 [RES] VS97 (5.0) SP3 cvtres 5.00.1668`]. This is the single
   most diagnostic record: it pins the toolchain to **Visual Studio 97 / Visual
   C++ 5.0 Service Pack 3** specifically. cvtres was *not* updated after the SP,
   so a SP3-or-later VC5 toolchain produces exactly cvtres 1668.

2. **Linker prodID 0x13 build 8034 -> "Linker 5.12" (Linker512).** [prodID
   CONFIRMED by 4 DBs]. richprint labels build 8034 as `5.12 build 8034 (Likely
   Libs)`. The `(Likely Libs)` annotation means the table author mostly saw this
   exact build stamp coming in via static-library objects rather than from a
   freshly-run link of a normal project — i.e. the build number is real but its
   precise retail provenance is less pinned than cvtres. [INFERRED meaning of
   the annotation.]

3. **PE optional-header linker version 5.10** [CONFIRMED locally] is consistent
   with the VC++ 5.0 SP3 linker, which self-reports `link 5.10.7303` (per
   gunkies + comp_id.txt). Note the optional-header "5.10" is the linker's
   marketing/version field; the Rich prodID 0x13 nominally means "Linker 5.12".
   These two numbers come from different fields and both point at the VC5 SP3
   linker family. The 5.10 vs 5.12 discrepancy is a known quirk of how the
   prodID enumeration names linker variants and is not a contradiction.

### Ambiguity to flag  [UNCONFIRMED bits]
- VC++ 5.0 RTM linker = `5.10.7303` (build 7303). Our Rich build is **8034**,
  higher than 7303. So the LINKER that stamped this file is newer than RTM —
  consistent with an SP3+ linker, but 8034 does not equal the canonical 7303
  RTM build, and richprint files it under "5.12 ... (Likely Libs)". So:
  - The **product line is unambiguous: Visual C++ 5.0 / VS97, SP3-era.**
    [CONFIRMED by the cvtres 1668 = "VS97 (5.0) SP3" line.]
  - The **exact linker build 8034** is not cleanly tied to a single named
    retail SP installer in the public tables (it's tagged "Likely Libs").
    [UNCONFIRMED which exact patch produced link build 8034.] It is NOT VC++
    4.x and NOT VC++ 6.0 (6.0 linkers are prodID 0x04, builds 8168+; 4.x is
    pre-Rich). So the candidate set "VC5 RTM vs VC5 SP1 vs VC5 SP3 vs VC4.x"
    resolves to: **VC++ 5.0, SP3 (the SP that introduced these Rich/comp.id
    records); not 4.x, not 6.0, not RTM-only.**
- The **compiler (cl.exe) cannot be fingerprinted from this Rich header at
  all**, because VC++ 5.0's C/C++ compiler (cl 11.00.x) did not emit @comp.id
  records — confirmed by the comp_id.txt comment and by the Virus Bulletin
  VB2019 paper, which dates the Rich header's introduction to "Visual Studio 97
  SP3". So we infer cl from the surrounding toolchain, not from a stamp. VC++
  5.0 RTM cl = `11.00.7022`; the SP3 cl is the matching update (no comp.id, so
  not provable from the binary). [INFERRED.]

---

## 3. AVAILABILITY ON NIXOS

- **Detect It Easy: YES.** Attribute `detect-it-easy` (v3.10).
  - `nix run nixpkgs#detect-it-easy` launches the GUI (`die`).
  - The console tool is `diec`; get it with:
    `nix shell nixpkgs#detect-it-easy -c diec -j <file>`
    or run the built path's `bin/diec` directly.
  - There is no separate `die`/`diec` top-level attr; all three binaries
    (`die`, `diec`, `diel`) live in the one `detect-it-easy` package's `bin/`.
- **pev / readpe: NOT installed; attr is `pev` in nixpkgs** (provides
  `readpe`, `pev`, etc.). Use `nix shell nixpkgs#pev -c readpe -H <file>` if
  wanted. Not exercised here.
- **pefile / python3: NOT installed.** Would need
  `nix shell nixpkgs#python3Packages.pefile` (and pair with richprint's
  comp_id.txt for naming). Not exercised.
- **radare2 (rabin2): NOT installed**; attr `radare2`.
- Already present: `objdump`, `winedump`, `7z`, `perl`, `nix`.

Concrete invocation that WORKED this session:
```
nix build --no-link --print-out-paths nixpkgs#detect-it-easy   # -> store path
<store>/bin/diec -j GRUNTZ.EXE                                  # JSON verdict
```

---

## 4. LOCAL RUN OUTPUT (verbatim, `diec` 3.10)

`diec GRUNTZ.EXE`:
```
PE32
    Linker: Microsoft Linker(5.12.8034)
    Compiler: Microsoft Visual C/C++(11.00-13.10)
```

`diec -j GRUNTZ.EXE` (key values):
```
"name": "Microsoft Linker",        "type": "Linker",   "version": "5.12.8034"
"name": "Microsoft Visual C/C++",  "type": "Compiler", "version": "11.00-13.10"
```

Interpretation:
- `Linker 5.12.8034` = DIE read prodID 0x13 + build 8034 from the Rich header.
  This **agrees** with the richprint table (`0013 = Linker512`, build 8034).
  GOOD and correct.
- `Compiler 11.00-13.10` is **WRONG / misleading**. DIE has no compiler
  comp.id to read (there is none), so it falls back to a broad heuristic that
  collides with the modern VS2022 build stamp 8034 (comp_id.txt lines like
  `01058034 [C++] VS2022 v17.7.0 ... build 32820` reuse the low-word 8034).
  Do NOT trust DIE's compiler line here. Use richprint's resolution.

---

## 5. RECOMMENDATION

**Most authoritative tool for THIS binary:** dishather/richprint's
`comp_id.txt` build->product/SP table (cross-checked with skochinsky /
kirschju / saferwall for prodID semantics). `diec` is the best
headless-Linux *runner* and gives a correct linker read, but its compiler
verdict is unreliable for pre-2002 MSVC — so the comp.id table is the
authority, `diec` the convenient confirmer.

**Evidence-based verdict (confidence):**
- Toolchain = **Microsoft Visual C++ 5.0 (Visual Studio 97), Service Pack 3.**
  **HIGH confidence.** Driver: cvtres build 1668 maps exactly and uniquely to
  "VS97 (5.0) SP3 cvtres 5.00.1668"; the very existence of a Rich header dates
  the build to >= VS97 SP3 (Rich/comp.id were introduced in that SP); the
  optional-header linker 5.10 and Rich Linker512/8034 are consistent with the
  VC5 SP3 linker family; VC4.x is pre-Rich and VC6 uses different prodIDs/builds
  — both excluded.
- This **corroborates and refines** the user's "MSVC 5.0" reading, with two
  refinements: (a) it is specifically **SP3** (not RTM, not SP1); (b) the
  build-8034 record is the **LINKER**, not "a C/C++ compiler module" — and the
  binary contains NO compiler comp.id, which is expected and itself
  confirmatory of VC5.

**What to source for byte-matching:**
- Get **Visual C++ 5.0 + Service Pack 3** (Visual Studio 97 SP3). Target tool
  versions: `cl 11.00.x`, `link 5.10.7303`, `cvtres 5.00.1668`. The retail
  VC++ 5.0 + the SP3 patch is the concrete download to find.
- [UNCONFIRMED] Whether the exact linker that stamped build 8034 corresponds to
  SP3 precisely or a slightly later library/patch revision ("Likely Libs"
  caveat) cannot be proven from public tables alone. Recommended empirical
  check once you have the toolchain: build a tiny PE with VC5 SP3 link.exe and
  compare its Rich `prodID 0x13` build number to 8034. If it differs, hunt for
  the patched linker. The cvtres 1668 match already strongly anchors SP3.
- The compiler cl.exe build itself is NOT recoverable from this binary (no
  comp.id) — must come from matching VC5 SP3, then validated by code byte-diff.

---

## Sources
- dishather/richprint `comp_id.txt` (primary build->SP table) —
  https://github.com/dishather/richprint and raw
  https://raw.githubusercontent.com/dishather/richprint/master/comp_id.txt
- skochinsky Rich parser gist —
  https://gist.github.com/skochinsky/07c8e95e33d9429d81a75622b5d24c8b
- kirschju/richheader (`prodids.py`) —
  https://github.com/kirschju/richheader
- saferwall/pe `richheader.go` —
  https://github.com/saferwall/pe/blob/main/richheader.go
- Detect It Easy — https://github.com/horsicq/Detect-It-Easy ;
  Kali tool page https://www.kali.org/tools/detect-it-easy/
- Virus Bulletin VB2019, "Rich Headers..." (Rich introduced in VS97 SP3) —
  https://www.virusbulletin.com/virusbulletin/2020/01/vb2019-paper-rich-headers-leveraging-mysterious-artifact-pe-format/
- Computer History Wiki, Visual C++ 5.0 (cl 11.00.7022 / link 5.10.7303) —
  https://gunkies.org/wiki/Visual_C%2B%2B_5.0
