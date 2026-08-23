# A loaded byte copied with `mov bl,al` is a temp whose store lands AFTER its first consumer
tags: cpp:local cpp:string cpp:call | asm:mov asm:cmp asm:movsx | topic:codegen-idiom topic:regalloc
symptoms: base is a few bytes SHORTER than retail; retail loads into AL and pays a
`mov bl,al` we do not; our `cmp bl,imm8` is the 3-byte 80 /7 form where retail has the
2-byte 3C AL form; a short `je` in the base is a near `je` in the target and the whole
tail shifts by the difference
confidence: 9/10

cl 5.0 coalesces a loaded value into the callee-saved variable it is assigned to when
the assignment is that value's FIRST use. Put a consumer between the load and the
store - the sign-extended `%c` argument here - and the load keeps its own (byte-capable,
volatile) register across the consumer and cl emits an explicit copy. Two source
properties are BOTH required: a distinct named temp, and the store after the first
consumer. Either alone leaves the coalesce in place.

```cpp
// NO - cl coalesces the load into `letter`: mov bl,[esp+0x10] / cmp bl,0x14
letter = value[0];
sprintf(drivePath, "%c:\\", letter);

// YES - retail's mov al,[esp+0x10] / cmp al,0x14 / movsx esi,al / mov bl,al
char regLetter = value[0];
sprintf(drivePath, "%c:\\", regLetter);
if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
    letter = regLetter;
    ...
}
```
```asm
    mov    al,BYTE PTR [esp+0x10]
    cmp    al,0x14                  ; 3C 14 - the AL form, one byte shorter
    jle    <miss>
    movsx  esi,al                   ; the consumer, from the TEMP not from `letter`
    push   esi
    ...
    mov    bl,al                    ; the store, scheduled into the push run
    call   _sprintf
```
Steerable. `GetGruntzDriveLetter` 0x0001ffe0 **98.4234 -> 100.00 EXACT** (winapicdrom
9/9). Measured: temp + late store reaches 0 instruction diffs in three placements
(store before the `if`, inside the `if`, second `sprintf` reading either name); temp
with the store FIRST is 72 diff rows and `value[0]` re-read with no temp is 43/70.
Negative control in the same TU: the sibling `CheckCdRomRegistry` 0x0001fde0 emits
retail's copy from the one-variable spelling and is EXACT - so a one-variable source is
not always coalesced, and the discriminator is only decisive in the longer function
where `letter` also serves as the A..Z loop induction variable and stays live to the
shared success label. Read the retail bytes for `mov <byte reg>,<byte reg>` before
assuming the two values are one.

related: [recycled-spill-slot-reloads-in-creation-order](recycled-spill-slot-reloads-in-creation-order.md)
