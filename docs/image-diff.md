# Image diff — which bytes of the candidate differ from retail, and why

Three questions, three tools, and only the third is answered here:

| tool | question |
|---|---|
| **objdiff** | does each **object** match the object we carved out of retail? |
| **`gruntz.audit.link_sections`** | is each **section** the SIZE retail shipped? |
| **`gruntz.audit.image_diff`** | **which bytes** of each section differ, and why? |

Section-size parity is necessary and nowhere near sufficient: two images can agree
on every section length and share almost no content. `.idata` and `.rsrc` both sit
at `+0`, and only one of them is actually a byte match. So every section gets two
halves, and the tool refuses to print either without the other:

* a **size attribution** — named buckets that must sum to the observed delta. The
  sum is asserted in code; whatever is left over is printed as an `unattributed`
  row rather than quietly dropped. That remainder is the honest worklist.
* a **byte similarity** — objdiff's method lifted to the linked image.

    python -m gruntz.audit.image_diff                    # the report
    python -m gruntz.audit.image_diff --section .text --detail 20
    python -m gruntz.audit.image_diff --referents 30     # the wrong-referent worklist
    python -m gruntz.audit.image_diff --selftest         # plant defects, find them
    python -m gruntz.audit.image_diff --tsv f.tsv --json f.json

Needs `ninja candidate` (both images + the `.map`) and a `gruntz build`
(`build/gen/symbol_names.csv`, `build/gen/delink_data_manifest.tsv`). Runs in ~4 s.

## Method

### Pair by symbol, never by file offset

Our `.text` is 25,813 B short, so everything downstream of the first divergence
sits at a different RVA. A positional differ reports near-0% for the whole image —
true and useless. Regions are therefore paired by **name**: retail's side from
`config/retail/functions.tsv` × `symbol_names.csv` × `library_labels.csv` (via
`gruntz.core.exe_map`) plus `delink_data_manifest.tsv` for data; ours from the link
`.map`'s publics. Pairing is placement-independent by construction, which is what
makes it survive the layout rules in `docs/link-text-layout.md` — a kept-COMDAT
exile or a group reordering moves a body without changing its name.

### Align inside a pair, do not compare at fixed offsets

Pairing alone is not enough. **One extra instruction early in a body shifts every
byte after it.** Measured: objdiff scores `CGruntzMgr::HandleCommand` 97.4% while a
positional byte diff of that same pair calls 90% of it different. So each pair is
aligned: k-gram anchors unique on both sides, longest increasing subsequence,
maximal extension (patience diff). O(n log n) — `difflib` needed 20 s on that one
pair and this needs milliseconds.

### Mask address operands by RESOLVING them

Every absolute operand points at a different address in the two images. That is not
a defect and an unmasked diff measures the placement delta, not correctness. Each
base-relocation dword, and each `E8`/`E9` displacement reaching a symbol start, is
replaced by a 4-byte token derived from **what it names**, so the same referent at
two different addresses tokenises identically — and a *different* referent does not.

The resolution precedence exists because the two images do **not** have the same
symbol coverage, and an asymmetry must never be reported as a difference:

1. **not an address in this image** → the literal value.
2. **code** → the symbol, and only if BOTH images know that name. Fingerprinting a
   function by its first bytes would match every `push ebp; mov ebp,esp` in the
   binary, so an unpairable code target is **UNDECIDABLE**.
3. **a NUL-terminated string** → the TEXT. Retail's manifest names literals
   `??_C@_0M@NCPH@LogicAttack?$AA@`; our `.map` does not publish them at all.
   Comparing the text is both symmetric and stronger than comparing either name.
4. **a paired data symbol** → the name.
5. **other data** → the 8 bytes at the target (a pool constant), marked *weak*.
6. **no file bytes** (`.bss`) → **UNDECIDABLE**.

An incremental-link thunk is followed, not compared: retail reaches a static-library
body directly while we reach the same body through an `E9` thunk, because only
objects named on the link line get one. An `FF 25` import stub is named from the
**import table** on both sides, so it needs no symbol table to agree.

UNDECIDABLE is a real answer. It is counted, printed, and folded into neither
"match" nor "differ".

### The headline is a floor

`retail reproduced` = matching bytes / **retail's** section size.

* Retail bytes we never paired count **against** it — that is exactly what
  per-object objdiff structurally cannot see, because an unclaimed hole has no
  object to score.
* Bytes we emit that retail does not have never count **for** it; they appear in a
  separate `candidate bytes with no retail twin` row.
* `not measurable` bytes stay in the **denominator**. Two secondary lines
  (`of the MEASURABLE part`, `of MEASURABLE, NON-FILLER retail`) give the reader the
  restricted views without letting either replace the headline.

### The referent test — the measure a byte diff cannot give

Independent of where the bytes landed: **does the paired body reach the same things,
in the same order?** The ordered list of resolved referents is compared per region.
A body can be byte-shifted throughout and still have a perfect referent sequence
(correct code, different scheduling), and it can be byte-identical everywhere it
aligns while calling the *wrong function* — only this test separates the two.

**This is the finding objdiff masks away by design.** A wrong string literal, a call
to the wrong overload, a constructor for the wrong class: all of them score 100% in
a relocation-masked per-object diff and all of them show up here.

## Results (candidate 2,491,392 B vs retail 2,511,872 B)

| section | retail | candidate | delta | reproduced | not measurable | unattributed |
|---|--:|--:|--:|--:|--:|--:|
| `.text` | 1,987,179 | 1,961,366 | −25,813 | **48.55%** | 58,731 | **0** |
| `.rdata` | 135,080 | 142,728 | +7,648 | **16.96%** | 66,776 | **0** |
| `.data` | 763,820 | 832,524 | +68,704 | **10.06%** | 627,628 | **0** |
| `.idata` | 15,169 | 15,169 | +0 | **78.03%** | 3,307 | **0** |
| `.rsrc` | 123,260 | 123,260 | +0 | **100.00%** | 0 | **0** |
| `.reloc` | 113,105 | 110,236 | −2,869 | **41.05%** | 60,301 | **0** |
| **total** | 3,137,613 | 3,185,283 | +47,670 | **39.72%** | 816,743 | **0** |

**Every section's size attribution closes exactly. The unattributed remainder is 0
bytes everywhere**, asserted in `SecReport.close()`.

Referents: **35,572 of 36,670** decidable address operands in paired regions
(**97.01%**) reach the same thing in the same order.

### `.text` −25,813

| bucket | retail | candidate | delta |
|---|--:|--:|--:|
| ILT thunk band | 27,680 | 45,760 | **+18,080** |
| plain `.text` — paired bodies | 1,169,910 | 1,314,553 | +144,643 |
| plain `.text` — unpaired filler `0xCC`/`0x90` | 442,515 | 362,724 | −79,791 |
| plain `.text` — unpaired zero | 30,401 | 6,540 | −23,861 |
| plain `.text` — unpaired CARVED bodies | 38,843 | 0 | −38,843 |
| plain `.text` — NEVER-CARVED code | 65,965 | 20,071 | −45,894 |
| `.text$AFX_*` — paired MFC bodies | 78,662 | 83,936 | +5,274 |
| `.text$AFX_*` — unpaired filler / zero / content | 74,462 | 69,188 | −5,274 |
| pad `AFX` → `$x` | 10 | 12 | +2 |
| `.text$x` (unmeasurable) | 58,731 | 58,582 | −149 |

Byte verdict over retail's 1,987,179:

| | bytes |
|---|--:|
| aligned + identical | 847,325 |
| masked operand → same symbol | 101,400 |
| masked operand → same target | 16,132 |
| masked operand, not aligned | 14,820 |
| masked operand, referent UNKNOWN | 49,052 |
| no aligned counterpart | 219,843 |
| never paired | 738,607 (58,731 unmeasurable) |

48.55% headline; **77.28%** within paired regions; **64.93%** of the measurable,
non-filler part. 4,876 bodies pair in plain `.text` and 904 more in `.text$AFX_*`.
Of the 32,958 decidable operands, **31,861 (96.67%) reach the same referent**;
610 regions do not, and that is the worklist below.

Three findings the size audit could not state:

* **The unpaired 104,808 B splits.** 38,843 B is inside a function somebody
  *carved* that we could not pair by name (497 regions) — an attribution defect.
  The other 65,965 B is code no carve, FID label or helper row has ever claimed —
  the reconstruction worklist. `docs/link-section-audit.md` had these as one number.
* **`.text$x` is honestly unmeasured.** The `/GX` unwind funclets carry no public
  symbol in *either* image, so 58,731 B is reported UNMEASURED rather than scored 0.
* **The ILT band is not code.** Retail emits 2,696 thunks, we emit 4,575, and 2,629
  reach the same function; at 10 B each the +18,080 is a `/INCREMENTAL` link-line
  fact, not a content difference.

### `.rdata` +7,648 and `.data` +68,704

Both headlines are dominated by content that cannot be aligned honestly:

* `.rdata`'s 66,776 B of `.xdata$x` is the compiler's `__ehfuncinfo` blobs, which
  **neither** image gives a symbol. The EH record and try-block census in
  `link_sections` is what covers them. Of the measurable part, `.rdata` is 33.54%,
  and **93.59% within paired regions** — 766 paired data objects carrying operands,
  and **3,564 of 3,565 referents correct (99.97%)**, one divergent region.
* `.data`'s 627,628 B is `.bss` zero fill with no bytes at all. The initialized part
  is 56.41%, and **99.99% within paired regions** (76,827 of 76,831 B), with all 147
  referents correct: every data object we do pin, we reproduce. The gap is coverage — 271 retail data regions have
  no candidate symbol — not correctness.

### `.idata` is a logical identity

Same 16 DLLs in the same order, **456 of 456 imports paired by name, 0 unpaired on
either side.** A raw byte compare reports 10,404 of 15,360 bytes differing, which is
almost entirely misleading: **12 of the 16 DLLs simply order their thunk array
differently**, an import-library member-order artifact. Paired by `(dll, import
name)`, 99.78% of the measurable bytes agree. The 3,307 B not paired is hint/name
pool alignment padding whose position follows that same ordering.

### `.rsrc` is byte-exact — proved, not asserted

75 resources, identical offsets and lengths, so a straight byte compare is
legitimate; the tool verifies that positional alignment before it compares, and
falls back to UNMEASURED if it ever stops holding. 123,120 of 123,260 bytes are
identical and **all 140 differing bytes are `IMAGE_RESOURCE_DATA_ENTRY.OffsetToData`
dwords differing by exactly the `+0xc000` placement delta. Zero unexplained.**

### `.reloc` is scored through the other sections

`.reloc` has no content of its own — it is a function of where the relocated words
are. A retail relocation counts as reproduced when the aligned candidate region
carries one at the same place naming the same thing, which the region differ has
already decided. 26,402 of retail's 44,604 sites are inside a paired region and
**23,212 of those are reproduced**; the other 18,202 are in unclaimed `.text` or
unpaired data and are UNMEASURED. Retail relocates 44,604 words and we relocate
43,181 — that −1,423 IS the section delta.

## The wrong-referent worklist

`--referents N`. 610 paired regions reach something else than retail does. A sample,
each of which is a real defect that scores 100% in a relocation-masked object diff:

| region | retail reaches | we reach |
|---|---|---|
| `CGruntzMgr::Close` | `"Num Runs"`, `"Num Movies"`, `"High Detail"`, `"Disable Joystick"` | `"Num_Runs"`, `"Num_Movies"`, `"High_Detail"`, `"Disable_Joystick"` |
| `ButeMgr::ParseAttributeFile` | `"ButeMgr:  duplicate symbol encountered - %s"` | `"ButeMgr:  duplicate tag encountered - %s"` |
| `CStatusBarMgr::LoadTabSprites` | `CSBI_RectOnly::CSBI_RectOnly` | `CStatusBarItem::CStatusBarItem` |
| `CGrunt::LoadPickupSprites` | `"GRUNTZ_PICKUPS_HEALTH1..3"` | `"GRUNTZ_PICKUPS_REDBRICK/BLUEBRICK/GOLDBRICK"` |
| `RegisterGruntActions` | `_zvec::IndexToPtr` | `zDArray<…>::Resolve` |

The registry-key underscores and the ButeMgr message are byte-level source bugs; the
`CSBI_RectOnly` and `_zvec` rows are modelling divergences (wrong class constructed,
different container API).

## Selftest

`--selftest` plants known defects and requires the differ to find, attribute and
classify each one. Every check states what a *wrong* implementation would report.

    [PASS] aligner: a 4-byte INSERTION still matches the whole body
    [PASS] aligner: a 7-byte overwrite costs exactly 7
    [PASS] .text/.rdata/.data/.idata/.rsrc/.reloc size buckets sum to the delta
    [PASS] 7 flipped body bytes -> exactly +7 unreproduced
    [PASS] ...and it is attributed to the right region
    [PASS] a repointed relocated dword -> a REFERENT divergence
    [PASS] ...and the masking did NOT swallow it
    [PASS] .rsrc: every differing byte is a classified placement shift
    [PASS] every section counts its UNMEASURABLE bytes in the DENOMINATOR

The closure assert is not decoration: an early return added to the region differ
silently dropped 1,340 B of operand-free `.rdata` regions until it fired.

## Known limits

* **Static functions do not pair.** Our side's symbols come from the `.map`, which
  lists publics only. A `static` helper is unpaired on both sides and lands in the
  unpaired-content bucket.
* **`.text$x` and `.xdata$x` are unmeasured**, not scored. Pairing an unwind funclet
  or an `__ehfuncinfo` record would mean chasing it from its owning function's EH
  data; that is real work nobody has done, and reporting 0% instead would be a lie
  in the other direction.
* **Retail extents come from `config/retail/functions.tsv`**, i.e. from a carve, not
  from ground truth. A mis-carved boundary shortens what gets compared.
* **The referent sequence is compared only over the length retail carved**, because
  cl parks a switch jump table in `.text` right after the body and our "up to the
  next public" extent can carry operands retail's carve never included.
* **The `weak` resolution class** (8 bytes at an unnamed data target) can in
  principle collide — two distinct pool constants with the same leading 8 bytes. An
  all-zero window is not evidence at all and is demoted to UNDECIDABLE, which leaves
  weak at **839 of 45,351 `.text` operands (1.8%)**; the rest resolve by symbol
  (28,120), string (4,129) or are UNDECIDABLE (12,263).
