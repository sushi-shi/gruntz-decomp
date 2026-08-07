# An UNSIMPLIFIED `n-- > 0` guard in both arms of a branch means ONE counter variable, declared above it

tags: cpp:loop cpp:local cpp:branch | asm:dec asm:test asm:inc | topic:codegen-idiom
symptoms: `mov <r2>,<r1> / dec <r1> / test <r2>,<r2> / jle` where we emit a bare
`test <r1>,<r1> / jle`, a stray `inc <r1>` right after the guard, the counter copy
hoisted ABOVE the branch that selects the arm
confidence: 9/10

## Symptom

Two near-identical copy loops behind a one-byte mode test. Ours:

```asm
cmp  dl,0x1
jne  <16bpp>
test eax,eax                    ; bare count-down guard
jle  <skip>
L8:  mov dl,[edi] / mov [ecx],dl / dec ecx / inc edi / dec eax / jne L8
```

Retail spends three more instructions to do the same thing:

```asm
cmp  dl,0x1
mov  edx,eax                    ; <-- HOISTED above the branch
jne  <16bpp>
dec  eax                        ; n--
test edx,edx                    ; ...and test the PRE-decrement value
jle  <skip>
inc  eax                        ; restore n
L8:  mov dl,[ebp] / mov [ecx],dl / dec ecx / inc ebp / dec eax / jne L8
```

## Reading it

`mov edx,eax` (save the pre-decrement value) is emitted **before** the arm-selecting
`jne`, and `dec eax / test edx,edx` is duplicated into each arm. That is head-merging of
two arms whose first operation is the identical `n--` on the identical variable — which
only happens when the counter is **one variable declared above the branch**, live into
both arms.

Give each arm its own `for (i32 k = bytes; k-- > 0;)` and cl specialises each loop
independently: it proves the local is dead after, folds `k-- > 0` into a plain
`test/jle` + `dec/jne` count-down, and the save/restore pair disappears. The three
"extra" retail instructions are the tell that it could not do that.

## Fix

```cpp
i32 bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;   // already above the branch
if (m_srcBpp == 1) {
    u8* d = dst0;
    while (bytes-- > 0) { *d-- = *s++; }
} else {
    u16* d = Pix16(dst0);
    u16* sw = Pix16(s);
    while (bytes-- > 0) { *d-- = *sw++; bytes--; }   // 16bpp: two bytes per word
}
```

The run length IS the loop counter — no separate `k`. Measured on
`CDDrawShadeBlit::BlitCopyMirrored` 0x149d00 (four such branch pairs)
**66.29 → 70.18**.

## Rule

Count the guard. A `while (n-- > 0)` that retail spells in five instructions and we
spell in two is not scheduling noise: it says retail's `n` outlives the loop's own
scope. Look one level up for the variable it shares.

related:
[counted-loop-is-post-decrement-guard.md](counted-loop-is-post-decrement-guard.md),
[mirrored-rle-run-counts-bytes-not-pixels.md](mirrored-rle-run-counts-bytes-not-pixels.md)
