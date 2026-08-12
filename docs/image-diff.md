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
    python -m gruntz.audit.image_diff --ordering 20      # the ordering-only worklist
    python -m gruntz.audit.image_diff --multiplicity 20  # same identities, different counts
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
5. **other data** → the bytes the containing instruction reads when that width is
   unambiguous (currently direct x87 32/64-bit real operands), otherwise the
   conservative 8 bytes at the target; marked *weak*.
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
Of the 36,687 decidable operands, **36,215 (98.71%) reach the same referent**;
180 regions reach a genuinely different decidable referent. Another 44 reach the
same multiset in a different order and are reported separately.

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

`--referents N` prints the paired regions that reach something else than retail does,
triaged by the **strongest evidence** behind each one, because the three classes cost
very different amounts to act on:

| evidence | what it means | how to act |
|---|---|---|
| `symbol` | both images name the referent and the names differ | an identity defect: prove the right callee/ctor/global by xref before changing anything |
| `string literal` | the TEXT at the two targets differs | decisive, and usually a one-line fix — but read retail's bytes, never guess (`"Num Runs"` vs `"Num_Runs"` is exactly what a plausible guess gets backwards) |
| `weak / content only` | at least one side is only a width-bounded byte window over an unnamed target | real, but **not proof**; do not spend symbol-class effort on it |

A historical sample from the former `data_value` scoring mode. Each defect scored
100% there; strict `all` scoring now makes named-target differences visible:

| region | retail reaches | we reach |
|---|---|---|
| `CGruntzMgr::Close` | `"Num Runs"`, `"Num Movies"`, `"High Detail"`, `"Disable Joystick"` | `"Num_Runs"`, `"Num_Movies"`, `"High_Detail"`, `"Disable_Joystick"` |
| `ButeMgr::ParseAttributeFile` | `"ButeMgr:  duplicate symbol encountered - %s"` | `"ButeMgr:  duplicate tag encountered - %s"` |
| `CStatusBarMgr::LoadTabSprites` | `CSBI_RectOnly::CSBI_RectOnly` | `CStatusBarItem::CStatusBarItem` |
| `CGrunt::ResetGeometry` | `ActFindId("E")` | `ActFindId("A")` |
| `RegisterGruntActions` | `_zvec::IndexToPtr` | `zDArray<…>::Resolve` |

The registry-key underscores and the ButeMgr message are byte-level source bugs; the
`CSBI_RectOnly` and `_zvec` rows are modelling divergences (wrong class constructed,
different container API).

### Resolved asymmetries that were NOT defects

A wrong-referent row is only worth a reader's time if a *correct* candidate could
have produced a matching one. Four readings could not, and each manufactured rows in
the hundreds. All four are one-sided by construction: whatever retail happens to
NAME, our `.map` does not, so the same target resolved differently on the two sides.

* **A relocated pool window.** Rule 5 fingerprints an unnamed target by the 8 bytes
  there. When those bytes are themselves relocated — a pointer table, a CRT `.data`
  slot holding an address — they are a **placement fact** and can never agree.
  `<a0cc5200a0cc5200>` vs `<10ba590010ba5900>` is 0x52cca0 vs 0x59ba10, the same
  object at two addresses. Now UNDECIDABLE. **208 regions.**
* **An IAT slot read as text.** The slot holds the hint/name-table RVA until the
  loader overwrites it, and carries no base relocation, so `36 55 2c 00` reads as the
  NUL-terminated string `"6U,"`. The import table names the slot on **both** sides,
  so that identity now wins outright. **~300 targets** (latent until the length floor
  below was lifted, which is why it never showed before).
* **A literal shorter than 4 characters.** Rule 3 required 4 chars, so `"A"` fell
  through to rule 4 — where retail's manifest names it `??_C@_01PFH@A?$AA@` and we
  name it nothing. Every one-character literal in the image read as a wrong referent.
  The relocation table, not a length floor, is what tells text from a stored address.
  **159 targets.**
* **A literal carrying `\n`.** `content()` tested printable-ONLY, so
  `"Heap stats: …%lu\n"` was demoted to an 8-byte raw window on the side that has the
  newline while the other side, lacking it, resolved as a string. `\t\n\r` are text.
  **103 targets.**

A fifth, different in kind: **an interior self-reference**. A switch jump table names
its OWN function at a codegen-dependent offset, and retail's carve routinely ends
before its table while our "up to the next public" extent does not. The referent is
"this function" on both sides and the offset carries no identity, so interior
self-references are excluded from the sequence (a self-**call**, offset 0, is a real
referent and stays). The bytes are still compared: a jump table with different
targets tokenises differently and lands in `slot not aligned`. **50 regions.**

Together these took the worklist from **610 to 262** without a single source change.

A sixth asymmetry was in the sequence comparison rather than the resolver:
**`<?>` is not a referent**.  An extra unnamed operand on one side used to split an
otherwise equal decidable sequence and make an ordering-only body look as if it
reached different assets.  `CGrunt::LoadPickupSprites` is the counterexample: both
images reach the same complete pickup-key multiset, while retail has two additional
undecidable operands and emits the case bodies in a different order.  Unknowns are
now counted, then removed before identity/order comparison; ordering-only regions are
counted separately by `--referents` and listed by `--ordering`.

That sixth correction removed another 14 false-positive regions: at that snapshot the
static worklist was **248 genuine wrong-referent regions**, plus **40 ordering-only**.
A seventh reporting defect did not change that total but did change how it should be
worked: lexical `min()` let a later string descriptor downgrade an already symbol-proven
region. The three apparent string-literal rows were all mixed structural regions; with
monotone evidence precedence the honest triage is **219 symbol**, **29 weak/content-only**,
and **0 literal-only**. A negative control keeps a later plausible string from erasing
stronger identity evidence.

An eighth asymmetry was the candidate-side byte cutoff. Retail has a complete carved
extent, while a candidate public runs to the next public and can be longer. Comparing
operands only through `min(retail_size, candidate_size)` therefore discarded real call
sites that merely moved later in a longer reconstruction. `BlitShadedMirrored` is the
decisive control: its 4,637-byte retail body and 4,844-byte candidate body both contain
exactly three `ConvertRowDouble` calls and four `ConvertRowFlip` calls, but one candidate
pair lay beyond the old cutoff. The audit now admits a candidate-tail referent only when
that exact identity fills a deficit in retail's multiset. A wrong or surplus tail
referent cannot hide a missing call. On the same candidate snapshot this changed
**220 wrong / 39 ordering-only** to **182 wrong / 44 ordering-only**: 38 regions left
the wrong list, five were correctly retained as ordering-only, and the combined
worklist lost 33 false positives.

A ninth false-positive class was a **pool window wider than the load**.
`UpdateMgrScroll` has two `fmul DWORD PTR` operands that both reach the retail
`-0.05f` value. The old eight-byte raw fallback also compared the unrelated next
pooled literal (`00000000` vs `9a999999`), manufacturing two divergences in one
region. The resolver now recovers the direct x87 real-memory width from the opcode:
four bytes for m32real and eight for m64real, retaining eight for unknown forms.
The negative controls prove all three cases. On the ordinary linked candidate this
removes **1 false-positive region**; the authoritative retail-order/real-archive
candidate already placed the neighbouring constants alike, so its ratchet stays flat.

A tenth defect was inside the sixth correction itself.  The wildcard aligner handled
one-sided unknowns correctly when the surrounding order was stable, but a large
permutation could align those unknowns beside different known operands and then delete
different identities from each side.  `LoadPickupSprites` is the decisive control:
before wildcard alignment, both images have the exact same **131 decidable referents**;
after the old alignment, it falsely claimed retail-only CONVERSION/SUPERSPEED and
candidate-only DEATHTOUCH/REACTIVEARMOR.  The complete decidable multiset is stronger
evidence than any wildcard placement, so equality is now decided first and only the
ordering comparison sees the unknown-free sequences.  A minimal four-element
permutation is a negative control in `--selftest`.  On the authoritative candidate this
moves the wrong count **77 -> 72** and exposes six ordering-only bodies (**33 -> 39**):
five were misclassified as wrong and one had been over-suppressed entirely.

An eleventh classification defect blended **referent multiplicity** into identity.
VC5 can merge several source arms onto one call site or factor repeated global loads;
the reverse spelling can emit more operands while every one still names an identity
present on the other side.  `CGrunt::LoadGruntTypeTable` is the compact control:
retail shares three health cases across two `GetIntDef("Powerupz", ...)` call sites,
while our build shares all three across one.  `CShadeTableCache::FlashTable` similarly
factors repeated `g_p01` loads.  Call/load count can matter, so these regions are not
discarded: `--multiplicity N` lists them and the integrity gate ratchets them
separately.  They are no longer called wrong-target evidence.  On the authoritative
candidate this partitions the 72-region bucket into **34 genuinely different
identities** and **38 multiplicity-only regions**.

At that stage, with the corrected audit, derived object order, and reconstructed
static archives, the authoritative worklist was **33 wrong-referent regions** (23
symbol-proven, 10 weak/content-only), **39 ordering-only regions**, and **39
multiplicity-only regions**.

A twelfth resolver defect treated printable bytes as stronger than a paired data
symbol.  That mislabeled `g_levelMsgRectsB[0].top` (integer value 92) as the string
`"\\"` and `g_sndCueTag` (100) as `"d"`.  Paired non-literal symbols now win over
content sniffing; compiler `??_C@` literals still compare by text.  This correction
also exposed two previously hidden CRT identity differences: retail's
`___lc_lctostr` and `___init_numeric` reach named `g_dot`, while our link reaches a
pooled `"."` literal.  The honest wrong-region count therefore rises **34 -> 36**.
The selftest uses the `RECT` field as a negative control.

`CChatBoxOwner::ProcessCheatInput` then supplied the first source correction from the
identity-only list.  Retail pushes `??_C@_00A@?$AA@` and calls
`CString::CString(const char*)` for the `group` local; `CString group = "";`
reproduces that evidence.  The region leaves the wrong-identity bucket (**36 -> 35**).
It remains in the multiplicity worklist (**38 -> 39**) because our frame emits one
additional default-constructor/destructor pair; that is still tracked code-shape
evidence, but no operand reaches a CString identity absent from retail.

Two duplicate MFC labels then hid one real call defect.  The bodies decide their own
identities: retail `0x1b2a0f` calls `_mbsrchr` and is
`CString::ReverseFind(char)`; `0x1b2a30` calls `_mbsstr` and is
`CString::Find(const char*)`.  They are not second copies of `Find(char)` and
`FindOneOf`.  Correcting the catalog removes those two false rows and exposes
`CFecFile::AddFile`, whose path split really did call `Find('\\')` where retail calls
`ReverseFind('\\')`.  Reconstructing that call takes the authoritative wrong-identity
count **35 -> 33**.  Objdiff cannot reflect this fix because it masks the call target;
the linked-referent gate is the acceptance evidence.

A thirteenth false-positive class was **asymmetric region pairing**.  The pairing
contract rejects a symbol name that occurs more than once on either image, because
choosing one copy would fit the answer.  The implementation enforced that only on the
candidate side.  Repeated retail FID labels such as `??_GCWinApp` therefore claimed
our single real function with whichever unrelated byte-identical retail body appeared
first.  Pairing now builds the retail name-to-address set too and admits a name only
when it identifies exactly one region on both sides.  A duplicate-retail/single-
candidate negative control is in `--selftest`.

The audit also now reads `CRuntimeClass.m_pfnCreateObject` at `+0x0c`: the class's own
runtime record points directly to its factory, proving identities FID cannot
distinguish.  It corrected six catalog rows (including `CMapStringToOb` and
`CDWordArray`; four were latent).  Static wrapper/callee evidence corrected
`0x12d1b0` from `__lseek` to `__write`, and vtable slots corrected the `CDialog` and
`_AFX_CTL3D_STATE` scalar destructors.  Together with symmetric pairing, this reduces
the current authoritative worklist **33 -> 25**: **15 symbol-proven**, **10
weak/content-only**, **39 ordering-only**, and **39 multiplicity-only** regions.

`CGruntzMgr::SetGruntColor` reaches the same RED/GREEN/BLUE/PURPLE asset keys as
retail, and the pickup loader reaches the same decidable key multiset. The observed
yellow-to-blue startup symptom is therefore not explained by a wrong named-asset
referent in this path; its remaining cause is elsewhere (for example the player/color
index or palette path), and static evidence does not justify guessing one.

## The ordering-only worklist

`--ordering N` prints the regions the wrong-referent worklist deliberately excludes:
paired bodies that reach retail's exact decidable referent **multiset in a different
order**. Nothing here points at the wrong thing — the multiset test has already
proven every referent correct — so the defect is *sequence*: statement order,
evaluation order or emitted-branch layout in the source, and the fix is a reorder,
never a retarget.

Regions are ranked easiest-first by **displaced operand slots** — sequence positions
outside the retail/candidate common subsequence. A referent that only *moved*
appears twice (deleted where retail has it, inserted where we emit it), so 2
displaced slots is ONE referent out of place, usually a two-statement swap in the
source. The listing opens with that histogram, and the grouped tally is asserted
against the section counters — a truncated listing is not a worklist. The dominant
current shape is a game-object constructor whose asset-key literal sits one
statement away from where retail evaluates it.

All three counts are ratcheted together by `gruntz.audit.data_integrity`
(`config/retail/data-integrity-ratchet.tsv`, full tier): none can silently increase,
and a genuine lower count must be banked with `--write`.

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
    [PASS] an IAT slot resolves to its IMPORT, not to the hint RVA read as text
    [PASS] a relocated pool window is UNDECIDABLE, not a byte mismatch
    [PASS] a literal shorter than 4 chars resolves to its TEXT
    [PASS] a literal carrying \n resolves to its TEXT
    [PASS] no wrong-referent row is an interior SELF-reference (a jump table)
    [PASS] an undecidable operand cannot manufacture a wrong referent
    [PASS] unknowns cannot turn a proven referent permutation into an identity defect
    [PASS] a repeated use-count difference is multiplicity, not wrong identity
    [PASS] a genuinely absent identity remains wrong
    [PASS] a duplicate retail name cannot claim one candidate region
    [PASS] a paired RECT field outranks coincidental one-byte string content
    [PASS] two swapped relocated dwords -> ONE ordering-only region
    [PASS] ...and NOT a wrong-referent region (the multiset is intact)
    [PASS] ...and the ordering worklist attributes it to the right region
    [PASS] a correct referent moved past retail's byte extent is retained
    [PASS] a wrong candidate-tail referent cannot fill that deficit
    [PASS] .rsrc: every differing byte is a classified placement shift
    [PASS] every section counts its UNMEASURABLE bytes in the DENOMINATOR

The closure assert is not decoration: an early return added to the region differ
silently dropped 1,340 B of operand-free `.rdata` regions until it fired.

The four resolver checks are **not** vacuous, and that was verified rather than
assumed: restoring each pre-fix behaviour one at a time makes exactly its own counter
go non-zero — no IAT guard **302**, no relocation guard **130**, a 4-character floor
**159**, printable-only text **103**. The literal check reads the bytes of every
`??_C@` symbol with its OWN local text predicate: asking `TEXT_BYTES` would be asking
the constant under test, and the check would pass by agreeing with itself.

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
* **The retail sequence uses the complete retail carve.** Candidate operands through
  the common byte extent are compared directly. Beyond it, only exact identities
  needed to fill a retail multiset deficit are admitted; this retains calls displaced
  by a longer candidate body without treating compiler tables after a public as new
  referents.
* **The `weak` resolution class** (8 bytes at an unnamed data target) can in
  principle collide — two distinct pool constants with the same leading 8 bytes. An
  all-zero window is not evidence at all and is demoted to UNDECIDABLE, as is a
  window overlapping a relocation. The surviving weak rows are reported as their own
  evidence class by `--referents` and should not be worked like symbol rows.
* **The weak window can straddle the datum.** It is a fixed 8 bytes, so for a 4-byte
  global it reads the neighbour too, and the two images order their neighbours
  differently. The residue is visible as pairs whose FIRST dword agrees and second
  does not (`<0200000000000000>` vs `<0200000020059319>`) and is concentrated in the
  CRT. Bounding the window by the target's extent needs an extent source neither
  image publishes for an unnamed datum.
