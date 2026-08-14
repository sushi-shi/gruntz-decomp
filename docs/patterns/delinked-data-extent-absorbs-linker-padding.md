# A `.rdata`/`.data` symbol whose target extent is 2-6 bytes long has absorbed the linker's inter-COMDAT padding

**Tags:** `data:rdata` `data:data` `data:objdiff` | `topic:wall` `topic:scoring-artifact` `topic:tooling`
**Confidence:** c9 (measured; retail bytes read at each boundary, both directions)

## Symptom

A data section is 99.x% with one `SIZE` row where the *target* extent is a few bytes
longer than the base's, on a symbol whose bytes are not in doubt:

```
anirecord   .rdata=104   ??_7CAniRecordView@@6B@   target=24 base=20
fader       .rdata=96    ??_7CFader@@6B@           target=24 base=20
ddrawsubmgr .rdata=864   ??_7CLoadable@@6B@        target=40 base=36
areamgr     .data=52     ??_C@_01NON@_?$AA@        target=4  base=2
netmgr      .data=112    ??_C@_0BK@…NetMgr?4cpp?$AA@  target=32 base=26
sbi_wellgoo .rdata=544   ??_R2CSBI_Image@@8        target=16 base=13
```

The `+4` on ten vtables reads like "retail has one more virtual". It does not.

## Mechanism

Each of these is its own COMDAT in the base object, so cl gives it its **exact** size
(a 5-slot vtable is 20 B, `"_"` is 2 B, a 3-entry `??_R2` base-class array is 13 B).
`link.exe` then aligns the *next* COMDAT, leaving 2-6 dead bytes after it in the image.

The delinker groups a unit's data by retail adjacency, so two neighbouring COMDATs
become **one** target section — and objdiff, which has no COFF symbol sizes, infers
`size = next symbol's offset`. The first symbol therefore measures *its own size plus
the linker's padding*, and can never equal the base's exact COMDAT size.

Read the retail bytes at the boundary and the padding is visible:

* `??_7CFader@@6B@` at `0x1f07a8` — five slots to `0x1f07bc`, and `0x1f07bc` holds
  `3a83126f`, i.e. the **float `0.001f`** from fader's own FP pool, not a sixth slot.
  Next vtable at `0x1f07c0`.
* `??_7InterfaceObject@@6B@` at `0x1f0748` — five slots, `0x1f075c` is `0`, next
  vtable at `0x1f0760`.
* `?g_defaultZ@@3IB` at `0x1f04e8` (4 B, value 24) — `0x1f04ec` is four unreferenced
  zero bytes, then a `double` at `0x1f04f0`. Here retail's real gap IS 8 and it is the
  *target* that is short (4), because that section is a synthetic repack of three
  non-contiguous `.rdata` runs (`0x1eaa88`, `0x1eab00`, `0x1f04e8`) packed at 4.

So the mismatch runs in **both** directions and neither side is wrong about the datum.

## Not steerable from `src/`

`.rdata` is emitted in declaration order with the anonymous FP-pool entries appended
*after* every named global, so a trailing 4-byte named constant can never be made the
last symbol in the section. Making the pool entries named statics would reorder them
ahead of it — but they already carry `DATA_COMPGEN` pins precisely because they have no
source definition, and giving them one to buy a layout is the fitted-artifact move.

Never "fix" this by declaring the padded extent (a 6-slot vtable, a `char[32]` for a
25-character literal). That writes the linker's slack into the model and would hide a
genuinely missing slot — which is the same evidence shape.

## Pinning the boundary datum does NOT close it — tested and refuted

The obvious fix is to pin whatever occupies the slack, so the previous symbol's extent
ends at its true size. It does not work, because **the delinker packs a group by
declared size and ignores each symbol's RVA offset within the group.** Measured on
`fader`: `0.001f` at `0x1f07bc` is a real, referenced datum (both refs land inside
`RunFadeStepped` 0x17e540 and `RunFade` 0x17e620), and pinning it put it at section
offset **0x2c**, after `??_7CObject@@6B@`, not at 0x14 where its RVA says. 98.58 ->
98.65, `??_7CFader@@6B@` still 24.

That also exposes the actual culprit: `??_7CObject@@6B@` is emitted by dozens of units,
so `data_manifest.py` withholds it from the *section* manifest (`duplicate name in
manifest`, 557 rows) while still enrolling it as a datum with `section_ordinal = -`.
Unplaced rows get appended to the unit's first candidate section, and the append is
8-aligned — which is exactly the phantom 4 bytes. Pin the constant anyway if the
evidence is there (it is real data and a real name), but do not expect the score.

## What DOES close it

* the fix is upstream — emit COFF symbol sizes on the target side, give every
  unplaced datum its own section row instead of appending it to a neighbour's, or let
  `data_manifest.py` enroll the vtable extents it already knows (`config/retail/
  data_vtables.tsv` carries `0x14` for exactly these vtables; `labels.py` deliberately
  drops the size because the manifest already enrolls the vtable, and feeding it twice
  aborts the delink with `duplicate data RVA` — measured 2026-08-09).

## Scope

2026-08-09 census, of 75 data sections below 100 not held by another lane: **41 are
this artifact alone** (`SIZE`-only), totalling ~11 KB of `matched_data` that no source
change can reach. Screen for it before spending a session on a 99.x% data section:
`??_7`/`??_C@`/`??_R[0-3]` with `target - base` in 1..7 is this, every time.

## See also

- [`bss-symbol-size-inference-hole.md`](bss-symbol-size-inference-hole.md) — the `.bss`
  form of the same inference hole, plus the name-hash layout finding.
- `scripts/gruntz/build/data_manifest.py` — the enrollment and its withheld list.
