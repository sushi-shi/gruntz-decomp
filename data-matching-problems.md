# Data matching: what the 100% does NOT mean

The old **`Data: 100.00%`** headline was arithmetically true and answered the wrong
question. The replacement reports three distinct facts. **Gross coverage 40.86%** means
110,882 distinct addresses are enrolled out of all 271,360 initialized bytes stored in
retail. A generated static-reachability partition proves 160,114 B are private,
unreachable library/compiler data or padding, so **reconstructable coverage is 99.67%**:
110,882 / 111,246 eligible bytes. **Fidelity 96.82%** means that share of the enrolled
bytes is byte-equal. `.bss` is reported separately.

Fidelity fell from a rounded 100.00% deliberately, twice, and both drops were the metric
becoming honest rather than the tree getting worse: enabling `functionRelocDiffs=data_value`
stopped masking wrong data referents, and the delinker fix in `delink(data): a data
hypothesis must CONTAIN the rva` stopped an unbounded nearest-symbol guess from beating an
exact PDB symbol (1,020 relocs across 185 objects had been decomposing past their symbol's
end). Referents the delinker now resolves *correctly* are not all enrolled yet, so they
read as unmatched — which is the true state, not a regression. Numbers here are a snapshot;
`gruntz status` is the authority.

Ownership never overrides reachability. Library-owned bytes directly named by game or
compiler code, or reached through pointers in enrolled/game-visible data, remain eligible.
Traversal stops at proven library-function boundaries, so calling an MFC/CRT routine does
not pull all of that library's private tables into the denominator. Unclassified bytes also
remain eligible; uncertainty can never improve the score.

The earlier 167,787 / 271,360 (61.8%) estimate added per-object section sizes. A folded
COMDAT defined by several objects was therefore counted several times. Coverage is an
address property, so the numerator is the union of retail address ranges and each byte is
counted once. objdiff's per-object `matched_data` remains visible for historical
comparison, but it is not a coverage numerator.

The distinction is not academic. A wrong model that scores 100% is worse than a low score,
because a low score is a worklist and a wrong 100% is a closed question that is not closed.

The campaign found two concrete examples: `g_clut` was pinned two bytes low with every
use biased by `+2`, and the image clip rectangle was spelled as an untyped integer array
rather than the single `RECT` that retail's assignment code proves.

**The `g_clut` correction was incomplete for months, and that is the sharpest lesson in
this file.** Fixing the *pin* left the compensation alive in the *code*: the writer
advanced its cursor before its three stores instead of after, so the whole 3 x 32768-entry
LUT sat one entry high (and the last store ran two bytes past the array), while `ShadeRect`
carried a matching `+2` that cancelled it on the read side. Writer and reader were
self-consistent, the menu dim looked right, and the function sat at 96.86% with nothing
pointing at it — because objdiff scored the wrong addend as free (§3c). It shipped as
visible in-game corruption: every alpha blend read `table[i]` and got `v(i-1)`, so any
zero colour channel wrapped into the previous row and blew out to near-full, wrecking every
shadow and sprite outline. Fixed 2026-08-10; `BuildColorChannelTables` is now 100.00% EXACT
with retail's addends byte-for-byte. **A compensated defect is not corrected until every
compensation is gone — grep for the bias, do not trust the pin.**

---

## 1. Unclaimed data is never compared, on either side

The most common misreading is that objdiff would surface data we have not modelled. It
cannot, because **we generate both sides of the comparison**:

```
our DATA() pins  ->  build/gen/symbol_names.csv  ->  synth PDB
                 ->  vostok-delinker carves the TARGET objects
                 ->  objdiff pairs them against our BASE objects
```

The delinker is not an independent authority on what retail contains — it is told what to
carve, by us. A datum with no `DATA()` pin is never carved into a target object and never
declared in a base object, so **the byte enters neither side of the pair**. objdiff is not
ignoring a difference; it was never shown the bytes.

### Why code does not have this hole

`config/retail/functions.tsv` is an inventory of retail's real function starts, derived
independently of `src/`. That is why an unreconstructed function appears as an `(unmatched)`
row, and why the 26,737-byte hole at `0x0006f16f` was *visible as a hole* rather than
silently absent.

**There has never been an equivalent independent inventory for data.** Two tools now serve
that role and they are the data-side answer to `functions.tsv`:

| tool | question it answers | derived from |
| :-- | :-- | :-- |
| `gruntz audit data_access_map` | which bytes does retail's code actually read/write? | the `.reloc` table + disassembly at each site |
| `gruntz audit data_coverage` | which bytes does no claim cover? | claim extents ∪ placed candidate sections |
| `gruntz audit data_denominator` | which uncovered bytes are eligible versus private? | function ownership + static pointer reachability + compiler/SDK payload parsers |

Their join defines the actionable gap: **uncovered ∧ game-visible = unmodelled eligible
data**. An untouched byte is excluded only when independent evidence proves private
library/compiler ownership or padding. Untouched non-zero bytes with no such proof remain
unclassified and eligible.

`data_access_map --findings unclaimed` remains a separate, ratcheted access worklist.
Those findings are outside fidelity until a real typed claim enrolls them.

The address-union partition is complete to an explicitly stated residue:

| unenrolled class | bytes |
| :-- | --: |
| compiler C++ EH records | 53,164 |
| game-visible unenrolled data | 1,460 |
| unclassified non-zero (eligible) | 776 |
| compiler RTTI records | 13,223 |
| compiler pooled literals | 40,681 |
| private static-library data | 22,522 |
| private SDK GUID data | 1,680 |
| EH padding | 4,564 |
| other zero padding/alignment | 23,525 |
| **total unenrolled** | **161,595** |

The private-library attribution is NAFXCW 12,772 B, unresolved static-library members
9,750 B, dxguid.lib 1,600 B, and UUID.LIB 80 B. Another 928 B of NAFXCW data, 96 B of
dxguid data, and 44 B from unresolved static-library members are game-visible and are
therefore **kept** in coverage despite their ownership. No fractional attribution is
invented for a run named from more than one library. The eligible unenrolled worklist is 2,236 B: 1,460 B statically
reachable from game-side roots plus 776 B unclassified.

`config/retail/data-coverage-partition.tsv` is generated, not hand-maintained.
`python -m gruntz.audit.data_denominator --check` re-derives every range and is a normal
build gate; `--write` refreshes it after a reviewed enrollment or reachability change.

This lane also enrolled 1,099 B of closeable zlib initialized data: fourteen tables and
copyright strings whose names, owning archive members, extents, and payloads are all
recoverable from the shipped zlib 1.0.4 objects/vendor source. They are library-owned,
but unlike CRT/MFC compiler internals they have a reproducible base-side definition, so
leaving them excluded would not be honest.

---

## 2. A wrong pin scores 100% when the code compensates

The score checks *the bytes at the address we claimed*. It does not check *that the address
is right*. If the pin is wrong and the source is written around the error, both sides agree.

### `g_clut`, the colour lookup table

The old source made the compensation explicit:

```c
DATA(0x00253c9e)
// 0x30002, not 0x30000: every user indexes the three 64K banks with a +2 bias
// (g_clut+0x2 / +0x10002 / +0x20002), so the top bank runs to g_clut+0x30001.
// Declared 0x30000 here, the 2-byte tail landed on whatever followed in OUR
// layout, which was g_rDown: it read 0xF000 instead of 3, so every red channel
// was mis-shifted.
u8 g_clut[0x30002];
```

The pin is **two bytes low**. The real object is `u8 g_clut[0x30000]` at `0x253ca0`,
ending exactly at `g_lut16`; every use site carried the same compensating `+2`. Those
independent extent and use facts prove the correction. Absolute retail-RVA alignment does
not: c2 aligns within an object contribution and the linker places that contribution.

### `g_imageClip`

`RenderFrameClipped` takes a retail-proven `RECT*` and emits four inline field stores.
A controlled VC5 `/O2 /MT` probe against the shipped MFC headers distinguishes the
plausible declarations: both CRect assignment forms call imported `CopyRect` (the pointer
form also constructs a temporary), while plain `RECT = *RECT*` emits retail's exact four
direct loads and stores. The source therefore models one `RECT`, not `CRect`, `i32[4]`, or
four unrelated scalar globals.

**Neither defect lowered the old 100% fidelity figure.** Both sit outside what that
relocation-masked byte comparison can establish.

---

## 3. A relocated word cannot be byte-compared at all

A relocated word's bytes are a placeholder the linker overwrites. Both sides hold the
**same placeholder**, so a byte comparison is structurally blind to a wrong referent — a
vtable slot bound to the wrong method moves no byte.

That is why `gruntz.audit.data_relocs` exists as a separate normal-tier gate: it resolves
every pinned word on both sides through the retail image's own `.reloc` table and compares
**resolved addresses, never names**. Two traps it cost to learn are recorded in
`docs/gotchas.md`; its scope is *pinned* data, so it inherits hole #1 above.

---

## 3b. A wrong REFERENT scores 100% — and this one is measured

Following directly from §3: because the placeholder bytes agree, a region can be
byte-perfect while every pointer in it aims somewhere else. `gruntz audit image_diff`
now measures this on the linked image by resolving each address operand to **what it
reaches**. The first pass reported 610 regions. Source corrections and independently
tested resolver/sequence corrections reduce the current archive-ordered result to
**36,215 of 36,687 decidable operands (98.71%) reaching the same referent, with 180
genuinely different regions and 44 ordering-only regions**. The 180 split into 156
symbol-proven and 24 weak/content-only regions; none is literal-only.

The original worklist exposed real perfect-score defects such as registry keys spelled
with underscores, the wrong Bute diagnostic string, and constructor/container identity
differences. It also exposed methodological false positives: relocated pool windows,
IAT slots read as text, short/control-character literals, interior self-references, and
undecidable operands splitting an otherwise equal sequence. A seventh reporting defect
let a later string downgrade symbol evidence; monotone precedence corrected the three
supposed literal-only rows to symbol-proven structural rows. A further cutoff defect
dropped valid calls moved past retail's byte extent in longer candidate bodies; it now
admits only tail identities that fill an exact retail multiset deficit. Each class has a
negative self-test. The remaining 180 are a ratcheted structural worklist, not honestly reducible
by substituting plausible strings or callees without per-site identity evidence.

## 3c. A wrong ADDEND scored nothing at all — until objdiff was patched

The narrowest and most expensive hole of the family, and the one that shipped §2's
`g_clut` bug. objdiff pairs a relocated operand by **symbol**; for `IMAGE_REL_I386_DIR32`
the addend is not in the relocation record at all — x86 COFF stores it **inline in the
instruction's displacement field**, which is exactly the field the masked diff blanks
*because* it is relocated. So `g_clut+0x20000` and `g_clut+0x1FFFE` scored **identical**.
It was the one operand class where a wrong constant was free.

Precisely: `FunctionRelocDiffs::{All,NameAddress}` *do* compare addends, and so does the
undefined-symbol arm — only **`data_value`**, the mode `configure.py` sets, short-circuits
that clause and drops the addend along with the name. A datum defined in-section on both
sides lands in exactly that hole.

Closed by a local patch (`nix/patches/objdiff-score-reloc-addend.patch`): absolute types
only (`DIR32`/`DIR32NB` and the ELF/64-bit forms), every `REL32` form excluded because it
is site-relative, and the same symbol required. Cost: 0.0007pp of fuzzy and **zero** exact
functions — the exact *set* is unchanged, which follows by construction, since byte-identical
code carries byte-identical addends. Proven by re-injecting the `g_clut` defect: unpatched
scored it **100.0000% EXACT with bit-identical overall fuzzy**; patched, it drops and loses
exactness.

The patch is **necessary but not sufficient**: objdiff can only charge rows its opcode diff
aligns, so a wrong addend inside a badly-unmatched body stays invisible to it.
`python -m gruntz.audit.reloc_addends` covers that case by comparing per-symbol addend
**multisets** (order-insensitive on purpose — the same offsets in a different order is
operand-evaluation order, not a wrong constant; a naive positional compare reported 182 rows
that were almost entirely artifact, including three false hits at 100.00%). Tree-wide it
finds **four** genuine rows, the strongest being `FillPolygon` reading `ClipVtx.x` (a float)
where retail reads `.fx` (fixed-point) — a wrong *field*, not a wrong table.

## 4. The size/similarity confusion, one level up

The README's **Link status** block reports per-section *sizes* of the linked candidate
against retail. Two images can match on every section size and share almost no bytes.
`.idata` and `.rsrc` show `+0` there, which means "same length", not "same bytes".

That block now carries a **`retail bytes reproduced`** column beside the size delta,
measured by `gruntz audit image_diff`: regions pair by **symbol** (never by file offset —
our `.text` was 26 KB short, so a positional differ would call everything after the first
delta different), the two streams are aligned, and address operands are masked by
resolving each to its referent. Retail bytes we never paired count *against* the figure;
bytes we emit that retail does not have never count *for* it.

The honest counterpart is the **`not measurable`** column — 815,839 retail bytes for
which no alignment exists (`.bss` zero fill, `.text$x` unwind funclets, `.xdata$x` EH
blobs — none carry a symbol in *either* image). They stay in the denominator, so every
percentage there is a **floor**, not a flatterer.

---

## 5. The open case: wrong colours at runtime

Observed: **gruntz render blue instead of yellow at start-up.** High fidelity is not
evidence against a data/referent defect; it is a reminder of what fidelity covers.

The named-asset hypothesis is now statically cleared: `CGruntzMgr::SetGruntColor` reaches
the same RED/GREEN/BLUE/PURPLE keys as retail, and `CGrunt::LoadPickupSprites` reaches the
same complete decidable pickup-key multiset. The symptom therefore lies elsewhere; the
game is not run to obtain matching evidence (standing project rule).

**Suspect 3 was right, and it was the answer.** The entry below read "the compensation is
currently consistent, so it may cost nothing at runtime today — but this exact object has
already produced one colour defect, and it is the one place in the colour path where the
*model* is known to be wrong." It cost plenty: the writer's off-by-one described in §2 was
still live, and it was corrupting every alpha blend in the game. Found and fixed
2026-08-10, after a user reported in-game artifacts on terrain and sprites. **When a
document says a known-wrong model "may cost nothing", that is a worklist item, not a
reassurance.**

Colour-path status now, for whoever picks this up next:

| function | then | now |
| :-- | --: | --: |
| `BuildColorChannelTables` | 96.86% | **100.00% EXACT** (the defect) |
| `CShadeTableCache::CompareLuma` | 70.00% | **100.00% EXACT** |
| `CShadeTableCache::FlashTable` | 60.94% | 83.82% |
| `CShadeTableCache::FindNearestColor` | 65.35% | 76.05% |
| `CDDrawShadeBlit::BlitShadedMirrored` | 55.92% | 75.13% |
| `CDDrawShadeBlit::BlitShadedForward` | 58.33% | 68.52% |

The whole DDrawMgr colour pipeline has since been verified byte-faithful end to end — mask
decode, the entire `CShadeTable` load/save family, `Select`/`SetShadeDescr`/`Blit` with
correct bank addends, and the `shadetablecache` `.rdata` float pool byte-identical to
retail. The residual per-pixel functions are register allocation, which cannot change a
pixel.

**Still open**, and outside that verified pipeline: a global red **tint** over an otherwise
correctly-coloured frame, and a per-row **shear** (a sprite's upper rows displaced right of
its lower rows — only a wrong row-advance produces that). Ranked suspects are the
compositing pair's geometry (`CDDrawSurfacePair::InitFromSurface` 77.5%, `SetGeom` 78.5%,
`CDDrawSubMgrPages::CreateChildren` 82.6%) and MapMgr's per-player palette install.
Suspect 1 below is untouched and still stands.

1. **The player/colour index feeding the proven key lookup.** The key strings agree, but a
   wrong index before the lookup could still select a coherent wrong variant.

---

## How to check a claim in this area

* Is the datum even compared? `gruntz audit data_access_map --symbol <name|rva>` prints the
  offset-resolved field map with per-offset access widths; `--findings unclaimed` is the
  worklist of data retail uses and we do not model.
* Is the extent right? `python -m gruntz.audit.link_sections --undersized N` sweeps pinned
  `.rdata` for non-zero slack before the next pin. Retail pads *between* contributions with
  zeros, so all-zero slack is filler, not an under-model.
* Is the address right? Use contribution-relative layout, neighbour extents, and access
  addends. c2's object-relative alignment does **not** have to divide an absolute retail
  RVA; applying that shortcut rejects 131 established source-backed controls.
* Is the referent right? `gruntz audit data_relocs` and `python -m gruntz.audit.assert_relocs`.
* Is the **addend** right? `python -m gruntz.audit.reloc_addends` (§3c). objdiff now charges
  a wrong DIR32 addend, but only where its opcode diff aligns the row — this census covers
  the rest.
* Is the datum **written** where retail writes it? `python -m gruntz.audit.store_offsets`
  diffs the ordered member-store destinations per function; an offset on one side only is
  the signature of a wrong member. Its docstring lists six traps that produced phantom rows
  before the tool was trustworthy — read them before believing a new detector of this shape.
* Is a compensation hiding the defect? Grep for the bias, not just the pin (§2). A pin fix
  that leaves `+N` in the users is not a fix.

## Related

`docs/data-attribution.md` · `docs/compiler-data-layout.md` · `docs/data-access-map.md` ·
`docs/link-section-audit.md` · `docs/gotchas.md`
