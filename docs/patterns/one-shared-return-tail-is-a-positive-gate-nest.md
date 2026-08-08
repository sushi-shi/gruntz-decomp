# N early `return 0`s collapsing into ONE tail means the source NESTS in positive gates

- **confidence**: 9/10
- **tags**: `cpp:branch` `cpp:goto` `cpp:eh` | `asm:jcc` `asm:xor` `asm:jmp` | `topic:codegen-idiom`
- **measured**: `CBattlezMapConfig::PathToNearestGoal` 0x30b20 72.81 -> **86.84** on this
  change alone (55.47 -> 86.84 with the two loop fixes); `CGrunt::StepArrivalDrop`
  0x4b370 32.84 -> 33.11 on the first of its gates

## Symptom

The recompile is logically right and the block KINDS line up, but every guard has the
opposite polarity from retail and the recompile is a few bytes longer at each one:

```
base:                              target:
  cmp  [esp+0x50],0x7fffffff         cmp  [esp+0x10],0x7fffffff
  jne  <skip>                        je   <RET0>            ; RET0 is the LAST block
  xor  eax,eax                       ...
  jmp  <epilogue>                    test eax,eax
<skip>:                              jne  <RET0>
  ...                                push 0xa               ; the body falls through
  test eax,eax
  je   <skip2>
  xor  eax,eax
  jmp  <epilogue>
```

Retail has exactly ONE `xor eax,eax` (and, when a destructible local is in scope, ONE
copy of its teardown) at the very end of the function; the recompile has one copy per
`return 0`. A failing arm that does work before returning — `PathToNearestCandidate(...)`,
`board->Clip(NULL)` — sits immediately ABOVE that tail and FALLS INTO it rather than
jumping.

## Reading

cl5 does not cross-jump these tails after the fact: their placement is decided by the
source's brace structure. A flat `if (bad) return 0;` ladder emits the `return 0` where
the guard is. Retail's shape is the nested positive form, with the failure work in an
`else` and one `return 0` at the bottom:

```cpp
// NO - one return-0 copy per guard, each inline at the guard
if (SearchEdge(...) == 0) { PathToNearestCandidate(unit, 1, bestX, bestY); return 0; }
if (list.GetCount() == 0) { return 0; }
head = list.RemoveHead(); ...
if (list.GetCount() == 0) { return 0; }
... ; return 1;

// YES - one return-0, and the else falls into it
if (SearchEdge(...) != 0) {
    if (list.GetCount() != 0) {
        head = list.RemoveHead(); ...
        if (list.GetCount() != 0) {
            ...
            return 1;
        }
    }
} else {
    PathToNearestCandidate(unit, 1, bestX, bestY);
}
return 0;
```

The earlier guards that run BEFORE a destructible local is constructed then cross-jump
into the same tail's `xor eax,eax`, skipping the teardown — which is why retail's
`je`/`jne` from those guards lands a few bytes PAST the destructor call.

A `goto` gate is the same story one level down: write
`if (cond == 0) goto elsewhere;` and let the success path fall through to its label,
never `if (cond) goto success; goto elsewhere; success:`.

## Distinguishing it from regalloc noise

The tell is not the polarity by itself — it is polarity PLUS the target being the
function's last block PLUS the recompile carrying duplicate `xor eax,eax`/teardown
copies. Count `xor eax,eax` on each side before rewriting: if both sides have one per
guard, the structure is already right and the difference is scheduling.

## Evidence (2026-08-08): the tail-CALL half

`CPlay::OnLButtonUp` @0xce530 **93.56 -> 100.00 EXACT** needed BOTH halves in one edit,
and either alone scores worse than the original:

1. the last statement is `return m_guts->OnPointerRelease(a, x, y);`, not
   `m_guts->OnPointerRelease(a, x, y); return 1;` — retail's final exit sets no `eax`
   at all, it pops straight off the call's return value (this alone took it to 69.31,
   because it left four duplicated `mov eax,1` epilogues around a now-different tail);
2. the four `return 1;` guards nest into positive gates with the box test spelled as a
   `||` chain, so every one of them `jcc`s to the single sunk `mov eax,1` tail:

```cpp
if (m_hudSuppressed == 0) {
    ...
    if (m_guts->m_position != STATUSBAR_HIDDEN) {
        LevelCoordRect vp = m_world->m_level->m_planeCtx;
        if (x < vp.left || x > vp.right || y < vp.top || y > vp.bottom) {
            return m_guts->OnPointerRelease(a, x, y);
        }
    }
}
return 1;
```

So when the *last* exit returns a callee's value, an early `return <literal>;` cannot
share with it — check the target's final `ret` block for a missing `mov eax,imm` before
you rewrite the gates.

## Related

- [[gate-falls-through-to-shared-latch]] — the same inversion for a single gate in front
  of a shared latch block.
- [[positive-gate-enables-shrink-wrap]] — the frame-level consequence.
- [[allocate-check-then-body-is-the-then-block]] — which arm is the fall-through names
  the if/else shape.
