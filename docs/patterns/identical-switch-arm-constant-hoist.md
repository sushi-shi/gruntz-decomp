# REFUTED: "identical switch arms sharing constants" is a 16-byte STRUCT ASSIGNMENT, not a hoist wall
tags: cpp:switch cpp:struct cpp:local | asm:mov asm:or asm:lea | topic:codegen-idiom topic:scoring-artifact
symptoms: switch/jump-table where every arm writes the SAME constant block; retail `lea ebx,[esi+N]; or eax,-1; or ecx,-1; mov edx,1; mov [ebx],eax; …` with the four value registers then recycled to 0 for a second block at `[esi+N+0x10]`; recompile instead hoists `-1`/`1`/`0` into callee-saved ebx/ebp/edi above the switch and emits 6-byte `mov [esi+0x2b0],ebx` stores per arm
confidence: 9/10

**This entry used to claim a cl build-8034 constant-CSE/LICM wall ("not steerable
from source"). That was wrong.** The shape is the ordinary MSVC5 16-byte
struct-assignment idiom, and the recompile diverged only because the destination
members were modelled as eight separate `i32`s instead of two 4-int aggregates.

Retail's per-arm sequence is a *block copy of a fully-constant aggregate*:

```asm
lea  ebx,[esi+0x2b0]   ; &m_toyRectA - a BASE REGISTER, because a 0x2b0 displacement
or   eax,0xffffffff    ;               costs 6 bytes per store and [ebx+N] costs 2
or   ecx,0xffffffff
mov  edx,1
mov  [ebx],eax         ; the four SRA'd fields of the source aggregate
mov  edi,edx
xor  eax,eax           ; ...the SAME four registers recycled to 0...
mov  [ebx+0x4],ecx
xor  ecx,ecx
mov  [ebx+0x8],edx
xor  edx,edx
mov  [ebx+0xc],edi
lea  ebx,[esi+0x2c0]   ; &m_toyRectB
xor  edi,edi
mov  [ebx],eax
mov  [ebx+0x4],ecx
mov  [ebx+0x8],edx
mov  [ebx+0xc],edi
```

The tell is the **register recycling**: the same four registers carry
`{-1,-1,1,1}` and then `{0,0,0,0}`. That is what SRA of one aggregate temp
assigned twice looks like - not four independent immediates.

FIX - model the destination as a real 16-byte struct and assign it:

```cpp
RECT m_toyRectA; // +0x2b0
RECT m_toyRectB; // +0x2c0
...
RECT a;
a.left = -1; a.top = -1; a.right = 1; a.bottom = 1;
m_toyRectA = a;
a.left = 0;  a.top = 0;  a.right = 0; a.bottom = 0;
m_toyRectB = a;
```

Written as eight scalar stores, cl sees ten identical constant stores across ten
arms and hoists `-1`/`1`/`0` into callee-saved registers above the switch; written
as two struct assignments it emits retail's `lea` + register-recycled block copy in
each arm. `CGrunt::LoadVehicleGruntSprites` 0x050ce0: **34 -> 97.4%** (the other
half of that function's gap was [[rva-extent-must-include-switch-tables]] - its
declared span stopped at the last `ret`, excluding the switch's jump table).

Corroboration that the members really are aggregates, not scalars: `Serialize`
moves them as `Write(&m_toyRectA, 16)` / `Read(&m_toyRectA, 0x10)`, and
`CGrunt::RectContainsGated` 0x51a20 builds a `CRect` out of each 4-int group.

General rule: **a `lea` of a member's address followed by short `[reg+0/4/8/c]`
stores is the struct-copy signature.** If your source spells that member group as
scalars you will never reproduce it, and the difference reads exactly like a
regalloc wall.
