# Force a remainder to reuse the quotient: write `n - q*C`, not `n % C`
tags: cpp:modulo cpp:int | asm:mul asm:lea asm:div | topic:codegen-idiom
symptoms: a constant-divide chain where the quotient is a magic-multiply (`mov eax,MAGIC; mul; shr edx,k`) but the remainder is an UNEXPECTED `div`/`idiv` (`mov ecx,C; div ecx`) — cl divided twice instead of deriving `rem = n - q*C`
confidence: 8/10

When source separately spells `*out = n / C;` and then `n = n % C;`, MSVC5 lowers
the quotient to a magic-multiply but lowers the modulo to an independent `div`
(it does NOT fuse them). Retail derives the remainder from the already-computed
quotient: `rem = n - q*C`, strength-reducing `q*C` to a lea/shift chain that is
negated and `add`ed to `n` (kept live in one reg). Capture the quotient in a
local and subtract it back.

```cpp
unsigned q1 = n / 3600000;   // magic-multiply, ONE division
*hh = q1;
n -= q1 * 3600000;           // q1*C strength-reduced (neg;shl;lea*;shl), add back
unsigned q2 = n / 60000;
*mm = q2;
n -= q2 * 60000;
*ss = n / 1000;
```
```asm
mov eax,0x95217cb1; mul ecx; shr edx,0x15      ; q1 = n/3600000
mov edx,...; neg; shl 2; sub; lea*4; shl 7      ; q1*3600000  (NOT a div)
add ecx,edx                                     ; n -= q1*C
```
STEERABLE → 100%. Evidence: TimeSplit 0x119210 (ms → H:M:S splitter) 52%→100%.
