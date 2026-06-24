# Storing a call result in a named local before `return x == 0;` emits the `setcc`-in-scratch form, not `neg/sbb`
tags: cpp:return cpp:local cpp:branch | asm:sete asm:test | topic:codegen-idiom
symptoms: retail returns a 0/1 bool via `xor edx,edx; test eax,eax; sete dl; mov eax,edx` (a scratch-register setcc) but your recompile emits the branchless `neg eax; sbb eax,eax; inc eax` arithmetic form
confidence: 8/10

For a leaf that returns `Fn(...) == 0` (or `!= 0`) as a 0/1 int, MSVC5 picks one of two
canonicalizations. A bare inline `return Fn(...) == 0;` folds into the branchless arithmetic
`neg eax; sbb eax,eax; inc eax` (see int-to-bool-normalize.md). But binding the call result to a
NAMED local first and returning the comparison of the local emits the **setcc-in-scratch** form:
`xor edx,edx; test eax,eax; sete dl; mov eax,edx` — the call result stays in eax, the boolean is
materialized in a separate register, then moved to eax. A `? 1 : 0` ternary on the inline call does
NOT flip it (still `neg/sbb`); the load-bearing detail is the intermediate local.

```cpp
long hr = m_8->vtbl->Unacquire(m_8);
return hr == 0;            // xor edx,edx; test eax,eax; sete dl; mov eax,edx
// vs.  return m_8->vtbl->Unacquire(m_8) == 0;   // neg eax; sbb eax,eax; inc eax
```
```asm
ff 51 20        call [ecx+0x20]   ; the COM/leaf call, result in eax
33 d2           xor edx,edx
85 c0           test eax,eax
0f 94 c2        sete dl
8b c2           mov eax,edx
```
STEERABLE. Evidence: CInputDevice::Unacquire (@0x134fe0) 68.9% → 100% on the named-`hr` local
(the inline `== 0` gave neg/sbb/inc). related: int-to-bool-normalize.md (the opposite, inline form),
seh-bool-return-canonicalize.md (the /GX setne variant).
