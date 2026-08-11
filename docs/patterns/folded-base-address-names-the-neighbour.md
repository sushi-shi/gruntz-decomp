# A folded constant index makes the delinker name the NEIGHBOURING global

`c10` — `cpp:global cpp:loop` | `topic:scoring-artifact topic:wall`

## Symptom

`gruntz audit reloc_multiset` reports "one reference moved to the neighbour", in
every function that walks the same array:

```
-- fileimage  ?DecodeBmp@CDDSurface@@QAEHPAVCDDrawPtrCollections@@PAXI@Z
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

## Do not "fix" it

Spelling the loop bound as the NEIGHBOUR symbol would make our reloc match the
delinker's guess rather than what the compiler really emitted. That is a fitted
artifact, and it puts a lie in the source to buy hundredths.

## Recognizing it

`reloc_multiset` flags the pair automatically — rows print
`[one-past-end artifact]`. It reads the extents out of `build/gen/symbol_names.csv`
and flags a target-only symbol whose extent ABUTS a base-only symbol's extent on
either side. 12 of the 146 remaining worklist functions are fully explained this
way. When checking by hand, compare the two symbols' RVAs and sizes: adjacency IS
the proof.
