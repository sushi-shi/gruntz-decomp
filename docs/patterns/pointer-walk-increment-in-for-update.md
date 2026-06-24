# Pointer-walk loop: advance the cursor in the for-UPDATE, not via `*p++` in the body
tags: cpp:loop cpp:pointer | asm:push asm:add | topic:codegen-idiom

A counted loop that streams a walking pointer (`func(p, ...); p += 1`) where the
pointer is the call ARGUMENT: writing `func(p++, n)` in the body makes `cl`
materialize the old pointer into a scratch reg first (`mov eax,edi; push eax`) and
hoist the `add edi,4` BEFORE the call. Move the increment into the for-statement's
update clause (`for (p=base, i=0; i<N; i++, p++)`) so the cursor reg is pushed
DIRECTLY and advanced AFTER the call, matching retail.

```cpp
// WRONG: mov eax,edi; push eax; ...; add edi,4 (reordered before call)
p = base; for (i = 0; i < N; i++) f(p++, 4);
// RIGHT: push edi; call; add edi,4
for (p = base, i = 0; i < N; i++, p++) f(p, 4);
// nested: keep the increment on the INNER update
for (r = 0; r < 4; r++) for (c = 0; c < N; c++, p++) f(p, 4);
```
```asm
mov  edx,[esi]          ; reload vtable each iter
push 0x4
push edi               ; the cursor, pushed directly
mov  ecx,esi
call dword ptr [edx+0x2c]
add  edi,0x4           ; advanced AFTER the call
dec  ebx
jne  short ...
```
Steerable. Closed every Read/Write streaming loop in CBattlezData::Serialize
(40.6%→86.9%→92.4% across this + the unroll/branch-order fixes); residual is the
shrink-wrapped prologue (see shrink-wrapped-callee-save-push.md).
