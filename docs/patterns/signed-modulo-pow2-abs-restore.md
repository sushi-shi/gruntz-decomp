# Signed `% 2^k` lowers to abs-then-restore-sign (one cdq, two xor/sub around the mask)
tags: cpp:modulo cpp:int | asm:cdq asm:xor asm:sub asm:and | topic:codegen-idiom
symptoms: `and 0xff` (or any prep); `cdq`; `xor eax,edx`; `sub eax,edx`; `and eax,0x7f`; `xor eax,edx`; `sub eax,edx` — a SINGLE cdq whose edx is reused by two branchless-abs pairs straddling the `& (2^k-1)` mask; looks like a redundant double-abs
confidence: 9/10

MSVC5 lowers signed `x % 128` (any `% 2^k`) to `sign*(abs(x) & (2^k-1))`: cdq once
(edx = sign(x)), abs `(x^edx)-edx`, mask `& 0x7f`, then re-apply the SAME edx to
restore the sign. Don't read the two xor/sub pairs as a nested `abs(abs(x)&0x7f)`
— spelling that re-derives the sign (a second cdq, or `x>>31` → `sar`) and diverges.
Note MSVC5 does NOT range-prove a non-negative operand (e.g. `c & 0xff`), so it
emits the full signed-modulo even when the result can't be negative.

```cpp
return m_table[(c & 0xff) % 128];        // not abs(...)&0x7f, not (...)>>31
```
```asm
25 ff 00 00 00   and eax,0xff
99               cdq
33 c2 / 2b c2    xor eax,edx ; sub eax,edx     ; abs
83 e0 7f         and eax,0x7f
33 c2 / 2b c2    xor eax,edx ; sub eax,edx     ; restore sign (reuse edx)
```
STEERABLE → 100%. Evidence: GlyphTable::Get 0xc0430 / Set 0xc03f0 (`(m_10+(c&0xff)) % 128`) — both 100%.
