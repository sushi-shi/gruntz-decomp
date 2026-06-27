# Naked-asm Win32 import call: `call dword ptr [iat-extern]`, not `call Import`

`cpp:naked cpp:import | asm:call | topic:codegen-idiom topic:eh`

## Symptom

In a `__declspec(naked)` reconstruction (see
[[naked-asm-funclet-reconstruction]]) a call to a Win32 API must reproduce the
retail **indirect IAT** form `ff 15 <abs IAT addr>` (`call DWORD PTR ds:0x6c3fdc`),
but writing `call FreeLibrary` (even with the real `<Win32.h>` `__declspec(dllimport)`
decl) emits a 5-byte **direct** `e8 <rel32>` to the `__imp__FreeLibrary@4` symbol.
The opcode differs (`e8` vs `ff 15`) and every following offset shifts by 1, so the
function caps ~1 byte short of exact even though the rest is byte-identical.

## Fix

Model the IAT slot as a typed **DATA extern** at the slot VA and call *through* it:

```cpp
DATA(0x006c3fdc)
extern int(__stdcall* g_impFreeLibrary)(void*);   // the FreeLibrary IAT entry
...
__asm { push eax
        call dword ptr [g_impFreeLibrary] }       // ff 15 <DIR32 reloc, masked>
```

`call dword ptr [sym]` lowers to `ff 15 <DIR32 to &g_impFreeLibrary>`; the reloc is
masked against retail's absolute IAT address, so the code bytes match.

## Also: the delinker carves PAST a `__EH_prolog` / FPO `/GX` prologue

The framed COM/registry module (0x1bf*-0x1d5*) builds `/GX` functions two ways the
delinker can split:
- **ebp-frame** dtors call the shared `mov eax,OFFSET <handler>; call __EH_prolog`
  helper, then `push ecx`; the symbol/length the delinker carves can begin AFTER
  that prologue (e.g. FreeLibrary host dtor: real entry 0x1d4a0d, carved 0x1d4a18).
  The carved body is `push esi; mov esi,ecx; mov [ebp-0x10],esi; …; leave; ret` —
  it references ebp/fs with no local prologue, so a normal C++ dtor can't reproduce
  it at the boundary. Write the body verbatim as naked asm.
- **FPO** ctors emit the full SEH frame inline at the entry
  (`push -1; push OFFSET handler; mov eax,fs:[0]; push eax; mov fs:[0],esp`) with
  esp-relative args; a plain field-set ctor won't get a `/GX` frame at all, so the
  whole function is reproduced as naked asm (e.g. the Tick/sound ctor 0x136fe0).

`mov fs:[0], ecx` → `64 89 0d 00 00 00 00`, `mov eax, fs:[0]` → `64 a1 00 00 00 00`,
`and dword ptr [ebp-4], 0` → `83 65 fc 00` (imm8 form) all lower as expected in the
MS-asm parser.

## Evidence

m5_freelibhostdtor 0x1d4a18 (97%→**100%** after the IAT-call fix) and m5_soundtickctor
0x136fe0 (**100%**, FPO `/GX` ctor reproduced verbatim).
