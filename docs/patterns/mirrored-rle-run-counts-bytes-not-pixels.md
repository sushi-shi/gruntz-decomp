# The mirrored RLE blit counts BYTES and re-reads the run length after the copy

- **confidence** c9
- **tags** `cpp:loop` `cpp:local` | `asm:sar` `asm:dec` `asm:mov` | `topic:codegen-idiom`

## Symptom

A `CDDrawShadeBlit::Blit*Mirrored` row loop stuck in the 40s with a frame several dwords
wider than retail, and `sar`s in the base that retail does not have at all.

## Three facts, each byte-proven on `BlitCopyMirrored` 0x149d00

**1. The loop counter is the BYTE count, in both the 8bpp and the 16bpp arm.**
Retail has ZERO `sar`/`shr` in the whole function, so `bytes / 2` is never spelled. The
16-bit arm decrements the byte counter *twice* per word:

```asm
dec eax / test edx,edx / jle skip      ; edx = bytes (pre-decrement copy)
L: mov dx,[ebp] / add ebp,2 / mov [ecx],dx / sub ecx,2
   dec eax / mov edx,eax / dec eax / test edx,edx / jg L
```
which is exactly `for (i32 k = bytes; k-- > 0; k--) { *dw-- = *sw++; }`.

**2. The run length is NOT cached in a local.** After the copy loop retail RE-LOADS
`m_rleData[pos]` (`mov eax,[esi+0xc] / mov cl,[edi+eax]`) to compute `x -= n` and
`pos += n*m_srcBpp + 1`. The copy loop's stores kill MSVC's memory CSE, so a source that
spells `m_rleData[pos]` inline gets the reload for free - a source that writes
`u8 b = m_rleData[pos]; i32 cnt = b;` keeps it live in a register across the loop and
pays a spill slot for it.

**3. `rowInc` IS `pitch`.** The vflip arm negates the pitch in place and the row loops
add that one variable; a separate `rowInc` local is one dword of frame that retail does
not have. (`BlitShadedMirrored` 0x14b770's frame becomes exactly retail's 0x30 on this
change alone.)

## Result

45.83 -> 58.65, frame 0x18 -> 0xc against retail's 0x8. The remaining dword is retail
parking `x` in the dead `dst` parameter's home slot.

related:
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md),
[cse-defeat-uncached-global-rewalk.md](cse-defeat-uncached-global-rewalk.md)
