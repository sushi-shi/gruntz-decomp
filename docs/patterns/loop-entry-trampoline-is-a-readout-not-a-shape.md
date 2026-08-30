# The loop-entry trampoline is a READOUT of the cursor's home, not a loop shape

tags: cpp:loop cpp:local | asm:jmp asm:mov | topic:wall topic:scoring-artifact
symptoms: one side carries an extra unconditional `jmp` at a loop entry that skips a
one-instruction block reloading a value from a stack home, and the loop latch branches
back INTO that block; the two sides are otherwise arm-for-arm identical
confidence: 10/10 (the mechanism), 0/10 (an isolated loop-spelling fix), 8/10
(a cross-arm cursor-lifetime fix when the whole function keeps the entry value live)

Nine independent `bounded` reviews described this same shape and asked for the source
form behind it. There is no lever in the local loop spelling. The block is C2's loop
header, spliced in front of the loop by `lg.c` (`0x0043aad7`), and it exists exactly
when the cursor's loop-carried location is MEMORY while the entry edge already has it
in a register:

```asm
;   ours - CGameLevel::BroadPhase 0x167ea0
  22: je   <exit>              ; while (pos)
  28: jmp  0x2e                ; <- the trampoline: entry has pos in EAX already
  2a: mov  eax,[esp+0x10]      ; <- the header block; ONLY the latch enters here
  2e: mov  esi,[eax+0x8]       ;    body
 16b: mov  eax,[esp+0x10]      ; latch reloads the same slot...
 171: jne  0x2a                ; ...and re-enters the header, which reloads it AGAIN

;   retail - no header block, the latch's own reload is the loop-carried definition
  28: mov  ecx,eax            ; body starts by copying the cursor out of EAX
 161: mov  eax,[esp+0x14]
 167: jne  0x28
```

## It is not an isolated loop-spelling question

Five spellings of the cursor walk, compiled with cl 5.0 `/O2 /MT` in one probe TU, are
**byte-identical** (one sha1 across all five):

| spelling | result |
|---|---|
| `while (pos) { N* nd = (N*)pos; pos = nd->next; ... }` | identical |
| `POSITION pos; ... list.GetNext(pos)` (by-reference accessor, `void*&`) | identical |
| the same accessor's `const` overload, returning by value | identical |
| `group->NextChild(pos)` (a member inline wrapping the accessor) | identical |
| `POSITION cur = pos; pos = ((N*)cur)->next;` (explicit latch/advance) | identical |

On the real body the same holds: `for` against `while`, a typed node cursor against a
`POSITION`, a two-statement advance, the `&` operand order in the gate chain, and the
declaration order of the eight geometry temps all produce one hash. A `for(;;)` with an
inner `break` is the only spelling that moves, and it moves AWAY.

## What it actually tracks

The probe reproduces the tree's object byte-for-byte, which makes it a real bisector.
Removing two of the guard's comparisons removes the trampoline; computing the candidate
rectangle before the first comparison removes it. Both change pressure, neither is a
shape retail has. So the trampoline follows the allocator, and it appears on **either**
side: of the nine rows, retail carries the extra edge in four (`ClaimTilesAround`
0x2d800, `FlipVertical` 0x13ebb0, `BlitShadedForward` 0x14a200 with two,
`DrawGlyphRun` 0x179e70) and our base carries it in four (`BlitCopyForward` 0x149950,
`ConvertRowFlip` 0x14cfc0 with two, `SubTable` 0x14f310, `BroadPhase` 0x167ea0). A
feature that appears on both sides in equal numbers is a readout, not by itself a
missing entity.

## The cases where it DOES close, and how to recognise them

The trampoline closes when the pressure difference under it is itself a source defect.
`CShadeTableCache::SubTable` 0x14f310 wrote its pixel as one OR expression, so cl
reassociated the loop-invariant red|green pair, hoisted it into a b-loop preheader and
homed it - and the preheader brought its trampoline. Accumulating channel by channel
(`v |= ...`) denies the reassociation; the hoist, the preheader and the trampoline all
go together, 7 -> 6 unconditional jumps against retail's 6, and 78.72 -> 89.76.

`CDDrawShadeBlit::ConvertRowFlip` 0x14cfc0 proves the second source class: a
**cross-arm cursor lifetime**. Testing one PAL-alpha cursor spelling in isolation left
the two trampolines and looked flat. A 64-cell Cartesian campaign over the six switch
arms showed that the legacy case-local aliases collectively made cl preload `src`
before the callee-save pushes. The alpha loop carried the cursor in memory, but its
entry edge still held that preload, so C2 skipped the first header reload with a
trampoline. Using the actual `src` parameter in the PAL, raw-alpha and PAL-alpha arms,
while retaining the natural locals in the ordinary destination, 16-bit destination
and source-by-level arms, removes exactly both jumps: 30 -> 28 branches against
retail's 28, with all 11 returns and 24 relocations unchanged. The score descends
74.73 -> 73.10 because the new allocation exposes later scheduling residue; the CFG
correction remains decisive. The all-parameter control also has 28 branches, while
every high-score local-alpha frontier restores the two wrong jumps.

`CPlay::AddLevelGruntz` 0xd5960 proves a third source class: a **named result used
only by the immediately following comparison**. The baseline assigned
`PlaceObject(...)` to `i32 r` and then tested `r == -1`. C2 eliminated the machine
store, but the C1 local still changed allocation across the function: `this` occupied
EBP, the `POSITION` cursor lived at `[esp+0x10]`, and C2 inserted the entry trampoline
over its reload. Testing the call directly in the condition homes `this`, keeps the
cursor in EBP, and removes the trampoline: 0x165 -> 0x160 bytes, 116 -> 114
instructions, 8 -> 7 branches, and 87.3929 -> 100.0000 exact with the same five calls,
three returns, and ten ordered referents. Predeclaring the cursor, moving it into a
`for` header, and declaring all ordinary locals at function scope were byte-identical
negative controls. The direct condition is therefore a local-census correction, not
a loop-spelling trick.

So the screen is not "who has the extra jmp" but **"why does the entry edge already
own the value whose stack home the latch reloads?"** First inspect a hoisted invariant.
Then remove unjustified one-use result locals before changing the loop. For a large
switch, also inspect parameter aliases across every sibling arm and run the Cartesian
product: a local test cannot see a function-entry preload selected by the other arms.
If none of these source defects exists and the block sits above a bare reload the other
side also performs, it is bounded allocation.

    # rows where ours carries more unconditional jumps than retail
    # (34 of 579 in the sub-100 queue; 514 agree exactly)
    for each row: count `jmp` in the normalized base vs target obj

related: [wall-reasons-allocation.md](../relevations/wall-reasons-allocation.md),
[or-chain-reassociates-and-hoists-the-invariant-pair.md](or-chain-reassociates-and-hoists-the-invariant-pair.md),
[backward-goto-sinks-its-target-region.md](backward-goto-sinks-its-target-region.md)
