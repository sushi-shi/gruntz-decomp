# Writing a contiguous output block: struct-copy, not field-by-field

**Tags:** `cpp:struct cpp:member cpp:local | asm:lea asm:mov | topic:codegen-idiom`
**Confidence:** 9/10 — recovered on CImage sprite blitters (0x1538c0, 0x154270)
and CWwdGrid::Setup (0x1915c0).

## Symptom

A function fills a run of adjacent member fields (e.g. a `RECT`/4-int block at
`obj+0x20`) at the end. Retail loads all the source values, then stores them
through a **base register**:

```
lea  edi, [esi+0x20]
mov  [edi],     edx      ; +0x20
mov  [edi+0x4], ebp      ; +0x24
mov  [edi+0x8], eax      ; +0x28
mov  [edi+0xc], ecx      ; +0x2c
```

Your recompile instead emits the stores with the **full displacement**, one
load+store at a time (`mov [esi+0x20],eax; mov eax,[esp+..]; mov [esi+0x24],..`),
interleaved with the other member writes — a 10–15 instruction divergence even
though the values are identical.

## Cause

Field-by-field assignment (`obj->m_20 = a; obj->m_24 = b; …`), especially with
arithmetic on some fields (`obj->m_28 = v - 1;`), is seen by MSVC5 as independent
stores: it schedules each load next to its store and never materializes a base
register. A **single struct-copy statement** is recognized as a block move and
lowered to `lea base,[obj+N]` + `mov [base+k]` — matching retail, which clearly
copied a struct/rect into the member.

## Fix

Model the output run as a struct member and assign it with ONE struct copy:

```cpp
typedef RECT BlitRect;
class CBlitInfo { …; BlitRect m_20; … };         // at +0x20

info->m_20 = d;                                   // one typed block store
```

### Off-by-one on right/bottom: adjust the SOURCE in place, don't compute per-field

The surface blitters pass an **exclusive** rect to BltEx (`d.right+1`) but record
the **inclusive** rect (`d.right`). Don't write `info->m_20.m_08 = d.right - 1;`
(that breaks the block move back into per-field stores). Instead decrement the
source rect back in place **after** the call, then struct-copy — the source RECT
is dead afterward, so MSVC5 does the `dec` in-register and folds it into the copy
(no stack write-back), exactly matching retail's `dec eax; dec ecx; mov [base+8]`:

```cpp
((CDDSurface*)dst->m_2c)->BltEx(&d, …);  // d = {l,t,r+1,b+1}
d.right  -= 1;                            // back to inclusive; d now dead
d.bottom -= 1;
info->m_18 = d.left;                      // the separate top-left point
info->m_1c = d.top;
info->m_20 = d;                          // grouped base-reg store
```

This took CImage::BlitNorm 97.5%→98.6% and CImage::BlitShadeNorm (inclusive, plain
`info->m_20 = d`) to 99.85%. Adjusting fields after the copy
(`info->m_20 = …; info->m_20.m_08 = d.right-1;`) is WORSE — it leaves dead +1
stores that MSVC5 does not eliminate.

### When the two structs have different semantic field names, give the owner a typed view

Do not use a cast merely because two proven layouts agree. If retail copies a by-value
`RECT` into a four-coordinate member through one `lea`d base, but the member's users name
the fields `m_minX/m_minY/m_maxX/m_maxY`, model both readings as an anonymous union:

```cpp
struct WwdRect {
    union {
        struct { i32 m_minX, m_minY, m_maxX, m_maxY; };
        RECT m_rect;
    };
};

m_bounds.m_rect = rect;
```

The union is the storage model; the assignment remains fully typed. In
`CWwdGrid::Setup`, replacing a temporary assembled field-by-field with this one assignment
made retail's entire four-load/four-store block agree. Mutating the dead by-value `rect`
itself for the two min/max swaps, instead of copying its fields into four scalar locals,
then reproduced retail's register rotation and subtraction sequence. Together those two
structural changes moved 82.79% to 92.14%.

The remaining residue there is not evidence against the view: six local-result spellings,
nine allocation-exit spellings, and 17 eligible mixed TU states did not move its dead
parameter homes or MSVC's one-vs-two EH-epilogue choice.
