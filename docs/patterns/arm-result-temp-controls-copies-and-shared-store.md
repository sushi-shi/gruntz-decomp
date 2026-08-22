# A value an if/switch arm PRODUCES has two shapes: write the destination, or bind an arm temp

An arm that computes something the code after the merge consumes can be spelled two
ways, and cl 5.0 /O2 emits visibly different code for each. Both shapes appear in
retail, so the choice is source evidence, not style.

| source shape | what cl emits |
|---|---|
| the arm writes the destination directly (`tile.m_x = ...;` / `m_idleTimerLo = ...;`) | the arm owns the destination's register or member store; nothing is left for the merge block |
| the arm writes a LOCAL, and the destination is assigned after the merge | the arm gets its own scratch pseudo, and the SSA-destruction copy (register) or the member store (memory) lands once, shared |

## Detection signature

* **register case** — retail's arms end in a `mov <callee-saved>,<scratch>` pair the
  base does not have, and the base is exactly that many instructions SHORT. The base
  computes straight into the registers the arms merge on; retail computes into
  EAX/ECX and copies.
* **memory case** — retail's arms `jmp` to a block that begins with the store the
  base duplicates inside every arm, and the base's `jmp` targets are one store
  further on. `walls diagnose` calls this REGALLOC/SCHEDULING because the counts
  are close, but the arm/merge split is structural.

## Worked example 1 — the register case (arm temp is MISSING)

`CKitchenSlime::LoadSprites` 0xb3160. Retail's NORTH arm:

```asm
mov  eax,DWORD PTR [esi+0x84]   ; m_tilePosition.m_y
mov  ecx,DWORD PTR [esi+0x80]   ; m_tilePosition.m_x
sub  eax,0x20
mov  edi,ecx                    ; <- the copy pair the base lacked
mov  ebx,eax
jmp  <merge>
```

Base, from `tile.m_x = m_tilePosition.m_x; tile.m_y = m_tilePosition.m_y - 0x20;`:

```asm
mov  ebx,DWORD PTR [esi+0x84]
mov  edi,DWORD PTR [esi+0x80]
sub  ebx,0x20
jmp  <merge>
```

Five copies missing across four arms. Giving each arm its own `Coord step` and
assigning `tile = step` at the end of the arm restores them: 94.75 -> 96.63.

A second, independent knob inside the same arm: retail's WEST arm applies the x
delta BEFORE it loads y, which is what lets the EAST arm `jmp` into WEST's
`mov ecx,[esi+0x84]` tail. A whole-aggregate copy (`Coord step = m_tilePosition;`)
pins both loads ahead of the arithmetic and costs that shared tail; assigning the
two fields explicitly recovers it: 96.63 -> 97.61.

## Worked example 2 — the memory case (arm temp is MISSING)

`CGrunt::LoadGruntDecayConfig` 0x612a0. Retail:

```asm
; arm A                              ; arm B
mov  DWORD PTR [esi+0x838],ecx       mov  DWORD PTR [esi+0x838],ecx
sub  eax,edx                         mov  DWORD PTR [esi+0x83c],edi
mov  DWORD PTR [esi+0x83c],edi       mov  eax,ds:g_frameTime
jmp  0x1b0                           ; falls through
0x1b0:  mov  DWORD PTR [esi+0x830],eax   ; <- SHARED
        mov  DWORD PTR [esi+0x834],edi
```

The base wrote `m_idleTimerLo` inside both arms, so the store was duplicated and the
arm `jmp` landed one store later. Binding the value to a local and assigning the
member after the if/else gives retail's shared block: 94.77 -> 95.26, 163/163
instructions.

## Two negative controls, both measured on `LoadSprites`/`LoadGruntDecayConfig`

1. **Do not hoist the temp's declaration out of the arm and do not initialise it.**
   `i32 tx = 0, ty = 0;` above the switch let cl hoist the zeroing out of the loop and
   removed the uninitialised frame home retail keeps (`sub esp,0x18` vs retail's
   `0x1c`): 94.75 -> 88.97.
2. **Keep the arms' LEADING statements different.** Once the produced value moved to a
   local, putting the two arms' identical member stores FIRST let cl hoist them above
   the branch entirely; retail keeps them per-arm. Order each arm so the differing
   statement leads: 89.72 -> 95.26.

## Not this pattern

A copy pair that is a plain register rotation (base and target both compute into
callee-saved registers, only the names differ) is R1/R2 in
[`../relevations/wall-reasons-allocation.md`](../relevations/wall-reasons-allocation.md),
not a missing pseudo level. The tell is the instruction COUNT: this pattern always
shows the base SHORT by exactly the copies or the duplicated store.
