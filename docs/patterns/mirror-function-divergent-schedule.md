# Mirror-image twins can exchange the matching schedule — TU-state wall
tags: cpp:local | asm:xor asm:mov asm:shr | topic:wall topic:regalloc topic:scheduling
symptoms: two structurally-identical functions use one macro but retail gives them different register schedules; changing a later sibling's copy shape flips which twin is close
confidence: 8/10

When a TU has two near-mirror functions (forward vs reverse of the same unrolled loop —
encrypt/decrypt, push/pop, ascending/descending sweeps), MSVC5 /O2 can pick a DIFFERENT
register allocation / instruction schedule for each, even though both are written with the
SAME macro/helper over the SAME locals. In the Blowfish TU, the source shape of the later
key-initialization function changes the schedule selected for both earlier functions.

```cpp
// The same BF_ENC macro drives both functions.
#define BF_ENC(LL, R, P) (LL ^= (P), LL ^= F(R))
void Blowfish_encipher(u32* xl, u32* xr) { l ^= P[0];  BF_ENC(r,l,P[1]);  ... }
void Blowfish_decipher(u32* xl, u32* xr) { l ^= P[17]; BF_ENC(r,l,P[16]); ... }
```

Controlled A/B evidence:

| S-box/copy model | Encipher | Decipher | Initialize |
| --- | ---: | ---: | ---: |
| flat `u32[1024]` plus scalar copy loop | 100% | 61.51% | 99.89% |
| `u32[4][256]` plus `memcpy` | 60.41% | 99.875% | 100% |

The current source keeps the second model because it represents the four S-boxes directly and
retail's `rep movsd` copy exactly. Reordering the macro's three-way XOR does not move either
schedule. Treat this as a TU-cumulative optimizer-state wall; do not describe either current
twin as exact without rebuilding the whole TU.
