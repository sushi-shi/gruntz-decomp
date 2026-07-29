# Two TUs deriving the SAME string RVA proves nothing — retail pooled its literals
tags: topic:scoring-artifact topic:tu-partition | cpp:global — data:.data

symptoms: a `static const char s_x[] = "..."` in two or more of our .cpp files
pairs to ONE retail address; "76 statics derive the same RVA from 2+ TUs"; a
string constant at `0x208000..0x21c000` while every ordinary global of the same
TU lives at `0x244000+`; `[labels] MISS` / a DATA() row refused as ambiguous

confidence: 10/10

Retail `GRUNTZ.EXE` was built with MSVC5 **string pooling** (`/Gf` — pooled,
writable, hence `.data` and not `.rdata`). Every string literal became a
content-named COMDAT that the linker folded across objects, so ONE cell is shared
by every compiland that used the literal. A shared string RVA therefore carries
**zero** translation-unit information, and re-partitioning our `.cpp` files on it
would merge provably unrelated objects.

Three independent measurements, all on the retail image:

1. **Duplicate census.** `.data` holds 2178 distinct ≥4-char NUL-terminated
   strings and only **2** have any duplicate copy (the CRT's `"1252"` locale
   table, and two genuinely-different save-game messages). Without pooling, the
   dozens of TUs that spell `"Grunt"`, `"GRUNTZ_"`, `"%d"` would each carry their
   own copy.
2. **Layout.** All literals sit in one dense block at the HEAD of `.data`,
   `0x208000..0x21c000` (~75% string bytes per 16 KB page), physically separate
   from every ordinary global (`0x244000+`, `0x2bf000+`). That is the linker
   gathering `??_C@` COMDATs, not per-object `.data` contributions.
3. **Reference spread.** `"Grunt"` @`0x20a9ec` is referenced from **28** code
   sites between `0xa4b5` (`RegisterGameObjectTypes`, unit `gameobjectfactory`)
   and `0xd1da9` (`CPlay::ExecCommand`, unit `playercommandstep`) — 0xc7000 of
   `.text` apart. `"GRUNTZ_"` @`0x20d28c`: **104** sites, `CWarlord::CWarlord`
   (`0x42d40`) through `CState::BuildAssetNamespacePrefixes` (`0xdca70`). One
   `.obj` contributes one contiguous `.text` run, so these cannot be one object.

The pipeline already models this, and says so out loud. In
`build/gen/delink_data_manifest.tsv` these cells carry MSVC's content-hashed
string-COMDAT name and are attributed to SEVERAL objects at once:

```
0x20a9ec  ??_C@_05DNOF@Grunt?$AA@     gameobjectfactory.c / gruntbehaviorleaf.c
                                      / triggermgr.c / triggermgrgrid.c
0x20d28c  ??_C@_07EGPJ@GRUNTZ_?$AA@   grunt.c / play.c
```

2289 of 3482 manifest rows are `??_C@` string COMDATs, 2468 rows fall inside the
pool region, spread over 186 objects, and 372 rvas legitimately have more than
one owner. `gruntz.audit.data_tu_order`'s `COMDAT_NAME_RE` (plus its
multiple-owner rule) exempts exactly these from the per-(TU, storage) band
invariant. Only **ordinary** data is linearly TU-attributed.

## Corollary: spell a shared literal as a LITERAL, not a `static const char[]`

`SomeCall("GAME_BADSELECT")` compiles to a `??_C@` COMDAT — pooled, multi-owner,
exempt, and it is what retail did. Hoisting it into
`static const char s_gameBadSelect[] = "GAME_BADSELECT";` for readability turns it
into an **ordinary** single-owner `_s_gameBadSelect$S<n>` array, which
(a) cannot be bound when two of our TUs both want it, and (b) becomes band-forming
data that trips the interleave audit against its neighbours in the pool. That is
why `0x212c28` is unclaimed in the manifest while `0x20a9ec` is claimed four
times. Prefer the inline literal for anything living in the pool region.

## What IS partition evidence

Ordinary (non-COMDAT) data interleaving at dword granularity, and `.text`
containment. Both are contiguity violations no linker can produce from two
objects:

```
.rdata 0x1efb10  0.0            ex WallProject.cpp
       0x1efb14  -0.01745329    (unclaimed)
       0x1efb18  16384.0        ex WarpTextureBlit.cpp   <- interleaved
       0x1efb1c  -16384.0       ex WarpTextureBlit.cpp
       0x1efb20  0.5            ex WallProject.cpp
       0x1efb24  -3.1415927     ex WallProject.cpp
```

That run (plus the same shape in `.bss`) proved six `.cpp` files were ONE
rasterizer object; they were merged into `src/Image/ImagePolyClip.cpp`
(2026-07-29). The corresponding `.text` test is
`python -m gruntz.audit.tu_order_check` — a TU whose whole span sits strictly
inside another's, bracketed by that TU's functions, is a slice of it, and
`gruntz sema disasm` on the bracketing addresses confirms it.
