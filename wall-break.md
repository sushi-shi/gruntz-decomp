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
- Result: 58.7547% in the ordinary build, with 158 blocks on both sides and the
  same-tile and toy-count branch polarities restored. An eight-form quadrant
  topology sweep rejected independent-arm hybrids. A 16-form flag-width sweep
  scored higher for mixed widths, but retail's first inlining directly proves
  the original byte reads; the score-only dword forms were rejected. The
  remaining one-branch and register-colouring residue is marked `@early-stop`.

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
