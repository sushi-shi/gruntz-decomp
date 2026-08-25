# A dead trailing `p++` lets cl fold the previous increment into the last load

tags: cpp:local cpp:loop | asm:mov asm:add | topic:wall topic:regalloc
symptoms: a straight-line pointer walk (`m_a = *p++; m_b = *p++; ...`) where
retail keeps every `add r,4` as its own instruction and ours folds the LAST one
into the final load's displacement (`mov eax,[eax+0x4]` for retail's
`add eax,0x4; mov eax,[eax]`)
confidence: 8/10 (three sibling functions, one of them EXACT as the control)

## The shape

`CUniformTileImageSet::Parse` 0x166d40 reads a serialized tile-image record through a
cursor. Retail:

    mov eax,[esp+0x4]     ; record
    add eax,0x8           ; p = &record->m_width
    mov edx,[eax]
    add eax,0x4           ; increment 1
    mov [ecx+0x4],edx     ; m_width  = *p++
    mov edx,[eax]
    add eax,0x4           ; increment 2
    mov [ecx+0x8],edx     ; m_height = *p++
    mov eax,[eax]
    mov [ecx+0xc],eax     ; m_collisionValue = *p++   (increment 3 is dead)
    mov eax,0x1
    ret 0x4

Ours, from source that is already the pointer walk:

    ...
    mov edx,[eax]
    mov [ecx+0x8],edx
    mov eax,[eax+0x4]     ; increment 2, FOLDED into the load
    mov [ecx+0xc],eax

One instruction, 12 against 11, and the function reads 86.67%. The sibling
`CRectTileImageSet::Parse` 0x166990 has the identical residue over eight fields
(94.07%).

## Why, and the control that proves it

Both sides delete the trailing `p++` (nothing reads `p` after it). cl then sees
an `add` whose only remaining use is the final load and folds it into the
addressing mode. Retail did not, so retail's cursor was **not dead at that
load**.

The control is the third sibling. `CPixelTileImageSet::Parse` 0x166d70 uses the SAME
`READ_TILE_IMAGE_DIMENSIONS` macro and is **EXACT** — because it ends with
`memcpy(dst, p, m_byteSize)`, so its cursor is live past the last read and the
fold never becomes available. Same source idiom, opposite outcome, decided
entirely by whether the cursor dies at the final load.

## What does NOT reach it

Measured on 0x166d40, all byte-identical to the folded form:

* the macro hand-expanded into the function;
* the last read spelled `*p` instead of `*p++`;
* each increment as its own statement (`m_width = *p; ++p; ...`);
* an inline helper that reads the two dimensions and RETURNS the advanced
  cursor (`i32* p = ReadTileImageDimensions(this, record);`) — cl absorbs the
  inline's return temp completely.

So there is no source spelling of "walk a cursor and stop" that keeps the
increment; what retail had was a cursor still live afterwards. Park the row
unless a later reconstruction gives the record walk a real consumer.

## Detection signature

`walls residue` reads `displacement`, `base ['+0x4'] vs target ['']`, with a
`sub-object +0x4 over 1` note: one side's load carries the stride as a
displacement and the other side's carries it as a separate `add`. Confirm by
counting `add r,K` on both sides — equal counts minus one on our side, with the
missing one folded into the FINAL memory operand.
