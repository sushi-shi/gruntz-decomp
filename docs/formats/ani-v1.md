# ANI — animation-control records, and what their fields may hold

ANI carries no pixels. It is a short program that advances the frame of an
image set already attached to a game object, nudges its position, and fires
sound cues. The container grammar is settled and documented in
[`tools/gruntz-codec/src/ani.rs`](../../tools/gruntz-codec/src/ani.rs); what
that module does **not** say is what values its named record fields — `step_mode`,
`loop_mode`, `position_mode`, `draw_value` — are allowed to take. That is this
document.

Two sources, both checked here:

* **Retail** — `CAniElement::Build` @0x165460, `CAniRecordView::Parse`
  @0x168c60, `CAniRecordView::GetSize` @0x168e50,
  `CAniAdvanceCursor::Advance` @0x15c360. Assembly only.
* **The corpus** — every ANI resource in both archives: 660 in `Gruntz.REZ`
  + 378 in `GRUNTDEM.REZ` = **1038 files, 13 480 records**. The record walk
  lands exactly on end-of-resource in all 1038, with zero trailing bytes,
  which is what validates the grammar the counts below are drawn from.

Tiers as in [`wwd-v1.md`](wwd-v1.md): **P** proven from an instruction,
**U** proven unread, **I** inferred, **?** undetermined.

## Layout recap

```
+0x00  [u8;8]    proven never read; `20 00 00 00 00 00 00 00` in all 1038
+0x08  i32       animation flags   -> OR-ed into CAniElement::m_flags
+0x0c  i32       record count
+0x10  u32       name byte length
+0x14  [u8;12]   proven never read; zero in all 1038
+0x20  [u8;n]    animation name, not NUL-terminated
then `count` records of 10 x i16, each optionally followed by a
NUL-terminated cue string when record flags bit 1 is set
```

`include/Gruntz/AniElement.h`'s `CAniSource` already models exactly this
(`m_pad00[8]` / `m_flags` / `m_count` / `m_namelen` / `m_pad14[0xc]` / `m_data`).

**Header `flags` at +0x08 is 0 in all 1038 files.** It is read
(`m_flags = src->m_flags | flags` in `Build`), so it is not *unread* — but no
shipped animation contributes a bit through it. Every `CAniElement::m_flags`
bit in play comes from `Build`'s caller.

Two more corpus facts worth having: **990 of 1038 files have `name_len == 0`**
(the on-disk animation name is usually absent — the registry key is the name),
and record counts run 1..89.

## Record fields

`CAniRecordView::Parse` @0x168c60 reads all ten `i16`s in order into members,
so the field order is proven. Values below are the **complete** set observed
across 13 480 records — an absent value is absent from shipped data, not proven
impossible.

| # | Field | Observed values (count) | |
|---|---|---|---|
| 0 | `flags` | `0` x12 695, `2` x785 | **P** |
| 1 | `step_mode` | `3` x13 434, `1` x20, `0` x6 | **P** |
| 2 | `loop_mode` | `0` x12 719, `9` x749, `8` x8, `3` x4 | **P** |
| 3 | `position_mode` | `0` x13 478, `1` x2 | **P** |
| 4 | `param` | 0..92, 93 distinct (0 x43) | **P** |
| 5 | `duration` | 0, 10, 20, 25, 30, 33, 35, 40, 50, 55…180, 200…800, 1000, 2500, 3000, 3500, 5000 — 42 distinct | **P** |
| 6 | `draw_value` | `0` x12 955, `1` x280, `2` x222, `99` x18, `100` x3, `3` x2 | **P** |
| 7 | `delta_x` | **`0` in all 13 480** | **P** (read, never non-zero) |
| 8 | `delta_y` | **`0` in all 13 480** | **P** (read, never non-zero) |
| 9 | *(reserved)* | 772 distinct, −32 412..32 676 | **U** |

### Field 9 is uninitialised authoring memory

It is parsed (`m_reserved28` in `CAniRecordView`) and never read again. Across
the corpus it takes 772 distinct values spanning the full signed range — and
**every observed value is even**. That is the profile of a word-aligned stale
pointer left in the editor's record buffer, not of a field. Do not assign it a
meaning; a re-encoder must preserve it verbatim, which `ani.rs` does.

## `flags` — the record flag word

| Bit | `ani.rs` name | Occurs? | | Evidence |
|---|---|---|---|---|
| 0x01 | `FLAG_TICK_DURATION` | **never** | **P** | `CAniRecordView::GetSize` @0x168e50: set means `duration` counts engine updates and is scaled by 22 ms; clear means `duration` is already ms |
| 0x02 | `FLAG_HAS_CUES` | 785 records | **P** | `Parse` @0x168cd3 `test 0x2` — a NUL-terminated cue string follows the fixed 20 bytes |
| 0x04 | `FLAG_POSITIONAL_CUE` | **never** | **?** | named in `ani.rs`; no consumer identified in this pass |
| 0x08 | `FLAG_FORCE_CUE` | **never** | **P** | `CAniAdvanceCursor::Advance`: with it (or object flag 0x2000000) set, the cue fires even when the object's dirty-rect is unarmed |

So **every shipped duration is already in milliseconds** and the tick-scaling
path is unexercised. `duration <= 0` falls back to 22 ms (`GetSize` returns
0x16), which 15 records rely on.

Cue strings are whitespace-separated token lists resolved against the sound
registry by `CAniRecordView::ResolveIndices` @0x168d00: 785 records carry 986
tokens — 626 with one, 124 with two, 32 with three, 2 with five, 1 with six.

## `step_mode` — how the frame index moves

Modelled already, as `WwdAnimStepMode` in `include/Wwd/WwdAnimStepMode.h`.
Dispatched by the `switch` in `CAniAdvanceCursor::Advance` @0x15c360.

| Value | Enumerator | Effect | Corpus |
|---|---|---|---|
| 0 | **missing** | falls to the switch `default` — frame unchanged | 6 |
| 1 | `WWDSTEP_NEXT` | +1, wrapping to `m_minIndex` | 20 |
| 2 | `WWDSTEP_PREV` | −1, wrapping to `m_maxIndex` | — |
| 3 | `WWDSTEP_SET` | jump to the frame in `param` | **13 434** |
| 4 | `WWDSTEP_FIRST` | jump to `m_minIndex` | — |
| 5 | `WWDSTEP_LAST` | jump to `m_maxIndex` | — |
| 6 | `WWDSTEP_FORWARD_BY` | += `param` | — |
| 7 | `WWDSTEP_BACK_BY` | −= `param` | — |

Value **0 occurs 6 times and has no enumerator** — e.g. `GAME\ANIZ\NOTHING`
record 0, `GRUNTZ\ANIZ\ENTRANCEZ\ONE` record 0. It is the `default` arm: a
record that holds the current frame for `duration` ms. Naming it would be a
`src/` change; see the worklist below.

This also explains `param`'s 0..92 range: with `step_mode == 3` on 99.7% of
records, `param` is overwhelmingly **an absolute frame index**.

## `loop_mode` — when the cursor moves to the next record

`WwdAnimLoopMode`, same header.

| Value | Enumerator | Corpus | Note |
|---|---|---|---|
| 0 | `WWDLOOP_NEXT` | 12 719 | |
| 1 | `WWDLOOP_AT_PARAM` | — | |
| 2 | `WWDLOOP_AT_FIRST` | — | |
| 3 | `WWDLOOP_AT_LAST` | 4 | all four are sprint/forward cycles (`GAME_ANIZ_GRUNTBOMBSPRINT`, `STATEZ_BOOTY_ANIZ_FORWARD100`) |
| 4 | `WWDLOOP_AFTER_FIRST` | — | |
| 5 | `WWDLOOP_BEFORE_LAST` | — | |
| 6 | *(no enumerator)* | — | undeclared and unobserved; leave the gap |
| 7 | `WWDLOOP_RESTART_AT_SECOND` | — | |
| 8 | `WWDLOOP_RESET_ANIMATION` | 8 | all eight are the four Warlordz' `BOOTY` animations, in both archives |
| 9 | `WWDLOOP_FINISH` | 749 | the terminator; `Advance` stops advancing |
| 0xffff | `WWDLOOP_INVALID` | — | runtime sentinel, set by the ctor and dtor; never on disk |

## `position_mode` — how `delta_x`/`delta_y` are applied

`WwdAnimPositionMode`. Dispatched by the second `switch` in `Advance`
(`case 1/2/3`, `default: break`).

| Value | Enumerator | Corpus |
|---|---|---|
| 0 | **missing** | **13 478** — the switch `default`: no position change |
| 1 | `WWDPOS_PLOT_OFFSET` | 2 — `STATEZ\MENU\ANIZ\CURSOR` record 3, in both archives |
| 2 | `WWDPOS_MOVE_RELATIVE` | — |
| 3 | `WWDPOS_MOVE_ABSOLUTE` | — |

Value 0 is 99.99% of the corpus and **has no enumerator**. And note the pincer
with `delta_x`/`delta_y` being 0 everywhere: even the two `WWDPOS_PLOT_OFFSET`
records apply a zero offset. **No shipped animation moves its object through
the ANI position mechanism at all.**

## `draw_value` — the per-record event code

`Advance` **returns** this value (via `m_curDraw`/`m_pendingDraw`), so it is a
per-frame event code the caller interprets. There is no single enum for the
domain; two partial ones exist and they disagree about scope.

| Value | Corpus | Meaning | | Evidence |
|---|---|---|---|---|
| 0 | 12 955 | no event. `Advance` zeroes `m_curDraw` after a consuming read | **P** | `Advance` @0x15c360 |
| 1 | 280 | **animation complete**. 222 of the 280 sit on the *last* record of their animation | **P** | tested `== 1` by `CGruntDecay`, `CRockBreakEffect`, `CWarlord`, and the Grunt entrance/arrival states |
| 2 | 222 | **effect / impact frame** — `ANI_EVENT_FRAME` in `include/Gruntz/AniAdvanceCursor.h`, `TILE_ARRIVAL_FX_IMPACT` in `include/Gruntz/TileArrivalFxCue.h` | **P** | four consumers: `CGrunt::StepAttackFire` @0x61cb0 (the attack lands), `CStaticHazard` @0xfc1a0, `CDroppedObjectShadow::Advance` @0xc7ab0, and `CTriggerMgr::LoadTileArrivalFx` @0x75e90 (spawns the dirt particle) |
| 3 | 2 | — | **?** | both are `GRUNTZ\ANIZ\GAUNTLETZGRUNT\ITEM` record 9, immediately after that animation's `99`. No consumer located |
| 99 | 18 | **tool effect applies now** — `TILE_ARRIVAL_FX_APPLY = 0x63` | **P** | `CTriggerMgr::LoadTileArrivalFx` @0x75e90: every arm returns early unless the cue is this, then it uncovers the powerup / breaks the brick. Every occurrence is at or near the end of a Grunt tool-use animation: `BOMBGRUNT_ITEM2`, `BRICKGRUNT_ITEM`, `GAUNTLETZGRUNT_ITEM`, `GOOBERGRUNT_ITEM`, `SHOVELGRUNT_ITEM`, `SPYGRUNT_ITEM` |
| 100 | 3 | — | **?** | only `GRUNTZ\ANIZ\WELDERGRUNT\PROJECTILE{2,3,4}`, retail archive only. No consumer located |

`-1` also flows out of `Advance` (`TILE_ARRIVAL_FX_END`) but it is the
"no animation bound" return, never an on-disk value.

The two existing enums each cover only their own consumer's slice of the
domain, which is legitimate — `Advance` is a generic integer-valued interface —
but it means neither is the domain. A `WwdAniDrawValue` covering 0/1/2/99 with
3 and 100 left as documented gaps would be the honest single home; that is a
`src/` change and is listed below rather than made here.

## Undetermined

* `draw_value` 3 and 100. Present on disk, no consumer found in `src/`. Our
  reconstruction is incomplete, so "no consumer" here means "not located",
  not "does not exist".
* `FLAG_POSITIONAL_CUE` (0x04). Named in `ani.rs`, never set in shipped data,
  no consumer identified in this pass.
* Whether header `+0x00` (`0x20`) was meant as a header size. Unchanged from
  `ani.rs`'s reading: the sibling PID/RID header is also 0x20 bytes and carries
  **10** in the same slot across 29 798 sprites, so both cannot be sizes, and
  the game reads neither. The size hypothesis stays a hypothesis.
* Record field 9. Even-valued garbage; see above.

## `src/` follow-ups (not applied in this lane)

* `WwdAnimStepMode` has no enumerator for **0**, the switch-default "hold the
  current frame" arm, which 6 shipped records use.
* `WwdAnimPositionMode` has no enumerator for **0**, which **13 478 of 13 480**
  records use.
* `draw_value` has no domain type; the knowledge is split across
  `AniAdvanceCursor.h` (`ANI_EVENT_FRAME`) and `TileArrivalFxCue.h`
  (`TILE_ARRIVAL_FX_IMPACT`/`_APPLY`), which name the same 2 twice.
* `CAniRecordView::m_positionDeltaX`/`Y` and `m_reserved28` are dead in all
  shipped data; the comment on `m_reserved28` ("never read") is correct and
  could cite the corpus.
