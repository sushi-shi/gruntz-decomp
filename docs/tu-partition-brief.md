# Brief: the TU-partition lane gates BOTH .text layout AND the data loop

For the dedicated lane. Written by the DATA lane after the contribution-manifest
attempt was measured and found blocked. Companion: `docs/data-attribution.md`.

> **CORRECTION (matcher-6, 2026-07-17) — READ THIS FIRST. The premise below,
> "ANY interleaving proves our TU partition != retail's compilands", is FALSE, and
> the 12134-pair headline is ~98% an artifact of it.**
>
> A compiland's contribution is contiguous **only for its ordinary (non-COMDAT)
> .text**. MSVC5 emits inline members, macro-expanded members, and compiler-generated
> ctor/dtor/`??_G` bodies as **COMDATs**, and the linker groups those into pools far
> from the owning obj's run. So a pooled body's RVA says nothing about which obj owns
> it, and `tu_order_check`'s INTER-TU invariant is over-strict for it. (`tu_layout.py`
> already knew this - `POOLS`, `--exclude-pools` - but `POOLS` is a hardcoded guess:
> its `(0x10000,0x14000)` misses the real pool's head at ~0xf2f0, and its
> `(0x80000,0x90000)` overlaps genuine compilands.)
>
> **Proof (not inference):** 0xf470 `CAniCycle::Serialize`, 0x10a10
> `CSecretLevelTrigger::Serialize` and 0x12bc0 `CToobSpikez::Serialize` are all
> **exactly 71 bytes and byte-for-byte identical** (all three `call 0x16e7f0`, zero
> relocations). MSVC5 has no `/OPT:ICF`, so they are three separate COMDAT copies of
> one macro-generated member that the linker pooled - not one compiland. The band
> 0xf2f0..0x13d00 is dozens of these: `Serialize` at +0x47 and `~X` at +0x44, over and
> over, for ~20 unrelated classes.
>
> **Measured, all-pairs:** of 190 outlier functions (5% of 3722) that cause **98.4%**
> of the interleaving (12099 pairs -> **195** if every TU kept only its largest
> cluster), **161 have a class that ALREADY lives in their TU** - i.e. they are pooled
> COMDAT residue, and the partition for them is *correct*. Only **20** had a class
> whose plurality home was a different TU (real rehomes; matcher-6 landed the provable
> ones), and 9 are classless.
>
> **So the data loop is NOT blocked on a wholesale re-partition.** What it needs is a
> pool-aware contribution model: measure the real COMDAT-pool bands empirically
> (replace the hardcoded `POOLS`), exclude pooled bodies from the contiguity
> invariant, and derive contributions from each TU's *ordinary* run. Homing a pooled
> body with its class is already the tree's convention and is correct
> (`~CMenuSparkle` 0x101b0 lives in MenuSparkle.cpp) - it is not a defect to "fix".
>
> Owner attribution is by **xref / vtable-data-ref / class identity**, never by RVA
> proximity - and that still works fine for pooled bodies, because the *class* is
> recoverable even when the *obj* is not.

## What a contribution range is, and why we need it

Per (compiland, storage) a **contiguous** `[rva, size)` interval. A linker lays each
object's **ordinary** contribution to a section down in one piece (COMDATs are the
documented exception - see the correction above), so real contributions never
interleave. homm2 reads them free from CodeView NB09 `sstModule`; **GRUNTZ.EXE has no
debug stream**, so ours must be derived from our own TU partition. They gate:

- `--contribution-manifest` → owner attribution for data we cannot otherwise place:
  the 290 COMDAT string payloads (296 retail copies) whose retail owner is currently
  unprovable, plus the `$T`/`$S` constant pools;
- `--data-section-manifest` → placing definitions at their candidate `section_offset`
  (candidate-shaped target sections), which is the rest of `matched_data`;
- absolute-RVA layout: `exe-diff` §B abs-rva ≈ 0, §E 0/771 at the correct
  section-relative offset.

## Measured state: our TU partition != retail's compilands

- `gruntz.analysis.tu_order_check`: **GATE FAIL — 49 TUs with intra-order violations,
  10832 interleaving TU-pairs** (was 52 / 12134 when this brief was written; matcher-6
  landed the first drain — and see the correction at the top: ~98% of the remainder is
  COMDAT-pool placement, not a partition defect) (e.g. `Fader [0x17e450-0x182935]` interleaves
  `RezBufferObjectDtor`/`RecordFill`/`CircleShadeBlit`; `MenuItem [0x184610-0x185a0e]`
  interleaves `RezColl`/`DebugPrintf`/`RezList`/`WapUncompress`).
- Independent check (hull of each unit's labelled symbols per storage, **all-pairs**
  overlap): `.text` **134/259** unit hulls interleave. Data is no better once measured
  correctly: only **8 of 86** bands overlap no other band — rdata **2/15**, data
  **5/17**, bss **1/54**.
- Consequence, measured: **0 of 296** ambiguous COMDAT string copies can be attributed
  by clean-band containment. The data contribution manifest is NOT independently
  recoverable; it waits on this lane.

CAUTION for whoever measures this next: check overlap **all-pairs**, not just adjacent
bands in a sorted list. Adjacent-only is blind to one band swallowing many others and
reports ~70-80% clean where the truth is ~9%. That error is why this brief exists.

## Worst .text offenders by span (a single compiland cannot do this)

| unit | fns | .text hull span |
|---|---:|---:|
| motionstate | 6 | 0x15bd60 (1.4 MB) |
| movinglogic | 6 | 0x15bb60 |
| gametext | 16 | 0x155280 |
| butemgr | 54 | 0x1539a0 |
| gruntzrandom | 5 | 0x14fee0 |
| movieplayer | 3 | 0x143730 |
| ddrawsurfacepair | 44 | 0x12bee0 |
| gruntvoice | 19 | 0x107530 |

Aggregates (`globals`, `vtables`) span by construction and are not compilands — exclude
them from band analysis, but they must drain eventually too. (`src/Stub/All.cpp` /
`engine_label_stubs` is **gone** — that backlog is fully drained; only prose references
survive in `config/units.toml` comments and in `CLAUDE.md`, which is stale on this
point. Excluding it changes nothing: 8/86 clean either way.)

## Instruments already in place

- `gruntz link` → candidate EXE + `.map` (392 objs, 4886 unresolved externs under
  `/FORCE`); `gruntz link --analyze` / `gruntz.analysis.link_order` for build order.
- `gruntz.analysis.tu_order_check` — the one-contiguous-block invariant gate.
- `gruntz exe-diff` §B — `.text` intra-TU order / block-exact / abs-RVA + a ranked
  reorder worklist; §E — data static-storage: 771/925 data symbols defined+placed,
  715/771 in the right storage class, 0/771 at the right section-relative offset, with
  the first section-relative divergence per section.

## Method (homm2's, and it is the right one here)

Fix the **earliest** divergence in a section, relink, re-read: later rows are cumulative
consequences, not independent defects. Split holding TUs by RVA gap and chase the real
owner by xref (see the holding-TU-drain notes).

**Revised metric (see the correction at the top).** Driving `tu_order_check`'s raw
interleave count to 0 is NOT achievable and not the goal: ~98% of it is COMDAT-pool
placement, which no source arrangement can express. The tractable targets are:

1. **Measure the real COMDAT-pool bands** and replace `tu_layout.POOLS`' hardcoded
   guess; then make `--exclude-pools` the gate. That is the honest invariant.
2. **Drain the holding TUs** - files that group unrelated classes by method name or by
   "orphan" rather than by compiland. matcher-6 dissolved seven
   (FlashRect, MenuStateAssets, GruntSpawnLevel, LogicActReg646010, OrphanLeaves,
   OrphanMethods, plus MoviePlayer's slice) and every move was byte-neutral or better
   (`CBootyState::FormatHudText` 88.64 -> 97.50 just from being homed to its real TU).
3. **The ~20 real rehomes** the class-identity check finds (outliers whose class's
   plurality home is another TU) - not the 161 pooled-COMDAT outliers.

Then `exe-diff` §B/§E start moving and the data lane can generate contributions from
each TU's *ordinary* run.

## Known adjacent defect, worth a dedicated lane (matcher-6, proven)

`CUserLogic`'s **vtable slot 4 is modelled with the wrong arity tree-wide**:
`include/Gruntz/UserLogic.h:360` declares `virtual i32 UserLogicVfunc2()` (no args),
but retail's own base body (ILT thunk 0x246e -> **0x8b70**) and every override
(e.g. 0xade60) end in **`ret 4`** - the slot takes one 4-byte arg.

Consequence: ~40 leaf classes cannot declare their REAL slot-4 body as the override, so
each parks it as a plain non-virtual `RunAct`/`FireActivation` *beside a declared-only
placeholder virtual* - the exact "body-less placeholder virtual" anti-pattern the
matcher brief bans, replicated ~40 times from one wrong signature. Many lanes hit it and
documented it in place (`Grunt.h:881`, `Projectile.h:77`, `PathHazard.cpp:232`,
`ActionArea.h:25`, `SpotLight.h`, `Teleporter.h`, ...). Fixing the base arity would let
every one become a real `OVERRIDE` and delete its placeholder. The sites are almost all
declared-only, so the blast radius is small; it needs one lane that owns UserLogic.h.
