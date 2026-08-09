# Data matching: what the 100% does NOT mean

**`Data: 100.00% size-weighted · matched_data 723,491 / 723,491 B (100.00%)` is true and
it is narrow.** It says *every byte we enrolled is byte-equal to retail's*. It does **not**
say the data is correctly modelled, and this file exists because those two are routinely
confused — including by the author of the scoreboard text.

The distinction is not academic. A wrong model that scores 100% is worse than a low score,
because a low score is a worklist and a wrong 100% is a closed question that is not closed.

> Live example, in the tree today: `g_clut` — the game's colour lookup table — is pinned
> **two bytes low**, every use site is biased `+2` to compensate, and it scores 100%.

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

As of this commit `data_access_map --findings unclaimed` reports **55 runs** (49 high, 6
medium) of data retail reads or writes that no `DATA()` claim covers — down from 69 runs /
4,229 accessed bytes, but not zero. Every one of those bytes is outside the 100%.

---

## 2. A wrong pin scores 100% when the code compensates

The score checks *the bytes at the address we claimed*. It does not check *that the address
is right*. If the pin is wrong and the source is written around the error, both sides agree.

### `g_clut`, the colour lookup table

`src/DDrawMgr/DDSurface.cpp` says it outright:

```c
DATA(0x00253c9e)
// 0x30002, not 0x30000: every user indexes the three 64K banks with a +2 bias
// (g_clut+0x2 / +0x10002 / +0x20002), so the top bank runs to g_clut+0x30001.
// Declared 0x30000 here, the 2-byte tail landed on whatever followed in OUR
// layout, which was g_rDown: it read 0xF000 instead of 3, so every red channel
// was mis-shifted.
u8 g_clut[0x30002];
```

The pin is **two bytes low**. The real object is `u8 g_clut[0x30000]` at `0x253ca0` —
8-aligned, ending exactly at `g_lut16`. That was proved independently by the delinker
alignment work: a `0x30002` array **cannot** start at an odd-word RVA under c2's alignment
rule (`docs/compiler-data-layout.md`). Someone made the bytes line up by declaring the
object oversized and biasing every use site, and it has already caused one real colour bug
(the mis-shifted red channel recorded in that comment).

### `g_imageClipRect`

`?g_imageClipRect@@3PAHA` at `0x2bf28c` is declared `i32[4]` (`src/Image/CImage.cpp:29`).
A 16-byte array would be 8-aligned; a 4-mod-8 start fits **four separate `i32` globals**
(`left`/`top`/`right`/`bottom`). Byte-neutral today, wrong as a model, and it changes
codegen when fixed.

**Neither of these costs a single byte of the 100%.** Both are queued for a source lane.

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

## 4. The size/similarity confusion, one level up

The README's **Link status** block reports per-section *sizes* of the linked candidate
against retail. Two images can match on every section size and share almost no bytes.
`.idata` and `.rsrc` show `+0` there, which means "same length", not "same bytes".

A per-section byte-similarity figure — aligned and reloc-masked, because our `.text` is
short and every relocated word differs by construction — is in progress. Until it lands,
read that block as a size check only.

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
* Is the address right? c2's alignment rule must divide the retail RVA
  (`docs/compiler-data-layout.md`); a violation is a mis-pin, which is how `g_clut` and
  `g_imageClipRect` were caught.
* Is the referent right? `gruntz audit data_relocs` and `python -m gruntz.audit.assert_relocs`.

## Related

`docs/data-attribution.md` · `docs/compiler-data-layout.md` · `docs/data-access-map.md` ·
`docs/link-section-audit.md` · `docs/gotchas.md`
