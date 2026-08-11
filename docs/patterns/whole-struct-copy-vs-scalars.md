# A struct-shaped spill pattern says `T r = *p;`, but only where the frame is the ONLY residue

tags: cpp:struct cpp:local cpp:temporary | asm:mov asm:sub | topic:codegen-idiom topic:wall
symptoms: a function whose only diff is `sub esp,N` plus a handful of `[esp+M]`
displacements; retail spills a subset of a rectangle's fields and the spill slots are
spaced by the STRUCT stride (4 apart for adjacent fields, 8 or 12 apart for
non-adjacent ones) rather than packed contiguously; a member's unread field is written
anyway
confidence: 8/10

## The signal

cl 5.0 gives a struct local its whole size and lays its fields at their real offsets,
even when only two of them ever reach memory. It gives four separate `i32` locals only
as many dwords as it actually spills, packed. So the SPACING of the spill slots is a
direct read on which the source wrote:

```asm
; retail CPlay::OnKeyDown 0xccc3d - .top at +0x14, .bottom at +0x1c, EIGHT apart
mov  ebx,DWORD PTR [ecx]          ; .left   -> register
mov  edi,DWORD PTR [ecx+0x4]      ; .top
mov  DWORD PTR [esp+0x14],edi     ;         -> slot +4 of a RECT based at [esp+0x10]
mov  edi,DWORD PTR [ecx+0x8]      ; .right  -> register
mov  ecx,DWORD PTR [ecx+0xc]      ; .bottom
mov  DWORD PTR [esp+0x1c],ecx     ;         -> slot +0xc of that same RECT
```

Two used slots 8 apart inside a `sub esp,0x10` frame is a 16-byte object, not two
scalars - two scalars would sit at +0x10 and +0x14.

The other tell is a **dead field written anyway**. `CPlay::DrawDebugStatsFull` copies
`m_planeCtx` and then builds a second rect from three of its four fields; retail still
stores the unread `.top` (0xcf3d3 `mov [esp+0x30],edx`). Field-by-field assignment lets
cl dead-store it away; `RECT lr = *src;` does not.

```cpp
// what the field-by-field version loses
RECT lr;
lr.left = src->left; lr.top = src->top;      // <- cl deletes this one
lr.right = src->right; lr.bottom = src->bottom;

// retail's shape
RECT lr = *src;
```

## When it pays

Measured on `play`, three functions, all `LevelCoordRect`/`RECT` bounds tests:

| function | before | after |
|---|---|---|
| `CPlay::DrawDebugStatsFull` 0xcf0a0 | 94.72 | **100.00 EXACT** |
| `CPlay::OnLButtonDblClk` 0xce660 | 95.70 | 98.65 |
| `CPlay::OnKeyDown` 0xcbcc0 | 90.42 | **85.95 - REJECTED** |

DrawDebugStatsFull needed the copy *and* a block scope around the pair, which let cl
overlay the 16 bytes and took `sub esp,0x2a8 -> 0x298`, retail's exactly.

## When it does NOT - check the instruction COUNT first

`CPlay::OnKeyDown` has the textbook signal (frame 0x10 = one RECT, two spills 8 apart,
at BOTH of its two `m_planeCtx` bounds tests) and the change is still wrong. Writing
both as whole-struct copies - together with dropping the `m_mgr`/`m_guts` locals, which
is what stops cl spilling them and does give retail's `sub esp,0x8 -> 0x10` - moves the
frame onto retail's value and the block skeleton from 371 to 373 of 374, and yet:

```
                        instructions   in-order agreement vs retail
retail                       1993              -
four scalars + cached this   1990            47.5%
whole-struct + inlined this  1957            33.5%
```

The struct form makes cl keep MORE of the rectangle in registers, so it emits 33 fewer
instructions than retail across the function. The scalar form's instruction count is
already retail's to within three.

**So the frame is not the arbiter; the instruction count is.** Before taking a
frame-size or spill-spacing argument, measure in-order agreement over the whole
function (a `difflib.SequenceMatcher` over the masked instruction stream is enough) and
keep the shape whose COUNT matches. A frame that matches while the body sheds 2% of its
instructions is the lie scoring better than the truth, inverted.

Corollary, also measured: the lever is specific to the 16-byte rect. Converting two
`SIZE` (8-byte) field-by-field copies to struct assignment took
`CPlay::ClampViewport2` 91.27 -> 89.72 and left `CPlay::LoadScrollSpeedOptions` flat.
With only two fields there is no spacing to read, so there is no evidence either way -
do not apply it on shape alone.

variants: [block-scope-overlays-a-local-with-a-dead-temp.md](block-scope-overlays-a-local-with-a-dead-temp.md),
[plain-rect-vs-crect-assignment.md](plain-rect-vs-crect-assignment.md)
