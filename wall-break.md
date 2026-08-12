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
