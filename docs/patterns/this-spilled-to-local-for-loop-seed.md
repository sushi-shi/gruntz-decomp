# `this` spilled to a fresh stack local + reloaded to seed a loop pointer (vs a register copy)

tags: cpp:loop cpp:local cpp:method | asm:push asm:mov | topic:wall topic:regalloc

symptoms: a small `__thiscall` whose body is otherwise byte-identical opens with an
extra `push ecx` (a 4-byte frame slot holding the incoming `this`) and later does
`mov edi,[esp+0x10]` to seed a loop/cursor register with `this`; your recompile
allocates NO such slot and seeds the same register with a register copy
(`mov edi,ebp`) or `xor edi,edi`, so the prologue is 1 instruction shorter and the
seed load is 2 bytes (reg) instead of 3 bytes (memory)

When a method seeds a loop-carried pointer with `this` (`T* cur = this;` as the
dead-but-defined initial value of a list/cursor variable that the loop overwrites
on its first useful iteration), MSVC 5.0 /O2 sometimes **spills `this` to a fresh
stack local** at entry and **reloads it** into the loop register, rather than
copying `ebp`/`ecx` directly. The frame slot is the lone `push ecx` in the
prologue; the reload is `mov <loopreg>,[esp+N]`.

```asm
; retail (this spilled):
push   ecx                 ; reserve a 4-byte local = incoming this (ecx)
push   ebx
push   ebp
mov    ebp,ecx             ; ebp = this (for member reads)
push   esi
push   edi
...
mov    eax,[ebp+0x24]
mov    edi,[esp+0x10]      ; edi = this, reloaded from the spilled local (loop seed)
xor    ebx,ebx
mov    esi,[eax+0x20]      ; node = owner->m_20
```

```asm
; recompile (no spill — register copy):
push   ebx                 ; (no `push ecx` slot)
push   ebp
mov    ebp,ecx
push   esi
push   edi
...
mov    eax,[ebp+0x24]
xor    ebx,ebx
mov    edi,ebp             ; edi = this (register copy)  — or `xor edi,edi` for `=0`
mov    esi,[eax+0x20]
```

WALL: this is a pure frame/regalloc choice. The loop seed is the dead initial
value of the cursor (the loop assigns it before any real use), so its *value* is
irrelevant to correctness — but retail materializes it through a stack round-trip
and the recompile through a register move. No natural C++ spelling forces MSVC5 to
spill `this` to a stack local here: `T* cur = this;` → `mov edi,ebp`; `T* cur = 0;`
→ `xor edi,edi`; an uninitialized cursor is UB and diverges clang. The body
(loop, calls, branches, immediates, epilogues) is otherwise byte-identical — keep
the dev-faithful `cur = this;` seed (retail's reloaded value IS `this`) and accept
the prologue delta. Same family as the `this`-register-spill residual noted in
[loop-preheader-vs-exit-block-order.md](loop-preheader-vs-exit-block-order.md),
isolated here to a function whose ONLY difference is the seed spill.

Evidence: Gruntz `CTileTriggerSwitchLogic::VerifyBlockLinks` (0x112c70, 196 B):
~86% fuzzy, byte-identical except the `push ecx` slot + `mov edi,[esp+0x10]` vs
`xor edi,edi`/`mov edi,ebp`.
