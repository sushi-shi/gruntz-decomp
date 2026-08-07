# A byte tested-and-OR'd around a one-time init is `static T x = <dynamic>;`
tags: cpp:static cpp:local cpp:branch cpp:data | asm:test asm:or asm:jne | topic:codegen-idiom topic:identity

symptoms: `mov al,ds:0xNNNN` / `test bl,al` / `jne` / `or al,bl` / `mov ds:0xNNNN,al` around an
initializer; a file-scope `u8 g_xxxSeeded` / `g_xxxRolled` / `g_xxxLoadFlags` OR'd with a bit and
tested with the same bit; two globals where one is a 1-byte flag and the other is the value it
guards; `g_flag & 1` and `g_flag & 2` in the same function; `?<var>@?<n>??<fn>@@...@4HA` in an obj

confidence: 10/10

MSVC 5.0 compiles `static T x = <dynamic initializer>;` inside a function into a **one-time guard**:
one flag BYTE per function, one BIT per static in it, tested and set around the initializer. Reading
that expansion as source gives you two fabricated file-scope globals plus an `if` — the classic
tell is a `u8`/`char` global whose only two uses are `& bit` and `|= bit`.

```cpp
// WRONG - this is the compiler's OUTPUT transcribed as if it were source:
i32 seed;
if (!(g_randSeeded & 1)) { g_randSeeded |= 1; seed = timeGetTime(); }
else                     { seed = g_randSeed; }
g_randSeed = seed * 214013 + 2531011;
return (g_randSeed >> 0x10) & 0x7fff;

// RIGHT - Monolith's own source (the game's CREDITZ easter egg prints it verbatim):
int GetRandomNumber() {
    static long holdrand = timeGetTime();
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}
```
```asm
mov    al,ds:0x64c01c        ; the guard BYTE
test   al,0x1                ; this static's BIT
jne    already_done
mov    dl,al
or     dl,0x1
mov    BYTE PTR ds:0x64c01c,dl
<initializer>
mov    ds:0x64c274,eax       ; the static itself - a SEPARATE bss object
already_done:
```

**Multi-bit variant.** One guard byte serves every static in the function, one bit each, in
declaration order: `CPlay::LoadScrollSpeedOptions` (0xd12b0) tests `& 1` then `& 2` on 0x24c01c for
two statics. `n` statics in one function = `n` `test`/`or` pairs on ONE address with distinct bits,
not `n` flags.

**Recognition is by CODE SHAPE, never by data layout.** The conditional branch between the byte read
and the byte OR-store is what separates a guard from an ordinary flags-byte update (`hdr.flags |=
0x80` in `CMulti::SendChannelStat422` 0xbb0b0 reads/ORs/stores the same byte with no branch — not a
guard). The guard and its datum are two INDEPENDENT bss objects: measured across all 71 guards in
`GRUNTZ.EXE` the guard→datum delta ranges from -0x88 to +0x258, and the datum sometimes precedes the
guard. A "+0xc / 0x10-byte slot" reading is a three-sample coincidence, not a rule; nothing in .text
references the bytes between.

**Linkage decides how many pairs exist.** `static __inline` in a header gives each TU its OWN
guard+datum; a plain `__inline` gives the local static external linkage (emitted COMMON) so the whole
module shares ONE. Retail has three distinct pairs for `GetRandomNumber` — Gruntz 0x2c127d/0x2c1288,
Wwd 0x2c278c/0x2c2798, DDrawMgr 0x2c279c/0x2c27a8 — because each module carries its own copy of the
source, not because the statics are per-TU.

**Re-initialization hazard.** When one guard governs a value read several times later, hold it in a
local; re-writing the initializer expression at each use silently re-runs it (for an RNG, that means
an extra advance). Conversely, DELETING the advance and leaving a bare read of the datum is the
opposite bug: `bcb6cb0cd` did that at six sites and four functions stopped calling the RNG at all.
Count the initializer's fingerprint in retail (for the LCG, the `0x269ec3` addend) per function and
match your call count to it.

**Pinning.** `DATA(rva)` goes on the local static itself — cl5 spells it `_?s_x@?<n>??<Fn>@@...@4HA$S<m>`
where clang reports `?s_x@?1??<Fn>@@...@4HA` (extra leading `_`, a scope ordinal cl counts by blocks
already left, and the `$S` CodeView suffix); `labels.msvc5_data_symbol` wildcards all three,
authority-checked. The guard byte itself is `?$S55@?1??<Fn>@@...@4EA` — a compiler-assigned counter,
unspellable in source — so it stays unnamed. That costs nothing measurable: `CPlay::GetAmbientId`
0xda200 is 100.00 EXACT with its guard unnamed. Never fabricate a file-scope stand-in to name it.

Steerable, and byte-exact: cl reproduces retail's guard expansion instruction-for-instruction
including the register choice. Evidence: `LoadScrollSpeedOptions` 0xd12b0 98.75 (unchanged across the
conversion), `GetAmbientId` 0xda200 100.00 EXACT, `CSpotLight::Tick` 0xb1af0 78.88 -> 79.79. The
advance-restoration half: `StepArrivalReroll` 0x63b60 63.98 -> 85.04, `PeekCycle` 0x984b0 69.74 ->
88.18, `StartChipMachineCycle` 0x107d00 90.95 -> 96.27, `UpdateBootyWalkingGruntz` 0x1b690 90.03 ->
95.25.
