# A vptr-stamping init that returns 0 (not `this`) is a void METHOD, not a ctor
tags: cpp:ctor cpp:member | asm:mov asm:xor | topic:codegen-idiom
symptoms: target `xor eax,eax; mov [ecx],&vftable; mov [ecx+N],eax; ret` keeps `this` in ecx, eax=0 at return, NO `mov eax,ecx`
confidence: 8/10

A standalone function that stamps a base vftable and zeroes a couple of members but
**returns eax=0 (not `this`)** and keeps `this` in `ecx` is NOT a real C++ `??0` ctor.
Modeling it as a ctor makes cl emit `mov eax,ecx` (copy this into eax to free ecx /
return this) and pin the zero in the *other* register, so the byte order diverges
(~73%). Model it as a plain `void` member instead: cl then keeps `this` in `ecx`,
materializes the zero with a bare `xor eax,eax`, and stamps + zeroes through ecx —
byte-exact. (The base-subobject init of a multi-level class is emitted this way: the
most-derived ctor/factory calls it, the base vptr store is its whole body, and the
caller ignores any return — so MSVC5 gave it a void signature.)

```cpp
// retail 0x00bb40: CRandomAmbientSound's CUserBase-base init (stamps vptr 0x5e70b4)
void CRandomAmbientSound::BaseInit() { // NOT a ctor — void, no return-this
    m_vptr = g_vtbl_CUserBase;         // mov [ecx],&vftable (DIR32, reloc-masked)
    m_4 = 0;
    m_3c = 0;
}
```
```asm
xor  eax,eax            ; zero stays in eax, this stays in ecx
mov  dword [ecx],offset ??_7Base   ; vptr stamp
mov  [ecx+4],eax
mov  [ecx+3c],eax
ret
```
STEERABLE. The inverse of comdat-inline-ctor-no-standalone (where a real ctor MUST
add `mov eax,ecx`): here the retail form has no `mov eax,ecx` because it is genuinely
void — so a void method matches it. Evidence: CRandomAmbientSound::BaseInit 0xbb40,
73%→100%.
