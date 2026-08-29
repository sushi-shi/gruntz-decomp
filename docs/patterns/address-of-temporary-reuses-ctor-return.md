# `push eax` after a ctor proves a temporary, not the exact reference spelling

**Tags:** `cpp:ctor` `cpp:temporary` `cpp:inline` | `asm:call` `asm:lea` | `topic:codegen-idiom`
**Confidence:** 10/10 for temporary versus named local; surviving source selects the setter spelling

## Symptom

A named local is constructed and its address passed to the next call. We emit a
fresh `lea` for the address; retail reuses the value the constructor returned:

```asm
; retail  0x173dd0 CButeMgr::SetPoint, the m_tree74 arm
mov  eax,[esp+0x30]      ; val
lea  ecx,[esp+0x10]      ; this = the object's slot
push eax
push 0x6
call ??0CSymTabItem@CButeMgr@@QAE@W4SymTypes@1@PAUButeIntPoint@@@Z
push eax                 ; <-- the CTOR'S RETURN VALUE is the const-reference argument
mov  ecx,esi
call ??4CSymTabItem@CButeMgr@@QAEABV01@ABV01@@Z

; named-local control
call ??0CSymTabItem@CButeMgr@...
lea  eax,[esp+0x10]      ; <-- re-materialised instead
mov  ecx,esi
push eax
```

## Cause

A `__thiscall` constructor returns `this` in `eax`. cl 5 only *uses* that return
value here when the object is a **temporary** - for a named local it re-takes
the address. So the source was not

```cpp
CSymTabItem box(POINT_TYPE, val);
*hit = box;
```

The earlier reconstruction used this MSVC-only rvalue-as-lvalue idiom:

```cpp
hit->CopyValue(&CButeValue(BUTE_POINT, val));
```

That experiment reproduced the `push eax`, but it did not select the source
boundary. The surviving class supplies the authentic spelling:

```cpp
*hit = CSymTabItem(POINT_TYPE, val);
```

The temporary binds directly to `operator=(const CSymTabItem&)`; its address is
passed as the reference argument using the ctor's EAX result. Both source forms
destroy the temporary at the full-expression boundary, so the instruction proves
temporary versus named local, not pointer parameter versus const reference.

**A named lifetime-extended `const` reference is not equivalent.** It gives the
temporary a source-visible local identity and cl re-materializes its address.

## What it is worth

On `CButeMgr::Set{Int,Dword,Float,Double,String,Rect,Point,Vector,Range}`, the
temporary spelling was a major historical improvement over named locals. The
complete surviving typed-union plus const-reference assignment composition now
makes all nine callers exact. The old `&Temporary()` cell remains a useful
negative control, not the retained setter source; see
[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md).

## Pointer-taking aggregate constructors still use the MSVC extension

Gruntz retail's RECT/POINT/VECTOR/RANGE constructor ABI takes pointers, unlike
the later surviving reference boundary. `CButeMgr::Statement` therefore still
uses `&Temporary()` for those pointer-taking aggregate constructors. Clang's
annotation, variant, and compdb paths retain `-Wno-address-of-temporary`; clang
does not produce the retail object code.

## The other half: a payload struct built as a stack temporary

The same idiom shows up wherever retail assembles an aggregate argument. In
`CButeMgr::Statement` (0x170750) the RECT/POINT/VECTOR/RANGE arms build the
payload **on the stack** and hand its address to the ordinary pointer constructor:

```asm
push 0x18                 ; sizeof(CAVector)
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

so the arm is `new CSymTabItem(VECTOR_TYPE, &CAVector(x, y, z))`, **not** a
`CSymTabItem(SymTypes, double, double, double)` overload. Four such overloads had been
invented (allocating `new i32[2]` / `new i32[4]` / `new double[2]` / `new double[3]`),
which also contradicted the destructor and assignment arms - those delete and copy
`POINT_TYPE`'s payload as a `ButeIntPoint`, not as an `i32[2]`. Deleting them and
giving the four payload structs their field-wise constructors took
`Statement` 46.00 -> 49.18.

**Rule:** an aggregate that appears as a *stack* image immediately before a
`operator new` + block copy is a temporary the source built inline; do not invent a
constructor overload that takes the fields.

## Related

[`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md),
[`struct-return-rvo-idioms`](struct-return-rvo-idioms.md),
[`byval-arg-copy-ctor-inline-vs-converting-ctor-call`](byval-arg-copy-ctor-inline-vs-converting-ctor-call.md).
