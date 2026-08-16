# A per-function MNEMONIC HISTOGRAM (base vs target) names the wrong idiom in one shot

- **confidence** c9
- **tags** `topic:method` `topic:triage` | `asm:cdq` `asm:or` `asm:fidiv` `asm:call` | `topic:codegen-idiom`

## Symptom

A 40-60% function whose `--diff` output is a wall of register renames and whose
`gruntz walls diagnose --asm` says the topology is fine. The instruction *stream* is too long to
read and the instruction *count* (docs/patterns/instruction-count-mismatch-finds-the-
real-bug.md) only says "a bug exists", not which.

## The tool

Count **each mnemonic** in the base obj and in the delinked target obj, per function,
and print only the opcodes whose counts differ. A wrong source idiom shows up as a
matched *pair* of rows - one mnemonic family we over-emit and one we under-emit.

```python
# llvm-objdump -d build/objdiff/base/<unit>.obj      -> b.txt
# llvm-objdump -d build/objdiff/target/<unit>.c.obj  -> t.txt
# split on `^[0-9a-f]+ <name>:` but do NOT treat `$L...` labels as new functions
# (they are intra-function), and drop `nop`/`int3` (COMDAT padding).
```

Calibrate on a function already at 100%: its histogram must be **identical**. If it is
not, your padding/label filter is wrong. Switch-heavy functions inflate the TARGET side
because the delinked obj disassembles the in-`.text` jump table as garbage instructions
(`addb`, `aas`, `hlt`, `sldtw`, `lretl`) - ignore those rows, they are data.

## Pairs measured on one 40-75 batch (2026-08-07), each a real source bug

| histogram row pair | what it means | fix |
|---|---|---|
| `cltd 0 -> 4`, `sarl 8 -> 4` | retail uses the `abs()` intrinsic; we hand-rolled `(v^(v>>31))-(v>>31)` | `#include <stdlib.h>`; `v = abs(v)` |
| `orw 16 -> 8`, `orl 0 -> 11` | the 16-bit blend OR is done in `int`, not `u16` | drop the `static_cast<u16>` around the OR chain; narrow only at the store |
| `shll 10 -> 0`, `movb 11 -> 0`, `orb 5 -> 0`, `testb 6 -> 0` | we INLINE an LCG + lazy-seed guard retail calls out-of-line | the CRT `rand()`, not the game's inline `GameRand()` |
| `fdivrp 2 -> 0`, `fidivl 0 -> 2`, `fildl 6 -> 3` | retail divides the FP accumulator by an **int memory operand** | (open - neither `t /= intexpr` nor a named int temp reproduced `fidiv` here) |
| `retl 16 -> 12`, `popl 64 -> 48` | we have four more epilogues: early `return`s retail tail-merged | restructure to if/else (forward-goto-hoists-target-block.md) |
| `movsbl 0 -> 4` | retail sign-extends a byte we keep as `i32` | a `char` local/member, or an explicit narrowing in the expression |

The direction matters: `base > target` on a *cheap* opcode next to `base < target` on an
*expressive* one (`lea`, `cdq`, `fidiv`, `imul`) almost always means **we open-coded
something retail expressed with one instruction or one call**.

related:
[instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md),
[abs-intrinsic-cdq-xor-sub-vs-hand-rolled-negate.md](abs-intrinsic-cdq-xor-sub-vs-hand-rolled-negate.md),
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md)
