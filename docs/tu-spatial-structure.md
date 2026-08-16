# TU spatial structure: proximity as a relatedness & attribution signal

**Question.** How far apart in retail RVA space are the functions of one class/TU,
and does that layout let us say which methods are *related* — even attribute
*unnamed* functions to a class? Measured over the matched `src/` (4.2k+ RVA-annotated
functions) joined with the Ghidra boundary export. Companion to
`docs/link-order-investigation.md` (intra-TU source order + cross-TU link order);
this doc covers what sits *inside* one TU's block and how to exploit it.

Tooling: the TU-layout probe that produced this analysis is retired (see
[tooling-map](tooling-map.md)), as are the stub *generators* that once turned
attributions into backlog TUs — that backlog is fully drained and `src/Stub/`
is gone.

## The core finding

Distance from each matched function to its nearest same-TU sibling, by kind:

| kind | n | median | p90 | within 8 KB |
|---|---|---|---|---|
| **method** (ordinary *and* virtual overrides) | 3381 | `0x50` | `0x1c0` | **99%** |
| constructor | 182 | `0x50` | `0x540` | 95% |
| destructor | 138 | `0x30` | `0x950` | **91%** |
| other | 300 | `0xc0` | `0x1b0` | 98% |

**The destructor exile has largely CLOSED in this metric.** It was the headline
finding — dtor p90 `0x9dac0` (~645 KB), only 57% within 8 KB — and it now reads p90
`0x950`, 91% within 8 KB. The linker still pools deleting-destructors (section 3 is
unchanged: 24 distinct classes in the `0x80000-0x90000` run), but only **23 of 141**
dtors now sit in a pool region, and TU re-homing put the rest beside their siblings,
so *same-TU* distance no longer shows the exile. Read section 3, not this table, for
the pooling itself.

The structuring axis is **not** virtual-vs-simple. Virtual overrides (`Tick`,
`GetTypeTag`, slot methods) cluster right next to the plain methods. The mechanism
behind the pools is unchanged: MSVC emits the vtable-referenced *deleting* destructor
as a COMDAT, and the linker pools those into shared low-address runs —
`0x10000-0x14000` (8 distinct ctor/dtor classes) and `0x80000-0x90000` (24) —
alternating `scalar_deleting_destructor` thunks and `~Class` bodies from *unrelated*
classes. Note both runs are now mostly ordinary **methods** (262 of the 305 pooled
functions); only 23 dtors remain inside a pool region.

**Mechanism.** Out-of-line methods written in one `.cpp` land as a contiguous
source-order block (the linker keeps an object's COMDATs together — see the
link-order doc). Special members and other implicitly/inline-emitted functions are
COMDATs the linker pools elsewhere. So a class's *own block* is tight; a special
member may be exiled — though far fewer are than when this was written.

## Relatedness model

- **Proximity ⇒ same class/TU** holds cleanly for ordinary + virtual methods: a
  matched method's tight RVA neighbours are its siblings (the "batch siblings"
  matching lever). `tu_layout --neighbors 0xRVA` lists them.
- **Pooled special members are the exception** — for a dtor that *is* in a pool
  region (23 of 141 today), proximity can't tie it to a class; recover those by leaked
  name / vtable / RTTI (which `gruntz walls inventory` already does). The
  majority of dtors are no longer pool-exiled and behave like ordinary methods.

## Exceptions & intermingling

Figures below are from `the retired TU-layout probe — **re-run it rather than
trusting these**; re-homing waves move them (an earlier revision of this section
survived long enough to name three examples that had all since dissolved).

1. **Special-member pools** (above) — still the dominant exception, and the only one
   of these three that has not moved. Two runs, ~150 functions each; the
   `0x80000-0x90000` run alone pools dtors from 24 distinct classes.
2. **Scattered classes** — 17 of 226 multi-method classes have <50% of methods in
   their longest contiguous run. Note the *rate* has collapsed as re-homing landed:
   this was ~20 of ~72 (≈28%) when first measured, now ≈7.5%. The surviving flavours
   are *conflated TUs* and *COMDAT-heavy families* — see the tool's own SCATTERED
   list for the current membership (`CBattlezMapConfig`, `CButeMgr`/`CButeValue`,
   `CGrunt`, `CPlay`, `CState`, the Multi/Single-Command pair, …).
   **Worked example — `CMenuState`, a state-class soup that is real today**: 16
   functions whose dtor is pooled at `0x8ce60`, whose main block sits at
   `0x9fxxx-0xa0xxx`, and whose `Vslot09`/`Vslot06` bodies are compiled into
   **`Attract.cpp`** while `InputVirtual` sits in `StateImages.cpp` — three TUs for
   one class. `CGameLevel` is the other live case: 83% dense, but spread over
   `GameLevel.cpp` (67), `GameLevelMove.cpp` (10) and `Play.cpp` (2).
3. **Lone far methods** — ~102 plain methods >0x4000 from any same-class sibling,
   mostly the only matched method of a class so far (resolves as coverage grows).

### The 3%: intermingled methods (`--intermingled`)

The leave-one-out rule misses 55 of 2183 (3%). **Every miss is a method sitting
inside another class's contiguous run** — which makes the miss list a matching
worklist, not noise. `the retired TU-layout probe --intermingled` regenerates it,
grouped by TU and tagged.

Read a row as one of three things:

| cause | what it means | tell |
|---|---|---|
| **Mis-modelled owner** | the method really belongs to the surrounding class; our class label is wrong | often DIFF-TU; confirm by vtable slot / caller `this`, never by proximity alone |
| **Not inlined** | an inline accessor of a small helper class that `cl` emitted out-of-line into whichever TU used it | a lone accessor-shaped name (`GetName`, `Reset`, `Hash`) inside a big foreign run |
| **Written in the same place** | two classes genuinely co-authored in one `.cpp` | a CLUSTER of ≥3 alternating rows, usually same-family; **not a defect** |

**SAME-TU vs DIFF-TU is the first discriminator.** 45 are SAME-TU — we already put the
body in the host's `.cpp`, so retail's placement and ours agree and only the *class
label* is in question. The 10 DIFF-TU rows are the stronger signal: our TU partition
disagrees with where retail put the code.

The clusters are the "written in the same place" case and need no fixing:
`ButeMgr.cpp` (8 rows, `CButeMgr` ↔ `CButeValue` alternating), `GruntzCmdMgr.cpp`
(7, `CGruntzMultiCommand` ↔ `CGruntzSingleCommand` in matched `Parse`/`Pack`/`Select`/
`FreeAll` pairs), `Attract.cpp` (4, the state-class soup), `Multi.cpp` (4).

`Multi.cpp` is the clearest **not-inlined** cluster: `CNetPlayerListNode::GroupName`,
`CNetSessionNode::GetName`, `CNetSession::ResetAll` and `CNetCmdSlot::BuildHostName`
each sit alone inside a long `CMulti` run — four small node-class accessors emitted
out-of-line at their use site. Same shape as `docs/exe-map/interleaved-comdats.md`,
which has the reproduction and the fix recipe.

**Do classes intermingle?** Mostly no, and the answer has strengthened. At the
method-block level (pools excluded): **2467** same-class adjacent pairs vs **523**
boundaries, and only **55** true A-B-A splices. And the splices are almost all
between **sibling classes** of one family (`CGruntzMultiCommand`/`CGruntzSingleCommand`
and `CButeMgr`/`CButeValue` lead at 7 each) — so a mis-attribution lands on a
same-family neighbour, not a random class.

## Attribution: tie unnamed functions to a class

`tu_layout --attribute` brackets each classless function: if it sits between two
matched functions of the **same class C**, within `--gap` (default `0x4000`), it is
C. The reported unnamed total subtracts complete `RVA(...)` and
`RVA_COMPGEN(...)` extents from both `src/` and `include/`. This is essential:
inline bodies may be annotated in a shared header, and Ghidra can place a false
`FUN_` start inside a larger reconstructed function. The Ghidra export is a
candidate-boundary inventory, not an authoritative current backlog. A body is
eligible only when its start lies outside every current source claim. Confidence:

- **HIGH** — C sits in a contiguous run of ≥3 of its own methods, and neither
  bracket end is in a pool.
- **MED** — short/mixed run or pool-adjacent (right family, not always exact).

Two target sets:

- **Unnamed `FUN_` bodies** (Ghidra boundary export) → attributed candidates, emitted
  by `--emit` with `kind=new-stub`.
- **Class-boundary functions** — where the two matched neighbours are *different*
  classes, the both-sides rule abstains (picking the nearer class is only ~58%
  exact). These are not dropped: `boundary_targets()` records **both** adjacent
  classes as a verify-then-pick worklist, deliberately *not* committed to one class.
  Why so many: catch-all/free helpers cluster at TU-block edges, so most sit at a
  boundary rather than inside a block.

**Validation (leave-one-out on matched methods):** hide each known method and predict
from its neighbours — the HIGH (both-sides) rule is **97% exact, 98% same-family**
(2128/2183; it was ~91%/~94% when first measured).
That precision is measured against ground truth, independent of any runtime trace
(the old dynamic-trace labels were loose, especially single-observation, so they were
*not* used as an oracle here; that tooling is archived).

Coverage rises automatically as matching progresses: the tool currently reports 219
remaining unnamed `FUN_` bodies (non-thunk, source claims excluded), of which 93 (42%)
get a same-class bracket — 73 HIGH, 20 MED. Run `--attribute` for the live figures.

### Consuming the attributions

`--emit <csv>` writes `rva,class,confidence,kind,current,file` for the HIGH/MED
attributions. Historically these were auto-generated into a backlog TU; that stage is
retired (the backlog drained and `src/Stub/` was deleted). Today the CSV is a
**worklist**: a matcher takes an attributed RVA, reconstructs the body directly in the
attributed class's real TU, and moves it if the proximity class turns out slightly off.

## Limits

- HIGH is 97% precise, so ~1 in 33 lands on the wrong (usually same-family) class —
  acceptable for a backlog worklist a matcher re-checks, not for silent ground truth.
- Pool regions and scattered/COMDAT-heavy families (the State classes) are where
  proximity is weakest; gate on purity and cross-check vtable/RTTI there.
- On the already-CURATED catch-all backlog, proximity is redundant-or-wrong and must
  be cross-checked against existing evidence. E.g. it brackets the icon-loader
  `LoadToyBoxIcon` under `CTriggerMgr`, but its siblings were hand-graduated to
  `IconLoaders.cpp` — the curation wins. Treat catch-all hits as a prompt to verify
  (a `@proximity:` notice on the stub), not to relocate.
- Coverage is bounded by current matching: sparse regions have no matched brackets
  yet.
