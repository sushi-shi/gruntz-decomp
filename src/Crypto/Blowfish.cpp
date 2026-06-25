// Blowfish.cpp - Bruce Schneier's reference Blowfish, single-block decipher.
//
// IDENTITY: the four 256-entry S-boxes and the 18-entry P-array are the standard
// Blowfish digits-of-pi tables (P[0]=0x243f6a88, S0[0]=0xd1310ba6, ...), proving
// this is Blowfish. The companion Blowfish_encipher (0x16f7f0) loads P[0] first
// (ascending); this routine loads P[17] first (descending) -> it is the DECIPHER.
//
// Free __cdecl functions over the two 32-bit block halves (*xl, *xr). The 16
// Feistel rounds are fully unrolled (straight-line, swap folded out by
// alternating which half is F-mixed), exactly as the macro-unrolled reference
// lowers under MSVC 5.0 /O2. F(x) = ((S0[x>>24] + S1[x>>16]) ^ S2[x>>8]) + S3[x].
#include <Crypto/Blowfish.h>

#define F(x)                                                                                       \
    (((g_bfS0[(x) >> 24] + g_bfS1[((x) >> 16) & 0xff]) ^ g_bfS2[((x) >> 8) & 0xff])                \
     + g_bfS3[(x) & 0xff])

// @early-stop
// regalloc wall (~56.6%): logic byte-exact in shape (same opcodes/operands/
// immediates, F-fn, round count, P-order, final P0/P1), but MSVC5 assigns the two
// arg pointers to esi/ebp where retail uses edi/ebp and hoists `mov eax,P[17]`
// above the pushes; no source form (alternating / temp-swap / deferred-read) flips
// it. See docs/patterns/blowfish-feistel-unroll-regalloc.md.
RVA(0x0016fc70, 0x48e)
void Blowfish_decipher(u32* xl, u32* xr) {
    u32 l = *xl;
    u32 r = *xr;

    l ^= g_bfP[17];
    r ^= F(l) ^ g_bfP[16];
    l ^= F(r) ^ g_bfP[15];
    r ^= F(l) ^ g_bfP[14];
    l ^= F(r) ^ g_bfP[13];
    r ^= F(l) ^ g_bfP[12];
    l ^= F(r) ^ g_bfP[11];
    r ^= F(l) ^ g_bfP[10];
    l ^= F(r) ^ g_bfP[9];
    r ^= F(l) ^ g_bfP[8];
    l ^= F(r) ^ g_bfP[7];
    r ^= F(l) ^ g_bfP[6];
    l ^= F(r) ^ g_bfP[5];
    r ^= F(l) ^ g_bfP[4];
    l ^= F(r) ^ g_bfP[3];
    r ^= F(l) ^ g_bfP[2];

    *xl = r ^ g_bfP[0];
    *xr = F(r) ^ l ^ g_bfP[1];
}
