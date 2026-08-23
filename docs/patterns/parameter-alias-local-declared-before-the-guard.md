# A parameter re-homed over its own incoming slot is an alias LOCAL declared before the guard

tags: cpp:local cpp:decl cpp:branch cpp:member | asm:mov asm:cmp | topic:codegen-idiom
symptoms: retail opens with `mov <r2>,<r1>` and `mov DWORD PTR [esp+N],<r2>`
writing the parameter's own incoming slot back with the value it already holds,
then loads the guarded member into a register (`mov eax,[ebx]` / `cmp eax,imm`)
where ours folds it into the compare (`cmp DWORD PTR [ebx],imm`)
confidence: 8/10

A redundant self-store into a parameter's home slot is not noise: it is a
DIFFERENT variable that cl gave the dead parameter slot. Declaring the alias
local before the guard (and reading the guard through it) reproduces both the
copy and the un-folded load - the guard now reads a local's member, which cl
materializes rather than folding into the `cmp`.

```cpp
WwdHeader* source = hdr;                  // declared BEFORE the guard
if (source->headerSize > sizeof(*source)) {
    return 0;
}
m_header = *source;
```
```asm
mov ebx,DWORD PTR [esp+0x20]   ; hdr
mov edx,ebx                    ; source = hdr
mov eax,DWORD PTR [ebx]        ; source->headerSize, NOT folded
mov DWORD PTR [esp+0x20],edx   ; source homed over hdr's dead slot
cmp eax,0x5f4
jbe ...
```
STEERABLE. `CGameLevel::LoadWwd` 0x15d280 91.22 -> 95.53 with the prologue byte
exact; the alias existed in the source but was declared after the guard and after
the header copy, so cl folded the load and sank the self-store.
