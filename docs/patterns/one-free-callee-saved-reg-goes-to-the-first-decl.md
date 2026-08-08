# With ONE callee-saved register left, the FIRST-declared local wins it — and a second variable for the same value costs you the coalesce
tags: cpp:local | asm:mov asm:sar | topic:codegen-idiom topic:regalloc
symptoms: frame is one dword SMALLER than retail's (`push ecx` vs `sub esp,8`), the two locals that compete for the last callee-saved register are swapped (ours spills the one retail enregisters), an extra 1-2 instruction block appears where retail falls straight through, a value retail keeps in `ebp` we re-load from the stack at every use
confidence: 9/10

When a body has exactly one free callee-saved register (`ebx`/`esi`/`edi` already taken
by params and `this`) and **two** locals live across the same call, one gets the register
and the other gets a fresh frame dword. Two source properties decide it, and both are
visible in the frame size:

1. **Declaration order picks the winner.** `i32 limit = ...; i32 coord = destX;` gave
   `limit` -> `ebp` and spilled `coord` into a dead parameter slot (so the frame needed no
   new local, `push ecx`). Swapping to `i32 coord = destX; i32 limit = ...;` gave `coord`
   -> `ebp` and spilled `limit` into a real local (`sub esp,8`) — which is what retail has.
   **A frame that is one dword too small is the tell that the wrong local won.**

2. **A second variable for the same value blocks the coalesce.** With
   `i32 mid; ... mid = (hi + lo) / 2; ... destX = mid;` cl put `mid` in `eax` and grew an
   extra `mov eax,[esp+N]` block on the path that leaves `mid` unassigned. Writing the
   result back into the SAME local (`coord = (hi + lo) / 2; ... destX = coord;`) let cl
   coalesce it with the register `coord` already owns, and the extra block disappeared:
   `mov ebp,eax; sar ebp,1` exactly as retail.

`CGameLevel::MoveRising` 0x15e4b0: **77.00 -> 92.61 -> 96.61 -> 100.00 EXACT** in those
three steps (the first was companion 3 below).

## Companion: cl sinks an initialiser into the `else` — so write the `else`

Retail's `mid`/`coord` is initialised from `destX` **only** on the not-taken side of
`if (moveFlags & 0x10)`, as a redundant reload (`mov ebp,[esp+0x20]`) of a value the
register already holds. `i32 mid = destX;` before the `if` hoists it and cl then proves
the else-path needs nothing; the shape that reproduces retail is

```cpp
i32 coord = destX;
...
if (moveFlags & 0x10) {
    i32 lo = coord, hi = coord;
    if (ClampSpan(coord, limit, &lo, &hi) != 0) {
        coord = (hi + lo) / 2;
    }
} else {
    coord = destX;          // redundant to a reader; retail emits the reload
}
```

Nothing else moved it: the value is genuinely re-loaded from the parameter's home slot on
that edge. This is the same family as
[uninitialized-local-reserves-frame-dword.md](uninitialized-local-reserves-frame-dword.md) —
where an initialiser lives is a codegen knob, not a style choice.

related: [decl-order-and-assign-order-are-two-knobs.md](decl-order-and-assign-order-are-two-knobs.md), [pin-local-for-callee-saved-reg.md](pin-local-for-callee-saved-reg.md), [frame-size-counts-the-locals.md](frame-size-counts-the-locals.md), [one-use-local-is-a-regalloc-knob.md](one-use-local-is-a-regalloc-knob.md)
