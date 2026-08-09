# Data matching: what the 100% does NOT mean

The old **`Data: 100.00%`** headline was arithmetically true and answered the wrong
question. The replacement reports two denominators: **coverage 40.10%** means 108,827
distinct addresses are enrolled out of the 271,360 initialized bytes stored in retail;
**fidelity 99.97%** means the enrolled objdiff sections are almost entirely byte-equal.
`.bss` is reported separately. A smaller headline is the successful result: bytes omitted
from both sides can no longer hide outside the score.

The earlier 167,787 / 271,360 (61.8%) estimate added per-object section sizes. A folded
COMDAT defined by several objects was therefore counted several times. Coverage is an
address property, so the numerator is the union of retail address ranges and each byte is
counted once. objdiff's per-object `matched_data` remains visible for historical
comparison, but it is not a coverage numerator.

The distinction is not academic. A wrong model that scores 100% is worse than a low score,
because a low score is a worklist and a wrong 100% is a closed question that is not closed.

The campaign found two concrete examples: `g_clut` was pinned two bytes low with every
use biased by `+2`, and the image clip group was spelled as an integer array rather than
four independently placed `LONG` globals. Both have now been corrected.

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

Their join is the definition of the defect: **uncovered ∧ touched = unmodelled data;
uncovered ∧ untouched = padding.**

`data_access_map --findings unclaimed` remains a separate, ratcheted access worklist.
Those findings are outside fidelity until a real typed claim enrolls them.

The address-union partition is complete to an explicitly stated residue:

| unenrolled class | bytes |
| :-- | --: |
| compiler C++ EH records | 53,164 |
| compiler RTTI records | 13,569 |
| compiler pooled literals | 41,677 |
| static-library data | 23,950 |
| SDK GUID libraries | 1,776 |
| EH padding | 4,564 |
| other zero padding/alignment | 23,493 |
| target-referenced but ownership unresolved | 28 |
| other unclassified non-zero | 312 |
| **total unenrolled** | **162,533** |

The library attribution is NAFXCW 14,164 B, unresolved static-library members 9,786 B,
dxguid.lib 1,696 B, and UUID.LIB 80 B. No fractional attribution is
invented for a run named from more than one library. The independent access/coverage
sieve found one game-owned initialized survivor, `g_table_20fa78` (64 B), and it was
already enrolled; the remaining game-data worklist is therefore 0 B. The 340 B residue
stays unclassified rather than being folded into “library”.

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

### `g_imageClipRect`

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
reaches**, and the answer is not zero:

**35,625 of 36,786 operands (96.84%) reach the same referent in the same order. 610
regions do not** — and every one of them scores **100% in a relocation-masked object
diff**. Four from the worklist (`image_diff --referents N`):

| region | retail reaches | we reach |
| :-- | :-- | :-- |
| `CGrunt::LoadPickupSprites` | `"GRUNTZ_PICKUPS_HEALTH1..3"` | `"..._REDBRICK"`, `"..._BLUEBRICK"`, `"..._GOLDBRICK"` |
| `CStatusBarMgr::LoadTabSprites` | `CSBI_RectOnly::ctor` | `CStatusBarItem::ctor` |
| `CGruntzMgr::Close` | `"Num Runs"`, `"Num Movies"` | `"Num_Runs"`, `"Num_Movies"` |
| `CButeMgr::ParseAttributeFile` | `"duplicate symbol encountered"` | `"duplicate tag encountered"` |

These are **behavioural defects with a perfect score**: wrong asset keys, a wrong
constructor, wrong strings. This is the sharpest available answer to "if the data
matches, why does the game misbehave" — and note the first row is the *same failure mode*
as the colour case in §5: a named-asset lookup fetching the wrong thing.

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

Observed: **gruntz render blue instead of yellow at start-up.** Data scores 100%, so by the
above that is not evidence against the data — it is a reminder of what the 100% covers.

Candidate causes, ranked by how well each explains *a clean wrong colour* rather than a
corrupted one. **None is confirmed**; the game is not run to obtain matching evidence
(standing project rule), so each must be settled statically.

1. **Colour-variant selection.** `CGruntzMgr::SetGruntColor` is not palette maths — it
   looks up a *named sprite set* (`GAME_TREASURE_GECKOS_RED`, `..._BLUE`, `..._PURPLE`) in
   `m_imageRegistry->m_10map` and copies the whole `CImage`. Blue instead of yellow is a
   **different asset**, which is what a wrong key or wrong player index produces. A shade
   table error would look like wrong shading, not a clean substitution. Ranked first for
   that reason.
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
