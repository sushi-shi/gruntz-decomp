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

## Characterization (2026-08-02 probe)

Ruled out: body-size mismatch (the flattened `??0CUserLogic@@QAE@XZ` body force-
emitted by userlogicctoremit is byte-perfect at 100%), missing EH frame (our
`SerialObjectFactory` carries the same fs:0 registration as retail), and header
visibility (identical on both sides). What remains is the caller's accumulated
per-site inline budget: cl5 decides call-vs-flatten from the surrounding
intermediate code, so every OTHER divergence in the caller shifts the decision.

**Consequence: this wall is a convergence phenomenon, not a source-shape defect.**
The force-emit devices (UserLogicCtorEmit, UnknownFileIOCtor, GameWnd,
WwdFactoryObject) dissolve not by restructuring them but by finishing their
CALLERS - SerialObjectFactory (86%), BuildGameMenu (59%),
CGruntzMgr::ChangeState (70%) - through the normal matching/permute campaign.
Re-test each device (delete it, census the emission) whenever its caller's
fuzzy% materially improves; ForceEmitCStateDtor already fell this way once
??1CPlay/??_GCState landed in gruntzmgr.

## Sibling divergence: funclet helper vs inline dtor pair (CButeMgr::Save)

Retail registers Save's iostream&-bound strstream temp with a funclet that
tail-jmps the `??_Diostream` helper; our cl, given the byte-proven same source
shape (`iostream& source = (iostream&)strstream(...)` - the cast retypes the
temp's EH registration, era ref-to-temp extension; clang gets an #ifdef
spelling), emits the funclet as an inline `??1iostream`+`??1ios` call pair and
never materializes the ??_D COMDAT. Same family of budget-dependent choices.
The EmitIostreamVbaseDtor device carries the label until this converges.
Byte-win from the reshape: ??_Dstrstream/??1strstream correctly vanish from
the TU (retail has neither anywhere).

## Save residue: the cast-ref temp's SLOT lifetime (open)

With the corrected strstrea.h the symbols and teardown match, but the frames
differ by 0x58: retail keeps the strstream temp's slot alive to scope end
(ifstream@0x28 / temp@0x8c / ofstream@0xe4 / block@0x13c, all distinct),
while our cl treats the static_cast-bound temp as expression-lived for slot
allocation and overlays it with the ofstream (temp@0x84 vs ofstream@0x78).
Hoisting `block` to function scope changed nothing. The winning spelling must
additionally make cl5 hold the temp's slot; the uncast binding does that but
resurrects the synthesized ??1strstream/??_Dstrstream. Next levers: permute
(slot assignment follows declaration/temp order), or a spelling that binds an
lvalue path before the upcast without retyping the dtor.

## Save residue: RESOLVED by the heap spelling (2026-08-03)

`strstream* sourceStore = new strstream(...)` + `iostream* source = sourceStore`
satisfies all three byte constraints at once: the pointer upcast (neg/sbb/and at
Decode), no synthesized ??1strstream/??_Dstrstream (scope-end destruction of a
stack local emits both - proven by the LNK2005 vs libcimt's _strstre.obj), and
teardown virtual-dispatching into the CRT's own scalar-deleting dtor (the retail
strstream vtable slot-0 target, 0x169aa0 - the "missing" dtor was never missing,
only unnamed). The slot-lifetime 0x58 delta reads naturally now: a heap pointer
lives in one slot to scope end.
