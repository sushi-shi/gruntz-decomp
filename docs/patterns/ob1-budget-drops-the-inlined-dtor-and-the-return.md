# cl 5.0 drops an inlined destructor AND its `return` when the /Ob1 budget shallows

**Tags:** `cpp:dtor` `cpp:eh` `cpp:inline` `cpp:switch` | `asm:jmp` | `topic:wall` `topic:codegen-bug`
**Confidence:** 10/10 (reproduced in cl's own `/FAs` listing, minimised to two functions)

## Symptom

A `/GX` function with a destructible local whose destructor is an inline `switch` on a
member the constructor just set to a constant. The `return` inside that local's scope
generates **only the EH-state store** and then **falls through into the next statement**:

```asm
; cl /FAs listing, CButeMgr::SetRect
; 803  :             hit->CopyValue(&box);
        ... inlined switch arms, all `jmp SHORT $L19712` ...
$L19712:
; 804  :             return;
        mov     DWORD PTR __$EHRec$[esp+44], -1
$L19746:
; 806  :         CButeNode* g48 = static_cast<CButeNode*>(m_tree48.Find(tag));
        lea     esi, DWORD PTR [ebp+72]
```

No `operator delete`, no `??1`, no jump to the epilogue — control runs on into the
`else` path. `llvm-objdump -dr` on the base obj agrees; it is cl, not the delinker.

## How to spot it

`gruntz sema disasm <rva> --base` and grep the function's relocations for the
destructor:

```
python3 -c '...'   # per-function reloc census, or simply:
llvm-objdump -dr build/objdiff/base/<unit>.obj | grep -c '??3@YAXPAX@Z'
```

In `butemgr`, `SetInt`/`SetDword`/`SetFloat`/`SetDouble` have 3-4 `operator delete`
calls each and `SetRect`/`SetPoint`/`SetVector`/`SetRange` have **zero** — exactly the
four functions the band tracks at 49-56%. Retail's `SetRect` has one shared
`call operator delete` reached by both inlined `CopyValue` expansions.

## Trigger

The victim's own source is correct. Compiled as the **first emitted function of its
TU** (`cl /nologo /c /O2 /MT /GX` on a file containing only the headers + that one
function) `CButeMgr::SetRect` is correct AND its prologue is byte-identical to retail.
Adding **any** preceding emitted definition to the TU — a real function, a
`void UseNothing() {}`, a referenced `static int` — flips it. Bisected over
`src/Bute/ButeMgr.cpp`: cutting the file down to `GetRect` + `SetRect` still
reproduces; `SetRect` alone does not.

The mechanical difference is tail merging. When the two inlined `CopyValue` switches
share one arm block (both jump tables holding the same 9 targets — what retail does,
and what the single-function TU does), the shared tail carries the destructor and the
`jmp` to the epilogue. When cl emits two separate arm sets, the tail is lost.

## Do not

* Do not "fix" the source — extra braces around the local, an explicit scope, and
  every guard polarity produce the same output.
* Do not blame `/GR`, `/Ob2`, or `~ButeIntRect() {}`; all three were tested and are
  inert.

## Related

[[ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach]] measured the same family's
expansion census: retail expands the ctor at 3 of 7 sites (2 stack + 1 heap — a strict
prefix), `CopyValue` at 2 of 3 and `~CButeValue` at 2 of 3; cl expands 6 of 7 ctors,
2 of 3 `CopyValue` and both dtors — and the two it expands vanish. Verify the census
from `push` sizes at each `operator new`: retail `SetRect` is 3x`push 0x10`
(expanded ctors), 4x`push 0x8` (heap `CButeValue`), 2x`push 0x2c` (`CButeNode`).
