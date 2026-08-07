# A spilled pointer local retail recomputes: write the indexed member at the far use

- **confidence** c8
- **tags** `cpp:local` `cpp:loop` `cpp:member` | `asm:lea` `asm:mov` | `topic:codegen-idiom`

## Symptom

Your frame is one dword wider than retail's and the extra slot holds a pointer you
introduced as a loop-local. Retail keeps the *strength-reduced cursor* in a callee-saved
register but, at one distant use, **recomputes the address from `this` and the loop index**:

```
 base                                   target (retail)
 add  ecx,0x3a8                         lea  ebp,[ecx+0x3a8]      ; cursor
 mov  [esp+0x10],ecx     ; cursor       mov  [esp+0x1c],ecx       ; this
 lea  eax,[ecx-0x4]                     ...
 mov  [esp+0x14],eax     ; the object   mov  eax,[esp+0x10]       ; blockIdx
 ...                                    mov  ecx,[esp+0x1c]
 mov  ecx,[esp+0x14]                    lea  eax,[eax+eax*4]
 call CPtrArray::RemoveAt               lea  ecx,[ecx+eax*4+0x3a4]
                                        call CPtrArray::RemoveAt
```

`lea ecx,[ecx+eax*4+0x3a4]` with `eax = idx*5` is `&m_array[idx]` rebuilt from scratch,
next to a register that already holds `&m_array[idx].m_pData`. cl will not rematerialise
a local you named; it keeps it, and because the cursor also has to live, one of them goes
to the stack - which widens the frame and shifts every `[esp+N]`.

## Cause

The devs used the local only where the compiler's own strength reduction wants it (the
loop condition and the element fetch), and spelled the far call site with the full
indexed member expression:

```cpp
CPtrArray* rec = &m_placedObjectCells[blockIdx];   // cursor: GetSize/GetAt
while (i < rec->GetSize()) {
    Coord* obj = (Coord*)rec->GetAt(i);
    ...
    m_placedObjectCells[blockIdx].RemoveAt(i, 1);  // NOT rec->RemoveAt
}
```

Writing `rec->RemoveAt(i, 1)` there is the whole 4-byte frame delta.

## Companion: load both bounds operands into named locals

The same function showed the other half of the pattern. Retail loads *both* coordinates
before the first bounds test where cl, given the member expressions, emits them lazily
around the short-circuit:

```
 base                          target
 mov  ecx,[esi]                mov  eax,[esi]
 cmp  ecx,[edx+0xc]            mov  edx,[esi+0x4]
 jae                           cmp  eax,[ecx+0xc]
 mov  edi,[esi+0x4]            jae
 cmp  edi,[edx+0x10]           cmp  edx,[ecx+0x10]
```

Two named locals (`i32 cellX = obj->m_x; i32 cellY = obj->m_y;`) restore it. `&&` still
short-circuits the *compares*; only the loads move.

## Measured

`CPlay::ClearPlacedObjects` 0xda030: 42.45 -> 51.28 (rematerialised RemoveAt receiver)
-> 52.98 (named coords). The remaining residue is a genuine regalloc choice - cl parks
`g_gameReg` in ebp for the whole function where retail parks the cursor there and
re-reads the global at each of its three uses; 200 permuter variants do not move it.

related:
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md)
