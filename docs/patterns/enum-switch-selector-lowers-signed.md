# A `switch` on an enum-typed selector lowers with SIGNED compares — retail's was unsigned

- **confidence** c10 (measured: 63 functions flipped to EXACT by one cast)
- **tags** `cpp:switch` `cpp:enum` `cpp:type` | `asm:jg` `asm:ja` `asm:jle` `asm:jbe` |
  `topic:codegen-idiom` `topic:regression`

## Symptom

A family of otherwise byte-perfect dispatch functions sits at the **same** sub-100
score (`97.86%` for the 12-branch act pump), and `gruntz walls diagnose <rva> --asm`
shows nothing but signed/unsigned twins:

```
 cmp eax,0x1d
-jg <tgt>          <- base (ours)
+ja <tgt>          <- target (retail)
```

`gruntz walls diagnose <rva>` reports exactly this per function - the branch
counts agree while the bytes do not:

```
SIGNEDNESS  97.86%  visualandmarkerlogicdispatch 0x0aa1e0  @_DispatchFrontCandyLogic
           12 branches, rets 1->1:  #0 jg->ja=dest  #4 jg->ja=dest  #7 jg->ja=dest
```

## Mechanism

MSVC 5.0 gives an `enum` whose enumerators all fit in `int` the type `int`, so a
`switch` on an enum-typed expression is a **signed** switch and cl builds the
binary-search ladder out of `jg`/`jle`. Retail switched on an **unsigned** value
(here `CDDrawWorker::EventCode()` read as `u32`), so its ladder is `ja`/`jbe`.

Nothing else about the function changes: same arms, same order, same jump targets —
only the condition family, which is why the diff is so small and so uniform.

## Fix

Cast the **selector**, not the case labels or the member:

```cpp
// keeps the named arms AND retail's unsigned ladder
switch (static_cast<u32>(rec->LogicEvent())) {
    case ACT_UNINITIALISED: ...
```

Enumerators still convert in the `case` context, so the naming survives intact.
Never retype the accessor's return or a parameter to fix this — that rewrites the
mangled name.

## Why it matters beyond one function

This is a **regression the enum-domain campaign introduced**. The retail-faithful
source had been `switch (static_cast<u32>(rec->EventCode()))`; `33e433fad` ("naked
numbers: LogicRecordEvent") replaced the raw key with a typed accessor and dropped the
cast, which cost **63 previously-EXACT functions** at once — the whole
`_Dispatch<Leaf>Logic` event-pump family plus `LOGIC_RECORD_DISPATCH`'s users.

Measured, restoring the cast at 21 call sites plus the one shared macro in
`include/Gruntz/LogicRecordHandler.h`:

| | before | after |
|---|---|---|
| tree exact | 3322 / 4290 | **3385 / 4290** |
| tree fuzzy | 89.08% | 89.11% |
| `jcc_sieve` SIGNEDNESS bucket | 71 | **8** |

**Rule:** any campaign that retypes a `switch` selector (enum domains, `i32`→named
type, an accessor swap) must re-check the affected functions with
`gruntz walls diagnose` before it lands. The pair shows it immediately; the
fuzzy% does not, because a
3-instruction flip in a 140-instruction function is a 2-point dip that reads as
regalloc noise.

Related: [`enum-domains.md`](enum-domains.md),
[`masked-diff-hides-branch-target.md`](masked-diff-hides-branch-target.md).
