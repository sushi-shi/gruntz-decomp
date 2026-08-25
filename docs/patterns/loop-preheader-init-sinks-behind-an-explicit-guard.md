# Retail forms the loop's pointers AFTER the trip-count guard - wrap the loop in `if (n > 0)`
tags: cpp:loop cpp:local cpp:branch | asm:add asm:test asm:jle | topic:codegen-idiom
symptoms: base adds the buffer base to the cursors BEFORE `test <n>,<n> / jle`, target adds them after; base indexes with SIB `[idx+base]` where target walks two plain pointers; equal branch sequence, equal size, tens of points apart
confidence: 8/10

A `for` loop whose induction pointers are initialised in statements *above* the loop
gets those initialisers emitted **before** cl's trip-count guard, because the
declarations are not part of the loop. Retail emits them **inside** the guarded
region — its `jle` skips them — which is what cl does when the loop is nested in an
explicit `if (n > 0)`. The tell is the position of the `add <cursor>,<base>` pair
relative to the guard, not the loop body.

Do NOT "fix" it by moving the addition into the loop body (indexing off the base
each iteration): cl then keeps the base in a register and emits SIB
`mov word ptr [idx+base],..`, which retail does not have — that scores higher and is
further from the target.

```cpp
// BEFORE - cl adds `base` before `test/jle`
char* lp = static_cast<char*>(base) + lo;
char* rp = static_cast<char*>(base) + ro;
for (i32 v = 0; v < h; v++) { *Pix16(lp) = c; *Pix16(rp) = c; lp += step; rp += step; }

// AFTER - the adds land in the loop preheader, behind the guard
if (h > 0) {
    char* lp = static_cast<char*>(base) + lo;
    char* rp = static_cast<char*>(base) + ro;
    i32 v = h;
    while (v != 0) { *Pix16(lp) = c; *Pix16(rp) = c; lp += step; rp += step; v--; }
}
```
```asm
    test   ecx,ecx
    jle    <end>
    mov    edx,DWORD PTR [esp+0x1c]   ; base - INSIDE the guard
    add    edi,edx
    add    edx,esi
<loop>:
    mov    WORD PTR [edx],bp
    mov    WORD PTR [edi],bp
    add    edx,eax
    add    edi,eax
    dec    ecx
    jne    <loop>
```
STEERABLE. `CMinimap::DrawBorderRaw` 0xa3a20 67.64 -> 71.02 with the `if`
wrapper alone, -> 72.07 with the countdown `while` (a `do..while(--h)` gives 71.84,
the offset-indexed form 71.10, a `u16*` cursor form 71.02). A 400-cell AST forest over
the same body moved nothing, so pick the shape from the guard position, not the search.
