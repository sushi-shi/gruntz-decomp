# A folded constant index makes the delinker name the NEIGHBOURING global

`c10` — `cpp:global cpp:loop` | `topic:scoring-artifact topic:wall`

## Symptom

`gruntz verify assert-relocs` reports "one reference moved to the neighbour", in
every function that walks the same array:

```
-- fileimage  ?DecodeBmp@CDDSurface@@QAEHPAVCDDrawDeviceManager@@PAXI@Z
     _s_palBmp$S27851   base 5 target 4
     _s_palPcx$S27852   base 0 target 1
```

The score sits a few hundredths below 100 and no source change moves it.

## Why

cl folds a constant index into the address constant:

| source | address cl relocates against |
| :-- | :-- |
| `while (d < &buf[N])` | `buf + N` — **one past the end of `buf`** |
| `buf[i - 1]` in a loop over `i` | `buf - 1` — **inside the PRECEDING global** |

Our base obj records that as `buf` plus an addend, because the compiler knows the
expression. The delinker has only the resolved ADDRESS, so it names whichever
symbol's extent contains it — the next global for the first form, the previous one
for the second. objdiff then compares two different symbol names and scores the
instruction unequal even though the byte at that address is identical.

`fileimage` is the whole worked example: six palette buffers laid out exactly
0x400 apart in declaration order (0x283ef0 `g_paletteRampBuf`, 0x2842f0
`s_palBmp`, 0x2846f0 `s_palPcx`, 0x284af0 `g_grayRamp`, 0x284ef0 `s_palPidData`,
0x2852f0 `s_palPcxData`, 0x2856f0 `g_warpU`), so every `&buf[0x400]` bound is the
next buffer's base and all six `Decode*` functions report it.
`?BuildSoundFontPath@@YAHD@Z` is the other direction: `g_sfDir[len - 1]` against
`g_sfDir` at 0x24dfa0 resolves to 0x24df9f, inside `g_sfRouterId` (0x24df9c, 4 B).

`SoundDevice::BuildVolumeTable` supplied the stronger negative control. Its
`i <= 100` loop relocates the terminal compare as `g_volumeTable + 0x190`.
An unsupported census fence at that address caused the delinker to invent a
neighbouring `g_panTable` identity and capped the otherwise-identical function
at 99.6667%. `SetPanPercent` then appeared to index backward from that supposed
object. Reconstructing the full data use showed one `g_volumeTable[101]` instead:
the pan code reads `g_volumeTable[100 - abs(idx)]`, the next real datum begins at
`0x253c4c`, and both functions are exact without a site oracle. A folded address
at a claimed boundary is not independent evidence that the boundary is real.

## Do not "fix" it

Spelling the loop bound as the NEIGHBOUR symbol would make our reloc match the
delinker's guess rather than what the compiler really emitted. That is a fitted
artifact, and it puts a lie in the source to buy hundredths.

The converse matters too: do not invent or retain a neighbour merely because the
delinker chose its address. `ChannelSlots_FindFree` walks
`g_soundChannelInUse[17]`; cl emits the loop limit as
`g_soundChannelInUse + 0x44`. A source claim for an otherwise unreferenced
`g_val_24c434` at that exact one-past address made the delinked target spell the
same operand as `g_val_24c434 + 0`, leaving the function at 99.5%. The retail
layout, source COFF relocation and declared array extent all say this is only the
array limit followed by four bytes of alignment before the next 16-byte physical
`GruntDirectionCell` contribution at `0x24c438`. Removing the unsupported datum,
classifying `0x24c434..0x24c438` as `pad`, and recording cl's exact
`g_soundChannelInUse + 0x44` spelling in
`config/retail/reloc_referents.tsv` restores the authentic relocation identity
through strict delinking and makes the function exact. The referent row preserves
the source COFF symbol/addend; it does not assert a datum at the resolved address.
Retain an adjacent symbol only when it has independent storage evidence; the
folded address alone proves no datum.

## Recognizing it

`gruntz verify assert-relocs` flags the pair automatically — rows print
`[one-past-end artifact]`. It reads the extents out of `build/gen/bindings.tsv`
and flags a target-only symbol whose extent ABUTS a base-only symbol's extent on
either side. 12 of the 146 remaining worklist functions are fully explained this
way. When checking by hand, compare the two symbols' RVAs and sizes: adjacency IS
the proof.

The initialized-data denominator must make the same distinction. A HIGHLOW target
at the first byte after an enrolled array does not prove a missing adjacent datum:
pair the retail code relocation with the candidate COFF DIR32 record and require
the latter to spell `array + sizeof(array)`. Pointer induction over a member has a
second form, `array + sizeof(array) + offsetof(member)`; accept it only when the
declared array type proves the element stride and the addend stays within that one
past element. The controls are uniqueness in both directions and agreement of every
other known relocation anchor in a positionally paired function. This removed two
false coverage roots: `g_cmdBitTable + 0x20` at 0x1e9628 and
`g_levelMsgRectsB + 0x8c` at 0x20b984. A different code relocation genuinely naming
the same address remains a root because the oracle suppresses relocation sites, not
target addresses.
