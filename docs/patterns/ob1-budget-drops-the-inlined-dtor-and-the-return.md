# cl 5.0 drops an inlined destructor AND its `return` unless the function is the compiland's first optimized one

**Tags:** `cpp:dtor` `cpp:eh` `cpp:inline` `cpp:switch` | `asm:jmp` | `topic:wall` `topic:codegen-bug`
**Confidence:** 10/10 (cl's own `/FAs` listing; trigger isolated to a single binary condition)

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

`gruntz sema disasm <rva> --blocks --diff --lite` on the victims reports a large block
divergence (e.g. `base 70 vs target 52` for `SetPoint`) whose first row is a set of
short arms all `jmp`-ing to one tail where retail has a different shape. That is not an
independent "switch-arm" problem to attack separately:

* retail emits **two** 9-entry jump tables (one per inlined `CopyValue`) that hold the
  **same 8 arm blocks**, whose shared tail is `push <pValue>; mov [EHRec],-1;
  call operator delete; jmp <epilogue>` — i.e. the folded destructor plus the `return`;
* when the defect fires, cl emits **two separate arm sets**, hoists the EH-state store
  into the second set's arms, and the tail that carried the destructor is gone.

So the arm duplication is a *consequence* of the dropped destructor, and both disappear
together. Chasing arm order/merging from the source is wasted effort.

## The trigger, isolated

The victim's own source is correct. Compiled as the **first optimized function of its
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
  victim*, `auto_inline`, `inline_depth(255)`, `inline_recursion`, `intrinsic`,
  `function`, `code_seg(".text")`, `code_seg()`, `component(browser,off)`,
  `warning(push/pop)`, `pack(push/pop)` — all inert. Only `optimize("",off)` wrapped
  around the *preceding* definition avoids the state, which is a diagnosis, not a fix.
* **Source spelling:** extra braces around the local + `return` outside — bit-identical
  to the plain early `return`. An `if/else` chain with no `return`s, and a
  `goto done;` chain, both raise the masked-diff ratio a little but **still emit the
  fall-through into the sibling branch** — the branch is dropped whatever it is spelled
  as. In the defect-free (first-function) state the plain-`return` spelling is the one
  that matches retail (and is the shorter of the three), so it stays.
* **Header shape:** moving every `CButeValue` ctor out of the class body into
  `inline CButeValue::CButeValue(...)` definitions after the class produces a
  byte-identical obj for all nine `Set<T>`. MSVC 5.0 makes no in-class/out-of-class
  inlining distinction.

## How to spot it

`gruntz sema disasm <rva> --base` and count the destructor in the function's
relocations — the in-body ones only, i.e. skip the `mov eax, DWORD PTR $T…[ebp-4]`
unwind funclets:

```
llvm-objdump -dr build/objdiff/base/<unit>.obj | grep -c '??3@YAXPAX@Z'
```

In `butemgr`, `SetInt`/`SetDword`/`SetFloat`/`SetDouble` have 3-4 in-body
`operator delete` calls each and `SetRect`/`SetPoint`/`SetVector`/`SetRange` have
**zero** (`SetString` has one of two). Those four are exactly the functions with the
biggest inlined ctor — `new T(*src)`, a copy-construction through a pointer parameter —
which is what pushes the function over whatever internal threshold the degraded state
lowers.

## Related

[[ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach]] is the other half of the same
family's residual and is present in **all nine** `Set<T>`, defect or not: retail expands
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

The five without the defect are already at or above their alone-state value — for them
the "later" state is the *better* one, because it expands one fewer ctor. Retail is
neither state: it has the later state's expansion count *and* the first state's
destructor. Only one function per compiland can occupy the first slot, and source order
is pinned to retail RVA order, so there is nothing to trade.
