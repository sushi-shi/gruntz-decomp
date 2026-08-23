# Identical `return 0` epilogues: retail duplicates, recompile tail-merges
tags: cpp:branch cpp:return | asm:ret asm:jmp | topic:wall
symptoms: body byte-identical, early-out/default `return 0` paths jump FORWARD to one
  shared `pop;pop;pop;ret` tail where retail emits the SAME 3-instr epilogue inline
  (duplicated) at each site; the merged tail also schedules `xor eax,eax` LATE
  (between pops) because a register stayed live to the merge; ~60-70% on an
  otherwise byte-exact leaf
confidence: 7/10

## Shape

A leaf with several paths that all `return 0` (a null-arg guard, a `default:` case,
and a fail-after-work tail), plus one `return <ptr>` success path. At `/O2`
(favor-speed) MSVC 5.0 in **retail** emitted each `return 0` as its OWN epilogue,
byte-for-byte identical and clean:

```
xor eax,eax
pop edi
pop esi
pop ebx
ret 4
```

three (or more) times, with the success path's `mov eax,esi; pop;pop;pop; ret 4`
also kept separate — i.e. **no epilogue cross-jumping at all**.

The recompile, from the equivalent source, instead **tail-merges** the identical
zero-returns: the early-out tests jump FORWARD to a single shared tail, and that
tail (being the fail-after-work block, where the object pointer in `esi` was live
through the final `cmp esi,edi` / Release call) schedules its `xor eax,eax`
*between* the pops:

```
pop edi
pop esi
xor eax,eax     ; <- delayed; clean retail form is xor-first
pop ebx
ret 4
```

So both the jump targets (every early `je`/`jne` displacement) and the tail bytes
diverge, dragging the fuzzy% well below the real (byte-exact) body match.

## Evidence

`CGameLevel::ReadImageSet` @0x15d820 (gamelevel TU). The image-set factory body —
the `dec eax;je` kind dispatch, the three `operator new(0x10/0x24/0x18)` +
manual-vtable-stamp variants (modeled via tiny `new CImageSetN` shells, see
[newd-class-real-size](newd-class-real-size.md) and the manual-stamp doctrine),
the shared null merge, the `Parse`(+0x14)/`Release`(+0x04) vtable dispatch — is
**byte-identical** to retail. The ONLY divergence is that retail emits four
separate epilogues (3× `xor eax,eax;pop;pop;pop;ret 4`, 1× `mov eax,esi;...`)
while the recompile tail-merges the three zero-returns into the fail tail. Plateau
~68%.

## Why it's a WALL

Whether MSVC duplicates vs cross-jumps identical small epilogues at `/O2` is an
internal layout decision driven by block-reachability and live-range shape, not by
any natural C spelling of the same logic. Tried and rejected:

- `switch`(default:return 0) vs `if/else-if` chain — the if-chain turns the exact
  `dec eax;je` kind-dispatch into a `cmp;je` chain (worse, 52%); the `switch`
  keeps the dispatch byte-exact but still merges the tails.
- explicit `else { x = 0; }` on the alloc-failure path vs implicit — no change to
  the merge.

The body is correct (offsets, alloc sizes, vtable stamps, dispatch all match).
Recognize the duplicated-vs-merged identical-epilogue signature and STOP — do not
restructure correct code chasing the optimizer's epilogue placement (orchestration
§2a). Sibling of [eh-state-numbering-base](eh-state-numbering-base.md) and the
register-pinning entropy walls.

## WHY retail sometimes declines the merge: one exit does not need the `xor`

The merge is not a coin. cl merges two `return 0` blocks when they are *identical*,
and a `return 0` reached on an edge where a register provably already holds 0 does
not need `xor eax,eax` — so it is NOT identical to a `return 0` that does, and
there is nothing to merge.

`CLatencyList::FillCombo` 0x37ff0 is the compact case. Two guards both `return 0`:

```cpp
if (m_list.GetCount() <= 0) { return 0; }          // eax = a count, may be negative
HWND combo = GetDlgItem(hDlg, ctrlId);
if (combo != NULL) { ... }
return 0;                                           // eax = GetDlgItem's NULL
```

Retail gives the FIRST its own `xor eax,eax / jmp <epilogue>` at 0x27 and lets the
SECOND branch straight to the epilogue (`je 0xd2`) with GetDlgItem's zero still in
eax. Ours jumps both to one shared `xor eax,eax` at 0xd0 — and it HAS to emit the
`xor` there, because the block now also serves the count path where eax holds a
possibly-negative count. Materialising the zero is what made the two blocks
identical, which is what allowed the merge.

So the reverse-audit reading is: **retail duplicating an epilogue says the two
exits differ in whether the return value is already live**, and it follows the
value flow, not the guard spelling. The guard spelling is byte-neutral —
`CGameObject::ResolveLinkedObject` 0x151b90 compiles
`if (ok) { m_carrier = found; return 1; } m_carrier = NULL; return 1;` and
`if (!ok) { m_carrier = NULL; return 1; } m_carrier = found; return 1;` to the
same bytes (85.88 both ways, verified instruction-for-instruction). Do not spend
an A/B on the polarity. Same signature, same verdict, on `CSBI_MenuItem::SetupImage`
0xe80e0 and `CWwdGameObject::CreateObject` 0x166640.

The OTHER direction — ours duplicates, retail shares — IS steerable, and the lever
is in [guard-skip-loop-not-early-return](guard-skip-loop-not-early-return.md):
state the return once in the source (SetGeom 0x164250, 78.53 -> 84.03).

## See also

The companion STEERABLE finding from the same TU: an engine helper whose disasm
calls it with a bare `call` and the operand object already in `ecx` (and which
ends in plain `ret`, no `ret N`) is `__thiscall` with `this` = that object — model
it as a method on the object's view struct, NOT as `Owner::Helper(Obj*)`. Doing so
for `RecomputePlaneCoords` (this = the CPlane, not CGameLevel) fixed both the
helper AND its two call sites in `LoadWwd` (matcher.md §4).
