# Exact C2 bytes can hide the C1 source shape that sets the /Ob1 expansion count

**Tags:** `cpp:inline` `cpp:ctor` `cpp:dtor` `cpp:switch` | `asm:call` `asm:jmp` | `topic:codegen-idiom`
**Confidence:** 10/10 (monotone response curve + the retail arm bytes; 25 measured cells)

## The claim this replaces

[`ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach`](ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach.md)
concluded that retail's "3 of 7 expansions" sits between two reachable cells with
nothing in it, because every *visibility* choice (all-inline / all-out-of-line /
`inline_depth` 0, 1, >=2) lands on an endpoint. That is true of visibility, and false
of the conclusion. **The budget is a function of the callee's PRE-OPTIMIZATION body.**
Hold visibility fixed, make the inline callee cost more in the front end, and the
expansion count walks down one site at a time.

Measured on `CButeMgr::SetPoint`, adding N dead `DWORD` statements to
`CButeValue::CButeValue(ButeType, ButeIntPoint*)` (the standalone COMDAT stays
**byte-exact at 100%** throughout — /O2 deletes every one of them):

| dead statements | out-of-line ctor calls | `operator new` | insn delta vs retail |
|---|---|---|---|
| 0 (our source) | 1 | 12 | +73 |
| 1-2 | 2 | 11 | +62 |
| 3-6 | 3 | 10 | +53 |
| 8-12 | **4 (retail)** | **9 (retail)** | +61 |
| 16-20 | 5 | 8 | +40 |

So the cell IS reachable. What was missing was not a lever but **cost**.

## Where the missing cost actually was: an arm-merged destructor

`CButeValue::~CButeValue` is an inline `switch` on the payload type, expanded into
all nine `CButeMgr::Set<T>`. Retail's out-of-line copy (`0x172160`) has a **9-entry
jump table at `0x1721b4` pointing at only 4 arm bodies** — cl tail-merged the eight
identical `delete` arms. Our header had transcribed that FOLD:

```cpp
case BUTE_STRING:                     delete (CString*)pValue; break;
case BUTE_DOUBLE: case BUTE_POINT:    delete (double*)pValue;  break;   // <- the fold
case BUTE_INT: case BUTE_FLOAT: case BUTE_VECTOR: delete (i32*)pValue; break;
case BUTE_DWORD: case BUTE_RECT: case BUTE_RANGE: delete (u32*)pValue; break;
```

Four arms where the dev wrote nine — and, because the payload types were erased to
`i32*`/`u32*`/`double*`, four *cheap* ones. Restoring one typed `delete` per
`ButeType` moved every function in the family at once:

| | arm-merged | nine typed arms |
|---|---|---|
| butemgr unit fuzzy | 74.4752 | **85.3231** |
| `SetInt` / `SetDword` / `SetFloat` / `SetDouble` | 74.2 / 80.0 / 66.2 / 81.3 | 80.4 / 89.4 / 89.5 / 89.3 |
| `SetRect` / `SetPoint` / `SetVector` / `SetRange` | 49.4 / 52.9 / 56.2 / 49.1 | 89.5 / 93.2 / 91.6 / 89.5 |
| `~CButeValue` | 99.30 | **100.00** |

## Two independent facts the retail arm bytes hand you

Read `gruntz sema disasm 0x00172160`. Only the `BUTE_STRING` arm null-tests the
pointer:

```asm
17216f:  mov esi,[ecx+4]; test esi,esi; je ...; mov ecx,esi; call ??1CString; push esi; call ??3
172188:  mov edx,[ecx+4]; push edx; call ??3; add esp,4; pop esi; ret     <- no null test
172196:  mov eax,[ecx+4]; push eax; call ??3; ...                        <- ditto
1721a4:  mov ecx,[ecx+4]; push ecx; call ??3; ...                        <- ditto
```

1. **The other eight payload types have TRIVIAL destructors.** `delete p` only
   null-tests when there is a destructor to run. `ButeIntRect`/`ButeIntPoint`/
   `ButeDoubleVector`/`ButeDoubleRange` therefore carry **no** `~T() {}` — and cl 5
   still emits the `$E` `atexit` helper for a function-local `static` of class type
   with a trivial destructor (the helper is then the bare `ret` at `0x173840` etc.),
   so the static-default `Get<T>()` functions stay byte-exact either way.
2. **Three registers for one body means three separately-generated arms**, folded
   afterwards. The fold is cl's, not the source's.

Giving those four structs an empty `~T() {}` scores *higher* on the nine `Set<T>`
(unit 88.78, `SetInt` 97.4, `SetPoint` 95.4) precisely because the spurious
destructors add front-end cost — and it drops `~CButeValue` 100 -> 57 by putting a
null test in eight arms retail does not have. **The lie scores better than the
truth**; take the truth and keep hunting the remaining cost.

## Rule

When retail shows a folded/merged construct (one jump-table target shared by several
case values, one tail shared by several arms), **write the unfolded source** — one
arm per case, each through its real type. cl re-folds it. Transcribing the fold is
not just cosmetically wrong: it changes the callee's inline cost and moves the
expansion count in **every** caller that inlines it.

Corollary for the census: `insn_seq --multiset` gives the count, but the count alone
is not the target — a shape can hit retail's exact `ctor`/`operator new` numbers and
still score worse (measured: a field-wise ctor variant reached 4/9 with the ctor
COMDAT at 23%). Match the count AND the arm bodies.

## The missing C1 shape was in `CopyValue`, despite its exact body

The ctor-cost panel above proved that C1 front-end mass controls the cutoff, but it
did not identify the original source. The retained answer came from a different
oracle: retail `CButeValue::CopyValue` has eight full return epilogues, one for each
distinct switch body. The old source assigned in each arm, used `break`, and returned
`this` once after the switch. C2 duplicated that shared epilogue in the standalone
COMDAT, so the function was already 100% and the source distinction was invisible in
its primary bytes.

Spelling `return this` in every arm keeps the standalone function 100% but changes
the inliner's C1 accounting. On a full build it makes `SetInt`, `SetDword`,
`SetFloat`, `SetDouble`, `SetRect`, `SetPoint`, `SetVector`, and `SetRange` exact.
`SetString` rises 75.0219% -> 81.7868%; its constructor/new census reaches retail and
its unwind map changes from 13/12 states to an identical 12/12 topology. Its remaining
three-call surplus is the string assignment/destruction/delete tail, not the former
constructor-cutoff mismatch.

The dead-statement and typed-pointer panels remain useful negative controls: they
show why byte-neutral C2 does not imply byte-neutral C1, but they are not admissible
source fixes. The source-selected lever is the independently visible per-arm return
structure.

## Related

[`ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach`](ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach.md)
(the superseded conclusion),
[`ob1-budget-drops-the-inlined-dtor-and-the-return`](ob1-budget-drops-the-inlined-dtor-and-the-return.md),
[`shared-inline-transcribed-once-per-call-site`](shared-inline-transcribed-once-per-call-site.md),
[`inline-switch-serialize-record-unroll`](inline-switch-serialize-record-unroll.md).


## Measured by the parallel lane (superseded spelling, but the boundary data stands)

| spelling | ctor COMDAT | expansions |
|---|---|---|
| `pValue = new T(x);` | 100% | 5 of 7 |
| `T* p = new T(x); pValue = p;` | 100% | **4 of 7** |
| …with `this->` qualification | 100% | 4 of 7 (identical obj) |
| …via a `void*` temp | 100% | 4 of 7 (identical obj) |
| …`type` in a member-init list | 100% | 4 of 7 (identical obj) |
| …declare-then-assign the temp | 100% | 4 of 7 (identical obj) |
| …two pointer temps | 100% | 4 of 7 (identical obj) |
| `type` stored *after* the allocation | **60-70%** | 4 of 7 |
| any null-check form (`if (p) … else pValue = NULL`) | **22-67%** | varies |

* **`default: break;`** on either inline `switch` — completely inert (identical census
  in all four cells; it only flips one unrelated function via ripple).
* **Routing the callers through the inline `Tags()`/`ModifiedTags()` accessors** (the `Get<T>`
  functions already do) moves the count the **wrong way**: 3/4 -> 2/4 on all nine. So the
  budget *grows* with the caller's front-end size — a bigger caller buys more expansions.
  Shrinking the caller — the half untried here — is since PROVEN and quantified on
  `CStatusBarMgr::AddTabItem` (dc842389b): N statements written out in a caller CREDIT
  its budget ~2N while the same N behind an inline call SPEND ~N, and an inline member's
  COMDAT is emitted iff some site declines. Re-confirmed by reverse-A/B 2026-08-17
  (reverting the 71 sites: 5 regressions, all confined to the TU, ctor COMDAT vanished).

**The general principle (all of the above are instances):** cl 5.0 decides inlining on
the C1 PRE-OPTIMIZATION form — of both callee (cost) and caller (budget) — never on
optimized size. A function inlining where retail declined (or vice versa) with matching
bytes everywhere else means a FRONT-END mass difference: statements /O2 deletes still
count. Byte-neutral C2 is not byte-neutral C1.

## Quantified (2026-08-17): the coefficients are one `add eax,eax`, and cb is computable

The "~2N credit / ~N spend" pair is not two empirical coefficients: it is
`budget = 2 * cb(caller)` at `0x00424938` in `c2.exe`. `cb` is a signed 16-bit
field read at `0x0042492f` (`movsx eax,WORD PTR [eax+0x64]`) and equals
`13 + SUM(per-statement cost)`, with an exact measured cost table (a store 7, a
call 9, a member store 10, a load+add+store 12, a `for` 32-33 — and a DEAD local
init 7, the same as a live one). Full spec, the c2.exe addresses for every
constant, cb of the real `CStatusBarItem`/`CSBI_RectOnly` ctors (55 / 61), a 7/7
predict-then-measure, and the per-builder mass deficits:
[`../relevations/cl5-inline-budget-is-arithmetic-you-can-compute.md`](../relevations/cl5-inline-budget-is-arithmetic-you-can-compute.md).
Direction correction for the sbi_rectonly builders: they need caller mass
REMOVED (~100-300 cb units), not added.
