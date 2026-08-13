# A `static char s_X[] = "lit"` declared in TWO TUs is ONE pooled literal

`c10` — `cpp:global cpp:string msvc5:gf` | `topic:data topic:correctness`

## Symptom

`gruntz audit reloc_multiset` reports, in one function, our base naming a static
and the delinked target naming something else at the same place:

```
-- grunt  ?LoadAnimNameTable@CGrunt@@QAEXHH@Z
     _s_pose_DEATH$S41745    base 1 target 0
     _s__DEATH$S34943        base 0 target 1
```

or, when the other side is a bare literal in some TU:

```
-- mainmenubuilder  ?BuildMainMenuTree@@YAHPAVCChatBox@@H@Z
     _s_BACK$S30550          base 13 target 0
     ??_C@_04FGAG@BACK?$AA@  base  0 target 13
```

The masked diff shows nothing: both sides push a DIR32 address, so the code bytes
are identical and only the relocation's SYMBOL NAME differs.

## Why

`/O2` implies `/Gf`, so cl emits every string literal as a `??_C@` COMDAT and the
linker folds identical ones across the whole image. Retail therefore has exactly
ONE address per distinct literal, shared by every TU that spells it.

A file-scope `static char s_X[] = "lit"` cannot be that: a file-static is not
visible to another TU, so if two TUs' relocations resolve to the SAME retail
address, neither of them wrote a static. The duplicate declaration is the
fabrication, not the literal.

## The two decisive tests

1. **Cross-TU sharing of a file-static.** `battlezmapconfig` spells a bare `"C"`,
   and its delinked target relocs to 0x0020cc90 — the address GruntAssetLoaders.cpp
   pinned to `static char s_dAnimKeyC[]`. A file-static cannot be referenced from
   another TU, so 0x0020cc90 is the pooled literal.
2. **The build already knows.** `build/gen/delink_data_manifest.tsv` carries TWO
   rows for such an address: a `candidate-COFF-string` claim from the unit that
   spells the bare literal, and a `provisional-pooled-literal-alias` row for the
   static. Grep for that kind — it is a complete, build-computed worklist.

Neither test needs a disassembly read.

## The fix

Every site becomes the bare literal. A pin is kept only when the content oracle
cannot reach the address (an ambiguous 1–2 byte payload the inference withholds —
`docs/data-attribution.md` §3b-iii): then exactly ONE `DATA_COMPGEN(rva, "lit")`
stays at a use site **in the unit that currently owns the pin** — `labels.py`
resolves it against the `??_C@` COMDAT in that TU's own base obj, so the use site
must spell the literal verbatim. Every other literal needs no pin at all: the
delinker names an unclaimed pooled literal from its content, re-proven each build.

Byte-neutral in the instruction stream; only the relocation's name changes.

## Measured

* 37 fabricated declarations removed over two passes (the `s_code*`/`g_sepSlash`
  family, the 14 `provisional-pooled-literal-alias` rows, and 23 statics
  duplicated across TUs). `3493 -> 3515` exact.
* `?BuildMainMenuTree@@YAHPAVCChatBox@@H@Z` 99.95 -> **100.00 EXACT** (13
  references to one literal).
* It also kills a banned construct: the eight `extern char s_codeX[];` in
  `Grunt.h` and `extern char g_nameFmt[];` in `PortalPath.h` existed only to share
  the fabricated statics.

## Trap

Three of the statics (`s_codeK`, `s_codeO`, `s_codeS`) had **no use at all** —
pure data-side fabrications that existed only to hold a pin. Deleting one is
coverage-safe only when another unit already claims the address; check the
`candidate-COFF-string` row before removing the last claimant.
