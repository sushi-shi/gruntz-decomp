# Animation-name accessors restore nested template call boundaries

tags: cpp:inline cpp:reference cpp:template cpp:macro msvc5:ob1 | asm:call | topic:inline-budget topic:source-model

Restoring a complete template can expose a missing abstraction in its callers.
The response is not to hide the template body or restore hand-expanded growth
fragments. In the action-name family, semantic accessor and comparison boundaries
recover many of retail's calls to `zDArray<CString>::operator[]` while leaving the
shared ZTools implementation unchanged. These game-facing helper names are
reconstructions, not recovered original source names.

## Controlled composition

The control is `dd6d439c2`, built with the pinned VC5 `/O2 /MT` toolchain.
Retail calls the typed name accessor 52 times in `StepRowUnits`, 11 times in
`FinishActiveAction`, and 10 times in `StepArrivalCommit`. The control expands
most of these into growth, error handling, and CString construction instead.

The retained source exposes three existing operations:

```cpp
// The name collection's public header:
inline const CString& GetAnimationActName(i32 id) {
    return g_typeColl[id];
}

// CUserLogic, which owns the current action record:
const CString& GetAnimationActName() const {
    return ::GetAnimationActName(m_logicRecord->m_eventCode);
}
bool IsAnimationAct(const char* name) const {
    return GetAnimationActName() == name;
}
```

The SDK's `operator==(const CString&, LPCTSTR)` delegates to `CString::Compare`
and then `_tcscmp`. It supplies a real library boundary, not a fabricated
comparison routine. References preserve collection ownership; no CString copy,
new field, cached action name, or changed lookup order is introduced. The four
existing action comparison macros delegate to the member predicate.

In Battlez, the repeated seven-key guard becomes an inline function. Its
`char&` result, assignments, short-circuit order, and I/G/L/P/J/C/R key order
are preserved. The otherwise redundant local single-name wrapper is removed.

| Real-TU composition | StepRowUnits | FinishActiveAction | StepArrivalCommit |
| --- | ---: | ---: | ---: |
| Control | 42.6102 | 28.6979 | 31.9180 |
| Shared name/comparison helpers, uniform previous-action lookup | 67.8630 | 56.8786 | 67.2734 |
| Add member name getter alone | 65.9052 | 56.8786 | 67.2734 |
| Compose seven-key inline guard | 69.9400 | 56.8786 | 67.2734 |
| Remove redundant comparison wrapper; previous-action sites use SDK equality | **69.9400** | **55.9659** | **65.6777** |

The member getter's initial dip was a useful composition base. In the final
source, StepRowUnits retains 26 typed-name calls, up from zero, and shrinks
from 4,731 instructions to 3,780 against retail's 3,370. FinishActiveAction
shrinks from 1,244 to 1,010 instructions against 792; StepArrivalCommit from
1,101 to 859 against 698. Neither the new name helpers nor the seven-key
helper survives as an extra out-of-line call in these three callers.

## Negative controls and limits

- Naming the existing Battlez helper's returned CString reference is flat.
- `CString::Compare` versus explicit `strcmp` is flat on the tested shared
  comparison base; replacing that wrapper with SDK equality is also flat.
- Using `EventCode()` instead of the field read does not increase StepRowUnits'
  retained typed calls and changes other cuts away from retail.
- Naming the reference inside the member predicate is flat. Naming its Boolean
  result reduces instruction counts further, but introduces an out-of-line
  `IsAnimationAct` call absent from retail. That is not a valid closure.
- Returning `const char*` instead of `const CString&` worsens all three;
  naming the converted pointer is flat relative to that control.
- A named array receiver and, independently, a named returned reference inside
  the free name getter are flat. Neither local is retained.

A disposable 25-site probe of the actual typed accessor expands nine sites and
rejects sixteen. Under the 1,000-floor assumption this bounds `cb` to [101,111].
It does **not** measure these large callers' budgets or prove their deficit.
An `/Ob0` census of their real TUs independently confirms the nested candidate
population: StepRowUnits has 24 direct predicates plus four seven-predicate
guards; FinishActiveAction has ten predicates and one previous-ID lookup;
StepArrivalCommit has nine predicates plus previous-ID and final name lookups.
No `/Ob0` object participates in the production score.

The first-pass callers still differ in inline/call set: they retain only 26/52,
2/11, and 4/10 of the target's typed-name calls. Ordered referents therefore
do not yet agree. These are partial structural recoveries, not exact matches,
proof of the original spelling, or exhausted walls. Do not steer register
allocation while these call boundaries remain unresolved.

## StepRowUnits follow-up: complete comparison phases

The next control is `4b03391e9`: 69.9400%, 26/52 typed-name calls, 3,780
instructions. Map each lookup by its following single-letter string relocation,
not just by the total number of calls. All 52 ordered keys agree with retail,
including the repeated C in the C/R/C/G/L/P/J chain. The missing calls comprise
three complete seven-check groups, three terminal R checks, and standalone D
and A checks.

Two distinctions matter beyond the name getter:

- SDK inequality is its own source boundary. `IsNotAnimationAct` uses
  `GetAnimationActName() != name`; spelling it as `!IsAnimationAct(name)`
  instead leaves equality materialization where retail has `setne`.
- The special-eligibility phase is not another short-circuit guard. Six checks
  clear an existing flag, but P exits StepRowUnits with zero. Later checks must
  still run after an earlier check clears the flag. Keep those operations and
  their order when extracting the complete phase.

| Controlled composition | StepRowUnits | Typed calls |
| --- | ---: | ---: |
| SDK inequality predicate | 72.2727 | 27/52 |
| Reuse the seven-key guard at its fifth site | 74.0600 | 32/52 |
| Remove unused comparison out-parameter/local | 77.0033 | 32/52 |
| Extract complete CRCGLPJ guard, without an unused char local | 79.0273 | 38/52 |
| IGLPJCR guard uses separate early exits | 80.1411 | 38/52 |
| Compose flag-update helper with complete eligibility phase | 84.1667 | 45/52 |
| CRCGLPJ guard also uses separate early exits | **85.3681** | **45/52** |

The flag-update helper alone is byte-flat. Composing it with the whole phase
restores all seven of that phase's typed calls. Flattening the same phase into
one helper with a reused char comparison local loses its final R call again
(44/52, 83.4340%). This is evidence for testing complete nested operations,
not for adding forwarding layers with no source-level responsibility.

The retained object has 3,417 instructions, 140 calls, 550 branches and seven
returns; retail has 3,370, 145, 529 and eight. No new game helper survives as an
out-of-line call. Seven typed lookups still expand: the CRC guard's final J,
four IGL guards' final R, and the standalone D and A. Six coordinate-list
GetNext and pool Push calls also remain expanded. These are still open
inline/call-set differences, not a bounded register-allocation wall.

Negative controls on this composition:

- A whole coordinate-recycle inline and a typed CoordNext accessor are flat;
  neither is retained.
- Char versus bool predicate returns is flat; bool stays the public domain.
- A named negative-predicate result adds no desired call and scores 85.1073%.
- A shared result/exit in the IGL guard is flat relative to its early exits.
- Including the five guards' common entrance/death/power flags introduces an
  out-of-line flag-update helper absent from retail (44/52, 83.7201%); reject it.
- The existing EventCode accessor is flat here and replaces the direct field
  read. That does not invalidate the first pass's different-context result.

The old caller-owned-char result is therefore not a universal requirement
after restoring the complete template family. Conversely, the fifth IGL guard
now shares an equality-based helper while retail materializes inequality at
that site. Equivalent conditions and a higher score do not prove that remaining
source spelling. Preserve this distinction when reopening the call-set work.

The follow-up full build reports 3,837 exact functions, 94.77% overall fuzzy
and 95.12% MAX. FinishActiveAction also moves to 57.1871%, StepArrivalCommit to
66.9022%. All 17 public-header controls pass; the raw near-exact audit covers
3,921 functions with no relocation defect. Banking preserves all 4,429
RVA-joined historical maxima and all 4,428 unchanged-source maxima. The shared
typed accessor and action registrar remain exact. Three fresh current-score
dips are adjudicated without lowering those banks: two unchanged functions
have matching semantic multisets and skeletons with register/schedule residue;
UseEquippedToolAt moves only 88.1407→88.1297 with unchanged ordered call
referents and store multisets. Its separate return-tail residue remains open.

## First-pass whole-tree control

The full build moves overall fuzzy from 93.71% to 94.52% and current exact
count from 3,830 to 3,835 (nine gains, four losses). All 16 fresh below-bank
rows have unchanged function fingerprints; the four exact losses are small
current-score changes with their source-scoped 100% banks preserved. Each of
those four has the same byte extent, instruction/call/branch/return counts,
semantic operand multisets, and ordered referents as retail; the first
divergence is register/scheduling, not a new call or store.

One much larger score dip requires more than that observation:
`StepArrivalDrop` moves 27.8033% to 0%. Direct objdiff confirms the same
named bodies are paired, not a missing function. The pair is 2,952 versus
2,856 bytes, 879 versus 853 instructions, and one return each. Semantic
comparison has no exclusive field/constant keys and identical 19-store
multisets. Its remaining call-set delta is a pool Push expansion and three
RemoveHead calls against retail's four; block placement also differs.
Objdiff aligns 204 instruction rows unchanged, but 513 as deletions and 540
as insertions, so zero is not a claim that the function contains no matching
instructions. The earlier [macro-origin control](macro-origin-can-perturb-later-c1-state.md)
already demonstrated the same zero-score/three-RemoveHead signature.
Keep the measured score and historical proof; do not restore raw name access
or add inert declarations to conceal it.

Validation: the full build and MAX gate pass after the adjudicated bank;
all 4,429 RVA-joined historical maxima and 4,426 unchanged-source maxima are
preserved. The shared typed accessor and action registrar remain exact.
The raw relocation audit covers 3,920 near-exact functions with zero defects;
17 public-header controls pass, including const-reference access and rejection
of mutation through the returned name view.

## Safe reverse use

### Complete action phases after the sourced array API

The next control is `0620b63b9`, after the complete array/error API restoration.
FinishActiveAction starts at 57.1871%, with 2/11 typed lookups; StepArrivalCommit
at 66.9022%, with 4/10. This is a fresh review, not an exemption derived from the
older tables. The reconstructed game helper names below are not original-source
identities.

Repeated retail consumers license three complete operations: restoring the
previous appearance, settling interrupted tube movement, and restoring the tool
after toy/vehicle use. Definitions belong in `Gruntz/GruntActionInline.h`, with
declarations on the real CGrunt class. None changes ZTools visibility or layout.
The applicable source/rejection queue is in `config/lithtech_lineage.tsv` under
`reassess-grunt-glp-*`, `reassess-grunt-tube-*`, and `grunt-glp-*`/`grunt-tube-*`.

| Real-TU composition | FinishActiveAction | StepArrivalCommit | Typed calls (Finish / Arrival) |
| --- | ---: | ---: | --- |
| Complete previous-appearance member | 62.4602 | 71.1036 | 1/11, 4/10 |
| Add early-return GLP predicate | 54.5398 | 0.0000 | 3/11, 4/10 |
| Compose complete tube-settle member | 58.4172 | 0.0000 | 3/11, 6/10 |
| GLP predicate owns a shared result | 58.4501 | 0.0000 | 3/11, 6/10 |
| Compose complete toy cleanup | 60.4412 | 0.0000 | 4/11, 8/10 |
| GLP macro retains caller result assignments | 69.1745 | 82.2345 | 4/11, 8/10 |
| Compose conditional tube-settle operation | **69.6296** | **83.0360** | **4/11, 8/10** |

The zero-score states are a useful composition example: actual instructions
shrink from 853 to 728 and desired typed calls grow from four to eight, but
the GLP predicate lets C2 place Arrival's final Q block before the O test.
Composing the complete cleanup first and then using the macro restores that
block's retail order without discarding the new phase helpers. A source-visible
macro is a legitimate boundary here; retaining a previously higher number by
removing the array API would not explain the caller.

The existing SetImageSetByName forwarding member versus the raw sprite-call
macro is flat in the first composition. Reusing EntranceCell, which owns the
complete direction copy, is also flat. Both final callers still load all three
direction fields and spill the otherwise unused third field. Moving the helper
bodies from the class header to the dedicated action header is flat. A
caller-owned GLP result assignment around the inline predicate is flat relative
to the shared-result predicate; it does not itself restore block order.

Final Finish/retail counts are 913/792 instructions, 49/42 calls, 118/97 branches,
10/10 returns and 101/80 relocations; Arrival is 759/698, 43/40, 112/101, 3/3 and
90/74. No new game helper or EntranceCell call survives out of line. The
conditional tube operation retains N's typed lookup, while another earlier
lookup expands instead: counts alone are insufficient. Finish still expands
I/G/L/P/O/J/K, Arrival O/J, and Arrival's final M expands get too far. These are
open inline/call-set walls, not exhausted register-allocation residue.

The sibling consumers remain separate required controls, including
RunEntranceMove's shallower lookup boundary and ambiguous D literal. The
whole toy predicate-plus-effect operation also remains open. This pass does not
close any of the sixteen broader source-helper reassessment families.

The full pinned build passes after auditing and banking three fresh
unchanged-fingerprint movements. GruntMachine Render and Boomerang AdvanceMotion
retain identical call/CFG, semantic multisets and ordered references; SpotLight
Update retains its calls, branches, stores and five references, with only FP
stack/load-store scheduling differences. All 4,429 historical RVA banks and
4,427 unchanged-fingerprint banks are preserved. The 23 real-VC5 header controls
pass, and the raw relocation audit covers 3,916 near-exact functions with no
defects. Overall current exact count stays 3,836; full-engine fuzzy moves from
94.69% to 94.74%, with 95.14% source-scoped MAX. This is partial recovery, not
merge readiness or a new historical high for either target.

### Complete pickup, knockback and bomb phases recover Arrival's call topology

The control is `747b1575d`: Finish 69.6296%, Arrival 83.0360%.
Fresh repeated-owner evidence licenses the pickup application, conditional
knockback settlement and conditional bomb termination. Canonical provenance,
scope and exclusions are under `arrival-j-*`, `arrival-o-*` and `arrival-m-*`
in the lineage ledger. The game-facing names remain reconstructed, not sourced
original names. All sixteen broad reassessments remain pending.

These are sequential real-TU controls, not independent percentage comparisons:

| Composition | Finish | Arrival | Important emitted feature |
| --- | ---: | ---: | --- |
| Complete conditional toy-use member | 63.2554 | 0.0000 | 5/11 and 9/10 typed calls; Q tail moves early |
| Add conditional J appearance member | 64.0657 | 0.3511 | EntranceCell survives as an extra call |
| Add complete knockback effect | 64.0657 | 75.5468 | Arrival gets ten typed calls; M still grows inline |
| Compose conditional knockback | 68.5752 | 68.6000 | Arrival's appearance helper declines |
| Compose complete bomb termination | 68.5752 | 79.0518 | Correct M get/construction; EntranceCell still a call |
| Conditional J macro | 65.4046 | 2.0763 | No extra game calls, but J/M lookup cuts swap |
| Shared-result conditional toy member | 65.0228 | 13.4561 | Q tail between O and J |
| Complete conditional toy macro | 65.7661 | 0.0000 | Correct M cut; Q still early |
| Separate toy guard and late cleanup tail | 74.1024 | 88.3957 | Only Arrival J lookup still expands |
| Complete pickup application member | 78.3451 | 80.8647 | Arrival tube effect declines; M becomes typed |
| Raw-name bomb predicate control | 78.3451 | 80.7050 | Same boundaries, different compare scheduling |
| Conditional tube macro | 78.4463 | 86.5410 | No extra game calls; Arrival M still typed |
| Complete conditional bomb macro | **78.4463** | **91.1094** | Arrival has every lookup at its correct depth |

The raw-name versus SDK predicate control is byte-flat on the earlier complete
bomb-member base, but not after the pickup composition. Do not transfer a
negative control across a changed caller/helper population without rebuilding.
Likewise, matching call totals can hide a wrong site: an intermediate Arrival
has ten typed calls and one get/construction, but expands J instead of M.
Pair by the ordered key and call topology, not just the multiset.

The selected source keeps the complete appearance, pickup, tube, toy and
knockback operations. Conditional J/N/M macros avoid adding another nested
member boundary; toy cleanup remains a late shared tail. No source visibility
restriction, template erasure, forced emitter or inert declaration is retained.
The inline-to-macro choices are scoped compiler controls, not proof that the
original authors used these exact spellings. Their reopening records are
`grunt-j-nested-conditional-control`, `grunt-n-nested-pickup-control`,
`grunt-m-nested-pickup-control` and `grunt-glp-complete-conditional-control`.

Arrival is now 0x843 versus retail 0x850 bytes, 691/698 instructions,
40/40 calls, 102/101 branches, 3/3 returns and 74/74 relocations. All ordered
references agree; FP, member-displacement, store and immediate multisets agree.
All ten typed lookups and the terminal M get/CString-construction are in the
retail positions. Its first divergence is an early zero-register setup at +8;
the remaining branch/predicate materialization difference still needs review.
This is a CFG wall, not an exact or bounded result.

Finish has 854/792 instructions, 45/42 calls, 106/97 branches, 10/10 returns
and 89/80 relocations. Eight typed calls remain; J, N and K still expand.
All 31 non-array calls agree in order, as do Arrival's 28. Both preserve the
three-field direction copy and the third-field stack spill; Arrival uses the
same +0x1c home, while Finish uses +0x28 against retail +0x24. The extra Finish
array expansions explain its three additional get/construction pairs, not a
new game operation. Sibling consumers and the complete conditional J operation
remain pending.

The three fresh unchanged-source gate movements were isolated with a disposable
include overlay containing the exact `747b1575d` Grunt class header, compiling
the same real TUs with `/O2 /MT /GX /GR`. It reproduces the previous scores:
UseEquippedToolAt 88.1407 versus new 88.1297, StepCompassMove 63.0219 versus
62.6506, and Timer AddTime 100 versus 99.8214. After normalizing compiler-local
symbols, each old/new pair has identical ordered references and identical FP,
displacement, store and immediate multisets. UseEquippedToolAt has the same
0x511 extent and 434/20/52/16 instruction/call/branch/return counts; its first
masked difference is +0x163. Timer retains 0xa3 and 57/0/5/1, first +0x73.
Compass changes 0xcf0 to 0xd08 and 866/22/129/2 to 874/22/131/2, first +0x242;
its decoded mnemonic delta is four moves and two jumps. This demonstrates
that a fresh branch-count change in an unchanged caller can follow class
declaration state. It does not resolve that caller's older retail CFG residue
or justify speculative source changes there. No overlay is in the build tree
or retained source, and no historical maximum is lowered.

Validation: full pinned build and every fast/normal gate pass after the
adjudicated bank. The raw audit checks 3,917 near-exact functions with zero
defects; all 23 action/container public-header tests pass. All 4,429 historical
RVA maxima and 4,427 unchanged-source maxima are preserved. Arrival's historical
maximum rises from 90.9914 to 91.1094; Finish's older 89.2642 high remains intact.
Full-engine current fuzzy is 94.78%, MAX 95.17%, with 3,838/4,428 functions exact.
The net two current exact gains are recoveries of existing historical exact
states, not two newly solved functions. This remains partial PR preparation.

### Caller-owned M comparison result on the complete macro base

After `2c5021e25`, the terminal M macro has the right get/construction cut but
branches directly on strcmp's result. Retail materializes the equality result
with `sete` and tests that byte. Passing the existing caller `eq` into the
complete macro restores that assignment without changing the handled result:
the manager's death-call return is still ignored. This raises Arrival from
91.1094 to 91.4820 in the real TU, restoring three instructions while preserving
all ordered referents, ten typed lookups, the M get/construction and every
value-level operand multiset. Finish is unchanged. The caller is now 0x84a,
694 instructions, 40 calls, 102 branches, three returns and 74 relocations;
retail remains 0x850/698/40/101/3/74. Its early zero-register setup and one extra
branch remain open. This control shows why the complete conditional macro must
preserve comparison-result ownership as well as side effects.

The full build and MAX gates pass with this follow-up banked at 91.4820.
The action-header test passes, the raw audit still reports 3,917 functions and
zero defects, and all historical maxima are preserved. No other current score
changes in this control.

### Complete entrance collision and commit responsibilities

The control is `19c1b9013`: Finish 78.4463%, Arrival 91.4820%, natural
LoadEntranceConfig 89.0307%. The fresh source/retail queue is `fk-*` in the
lineage ledger. The two entrance owners share a complete 130-byte masked
collision phase with the same ordered death-call relocation. The complete
postcommit phase also shares ownership, conditions, stores and call order.
These are reconstructed responsibility names, not surviving original names.

| Sequential real-TU control | Finish | Arrival | Natural entrance |
| --- | ---: | ---: | ---: |
| J macro preserves caller comparison result | 78.4336 | 91.4676 | 89.0307 |
| Compose N result ownership | 78.4336 | 91.4676 | 89.0307 |
| Complete J pickup macro | 78.4336 | 91.4676 | 89.0307 |
| Complete J member inline | 69.1454 | 78.3410 | 89.0307 |
| Correct natural current-player guard | 69.1454 | 78.3410 | 89.0526 |
| Shared collision member | 74.1454 | 78.3410 | 88.1360 |
| Compose complete postcommit member | 71.5107 | 78.3410 | 88.3289 |
| Complete J macro on composed base | 81.9532 | 91.4676 | 88.3289 |
| Complete postcommit macro | **83.1934** | **91.4676** | **88.3289** |
| Whole interrupted K effect member | 66.2174 | 91.4676 | 88.3289 |
| Whole interrupted K effect macro | **83.1934** | **91.4676** | **88.3289** |

The dipped J-member base was composed through both independently evidenced
K responsibilities before selecting macro forms. Intermediate Finish states
with all eleven typed lookups still contained unsupported out-of-line game
calls: inspect the entire ordered sequence, not just template-call totals.
The scoped member-form controls and reopening conditions are recorded only
in `grunt-j-complete-nested-control`, `fk-postcommit-inline-control` and
`fk-effect-inline-control`.

The selected composition preserves all phase abstractions. The collision
member is same-TU inline; the complete postcommit and interrupted-drop macros
stay in that TU, where the actual entrance animation-key datum lives. Shared
J/N operations remain in the action header. No copied static key, visibility
restriction, unused declaration or forced emitter is introduced.

Finish now has 9/11 typed calls: J is restored, N/K remain expanded. It is
0x97b bytes, 823 instructions, 44 calls, 103 branches, ten returns and 86
relocations against retail 0x936/792/42/97/10/80. Its frame is now the retail
0x14, including the complete direction-copy third-field spill at +0x24.
All 31 non-array calls remain in retail order; the only ordered-reference
replacements are the two expanded array sites. Arrival retains all ten typed
calls, terminal M get/construction and all 28 non-array calls in order, with
identical FP, displacement, store and immediate multisets. Natural entrance
retains 13 calls, 19 branches, one return and 24 relocations, and now has the
retail 0x0c frame. None of these functions is declared exact or bounded.

The guard correction is an important negative control on the diagnostic
method: equal counts and ordered referents did not establish reachability.
See [the branch-edge control](equal-counts-can-hide-a-side-effect-guard-defect.md).
All sixteen broad helper reassessment families remain open.

The fresh unchanged-source Spotlight Tick movement was isolated by compiling
its real TU with the exact `19c1b9013` Grunt header in a disposable include
overlay. That restores 84.2097 versus 83.2984 with the new declaration. Calls,
branches, returns and ordered references agree between the old/new objects;
displacement, store and immediate multisets agree too. The extent changes
0x2ec to 0x2ee and the instruction count 234 to 235; the only added opcode is
one x87 `fxch`. No overlay or inert declaration is retained. The natural
entrance store-multiset difference is separately accounted for: its sort-key
flag update is a memory `or [object+8],0x20000`, while retail uses load/or/store.
The update is present; it is not a missing field store.

Validation: the full pinned build and MAX/fast/normal gates pass after the
adjudicated bank. All 23 action/container header controls and both real-owner
guard tests pass. The raw referent audit finds zero defects in 3,917 near-exact
functions. All 4,429 historical maxima and 4,426 unchanged-source maxima are
preserved. Overall current exact count remains 3,838/4,428, fuzzy 94.79%, MAX
95.18%; this pass creates no new historical exact match. The canonical ledger
contains 470 rows, including 32 freshly source-verified candidates/decisions;
all sixteen broad reassessment rows remain pending.

## Typed occupancy operations restore Finish's remaining call boundaries

Starting from `9f66bec59`, reusing the existing const/u32
`CGruntzMapMgr::OccupantAt` in the shared collision phase is byte-flat. An
include-only control is also flat. The actual grid receiver is already derived;
there is no need for a duplicate accessor or a downcast.

The next missing boundaries are complete two-write occupancy operations:
release clears bit 29 then stores occupant -1; acquisition sets bit 29 then
stores the supplied packed identity. Their declarations belong to the real
shared map class and their inline definitions sit beside `OccupantAt` in
`MapCellInline.h`. This is a reconstructed API, not an externally recovered
original name. Canonical ownership alternatives, wider consumers and scoped
exclusions are recorded under `grid-*` in the lineage ledger.

| Real-TU control | Finish fuzzy | Typed lookups | Calls / branches / returns | Bytes |
| --- | ---: | ---: | ---: | ---: |
| Original complete entrance-phase base | 83.1934 | 9 | 44 / 103 / 10 | 0x97b |
| Existing bounded occupant getter | 83.1934 | 9 | 44 / 103 / 10 | 0x97b |
| Compose complete release member | 90.0493 | 11 | 42 / 97 / 10 | 0x924 |
| Same header, flatten release callers again | 83.1934 | 9 | 44 / 103 / 10 | 0x97b |
| Compose complete acquisition member | 91.0936 | 11 | 42 / 97 / 10 | 0x937 |
| Retail | 100 | 11 | 42 / 97 / 10 | 0x936 |

The declaration-only negative control is essential: the N/K recovery requires
actual operation calls, not merely an extra declaration. All 31 non-array
calls and the entire ordered reference sequence now agree with retail, with
no unsupported out-of-line helper. The release helper preserves the row-table
reload between stores and retail's add-before-AND shape. Acquisition likewise
reloads the row and snapshots identity members before the flag write. The
shift/OR still occurs earlier than retail, so this is not an exact scheduling
claim. Natural entrance improves from 88.3289 to 88.6316; Arrival remains
91.4676. Both callers retain their different pixel/tile sentinel guards.

Finish's first divergence moves from +0x22 to +0x53: the zero register now
agrees, but base tests the Boolean with `cmp cl,bl` where retail uses
`test cl,cl`. The remaining constant -1 lifetime, return-tail placement and
scheduling still need investigation. Equal call/branch/return counts do not
close the semantic checklist or the broader helper reassessments.

The release-only full build also provides an unchanged-source header control:
exact old-header overlays reproduce all eight fresh score drops. Six have
identical opcode/operand/reference multisets; Spotlight Update adds an x87
exchange, and BuildCellAttributes moves its EBP reload from loop header to
latch while removing the entry trampoline. Its jump-table offsets move with
the code, not with class layout. The overlays are disposable; only the used
member API is retained. Historical maxima are preserved by the real build.

The acquisition declaration causes five further fresh unchanged-source drops.
Release-only header overlays reproduce all five prior scores, including exact
SBI Image Render and ToggleToolTargeting. Ordered references, stores and
constants are identical in all five old/new pairs; four also have identical
opcode counts. ToggleToolTargeting adds one register copy while preserving
both coordinate arguments and all call/guard paths. WireTileSwitchLogic's
existing extra retail-relative call is unchanged by this declaration. This
separates a header-state perturbation from a new call-set defect.

Validation: full pinned build, MAX, fast and normal gates pass; 30 focused
action/container/guard/occupancy tests pass. The raw referent audit finds zero
defects across 3,918 near-exact functions. All 4,429 historical maxima and
4,428 unchanged-source maxima are preserved. Finish's historical best rises
from 89.2642 to 91.0935. Overall current exact count is 3,841/4,428, fuzzy
94.80%, MAX 95.20%. None of this closes the sixteen broad reassessments.

### Applying the evidence

When shared-template restoration causes repeated constructor/error expansions
inside readers, inspect the semantic name/value accessor and SDK comparison
layers before altering the template's visibility. Compare the retained call
targets, not just total instruction count or fuzzy score. Compose a justified
helper boundary with the complete repeated guard before rejecting its first
dip; remove redundant forwarding layers and flat local probes afterward.
