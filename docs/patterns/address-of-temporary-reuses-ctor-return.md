# `push eax` straight after a ctor call means the source wrote `&Temporary()`

**Tags:** `cpp:ctor` `cpp:temporary` `cpp:inline` | `asm:call` `asm:lea` | `topic:codegen-idiom`
**Confidence:** 9/10 (the register choice is decisive; measured on nine `CButeMgr::Set<T>`)

## Symptom

A named local is constructed and its address passed to the next call. We emit a
fresh `lea` for the address; retail reuses the value the constructor returned:

```asm
; retail  0x173dd0 CButeMgr::SetPoint, the m_tree74 arm
mov  eax,[esp+0x30]      ; val
lea  ecx,[esp+0x10]      ; this = the object's slot
push eax
push 0x6
call ??0CButeValue@@QAE@W4ButeType@@PAUButeIntPoint@@@Z
push eax                 ; <-- the CTOR'S RETURN VALUE is the argument
mov  ecx,esi
call ?CopyValue@CButeValue@@QAEPAU1@PAU1@@Z

; ours, from `CButeValue box(BUTE_POINT, val); hit74->CopyValue(&box);`
call ??0CButeValue@...
lea  eax,[esp+0x10]      ; <-- re-materialised instead
mov  ecx,esi
push eax
```

## Cause

A `__thiscall` constructor returns `this` in `eax`. cl 5 only *uses* that return
value when the object is a **temporary** - for a named local it re-takes the
address. So the source was not

```cpp
CButeValue box(BUTE_POINT, val);
hit74->CopyValue(&box);
```

but the MSVC-only rvalue-as-lvalue idiom (warning C4238), ubiquitous in
1990s MSVC code:

```cpp
hit74->CopyValue(&CButeValue(BUTE_POINT, val));
```

Both destroy the object at the same point (the `return` follows immediately), so
the destructor placement does not discriminate - only the `push eax` does.

**A lifetime-extended `const` reference is NOT equivalent.** `const CButeValue& box
= CButeValue(...); ...CopyValue(const_cast<CButeValue*>(&box));` compiles but cl
treats it like the named local again (measured: it moved the /Ob1 expansion count
the *opposite* way).

## What it is worth

On `CButeMgr::Set{Int,Dword,Float,Double,String,Rect,Point,Vector,Range}`, three
sites each, this single change took the family from **784.52 to 823.59** and the
`butemgr` unit from 74.48 -> 87.55 over the two lanes. It also removed a whole
`/Ob1` expansion from the four scalar `Set<T>` (they had one more inlined ctor than
retail); see
[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md).

## Cost of adopting it

`&Temporary()` is a hard error in clang (`-Waddress-of-temporary`), and the build's
annotation reader, the record-layout dumper and the clangd DB are all clang. The
three flag sites (`gruntz.core.ir`, `gruntz.build.ghidra_metadata_generate`,
`gruntz.init.clangd`) now pass `-Wno-address-of-temporary`. That is byte-neutral -
clang never produces object code here.

## The other half: a payload struct built as a stack temporary

The same idiom shows up wherever retail assembles an aggregate argument. In
`ButeMgr::ParseAttributeFile` (0x170750) the RECT/POINT/VECTOR/RANGE arms build the
payload **on the stack** and hand its address to the ordinary pointer constructor:

```asm
push 0x18                 ; sizeof(ButeDoubleVector)
mov  [esp+0x44],edx       ; \
mov  [esp+0x48],eax       ;  } the three doubles, into a 24-byte stack temporary
mov  [esp+0x4c],ecx       ; /
mov  [ebx],0x7            ; type = BUTE_VECTOR
call ??2@YAPAXI@Z
test eax,eax
je   ...
mov  ecx,0x6
lea  esi,[esp+0x34]       ; src = that temporary
mov  edi,eax
rep movsd                 ; the pointer ctor's own bitwise copy
```

so the arm is `new CButeValue(BUTE_VECTOR, &ButeDoubleVector(x, y, z))`, **not** a
`CButeValue(ButeType, double, double, double)` overload. Four such overloads had been
invented (allocating `new i32[2]` / `new i32[4]` / `new double[2]` / `new double[3]`),
which also contradicted the destructor and `CopyValue` arms - those delete and copy
`BUTE_POINT`'s payload as a `ButeIntPoint`, not as an `i32[2]`. Deleting them and
giving the four payload structs their field-wise constructors took
`ParseAttributeFile` 46.00 -> 49.18.

**Rule:** an aggregate that appears as a *stack* image immediately before a
`operator new` + block copy is a temporary the source built inline; do not invent a
constructor overload that takes the fields.

## Related

[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md),
[`struct-return-rvo-idioms`](struct-return-rvo-idioms.md),
[`byval-arg-copy-ctor-inline-vs-converting-ctor-call`](byval-arg-copy-ctor-inline-vs-converting-ctor-call.md).
