# Fully-unrolled Blowfish/Feistel block cipher — body byte-exact in shape, arg-pointer regalloc differs
tags: cpp:local cpp:loop | asm:mov asm:xor asm:shr | topic:wall topic:regalloc

symptoms: 16 near-identical S-box-lookup rounds, four `mov reg,[idx*4+0x61aef8/b2f8/b6f8/baf8]` (the four 256-entry Blowfish S-boxes), per-round spill `mov [esp+0x14],val` + byte read `mov cl,[esp+0x16]` (the `>>16` byte from the spill slot), 18 absolute `ds:0x61aeb0..0x61aef4` P-array loads; recompile reaches only ~50-57% though every opcode/operand/immediate matches
confidence: 7/10

The engine embeds Bruce Schneier's reference Blowfish (P-array @0x61aeb0 init to the
digits-of-pi constants `0x243f6a88,...`; S-boxes @0x61aef8/b2f8/b6f8/baf8 init to
`0xd1310ba6,...`). The single-block encipher (0x16f7f0, loads P[0] first) and
decipher (0x16fc70, loads P[17] first) are 16 fully-unrolled Feistel rounds. The
round body — `Xr ^= ((S0[Xl>>24]+S1[Xl>>16])^S2[Xl>>8])+S3[Xl] ^ P[i]`, with the
current half spilled to `[esp+0x14]` and its `>>16` byte re-read from `[esp+0x16]`
— reproduces in SHAPE (identical instruction multiset), but MSVC5 assigns the two
`u32*` arg pointers to different callee-saved registers than retail (retail pins
xl→edi, xr→ebp and hoists `mov eax,P[17]` ABOVE the 4 pushes; recompile lands
esi/ebp or esi/edi and pushes first), and the phase shift cascades through all 16
rounds + the two final stores.

```cpp
#define F(x) (((g_bfS0[(x)>>24] + g_bfS1[((x)>>16)&0xff]) ^ g_bfS2[((x)>>8)&0xff]) + g_bfS3[(x)&0xff])
// decipher: swap folded out by alternating which half is F-mixed (closest, ~56.6%)
u32 l = *xl, r = *xr;
l ^= g_bfP[17];
r ^= F(l) ^ g_bfP[16];  l ^= F(r) ^ g_bfP[15];  /* ... P14..P2 ... */
*xl = r ^ g_bfP[0];
*xr = F(r) ^ l ^ g_bfP[1];
```
```asm
mov eax,ds:0x61aef4          ; P[17] HOISTED above the pushes (retail)
push ebx; push ebp; push esi; push edi
mov edi,[esp+0x14]           ; xl -> edi   (recompile picks esi/ebp instead)
mov ebp,[esp+0x18]           ; xr -> ebp
... mov [esp+0x14],Xl ; mov cl,[esp+0x16] ; mov ebx,[ecx*4+0x61b2f8] ...  (x16)
```
WALL (regalloc/scheduling). Alternating, temp-swap, and deferred-`*xr`-read source forms
all land 50-57%; logic is provably correct (tail computes the same `*xl=h^P0`, `*xr=F(h)^h'^P1`).
Evidence: Blowfish_decipher 0x16fc70 ~56.6% (@early-stop), src/Crypto/Blowfish.cpp.
