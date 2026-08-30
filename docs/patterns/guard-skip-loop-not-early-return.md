# A mid-function guard before a loop: write `if (cond) { loop }` (fall through to the single return), NOT `if (!cond) return acc;`

tags: cpp:branch cpp:return cpp:loop | asm:jcc asm:jmp asm:ret | topic:codegen-idiom
confidence: 8/10
variants: identical-return-epilogue-tailmerge.md, nested-if-success-deepest-error-tail.md, void-vs-bool-return-epilogue-split.md

## Symptom

A function accumulates a result, then has a guard that can skip the rest of the
work and return the accumulator-so-far, then a loop, then `return acc;`. Spelled as
an **early return**:

```cpp
int acc = 0;
... ;
if (lo > hi) {          // guard
    return acc;         // early-out
}
do { ... } while (...);
return acc;
```

the recompile emits a SECOND full pop/ret epilogue at the guard site, where retail
emits a single `jg <shared-exit>` that jumps to the one `mov eax,[acc]; pop…; ret`
tail. The body is otherwise byte-identical, but the duplicated epilogue + the
slightly different live-range around it drag a clean function down (~95%).

## Fix — invert the guard so it SKIPS the loop and falls through to the one return

```cpp
int acc = 0;
... ;
if (lo <= hi) {         // guard now GATES the work
    do { ... } while (...);
}
return acc;             // the SINGLE exit both paths reach
```

Now the guard is a `jg`/`jcc` over the loop to the shared `return acc;` — exactly
retail's `jg <shared-exit>`. No duplicate epilogue. The accumulator's live range
also matches (it stays in its [esp] slot / register across the skipped loop instead
of being finalized at two sites).

Evidence: `CWwdGrid::Query` (0x1918c0) 95.0%→99.7% on this single rewrite — the
early `if (colA > colB) return fired;` (which duplicated the
`mov eax,[esp+0x10]; pop…; ret 0x14` tail) became `if (colA <= colB) { <nested
loops> }` with the lone `return fired;` after. The residual 0.3% is pure
addressing-mode/schedule entropy (`lea ebp,[ecx*8+0]` vs `[8*ecx]`).

This is the mirror of identical-return-epilogue-tailmerge (where the recompile
*tail-merges* and that's a wall): here the recompile *duplicates* and the C
spelling controls it. Use it whenever a guard sits BEFORE a loop and returns a
running accumulator — gate the loop, don't early-return.

## It is not about the loop, and not about an accumulator (2026-08-23)

The loop and the accumulator are incidental. What the lever needs is only that
**the source names the same return value twice** and the two sites can be reduced
to one. `CDDrawSurfacePair::SetGeom` (0x164250) had no loop and returns a literal:

```cpp
if (m_width != w || m_height != h || m_bpp != bpp) {
    ... ;
    if (formatOk) { <7 stores>; return 1; }     // <- return #1
    return 0;
}
return 1;                                        // <- return #2
```

cl declined to merge the two `return 1`s, so it laid the guard's own
`mov eax,1 / pop ebx / ret 0xc` inline at 0x25 and jumped the body past it -
three `jne 0x2d`. Retail has one `return 1` at the tail and therefore the textbook
`||` lowering, `jne BODY / jne BODY / je TAIL`. Negating the inner check into an
early `return 0` and letting the stores fall through to the single trailing
`return 1`:

```cpp
    if (!formatOk) { return 0; }
    <7 stores>;
}
return 1;
```

**78.53 -> 84.03**, and the first 0x3e bytes became identical to retail apart from
the tail displacement. So the detection signature is the branch POLARITY of the
last term of an `||`/`&&` guard chain: retail's last term reads `je <far tail>`
where ours reads `jne <near body>`, which says retail's guard falls through to a
return the source states once and ours states twice. `gruntz walls jccscan` names
these rows directly - a balanced single je/jne flip.

### The check that stops this from being over-applied

`retail jumps FAR where we jump NEAR` at ONE site is not enough. Confirm retail
has **no inline epilogue of its own** for the neighbouring guards, because cl also
picks *which* of several identical returns to keep inline, and that choice is
placement, not a return count.

`CTriggerMgr::UseEquippedToolAt` 0x6dae0 is the trap. Two adjacent guards, both
`return -1`:

```cpp
if (o->m_screenX != cell->m_lastTilePx.m_x) { return -1; }
if (o->m_screenY != cell->m_lastTilePx.m_y) { return -1; }
```

The first reads `je <near>` for us and `jne <far 0x1d5>` for retail, which is the
signature above - but retail then gives the SECOND guard its own inline
`or eax,0xffffffff / pop.. / ret 0x10` at 0x9c. Retail duplicates too; it just
duplicates the other one. Merging the two into `if (A || B) return -1;` gave both
guards the far tail and cost **87.45 -> 86.86**. Reverted.

So: look at the guards on BOTH sides of the flip before reaching for the lever.
Retail holding an inline epilogue anywhere in the neighbourhood means the return
count already agrees and only the placement differs, which is
[identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md)'s
wall.

The bounded follow-up found one source correction short of that final merge.
Keeping the Y guard and the later `RectContains` failure as ordinary early
returns, but gating the complete body on the positive X comparison, moves
`UseEquippedToolAt` **87.4484 -> 87.6802**. Hoisting the two real `CellHitTest`
output locals to function scope composes to **88.1407**. A second positive gate
around the `RectContains` success arm does reproduce retail's 52 branches, 15
returns, 20 calls, 37 ordered relocations, and `0x14` frame, but recolours the
whole function and lands at 86.3626. That lower state is useful structural
evidence, not a spelling to commit by itself; the bankable partial correction
keeps 16 returns and leaves the last tail-placement question open.

## One source return can still produce two machine epilogues — and recolour the whole function

`CRezImage::PasteFrom` 0x176960 is the composition control.  It has two mutually
exclusive copy loops and returns the clipped height.  The early-return spelling

```cpp
if (src->m_transparent) {
    // transparent copy
    return h;
}
// opaque copy
return h;
```

already had retail's two `ret` instructions, so return COUNT alone did not expose the
source mistake.  Writing the two loops as sibling `if/else` arms with one trailing
`return h` kept two machine epilogues but changed the value lifetimes and homes across
both arms, moving **84.69 -> 92.67** and making both copy loops instruction-identical to
retail.  The remaining eight-byte clipping prelude required two ordinary locals:

```cpp
i32 dstW = m_width;
i32 dstH = m_height;
```

used by the horizontal and vertical clamps.  That local pair on the early-return base
was a misleading descent (**79.55**, 358 bytes); on the correct single-return `if/else`
base it produced retail's 360-byte object and **100.00 exact**.  Reversing only the two
destination declarations missed by one instruction at 99.90.

This is the explicit composed-search rule for this pattern: when the body has mutually
exclusive arms that return the same value, test one source return even if the retail
`ret` count already agrees.  Then re-test authentic locals whose isolated form moved a
retail feature but spilled or overgrew the baseline; branch structure can completely
change whether C2 homes those values.
