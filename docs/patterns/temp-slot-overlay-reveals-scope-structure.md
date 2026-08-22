# The mem-init temp's stack-slot overlay reveals the retail BLOCK structure

tags: cpp:local cpp:ctor cpp:scope | asm:lea asm:sub-esp | topic:codegen-idiom topic:regalloc
symptoms: a ctor (or any EH function with a compiler temp) matches call-for-call but the
`lea ecx,[esp+N]` addresses of a constructed temp and of body locals disagree with retail
while the frame size is equal - or the frame is LARGER than retail because two locals that
retail overlays got distinct slots
confidence: 9/10 (controlled A/B on one ctor, three layouts, each fully explained)

cl 5.0 assigns stack slots by SCOPE, not by liveness: locals of the outermost body scope
get the low offsets in declaration order; every nested block (and the mem-init inline-
expansion scope, which hosts constructed temps like a `zBitVec tmp("", 0)`) is a SIBLING
region that overlays the same high offsets. Which local shares the temp's slot is
therefore a fossil of the original block structure.

Worked example, `??0CSingleFrameMessage` 0x000ab310 (locals base = first byte above the
saved registers; frame `sub esp,0x24` on both sides; temp = the zBitVec ctor temp):

| source layout | r | bounds | temp | frame |
|---|---|---|---|---|
| `{ RECT r; RECT bounds; ... }` one inner block | +0x04 | +0x14 | **+0x04 (shares r)** | 0x24 |
| `RECT r; RECT bounds;` both at body top | +0x04 | +0x24 | +0x14 | **0x34** |
| `RECT r;` top, `{ RECT bounds; CopyRect(...); }` | +0x04 | **+0x14 (shares temp)** | +0x14 | 0x24 |

Retail has r exclusive at +0x04 and bounds sharing the temp's +0x14 - only the third
layout produces it: `r` was a top-level body local, `bounds` lived in a nested block that
closed after the `CopyRect`, and the center-math read `r` at top level. The score does not
move while a larger divergence (here a zero-register pin) dominates, but the three `lea`
bytes and the frame size are decided by this alone.

Reading rule: when base and target agree on frame size but a temp's `lea` disagrees by a
constant, list each local's offset on both sides and ask which pair OVERLAYS. The overlay
partner names the nested block; an oversized frame means the reconstruction hoisted a
block-scoped local to the top level.
