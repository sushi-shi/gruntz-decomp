# A `push` of a callee-saved register BELOW the entry guard means the source had exactly ONE tail return
tags: cpp:branch cpp:local | asm:push asm:pop | topic:codegen-idiom
symptoms: push esi push edi push ebx after the guard, shrink wrap, prologue split, early-out pops fewer registers, entry pushes, +2 instruction count, delayed register save
confidence: 10/10

cl 5.0 shrink-wraps: registers a function's *entry guard* does not use get their
`push` emitted at the register's first definition, deep in the body, and the
guard's `return` then pops only what the entry pushed. It only does this when the
guard's `return` is the **sole early return** — any other `return` in the body
pins every save back into the prologue, so our base pushes all of them at entry
and every early-out pops them all (base is +2/+4 instructions and every register
rotates). The decision is made on the source-level flow graph, so it survives cl's
later tail duplication: retail functions with six `ret`s are still shrink-wrapped
because their source had one tail `return` that cl copied into each block.

```cpp
// NO - the second `return` pins all four pushes into the prologue
if (!p) { Report(); return 0; }
int x = q(b);
if (x == 0) { return 0; }        //  <-- this line is the whole bug
y = q(c);
return 0;

// YES - guard + ONE tail return; cl sinks push edi/ebx past the guard
if (!p) { Report(); return 0; }
int x = q(b);
if (x != 0) { y = q(c); }
return 0;
```
```asm
push   ebp                  ; only what the guard needs
push   esi
mov    esi,ecx
mov    ebp,DWORD PTR [esi+0xb0]
test   ebp,ebp
jne    L1
xor    eax,eax
pop    esi                  ; <-- the early-out pops TWO, not four
pop    ebp
ret
L1:
push   edi                  ; <-- sunk saves, in FIRST-USE order (not canonical)
push   ebx
```
Steerable. Nested `if`/`else` with a single tail return; a result variable also
works (`int r = 0; … r = call(); return r;`) but only if it is declared where the
guard does not need a zero register. Sieve: leading-push count base vs target
(33 candidates found tree-wide). `SoundSample::AcquireInstance` 0x135d70 90.31 ->
100.00 EXACT, `CGrunt::UpdateGruntStatus` 0x617c0 94.63 -> 100.00 EXACT,
`CDDrawWorkerHost::DeactivateDistantObjects` 0x163370 87.88 -> 100.00 EXACT,
`CPlay::StepGridWalk` 0xd0a60 66.67 -> 100.00 EXACT (that one was filed as a
WALL - see the related link).

## Read it in REVERSE too: base shrink-wraps, retail does not

The same rule run backwards names a missing source `return`. When the BASE sinks its
saves past the guard (early-out with no pops, sunk pushes in non-canonical order) and
the TARGET pushes all of them at entry with the early-out popping every one, retail's
source had a **second** early return that ours lacks - so cl shrink-wrapped ours and not
retail's. Add the return retail's flow graph had; nothing else changes.

```asm
; base: shrink-wrapped => our source has ONE early return
mov  eax,[ecx+0x78] / test eax,eax / jne L1 / xor eax,eax / ret   ; no pops
L1: mov eax,[ecx+0xc] / push edi / push esi / push ebp / push ebx  ; sunk, reversed
; target: NOT shrink-wrapped => retail's source had TWO
mov  eax,[ecx+0x78] / push ebx / push ebp / push esi / push edi    ; canonical order
test eax,eax / jne L1 / xor eax,eax / pop edi / pop esi / pop ebp / pop ebx / ret
```

The push ORDER is the cheap tell: sunk saves come out in first-definition order and read
*reversed* against the canonical `ebx, ebp, esi, edi` an entry prologue emits.

`SoundDevice::ClearVolumeRamps` 0x136ed0 **77.41 -> 100.00 EXACT** - body already byte-identical,
the entire residue was the prologue placement. Its guard-walk-delete twin
`SoundDevice::TickVolumeRamps` 0x136e20 sits three lines up already EXACT with the canonical
entry prologue, and it has THREE early returns; copying that shape (`if (node == NULL)
return 1;` ahead of a `do/while`, which cl rotates to the same blocks as the `while`) closed
ClearVolumeRamps. Prefer an exact neighbour in the same file as the model for the missing return.

### The reverse direction also moves the /GX FRAME, and it is a `void` rule too

`BuildLevelTitleString` 0x000e44e0 **95.7850 -> 100.00 EXACT** is the reverse reading on a
`void` function with three entry guards and no body return at all. Its preview loader was
written `if (Open() == 0) { g_previewImage = NULL; } else { ... }`; spelling the two failures
as `return;` closed it.

Two things this instance adds:

* **The EH object packing follows the prologue.** The frame pass runs after register
  assignment, so the shrink-wrap also permuted the two `/GX` objects: base packed
  `[flag][CString][CFile]` where retail has `[CFile][flag][CString]`, and both unwind
  funclets read the wrong `[ebp-N]`. `gruntz walls ehactions --shift` files that as
  `per-object` slot-shift and correctly says no edit targets the funclet - the edit is the
  parent's return count, and the funclets close with it (29 B and 11 B, both EXACT).
* **`void` counts returns the same way.** Nothing has to be returned for the rule to bite;
  a bare `return;` in the body is the whole fix, and the `f.Close()` before it is retail's
  own order.

**Census instrument, and it has TWO forms.** A shrink-wrapped function's epilogue comes
out either MERGED - the guard's exit branches strictly INSIDE the final pop run, below the
sunk pops - or TAIL-DUPLICATED, with the guard given its own shorter pop run and `ret` and
no branch into the tail at all. They are the same decision. `ClearVolumeRamps`, `AcquireInstance`,
`UpdateGruntStatus`, `DeactivateDistantObjects` and `StepGridWalk` above are ALL the
duplicated form, so a census written only for the merged form does not see any of them.

Decode each side from its OWN instruction stream. The two sides are not byte-symmetric -
our `call rel32` displacements are relocation-zeroed while the delinked target resolves
self-calls internally - so a raw-byte scan mixes decoder noise into the answer.

* **MERGED**, 2026-08-23: 48 base / 48 target / **48 agreeing** over 4712 paired symbols,
  **zero disagreements**. The last target-only row, `CFaderRadial::RenderFrame` 0x17fc60,
  closed 88.58 -> 99.72 (see the related link) and now splits on both sides.
* **CONTROL for that zero.** Retargeting RenderFrame's guard `je` by one byte in the real
  base obj (`0f 84 e7` -> `e6`, onto the head of the pop run instead of below `pop ebp`)
  makes the census report exactly one TARGET-ONLY row naming that function; the same byte
  in the target obj reports exactly one BASE-ONLY row; restoring both returns 48/48/48.
* **DUPLICATED**, read as the pop-run length multiset at every `ret` and restricted to
  symbols whose two sides have EQUAL ret counts (unequal counts are a tail-duplication
  difference, not a shrink-wrap one): 814 base / 814 target, 811 agreeing, and a six-row
  worklist - base-only `CGameLevel::MoveToward` (1,4,4,4 vs 4,4,4,4),
  `WarpTextureBlit` (2,3 vs 2,2), `CMapMgr::CellPush` (1,3 vs 3,3); target-only
  `CGrunt::LoadGruntDecayConfig2` (1,1 vs 1,3), `CPlay::HandleDragMove` (4,4,4,4,4 vs
  1,4,4,4,4), `CVariantSlot::Add` (3,3,3,3,3 vs 2,3,3,3,3).

**Screen out TU state before searching the source.** On BuildLevelTitleString, moving the
function to the head and to the tail of its TU, inserting a static definition immediately
above it, and declaration-count probes (typedef 0..12, class 0..8) are all byte-identical -
so the decision is function-intrinsic. Local scope (the arrays inner, outer, after the
guards, beside the `CFile`), guard shape (`&&` gate, `||` gate, brace-less returns), dead
initializers and value hoists are byte-identical too. When the census says one side is the
outlier, count the source's returns; do not sweep declarations.

related: [named-element-pointer-steals-a-callee-saved-register.md](named-element-pointer-steals-a-callee-saved-register.md) (the forward direction: a named element pointer eats the register the guard needed), [tail-block-placement-cross-jump-wall.md](tail-block-placement-cross-jump-wall.md) (the wall this breaks), [trailing-error-block-is-a-crossjump-magnet.md](trailing-error-block-is-a-crossjump-magnet.md) (the half that is still a wall: merging guards with `||` feeds cl's cross-jump).
