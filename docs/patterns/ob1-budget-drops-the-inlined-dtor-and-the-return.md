# Historical wrong-code probe on the superseded `CopyValue` source family

**Tags:** `cpp:dtor` `cpp:eh` `cpp:inline` `cpp:switch` `cpp:return` | `asm:jmp` | `topic:wall-refuted` `topic:codegen-bug`
**Confidence:** 10/10 for the compiler behavior; twice refuted as a retail source diagnosis

## Correction (2026-08-29)

Surviving NOLF source proves that retail's semantic boundary is the nested
`CButeMgr::CSymTabItem::operator=(const CSymTabItem&)`, with `break` in every
switch arm and one shared trailing return. Composed with the source-proven typed
union, that shared-return spelling emits the exact 0x172040 body and makes all
nine Set callers exact. The earlier per-arm-return `CopyValue` correction below
was another local solution inside the incomplete global-`CButeValue`/`void*`
source family. It is not the authored Gruntz source.

The wrong-code behavior measured by this document remains a valid cl 5.0
negative control. It demonstrates what the compiler does to that superseded
input, not what retail was compiled from.

## Correction (2026-08-14)

The compiler behavior below is reproducible, but the 2026-08-14 conclusion was
still scoped to the wrong source family. The then-modeled `CButeValue::CopyValue` owns one complete
return epilogue per distinct switch body. The candidate used `break` in every arm and
one shared `return this`; its standalone C2 body happened to remain 100%, hiding the
C1 distinction. Restoring per-arm returns makes eight `CButeMgr::Set*` callers exact
and closes SetString's 13/12 unwind map to 12/12 without any pragma. The old
first-optimized-function state and `inline_depth` experiments describe how cl treats
that wrong shared-return source; they are negative controls, not a retail workaround.

## Symptom

A `/GX` function with a destructible local whose destructor is an inline `switch` on a
member the constructor just set to a constant. The `return` inside that local's scope
generates **only the EH-state store** and then **falls through into the next statement**:

```asm
; cl /FAs listing, CButeMgr::SetPoint  (any preceding optimized definition present)
; 36  :                 hit48->CopyValue(&box);
        ... inlined switch arms, all `jmp SHORT $L19720` ...
$L19720:
; 37  :                 return;
        mov     DWORD PTR __$EHRec$[esp+44], -1
$L19714:
; 31  :         CButeNode* g48 = static_cast<CButeNode*>(m_tree48.Find(tag));
        lea     esi, DWORD PTR [ebp+72]
```

No `operator delete`, no `??1`, no jump to the epilogue — control runs on into the
`else` path. **This is WRONG CODE, not a size difference**: the reconstructed function
would leak the payload and then perform the sibling lookup as well. `llvm-objdump -dr`
on the base obj agrees; it is cl, not the delinker.

## The block-skeleton symptom is the SAME defect

`gruntz walls diagnose <rva> --asm` on the victims reports a large block
divergence (e.g. `base 70 vs target 52` for `SetPoint`) whose first row is a set of
short arms all `jmp`-ing to one tail where retail has a different shape. That is not an
independent "switch-arm" problem to attack separately:

* retail emits **two** 9-entry jump tables (one per inlined `CopyValue`) that hold the
  **same 8 arm blocks**, whose shared tail is `push <pValue>; mov [EHRec],-1;
  call operator delete; jmp <epilogue>` — i.e. the folded destructor plus the `return`;
* when the defect fires, cl emits **two separate arm sets**, hoists the EH-state store
  into the second set's arms, and the tail that carried the destructor is gone.

The arm duplication and dropped tail disappear together. In this historical
source family, per-arm returns avoid the defect; the 2026-08-29 correction above
shows that they are not the authentic source lever.

## The trigger, isolated

The old shared-return probe, compiled as the **first optimized function of its
compiland**, `CButeMgr::SetPoint`'s first 127 instructions are byte-identical to retail
— prologue, both `CButeValue` ctor expansions, the single shared arm set and the shared
`push ebx; call operator delete` tail included. What flips it (measured on a probe that
is the includes + one function, `cl /nologo /c /O2 /MT /GX /FAs`):

| preceding item | defect fires? |
|---|---|
| nothing | no |
| `void UseNothing() {}` | **yes** |
| 20 x `void UseNothingN() {}` | **yes** (identical output to one) |
| `static void f() {}` + a pointer to it | **yes** |
| `extern "C" void f() {}` | **yes** |
| a real sibling (`SetRect` before `SetPoint`) | **yes** (and `SetRect`, being first, is clean) |
| `#pragma optimize("", off) void UseNothing() {} #pragma optimize("", on)` | **no** |
| `int g_x = 5;` / `int g_x;` (data only) | no |
| `void f();` (declaration only) | no |
| the same content placed *after* the victim | no |

So the condition is exactly **"cl's optimizer has already run on a function in this
compiland"**. It is binary, not graded: one trivial preceding function is as bad as
twenty real ones.

## What does NOT reach it

Everything below was measured against the delinked retail obj with a masked
instruction-sequence diff; none restores the destructor.

* **Flags:** `/Zm50` `/Zm200` `/Zm1000` `/Ob1` `/Ob2` `/G3` `/G5` `/Gy` `/Gf` `/Ot`
  `/Os` `/Oy` `/Op` `/Gs` — all byte-identical to the default. `/Oa` and `/Ow` shift
  scheduling (they raise `SetRect`/`SetPoint` and leave `SetVector`/`SetRange` alone)
  but the destructor is still gone, and they crater `CButeMgr::Parse`, so they are not
  retail's flags either.
* **Pragmas:** `optimize("",on)`, `optimize("g",on)`, `optimize("",off)/on` *at the
  victim*, `auto_inline`, `inline_recursion`, `intrinsic`, `function`,
  `code_seg(".text")`, `code_seg()`, `component(browser,off)`, `warning(push/pop)`,
  `pack(push/pop)` — all inert. `optimize("",off)` wrapped around the *preceding*
  definition avoids the state, which is a diagnosis, not a fix.
* **`inline_depth` is the ONE pragma that is NOT inert** (2026-08-08). `inline_depth(2)`
  and above (incl. `255`) are inert, but **`inline_depth(0)` scoped to the victim, and
  `inline_depth(1)` written above the whole include block, both bring the destructor
  back** — see
  [`msvc5-inline-depth-zero-is-the-only-live-lever`](msvc5-inline-depth-zero-is-the-only-live-lever.md)
  for the placement rules and the whole sweep. Neither reproduces retail's 3-of-7
  expansion count. They are compiler-state probes only; no `inline_depth` workaround
  is retained in `src/Bute/ButeMgr.cpp`.
* **Source spelling:** extra braces around the local + `return` outside — bit-identical
  to the plain early `return`. An `if/else` chain with no `return`s, and a
  `goto done;` chain, both raise the masked-diff ratio a little but **still emit the
  fall-through into the sibling branch** — the branch is dropped whatever it is spelled
  as. Also tried and still defective: the **temporary** form
  `hit->CopyValue(&CButeValue(BUTE_POINT, val));` (which ends the payload's lifetime at
  the full-expression instead of at scope exit) and a **consumed return value**
  `CButeValue* r = hit->CopyValue(&box);`. Both make the census uniform across all nine
  (`new`=11, out-of-line ctor=2, in-body `??3`=3) yet still emit
  `mov __$EHRec$[esp+N],-1` falling through into the `m_tree48` lookup. In the
  defect-free (first-function) state the plain caller `return` spelling was closest.
  This panel never tested the independently evidenced `CopyValue` per-arm returns.
* **Header shape:** moving every `CButeValue` ctor out of the class body into
  `inline CButeValue::CButeValue(...)` definitions after the class produces a
  byte-identical obj for all nine `Set<T>`. MSVC 5.0 makes no in-class/out-of-class
  inlining distinction.

## How to spot it

`gruntz walls diagnose <rva> --asm` and count the destructor in the function's
relocations — the in-body ones only, i.e. skip the `mov eax, DWORD PTR $T…[ebp-4]`
unwind funclets:

```
llvm-objdump -dr build/objdiff/base/<unit>.obj | grep -c '??3@YAXPAX@Z'
```

In the refuted shared-return build, `SetInt`/`SetDword`/`SetFloat`/`SetDouble` had 3-4 in-body
`operator delete` calls each and `SetRect`/`SetPoint`/`SetVector`/`SetRange` have
**zero** (`SetString` has one of two). Those four are exactly the functions with the
biggest inlined ctor — `new T(*src)`, a copy-construction through a pointer parameter —
which is what pushes the function over whatever internal threshold the degraded state
lowers.

## Related

[[ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach]] recorded the other half of the
same historical residual: retail expanded
the ctor at 3 of the 7 sites, cl at 6 (7 when the function is alone). Measured
masked-diff ratio against retail, per function, current TU vs. the same function alone
in a TU:

| | current TU | alone |
|---|---|---|
| SetInt | 65.8 | 67.9 |
| SetDword | 82.0 | 75.0 |
| SetFloat | 68.9 | 75.6 |
| SetDouble | 82.5 | 76.4 |
| SetString | 80.9 | 76.8 |
| SetRect | 39.1 | **76.4** |
| SetPoint | 37.0 | **77.6** |
| SetVector | 49.4 | **73.0** |
| SetRange | 43.0 | **77.0** |

These measurements remain useful evidence for compiler-state sensitivity, but their
"nothing to trade" verdict is superseded by the per-arm-return reconstruction.
