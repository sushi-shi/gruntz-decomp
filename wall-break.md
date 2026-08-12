# Wall breaks

This is the evidence ledger for walls broken while applying the HoMM3
`wall-identifier` doctrine to Gruntz. Entries follow the classifier order:
inliner, control flow, then register allocation. A score change alone is not a
break; the retail instruction, relocation, or table evidence must identify the
compiler decision, and a real VC5 build must confirm the fix.

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
