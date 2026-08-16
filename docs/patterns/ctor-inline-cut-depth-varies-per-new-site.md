# cl5 cuts a ctor chain OUT-OF-LINE at a depth that varies PER `new`-SITE
tags: cpp:ctor cpp:new cpp:inline | asm:call asm:eh topic:wall
symptoms: base has NO `/GX` EH frame where retail does (`push -1 / push <handler> / mov fs:0,esp`), a smaller `sub esp,N`, and retail has a `call <ILT thunk>` at a `new T` site where the base stamps the base-class fields inline; retail also sets an EH state (`mov [esp+N],1/2/3`) around each such `new`
confidence: 9/10 (three distinct cut depths measured in two functions; every OOL body identified by address)
variants: ool-ctor-device (docs/patterns/ INDEX), phantom-method-reconstruction.md

`new CSBI_MenuItem` runs a four-deep ctor chain
`CSBI_MenuItem -> CSBI_Image -> CSBI_RectOnly -> CStatusBarItem`, all four defined in-class
in headers. cl5 inlines a PREFIX of that chain and emits a `call` for the rest, and **the
cut depth is not a property of the declaration** - the same ctor is called at one site and
inlined at another **inside a single function**:

| site | retail | cut after |
|---|---|---|
| `BuildStatusBarTabs` @0xffde0, the three `new CSBI_RectOnly` | 5 inline stores, no call | nothing (whole chain inlined) |
| `BuildStatusBarTabs`, all five `new CSBI_MenuItem` | `call 0x22c0` -> `??0CStatusBarItem@@QAE@XZ` @0x1005d0 | depth 3 |
| `BuildGameMenu` @0x101580, sites 1-3 | `call 0x1e88` -> **`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0** (27 B, unnamed in our tree) | depth 2 |
| `BuildGameMenu`, site 4 | `call 0x22c0` | depth 3 |
| `BuildTabzDialog` @0x10a340, `new CSBI_Image` | `call 0x22c0` | depth 2 |

The consequence is structural, not cosmetic: an out-of-line ctor **can throw**, so cl must
be able to `operator delete` the half-constructed object. That forces the whole function
into a `/GX` EH frame with a state variable (`mov [esp+N],1`, `2`, `3` at successive
`new`s, reset to `-1` once each ctor returns), and grows the frame. A fully-inlined chain
contains no call, cannot throw, and gets no frame at all. So one inliner decision moves the
prologue, the epilogue, the frame size and the register allocation of the entire function.

## What does NOT control it

- **Declaration form.** All four ctors are in-class inline on both sides; `??0CSBI_RectOnly`
  is inlined at some sites and called at others in the same object file.
- **`#pragma inline_depth(3)`** reproduces `BuildStatusBarTabs` (71.58 -> 78.21, EH frame and
  all five calls appear) but is a global setting and therefore cannot express a per-site cut;
  it drives `BuildGameMenu` the wrong way (72.33 -> 66.93). It is a fitted artifact - do not
  land it.
- **Per-site typed locals** instead of one reused `CStatusBarItem* it` - byte-identical
  (71.58 either way). Worth doing for cleanliness, not for the match.

## Consequences for the worklist

Any function whose `eh 0->1` differs in
a `gruntz walls diagnose --asm` prologue comparison, and which contains a `new` of
a deeply-derived class is in this family. Until the heuristic is understood these are
`@early-stop` walls, and the OOL bodies retail emitted (`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0
is still unnamed) need `RVA_COMPGEN` pins in whichever TU emits the COMDAT.

## RETRACTED 2026-08-14: a per-class majority does not prove sibling constructors

The `??0CLoadable` result (`ob1-inline-budget-divergence.md` § RESOLVED) says to run
`sema xref` on the callee one level UP and read each `new`'s `??_7` stamp before calling
a cut a budget wall. Doing that here changes the verdict for most of the sites.

`sema xref 0x001005d0` lists exactly four callers of `??0CStatusBarItem@@QAE@XZ`, all
`CStatusBarMgr` builders. Pairing every `push <size>; call ??2@YAPAXI@Z` in those four
with the ctor call that follows it and with the `??_7` the site stamps gives 38 sites:

| most-derived class (by `??_7`) | none | `call ??0CStatusBarItem` | `call ??0CSBI_RectOnly` |
|---|---|---|---|
| `CSBI_RectOnly` / `CSBI_GruntMachine` / `CSBI_StatzTabGruntBar` (depth 2) | **6** | 0 | 0 |
| `CSBI_Image` (depth 3) | 4 | **10** | 0 |
| `CSBI_MenuItem` (depth 4) | 6 | **11** | 5 |
| `CSBI_ImageSet` (depth 4) | 6 | **9** | 6 |
| `CSBI_ImageSetAni` / `CSBI_WarlordHead` / `CSBI_StatzTabArrow` / `CSBI_WellGoo` | 0 | 0 | **8** |

That table remains a useful census, but the conclusion formerly drawn from it was too
strong. A tagged family (`NO_SEED`, `INLINE_SELF`, `BASE_CALL`, `CALL_RECTONLY`) can fit
the observed call depths and can emit both exact COMDATs, but neither fact proves that
those overloads existed. The tags choose the desired output at each site and therefore
encode the answer when the real cl5 decision may instead come from front-end cost and
candidate order. They were removed from the retained source.

The controlled natural-chain rebuild is the negative control. With one ordinary default
constructor per class, `CStatusBarItem::CStatusBarItem` remains 100% exact, yet the four
builders start at 29/32/141/55 calls against retail's 33/39/163/61. Moving the same
definitions from late-inline form into the class was byte- and call-count-flat, so
declaration position is not the missing lever.

Disposable unused-local titration then changed only cl5's front-end cost; every probe was
removed after measurement:

| probe | Tabs calls/score | Game calls/score | Load calls/score | Dialog calls/score |
|---|---:|---:|---:|---:|
| natural chain | 29 / 85.55 | 32 / 81.02 | 141 / 83.77 | 55 / 79.66 |
| +4 in `CSBI_RectOnly` | 29 / 85.55 | 35 / 84.13 | 146 / 84.57 | 57 / 82.02 |
| +8 in `CSBI_RectOnly` | 30 / 91.76 | 37 / 85.94 | 152 / 88.33 | 60 / 84.39 |
| +12 in `CSBI_RectOnly` | 31 / 94.42 | 38 / 85.42 | 154 / 89.40 | 63 / 86.53 |
| +16 in `CSBI_RectOnly` | 32 / 97.07 | 38 / 85.43 | 155 / 89.26 | 64 / 87.81 |

At +8 the `CSBI_RectOnly` COMDAT materializes and its optimized body is 100% exact, but
rich disassembly shows `BuildGameMenu` still calls only
`CStatusBarItem::CStatusBarItem`; it never calls `CSBI_RectOnly::CSBI_RectOnly` as retail
does. `CSBI_Image` +4/+8 reproduces the RectOnly +4/+8 call-count rows and the same wrong
callee. `CSBI_MenuItem` +4 is flat; +8 adds only one base-ctor call in BuildGameMenu and
is flat in the other three builders. A single global constructor-cost knob therefore
cannot recover the mixed retail population, and exact optimized constructor bodies do
not bound the source-level inline cost.

Removing the apparently redundant inherited-pointer stores from the natural derived
constructors is also a negative control, not a cleanup. Dropping `m_frame = NULL` from
`CSBI_MenuItem`, `CSBI_ImageSet`, and `CSBI_WellGoo`, and dropping the inherited
`m_frame`/`m_frameSet` stores from `CSBI_ImageSetAni` and `CSBI_WarlordHead`, leaves the
same initialized values available from their base constructors but changes the caller
population: `LoadTabSprites` falls from 141 to 139 calls and `BuildTabzDialog` from 55
to 54 (retail 163/61). The repeated stores therefore carry real front-end-cost evidence;
do not delete them merely because optimized standalone code cannot attribute a surviving
store to one level of the chain.

**Revised method:** pair `operator new`, ctor-call referent, and `??_7` at every site, but
use the class table only to locate the missing inline population. Do not synthesize
tagged sibling constructors from a majority column. Titrate candidate edges only as
disposable A/B tests, require the actual ctor referent (not just call count or fuzzy) to
move correctly, and then recover the real caller/helper/constructor statements that
account for the measured cost. Here the remaining wall is a joint population problem:
the base must be cheap enough to fold inside RectOnly while selected outer chains must
reject RectOnly itself.
