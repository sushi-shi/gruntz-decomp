# Data matching: what the 100% does NOT mean

The old **`Data: 100.00%`** headline was arithmetically true and answered the wrong
question. The replacement reports three distinct facts. **Gross coverage 40.45%** means
109,765 distinct addresses are enrolled out of all 271,360 initialized bytes stored in
retail. A generated static-reachability partition proves 159,359 B are private,
unreachable library/compiler data or padding, so **reconstructable coverage is 98.00%**:
109,765 / 112,001 eligible bytes. **Fidelity 100.00%** (rounded) means the enrolled
objdiff sections are almost entirely byte-equal. `.bss` is reported separately.

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
rather than the single `RECT` that retail's assignment code proves. Both have now been
corrected.

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

1. **The player/colour index feeding the proven key lookup.** The key strings agree, but a
   wrong index before the lookup could still select a coherent wrong variant.
2. **The shade/palette subsystem**, which is the worst-matched area in the tree — **53
   colour-path functions below 100%**:

   | function | fuzzy |
   | :-- | --: |
   | `CDDrawShadeBlit::BlitShadedMirrored` | 55.92% |
   | `CDDrawShadeBlit::BlitShadedForward` | 58.33% |
   | `CShadeTableCache::FlashTable` | 60.94% |
   | `CShadeTableCache::FindNearestColor` | 65.35% |
   | `CShadeTableCache::CompareLuma` | 70.00% |
   | `RgbToHsv` | 83.65% |

   `FindNearestColor` maps a desired RGB onto a palette index; getting it wrong yields the
   wrong entry, not a corrupted one.
3. **The `g_clut` re-pin** (§2). The compensation is currently consistent, so it may cost
   nothing at runtime today — but this exact object has already produced one colour defect,
   and it is the one place in the colour path where the *model* is known to be wrong.

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

## Related

`docs/data-attribution.md` · `docs/compiler-data-layout.md` · `docs/data-access-map.md` ·
`docs/link-section-audit.md` · `docs/gotchas.md`
