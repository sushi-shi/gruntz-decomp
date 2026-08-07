# A switch's default arm that loads a PARAMETER slot for no reason: an uninitialised local
tags: cpp:switch cpp:local | asm:mov asm:jmp asm:ja | topic:codegen-idiom topic:frame
symptoms: mov eax,[esp+0x38] in the default block, ja <join> vs retail ja <return>, or esi,-1
confidence: 8/10

A `switch` that sets two locals per arm, followed by `if (tag != -1) { ... }`.
Retail's range check jumps the default edge **straight to the shared `return`**
and drops the pre-switch `tag = -1` / `id = 0` seeds as dead. Ours keeps both
(`or esi,0xffffffff` / `xor eax,eax`) and grows a default block whose only
instruction reads a **parameter's** stack slot — that garbage load is cl5
materialising an *uninitialised* local so the join has a value to merge, and a
non-empty default block is exactly what stops the edge being threaded.

```cpp
i32 slot   = -1;
i32 nameId = 0;          // WITHOUT this cl loads [esp+0x38] (a param slot) in the default arm
switch (c) {
    case CTRL_SAVESLOT_LOAD0: slot = 0; nameId = CTRL_SAVEDLG_SLOT0; break;
    ...
}
if (slot != -1) { ... }  // keep this shape: `if (slot == -1) return 0;` un-shares the epilogue
```
```asm
lea    eax,[edx-0x490]
cmp    eax,0x9
ja     0xe430c               ; retail: the DEFAULT edge is threaded to `return 0`
jmp    DWORD PTR [eax*4+0x4e4390]
```

Two more readings from the same function: the frame was `sub esp,0x20` against
retail's `0x24` with the body otherwise byte-identical — the name buffer is one
byte longer than the `GetDlgItemText` cap it is passed (`char name[0x21]`,
rounded to 0x24), which realigns every `[esp+N]`; and inverting the guard to an
early `return 0` makes cl5 inline the epilogue at each early return instead of
sharing retail's one tail (98.99 -> 83.45). DrawSaveGameMenu 0xe3f40 98.69 ->
98.99; the residual `or esi,-1` / `xor eax,eax` pair is the unthreaded edge.
