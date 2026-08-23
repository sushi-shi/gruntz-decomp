# The loop-entry trampoline is a READOUT of the cursor's home, not a loop shape

tags: cpp:loop cpp:local | asm:jmp asm:mov | topic:wall topic:scoring-artifact
symptoms: one side carries an extra unconditional `jmp` at a loop entry that skips a
one-instruction block reloading a value from a stack home, and the loop latch branches
back INTO that block; the two sides are otherwise arm-for-arm identical
confidence: 10/10 (the mechanism), 0/10 (a loop-spelling fix - there is none)

Nine independent `bounded` reviews described this same shape and asked for the source
form behind it. There is none. The block is C2's loop header, spliced in front of the
loop by `lg.c` (`0x0043aad7`), and it exists exactly when the cursor's loop-carried
location is MEMORY while the entry edge already has it in a register:

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

## It is not a loop-spelling question

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
feature that appears on both sides in equal numbers is a coin, not a missing entity.

## The one case where it DOES close, and how to recognise it

The trampoline closes when the pressure difference under it is itself a source defect.
`CShadeTableCache::SubTable` 0x14f310 wrote its pixel as one OR expression, so cl
reassociated the loop-invariant red|green pair, hoisted it into a b-loop preheader and
homed it - and the preheader brought its trampoline. Accumulating channel by channel
(`v |= ...`) denies the reassociation; the hoist, the preheader and the trampoline all
go together, 7 -> 6 unconditional jumps against retail's 6, and 78.72 -> 89.76.

So the screen is not "who has the extra jmp" but **"does the extra jmp sit above a
hoisted invariant we created?"** If it sits above a bare reload of a value the other
side also reloads, it is allocation - park it.

    # rows where ours carries more unconditional jumps than retail
    # (34 of 579 in the sub-100 queue; 514 agree exactly)
    for each row: count `jmp` in the normalized base vs target obj

related: [wall-reasons-allocation.md](../relevations/wall-reasons-allocation.md),
[or-chain-reassociates-and-hoists-the-invariant-pair.md](or-chain-reassociates-and-hoists-the-invariant-pair.md),
[backward-goto-sinks-its-target-region.md](backward-goto-sinks-its-target-region.md)
