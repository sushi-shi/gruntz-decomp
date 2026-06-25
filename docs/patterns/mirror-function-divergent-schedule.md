# Mirror-image twin functions schedule the same source differently — regalloc wall
tags: cpp:local | asm:xor asm:mov asm:shr | topic:wall topic:regalloc topic:scheduling
symptoms: two structurally-identical functions, one 100% the other ~99.8%, the high-% twin's round-1 prologue differs by an extra `xor reg,reg` + a `shr` via a different register
confidence: 8/10

When a TU has two near-mirror functions (forward vs reverse of the same unrolled loop —
encrypt/decrypt, push/pop, ascending/descending sweeps), MSVC5 /O2 can pick a DIFFERENT
register allocation / instruction schedule for each, even though both are written with the
SAME macro/helper over the SAME locals. The optimizer's choices are per-function and
essentially a coin flip; one twin matches retail 100%, the other plateaus ~99.8% on a tiny
prologue permutation that no source spelling reproduces (the bodies past round 1 stay
byte-exact).

```cpp
// Same BF_ENC macro drives BOTH; encipher is 100%, decipher is 99.84%.
#define BF_ENC(LL, R, P) (LL ^= (P), LL ^= F(R))
void Blowfish_encipher(u32* xl, u32* xr) { l ^= P[0];  BF_ENC(r,l,P[1]);  ... }
void Blowfish_decipher(u32* xl, u32* xr) { l ^= P[17]; BF_ENC(r,l,P[16]); ... }
```
```asm
; encipher round 1 (retail == ours):  mov ecx,edx; mov al,[esp+0x16]; shr ecx,0x18
; decipher round 1 (retail):          xor ecx,ecx (early); mov eax,edx; mov cl,[esp+0x16]; shr eax,0x18
; decipher round 1 (ours):            matches the encipher schedule, not retail's -> ~99.8%
```
WALL (regalloc/scheduling). Reordering the macro's XOR (`(LL^P)^F` vs `(F^LL)^P`) does NOT
move it — the compiler normalizes the 3-way XOR. Evidence: Blowfish_encipher (0x16f7f0) 100%
exact, Blowfish_decipher (0x16fc70) 99.84% — identical macro, one diverging round-1 prologue.
