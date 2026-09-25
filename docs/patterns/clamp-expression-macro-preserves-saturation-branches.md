# A clamp expression macro preserves saturation branches that nested templates lose

tags: cpp:macro cpp:inline cpp:template cpp:return | asm:setle asm:and asm:jge asm:call | topic:source-shape topic:cfg
symptoms: a sourced nested Min/Max clamp replaces conditional saturation with a mask and merges a caller continuation
confidence: 10/10

## Controlled complete-family result

`CAmbientSound::ScaleVolume` scales a signed integer through two separate
percentage divisions and saturates the final result to 0..100. Its five original
expansions occur in four retail owners: ApplyMasterVolume, StartPlayback,
SetVolumeLevel and both volume phases of positional Update. All four are exact
before this source reassessment, including their 14 ordered original references.
The complete helper is retained in every control; only its saturation changes.

| Saturation source | ApplyMaster | Start | SetVolume | Positional Update |
| --- | ---: | ---: | ---: | ---: |
| Two early bound returns, then value return | 100 | 100 | 100 | 100 |
| `return Clamp(volume,0,100)` | 85.2439 | 90.5645 | 87.2917 | 96.6092 |
| `volume=Clamp(volume,0,100); return volume` | 85.2439 | 90.5645 | 87.2917 | 96.6092 |
| `return LTCLAMP(volume,0,100)` | 100 | 100 | 100 | 100 |

The authored template is `Min(max,Max(val,min))`. The independently sourced
AVP2 macro is a lower-first conditional expression: select the lower bound when
below it, otherwise select the upper bound when above it, otherwise the value.
It is not an invented in-place statement macro. Its pure named integer arguments
here make repeated evaluation harmless; that is not a general macro contract.
Canonical provenance/adoption is `reassess-ambient-avp2-ltclamp`.

Both nested-template spellings replace the lower branch with
`xor; test; setle; dec; and`. ApplyMaster loses the original negative-path
SetVolume call/return, becoming one call instead of two. The other four
expansions keep their common call but lose conditional edges. The macro restores
the complete normalized bytes, extents and ordered references of all four
owners: 114, 169, 126 and 479 bytes. Header-dependency-only control was flat
across all 36 scored TU bodies. No newly exact function is claimed.

## Composition and limits

This was not a first-dip rejection. Both positional Clamp sites and both Fade
scaler boundaries were composed on the nested-template base. They did not
recover the missing branch texture. The positional selectors have a different
original upper-first order: merely putting the lower-first macro there gives
91.8966%, and upper-first by-value Min/Max gives 89.4828%, versus the exact
479-byte baseline. These outcomes are scoped in the lineage ledger, not a
global rule against either abstraction.

Restoring the existing ScaleVolume call in both Fade start arms, with the
original multi-return or macro scaler, recovers Fade from 88.1250% to 91.8056%.
It remains 380 bytes/134 instructions/6 calls/18 branches/4 returns/7 references
against retail's 414/144/7/17/5/8. The missing duplicated negative SetVolume
continuation and fade-out incoming-parameter home remain open. A same-TU
inline SetVolumeLevel plus all three complete Fade operations is flat in Fade,
removes the actual standalone retail owner and changes earlier/later callers;
no artificial keeper is retained.

Use this signature to test the complete authored macro boundary before removing
a helper because its inline-template analogue loses branches. Do not rewrite
the shared template's comparison globally or assume equivalent integer results
prove source identity. Compare against the original baseline, including complete
caller effects and repeated call multiplicity. Generic semantic summaries that
deduplicate adjacent referents cannot prove this property: Fade's summary says
7/7 while the actual ordered references are 7/8.

`scripts/test_ambient_scaler_consumers.py` checks the four complete scaler owners and
all their raw referents. It must reject altered arithmetic, signed guards,
wrong callees and missing repeated references, not merely recognize a clamp
mnemonic sequence. It does not certify the still-open Fade owner.
