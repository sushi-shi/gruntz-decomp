# The /Ob1 expansion count is set by the callee's FRONT-END size — transcribing cl's own fold under-costs it

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

## Which callee is the lever — measured, not assumed

Three inline callees are expanded into each `Set<T>` (the ctor at up to 7 sites,
`CopyValue` and `~CButeValue` at up to 3 each). Only one of them can move the S4
decision, and **the caller's own size moves it the wrong way**:

| lever | effect on the out-of-line ctor count |
|---|---|
| **ctor** front-end size | monotone, reaches retail's 4 — **this is the lever** |
| `~CButeValue` front-end size | saturates one notch short until the caller shape is right; then needs 2x the ctor's cost |
| `CopyValue` front-end size | same |
| **caller** front-end size | **inverts** — a bigger caller gets MORE expansions (12 dead statements in `SetInt` took it from 2 out-of-line to 1) |

So MSVC 5's `/Ob1` rule is not "stop once the caller is big"; it is a growth
allowance that scales with the caller. Adding dead code to the *caller* to force a
callee out of line is therefore counter-productive, and any "the caller is missing a
statement" hypothesis is refuted by that inversion alone.

## Byte-neutral cost, weighed in dead-statement units

`DWORD dN = k;` in each ctor is the unit (`/O2` deletes it; the nine ctor COMDATs
stay byte-exact). Against that ruler, with the `&Temporary()` call shape in place
(see [`address-of-temporary-reuses-ctor-return`](address-of-temporary-reuses-ctor-return.md)),
every `Set<T>` reaches retail's 4/9 at **+3** and stays there through +6:

| ctor construct (all leave the 11 callee COMDATs byte-identical) | units |
|---|---|
| `T* p = new T(v); pValue = p;` (or `void* p`, or a temp for `t`) | 1 |
| `if (!pValue) { }` / `if (!pValue) { return; }` / a call to an empty inline member | 3 |
| out-of-class `inline` definitions, `this->`, `const` params, C-style casts, named locals in the `CopyValue` arms | **0** |

Constructs that reach the cell but are **refuted by the ctor bytes**: `pValue = NULL;`
before the allocation and an `Init()` helper (the null store survives — a call
intervenes, so cl cannot drop it), and `T* p = new T(v); if (p) pValue = p; else
pValue = NULL;` (a second `test` survives; the COMDAT grows 0x39 -> 0x43).

**The residue is 3 IR nodes that retail's bytes cannot name.** Only a real statement,
a branch, or an inline call carries weight — pure style carries none — so the missing
source is one branch, one inline call, or three statements. Nothing in the binary
distinguishes them, and a fitted choice would be a guess dressed as a model. Left
`@early-stop`.

## Related

[`ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach`](ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach.md)
(the superseded conclusion),
[`ob1-budget-drops-the-inlined-dtor-and-the-return`](ob1-budget-drops-the-inlined-dtor-and-the-return.md),
[`shared-inline-transcribed-once-per-call-site`](shared-inline-transcribed-once-per-call-site.md),
[`inline-switch-serialize-record-unroll`](inline-switch-serialize-record-unroll.md).
