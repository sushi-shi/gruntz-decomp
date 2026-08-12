# Wall breaks

This is the evidence ledger for walls broken while applying the HoMM3
`wall-identifier` doctrine to Gruntz. Entries follow the classifier order:
inliner, control flow, then register allocation. A score change alone is not a
break; the retail instruction, relocation, or table evidence must identify the
compiler decision, and a real VC5 build must confirm the fix.

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
