# MSVC5 varies ctor-inline depth per call site within one function

Evidence: retail `CStatusBarMgr::BuildGameMenu` @0x101580 constructs ten SBI
items. All ctors were header-inline (same visibility at every site), yet the
emitted base-chain call depth differs site by site, in order:

| sites | emitted shape | depth |
|---|---|---|
| 1-3 (RESUME/PAUSE/LOAD MenuItem) | call `??0CSBI_RectOnly` | 2 |
| 4-6 (SAVE/SETT/HELP MenuItem) | call `??0CStatusBarItem` | 3 |
| 7 (QUIT MenuItem), 8 (DEST ImageSet) | fully flattened, no call | 4+ |
| 9-10 (MISS ImageSet) | call `??0CSBI_RectOnly` | 2 |

Same compiler, same headers, same class - the inliner's per-site budget, not
source shape, decides where the chain becomes a call. Consequences:

- A reconstruction that matches the *construction set* can still diverge hard
  (here 59%): every call-vs-flatten difference also toggles the whole
  function's **EH frame** (a kept ctor call is throwing -> unwind states +
  fs:0 registration; a fully flattened chain drops the frame entirely).
- You cannot force the retail shape with guards/visibility tricks (banned, and
  wrong - retail had uniform visibility). This residue class is currently a
  WALL: neither hand shape-tweaks nor the permuter's statement reordering
  reliably moves cl5's per-site inline budget.
- The same effect explains `GameSerializationCallback`'s arms: `new CProjectile`
  inlines the ctor but keeps its base `??0CMovingLogic` call (depth 1), while
  the `new CGrunt` arm inlines two levels down. Our cl flattened deeper at
  those sites, so some retail-emitted base-ctor COMDATs
  (`??0CMovingLogic` 0x13940, `??0CSBI_RectOnly` 0x101fa0) have no emitting
  base obj and their RVA_COMPGEN pins dangle (labels lost, documented at the
  pin sites).

Related: docs/tu-partition-brief.md (realization groups) and
docs/patterns/inline-base-ctor-emission-wall.md. Per-TU visibility changes are
banned: retail had uniform visibility, so changing it models the desired output
rather than the source.

## Characterization (2026-08-02 probe)

Ruled out: body-size mismatch (the historical standalone
`??0CUserLogic@@QAE@XZ` body was byte-perfect at 100%), missing EH frame (our
`GameSerializationCallback` carries the same fs:0 registration as retail), and header
visibility (identical on both sides). What remains is the caller's accumulated
per-site inline budget: cl5 decides call-vs-flatten from the surrounding
intermediate code, so every OTHER divergence in the caller shifts the decision.

**Consequence: this wall is a convergence phenomenon, not a source-shape defect.**
Artificial emission devices were removed in August 2026. Finish the real callers -
GameSerializationCallback, BuildGameMenu, and CGruntzMgr::PlayMovieEntry - through the normal
matching campaign, then census which inline COMDATs the compiler emits naturally.

## Save correction: the heap "resolution" was refuted

The 2026-08-03 heap spelling was selected while the strstream/iostream destructor
identity and slot attribution were still wrong. Later retail attribution proves a stack
`strstream` local: its constructor receives the local frame address, scope exit tears down
the same object and its virtual `ios` base, and the surviving Bute `Save` independently
declares `strstream ss` on the stack. The heap spelling is therefore not a reverse-use
rule and is not retained.

With the stack model restored, current base and retail agree on the 0x1124 local extent,
38 calls, 20 branches, two returns, 41 ordered relocations, and all stream operations.
Retail pins constant one in EBX and reuses it for boolean stores, virtual-base flags,
stream-state tests, and the final return; base emits immediate ones and omits EBX. The
public function-scope 4096-byte buffer, chained flag stores, a named success result, and
all 44 syntax-aware declaration/expression variants leave the same 0x404-byte base island.
This is bounded C2 constant pinning, not a missing object-lifetime abstraction.

## The CStatusBarMgr half is BROKEN (2026-08-16)

"This residue class is currently a WALL" no longer holds for the four `CStatusBarMgr`
builders, and `??0CSBI_RectOnly` 0x101fa0 is no longer unemitted. The cut population is
driven by how many INLINE EXPANSIONS the caller contains, and the reconstruction was
missing one member function: `AddTabItem(i32 tab, CStatusBarItem* item)` over 71
`m_tabLists[N].AddTail(item)` sites. See
repeated-container-call-is-an-inline-member.md and the BROKEN section of
ctor-inline-cut-depth-varies-per-new-site.md. The `GameSerializationCallback` /
`CGruntzMgr::PlayMovieEntry` rows are untouched by that measurement.
