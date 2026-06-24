# Pointer-to-member-function call: define the class BEFORE the PMF typedef so it stays 4 bytes
tags: cpp:virtual cpp:member | asm:call asm:mov asm:add | topic:codegen-idiom topic:regalloc
symptoms: extra `mov ecx,[entry+4]` + `add ecx,this` before `call [entry]`; 8-byte vs 4-byte member-fn-ptr; `(this->*(e->m_fn))()`; first dispatch call diverges, a second (through a separately-loaded pointer) matches
confidence: 9/10

A registry entry whose first dword is a member-function pointer of a polymorphic
class, invoked `(this->*(e->m_fn))()`. MSVC sizes a pointer-to-member-function
from the class's **completeness at the typedef point**: a *complete*
single-inheritance class -> a **4-byte** code pointer, lowered to bare
`mov ecx,this; call [entry]`. An *incomplete* class (only forward-declared when
the `T::*` typedef is formed) -> the most-general **8-byte** representation,
lowering to an extra `mov ecx,[entry+4]` adjust-load + `add ecx,this` before the
call. So the same source diverges purely on declaration order.

```cpp
class CFoo : public CBase { ... };          // DEFINE the class FIRST (complete)
typedef void (CFoo::*Handler)();            // PMF now 4-byte (single-inheritance)
struct Entry { Handler m_fn; };
// ... (foo->*(e->m_fn))()  ->  mov ecx,foo; call [e]
```
```asm
; complete-class (4-byte PMF) — matches retail:
107: add  esi, eax          ; esi = entry
109: mov  ecx, ebp          ; this
10b: call dword ptr [esi]
; incomplete-class (8-byte PMF) — DIVERGES:
c3:  mov  ecx, 0x4(esi,eax) ; load the this-adjust word [entry+4]
c7:  add  esi, eax
c9:  add  ecx, ebp          ; apply adjust to this
cb:  call dword ptr [esi]
```
Steerable: move the `class` definition above the `T::*` typedef.
CGruntVoice::Dispatch (0x119e40) 93.7%->100% on the reorder; the working twin
CSecretTeleporterTrigger::FireActivation (0x042150, userlogic) already defines
its class before the typedef. Note a same-TU *second* call through a freshly
`mov`-loaded entry pointer can match even with the 8-byte PMF (the adjust folds
away when the pointer is reloaded), masking the bug to a single call site.
