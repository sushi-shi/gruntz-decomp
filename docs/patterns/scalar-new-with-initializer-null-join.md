# `new T(val)` NULLs the join register: `xor eax,eax` where a literal 0 gives an immediate
tags: cpp:new cpp:ctor | asm:xor asm:test asm:jcc | topic:codegen-idiom
symptoms: xor eax,eax; mov [esi+0x4],eax vs mov dword ptr [esi+0x4],0x0; duplicated `mov eax,esi; pop esi; ret` epilogue; test eax,eax; je after ??2@YAPAXI@Z
confidence: 9/10
variants: new-null-guarded-ctor-opened.md, const-materialize-into-reg-vs-immediate.md

A hand-written `p = new T; if (p) { *p = v; m = p; } else { m = 0; }` and a
`m = new T(v)` are semantically identical but lower differently. In the *new
expression* the NULL is the **value of the expression**, so cl materializes it into
the same join register the success path used (`xor eax,eax`) and duplicates the
epilogue; a source-level literal `0` is just a constant store (`mov dword ptr [..],0`).
Works for scalars (`new u32(v)`, `new double(v)`) and for class copies
(`new CButeValue(*src)`, whose implicit memberwise copy ctor inlines).

```cpp
// WRITE this:
this->pValue = new u32(val);      // or new double(val) / new CButeValue(*src)

// NOT this — same semantics, but the else-0 becomes an immediate store:
u32* p = new u32;
if (p) { *p = val; this->pValue = p; } else { this->pValue = 0; }
```
```asm
push 4
call ??2@YAPAXI@Z
add  esp,4
test eax,eax
je   SHORT $fail
mov  ecx,DWORD PTR [esp+0xc]
mov  DWORD PTR [eax],ecx          ; the initializer — only on the success path
mov  DWORD PTR [esi+0x4],eax
mov  eax,esi
pop  esi
ret  0x8
$fail:
xor  eax,eax                      ; <- the new-expression's NULL value, in the join reg
mov  DWORD PTR [esi+0x4],eax
mov  eax,esi
pop  esi
ret  0x8
```
STEERABLE. Flipped five butemgr functions at once (92→100 each):
`CButeValue::SetDword/SetInt/SetFloat/SetDouble` and `CButeValue::CButeValue(i32,CButeValue*)`,
all previously `@early-stop`-ed as a "const-materialize wall".
