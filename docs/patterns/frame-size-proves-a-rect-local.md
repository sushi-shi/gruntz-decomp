# Four scalars + a frame you cannot account for = a `RECT` local, and the SLOT ORDER proves it

tags: cpp:local cpp:struct cpp:rect | asm:sub asm:cmp | topic:codegen-idiom topic:correctness
symptoms: retail's `sub esp,N` is 16 bytes larger than anything your locals explain, and the
two or three slots it actually uses sit at +4 / +8 / +12 of that area rather than at the
bottom; or the slot ORDER of four related ints does not follow their declaration order
confidence: 9/10

A reconstruction naturally writes a hit box as four ints:

```cpp
i32 ylo = y - 7;
i32 yhi = y + 7;
i32 xlo = x - 7;
i32 xhi = x + 7;
```

cl register-allocates whichever of them it can and spills the rest into **whatever slots are
free, in first-spill order**. Retail instead puts them at a fixed +0/+4/+8/+12 inside a
16-byte frame area, in **left/top/right/bottom order regardless of the order the source
computes them**. That is not an allocation coincidence: it is a `RECT` (or `POINT` pair),
and the frame is the object.

Two ways to read it off:

**1. The frame size.** `CTriggerMgr::HitTestCell` 0x75af0 has `sub esp,0x10` and uses
exactly two of those four slots - `[S-0xc]` for `ylo` and `[S-4]` for `yhi`. Those are
`+top` and `+bottom` of a RECT based at `S-0x10`. Our version had `push ecx` (one 4-byte
local) and homed the same two values in dead PARAMETER slots instead. Writing

```cpp
RECT box;
box.top = y - 7;
box.bottom = y + 7;
box.left = x - 7;
box.right = x + 7;
...
if (box.left > ox + 14 || box.right < ox || box.top > oy + 14 || box.bottom < oy)
```

made the frame `sub esp,0x10` and closed the entire body: **86.24 -> 94.16**, everything
after the prologue byte-identical.

**2. The slot order, when the frame size already agrees.** `CTriggerMgr::CombatCue` 0x7b930
computes its four cue bounds in the order xLo, xHi, yLo, yHi but retail stores them to
`+0x28 / +0x30 / +0x2c / +0x34` - i.e. **xLo, yLo, xHi, yHi by ADDRESS**. Four independent
ints would be laid out in declaration order; left/top/right/bottom is a RECT whose fields
are assigned in the source's (x,x,y,y) order. Same rewrite, same slots.

## The neighbouring tell: named bound locals

When the bounds do NOT get a frame - they stay in registers - the same "the dev named it"
signal shows up in the SCHEDULE instead: retail computes both `+N` bounds adjacently, right
after the two base values, where a condition-inline `x0 + 30` is computed at its own
compare. `CTriggerMgr::CellHitTest` 0x6bea0:

```cpp
// retail computes lea ecx,[eax+0x1e] and lea esi,[edx+0x1e] back to back
i32 x0 = o->m_screenX - 15;
i32 y0 = o->m_screenY - 15;
i32 x1 = x0 + 30;
i32 y1 = y0 + 30;
if (px < x1 && px >= x0 && py < y1 && py >= y0) {
```

80.71 -> 87.78 with that plus walking the `startRow` parameter itself instead of a copy
(see outparam-through-the-parameter-slot.md).

## Read it BACKWARDS too: a 4-byte frame proves there is NO aggregate temp

The inference runs both ways, and the negative direction is the cheaper find because
`sub esp,N` is the first instruction of the function. `CStatusBarMgr::UpdateChipGrinder-
StatusBar` 0x1076a0 allocates its whole frame with a bare **`push ecx`** - four bytes -
so nothing 16-byte-wide lives in it. Our reconstruction had

```cpp
CSBI_ImageSet* w = m_extraNotify1;
if (w) {
    RECT rc;                        // 16 bytes cl cannot elide: the aggregate
    i32 sx = m_rect10.left;         // assignment below reads it as a whole
    rc.left = m_fallRect.left + sx;
    ...
    w->m_rect14 = rc;
}
```

and paid `sub esp,0x14` for it. Storing the four coordinates through the owner —
`w->m_rect14.left = m_fallRect.left + sx;` and so on — removed the frame and closed
the body: **83.15 -> 87.81**, instruction counts equal at 139, residue only the R4
coin between the hoisted zero and a state literal.

**This falsifies an earlier note on that function** which claimed retail's `add obj,0x14`
anchor "exists only under aggregate IL - field-by-field never anchors". Field-by-field
member stores reproduce the anchor and the frame; the note has been removed. When a
comment asserts that only one IL shape can produce an addressing form, check the frame
size before believing it.

Do not sweep this: the sibling `UpdateFallingItemStatusBar` 0x107590 keeps its `RECT rc`
because retail has `sub esp,0x10` there and spills `rc.left`.

## Related

- [member-aggregate-copied-not-field-by-field.md](member-aggregate-copied-not-field-by-field.md)
- [plain-rect-vs-crect-assignment.md](plain-rect-vs-crect-assignment.md)
- [outparam-through-the-parameter-slot.md](outparam-through-the-parameter-slot.md)
