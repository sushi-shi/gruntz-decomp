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
- The same effect explains `SerialObjectFactory`'s arms: `new CProjectile`
  inlines the ctor but keeps its base `??0CMovingLogic` call (depth 1), while
  the `new CGrunt` arm inlines two levels down. Our cl flattened deeper at
  those sites, so some retail-emitted base-ctor COMDATs
  (`??0CMovingLogic` 0x13940, `??0CSBI_RectOnly` 0x101fa0) have no emitting
  base obj and their RVA_COMPGEN pins dangle (labels lost, documented at the
  pin sites).

Related: docs/tu-partition-brief.md (realization groups), the retired
OOL_CTOR lever (banned by the no-guard ruling; it was a workaround for
exactly this).
