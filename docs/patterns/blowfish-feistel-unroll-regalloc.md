# A surviving standard-algorithm layer can expose a separate TU-state wall
tags: cpp:union cpp:macro cpp:local | asm:mov asm:xor asm:shr | topic:source-oracle topic:regalloc

symptoms: 16 near-identical S-box-lookup rounds, scalar shifts or a flattened round
macro reproduce the arithmetic, one mirror function improves while the other collapses,
and both retain the retail call set and branch skeleton
confidence: 10/10

The engine embeds Bruce Schneier's reference Blowfish (P-array @0x61aeb0 init to the
digits-of-pi constants `0x243f6a88,...`; S-boxes @0x61aef8/b2f8/b6f8/baf8 init to
`0xd1310ba6,...`). The single-block encipher (0x16f7f0, loads P[0] first) and
decipher (0x16fc70, loads P[17] first) are 16 fully-unrolled Feistel rounds. The
round body is `Xr ^= ((S0[Xl>>24]+S1[Xl>>16])^S2[Xl>>8])+S3[Xl] ^ P[i]`, with the
current half spilled to `[esp+0x14]` and its `>>16` byte re-read from `[esp+0x16]`.
Earlier explicit alternating/temp-swap/deferred-read reconstructions reproduced that
instruction multiset but chose the wrong argument registers and remained near 50-57%.
The later hand-expanded comma macro could make either mirror exact depending on the TU
declaration state, but it hid the developers' actual abstraction layer.

The pinned LithTech source resolves that layer: a little-endian `union aword` names the
word/byte view, `S`, `bf_F`, and `ROUND` preserve the nested macro family, and each source
line holds two rounds. Its key schedule likewise uses an `aword temp`, separate short loop
locals, table `memcpy`s, and `bf_P[i] = bf_P[i] ^ data`.

Applied as one source family, encipher moved 60.3505 -> 99.9357 while decipher moved
100 -> 61.4969. That reciprocal movement was not a vote against the source: diagnosis
showed the same call set and branch skeleton, while retail itself uses different register
schemes for the mirrors. Composing the already-proven real `<string.h>` declaration
boundary between the definitions made encipher, decipher, and initialization all exact.

The reverse-use rule is: restore a surviving standard algorithm completely, then classify
its residual independently. If mirror functions move reciprocally with identical semantic
topology, test the TU-state boundary before rewriting the algorithm. A flattened body can
be a local maximum even when one half happens to be exact.
