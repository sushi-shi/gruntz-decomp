# Wall breaks

This is the evidence ledger for walls broken under the project `wall-identifier`
skill (`gruntz sema diagnose <rva>`). Entries follow the classifier order:
inliner, control flow, then register allocation. A score change alone is not a
break; the retail instruction, relocation, or table evidence must identify the
compiler decision, and a real VC5 build must confirm the fix.

## 2026-08-12 — `CGameLevel::BroadPhase`

- Unit/RVA: `gamelevelmove`, `0x00167ea0`; historical MAX remains 76.1119%.
- Classification: scalar-replacement/register-allocation wall. Candidate and
  retail have the same ordered 20 conditional branches, three returns, and
  symbolic branch targets. Retail reserves a 0x1c-byte frame and spills the
  four current-object bounds contiguously; the candidate reserves 0x10 and
  keeps more bounds in registers.
- Refuted structure: contiguous retail spills do not prove a source `RECT`.
  Three `RECT`s produce a 0x30 frame and 75.4701%; one `RECT` plus scalar other
  and candidate bounds produces 0x20 and 76.0970%; removing the candidate
  scalar entities produces 0x14 and 70.7612%. None reproduces retail's frame or
  scheduling, so the simple scalar collision bounds remain.
- Verdict: the control-flow and collision semantics are complete. No aggregate
  invented from spill adjacency and no frame-forcing local is retained.

## 2026-08-12 — `CBattlezMapConfig::ClaimTilesAround`

- Unit/RVA: `battlezmapconfig`, `0x0002d800`; historical MAX remains 74.0925%.
- Classification: loop-layout/register wall. Both sides emit 69 conditional
  branches and one return. The only symbolic branch difference is the final
  `g_stepRun` back-edge: candidate branches backward on `jne`; retail exits on
  `je` and follows it with an unconditional backward jump.
- Control: spelling the source as an infinite loop with an explicit
  `g_stepRun == 0` break compiles byte-identically, preserving the candidate's
  0x68-byte frame, 88 blocks, and opposite ownership. Retail uses a 0x60-byte
  frame and 90 blocks. The source spelling is therefore not the lever.
- Verdict: the recursive neighbor order, branch count, and behavior agree; the
  remaining back-edge ownership follows the broader frame/register allocation
  difference. No explicit label or artificial loop carrier is retained.

## 2026-08-12 — `CPlay::ExecCommand` tail merge follow-up

- Retail has four physical `ClearCell` calls: MOVE, each of the two on-grunt
  arms, and one tail shared by both point-target arms. The shared tail receives
  mode 2 or 3 from its predecessor. The candidate has five calls because VC5
  does not merge the two textually duplicated point-target tails in the current
  register state; this accounts for a substantial part of the existing
  103-versus-91 branch and 155-versus-142 block gap.
- Control: expressing that merge explicitly with a shared label removes one
  `ClearCell`, five branches, and five blocks, but lengthens every epilogue by
  introducing an eight-byte frame and moves fuzzy 74.0949% to 72.1715%. A
  reduced-lifetime variant still creates the frame and scores 71.2255%.
- Verdict: the retail tail merge is proven, but a source `goto` is not. The
  duplicated semantic arms remain until their natural entity/register state
  lets VC5 perform the same merge; no explicit cross-case jump is retained.

## 2026-08-12 — `CTileTriggerContainer::AddLogic`

- Unit/RVA: `tiletriggercontainer`, `0x00116610`.
- Before: 73.6779%. The reconstructed container method hand-expanded object
  initialization as one block, obscuring the original inline ownership.
- Inliner break: retail checks `m_initGate`, copies the 24 dwords occupied by
  six `RECT`s, then checks `m_initGate` again before the scalar stores. The
  exact `CTileTriggerSwitchLogic` sibling independently proves this two-layer
  design: its outer `BuildSmall` performs the first gate and copy, then calls
  `Setup`, whose first instruction sequence performs the second gate. Restoring
  corresponding `CTileTriggerLogic::Build` and `Setup` methods makes the full
  rectangle-copy and gate region align.
- Result: 83.8558% current-source fuzzy. The residual begins in allocation
  temporary placement and register ownership (`logicType` in candidate `ebp`
  versus retail `edi`), then changes the shared-return layout. Directly using a
  conditional list receiver was byte-neutral, so list selection is not the
  remaining cause. No compiler-state probe was introduced.

## 2026-08-12 — `CTriggerMgr::PlaceObject`

- Unit/RVA: `triggermgrgrid`, `0x0006b6d0`.
- Before: 73.3010%. The candidate used an exit-tested occupied-cell scan, kept
  a redundant `m_world` snapshot, and copied `typeKind` into a `PickupType`
  before the AI-unit switch. The switch then changed `typeKind`, but `Place`
  received the stale copy. That was a behavioral defect hidden inside a broad
  code-generation mismatch.
- Source levers: use the top-tested retail scan, keep one `g_gameReg` lifetime
  across the placement body, pass the post-switch `typeKind`, and express the
  row-capacity success arm as the guarded body. These restore retail's 12-byte
  frame, 31-branch/four-return topology, and the intended placed-object type.
- Result: 92.1651% current-source fuzzy. The remaining broad residue is
  scheduling and entity allocation around sprite creation and the AI switch;
  the structural fixes are retained regardless of that current score.
- Refuted wall hypothesis: explicit labels for the special-tile false arm were
  byte-for-byte flat at 92.1651% and still did not reproduce retail's shared
  out-of-line zero assignment. They were removed instead of becoming a manual
  source probe. A direct `switch (aiType)` was also byte-identical to a typed
  temporary, so the unnecessary cast/local was removed.

## 2026-08-12 — `CPlay::ExecCommand`

- Unit/RVA: `playercommandstep`, `0x000d1b60`.
- Before: 69.7691% current-source MAX, 942 candidate instructions against 885
  retail instructions. The candidate had one extra frame dword and all 18
  returns therefore carried an extra `pop ecx`.
- Register/entity break: retail's two `CellHitTest` sites take the addresses of
  two dead incoming command-argument homes and write 32-bit coordinates through
  them. Two case-local coordinate pairs created independent entities; one
  function-scope pair lets cl coalesce the shared lifetimes into parameter
  homes, removing the frame and all 18 surplus pops.
- Refuted inference: those homes do not make the parameters `i32`. All arguments
  have four-byte ABI slots, and an unrelated 32-bit local may reuse a dead
  narrow parameter's home. Widening the signature destroyed two byte-exact
  callers (to 61.55% and 72.67%) and was rejected. The caller-proven narrow
  signature plus shared `i32` scratch pair is the evidence-backed model.
- Control-flow break: after `PlaceObject`, retail falls through on `-1` into the
  bad-selection cue and branches on success to the reset path. Reversing the
  reconstructed success-first `if` to the retail failure-first arm made the
  entire placement arm align and raised the score by about 2.5 points.
- Referent break: retail's lookup operand reaches the pooled literal
  `"GAME_BADSELECT"`; the reconstructed TU-static array was a distinct datum.
  Passing the literal directly removes that relocation-masked wrong referent.
- Result: the evidence-backed form is about 74.1% in an ordinary build, with no
  synthetic frame. The remaining residue is not declared bounded: candidate
  still emits one extra physical `ClearCell` tail, three excess `g_curPlayer`
  relocations, and broad cross-jump/register differences across the four
  trigger arms. These are the next worklist, not a reason to restore the
  independent scratch pairs or the wrong static string.

## 2026-08-11 — `CTileTriggerLogic::LoadBridgeMove`

- Unit/RVA: `tileswitchlogic`, `0x00110860`.
- Before: 99.9887% historical MAX; executable instructions, 28 conditional
  branches, five returns, and the relocation multiset agreed.
- Classification: control flow, specifically dense-switch IR arm identity. The
  entire objdiff residue was three byte-index entries: candidate
  `00 07 07 07`, retail `00 00 00 00` for cases 15–18.
- Source lever: replace the empty-case `break;` with an explicit `goto done;`
  to a shared final `return;`. This keeps all four labels on retail slot 0
  instead of folding the last three into default slot 7.
- Verdict: 100.00% exact; current exact-function total increased 3,543 → 3,544.

## 2026-08-11 — `CGameLevel::ProbeFootSoft`

- Unit/RVA: `gamelevel`, `0x00160080`.
- Before: 99.9863% historical MAX. The only residue was the order of two
  independent member loads used by one addition; the 14 branches, two returns,
  function size, and empty relocation multisets agreed.
- Classification: register allocation driven by TU-wide compiler handle state.
  Reversing the commutative expression, naming locals in retail load order, and
  seeding then widening the row were byte-identical at 99.9863%.
- Probe evidence: the mixed declaration-family sweep reached exact in 44 of 60
  disposable TU states, including trial 1. The exact state preserved the 391-byte
  function size and zero relocations; the real source and its hash
  (`a7b2facaa76c`) remained unchanged.
- Verdict: 100.00% historical MAX banked from an audited exact TU state; all
  probe declarations were removed.

## 2026-08-11 — `CGruntPuddle::Place`

- Unit/RVA: `wormhole`, `0x00040c30`.
- Before: 73.5400%. The body kept `placeIndex` in `edi`, shifting nearly every
  stack operand and the zero-case schedule.
- Classification: source defect first, then a bounded scheduling wall. Tracking
  the argument homes through retail's early `push 0` proves the post-lookup
  test reads the third argument (`color`), not the second (`placeIndex`).
- Source lever: change `if (placeIndex == 0)` to `if (color == 0)`. This removes
  the false live range and restores retail's prologue, parameter loads, member
  stores, calls, and all seven relocations.
- Result: 93.5600%. The residue is two independent load/store and
  load/argument-push transpositions. A receiver local, 60 mixed TU states, and
  33 AST variants were byte-identical, so the remaining scheduler residue is
  parked while the behavior correction is retained.

## 2026-08-11 — `CWwdGrid::Setup`

- Unit/RVA: `wwdgrid`, `0x001915c0`.
- Before: 80.8718%. The body has the right arithmetic and allocation calls, but
  its `RECT` parameter homes and allocation epilogue layout differ.
- Classification: the main defect was structural, followed by two bounded
  register/EH walls. Retail's `lea eax,[esi+0x28]` and four stores through one
  base prove one typed struct assignment; its swap triples operate on the dead
  by-value `RECT`, not four scalar copies.
- Source lever: give `WwdRect` an overlapping typed `RECT` view, assign
  `m_bounds.m_rect = rect`, normalize `rect` in place, and derive width/height
  from it. This makes the complete bounds-copy, both swaps, and both dimension
  calculations agree instruction-for-instruction.
- Result: 92.14%, up from 82.79% at the start of this pass (80.8718% before the
  earlier field-order sweep). Remaining differences are two dead-parameter
  homes and retail keeping two /GX return epilogues where cl merges one.
  Six shift-result forms, nine allocation-exit forms, and 17 eligible states
  from an 80-trial mixed TU probe were flat or worse. The structural correction
  is retained and the bounded residue is marked `@early-stop`.

## 2026-08-11 — `CSingleFrameMessage::CSingleFrameMessage`

- Unit/RVA: `singleframemessage`, `0x000ab310`.
- Before: 84.0175%. Retail computes both rectangle centers before either member
  store, keeping the vertical result in `ecx` while it computes the horizontal
  result. The source's two direct assignments let cl finish and store one center
  before starting the other.
- Source lever: materialize `centerY` and `centerX` as two simultaneously-live
  locals, then store them in member order. This restores retail's two-result
  dataflow and prevents the first store from shortening the second expression's
  live range.
- Result: 86.31%. The remaining whole-function rotation is retail reserving
  `ebx` as a zero value through the inlined base constructor and reusing it for
  `r.left`, while the candidate reuses `edi`. Eight reviewed center/local
  shapes and all 80 mixed TU states were flat or worse. The liveness correction
  is retained and the bounded register residue is marked `@early-stop`.

## 2026-08-11 — `CGameLevel::LoadWwd`

- Unit/RVA: `gamelevel`, `0x0015d280`.
- Before: 84.7577%. The compressed path named the allocation size rather than
  the inflater's capacity and kept the original header and returned payload as
  unrelated pointer values, rotating their registers and stack homes.
- Classification: source-level value identity followed by a bounded register
  wall. Retail preserves the original header for metadata in a stack home, but
  consumes the inflater result in the pointer parameter's former callee-saved
  register.
- Source lever: name the capacity directly, save `source = hdr`, then rebind
  `hdr` to the inflater result and derive the payload byte view from it. This
  expresses the original-header versus current-buffer lifetimes visible in the
  retail dataflow.
- Result: 91.22%, with all 10 ordered relocations exact. Swapping the plane-loop
  cursor/index declaration shape was byte-identical, and all 14 eligible states
  from a 60-trial mixed TU probe stayed at 91.22%. The remaining differences are
  stack-home and loop-register colouring, so the structural correction is
  retained and the bounded residue is marked `@early-stop`.

## 2026-08-11 — `Blowfish_encipher`

- Unit/RVA: `blowfish`, `0x0016f7f0`.
- Before: 60.3505%. Its mirror `Blowfish_decipher` was exact, but the encipher
  diff was a pure whole-function register/scheduling rotation over the same 16
  unrolled Feistel rounds.
- Classification: TU declaration state, confirmed by the existing controlled
  include-position matrix. A compiland merge had hoisted `<string.h>` above both
  twins, although retail's two functions occupy different compiler-state windows.
- Source lever: use `<memory.h>` above `CButeTail::Encode` for `memset`, and parse
  `<string.h>` only between encipher and decipher. This changes only the later
  twin's cumulative declaration state without adding a synthetic declaration.
- Verdict: both ciphers are 100.00% byte-exact, and the complete `blowfish` unit
  is 6/6 exact. This recovers a historical exact state lost during the compiland
  merge and validates the load-bearing include-position pattern end to end.

## 2026-08-11 — `CSBI_ImageSet::Render`

- Unit/RVA: `sbi_imageset`, `0x000e7440`.
- Before: 86.7368%. The CFG, 94-byte extent, and both relocations already
  agreed, but the frame-table lookup and render-call halves used opposite
  register schedules.
- Source lever: declare the frame index before the frame-set pointer. This is
  not cosmetic: it makes the complete lookup half instruction-exact and raises
  the ordinary build to 87.8421%.
- Classification: reachable VC5 declaration-state wall. On the corrected
  source, mixed trials 22, 33, 51, and 56 each produce a fully exact 94-byte
  function; the same trials stop at 98.8947% with the old local order. An
  isolated 110-trial family sweep shows that typedef/member/extern/static-data/
  prototype state moves the schedule while enum/struct/class/function state is
  flat.
- Verdict: the semantic local-order correction is retained, the synthetic
  probe declarations are not. The audited 100% state is banked as MAX for
  source hash `21bbbd1672f1`; ordinary codegen remains 87.8421% and is marked
  `@early-stop` until an authentic declaration window is recovered.

## 2026-08-11 — `CWwdGameObjectA::ApplyLookupSprite`

- Unit/RVA: `wwdgameobject`, `0x001504d0`.
- Before: 87.2857%. The lookup result and frame index were assigned separately
  in both range-test arms, shortening the selected frame's lifetime and
  rotating every post-`Lookup` register assignment.
- Classification: source-level pseudo lifetime followed by one bounded
  scheduling slot. The retail success and failure arms converge before storing
  `m_frameIndex` and `m_layer`, proving one shared `CImage*` result local.
- Source lever: declare that result once across both arms, assign it in the
  range test, and perform the two member stores after the join. Declare the
  typed worker only after the MFC out-parameter lookup so it has the same live
  range as retail.
- Result: 94.2857%. The complete post-call body, six-block CFG, three symbolic
  branch targets, two returns, and the ordered relocation all agree. The sole
  residue is `sprOb = 0` scheduled between the two `Lookup` argument pushes
  instead of after both. A 35-variant local matrix and 96 mixed TU-state trials
  were flat; the same measured residue already occurs at three other complex-
  receiver `CMapStringToOb::Lookup` sites. The structural correction is retained
  and the bounded scheduling residue is marked `@early-stop`.

## 2026-08-11 — `zDArray<CActHandler>::~zDArray`

- Unit/RVA: `zdarrayderived`, `0x00008750`.
- Before: 20.00%. An empty derived destructor only stamped its vtable and
  tail-jumped to `_zvec::~_zvec`; retail also homed `m_base` in a four-byte
  frame and therefore called the base destructor.
- Classification: control-flow source shape followed by VC5 dead-loop
  elimination. The template element is a pointer-to-member function, so its
  pseudo-destruction is trivial, but retail retains the initializer of the
  deleted loop's pointer control variable.
- Source lever: express the real element walk as a pointer-controlled `for`
  loop from `AsElem(m_base)` through the last allocated element. A 10-form VC5
  matrix showed that counter-controlled loops retain `m_lo`, count-controlled
  loops vanish, and only this pointer-controlled form retains `m_base`.
- Verdict: 100.00% exact. The paired placement-construction loop also restores
  the constructor's `m_alloc` home, raising it from 82.75% to 92.1875% without
  `volatile`; its remaining eax/edx scheduling rotation is marked
  `@early-stop`.

## 2026-08-11 — `CGrunt::StepCompassMove`

- Unit/RVA: `gruntsteps`, `0x00051c00`.
- Before: 56.6221% current-source MAX. The two inlined movement-validity trees
  had 146 candidate blocks versus 158 retail blocks, and the toy-index guards
  used signed branches where retail uses `jbe`/`jae`.
- Classification: control-flow source shape plus a real field signedness defect,
  followed by register/inliner state. Retail tests all four diagonal quadrants
  as mutually exclusive compound conditions; the nested 2x2 tree encoded the
  same behavior but gave VC5 a different CFG. Retail also treats
  `m_toyTileIndex` and the `GetDwordDef` result as unsigned.
- Source lever: spell the quadrants as an `if`/`else if` chain, make the
  same-tile case the outer success arm so both inlinings jump to their shared
  success tail, and type the toy index/count as `u32`.
- Follow-up classification: the 831-versus-872 instruction deficit was not
  allocator noise. In each retail diagonal quadrant, VC5 reloads
  `board->m_width` and recomputes the 28-byte row stride. Our named `stride`
  local falsely shared that value across all four mutually exclusive arms.
- Follow-up lever: keep `board->m_width * 7 * 4` in each arm's actual pointer
  expression. This recovered 24 missing instructions and raised the ordinary
  build from 58.7547% to 60.7711%. A `goto` exit from the random-direction loop
  brought the total instruction count within two but collapsed 12 real retail
  branches and fell to 54.6561%, so it was rejected.
- Result: 60.7711%, with the source-level stride CSE defect removed. An
  eight-form quadrant topology sweep rejected independent-arm hybrids. A
  16-form flag-width sweep scored higher for mixed widths, but retail's first
  inlining directly proves the original byte reads; the score-only dword forms
  were rejected. The remaining branch and register/inliner residue stays
  `@early-stop`.
- Switch-order follow-up: decoding the two retail tables at `0x0045287c` and
  `0x004528a0` proves the outer fixed-arrow cases own the cardinal bodies in
  `N,E,S,W` order. The nested `ARROW_CURRENT` table points those four entries
  back at the same bodies and owns only the four later diagonal bodies. Moving
  `ARROW_CURRENT` after the fixed cases changes the candidate body order from
  `NE,SE,SW,NW,N,E,S,W` to retail's `N,E,S,W,NE,SE,SW,NW` and raises the
  current-source result from 60.7711% to 60.7996%.
- Negative controls: five scalar/`Coord` entity forms left the four-scalar form
  best; source-only `Coord` values were effectively flat, while reference-backed
  source/move objects grew the frame and fell to 54.40%, 52.50%, and 52.26%.
  A 1,024-state request produced 984 legal variants and was stopped once the
  tables proved a source-order defect; no completed state before interruption
  exceeded the baseline. The
  remaining 82-versus-83 branch residue lies in the second movement-validity
  expansion, where retail spills `result` and widens four aligned flag probes
  to dword `0x2000` tests while the candidate retains byte `0x20` tests.
- Completed state exhaustion: 124 of 128 parser-visible trials compiled against
  the corrected switch order; every valid typedef/enum/struct/class/packed/
  member/extern/static-data/prototype/function/include/mixed cell was
  byte-identical at the focused 60.5542% baseline (155 candidate versus 151
  retail relocations). Four include/mixed cells were rejected by compilation.
  The state axis therefore cannot supply the missing spill/branch/inlining
  decisions; no synthetic TU declaration was retained.

## 2026-08-12 — `CAreaMgr::IsSameWorld`

- Unit/RVA: `areamgr`, `0x0009b430`.
- Before: 58.6471% current-source MAX. Retail and candidate already had the
  same three-block CFG, one branch, two returns, and no relocations; the wall
  was a whole-function parameter/divisor register rotation.
- Classification: VC5 TU declaration state. An exhaustive 1,024-trial state
  sweep moved the unchanged body to the retail 73-byte extent. Its best class
  reached 99.7059%, with the sole residue `cmp esi,eax` versus retail
  `cmp eax,esi`.
- Source lever: retain the semantic operand order visible in retail,
  `currentWorld == requestedWorld`. Under the proven mixed state this emits all
  34 retail instructions exactly. The function has no address operands, so the
  byte-exact result also closes the relocation audit.
- Verdict: 100.00% current-source MAX banked for source hash `f06c6ed10d01`.
  The synthetic state was removed; ordinary codegen remains at 58.5000% and is
  parked because it is the equivalent alternate register assignment.

## 2026-08-12 — `CGrunt::StepDiggerBehavior`

- Unit/RVA: `gruntdiggerstep`, `0x000f36a0`.
- Before: 62.6290% current-source MAX. The wall classifier found identical call
  multisets but 53 candidate conditional branches against 54 retail, plus a
  whole-function callee-saved-register rotation and an oversized frame.
- Source levers: each `GetScreenPos` writes one eight-byte `Coord`, so replace
  the two unsupported `Coord[2]` locals with single objects; this removes 16
  frame bytes. Retail then materializes `m_poweredUp == 0` and unsigned
  `m_dwell > 1000` as 0/1 values and combines them with `test`, proving eager
  bitwise boolean `&` rather than short-circuit `&&`.
- Result: 63.3527%, with retail's 85 basic blocks, five returns, 13-callee
  multiset, and 29 relocation identities in order. The frame narrowed from
  0xa4 to 0x94; retail is 0x8c. The remaining two conditional branches are the
  low-stamina `m_poweredUp` and `m_neighborValid` rechecks that retail retains
  even though candidate cl folds them from the enclosing guards.
- Bounded residue: 164 parser-visible TU states produced only two code hashes;
  156 were byte-identical at 63.3527% and eight were slightly worse. Every state
  kept the 1,924-byte extent, 29 relocations, 0x94 frame, and `this` in `ebx`
  rather than retail's `ebp`. The structural fixes are retained and the
  compiler-state residue remains `@early-stop`.
- Shared-entity follow-up: the same retail bounds tail reads every field through
  one materialized pointer. Modelling that pointer in `GRID_RECT_BOUNDS` raises
  this function to 65.9534% without changing its source, frame, CFG, call set or
  ordered relocations. The gain therefore belongs to the reconstructed inline
  entity; the two redundant low-stamina guards remain the first unresolved
  control-flow wall.
- Assigned-temporary follow-up: retail's second `CRect` constructor returns a
  pointer in eax, and the next four loads copy from that pointer into a distinct
  `RECT`. Reconstructing the shared expansion as `RECT ra; ra = CRect(...)`
  reproduces that dataflow and raises this function to 67.2912%. Candidate and
  retail still have the same callee/relocation multiset; the remaining five
  instruction and two-branch deficit stays in the already bounded control-flow
  and register wall.

## 2026-08-12 — `SoundStream::CreateStreamBuffer`

- Unit/RVA: `soundstream`, `0x00137780`.
- Before: 63.0248% current-source MAX. The classifier found identical callees
  but 15 candidate blocks against 19 retail and two returns against one. Our
  positive compound gate inverted all four entry branches and gave the HRESULT
  failure path a separate /GX epilogue.
- Source levers: reconstruct the four retail-negative entry guards as flat
  early returns; declare the POD work objects only after those guards; leave
  the DirectSound buffer pointer uninitialized because the successful COM call
  defines it and the failed path returns before reading it; directly initialize
  the `StreamVoice*` from `new`; retain the proven `!= 0` normalization of the
  DirectSound result.
- Verdict: 100.00% exact. Candidate and retail are both 369 bytes and 121
  instructions, with 19 blocks, eight conditional branches, one return, the
  same five-callee multiset, and an exact ordered relocation stream including
  offsets, types, identities, and addends. The stale `@early-stop` marker was
  removed.
- Doctrine correction: an older pattern table called this a `goto fail` case.
  That was an intermediate improvement against the mismodelled local set, not
  the original structure; ordinary flat guards reach exact once the associated
  dataflow is faithful.

## 2026-08-12 — `CMenuPage::AddSubItem2`

- Unit/RVA: `menupage`, `0x00183850`.
- Before: 63.1176% current-source MAX. Candidate and retail agreed on the
  allocation, six `CString` constructors, and `Append`, but candidate expanded
  `CMenuItem::Reset` into four `CString::Empty` calls where retail emitted one
  call to the standalone `Reset` COMDAT. That expansion also merged two retail
  exits and rotated the frame/register plan.
- Classification: per-caller inline-budget wall caused by missing source
  entities, not declaration state. All 192 mixed parser-visible TU states were
  byte-identical at the plateau. Adding 128 dead locals or 16 redundant member
  stores to `Reset` also changed neither site choice nor emitted bytes.
- Source levers: model the two command-field writes as the class's tiny inline
  `SetCommandParam` and `SetSecondaryCommandId` methods in both sub-item
  factories. Make the existing virtual `CMenuItem2::SetFrame` header-visible
  and call it from the constructor instead of duplicating its one store. Those
  three real inline sites compile to the same stores but charge the front-end
  candidate list; together they make VC5 decline the nested base `Reset` only
  in the largest factory. A combined two-field setter alone, the two setters
  without header-visible `SetFrame`, and header-visible derived `Reset` were
  measured and rejected.
- Verdict: 100.00% exact. All four menu-item factories remain exact, as do the
  standalone `SetFrame` and `Reset` bodies. `AddSubItem2` has the retail
  315-byte admitted extent, 102 instructions, eight blocks, three branches, two
  returns, and the exact ordered relocation stream including offsets, types,
  identities, and addends. The stale `@early-stop` and historical “TU state”
  explanation were removed.

## 2026-08-12 — `CGrunt::StepBrickLayerBehavior`

- Unit/RVA: `gruntbricklayerstep`, `0x000ecc90`.
- Before: 59.0364% current-source MAX. The first 15 blocks agreed, but eight
  candidate returns versus six retail returns rotated the powered-up state
  machine. The candidate also carried one `CRect(int,int,int,int)` relocation
  in the later clipping path that retail does not have.
- Classification: control-flow/source-entity wall. The structurally parallel
  `StepGooSuckerBehavior` spells the powered-up failures as jumps to one local
  return, and its six retail exits were already reproduced. Retail's clipping
  null arm writes all four `RECT` fields directly; the ordered relocation
  stream therefore disproves a constructor at that site.
- Source levers: merge the powered-up failures at `L_powered_yes`, and use a
  field-store variant of the inlined clip expansion. This raises the ordinary
  build to 59.9576%, narrows the candidate to seven returns, restores retail's
  0x7c frame, aligns the control-flow skeleton through B24, and removes the
  extra constructor relocation. Candidate and retail now both have 34 address
  relocations.
- Bounded residue: the first true skeleton divergence is B25, where cl deletes
  the low-stamina `m_poweredUp` re-test retained at retail `0x000ece99`; one
  later return remains unmerged, and `this` is in ebp instead of retail's ebx.
  Sharing the powered-up check across both stamina arms worsened the score to
  59.7379%. Eighty mixed TU-state trials never changed the wall class and
  peaked at 59.9727%, so the authentic structural corrections are retained
  while the remaining control-flow/register residue stays `@early-stop`.
- Wall-identifier follow-up: inline/call counts and all 34 relocation identities
  agree, so the portable classifier keeps this in control-flow/register routing,
  not the inliner bucket. The two legal `Coord` declaration orders and grouped
  form all compiled byte-identically at 59.9576%. Retail's initial bounds tail,
  however, uses one materialized `RECT*` for `IntersectRect`, the fallback copy,
  and both size calculations. Giving the shared `GRID_RECT_BOUNDS` entity that
  typed `clipBounds` local changes exactly those reads plus the downstream
  register schedule and raises current-source MAX to 61.0500%; the 70-versus-69
  branch and 6-versus-7 return residue is unchanged and remains bounded.
- Assigned-temporary follow-up: retail dereferences the second `CRect`
  constructor's eax return for four copy loads before `IntersectRect`, proving
  that `ra` is a separate `RECT` assigned from a temporary rather than a second
  `CRect` copied from `rb`. Applying that shared lifetime raises current-source
  MAX to 61.8076%. The candidate is now three instructions short with the same
  callee/relocation multiset; its 69-versus-70 branches and 7-versus-6 returns
  remain the next structural wall.
- Entry-snapshot follow-up: the sibling `ChargeStep` and retail's live `ecx`
  through both stamina arms identify a distinct entry value for `m_poweredUp`.
  Introducing that local preserves retail's otherwise redundant low-stamina
  guard, restores the conditional-branch count to 70-versus-70, and raises MAX
  to 61.82%. The remaining seventh return is now localized to the first
  `DRAIN_COORDS` expansion: retail sends its zero-count arm to the shared final
  epilogue while the candidate duplicates that epilogue; the second expansion
  already has retail's split. No `STATE` probe was used because this function is
  still a source-layout problem, not an isolated high-score register wall.
- Positive-null-gate follow-up: retail's `test esi; je null` falls through into
  the full non-null combat body, while the inverse source spelling parked that
  body out of line. The higher-scoring `StepGooSuckerBehavior` independently
  uses the positive `if (g != NULL)` form. Reorienting only that gate raises MAX
  from 61.8152% to 83.85% and makes all 70 conditional branches agree in their
  complete mnemonic and symbolic-target sequence. The remaining structural
  residue is the already-localized seventh return at the first empty
  `DRAIN_COORDS` path.

## 2026-08-12 — `SoundDevice::CreateBuffer`

- Unit/RVA: `directsoundmgr`, `0x001366f0`.
- Before: 65.5630% current-source MAX. Candidate and retail had the same calls
  and one `/GX` epilogue, but the source initialized one `voice` pointer to zero
  and reused it for the successful allocation. VC5 therefore put both the
  failure result and working voice in esi, collapsed each guard into
  `xor esi,esi`, and repeatedly reloaded `bytes`; retail returns failure zeros
  through eax, keeps the successful voice in esi, and holds `bytes` in ebp.
- Classification: control-flow plus register-live-range wall, not TU state.
  Plain early returns duplicated seven complete epilogues. Assigning zero in
  each failure arm through the one shared pointer restored the retail branch
  skeleton and raised 65.5630% to 77.4202%, proving the common-return shape but
  retaining the wrong live-range identity.
- Source levers: use a distinct uninitialized `result` for the common return and
  a block-local `voice` for the successful construction, assigning
  `result = voice` only after the post-construction work. This restored retail's
  eax/esi/ebp roles and reached 96.5546%. Removing the unsupported pre-zero of
  the COM out-parameter and moving `wf.cbSize = 0` immediately after the format
  copy removed the final store and instruction-order difference.
- Verdict: 100.00% exact. Candidate and retail are both 360 bytes with the same
  branch skeleton, one return, ordered relocation stream, and EH funclet. The
  stale `@early-stop` marker was removed.

## 2026-08-12 — `RotateRasterize`

- Unit/RVA: `imagepolyclip`, `0x00146550`.
- Before: 66.1540% current-source MAX. The branch skeleton already agreed, but
  all four clipping passes associated each attribute delta with a shared
  `clipDelta / axisDelta` ratio. Retail divides each attribute delta by the
  axis delta first, producing three independent x87 divides.
- Classification: algebraic-expression, traversal-identity, and pseudo-lifetime
  walls. The last three passes copied and interpolated from `cur`; retail's
  relocation and pointer progression prove that every pass emits from `prev`
  and advances `prev = cur`. The two traversals are geometrically equivalent
  only up to a cyclic vertex rotation, so this was a real source correction.
- Source levers: associate each interpolation as
  `(attributeDelta / axisDelta) * clipDelta`; use the retail `prev` traversal in
  all four passes; initialize the rotated default X bound from
  `dst->m_height`, the default Y bound from `dst->m_width`, keep `clip1` as an
  immediate zero, and load `bound0` independently from `g_c10`; create each
  last-vertex pointer before its positive-count guard; in pass two, create the
  input-array pseudos before assigning the output array.
- Result: 95.5313% in the ordinary build. The 1,226-byte admitted extent, CFG,
  all 29 floating-point branch polarities, and the first interpolation block
  agree. The successive measured plateaus were 81.3884%, 90.1116%, 91.0446%,
  95.5089%, and 95.5313%, so each retained lever has an independent compiler
  verdict.
- Bounded residue: retail keeps the final default bound live across two initial
  pointer loads and batches the final two weight multiplications before any
  component additions in passes two through four. Named weighted-delta locals
  forced an eight-byte frame and regressed to 87.5893%; commutative addition
  order compiled identically, and `u, v, y` statement order was slightly worse.
  The closest structural reconstruction remains `@early-stop` while the narrow
  x87 scheduling state is unresolved.

## 2026-08-12 — `CBattlezMapConfig::RouteToNearbyEnemy`

- Unit/RVA: `battlezmapconfig`, `0x0002e3a0`.
- Before: 67.4795% current-source MAX. Candidate and retail both had 58
  conditional branches, but candidate had four returns against retail's three.
  At retail `0x0002e7d1`, unsigned `m_dwell <= 100` branches forward to the
  function's existing `mov eax,1` success epilogue; the source instead returned
  immediately from the guard and made VC5 emit a fourth success epilogue.
- Classification: control-flow layout and tail merging. The branch target, not
  the equivalent C condition, proves that the cheap path shares the later
  success return. This is the mid-function form of
  `positive-gate-enables-shrink-wrap.md`.
- Source lever: express the expensive route update as the positive gate
  `if (static_cast<u32>(m_dwell) > 100) { ... }`, then leave one `return 1`
  after it. This preserves the behavior while giving both success paths the
  same source return node.
- Result: 68.7256%. Candidate and retail now both have 96 basic blocks, 58
  conditional branches, and three returns. A follow-up lifetime experiment
  removing the scope around the four initial `Coord` probes enlarged the frame
  from `0x6c` to `0x88` against retail's `0x80` and regressed to 68.21%, so it
  was rejected. The proven exit-layout correction is retained; the remaining
  frame and register-colouring residue stays `@early-stop`.

## 2026-08-12 — `CGruntzCmdMgr::BlitTileMarker`

- Unit/RVA: `gruntzcmdmgr`, `0x00023d90`.
- Before: 65.0790% current-source MAX. Candidate and retail have one block, no
  conditional branches, one return, and the same sole callee/referent. Retail
  emits 38 instructions and saves esi/edi; candidate emits 40 and additionally
  saves ebx. The extra live arithmetic pseudo rotates the complete coordinate
  calculation.
- Classification: register allocation driven by TU parser state. The exact
  current function hash had a reproducible historical 66.4210% proof, so this
  was not inferred from a plausible register pattern. A correctly configured
  512-trial state sweep reproduced that value in 20 mixed/include states; the
  other 492 states were byte-identical at 65.0790%.
- Negative controls: splitting each compound coordinate into raw/snap locals,
  then splitting out the camera-origin deltas, compiled byte-identically. A
  600-iteration semantic permutation sweep over commutative operands,
  declarations, and independent statements was completely flat. The source
  was restored after every experiment.
- Verdict: 66.4210% current-source MAX restored from the independently
  reproduced same-source proof. None of 512 parser states reached 100%, so the
  remaining two-instruction, ebx-save residue is a bounded VC5 handle-state
  wall. No unused declaration/include or unproven entity spelling was retained;
  the complete body remains `@early-stop`.

## 2026-08-12 — `CDDSurface::ShadeBlt`

- Unit/RVA: `ddsurface`, `0x0013f020`.
- Before: 66.6959% current-source MAX. Candidate and retail both have 29
  conditional branches and two returns, but branch 11 differs: retail factors
  the common `g_rDown != 3` failure directly to reject while retaining the
  repeated `g_rDown` comparison in the RGB565 arm. Candidate enters that arm.
  Retail also has 39 blocks against candidate's 37 because each row loop keeps
  a forward-jump first-iteration path around one loop-carried reload.
- Classification: optimizer control-flow and register-state wall after the
  already-proven semantic reconstruction. The two format arms use the same
  conditions, pixel equations, callees, and cleanup outcomes; the bank spelling
  already emits retail's separate `and 0xff / shr 3 / shl 0xb` sequence.
- Negative control: adding an explicit early `g_rDown` guard looked like the
  branch-11 source fix, but VC5 then removed the later redundant comparison.
  Candidate fell to 28 branches and 66.19%, disproving that source shape; the
  guard was removed.
- State sweep: 512 single declaration/include/parser-state trials produced four
  score classes in the focused scorer: 117 at the 66.68025% baseline, 2 at
  66.31035%, 380 at 66.27273%, and 13 at 64.59875%. None reproduced retail's
  topology or exceeded baseline.
- Verdict: bounded at 66.6959% current-source MAX. Source is unchanged and the
  complete function remains `@early-stop`; no dummy state carrier or falsified
  guard was retained.

## 2026-08-12 — `CGrunt::TileSwitch`

- Unit/RVA: `grunt`, `0x0004b320`.
- Before: 66.7500% current-source MAX. Candidate and retail are each one basic
  block with no conditional branches, one return, and the same sole relocation
  to `CGrunt::StepArrivalDrop`.
- Classification: register-allocation wall. Retail saves esi, moves each of the
  four pass-through arguments through esi before pushing it, computes col/row
  in edx/eax, then restores esi. Candidate pushes the pass-through arguments
  directly and computes the same two coordinates in eax/edx. The call's values,
  order, target, and callee-cleaned stack size agree.
- Exhaustion: 49 semantic permutation variants were already flat. A new sweep
  compiled every one of the 494 valid single declaration/include/parser-state
  mutations for this TU; all 494 were byte-identical at 66.7500%, size 50, with
  the relocation count matching 1/1.
- Verdict: bounded at 66.7500% current-source MAX. Source remains unchanged and
  the complete wrapper stays `@early-stop`; no unused include, declaration, or
  artificial argument shuttle was retained.

## 2026-08-12 — `CPlay::SetEffectSpriteDurations`

- Unit/RVA: `playassetload`, `0x000dc060`.
- Before: 67.0648% current-source MAX. Candidate and retail have the same 1307
  byte size, 65/65 basic blocks, 32/32 conditional branches with identical
  symbolic targets, one return, and 64/64 matching relocations.
- Classification: repeated instruction-scheduling wall. For each of the 32
  sound-cue lookups, candidate writes `d = NULL` before forming and pushing the
  arguments. Retail forms and pushes them first, then writes the same stack slot
  before the same MFC lookup call. Every block has the same instruction count;
  only this independent-instruction order differs.
- Negative control: replacing the first typed `MapLookup` wrapper with the
  direct MFC `void*&` boundary expression compiled byte-identically, disproving
  the later union-view wrapper as the scheduling cause. The source was restored.
- State sweep: all 445 valid single declaration/include/parser-state mutations
  compiled byte-identically at 67.064835%, size 1307, relocations 64/64.
- Verdict: bounded at 67.0648% current-source MAX. The explicit per-cue records
  and values remain intact; no artificial table, cast, include, or state carrier
  was retained. The complete body remains `@early-stop`.

## 2026-08-12 — `CGrunt::ClaimSwitchTile`

- Unit/RVA: `gruntsteps`, `0x00052c70`.
- Before: 67.8429% current-source MAX. Retail's direction jump table proves the
  six distinct switch arms plus `SOUTHWEST -> SOUTH` and `NORTHWEST -> NORTH`
  fallthroughs. It also proves the odd out-of-range path: retail loads the
  uninitialized output pair from stack homes, while every valid arm derives it
  from `m_lastTilePx`. The source already expresses both facts; initializing the
  outputs would be a behavioral fabrication despite an older 67.8786% history.
- Classification: register coalescing at the switch join. Retail uses a 16-byte
  frame, keeps the seed/output pair in ebx/edi through the valid arms, and loads
  their uninitialized homes only on the default edge. Candidate uses an 8-byte
  frame, carries the seeds in eax/ecx, and stores the selected outputs before
  the join. The branch counts and two returns agree, but retail has 18 basic
  blocks against candidate's 19; the focused scorer sees 13/14 relocations.
- Rejected false gain: a 600-iteration semantic permutation found a 68.90%
  normalized candidate by reversing the x/y assignments in the northeast,
  east, and southeast arms. That reduced the candidate to 17 blocks and moved
  it farther from retail's raw instruction order. Restoring retail's assignment
  order restored 67.8429%; declaration splits from the same candidate were
  byte-neutral. The higher fuzzy score was therefore rejected.
- State sweep: none of 512 valid single declaration/include/parser-state
  mutations beat the faithful focused baseline of 67.557144%. The source was
  restored automatically after the sweep.
- Verdict: bounded at 67.8429% current-source MAX. The retail-proven jump-table
  behavior remains intact and no score-only arm reorder, output initialization,
  unused include, declaration, or artificial state carrier was retained. The
  complete body remains `@early-stop`.

## 2026-08-12 — `CGrunt::ScanNearestTarget`

- Unit/RVA: `gruntscantarget`, `0x000f42f0`.
- Before: 68.2796% current-source MAX. The 5.4 KB candidate is one block, one
  conditional branch, and one return short: 396/397 blocks, 90/91 branches, and
  12/13 returns. Its frame is `0x44` against retail's `0x40`. The first 60 block
  topologies agree, while the prologue and early target-selection code use a
  whole-function callee-saved-register rotation.
- Classification: control-flow tail plus TU-sensitive register allocation. The
  old source comment overlocalized the missing exit to a merged powered-up reset
  arm, but the candidate already has two ordered relocations to
  `ResetEntranceAnimation`, matching retail's two call sites. The comment was
  removed rather than preserving a disproven mechanism.
- State sweep: all 437 valid single declaration/include/parser-state mutations
  compiled. 434 were byte-identical at the focused 67.513310% baseline; three
  include states reached 67.513985%. Every variant retained the 5,436-byte
  candidate extent and 312/312 relocation count, and none fixed the frame, CFG,
  or authoritative normalized MAX. The best disposable state was reproduced in
  a normal build, where it remained 68.2796%, then removed.
- Rejected entity hypothesis: the five-store plus `ResetEntranceAnimation`
  sequence recurs throughout the class, so it was tested as one header-inline
  `CGrunt` method at both sites. VC5 emitted the target function identically and
  rebuilt scores of unrelated consumers through header state; the helper was
  removed.
- Verdict: bounded at 68.2796% current-source MAX. All 312 relocation slots are
  present, the complete behavior remains `@early-stop`, and no synthetic TU
  state, unsupported helper, or score-only source change was retained.

## 2026-08-12 — `CRandomAmbientSound::InitCycleTiming`

- Unit/RVA: `worldsoundset`, `0x0000cd70`.
- Before: 68.8846% current-source MAX, 219 candidate bytes against 229 retail
  bytes, and 10/9 relocations. The source called `GetRandomNumber` separately in
  the zero-span and modulo arms, so cl reloaded the inline local-static guard in
  both arms. Retail loads that guard once before the span branch, then retains
  two mutually exclusive PRNG update blocks.
- Source entity recovered: one `random` local is assigned by both call sites and
  consumed in their respective arms. This preserves the two calls and observed
  behavior while exposing their shared result identity to cl. The candidate is
  now 74.6667%, 214 bytes, and 9/9 relocations; its 11-block CFG, four ordered
  conditional branches, and three returns agree with retail exactly.
- Rejected shapes: one call before the branch collapsed both PRNG updates and
  fell to 19.7692%; a shared zero-span countdown merged retail's two exits and
  scored 66.9872%; inverted coin-flip polarity scored 64.6795%. Endpoint locals
  and moving the span declaration were byte-neutral. The original two unrelated
  result expressions retained the extra guard relocation.
- Exhaustion: all 128 parser-visible TU states were byte-identical at 74.666664%,
  214 bytes, and 9/9 relocations. All 14 legal depth-two expression/declaration
  AST variants were also flat. An earlier 128-state sweep of the merged-exit
  shape was flat at 66.987180%.
- Remaining wall: retail colors `min/max` as `ebx/ebp`; cl colors them as
  `ebp/ebx`, rotates the affine PRNG scratch registers, and emits direct member
  stores where retail uses an `edx` move. No synthetic TU state or unsupported
  helper was retained. The function remains `@early-stop` at a new 74.6667%
  current-source MAX.

## 2026-08-12 — `CGrunt::LoadPickupSprites`

- Unit/RVA: `gruntpickupload`, `0x00065e80`.
- Before: 68.8631% current-source MAX. Candidate and retail already had the
  same 63 conditional branches and two returns, but the first true CFG-skeleton
  divergence was block 97. The source emitted the brick bodies before the
  health and powerup bodies, so relocation masking hid a large source-order
  error behind plausible fuzzy matches.
- Source structure recovered: the retail COFF relocation stream proves the
  outer switch-body order independently of enum values. After the ordinary
  tool/toy bodies it emits `HEALTH1..3`, `CONVERSION`, `DEATHTOUCH`, `GHOST`,
  `INVULNERABILITY`, `REACTIVEARMOR`, `ROIDZ`, `SUPERSPEED`, then the megaphone
  body and its inner switch, the four brick bodies, the four forced-effect
  bodies, and finally `W`, `A`, `R`, `P`, `HELPBOX`, `COIN`, `STOPWATCH`.
- Progression: moving megaphone behind the health/powerup band raised fuzzy to
  70.9176% and moved the first skeleton divergence to block 103. Moving the
  brick band behind megaphone raised it to 75.2437% and moved the divergence to
  block 157. Reordering the final literal bodies and the internal powerup band
  made the complete ordered literal-referent stream agree with retail. The
  faithful final source is 74.6667%; the lower score is accepted because the
  75.2437% intermediate still had retail-disproven case order.
- Behavioral payoff: the candidate now reaches `GRUNTZ_PICKUPS_HEALTH1..3` in
  the same switch-body positions as retail instead of comparing those bodies
  against the red/blue/gold brick literals. This removes the named-asset
  identity defect at its source; it is not a relocation-masked score claim. A
  fresh static candidate link reports 24 wrong-referent regions and 37
  ordering-only regions globally; `LoadPickupSprites` is absent from both.
- Remaining wall: the branch sequence is exact, while the first skeleton
  divergence remains at block 157 where VC5 shares the forced-effect tail
  differently. Retail has no local stack allocation and reuses the unread
  fourth parameter's home as the `CAniElement*` scratch; the candidate emits
  `push ecx` and uses a separate local, perturbing stack references and register
  scheduling across the 5 KB body. The source remains `@early-stop`.

## 2026-08-12 — `CBattlezMapConfig::Step`

- Unit/RVA: `battlezunitstep`, `0x00031610`.
- Before: 76.2066% current-source MAX. Candidate and retail both had 36
  conditional branches, but candidate had six returns against retail's five,
  two branch-polarity mismatches, and the first CFG-skeleton divergence at
  block 28.
- Control-flow reconstruction: the active-target half is a positive
  `if (cur != NULL)` region whose fallthrough is the clear-target path. Its
  distance test has separate recycle arms; the far arm jumps to `L_clearAt`,
  while the near arm reaches `TileSwitch` and shares `L_done` only on success.
  This recovered retail's shared dwell reset without fabricating behavior.
  Candidate and retail now have 55 blocks, 36/36 conditional branches with the
  same polarity and symbolic targets, and 5/5 returns.
- Entity spelling: retaining the arrival cell as one copied `Coord` raised the
  faithful source from 86.1148% to 86.8163%. The compiler still deletes the
  copy's two stack stores, consistent with
  `dead-eight-byte-coord-temp-is-unreproduced.md`; it is not a claim that a
  source local can force retail's spill.
- Relocation and constant audit: all 32 function relocations have the same
  ordered target sequence as retail. The strong-immediate sieve has no
  candidate-only value; its only retail-only row is three extra `-1` encodings
  caused by the differing store/spill shape.
- Exhaustion: 56 valid single parser-visible TU-state trials produced only
  86.8163% or 86.8112%; eight include/mixed cells failed compilation. None
  changed the 24-byte candidate frame into retail's 32-byte frame or reached an
  exact closure.
- Verdict: 86.8163% faithful current-source result. The remaining whole-function
  register rotation (`this` in ebx versus edi) and dead eight-byte spill pair
  are a bounded VC5 allocator wall, so the complete function remains
  `@early-stop`. No volatile local, unused declaration/include, or score-only
  control-flow spelling was retained.

## 2026-08-12 — `CLightFxRender::Resize`

- Unit/RVA: `lightfxrender`, `0x000a3460`.
- Source state: the repeated `OccupantAt`/`TileIdAt` expressions are already
  recovered as separate inline entities. That prevents a false whole-loop CSE,
  restores retail's frame, and leaves the prologue plus the first 68 loop
  instructions byte-exact. Candidate and retail both have 34 conditional
  branches and four returns.
- Remaining wall: after the aligned prefix, VC5 colors x/alt/this as
  edi/ebx/ebp instead of retail's esi/edi/ebp and swaps the y and x-stride spill
  slots. The changed register schedule makes the cross-jumper group and place
  the later loop exits differently (62 candidate versus 61 retail blocks), so
  the linear fuzzy score understates the aligned work. Every desc/dst/alt local
  spelling and the earlier 24 parser-state cells left this wall class intact.
- MAX-ledger correction: git history proves the exact current source hash
  `aaae03af7bd2` scored 69.2227%, and an older baseline correctly held
  `best_pct=69.2227`. A later update lowered best to 68.3193 without changing
  that hash while retaining `hist_pct=69.2227`. Restoring best to 69.2227 is a
  same-source MAX repair, not a historical-source carry or a new fuzzy claim.
- Verdict: complete body, bounded register/cross-jump wall at a proven 69.2227%
  current-source MAX. No unused include, declaration, or fitted ternary was
  retained.

## 2026-08-12 — `zPTree::FindOrInsert`

- Unit/RVA: `butetree`, `0x001933b0`.
- Classification: register-homing wall after the inline `strlen`. Retail keeps
  `key` in esi, copies it to edi for `repnz scas`, and spills `sbit` to the
  frame. Candidate loads `key` directly into edi, loses it across the scan, and
  keeps `sbit` in a register. That removes retail's separate loop preheader and
  accounts for the complete 48-versus-52 block and 25-versus-26 branch delta.
  Both sides retain three returns and the ordered 10/10 relocation sequence.
- Local exhaustion: the existing 120-cell Cartesian matrix crossed `sbit`
  declaration, child-slot selection, loop form, and key/strlen lifetime. Every
  legal semantics-preserving spelling was byte-identical at 68.71915%; the two
  child-selection variants that emitted different code scored lower.
- TU-state exhaustion: 56 of 64 parser-visible typedef/enum/struct/class/
  packed/member/extern/static-data/prototype/function/include/mixed trials
  compiled, and every valid cell was byte-identical at 68.71915%. Eight
  include/mixed cells were rejected by compilation.
- Verdict: complete body and bounded VC5 register/preheader wall. No synthetic
  declaration, forced spill, or score-only child selector was retained; the
  function remains `@early-stop` at 68.7191% current-source MAX.

## 2026-08-12 — `CTileTriggerSwitchLogic::SwitchUp`

- Unit/RVA: `tileswitchlogic`, `0x001106b0`.
- Break: retail loads the sound-enabled flag and cue tag together before the
  enabled branch, then keeps separate unsigned clock, elapsed, and replay-delay
  entities. Recovering those lifetimes changes the clock comparison from the
  incorrect signed `jl` to retail's wrap-safe unsigned `jb` and makes the
  complete sound-play tail align. Current-source MAX rises from 68.9195% to
  80.9310%.
- Controls: candidate and retail now emit 87/87 instructions, ten blocks, eight
  conditional branches, one return, and the same ordered 9/9 relocation
  sequence. The calls, globals, and `"GAME_SWITCHUP"` referent therefore agree;
  this is not a relocation-masked score gain.
- Refuted axis: naming `tileX` and `tileY` locals caused VC5 to spill `tileY`,
  lowered the score to 77.3333%, and moved farther from retail's register-only
  prefix. That spelling was removed.
- Residual wall: the two grid accesses and cue lookup have the same operations
  and control flow but different register colors and instruction scheduling.
  No forced register, unused declaration, or include-state steering was
  retained; the function remains `@early-stop` pending a source-level entity
  that explains that prefix.

## 2026-08-12 — `CMultiStartDlg::SyncChannelSlot`

- Unit/RVA: `multistartdlgroster`, `0x000c2ab0`.
- Source correction: on the empty-slot arm, retail tests
  `m_humanControlled != 0`, conditionally drops the player, then tests
  `m_humanControlled == 0` before restoring the color slot. The second test is
  present in the instruction stream, proving two independent guards rather
  than the reconstructed `else if (m_liveGate != 0)`.
- Controls: candidate and retail retain the same ordered 19/19 relocation
  sequence. The ordinary build remains 125 versus 128 instructions and
  69.3281% fuzzy: candidate reuses `edi` as a zero value on the early-return
  arm and folds the second human-control test, while retail keeps the channel
  index in `edi`, uses `test`, and retains the path-sensitive recheck.
- Exhaustion: independent guards, an explicit nested `else`, and cached
  human/live values reached no higher state; caching the human value across the
  arm rotated it into `edi` and regressed to 69.2891%. A 64-cell TU-state sweep
  produced 62 valid byte-identical objects at 69.3281%; two include/mixed cells
  failed compilation.
- Verdict: the control-flow identity is corrected, but its ordinary output is
  a bounded VC5 liveness/CSE wall. No long-lived cache, volatile value, unused
  declaration, or include steering was retained; the function remains
  `@early-stop` at 69.3281% current-source MAX.

## 2026-08-12 — `CLightFxRender::ComputeRect`

- Unit/RVA: `lightfxrender`, `0x000a3820`.
- Misclassified wall: the former source comment called the whole residue
  register colouring, but candidate had 158 instructions against retail's 156.
  Its combined inclusive-edge expressions emitted four `inc` and two `dec`
  operations; retail scales the four edges first and then extends right/bottom
  with two LEAs.
- Source break: preserve the four scale assignments and name the shared border
  extension `m_scale - 1` before applying it to right and bottom. This prevents
  VC5's `(edge + 1) * scale - 1` factorization, raises current-source MAX from
  69.4551% to 71.8590%, and reaches retail's 156-instruction count.
- Controls: candidate and retail now have 11 blocks, five identical symbolic
  branches, three returns, no strong-immediate mismatch, and the same ordered
  2/2 relocation sequence (`BltEx`, then `DrawBorder`).
- Exhaustion: a 25-cell Cartesian matrix crossed five inclusive-width forms
  with five combined/shared/per-edge extension forms. Only the shared/per-edge
  extension family reached 71.8590%; all width spellings were byte-identical,
  and direct/combined extensions returned to 69.4551%. On the corrected body,
  57 valid cells of a 64-family TU-state sweep were byte-identical at 71.8590%;
  seven include/mixed cells failed compilation.
- Residual wall: candidate still uses a 0x18 frame against retail's 0x14 and
  rotates `surf`, the width quotients, scale, and destination coordinates.
  With instruction/CFG/referent identity closed and both local and TU axes
  exhausted, this is a bounded VC5 register-homing/frame wall; no unused
  declaration, include, or synthetic spill was retained.

## 2026-08-12 — `CGrunt::StepGruntMovement`

- Unit/RVA: `grunt`, `0x0004c170`. Before: 69.4635% current-source MAX. The
  old source comment called the residue a record-home allocator wall, but the
  wall-identifier controls contradicted that classification: candidate and
  retail had different instruction and return counts, and the first CFG
  divergence was the `CoordCount() == 0` exit rather than a register-only row.
- Source break: retail's first direction cascade selects three independently
  live values. Each arm reads `column`, `row`, and `direction`, and only later
  materializes the `GruntDirectionCell` at the join. Modeling those selected
  scalars explicitly, then assigning the record once, raises the faithful
  current-source result to 76.2346%. The rescanned cascade has the same entity
  split, with retail's observed `row`, `column`, `direction` order.
- Referent and constant controls: the retained source has the same 99-entry
  relocation multiset as retail and still emits all four physical
  `SetEntrancePos` calls. The ordered relocation stream differs only by that
  call's placement: candidate emits the `CoordCount` failure copy near the
  function head while retail retains it at the end. The strong-immediate and
  mask-immediate sieves report no row for this function.
- Tail controls: sharing `label_dropRet0` from the later `PlaySound` arm raised
  the scalar variant to 77.7722%, but emitted 98/99 relocations, three
  `SetEntrancePos` calls, and seven returns against retail's nine. Sharing the
  immediately preceding `ValidateUnitPath` failure reached 77.0393% but again
  emitted only three calls and seven returns. Both higher fuzzy results were
  rejected because raw relocation/call evidence disproves them. The explicit
  fourth call is retained despite the lower score.
- Local exhaustion: all six declaration orders were byte-identical at
  77.7329% in the shared-tail diagnostic state. A 36-cell Cartesian product of
  the two record-store orders peaked at 77.7520% only by reordering the second
  join away from retail's observed field order, so that fitted result was
  rejected. Keeping only the scalars and constructing a temporary cell at each
  call regressed to 70.1740% and restored six-instruction arms.
- Remaining source/codegen split: the retained candidate has 843 instructions,
  185 blocks, 121 conditional branches, eight returns, and a 0x30 frame;
  retail has 891 instructions, 187 blocks, 119 conditional branches, nine
  returns, and a 0x38 frame. Retail spills the selected column in every first
  cascade arm and retains a second record home; candidate keeps the three
  selected scalars in registers and emits four-instruction arms. Separately,
  VC5 replicates the single-predecessor drop tail at the head. No fake
  predecessor, volatile spill, unused declaration, or score-only field order
  was retained; the complete function remains `@early-stop`.

## 2026-08-12 — `CSBI_MenuItem::ResolveFrame`

- Unit/RVA: `sbi_menuitem`, `0x000e81e0`; current-source MAX 70.3774%.
- Classification: register-homing wall. Retail zeroes `eax` before loading the
  selected frame and ends both selection arms with
  `test ecx,ecx; mov [esi+0x30],ecx; setne al`. Candidate reuses `eax` for the
  frame load and must materialize the boolean through `cl`.
- Local controls: four evidence-compatible result/frame spellings removed one
  candidate reload but scored about 65.1%, below the retained direct member
  form. An asymmetric local likewise measured 65.1887% and was removed.
- TU-state exhaustion: 14 compiling cells from a 60-state mixed declaration
  sweep were byte-identical at 70.377360%; 46 cells were rejected. No source
  spelling or parser-visible state moved the flag-register choice.
- Verdict: complete body and bounded flag-register wall. The stale causal prose
  was removed from the source; `@early-stop` remains the machine-visible state.

## 2026-08-12 — `CRezImage::FlipVertical`

- Unit/RVA: `imagepool`, `0x00176840`; current-source MAX 71.0707%.
- Classification: induction-variable/frame wall. Retail uses a 0x18-byte frame
  and retains the row counter, scratch pointer, a decreasing bottom-row entity,
  a byte offset, and the height. Candidate's direct symmetric loop uses a
  0x10-byte frame while preserving the same row-swap behavior.
- Source controls: explicit row/byte induction forms scored about 59–62%.
  Algebra chosen only to force the 0x18 frame emitted 102 instructions against
  retail's 99 and scored 57.98%; an explicit height local scored 56.78%, while
  symmetric top/bottom pointers scored 66.29%. All were removed as less
  faithful or artificial.
- TU-state exhaustion: 49 compiling cells from a 256-state mixed sweep were
  byte-identical at 71.070710%; 207 cells were rejected.
- Verdict: the simple complete swap loop is retained with no synthetic local or
  forced frame. The residue is a bounded VC5 induction/register allocation wall.

## 2026-08-12 — `CProjectile::AdvanceMotion`

- Unit/RVA: `projectile`, `0x000dfd00`. The full-build current-source result
  rises from 71.5214% to 88.5363%.
- x87 entity break: retail reloads `m_flightDist` from `[esi+0x188]` before each
  of eight threshold comparisons, with no intervening call or store that could
  invalidate it. Deleting the invented shared `double mag` and spelling the
  member at every use raises the result to 76.8162%. A six-form Cartesian
  distance matrix made every raw/absolute-value alternative worse.
- Control-flow break: retail emits `test ah,0x9; jne` to a far, out-of-line water
  splash arm while the ordinary landing logic falls through. Expressing the
  ordinary path as the negative gate and the splash as its `else` restores that
  layout and raises the result to 87.5256%.
- Tail reconstruction: loading and testing `PF_FALL` and `PF_IMPACT` inside
  their respective arms, then sharing the setup/no-sprite labels, raises the
  full-build result to 88.5363% and brings candidate and retail to 90/90 CFG
  blocks. The first remaining skeleton divergence is the placement of that
  shared setup block, not a missing arm.
- Controls: a four-form tail-layout matrix found the retained structured form
  tied for best; explicit impact/fall labels were worse. All variants retained
  the ordered 49/49 relocations. Of 64 requested parser-state trials, 56 were
  legal and every one was byte-identical at the standalone scorer's 88.482900%;
  eight unsafe include/mixed states were rejected.
- Residual wall: candidate and retail still rotate the zero/kind registers,
  order the first x87 multiply pair differently, and place the shared sprite
  setup block on opposite sides of the impact arm. With local CFG forms and
  parser-visible state exhausted, this is bounded VC5 scheduling/register
  placement. No forced register, unused include/declaration, or duplicate call
  was retained.

## 2026-08-12 — `CGrunt::StepArrivalDrop`

- Unit/RVA: `grunt`, `0x0004b370`; current-source MAX remains at the linear
  scorer's 0.0000% floor.
- Entity break: retail references `g_gameReg` seven times; the candidate
  referenced it ten times and had a 0x4c frame against retail's 0x50. One
  nudge-region `CMapMgr*` local and one post-`idiv` local in each Bresenham arm
  recover the exact seven-reference sequence and the exact frame size.
- Scope control: declaring both Bresenham locals before their division lets cl
  common-hoist the load and emits only six references, disproving a shared
  cache. The two post-division declarations preserve retail's one load per arm.
- Result: candidate size falls from 2908 to 2856 bytes against retail's 2864.
  The referent multiset now differs only in the already-known duplicated
  path-list tail: `RemoveHead` 3/4 and `g_coordPool` 18/21.
- Score trap: a ten-form `strcmp`/guard matrix found direct comparison forms at
  33.2309%, but they delete retail's explicit `setne` boolean materialization
  and were rejected. The faithful `bool` form remains despite its 0% score.
- Exhaustion: of 64 requested parser-state trials, 56 were legal. All stayed at
  0.0000%; none changed the macro-region rotation. The residual is still the
  backward-`goto`/EH-scope wall documented in
  `backward-goto-sinks-its-target-region.md`, plus the initial inline-`strcmp`
  register choice. No probe declaration, include, or score-only guard was kept.

## 2026-08-12 — `CGameObject::Play`

- Unit/RVA: `wwdgameobject`, `0x00151150`; ordinary current-source result rises
  from the linear scorer's 0.0000% floor to 92.9655%.
- CFG break: retail has three notification call sites, not four. LOAD and
  POSTLOAD establish different action values and then share one notify/restore
  tail. PRESAVE falls directly into the middle dispatch block, while SAVE and
  the shared load tail jump backward to it. Modeling those edges removes the
  whole-stream rotation that had hidden the reconstructed body at 0%.
- Failure-layout break: spelling POSTLOAD's worker test as a positive gate puts
  the single failure epilogue after the shared tail, as in retail. Candidate and
  retail now both contain 139 instructions, 14 conditional branches, five
  returns, and the same ordered relocation targets.
- Controls: restoring separate LOAD/POSTLOAD tails falls to 73.0276% and 136
  instructions; returning directly from every switch arm expands to 155
  instructions; a flat zero-carrier guard falls to 91.5172% and moves the
  polarity mismatch to the preceding branch. Typed `AnimWorkerAct` accessors
  and direct MFC `Lookup` spelling are neutral at 92.9655%.
- Residue: retail preserves separate lookup-failure and zero-id null stores and
  uses saved/action registers `edi`/`ebx`; cl currently merges the null stores
  and chooses `ebx`/`edi`, leaving one branch polarity mismatch. This is a
  localized CFG/register-lifetime wall. No parser-state declaration, forced
  register, or score-only side effect was introduced.

## 2026-08-12 — `CBattlezMapConfig::StepDefenderUnit`

- Unit/RVA: `gruntstatestep`, `0x00033520`; full-build current-source result rises
  from 70.8105% to 77.5979%.
- Wall classification: local-lifetime and entity-creation-order wall. The
  candidate began with a `0x8c` frame against retail's `0x58` and only 28 of
  retail's 34 arithmetic shifts.
- Lifetime break: close each completed `Coord` group at its last semantic use.
  VC5 then reuses their stack homes and reduces the frame to `0x60`, without a
  fabricated aggregate, unused declaration, or forced spill.
- Entity-order break: each four-coordinate rectangle cluster is constructed in
  retail's bottom/right/top/left order, with each surviving component shifted
  and written back immediately after its `GetScreenPos` call. The first three
  objects retain both components; the final object retains only `m_x`. Applying
  the same `b3, b2, b1, b0` / `d3, d2, d1, d0` sequence restores all six missing
  `sar` instructions and raises the result from 75.0262% to 77.5979%.
- Controls: the candidate has 947 instructions against retail's 955 and all
  81 relocation targets remain in the same order. The frame remains eight bytes
  too large and the CFG/homing residue is not exact, so the function stays
  `@early-stop`.

## 2026-08-12 — `WarpTextureBlit`

- Unit/RVA: `imagepolyclip`, `0x00146a20`; current-source MAX rises from
  71.4791% to 71.7824%.
- Wall classification: induction-variable lifetime wall. Retail computes the
  common `minY * sizeof(ClipVtx)` contribution before locking both surfaces and
  later advances two independent edge cursors. The candidate declared each
  pair inside its mode arm after the calls, letting cl collapse them into one
  shared byte index.
- Lifetime break: declare both typed edge cursors before the two `Lock` calls,
  then guard each mode loop with `minY < maxY` and use its natural decreasing
  row count. This makes their address computation live across both calls and
  raises the result by 0.3033 points without a cast, forced spill, or invented
  datum.
- Strict controls: candidate and retail still have two returns, but candidate
  has 28 conditional branches against retail's 27. The remaining referent
  difference is an over-read of `_g_rasterEdgeL`: cl still folds one cursor into
  an index in each of the three mutually exclusive mode arms. The row-indexed
  alternative increased the array-reference mismatch and fell to 65.5978%, so
  it was rejected.
- Residue: retail uses a 0x28 frame and does not peel the first power-of-two
  test; candidate uses 0x20 and emits the peeled `test al,cl` plus a separate
  upper-bound branch. Four evidence-compatible loop spellings failed to remove
  that first CFG divergence. The partial lifetime break is retained and the
  function remains `@early-stop`.

## 2026-08-12 — `CBoomerang::LoadProjectileSprites`

- Unit/RVA: `projectile`, `0x000e0690`; current-source MAX rises from 73.2439%
  to 76.8618%.
- Wall classification: wrong floating-point entity followed by x87 scheduling.
  Retail converts unsigned `m_timePerTile` once, preserves that duration on the
  x87 stack, derives the reciprocal velocity scale from a duplicate, and later
  consumes the original duration when computing `m_holdWindowLo`.
- Semantic break: the source instead used the reciprocal `d` in the hold-window
  expression. Naming the unsigned duration, constructing `d` before the launch
  coordinate stores, and using the duration again in the hold-window expression
  restores the retail entity. This is behaviorally material: duration and its
  reciprocal are not interchangeable.
- Lifetime/frame break: assign `m_originX` and `m_originY` directly and derive
  the direction fields from the stored launch/origin members. Removing the two
  invented origin temporaries reduces the frame from 0x10 to retail's 0x8.
- Controls: candidate and retail have the same six conditional branches and two
  returns, and the data-referent audit reports no one-sided global. An explicit
  scaled-duration temporary fell to 71.4715%; a single `originY` temporary fell
  to 72.4146%. Both were removed.
- Residue: retail uses `fld st(0)` to duplicate duration before multiplying by
  `g_boomTimeScale`; cl loads the constants into deeper x87 slots instead. Four
  evidence-compatible entity/order forms bounded this scheduling wall. The
  corrected source remains `@early-stop`; the lower historical spelling is not
  restored merely to protect a score.

## 2026-08-12 — `CBoomerang::AdvanceMotion`

- Unit/RVA: `projectile`, `0x000e08b0`; current-source MAX rises from 73.9535%
  to 86.2481%.
- Control-flow break: retail tests `m_launched` first, uses the phase-0 compare
  for the initial snap, and reaches the phase-1 cleanup only when the saved
  launched state is nonzero. Expressing those as one `if` / `else if` chain
  restores all seven conditional branches, both returns, and every symbolic
  branch target. The previous nested condition plus `goto` let cl remove the
  second launched-state test.
- Semantic break: tracing retail's x87 stack proves that the motion is a
  rotation about `(m_originX, m_originY)`: the old phase supplies `sin` and
  `cos`, `m_dirY` is negated, and the two rotated components are written to
  `m_posX` and `m_posY`. Separately, retail advances
  `m_phase += double(g_frameDelta) * m_velScale`. The previous source scaled
  position terms by frame time and velocity, then assigned the computed X
  coordinate to `m_phase`; those are behaviorally different entities.
- Source-shape controls: removing the invented `px`/`py` locals raised 81.0078%
  to 83.9380%; moving the proven phase advance before the position assignments
  raised it to 86.2481%. Repeating the members directly collapsed the frame to
  0x8 and fell to 82.0388%; a named delayed `nextPhase` used a 0x10 frame and
  fell to 83.0000%. Both were rejected.
- Residue: retail uses a 0x20 frame and spills both rotated components before
  converting `g_frameDelta`; the retained equivalent source uses 0x18 and a
  different x87 spill schedule. The CFG and arithmetic entities are now
  settled, so this bounded remainder is classified as x87 lifetime/scheduling
  rather than reopened with unused declarations or score-only spills.

## 2026-08-12 — `CDDrawShadeBlit::ConvertRowDouble`

- Unit/RVA: `ddrawshadeblit`, `0x0014d950`; current-source MAX rises from
  69.5031% to the reproduced 73.5610% high-water.
- Wall classification: parser-visible TU state followed by register homing.
  Candidate and retail already have the same 24-block CFG, 12 conditional
  branches, five returns, and all 17 relocation identities in order. Ordinary
  compilation carries one four-byte spill slot that retail avoids.
- State break: a bounded 128-cell wall-identifier sweep found six independent
  member-declaration states at the same best class. A representative cell makes
  the complete `SHADE_DST_BY_SRC` arm block-for-block exact and reproduces the
  73.5610% current-source peak without changing any referent. The synthetic
  declarations are diagnostic only and were removed.
- Source-shape control: sequencing each alpha result as LUT0 followed by
  `LUT1 | LUT2` makes one 46-instruction loop exact in the ordinary build and
  both 46-instruction loops exact under the winning state. It also expands the
  ordinary frame to eight bytes and changes cross-arm alignment, falling to
  65.7012% ordinary and 70.0427% under that state. The flat, semantically
  equivalent expression is therefore retained.
- Residue: no tested state reaches exact, and the repeated best cells do not
  identify one missing semantic declaration. The remaining differences are
  register homes and LUT evaluation scheduling, not missing raster behavior.

## 2026-08-12 — `CDDrawShadeBlit::BlitShadedForward`

- Unit/RVA: `ddrawshadeblit`, `0x0014a200`; the ordinary current-source result
  rises from 72.7725% to 73.5407%.
- Type break: the RLE cursor is a nonnegative byte-array index bounded by the
  `u32 m_rleLen`. Retyping only `pos` from `i32` to `u32` produces the gain; a
  control that merely splits the original signed declaration remains exactly
  at 72.7725%, so this is a type effect rather than declaration-order state.
- Structural controls: candidate and retail each have 102 conditional branches
  and one return, but one branch still lands in the preceding block. The full
  block census is 196 versus 197, not the stale 197/197 claim. Candidate has
  1,835 instructions against retail's 1,797 and a 0x38 frame against 0x34.
- Rejected probe: a bounded parser-state sweep peaked at 73.7042% but did not
  close the function or repair its structural residue. No synthetic declaration
  was retained or banked. A staged `u32` LUT accumulator fell to 73.0153%, and
  narrow accumulator/store spellings changed retail's 32-bit OR into a 16-bit
  OR; all were removed.
- Residue: the first genuine block split is in the first 16-bit scratch loop,
  where retail reloads its scratch bias on the backedge. The candidate also has
  20 excess zero-extension `xor` instructions and under-reads `g_scratch` twice.
  This remains a source type/lifetime and register-homing problem, not a closed
  state wall.

## 2026-08-12 — `CBattlezMapConfig::Scan`

- Unit/RVA: `tilescan`, `0x00035f10`; current-source MAX remains 72.3025%.
- Entity correction: the source carried a file-local `GridLookup` clone even
  though the reconstructed class already owns the bounds-checked
  `CMapMgr::CellFlagsAt` entity with a retail COMDAT at `0x00075a40`. The call
  site now uses that real member and the clone is removed.
- Structural evidence: retail has 25 blocks and 17 conditional branches against
  the candidate's 21 and 15. The missing pair is retail repeating the same
  unsigned width/height guards inside the expanded cell lookup immediately
  after the caller's own guards. The member spelling compiles byte-identically
  to the clone, so VC5 still eliminates the repeated pair here.
- Verdict: the entity model is corrected without inventing a volatile access,
  fake alias, or redundant branch. The remaining shift CSE and late-inline
  boundary residue stays `@early-stop`; no parser-state probe was used.

## 2026-08-12 — `CGruntzCmdMgr::Serialize`

- Unit/RVA: `gruntzcmdmgr`, `0x00024890`; current similarity rises from
  79.9701% to 99.4012%.
- Control-flow break: retail performs the virtual command `Serialize` call
  inside both record-kind arms, immediately after allocating the concrete
  single- or multi-command. The source had merged that call below the arms.
  Restoring the duplicated branch-local calls raises the result to 91.7186%
  and reproduces retail's branch and register-lifetime structure.
- Lifetime break: load-side iteration and save-side command count are mutually
  exclusive uses of the same function-scope scratch. Modeling them as one
  `cursorOrCount` object removes the source's extra stack slot and reproduces
  retail's one-local frame, argument homes, and loop register allocation.
- Residue: the only remaining instruction difference is retail's redundant
  `mov ecx,edi` immediately before the proven exact global
  `__stdcall IsActive2(void*)` call. The call referent, argument, following
  member call, CFG, and every other instruction agree. No unused declaration
  or explicit register-state probe is introduced for that dead receiver load,
  so the function remains `@early-stop`.

## 2026-08-12 — `CLatencyList::FillCombo`

- Unit/RVA: `slotcombofill`, `0x00037ff0`; historical MAX remains 72.0132%.
- Structural agreement: candidate and retail perform the same list-count gate,
  dialog lookup, combo reset, list traversal, temporary `CString` conversion,
  add-string call, conditional item-data call, and final list-count return.
  Retail assigns `this`, combo, position, packed data, and item index to
  `edi`/`ebx`/`esi`/`ebp`/`edi`; cl rotates those values through
  `esi`/`ebp`/`edi`/`esi`/`ebx` in the candidate.
- Packing controls: direct, named-intermediate, and stepwise 32-bit spellings
  compile byte-identically. `MAKELPARAM` instead emits two partial-word writes
  absent from retail and falls to 68.0921%, disproving the apparent SDK-macro
  shape. An initialized result combined with that macro reaches 72.6842% only
  by removing a required saved register, so it is not retained as a score-only
  gain.
- CFG controls: positive nesting and function-scope declarations are
  byte-neutral. Assigning a result at each exit and spelling the shared retail
  epilogue with `goto` makes VC5 duplicate both EH epilogues and falls to
  63.4474%. The retained source has no synthetic state; the remaining whole-
  function register coloring and EH-epilogue merge stay `@early-stop`.

## 2026-08-12 — `CWwdGameObjectA::BltDirtyEx`

- Unit/RVA: `wwdgameobject`, `0x001506b0`; current-source MAX rises from
  73.7702% to 78.8385%.
- Type and lifetime break: the four-word blit scratch was modeled as an
  untyped `i32[4]`, although both of its uses are `CDDSurface::BltEx` source and
  destination rectangles. It is now a real `RECT` in each mutually exclusive
  branch. Retail likewise reuses one 16-byte stack range for those branch-local
  rectangles while retaining a distinct 16-byte intersection rectangle.
- Arithmetic evidence: retail preserves the union's named `x`, `y`, width, and
  height entities through the right/bottom calculation before constructing the
  blit rectangle. Direct field expressions, `POINT`/`SIZE`, and named scalar
  spellings compile identically here; the scalar names are retained because
  they state those proven entities without inventing aggregate storage.
- CFG control: explicit early returns between the both-armed, dirty-only, and
  shadow-only cases compile identically to the `else if` chain. Retail keeps
  separate call/return tails where VC5 merges the candidate's last two tails,
  so that residue is not reachable through this local control-flow spelling.
- Residue: the candidate still allocates the intersection and blit rectangles
  to the opposite stack halves in the both-armed path and chooses different
  callee-saved homes for the coordinate construction. No unused object or
  manual state probe is retained; the function remains `@early-stop`.

## 2026-08-12 — `CSBI_WarlordHead::ShowFrames`

- Unit/RVA: `sbi_warlordhead`, `0x000eb740`; historical MAX rises from
  95.1111% to 100% exact.
- Entity/lifetime break: the source invented a function-wide `cfg` cache for
  `m_frameSet`. Retail keeps `this` in `edi`, reads `m_frameSet` for frame 1,
  and deliberately reloads the member before resolving frame 2; the cached
  candidate instead kept the first pointer in `edi` across both halves.
- Fix: remove the cache and express both range checks and item lookups through
  the owning `m_frameSet` member. This reproduces the retail reload, register
  homes, and all instructions exactly without a state probe.

## 2026-08-12 — `CRandomAmbientSound::InitCycleTiming`

- Unit/RVA: `worldsoundset`, `0x0000cd70`; historical MAX rises from 74.6667%
  to 75.6923%.
- Lifetime break: `span` was declared before the four duration fields were
  initialized. Moving that derived value below the parameter-to-member stores
  makes VC5 load the duration arguments as one initialization group before it
  computes the play-duration range, matching more of retail's prologue.
- Structural evidence: candidate and retail each retain 11 blocks, four
  conditional branches, and three returns with identical symbolic branch
  targets. Both zero-span arms and the modulo arm write the same phase and
  countdown values.
- Residue: retail assigns play minimum/maximum to `ebx`/`ebp`; the candidate
  assigns the same values to `ebp`/`ebx`, which rotates the range calculation
  and both countdown stores without changing their meaning. No declaration
  permutation or manual state probe is retained for that register wall, so the
  function remains `@early-stop`.

## 2026-08-12 — `CBattlezMapConfig::CheckQueuedSpawnTile`

- Unit/RVA: `battlezspawncheck`, `0x00034c70`; historical MAX rises from
  75.1778% to 76.7778%.
- Aggregate break: the source split `m_arrivalCell` into independent `x` and
  `y` scalars. Retail loads both fields together and writes them to one adjacent
  eight-byte stack range before computing the tile address, which is evidence
  for a copied `Coord`. The source now models that object directly.
- Structural evidence: candidate and retail still have 20 blocks, 12
  conditional branches, and one return with identical symbolic branch targets.
  Tile selection, dwell tests, `TileSwitch`, both coordinate-list drains, and
  final state resets agree.
- Residue: this VC5 build eliminates the candidate's copied `Coord`, while
  retail retains both otherwise-dead stack stores, an extra zero register, and
  the corresponding eight-byte frame. The declaration-plus-assignment control
  compiles identically to copy initialization. No address-taking or manual state
  probe is added to force those stores; the function remains `@early-stop`.

## 2026-08-12 — rectangle type sweep

- Function/RVA: `CWwdGameObjectC::BltDirtyEx`, `0x001662a0`; historical MAX
  remains 76.1387%.
- Type correction: its four-word blit scratch is now a `RECT`, not an
  `i32[4]`. Every use supplies the same object as both source and destination
  rectangle to `CDDSurface::BltEx`, which proves the aggregate identity and
  field order. The typed spelling is byte-identical.
- Structural verdict: candidate and retail have the same eight conditional
  branches with identical symbolic targets. Retail has four physical returns
  where VC5 tail-merges the candidate to three; explicit early-return and
  flattened top-level controls compile identically and are not retained. This
  is a bounded tail-merge wall, not missing rectangle structure.
- Cross-file correction: `CDrawSubWorker::m_srcRect` is likewise now a real
  `RECT`. Its four fields are initialized as left/top/right/bottom and all
  consumers pass the complete object to DirectDraw blit APIs. The class remains
  size `0x30`, and the 52-TU compile produced no codegen change from the type
  repair.

## 2026-08-12 — `CBattlezMapConfig::Scan`

- Unit/RVA: `tilescan`, `0x00035f10`; historical MAX rises from 72.3025% to
  77.9664%.
- Inline-structure break: the source cached the center tile and `m_board`, so
  VC5 proved both bounds checks inside the inlined `CMapMgr::CellFlagsAt`
  redundant. Retail retains those two checks and has 17 conditional branches;
  the candidate had only 15.
- Source lever: retain the center in pixel coordinates, derive the four scan
  bounds independently, and access `m_board` at the two source call sites. The
  inlined accessor is now reconstructed in full, and candidate and retail have
  17 branches, two returns, and identical symbolic branch targets.
- Aggregate control: the four bounds describe a rectangle geometrically, but a
  `RECT` spelling scores lower and VC5 eliminates its storage. Retail's setup
  order instead supports four loop-bound entities, so the source keeps named
  row/column start/end scalars rather than fitting an unproven aggregate.
- Residue: retail preserves the four unshifted pixel-coordinate copies long
  enough to allocate a `0x14` frame; the candidate common-subexpresses the shifts
  and needs only one stack slot. This is now a lifetime/register wall after the
  missing inline CFG was restored; no address-taking or manual state probe is
  retained.

## 2026-08-13 — `CGrunt::LoadGruntCombatAnimations`

- Unit/RVA: `gruntcombat`, `0x000597a0`; current 73.672, MAX banked.
- Classification: DUP-EXIT, epilogue cross-jump class. Base emits 7 rets vs
  retail 6: two identical `return 1` epilogues (base +0x1024/+0x130e) that
  retail cross-jumps into one, at the tail of the four flat quadrant-probe
  arms (their `||` chains already share arm-internal targets).
- Negative controls: a four-option axes family over the welder/health exit
  cluster (baseline, `||` pair, goto-mid, goto-health-only) compiled BYTE-
  IDENTICAL (73.668180, size 5012, relocs 207/208 for all four) - that site
  is exit-merge-invariant and was the wrong suspect. The goto-fail pattern's
  own caveat identifies epilogue cross-jumps as scheduling-driven coin-flips
  its levers do not reach; a 150-iteration hill-climb moved 73.668 -> 73.672
  without flipping the merge.
- Verdict: structure complete (call set and probe threading retail-proven in
  the source comment); the remaining residue is the cross-jump plus register
  homing. Revisit with the IL tap if a front-end lever is ever suspected.

## 2026-08-13 — `CMultiBootyState::LoadGameAssetNamespaces` (signedness, blocked spelling)

- Unit/RVA: `bootystateactivate`, `0x0001d440`; current 82.49, proven +0.107
  reachable.
- Evidence: branch #41 is a SIGNEDNESS twin - base `jb`, retail `jl` - on the
  flag-cursor loop bound (`cmp cursor, g_bootyFlagPos+0x20`). Retail compared
  the cursor as a SIGNED integer address (the 1998 idiom). A
  `reinterpret_cast<i32>` respell of both compare operands flips the byte and
  scores 82.598 (sibling CBootyState untouched at 92.489).
- Blocked: the spelling costs +2 on the reinterpret_cast cleanliness ratchet,
  which is fatal and must never be blessed upward. Cast-free alternatives
  (ptrdiff compare, index loop) change the emitted compare shape. The site
  needs either a cast-policy ruling or a typed model for address-valued
  signed integers before the byte can be taken.

## 2026-08-13 — `CTriggerMgr::PlaceObjectFull` (mirror-direction exit split)

- Unit/RVA: `triggermgr`, `0x00078a50`; current 72.53.
- Classification: the goto-fail pattern's MIRROR direction at four sites -
  retail emits a full inline epilogue after each per-arm handler call
  (HitClick / LoadCursorSprites arms, distinct callees so the call blocks
  cannot merge), while our cl cross-jumps the four identical plain epilogues
  into one shared exit (base 69 branches/13 rets vs retail 81/16; base B27/
  B34/B46/B50 are 2i jmp stubs against retail's 12-21i inline returns).
- Source state: every per-arm call and return is present and correct; the
  divergence is purely cl's epilogue cross-jump coin, which the pattern doc
  records as unsolved in this direction (/Os refuted there).
- Verdict: bounded. Candidate future levers: the mirror-direction study the
  pattern doc calls for, or TU-state scheduling nudges; not a source-shape
  defect.

## 2026-08-13 — `CStatusBarMgr::BuildTabzDialog` (exit grouping, C2362 boundary)

- Unit/RVA: `sbi_rectonly`, `0x0010a340`; current 88.04 (same-harness), rets
  8 vs retail 6 - two merges, two distinct shared targets (B71/B84).
- Refuted: three-into-one goto (86.71, wrong grouping); adjacency pairs with
  `if (0) { label: }` devices (82.17, the device pollutes the shape);
  end-of-function labels (C2362 - the gotos cross initialized declarations).
- EVIDENCE from the failures: cl 5.0's C2362 forbids the merged-exit goto
  spelling unless the crossed locals are declared scope-top. Retail's merged
  exits therefore imply the original declared its CSBI_* locals at scope top
  (1998 declare-then-assign style). The productive next attempt is the
  declare-at-top restructure of the arm scopes FIRST, then the pair-grouping
  beam over legal label placements.
- CONTINUED (same day): the declare-at-top hoist of all 12 locals is proven
  BYTE-NEUTRAL (88.03716 exact, re-appliable mechanically). On the hoisted
  base, two further groupings scored and refuted: victory/else status-rsn
  pairs (84.59) and the tail statz-pair + observe/quit INLINE_CHAIN pair
  (82.11) - the latter after mapping base B44 to the OBSERVE arm via its
  SBICMD_DIALOG_PRIMARY (0x324) push. Four controlled refutations across
  every plausible grouping reclassify the site: the 8-vs-6 ret delta is cl's
  epilogue cross-jump coin (the PlaceObjectFull mechanism), not a goto/||
  spelling artifact. Bounded pending the mirror-direction study.

## 2026-08-13 — `CGrunt::PhaseStep` (first IL-tap-proven C2-side wall)

- Unit/RVA: `gruntphasestep`, `0x000f60f0`; current 82.85.
- Chain: 40-iteration hill-climb flat; 150 variants (AST depth 1 + 40
  TU-state trials) all byte-flat; the IL tap then closed the question from
  the other side - a +7-stride struct probe DOES reach the TU's IL (4/4
  streams, handle signature, causation verdict FRONT-END) yet the entire
  object delta is 7 bytes of symbol-table text (renamed $S ordinals) plus a
  .bss header artifact: ZERO .text bytes move, and PhaseStep's own section
  is untouched.
- Verdict: the residue is C2-ANCHORED - register/schedule state the front
  end cannot reach through declaration handles. This is the IL tap's first
  use as a NEGATIVE oracle (proving a wall is not front-end reachable), the
  complement of its designed purpose. Functions classifying here need
  C2-side levers (statement mass, expression shape) or stand bounded.
  Sharpened: PhaseStep is ALONE in its TU (no predecessor state exists),
  so its residue is the pure C2 allocation of its own tuple stream - the
  cleanest possible specimen for the eventual c2.exe allocator RE.

## 2026-08-13 — TU-state family scan (negative-oracle panel, first pass)

- Members scanned with the +7 struct probe + causation: gruntphasestep,
  font (Font.cpp), grunt (Grunt.cpp), playercommandstep. ALL show zero
  .text movement - the object deltas (7/103/357/7 B) are symbol-table text
  and .bss header artifacts only.
- CAVEAT (doctrine): one probe kind at one position proves only itself; the
  tu-state pattern's movers were MIXED-kind panels at both insertion points
  (ProbeFootSoft moved in 44/60 mixed states). A C2-ANCHORED verdict needs
  the full probe panel (typedef/enum/prototype/struct/class-with-member ×
  both positions, per the stride table). PhaseStep additionally has 150
  flat variants including 40 engine state trials; the other three carry
  only the single-probe result so far. Next pass: the panel run, mechanized
  through ilcap.py, before any of the three is declared bounded.
- PANEL COMPLETE for gruntphasestep (same day): all five probe kinds at the
  append position (typedef/enum/prototype/struct/class-with-member) move
  ZERO .text bytes (5-8 B symbol text each). With the 150 engine variants
  (mixed positions) also flat, PhaseStep's C2-anchored bound is sealed. The
  per-TU panel costs ~8 min through causation.py and is the standing recipe
  for the rest of the family.
- PANEL COMPLETE for font/Font.cpp (same day): all five kinds, zero .text
  movement (56-110 B symbol text each - the TU's many statics amplify the
  rename surface, none of it code). DrawWrapped's C2-anchored bound seals
  on panel + flat climb. Remaining panels: grunt, playercommandstep.
- PANEL COMPLETE for grunt/Grunt.cpp (same day): all five kinds, zero .text
  movement (235-432 B symbol text - the tree's biggest TU, biggest rename
  surface, no code response). XferName, StepGruntMovement's residue, and
  StepArrivalDrop's remaining this-rotation all classify C2-anchored here;
  their gains, if any remain, must come from C2-side levers (statement
  mass, expression shape) per the inline-budget model's calibration.
- PANEL COMPLETE for playercommandstep (same day): all five kinds, zero
  .text movement (3-9 B). THE FAMILY VERDICT IS IN: all four TU-state-
  suspected units are .text-immune to the full declaration-probe panel at
  the append position. Combined with each function's flat climbs, the
  entire register-residue family of this campaign wave classifies
  C2-ANCHORED. The tu-state pattern's movers (DDrawMgr/palette-era TUs)
  remain real - state-reachability is per-TU, and the oracle panel is now
  the 8-minute test that sorts any TU before a state sweep is attempted.
  Doctrine: run the panel BEFORE any declaration-sweep campaign.

## 2026-08-13 — `CPlay::ExecCommand` (switch-arm interleave, dossier)

- Unit/RVA: `playercommandstep`, `0x000d1b60`; current 73.998.
- The 11-slot jump table AGREES both sides (same targets). The 13 excess
  blocks are WITHIN the arms: retail shares exit tails across cases (case-1
  MOVE's handler at 0xd2783 is a `jne` target FROM the case-0 PLACE_GRUNT
  arm), so the cold sound-cue and ResetAll paths are hoisted to shared
  out-of-line tails. Ours keeps each arm's cold path inline.
- Not a per-arm spelling: the hoist is cl's whole-switch cold-block
  scheduling. Needs the forward-goto-hoists-target lever applied
  CONSISTENTLY across the shared-exit arms, or stands as a scheduling wall.
  Bounded; large correct-shape reconstruction, not a quick respell.

## 2026-08-13 — DUP-EXIT worklist fully triaged (no clean goto-fail targets remain)

Systematic sweep of all 11 sieve DUP-EXIT hits. NONE is a clean goto-fail lever:
- UpdateStatusBarTabHighlight 0xfe910, BuildTabzDialog 0x10a340 — switch-context
  cross-jump coin (BuildTabzDialog: 4 groupings refuted, C2362-bounded).
- ValidateUnitPath 0x29b40 — the stray return already compiler-merges
  (byte-neutral 85.033 both ways).
- ApplyTriggerA 0x6dae0 — MAX-held (blessed dip, do not chase).
- AdvanceAnim@CExitTrigger 0x3f5f0 (4->1) — already has `goto done; done: return
  0`; the 4 epilogues are /GX UNWIND funclets from the CString concat
  temporaries crossing the gotos (the pattern doc's excluded unwind-coin class).
- LoadGruntCombatAnimations, StepArrivalDefense, AdvanceToEnemyBase,
  StepBrickLayerBehavior, BuildSmall — epilogue cross-jump / register class.
VERDICT: the goto-fail lever is exhausted on this tree. Remaining DUP-EXITs are
the unwind-epilogue and cross-jump coins (unsteerable) — do not re-sweep.

## 2026-08-13 — `zPTree::FindOrInsert` (genuine structural-CFG target, dossier)

- Unit/RVA: `butetree`, `0x001933b0`; 68.72%. The RARE source-reachable kind:
  block skeletons 48 (ours) vs 52 (retail) = a real 4-block control-flow gap,
  NOT scheduling. diagnose masks it as jump-table truncation; the block diff
  shows the true divergence at B3-B11 (the crit-bit descent loop).
- Retail loop (0x193413): `lea eax,[edi+4]` computes &m_child[1] UNCONDITIONALLY,
  then `test ebx,ebx; jne; mov eax,edi` overrides with &m_child[0] (child[0] at
  +0, child[1] at +4); `mov edi,[eax]` loads the child. Our ternary
  `dir ? &m_child[1] : &m_child[0]` structures the select as a branch, adding
  blocks. Retail also threads m_candidateLeaf (+0x20) and m_descentCursor
  (+0x1c) stores INSIDE the loop at specific points; ours hoists/reorders them.
- The fix is a loop-body reconstruction: unconditional child[1]-address then
  conditional child[0] override (a select, not a branch), with the two
  candidate stores placed at retail's points. A full sitting - the crit-bit
  descent has ~11 interacting instructions per iteration to align.
- Verified NOT @early-stop and NOT C2-anchored (branch-count differs), so real
  source work lands here. This is the model of a source-reachable target after
  the diagnose-REGISTER/SCHEDULE screen: pursue ONLY these.

## 2026-08-13 — `CGrunt::StepArrivalDrop` (edge-map audit; tails corrected; sink still unbroken)

- Unit/RVA: `grunt`, `0x0004b370`. Mapped every backward edge into region A from
  the retail block graph: pathGate 0x4b4ff is entered by nudgeDone's bare `jne`
  and reProbe's threaded `je`/`jmp` pair; 0x4b605 / 0x4b787 / 0x4b78c are
  cross-jumped return tails. Retail FuncInfo maxState=1 (one CPtrList scope) —
  full-region source duplication is refuted; retail's source had OUR two-goto
  structure and cl still kept A inline. The sink trigger difference remains
  unfound.
- Structure recovered: both late commit tails return `arrivalPhase != 0` after
  storing `m_arrivalPhase` (retail tests eax and branches into the shared
  `mov eax,1`; fallthrough reuses eax=0). Ours returned constant 1 — a real
  behavioral divergence when arrivalPhase==0. Corrected in source; cl emits a
  shared `setne` block for every local spelling (if/return, ternary, `!= 0`),
  so the branch-vs-setne residue joins the rotation wall.
- Refuted levers (one build each): un-nesting A to top level
  (`if (SearchEdge==0) goto nudgeStart`) — still sinks; end-of-function
  `goto arrivalBail` block — hoisted to the goto site, opposite of retail's
  end placement.
- Current-source score sits at the linear scorer's 0.00 floor (rotation clamp);
  the 32.89 peak belongs to the pre-correction fingerprint. Blessed as a
  correctness dip. Next untested hypothesis for the sink: TU body-set parity.

## 2026-08-13 — StepArrivalDrop loop-family refutation (user-suggested for/while spellings)

- Three goto-free re-entry structures, one build each, all leave region A (the
  pathGate region) sunk behind B/C/D: (a) `for (;;)` with A at the loop top,
  entry `goto tryNudge` on search-fail — the forward goto HOISTS tryNudge's
  region (the hoist, not the sink, controls this variant); (b) `for (;;)` with
  an `if (havePath)` loop-top test, no goto anywhere, re-entries as
  `havePath = 1; continue;` — still sunk; (c) do-while(0)/break analysis: its
  source order is B-first, reproducing the rotated layout by construction, not
  retail's (not built).
- SHARPENED MECHANISM: cl takes A off the fall chain whenever A both (i) never
  falls through (every path returns) and (ii) has branch entries from below —
  independent of the construct family (goto, loop back-edge, structured flag).
  The doc's return-probe control (no below-entries → A stays inline) fits this.
  Retail kept A on the fall chain WITH below-entries, so the deciding input is
  outside this function's body: C2-side TU state. Next probe when tackled:
  TU body-set parity / preceding-function state in Grunt.cpp, via the IL tap
  as the C1-vs-C2 discriminator for the ordering decision.

## 2026-08-13 — `CGrunt::ScanNearestTarget` (two arm-placement fixes, 68.32 -> 69.31)

- Unit/RVA: `gruntscantarget`, `0x000f42f0`. The 2026-08-12 bound (68.28) fell
  to the goto-continuation arm-swap lever, applied twice from the retail block
  graph: (1) the powered guard battery nests under `if (neighborValid == 0)`
  with `m_neighborValid = 0; return 1;` AFTER the if (retail emits that arm
  out-of-line at 0xf477d after both 16i five-store rets); (2) case
  AISTATE_ATTACK's head inverts to `if (m_poweredUp == 0) { chase+500ms;
  return 1; }` with the big arm following unconditionally.
- Fix (2)'s emission is still re-normalized by cl: our head copy cross-jumps
  into the L_setLock copy (je far) while retail keeps two identical 9i blocks
  unmerged (0xf47b0 in-1, 0xf4a30 in-7). Merge-regime state, not source shape.
- Remaining residue: the atTarget join zeroes (`xor ebp` sunk per-predecessor
  vs retail's single join copy, +2i in B60), the frame word (0x44 vs 0x40),
  and the callee-saved rotation — register/schedule class. Blocks 396 vs 397.

## 2026-08-13 — `CDDrawShadeBlit::BlitShadedForward` reclassified C2-anchored

- Unit/RVA: `ddrawshadeblit`, `0x0014a200`, 73.54. The 2026-08-12 entry called
  the residue "not a closed state wall"; that predates the REGISTER/SCHEDULE-
  is-C2-anchored ruling. Fresh evidence: (a) the retail "string-op intrinsics"
  in the histogram (cmps/scas/stos/lods/lahf) are the tail jump table decoded
  as code — not real; (b) pre-guard hoisting of the loop invariants regresses
  73.54 -> 67.11 (the licm-placement pattern already proves in-body is the
  retail shape here); (c) the 16-bit loop pair diverges only in induction
  choice (ours up-counts and spills `s`; retail down-counts, keeps `s` in esi,
  spills the scratch bias and reloads it in a 1i backedge block). Same source
  statements both sides — C2 induction/allocation state. PARKED C2-anchored;
  the +20 xor / +37 add excess is this allocation difference across the ~13
  RLE loops, not a type defect.

## 2026-08-13 — `CGrunt::LoadPickupSprites` geo block-scoping 74.75 -> 75.84

- Unit/RVA: `gruntpickupload`, `0x00065e80`. The function-scope `CAniElement*
  geo` becomes a per-PICKUP-site block-scoped local (macro-internal + the
  megaphone arm's own). VC5 stack-colors the copies and the surrounding
  scheduling tightens: +1.09. The dead-4th-param home reuse (retail's `push
  ecx`-free prologue) still does not reproduce - our address-taken out-arg
  keeps its own slot; coloring INTO a param home is cl slot-assignment state
  (SaveRle16 precedent), not a source construct. Verdict stays
  REGISTER/SCHEDULE; type model now clean.

## 2026-08-13 — `CBattlezMapConfig::StepRowUnits` per-iteration hit reset (structure over score)

- Unit/RVA: `battlezmapconfig`, `0x000267c0`; 88.22 -> 84.89 current (blessed,
  hist banked). Retail zeroes the `hit` slot ([esp+0x3c]) TWICE: once deep in
  the guarded chain (our line ~1287) and once unconditionally per iteration,
  fused before the `if (unit != NULL)` guard (0x27b99). Ours lacked the second
  - so paths skipping the nest read a stale/uninitialized `hit` at the
  `if (hit == 0)` consumer. Inserted `hit = 0;` at body top level; the entry
  edge (#0 je) now targets the reset block exactly as retail (blk249).
- Remaining POLARITY row (#422, jl vs jge at the row-loop backedge) is a
  layout consequence: retail parks ~0x5EB B of out-of-line cold arms between
  the loop bottom and the exit block, so its backedge is jge-exit + jmp-top
  while ours falls into the exit. Cold-arm placement work, not a spelling of
  the loop itself.

## 2026-08-13 — LoadGruntCombatAnimations goto-fail A/B confirms the triage

- 0x597a0: converting the four quadrant-arm `return 1;`s to `goto` the final
  return (retail's single 0x5aad3 epilogue, 6 rets vs our 7) is byte-neutral -
  cl re-inlines the arm copies either way. The DUP-EXIT triage's
  "epilogue cross-jump coin" classification stands on direct evidence.
  Reverted; bounded.

## 2026-08-13 — META: one C2 block-placement/merge coin spans tonight's heavy rows

- Run@CGruntzMgr A/B seals it: nesting the body as the alloc-check's then-block
  reproduces retail's head EXACTLY (je-far to an end arm, init loop inline) but
  the later error arms then over-merge into the end copy (79.75, reverted);
  the original inline spelling emits row-1 while retail emitted the sunk arm
  FROM row-1 source (the allocate-check pattern's own warning, mirrored).
- The same signature, one session: StepArrivalDrop's region sink (construct-
  independent), ScanNearestTarget's unmerged twin 9i rets, StepRowUnits'
  cold arms parked below the row loop, ExecCommand's switch-arm interleave
  (prior dossier), LoadGruntCombatAnimations' 7-vs-6 epilogues, Run's error
  arm. All are C2 BLOCK PLACEMENT + CROSS-JUMP decisions where our compile
  and retail's diverge on identical-shape source.
- IMPLICATION: these are not N independent walls - one C2 mechanism (cold-arm
  sinking / tail-merge aggression) systematically differs. Finding its state
  input (c2.exe RE of the layout pass, or a TU-body-parity discovery) is the
  highest-leverage single investigation left on the heavy worklist; per-fn
  source spellings cannot reach it (proven across 6 constructs tonight).

## 2026-08-13 — `CPlay::ValidateLevelTiles` dossier (two named-local levers, unapplied)

- Unit/RVA: `leveltilevalidation`, `0x000d2dd0`; 85.87, weight ~1092. The
  outer do-while over m_childGroup diverges at its head: retail establishes
  ebp=1 in the guard block AND re-establishes it in a 2i backedge-only block
  (0xd2e42), and holds the 0x10000 flags-mask in ebx across the whole body
  (preheader load from [esp+0x14]); ours re-materializes both per iteration
  in caller-saved regs (loop-head `mov edx,1; mov ebx,0x10000` clobbered by
  every call). CONTENT class, not the placement coin: the fix is two named
  locals declared before the loop (the mask; and the per-iteration ok=1
  re-init actually spelled inside the body), used at the ~10
  `obj->m_flags |= 0x10000` sites - the register-residency evidence
  (callee-saved + spill home + backedge reload) is the scoped-local proof
  class (global-reload-runs pattern). CORRECTED same day: the use map refutes
  the named-local read - retail ors 0x10000 as an IMMEDIATE at 8 of 10 sites
  (ebx carries it once at 0xd4212) and ebp=1 feeds triple-stores/call args -
  the CGruntPuddle constant-pinning class, C2-anchored. PARKED.

## 2026-08-13 — `CRollingBall::Update` act/act2 else-path zero-init 83.94 -> 84.48

- Unit/RVA: `rollingball`, `0x000b0140`. Both `i32 act = 0; if (ok) act =
  VtblResolve(...);` pairs re-spelled with the zero on the else path
  (redundant-local-becomes-the-zero-register's fix shape): +0.54. The ebp=0
  pin itself SURVIVES (9 cmp-vs-ebp + 7 push-ebp sites) - the seeding local
  is still unfound; remaining candidates need the doc's one-at-a-time
  measurement. Guard-block +1i residue (mov/test vs cmp-ebp) persists until
  the pin dies.

## 2026-08-13 — `CGrunt::StepCombatReaction` goto-chain nested 84.07 -> 93.65

- Unit/RVA: `gruntentrancearrival`, `0x000646b0`. The flat `goto reject`
  guard chain (G/L/P probes) had cl pull the reject body up after its LAST
  predecessor with the probe inverted (the goto-chain-of-distinct-bodies
  mechanism, textbook signature). Nesting the three probes as `if (!eq) {`
  makes the reject body the join after the nest: +9.58 in one edit, and the
  diagnose verdict flips CONTROL FLOW -> REGISTER/SCHEDULE (5 drift rows
  left). The chain family (ActKey dispatch batteries) recurs across CGrunt -
  the same signature is worth screening on every strcmp-chain function.

## 2026-08-13 — `CBattlezMapConfig::RouteToNearbyEnemy` RVO-copy dossier (68.73, open)

- 0x2e3a0, CONTROL FLOW many-flips. The B57+ window: both sides call the
  Coord-probe helper via hidden-pointer return; RETAIL then copies 4 dwords
  through the RETURNED pointer (`mov edx,[eax]...`) into a SEPARATE
  destination object (frame 0x80 vs our 0x6c - 5 dwords more), and its
  rect-adjust stores a field pre-inc AND post-inc (copy-then-increment of a
  whole object). Ours aliases the temp and destination into one object and
  computes the inc before storing. This is the struct-return-rvo-idioms
  class: model the temp + destination as distinct objects, adjudicate the
  copy shape per site (variants beam). PARTIAL same day: the then-arm IS
  `a = *src; a.right++; a.bottom++;` (aggregate copy + re-stores) - applied,
  68.73 -> 69.35. Remaining: the else-arm's copy-through-returned-pointer
  (CRect ctor eax) and the frame 0x6c vs 0x80 - the four probe Coords' slot
  sharing sits BETWEEN our scoped (0x6c) and unscoped (0x88) spellings; a
  partial-sharing arrangement is the next experiment.

## 2026-08-13 — 330-440 weight band screen: the coin owns the residue

- Screened: ResolveArrival 0x2c690 (1 flip = backedge rotation, cold code
  parked before the exit), ArrivalReticleScan 0xee800 (1 "flip" = our jne+jmp
  split around an interloper block vs retail's single far je),
  StepArrivalDefenseAlt 0xf1c70 (2 flips, same family), LoadGruntDeathAnimations
  0x60150 (TOPOLOGY - unexamined, queued), SeekTarget/DefenseLean (jump-table
  truncation masks), RepathAroundBlockedTiles + SetEffectSpriteDurations
  (REGISTER). Every examined CONTROL FLOW verdict in this band decodes to the
  C2 block-placement coin, not to arm content. With StepCombatReaction's chain
  and the local-guard family closed, the source-reachable levers on the heavy
  worklist are exhausted down to weight ~330; the coin investigation
  (c2.exe layout pass / IL-tap discriminator) is the gate to the next tier.

## 2026-08-13 — `CGrunt::LoadGruntDeathAnimations` duplication model refuted once (dossier sharpened)

- 0x60150, 90.70, TOPOLOGY. The QUICKFALL sortKey guard's je (0x607e9) lands
  on a snap+value+ApplyGeometryDirect copy at 0x608fa; modeling it as a
  duplicated else-arm inside QUICKFALL over-duplicates (8 snap-ANDs vs
  retail 6) and drops to 85.40 - reverted. Correct model to test next: 0x608fa
  is a tail SHARED with the DEATH_FALL/DEATHBRIDGE arm (map its in-edges
  first); the QUICKFALL then-arm carries one inline copy (0x607ef) and the
  guard's skip path converges on the shared copy. Also retail orders the pose
  LookupValue BEFORE the guard (ours snaps first) - that reorder is part of
  the same sitting.

## 2026-08-13 — `CGrunt::LoadGruntDeathAnimations` shared-tail model LANDED (structure over score)

- 0x60150: the in-edge map (0x608fa in: both 0x357 guards) proves QUICKFALL
  and FALL's DEATHBRIDGE arm converge in source: QUICKFALL = tag pin, Lookup,
  sortKey guard, `goto fallSnap` into FALL's bridge arm at the snaps, sharing
  FALL's whole ending - including NotifyCell + LoadGruntMovingDeathConfig,
  which the old `goto finalize` spelling SKIPPED on the QUICKFALL path (a
  real behavior divergence, now fixed). Jump legality: FALL's grid/attr/tag
  became declare-then-assign; tag hoisted before the switch.
- Current 86.28 vs the old shape's 90.70 (blessed): retail's compile
  tail-duplicates [snaps+value+apply] into the first guard's then-arm (6
  snap-ANDs vs our 4, 56 vs our 54 branches) - the same C2 duplication/
  placement coin; our compile merges instead. The old peak was alignment on
  a structure the edge map refutes.

## 2026-08-13 — StepDefenderUnit shares RouteToNearbyEnemy's Clip-expansion residual

- 0x33520 (142 vs 134 blocks, ours +4 branches): the first divergence is the
  SAME expanded CMapMgr::Clip if/else as RouteToNearbyEnemy. Ours constructs
  `CRect b(0,0,w,h)` unconditionally BEFORE the src null-test; retail
  constructs the temp only on the ELSE path, IN the slot ([esp+0x48]) that
  IntersectRect later receives as &b, and copies `a` through the ctor's
  returned pointer. The then-arm aggregate+inc shape already matches. One
  investigation for both functions: RESOLVED same day - retail constructs b
  BEFORE the test too (0x33769, the same slot after push-accounting), so the
  GRID_CLIP macro's shape is already canonical (rb placement-ctor, aggregate
  ra = *src + right/bottom increments, else-arm temp). The +4-branch excess
  in StepDefenderUnit is NOT the Clip expansion: it lives in the four-Coord
  box-construction cluster upstream (b0..b3 GetScreenPos group, the old
  entity-order wall) - the bounded lifetime/homing residue. The placement-new
  devices in GRID_CLIP/STEP_BOUNDS stay flagged as cast-debt for the
  dissolution campaign, but their emitted shape matches retail.

## 2026-08-13 — `CGrunt::SeekTarget` local-guard lever 80.82 -> 85.14 (a "bounded" wall falls)

- 0xf71c0: the file's @early-stop comment declared the <STAMINA_FULL arm's
  folded re-tests "not reachable from source". The local-guard pattern reaches
  them: outer powered/neighborValid guards through locals, inner re-tests as
  members. +4.32 in one edit; comment retired. DefenseLean's similar-looking
  window is NOT the battery (PointInRect all four bounds present; residue is
  block fusion + one frame word) - bounded.

## 2026-08-13 — fold-signature sweep: UpdateArrival +4.01; ChargeStep rotation-blocked

- Bulk missing-branch screen (base branches < retail) over the top-90 rows
  found 7 candidates. `CGrunt::UpdateArrival` 0xf0130: its earlier locals
  attempt spelled the else-arm re-tests through the LOCALS (the copy-prop
  fold variant) - switching them to members lands 85.95 -> 89.96 (+4.01).
- `CGrunt::ChargeStep` 0xef6b0: adding the neighborValid local recovers 4 of
  5 missing branches (56 -> 60 vs 61) but triggers the region-sink coin:
  the layout rotates and the linear score collapses to the 0.00 floor
  (MAX 81.94 held). Reverted pending the placement-coin investigation - the
  lever is right but unverifiable at the floor. AdvanceMotion's 32-branch
  "gap" is jump-table decode artifact (blocks 90/90).

- Sweep addendum: `StepArrivalDefense` 0xf2b20 (missing 2) remains open - the
  dominated `else if (occ == NULL)` fold is the visible candidate but the
  grid-slot load census matches (4/4), so the kept branches are elsewhere;
  needs its own block-diff sitting. The bulk missing-branch screen itself
  (branches --diff over the weighted top-N, base<target filter) is the
  fastest fold-candidate finder and should run after every new lever lands.

## 2026-08-13 — placement-coin discriminator: the rotation is decided INSIDE c2.exe

- IL-tap A/B on ChargeStep's rotation flip (the one-line neighborValid local
  that collapses 81.94 -> 0.00): the captured C1 IL differs only by the new
  local's tuples and a +1 gl handle renumber (name sequence identical,
  1878/1878). The IL is a linear tuple stream carrying NO block-order
  decision - c2.exe takes near-identical IL and produces opposite layouts.
  VERDICT: the block-placement/merge coin is C2-internal state with
  hair-trigger IL sensitivity; no C1-side steering exists. The investigation
  narrows to c2.exe's layout pass (660,240 B, standalone program - the
  HoMM3-style RE now has a precise target and a minimal reproducer pair:
  chargestep_A/B one-line variants).

- Causation leg (same day): fed-A == plain-A and fed-B == plain-B byte-exactly
  (1872 vs 1936 text bytes), through the SAME c2 in ONE workdir. The layout
  decision is a pure function of the IL bytes - no environment or instance
  state. Next increment: the ex-stream splice bisect (needs the tuple framing
  from REPORT.md's normalization section) over the 329 diff clusters to
  localize the deciding bytes; ~9 feeds at binary-search pace. The
  chargestep_A/B pair and the capture/feed workdirs are the standing
  reproducer (scratchpad copies; re-derive from gruntchargestep.cpp +- the
  neighborValid local).

- Source-level bisect of the reproducer (same day): declaration-only (C) and
  an unrelated single-use temp of m_stamina (E) BOTH compile byte-identical
  to A - C1 copy-propagates single-use member temps away UNLESS the member is
  also STORED in the function (m_neighborValid's five-store blocks the
  forwarding; m_stamina has no store, so its temp folds). Consequences:
  (1) the deciding IL delta is precisely the neighborValid condition-operand
  tuple - the byte-splice targets ONE cluster family, not 329; (2) a NEW
  steering rule for the local-guard pattern: the lever only takes for members
  the function also stores - which predicts exactly the five-store battery
  family and explains every hit and miss to date.

- SPECIMEN DESIGNATION (same day): gruntchargestep is a SINGLE-FUNCTION TU
  (like PhaseStep) - so the reproducer pair carries no TU/body-set state at
  all. And the decisive contradiction: RETAIL emits B's guard set (61
  branches, the surviving re-tests) WITH A's layout (no rotation) - a
  combination our c2 produces from NEITHER variant's IL. Any correct model
  of c2's layout pass must explain all three points. ChargeStep supersedes
  PhaseStep as the c2-RE anchor: deterministic c2(IL), one-operand IL
  delta, no TU state, three-point constraint set.

- SPLICE TARGET ISOLATED (same day): variant C (dead local + member test) vs
  B (local test) ex streams share a 258,414-B prefix and 6,484-B suffix; the
  WHOLE deciding delta is one span at ex+0x3f16e - B: `45 a9 01 00` (load
  local 0x1a9); C: `3c a9 01 00` + `5a a0 43 33 41 80 1c 02 00 00 27 41`
  (dead-local marker + member load, the 0x21c m_neighborValid offset literal
  in-stream). ONE condition tuple (16B member-form vs 4B local-form) flips
  the 64-byte relayout. Next mechanical step: splice with gl ex-offset
  patching (gl records carry per-function ex offsets; a 12-B shrink shifts
  later spans), then the region-size-vs-operand-kind hybrid to name c2's
  actual threshold. The tuple opcodes read directly: 0x45 = local load,
  0x5a = member load - two more rows for the ex grammar.

- Threshold-model refinement (same day): variant F (redundant member
  conjunct) folds the OTHER way - cl keeps the member test, drops the local,
  59 branches, A's un-rotated shape at A's exact 1872 B. Net reading across
  A/B/C/E/F: the rotation engages when the always-returning below-entered
  region GAINS its 60th surviving branch (B) and disengages at 59 (A/C/F) -
  a block/branch-count threshold in c2's sink decision, with ChargeStep
  sitting exactly on the boundary. Retail's 61-branch un-rotated emission
  remains the constraint the count model alone cannot satisfy - the
  threshold must be relative (region size vs function size, or vs the fall
  chain), which the c2 layout-pass RE can now search for with the dispatch
  tables already extracted.

- Relativity REFUTED (same day): branch-carrying pads outside the region
  (k=2..64, function 1936 -> 3248 B, 60 -> ~124 branches) never disengage
  B's rotation - LCP-vs-B constant at every k. The sink decision is invariant
  to all function mass outside the region: not function-relative, not
  total-branch-relative. It is region-INTRINSIC - yet retail's region
  carries MORE surviving guards than B's and still lays out un-rotated, so
  a region-absolute count model fails too. The empirical space at source
  level is exhausted; the decision procedure must be read out of c2.exe's
  layout pass itself (dispatch tables extracted, reproducer minimal, all
  hypotheses to test enumerated: region tuple order, per-tuple weights,
  entry-edge classification).

- c2 interface map (same day): the flag table at c2.exe 0x99660-0x99778 is
  recovered (-dos -Fo# -Brepro -Fs# -Fa# -FA# -QI0f -QIfdiv -p6gj -noblend
  -nolock -noehopt -ehopt -bzalign -basic -nogen -MTd/-MDd/-MLd -loopopt
  -Loop# -EHa -EHs -Inl# -vol# -il# -isize# -ide -QIf). The /d2<flag>
  passthrough is PROVEN (bogus flag -> C1007 'unrecognized flag in p2').
  Toggling -noblend/-basic/-loopopt/-noehopt on the variant-B compile is
  byte-neutral: at /O2 the driver sends c2 NO optimization switches - the
  entire -Og/-Oi/-Ot/-Oy/-Ob1/-Gf decomposition goes to C1, so the
  optimizer state travels IN THE IL, and the layout decision is c2 code
  parameterized by in-IL state only. The toggle route to the sink is
  eliminated; the RE proceeds by disassembly of the layout pass proper.

## 2026-08-13 — near-exact cluster analysis: the exact-count ceiling IS the C2 coin

- To raise the EXACT function count directly (not fuzzy%), screened the
  [99,100) cluster (~100 fns, one fix flips each to exact). Composition by
  real diff rows (masked fs:/scas/reloc removed): 14 at ZERO rows, 22 at 2,
  18 at 4 - i.e. ~55 fns sit at <=4 real rows. Spot-checks of the top ~10:
  * imul/lea/mov COMMUTATIVE operand-load-order swaps (Save gridW*gridH,
    ProbeHeadSoft/HoldMove/ProbeColumn 0x60-vs-0x138 pair, CCheckpointTrigger
    lea [ecx+edx] vs [edx+ecx]) - the proven C2 load-order canonicalization;
    a source `a*b`->`b*a` flip is byte-NEUTRAL (cl re-canonicalizes).
  * the ZERO-row functions (RunFadeStepped, PlayMoveSound, the ButeMgr
    Get* family) diff ONLY in masked reloc blocks = DIR32 addend / referent
    identity against synthesized delink data, not codegen.
- VERDICT: the near-exact cluster - the most direct exact-count lever - is
  dominated by the SAME C2 operand canonicalization as the placement coin,
  plus delink-addend referent noise. The 81.1%-exact -> higher-exact path and
  the 94%-fuzzy -> 100% path are ONE gate: c2's operand/layout canonicalization.
  This unifies the campaign - every remaining tranche waits on the c2 RE, now
  characterized to a bounded disassembly problem. Confirmed source levers are
  exhausted across BOTH the heavy-weight and the near-exact ends of the list.

## 2026-08-13 — mid-band screen (weight 150-340), third confirmation of the ceiling

- Bulk-diagnosed the previously-unscreened 150-340 weight band (~28 CONTROL
  FLOW verdicts). Branch-count check: all but one are equal-count
  (TOPOLOGY/POLARITY = the placement coin or if-body-fallthrough), not
  count-mismatch structural. The one over-build (SetString 0x1732a0, 20 vs 18)
  is the inlined CButeValue CopyValue TYPE-SWITCH jump-table - C2 structure,
  the documented switch-overbuild caveat.
- The single clean-polarity candidate (WireTileSwitchLogic 0x6c130 #40) was
  A/B-tested: flipping the source `VerifyBlockLinksB()==0`/`SwitchDown()==0`
  guards to `!=0` LOWERS the score (92.43 -> 92.26 -> 92.21) and just shifts
  the polarity flag to the next site - the if-body-owns-fallthrough C2 arm
  choice from IDENTICAL source, NOT a transcription bug. Reverted.
- THREE independent screens now agree (heavy-weight, near-exact [99,100),
  mid-band [150,340]): the readily source-reachable structural/polarity wins
  are harvested; the residue across the whole distribution is the C2
  canonicalization/placement mechanism. The campaign's remaining path to
  higher exact-count is the c2.exe layout-pass RE, characterized and staged.

## 2026-08-13 — FULL-SET branch-count scan: the source-reachable surface is bounded at <=76

- Scanned ALL 720 sizeable (>=120 B) sub-100% functions for branch/ret count
  mismatch (the unambiguous source-reachable signal). RESULT: only 76 have any
  count mismatch; 644 are EQUAL-count = register/schedule/placement (C2).
- The 76 characterized by representative A/B: (a) the butemgr Set*/Get* cluster
  (~10 fns, all 18-vs-17) is the inlined CButeValue CopyValue TYPE-SWITCH
  jump-table overbuild - C2; (b) the ret-count mismatches are the exit-merge
  regime coin - FillCombo 0x37ff0 (2 rets vs 1) restructured to a single
  `result` local MATCHES the ret count (5/1) but LOWERS fuzzy 72.0->68.5
  because retail cross-jumps its two `return 0`s automatically from the
  three-return source while our C2 keeps them split - the goto-fail lever this
  tree already exhausted (DUP-EXIT triage). Reverted.
- DEFINITIVE: the readily source-reachable structural work across the ENTIRE
  binary is <=76 functions, and the sampled representatives are all C2
  switch/exit-merge coins. The exact-count ceiling at 81.1% is the c2.exe
  canonicalization/layout/merge mechanism, proven now by exhaustive scan
  from three directions (weight, near-exact, full branch-count).

## 2026-08-13 (late): EH-unwind-map campaign - the C1 fingerprint the code cannot show

User directive: match EH state transitions retail-vs-candidate as a worklist.
The mechanism (docs/patterns/eh-unwind-map-is-a-c1-fingerprint.md): maxState
counts C1 scope allocations; C2 deletes code but never map entries, so retail's
FuncInfo exposes dead TRACEs, destructible locals, and inline-expansion
decisions invisible in bytes. `gruntz.audit.eh_band` STATES had 10 rows;
scratchpad/eh_states_diff.py dumps any function's two maps side by side.

CLOSED rows:
- rezsync Run 40->42/42: dead TRACE with CString(a)+b nested temp pair (25,26
  NULL, never stored). Score ~neutral; map retail-proven.
- leveltilevalidation ValidateLevelTiles 22->23/23: trailing dead TRACE temp.
- gruntzmgr TransitionState 21->24/24, 86.42->93.22 NEW MAX, branches 21/21:
  entry TRACE's caller-cb mass re-enabled the DECLINED CCreditsState inline
  ctor (states 20-22 = CState guard, ~CRgn@+0x1e8, ~CString@+0x1f0; layout
  already correct). Sequence proof: retail stores 0x13,0x14,0x15 at push 0x218.
- spriteloaders CTimer: Init() WAS the ctor (retail delete-guard state at
  new CTimer + return-this + post-new null-check shape). ??0CTimer 100.00;
  caller +1.2.
- creditsstate ~CMoviePlayer 4-vs-5: NOT a defect in the image - the
  creditsstate.obj COMDAT copy is 5/5 and link order keeps it (unit 46 < 98);
  gruntzmgr.obj's 4-state copy diverges because its TU context declines one
  ~CObject-scope expansion (Mfc.h vs MfcWin.h axis). Audit reads an arbitrary
  copy. TU-parity clue for gruntzmgr only.
- keyedlist AddNode: code 100 + missing NULL state is UNRESOLVABLE together at
  154 B: every TRACE-temp spelling (by-value, CString(key), static_cast,
  comma) adds the lifetime-flag zero that flips the zero-reg heuristic
  (cmp/test + push edi); dead-after-return allocates nothing; lvalue-CString
  varargs is bitwise (no temp, no state) and clang refuses it anyway. Parked
  with the A/B matrix; map-only residue, does not affect fuzzy.

OPEN rows, each decoded to its mechanism:
- play LoadGameAssetNamespaces (12->13 ours vs 9): remaining 4 extra states =
  ours inline-expands ~CTileTriggerContainer in the delete m_beginMarker arm;
  retail calls the play-unit standalone 0xc8640. Budget: our CChatBoxOwner
  ctor expansion is missing statement mass (retail has an extra push <addr>
  after the 5x0x18 zero loop; loop anchored +0x228 vs our +0x230) - finish
  the ctor body, the budget then declines the dtor site.
- ddrawsurfacemgr Snapshot/RestoreChildren: ours expands ~CFileMemBase at 5
  sites (m_name ~CString member-destruction states surface at -0x14c);
  retail calls ??1CFileMemBase at all of them (the @early-stop comment knew).
  14 retail (base,CFile) dtor-phase pairs = 14 return sites. ChangeState-style
  free-site titration needed. CFile<->CFileMemBase census swaps are THIS.
- butemgr SetString 13-vs-12: map constrains the FindOrInsert site - retail's
  5th site has NO new-guard and its 6th is an expanded del+dtor TEMP pair,
  i.e. retail shape is FindOrInsert(key)->CopyValue(&CButeValue(...)), not
  our FindOrInsert(key, new CButeValue(...)). Feeds the FindOrInsert dossier.
- chatboxowner ProcessCheatInput 27-vs-26 + zErrHandling<->zPtrColl funclet
  swaps: undumped, next sitting.
- Frame-offset census rows (uniform frame-size fixes): multi
  LoadGameAssetNamespaces +0x8, BuildVoiceSoundList -0x4, AdvanceAnim +0x4,
  menustate +0x28, AddLogic -0x78, PathScan +0x20, SetRect/SetRange +0x4,
  DrawWrapped/MeasureWrapped mixed - each is one surplus/missing local.

META: the "C2 coin" verdicts on Run/TransitionState-class walls were
INCOMPLETE - the reachable set includes C1-input defects the unwind map
exposes. The placement mechanism itself still stands where maps already match.

## 2026-08-13 (cont): dead-TRACE titration is QUANTITATIVE + three oracle screens drained

- TransitionState 93.22 -> 93.92 NEW MAX: retail's image has NO
  ??0CMultiBootyState COMDAT anywhere => retail never declined that expansion.
  Titration: entry TRACE alone doesn't flip it; +2 more statement-units
  (distinct pre-switch and tail TRACEs) flip it exactly; +3 over-expands
  (89.76); a 4-arg TRACE is still ONE cb unit - cl's cb counts STATEMENTS,
  not arguments. COMDAT emission now matches retail.
- Screen 1 (drained): full-map (toState, has-action) comparison over all 750
  EH groups - ZERO equal-count structural diffs. The EH oracle's reachable
  surface is exactly the 7 count rows + the funclet-content census.
- Screen 2: COMDATs we emit that retail's image never kept - 27, of which
  real signals were CMultiBootyState (fixed), ??1CPen (fontconfig
  MeasureLabel local type), ??1CTileImageSet (gamelevel), ??3CDDrawSurfaceMgr
  (rezsync Run state 0); rest are the arrayserialize instantiation TU and
  Realize* devices.
- Screen 3 (drained): retail functions with no defining base obj - 0; retail
  COMDAT owner-unit vs our emitter sets - 0 divergences. Inline divergence
  is therefore entirely SITE-level inside callers where both sides emit -
  the parked titrations (play LoadGameAssetNamespaces sites-divisor,
  ddrawsurfacemgr ~CFileMemBase 5 sites).
