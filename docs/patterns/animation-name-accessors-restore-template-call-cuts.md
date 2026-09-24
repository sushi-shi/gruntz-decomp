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

When shared-template restoration causes repeated constructor/error expansions
inside readers, inspect the semantic name/value accessor and SDK comparison
layers before altering the template's visibility. Compare the retained call
targets, not just total instruction count or fuzzy score. Compose a justified
helper boundary with the complete repeated guard before rejecting its first
dip; remove redundant forwarding layers and flat local probes afterward.
