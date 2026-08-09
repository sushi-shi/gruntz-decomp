# MSVC 5.0 data layout: how cl orders globals in .data / .rdata / .bss

Reverse-engineered from the toolchain binaries (`c1xx.dll` front end,
`c2.exe` back end) and validated by a blind predictive test: **41/41 exact**
placements (offset + alignment + order) on a fresh TU of never-probed random
names — 9 `.data` + 5 `.rdata` + 27 `.bss`, covering collisions, extern
pinning, local/class statics, and every alignment branch. Probe flags were the
real unit flags (`/nologo /c /O2 /MT /GX /GR`).

Checkable predictor + reverse tool: `python -m gruntz.audit.data_layout`
(`predict` = declaration list -> section layouts; `infer` = observed slots ->
name/decl-order constraints; `hash` = per-name h/check16/bucket).

## Architecture: who decides what

`CL.EXE` runs `c1xx.dll` (front end) then `c2.exe` (back end, UTC "P2"); they
meet in four temp files (`*ex`, `*gl`, `*in`, `*sy` — poll-copy the wine TEMP
dir to capture them). The split of responsibilities:

1. **Initialized data**: the front end streams an initializer record per
   definition, at the point of definition, into the `in` stream. c2 assigns
   `.data`/`.rdata` offsets as those records arrive. **=> declaration order,
   always.** (This is a mechanism, not a tendency.)
2. **Uninitialized data**: no initializer record exists; the symbol is emitted
   only by **c1xx's end-of-TU walk over the global scope's symbol hash
   table**, and c2 assigns `.bss` offsets in record-arrival order. **=> hash
   order, never declaration order.** The captured `gl` stream already lists
   the records in the final `.bss` order.
3. **C, not C++**: uninitialized C globals are tentative definitions and
   become COFF COMMON symbols — no per-TU placement at all (linker decides).
   Gruntz is C++, so its per-TU `.bss` blocks come from mechanism 2.

## The .bss walk, exactly (read from c1xx.dll, image base 0x10400000)

Identifier intern (lexeme table) — `FUN_1040e132`, hash inlined in the lexer
(e.g. 0x1040edc2), by-cstring variant 0x10406608:

    h = 0
    for c in identifier:  h = h*4 + (h >> 4) + c        # uint32
    check16 = (h >> 16) ^ (h & 0xFFFF)                   # stored in name node

(The lexeme table itself is 2048 buckets `h & 0x7FF` with move-to-front
chains; it only supplies `check16`.)

Symbol-table insert — `FUN_1040b2b6` (vtable slot of the scope-table class;
allocation of the bucket array in `FUN_1040b0d6`):

    bucket = check16 & table->mask        # global scope: mask = 0x3FF (1024)
    sym->next = table->buckets[bucket]    # PREPEND (LIFO)
    table->buckets[bucket] = sym

End-of-TU walk — `FUN_10403ea6` (slot 9/10 of the two scope-table vtables at
0x104C92C8 / 0x104C98DC; nested-scope lists via `FUN_10433ef4`; per-symbol gl
record emit via `FUN_104039ee`):

    for bucket in 0..mask:                      # ascending
        for sym in chain head..tail:            # LIFO = reverse declaration
            emit record  (kind 4 = data symbol; 0x10/0x11 descend nested)

So, per TU:

- `.bss` order = sort by `(check16(name) & 0x3FF, reverse first-declaration)`.
- **What name is hashed:** the plain identifier for file-scope symbols
  (extern AND `static` — decoration/type never enters, proven by C/C++-linkage
  and type-variation probes). For **function-local statics** and **class
  statics** the front end interns the C++ **decorated** name and that is what
  hashes: `?x@?1??fn@@YAHH@Z@4HA`, `?s_m@CFoo@@2HA` (no leading `_`, no
  `$S<id>` suffix — those are appended later, in c2's `outdname`).
- The chain slot is fixed at the **first declaration** (an early `extern int
  x;` pins it; the later definition does not move it).
- **Use order is irrelevant** — the symbol table has no move-to-front (probe:
  referencing the earlier-declared member of a colliding pair does not swap
  them).
- Statics, externs, and (decorated) local/class statics interleave in ONE walk.

## Alignment (c2 side, VC5-specific; probe-derived, 41/41 predictive)

Per section, with a per-section **ratchet** that starts at 4 and evolves in
emission order:

    scalar double / __int64          -> align 8
    every other scalar (char..int)   -> align 4    (chars occupy 4-byte slots)
    array/aggregate, size > 8        -> align 8
    array/aggregate, size < 4        -> align 4
    array/aggregate, 4 <= size <= 8  -> current ratchet (8 if this section
                                        already placed an 8-aligned object,
                                        else 4)
    the ratchet is per-section (.data, .rdata, .bss independent); no trailing
    pad after the last object (section size ends exactly at the last byte).

This explains the previously-contradictory observations (`char[5]` getting 8
in one TU and 4 in another: a double or size>8 array earlier in the SAME
section flips the ratchet). The delink data manifest's synthetic "largest power
of two dividing both the rva and the size" matched none of this; it now calls
`data_layout.obj_align` (item 3 under "What this means for the project").

## VC5 vs VC6 (12.00.8964 SP5, homm2-buka toolchain)

- **Order: identical.** Same hash, same 0x3FF walk, same LIFO ties on every
  probe including the full blind TU. The c1xx symbol-table lineage is shared.
- **Alignment: different.** VC6 packs naturally (`char` at +1, `short` at 2,
  `char[5]` at 4, no ratchet quirk). Do not borrow VC6 packing intuitions for
  the VC5 delinker.

## What this means for the project

1. **`.data`/`.rdata`: "ascending retail RVA = declaration order" is a LAW,**
   not an empirical rule. Sorting `DATA()` pins ascending inside a TU's
   `.data`/`.rdata` contribution recovers the original declaration order
   exactly. Keep the convention.
2. **`.bss`: declaration order is the WRONG model.** Retail `.bss` order
   inside a TU is the hash walk over the ORIGINAL identifiers. Two
   consequences:
   - our recompiled `.bss` layout will generally differ from retail unless
     our chosen names happen to walk in the same order — a per-TU `.bss`
     offset mismatch against the delinked target is NOT evidence of a wrong
     member set;
   - the walk is a **name oracle**: a proposed original name for a `.bss`
     slot must hash into the bucket window between its neighbours
     (`data_layout infer`). With several known-original names in a TU this
     both validates attributions and refutes invented names (e.g. the eight
     `actionoptionsmenubar` `s_gruntDir*` names cannot all be original
     file-scope identifiers in that order).
3. **Delinker alignment**: DONE, 2026-08-09 (`docs/data-attribution.md`
   §3d-ii). `data_manifest._alignment` calls `data_layout.obj_align` instead of
   the synthetic rule. Object kind comes from the declared type —
   `build/gen/globals.json` (clang's qualType for the `DATA()` pin) or the MSVC
   mangled type in the symbol name — and the question collapses to *"is this an
   8-byte wide scalar?"*, because `size < 4` is 4 and `size > 8` is 8 for every
   kind. **The per-section ratchet is NOT recoverable** and is left un-latched
   at 4: c2 advances it in cl's EMISSION order inside the original TU, which for
   `.bss` is the hash walk over the ORIGINAL identifiers (unknowable — our names
   are reconstructions) and for `.data`/`.rdata` needs the whole section, while a
   row that still needs a modelled alignment is by definition one whose section
   could not be reconstructed. 4 is the conservative branch (it never fabricates
   padding) and every case the image can adjudicate agrees with it. The image is
   also the refutation: `align` must divide the object's retail rva, and the
   three rows where it does not are source defects the rule FOUND (`g_clut`
   pinned two bytes low; `g_imageClipRect` modelled as one array where the
   4-mod-8 rva says four separate `i32`s; and the one correct exception, an
   inline function's guard byte, which is a COMMON the LINKER places — see the
   six cases below, c2 never saw it).
4. Function-local statics participate in the same `.bss` walk under their
   decorated names — a TU's `.bss` is one interleaved pool, not
   "globals first, then per-function statics".

## Function-local statics: the six cases (probed, VC5 `/O2 /MT /GX /GR`)

`static` inside a function does not pick ONE storage. Which of the two
mechanisms above applies — or whether *neither* does — depends on the
initializer AND on whether the enclosing function is `inline`. Six probes,
`docs/probes/local-statics-1.cpp` + `local-statics-2-inline.cpp`:

**Non-inline function — a PRIVATE static, `$S<id>`-suffixed, placed by the TU:**

| initializer | symbol cl emits | storage |
| :-- | :-- | :-- |
| `static int hello = 4;` | `_?hello@?1??a@@YAXXZ@4HA$S167` | **`.data`**, declaration-order stream (rule 1) |
| `static int zed;` | `_?zed@?1??b@@YAXXZ@4HA$S171` | **`.bss`**, hash walk (rule 2) |
| `static int dyn = side();` | `_?dyn@?1??c@@YAXXZ@4HA$S176` **+** `_?$S1@?1??c@@YAXXZ@4EA$S178` | **both `.bss`** — the datum plus its 1-byte **guard** |

So a *constant*-initialized local static is ordinary initialized data and obeys
declaration order at the point the **function body** is parsed — in the probe,
`hello` (line 2) takes `.data+0`, `CFoo::s_m` (line 15) `+4`, `g_first` (line
19) `+8`, exactly source order. Only a *dynamic* initializer creates a guard.

Two naming traps: the hash input is the **decorated** name **without** the
leading `_` and **without** the `$S<id>` suffix (c2 appends both later, in
`outdname`), and the non-inline guard is spelled `?$S1@…@4EA`, **not** `??_B` —
grepping for `??_B` will not find it.

**`inline` function (i.e. anything defined in a header) — an EXTERNAL,
foldable symbol, and the TU does not place it at all:**

| initializer | symbol cl emits | storage |
| :-- | :-- | :-- |
| `static int inl = 7;` | `?inl@?1??d@@YAXXZ@4HA` | its own **COMDAT `.data`** section (`char=0xc0301040`), external |
| `static int zi;` | `?zi@?1??iz@@YAXXZ@4HA` | **COFF COMMON**, size 4 |
| `static int dv = side();` | `?dv@…@4HA` **+** `??_B?1??id@@YAXXZ@51` | **both COMMON** (4 B + 1 B guard) |
| `static CBar obj;` | `?obj@…@4UCBar@@A` **+** `??_B…@51` | **both COMMON**, plus a `.text` COMDAT `?obj@…@$AUCBar@@A` — the `atexit` dtor registration |

**This is the case that escapes both mechanisms.** A COMMON is a *request*, not
a placement: the **linker** chooses its address, so an inline function's local
static is outside the initializer stream AND outside c1xx's hash walk, and no
TU can be said to own it. That is precisely why
`config/retail/compiler-generated-data.tsv` exists (see `CLAUDE.md`) — such a
datum, and its guard byte, have no source spelling in any single `.cpp` to hang
a `DATA()` pin on, so only the retail ADDRESS is stated there.

Note also that the inline case emits the guard under a real `??_B` name while
the non-inline case does not, and that only the inline case takes a COMDAT — so
the same source line moved from a `.cpp` into a header changes the symbol's
name, its linkage, its section, AND which component decides its address.

## Prediction record

- Blind test (single fresh TU, names never used in any calibration probe):
  **41/41 exact** — every symbol's section, offset, and implied alignment, in
  all three sections, including a same-bucket collision pair (LIFO verified),
  an extern-pinned chain slot, decorated local/class statics, and the
  ratchet/no-ratchet `char[5]`/`char[7]` cases in all three sections.
- VC6 cross-check: same orders on all probes (order model corroborated),
  alignment deltas as expected from its natural packing.
- Calibration corpora (fitted, then re-verified): 16-name digit probe, 400-name
  IL-captured walk, 28-name mixed-length probe, 10-name mixed-size probe,
  static/extern interleave, local-static ruler probe (22 bucket rulers).
