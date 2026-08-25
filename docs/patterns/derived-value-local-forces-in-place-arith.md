# A named local for a DERIVED value makes cl compute IN PLACE (`sub`/`add`); the expression written at each use site stays a temp and gets `lea`
tags: cpp:local cpp:cse | asm:lea asm:sub asm:add | topic:codegen-idiom topic:regalloc
symptoms: `sub eax,0x22` where retail has `lea eax,[ecx-0x22]`; `add eax,0x170` where retail has `lea eax,[edx+0x170]`; the source member load lands in the SAME register as the result; the two feeding loads get re-ordered around the arithmetic
confidence: 9/10
variants: member-store-direct-not-via-temporary.md, single-use-local-register-knob.md

Naming a difference/offset (`i32 a = w - 0x22;`, `T* p = &base->arr[i];`) ends the
feeding value's live range at the arithmetic, so cl5 reuses its register and emits the
2-operand form. Written at each use site the value is an *expression temp*: cl CSEs it
into a FRESH register with a 3-operand `lea`, and the feeding loads keep their own
registers — which is also what re-orders the loads back into retail's schedule.

```cpp
// NO - `a` dies into w's register:  mov eax,[reg+0x8c] / sub eax,0x22
i32 w = g_gameReg->m_modeW;
i32 a = w - 0x22;
if (m_24 > a) { m_24 = a; }

// YES - the difference stays a temp:  mov ecx,[eax+0x8c] / lea eax,[ecx-0x22]
i32 w = g_gameReg->m_modeW;
if (m_24 > w - 0x22) { m_24 = w - 0x22; }
```
```asm
mov    eax,ds:0x64556c
mov    ecx,DWORD PTR [eax+0x8c]     ; both feeding loads FIRST, own registers
mov    edx,DWORD PTR [eax+0x90]
lea    eax,[ecx-0x22]               ; 3-operand: fresh destination
mov    ecx,DWORD PTR [esi+0x24]
```
STEERABLE. CStatusBarMgr::Activate 0x104dd0 93.58 -> 100 EXACT and
CStatusBarMgr::Deactivate 0x100cb0 97.21 -> 100 EXACT (both filed "scheduling wall:
retail computes m_8c-0x22 via lea"); the same lever on an array base
(`GruntzPlayer* player = &g_gameReg->m_players[i]` instead of a hoisted `mgr` local) was
one of the two fixes that took CMulti::Poll 0x0bba10 94.22 -> 100 EXACT.
