# Two if/else arms retail emits in FULL: give each arm its own local

tags: cpp:branch cpp:local | asm:jmp asm:jne | topic:codegen-idiom
symptoms: retail duplicates two `if/else` arms byte-for-byte (only one string
immediate differs) while ours ends one arm with `push reg` + `jmp <middle of the
other arm>`; base is 30-50 instructions SHORT; the deficit is concentrated in one
arm plus its `return` blocks; `insn_seq` shows a whole `+ call A / + call B / + call C`
run the base lacks
confidence: 9/10

cl 5.0 cross-jumps two `if`/`else` arms whose remaining statements are *the same
statements* - not merely the same machine code. The unit of comparison is the IL
statement list, so **which variables the statements name is load-bearing**: two arms
that both write into one function-scope buffer are identical and get merged; two arms
that each write into their OWN block-scope buffer are not, and cl emits both in full.
That is why the retail duplication exists at all - the dev declared the buffer inside
each branch.

Machine-level tail merging is NOT what is happening. The proof is in the same
function: retail leaves six byte-identical 5-instruction `return 0` blocks
unmerged, and so does ours. Only the *statement-identical* arms collapse.

```cpp
// NO - one function-scope buf makes the two arms statement-identical; cl emits the
// second arm once and jumps the first into its middle (-50 insns, 78.7%):
WwdHeader buf;
if (hi) { node = Resolve("GAME_BATTLEZ"); ... memcpy(&buf, p, 0x5f4); return buf.checksum; }
else    { node = Resolve("GAME_MULTI");   ... memcpy(&buf, p, 0x5f4); return buf.checksum; }

// YES - a buf per arm; cl emits both arms in full, like retail (-3 insns, 97.9%):
if (hi) { WwdHeader buf; node = Resolve("GAME_BATTLEZ"); ... return buf.checksum; }
else    { WwdHeader buf; node = Resolve("GAME_MULTI");   ... return buf.checksum; }
```

The block-scoped copies still share ONE stack slot (disjoint scopes), so the frame
size is unchanged - this costs nothing and is what the frame offsets already say.

Corollary for reading the target: **a local that shares a slot with another local
proves the two are in disjoint scopes.** In `CGruntzMgr::BuildLevelRezPath`
(0x00093d40) `scratch` sits at `[esp+0x18]`, the same slot as the `CFile`, so
`scratch` lives inside the `isEmpty != 0` arm; and the 0x20-wide union
(`buf` lands at `[esp+0x38]`) sizes it at `char scratch[32]`.

CGruntzMgr::BuildLevelRezPath 0x93d40: 78.67 -> 97.86.
