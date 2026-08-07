# Two locals in each other's `[esp+N]` slots = their DECLARATIONS are in the wrong order
tags: cpp:local cpp:scope | asm:lea asm:mov | topic:codegen-idiom topic:frame
symptoms: lea ecx,[esp+0x1c], lea eax,[esp+0x20], frame size AGREES, two slots swapped
confidence: 9/10

The frame size matches and every instruction pairs, but two address-taken locals
sit in each other's slots (`&key` at `[esp+0x1c]` here, `[esp+0x20]` in retail;
`&found` the mirror). cl5 hands scalar locals frame slots in **declaration order,
ascending address**, and an inner-scope local declared later still comes later —
so the slot ORDER is a direct readout of the source's declaration order, and a
swapped pair is a source bug, not allocator noise.

```cpp
// retail: found at [esp+0x1c], key at [esp+0x20]  =>  found is DECLARED FIRST,
// even though its initialiser lives inside the `if` that key gates.
CGameObject* found;
i32 key;
arc->Read(&key, sizeof(key));
if (key != 0) {
    found = NULL;
    ...
}
```
```asm
lea    eax,[esp+0x20]        ; &key   - the SECOND declared local
...
lea    ecx,[esp+0x1c]        ; &found - the FIRST declared local
mov    DWORD PTR [esp+0x1c],ebx
```

Steerable: reorder the declarations, and move an initialiser that retail emits
inside a branch out of the declaration into a plain assignment there (a
declaration-with-initialiser at the outer scope emits the store at the outer
scope). CExitTrigger::SerializeMove 0x3f040 96.65 -> 98.95 (this plus dropping an
`i32 key = 0` initialiser retail never stores).
