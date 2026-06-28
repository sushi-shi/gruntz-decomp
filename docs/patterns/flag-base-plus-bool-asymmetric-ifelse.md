# `base + bool` call-arg held in a register: init + one-sided `if` (not ternary/symmetric if-else)

tags: cpp:branch cpp:local | asm:setne asm:mov asm:jmp | topic:codegen-idiom
symptoms: a call argument that is `base + (cond?1:0)` (base != 0, e.g. 0x10/0x11) compiles to
`xor eax,eax; test; setne al; add eax,base` where retail materialises it with a two-way branch
`mov eax,base; je .skip; mov eax,base+1; .skip:` and a single shared push of the whole arg list

Sibling of [boolarg-branch-push-not-sete](boolarg-branch-push-not-sete.md). That pattern is for a
**lone 0/1** arg, where retail pushes the literal directly in each branch (`push 0` / `push 1`) and
the fix is `if(cond) f(0); else f(1);`. THIS pattern is different: the value is `base + bool` with a
non-zero base AND the arg shares a multi-argument push list, so retail keeps the value in a
**register** (`mov eax,base; je; mov eax,base+1`) and pushes it once with the other args.

Three spellings all FAIL (all fold to `setne` because `base` and `base+1` differ by exactly one):
```cpp
f(..., mode ? 0x11 : 0x10);                          // xor;test;setne al;add 0x10  (~95%)
int flags = mode ? 0x11 : 0x10; f(..., flags);       // same — intermediate local still folds
if (mode) flags = 0x11; else flags = 0x10;           // symmetric if/else — STILL folds to setne
```

The fix is an **asymmetric init + one-sided override** — initialise to the base, then conditionally
bump:
```cpp
i32 flags = 0x10;
if (mode) {
    flags = 0x11;
}
f(dx, dy, src, &rc2, flags);          // -> mov eax,0x10; test; je; mov eax,0x11; ...; push eax
```
cl emits the two-way register branch (no `setne`) and the shared push list, fixing the
RECT-copy scheduling that the `setne`'s early `xor eax,eax` had perturbed too. Steerable. Closed
`winapi_115300_SetRect` (0x115300) 94.9% → 100%.
