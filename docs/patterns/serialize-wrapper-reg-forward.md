# Thin forwarding wrapper pushes uninitialised callee-saved regs as args — no source form
tags: cpp:method cpp:call | asm:push asm:call | topic:wall topic:regalloc
symptoms: tiny `__thiscall` wrapper whose prologue is `push ebx;push ebp;push esi;push edi` with NO loads before the call, then `add esp,0x10`; the same 4 regs re-pushed for a second call; one arg re-read from `[esp+N]` for a `->m_7c` member
confidence: 7/10

A thin wrapper that calls `f(a,b,c,d)` then `obj->g(a,b,c,d)` can lower, in retail, to
`push ebx;push ebp;push esi;push edi; call f; add esp,0x10` with **no `mov reg,[esp+N]`
loads** — the four forwarded values arrive already in the callee-saved registers
(separate from the stack args). The wrapper still reads a *different* arg from the stack
(`mov eax,[esp+0x10]; mov ecx,[eax+0x7c]`) for the second callee's `this`. A standard
`__thiscall(i32,i32,i32,i32)` reconstruction MUST emit the four `mov reg,[esp+N]` loads
retail omits, capping the match (~38%). No natural C++ signature reproduces the
register-resident-args ABI here.

```cpp
// what you write (correct logic, but emits the arg loads retail lacks):
i32 Wrap(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (Curve(a0, a1, a2, a3) == 0) return 0;
    return ((Sub*)a3)->Serialize(a0, a1, a2, a3) != 0;
}
```
```asm
; retail — the 4 forwarded args are pushed straight from registers, never loaded:
push ebx ; push ebp ; push esi ; push edi
call   Curve
add    esp, 0x10
test   eax, eax
jne    L1
...
L1: mov eax,[esp+0x10] ; mov ecx,[eax+0x7c] ; push ebx ; push ebp ; push esi ; push edi ; call sub
```
WALL. Evidence: CBrickz::Serialize 0x09356c (56 B), ~38%; body logic byte-correct.
