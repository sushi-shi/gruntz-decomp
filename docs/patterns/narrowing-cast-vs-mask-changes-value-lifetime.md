# A narrowing conversion and an equal mask create different VC5 value lifetimes
tags: cpp:cast cpp:expr cpp:local | asm:and asm:push asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail walks the receiver first and then emits `mov reg,param; and reg,0xffff`, while the candidate emits the same mask before the walk, saves EBX, and carries an extra live value
confidence: 10/10

`x & 0xffff` and `static_cast<u16>(x)` produce the same numeric value for an
`i32` input, and cl 5.0 can lower both to the same `mov` plus `and 0xffff`
instructions. They are not equivalent inputs to the optimizer. The former is a
32-bit bitwise expression; the latter is a 16-bit value which is promoted at its
use. Their front-end value identities can therefore receive different
instruction ordinals and lifetimes even when the final instruction selection is
the same.

`CGruntzCmdMgr::EnqueuePlaceGruntAtScreenPoint` is the control. Retail first resolves the level
and view rectangle, keeps the level in ESI, and loads the two rectangle origins
into EAX/EDX. Only then does it load the packed X/Y inputs into EDI/ESI and mask
them with `0xffff`. The bitmask source instead scheduled X before the object
walk, forcing the level, view, and origin values to overlap and making EBX a
third saved register:

```cpp
// Extra EBX lifetime: 0x65 bytes, 40 instructions, 65.078950%.
i32 sx = ((vr->left - level->m_planeCtx.left + (x & 0xffff)) & ~TILE_MASK_PX)
    + TILE_HALF_PX;

// Retail lifetime: 0x64 bytes, 38 instructions, exact.
i32 sx = ((vr->left - level->m_planeCtx.left + static_cast<u16>(x)) & ~TILE_MASK_PX)
    + TILE_HALF_PX;
```

The conversion alone is not the whole reconstruction. Retail materializes
`&m_viewRect` with `add edx,0x40` and reads `left`/`top` through that pointer.
Using direct member expressions with the correct conversion removes EBX and
recovers the schedule, but folds the two loads to `[edx+0x40]`/`[edx+0x44]`:
0x62 bytes and 37 instructions. The short-lived `const RECT*` plus the unsigned
conversion closes both the address formation and the value lifetime.

Negative controls were bounded. Twelve reviewed expression, pointer, local,
association, and statement-splitting forms produced only three instruction
islands; none beat the 65.868420% direct-member mask form. A separate 66-cell
search over two syntax shapes and 33 TU states found one meaningful compiler
island and explicitly routed the residue back to structure. The call set, CFG,
constant set, and sole relocation to `EnqueueSingle` remained identical.

Reverse-use rule: when retail and candidate both visibly narrow an `i32` to the
low 16 bits but the candidate schedules the `and 0xffff` too early and gains a
callee-saved register, determine whether the source operation is a real
`u16`/`WORD` conversion rather than an integer mask. Retain the conversion only
when the value domain supports it; this is not a general register-steering cast.
