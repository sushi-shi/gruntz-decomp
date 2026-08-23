# A local declared ABOVE the region that writes it takes a frame home; declared at first use it reuses a constant carrier

tags: cpp:local cpp:scope cpp:branch | asm:sub asm:mov asm:xor | topic:codegen-idiom topic:regalloc
symptoms: `sub esp,N` is one dword larger than retail's and the first divergence is a
`mov DWORD PTR [esp+K],0` at the very top of the body that retail does not have; retail
instead carries the same 0 in a callee-saved register it already needed, and a tail that
retail cross-jump-merges into one call site is duplicated on our side
confidence: 8/10
variants: block-scope-overlays-a-local-with-a-dead-temp.md

## The signal

Two facts arrive together and they are the same fact:

* our frame is exactly one dword bigger, and the extra dword is initialized to a
  constant in the prologue (`mov DWORD PTR [esp+K],0`);
* a tail that retail reaches from several arms through ONE call site is emitted
  twice on our side.

cl 5.0 gives a local a frame home when its live range spans calls and no
callee-saved register is free for it. When the SAME constant is already parked in
a callee-saved register for other reasons — a comparison's zero, a flag's one —
and the local's live range starts *below* that parking point, the two coalesce
and the local costs nothing. Declared at the top of the body, the local's range
starts *above* the parking point, the coalesce is impossible, and it spills.

The duplicated tail follows: arms that would have ended in the identical
`push <reg>; push <reg>; call f` cannot cross-jump when one of them has to
materialise the constant as an immediate instead.

## CGrunt::RunMoveConfig 0x065630, 83.18 -> 92.07

```cpp
// NO - poseIdx is live from the top, so it gets [esp+0x10] and cl needs sub esp,0x10
i32 CGrunt::RunMoveConfig(i32 a, i32 b) {
    i32 poseIdx = 0;
    bool eq = ANIMATION_ACT_EQUALS("I");
    ...
    if (m_entranceReason == PICKUP_BOMB) { ... poseIdx stays 0 ... }
    else if (...) { poseIdx = 1; }
    SwitchAnimation(m_poseItem[poseIdx]);

// YES - declared just above the chain that writes it; retail's sub esp,0xc
    bool eq = ANIMATION_ACT_EQUALS("I");
    ...
    i32 poseIdx = 0;
    if (m_entranceReason == PICKUP_BOMB) { ... }
```

Retail's EBX is the inline `strcmp`'s zero (`xor ebx,ebx` at the compare's exit),
used for `cmp eax,ebx`, `cmp [esi+0x21c],ebx` and four zero member stores — and
then, unchanged, as the pose index, which the `rand() % 100 < 80` arm sets with
`mov ebx,edi` (edi = the parked 1) and the other arm with `xor ebx,ebx`.

After the move: base and retail agree on 256 instructions, 19 calls, 27 branches,
1 return and 35 ordered referents; `walls diagnose` drops from CFG to
REGALLOC/SCHEDULING and the residue is a one-step register rotation from the
compare result onward.

## Not this

* Splitting the declaration (`bool eq; eq = ...;` against `bool eq = ...;`) is
  byte-identical — measured on the same function, same 92.07. Only the position
  of the declaration relative to the writing region moves the allocation.
* The mirror does not hold: moving a declaration DOWN cannot help when the local
  has no constant carrier to coalesce with. Read retail's callee-saved registers
  first and confirm one of them already holds the local's initial value.
