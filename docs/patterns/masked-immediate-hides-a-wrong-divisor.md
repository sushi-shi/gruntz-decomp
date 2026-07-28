# A wrong `/N` divisor shows ONLY as a different `shr` — `--diff` masks the magic constant

tags: cpp:int | asm:mul asm:shr | topic:mis-model topic:tooling
symptoms: `shr edx,0x5` vs `shr edx,0x6` as the sole diff after a `mul`,
`mov eax,[<addr>]` where the operand is really an immediate, `imul` + `mul` + `shr`
confidence: 10/10

`gruntz sema disasm --diff` masks large immediates the same way it masks addresses, so
an unsigned magic-division constant prints as `mov eax,[<addr>]` on BOTH sides and
compares equal. The only visible trace of a wrong divisor is the post-multiply shift.
`0x51eb851f + shr 5` is `/100`; `0x10624dd3 + shr 6` is `/1000`; `0xaaaaaaab + shr 3` is
`/12`, and so on. **Read the constant with `--base` / `--target` (unmasked) before
believing a shift is a scheduling nit.**

```asm
base:   mov eax,0x51eb851f | imul edx,[esp+0x18] | mul edx | shr edx,0x5   ; / 100
target: mov eax,0x10624dd3 | imul edx,[esp+0x18] | mul edx | shr edx,0x6   ; / 1000
```

Not a wall — a source constant that is simply wrong, and one the function's own comment
may repeat. Evidence: `CPlay::LoadScrollSpeedOptions` @0x0d12b0 had four `* speed / 100`
sites that are `/ 1000` in retail; with two other real bugs found in the same pass it went
90.0 → 93.9. Same family as [compensating-error-signatures](compensating-error-signatures.md):
verify every constant with `--base` when a high-% function's residue looks like one shift.
