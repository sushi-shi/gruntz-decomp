# A signed `>>` lowers to `sar`; the unsigned cast that flips it to `shr` reschedules the block
tags: cpp:bitop cpp:shift | asm:sar asm:shr | topic:wall topic:regalloc
symptoms: body byte-exact EXCEPT one `sar reg,N` where the target has `shr reg,N`; casting the shift operand to unsigned flips that opcode but cascades many diffs (this/arg register flow swaps, `lea`<->`shl` for *4)
confidence: 7/10

MSVC 5.0 lowers `int >> N` to `sar` (arithmetic) and only `unsigned >> N` to `shr`
(logical) — it will NOT prove a guarded-positive `int` is non-negative. So an
otherwise byte-exact body can miss by exactly ONE opcode (`sar` vs `shr`) on a
right-shift whose retail source treated the value as unsigned. The tempting fix —
`(unsigned)x >> N`, an `unsigned` parameter, or a `u32` temp shared with the guard
— DOES flip the opcode, but it perturbs evaluation order / register allocation
for the whole surrounding block (the arg lands in `eax` vs `ecx`, the `this`-save
moves, `*4` becomes `lea`<->`shl`), trading a 1-instruction miss for ~10. Keep the
signed form (single-opcode miss) unless the cast happens to leave the schedule
intact.

```cpp
// retail bit-count round-up: keep the signed int form (closest, 1-opcode miss);
// (u32)nbits>>5 gives shr but reschedules this->esi/arg->eax + lea/shl => 11 diffs.
i32 nwords = (nbits >> 5) + ((nbits & 0x1f) != 0 ? 1 : 0);   // emits `sar eax,5`
```
```asm
; target wants:                ; cl (signed int) emits:
shr  eax,0x5                    sar  eax,0x5
```
WALL. Evidence: zBitVec::SetSize (0x16e100) — body byte-identical bar the one
`sar`/`shr`; every unsigned spelling rescheduled the round-up block (1 diff -> 11).
